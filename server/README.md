# Labs marketplace backend

Self-hosted script registry. Static files via Nginx, token validation via a
tiny Flask service. JSON-flat token store, no DB, no user accounts.

## Files

| File                     | Role                                                |
|--------------------------|-----------------------------------------------------|
| `marketplace/manifest.json` | Served at `/marketplace/manifest.json` (public) |
| `marketplace/downloads/public/`  | Public script files                        |
| `marketplace/downloads/locked/`  | Locked script files (gated by token)       |
| `tokens.json`            | Token → slug mapping. `"*"` = master key            |
| `validate.py`            | Flask validator on `127.0.0.1:8787`                 |
| `nginx.conf`             | Site config — `auth_request` for locked downloads   |
| `labs-validate.service`  | systemd unit for the validator                      |

## Deploy

```bash
# 1. Lay out files on the VPS
sudo mkdir -p /var/www/marketplace/downloads/{public,locked}
sudo cp -r marketplace/* /var/www/marketplace/
sudo cp tokens.json validate.py /var/www/marketplace/
sudo chown -R www-data:www-data /var/www/marketplace

# 2. Install Python deps for the validator
sudo apt install -y python3-flask

# 3. Wire up systemd
sudo cp labs-validate.service /etc/systemd/system/
sudo systemctl enable --now labs-validate

# 4. Wire up Nginx
sudo cp nginx.conf /etc/nginx/sites-available/labs-marketplace
sudo ln -sf /etc/nginx/sites-available/labs-marketplace /etc/nginx/sites-enabled/
sudo nginx -t && sudo systemctl reload nginx

# 5. (Recommended) TLS via certbot
sudo certbot --nginx -d marketplace.example.com
```

## Adding a script

1. Drop the file into `downloads/public/<slug>.<ext>` (or `downloads/locked/<slug>.zip`).
2. Append an entry to `marketplace/manifest.json` with the matching `slug`,
   `download` URL, and `is_private` flag.
3. For locked entries, add a token in `tokens.json`:
   ```json
   { "token": "send-this-to-the-buyer", "slugs": ["my-script-slug"] }
   ```
   No restart needed — `validate.py` re-reads the file on every request.

## Client flow (Labs Engine)

1. Client GETs `https://<host>/marketplace/manifest.json`.
2. Renders cards. Locked entries (`is_private: true`) show a lock badge and
   an "Enter access key" field instead of the Install button.
3. User pastes a key — client stores it under `marketplace/keys/<slug>` in
   `SettingsManager`.
4. Install request: `GET <download_url>?token=<stored_key>`.
5. Nginx fires `auth_request` to `/api/validate` with the slug + token.
   200 → file streams to the client. 403 → install fails with "invalid key".

## Notes

- `tokens.json` is read-on-every-request. Edits are live; no restart.
- Validator binds to `127.0.0.1` only. Public traffic always goes through
  Nginx's `auth_request`, never directly to the validator.
- Master keys (`"slugs": ["*"]`) are convenient for testing and for "all-access"
  keys you might sell. Rotate the demo master key before going live.
