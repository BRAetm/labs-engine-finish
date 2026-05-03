#!/usr/bin/env python3
"""
Labs marketplace token validator.

Single endpoint:
    GET /api/validate?token=<t>&slug=<s>
        200 {"valid": true,  "scope": "*"|"slug"}     — token authorizes <slug>
        403 {"valid": false, "reason": "..."}          — token unknown / not authorized
        400 {"valid": false, "reason": "..."}          — missing params

Token store is a flat JSON file at TOKENS_PATH (env override) or ./tokens.json.
The file is re-read on every request so creators can edit it without bouncing
the service. For v1 this is a workable trade-off: the file is small, traffic
is low, and edits land instantly.

Run behind Nginx as the auth_request target for /downloads/. The download
location passes the original ?token=... and the script slug (resolved via a
map_directive or rewrite — see nginx.conf) so this endpoint can answer with
a fixed set of inputs.

Run standalone (dev):
    pip install flask
    python validate.py            # binds 127.0.0.1:8787
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from threading import Lock

from flask import Flask, jsonify, request


TOKENS_PATH = Path(os.environ.get("LABS_TOKENS_PATH", "tokens.json"))
BIND_HOST   = os.environ.get("LABS_VALIDATE_HOST", "127.0.0.1")
BIND_PORT   = int(os.environ.get("LABS_VALIDATE_PORT", "8787"))

app = Flask(__name__)
_io_lock = Lock()


def _load_tokens() -> list[dict]:
    with _io_lock:
        if not TOKENS_PATH.exists():
            return []
        try:
            return json.loads(TOKENS_PATH.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            return []


def _authorize(token: str, slug: str) -> tuple[bool, str]:
    for entry in _load_tokens():
        if entry.get("token") != token:
            continue
        slugs = entry.get("slugs") or []
        if "*" in slugs:
            return True, "*"
        if slug in slugs:
            return True, "slug"
        return False, "token-not-authorized-for-slug"
    return False, "unknown-token"


@app.route("/api/validate", methods=["GET"])
def validate():
    token = request.args.get("token", "").strip()
    slug  = request.args.get("slug",  "").strip()
    if not token or not slug:
        return jsonify(valid=False, reason="missing-token-or-slug"), 400

    ok, reason = _authorize(token, slug)
    if not ok:
        return jsonify(valid=False, reason=reason), 403
    return jsonify(valid=True, scope=reason), 200


@app.route("/healthz", methods=["GET"])
def healthz():
    return jsonify(ok=True, tokens=len(_load_tokens())), 200


if __name__ == "__main__":
    app.run(host=BIND_HOST, port=BIND_PORT, debug=False)
