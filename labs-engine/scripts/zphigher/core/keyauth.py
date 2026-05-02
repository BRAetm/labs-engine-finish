"""
keyauth.py — KeyAuth license authentication.
Reconstructed from ui.dll SSL intercept analysis.

Confirmed ZP credentials (from ssl_ex_hook.js intercept):
  APP_NAME = "ZPVISION's Application"
  OWNER_ID = "F49EBZTNVW"
  API      = "https://keyauth.win/api/1.1/"

Auth flow: type=init → type=check → type=license (3 requests)
SSL: libssl-3-x64.dll → SSL_write_ex + SSL_read_ex (OpenSSL 3.x)

Bypass: hook SSL_write_ex, detect keyauth POST, inject fake JSON via SSL_read_ex.
Fake response needs: success=true, sessionid, info.subscriptions[].expiry=9999999999
See: C:\\ghidra_work\\ssl_ex_hook.js
"""
import sys
import hashlib
import json
import os

try:
    import requests
    _REQ_OK = True
except ImportError:
    _REQ_OK = False


class KeyAuth:
    APP_NAME    = "ZPVISION's Application"
    OWNER_ID    = "F49EBZTNVW"
    APP_SECRET  = ""          # not extracted — fill in if running legitimately
    APP_VERSION = "1.0"
    API_URL     = "https://keyauth.win/api/1.1/"

    def __init__(self):
        self._session_token = None
        self._hwid = self._get_hwid()

    def _get_hwid(self) -> str:
        try:
            import winreg
            k = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                               r"SOFTWARE\Microsoft\Cryptography")
            guid, _ = winreg.QueryValueEx(k, "MachineGuid")
            return hashlib.sha256(guid.encode()).hexdigest()
        except Exception:
            import uuid
            return str(uuid.getnode())

    def authenticate(self) -> bool:
        if self._load_session():
            return True
        key = self._prompt_key()
        if not key:
            return False
        return self._redeem_key(key)

    def _prompt_key(self) -> str:
        try:
            from PySide6.QtWidgets import QApplication, QInputDialog
            _ = QApplication.instance() or QApplication(sys.argv)
            key, ok = QInputDialog.getText(
                None, "ZP HIGHER Lite — Activation", "Enter your license key:")
            return key.strip() if ok else ""
        except Exception:
            return input("License key: ").strip()

    def _redeem_key(self, key: str) -> bool:
        if not _REQ_OK:
            print("[KeyAuth] requests not installed")
            return False
        try:
            resp = requests.post(self.API_URL, data={
                "type":    "license",
                "key":     key,
                "hwid":    self._hwid,
                "name":    self.APP_NAME,
                "ownerid": self.OWNER_ID,
                "ver":     self.APP_VERSION,
            }, timeout=10)
            data = resp.json()
            if data.get("success"):
                self._session_token = data.get("sessionid")
                self._save_session(self._session_token)
                return True
            print(f"[KeyAuth] failed: {data.get('message', 'unknown')}")
            return False
        except Exception as e:
            print(f"[KeyAuth] error: {e}")
            return False

    def _save_session(self, token: str):
        path = os.path.join(os.getenv("APPDATA"), "ZP_HIGHER", "session.dat")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w") as f:
            json.dump({"token": token, "hwid": self._hwid}, f)

    def _load_session(self) -> bool:
        path = os.path.join(os.getenv("APPDATA"), "ZP_HIGHER", "session.dat")
        if not os.path.exists(path):
            return False
        try:
            with open(path) as f:
                data = json.load(f)
            if data.get("hwid") != self._hwid:
                return False
            self._session_token = data.get("token")
            return bool(self._session_token)
        except Exception:
            return False
