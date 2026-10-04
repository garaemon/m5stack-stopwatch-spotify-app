#!/usr/bin/env python3
"""Obtain a Spotify refresh token with the PKCE flow and store it in include/secrets.h.

Usage:
    python3 tools/spotify_auth.py

The script reads SPOTIFY_CLIENT_ID from include/secrets.h, opens the Spotify
consent page in a browser, and receives the callback on 127.0.0.1:8888.
"""

import base64
import hashlib
import http.server
import json
import pathlib
import re
import secrets
import sys
import urllib.parse
import urllib.request
import webbrowser

REDIRECT_URI = "http://127.0.0.1:8888/callback"
SCOPES = [
    "user-read-currently-playing",
    "user-read-playback-state",
    "user-modify-playback-state",
    "user-library-read",
    "user-library-modify",
]
SECRETS_PATH = pathlib.Path(__file__).resolve().parent.parent / "include" / "secrets.h"


def read_client_id(secrets_text: str) -> str:
    match = re.search(r'#define\s+SPOTIFY_CLIENT_ID\s+"([^"]+)"', secrets_text)
    if match is None:
        sys.exit(f"SPOTIFY_CLIENT_ID not found in {SECRETS_PATH}")
    return match.group(1)


def create_pkce_pair() -> tuple[str, str]:
    code_verifier = secrets.token_urlsafe(64)
    digest = hashlib.sha256(code_verifier.encode("ascii")).digest()
    code_challenge = base64.urlsafe_b64encode(digest).rstrip(b"=").decode("ascii")
    return code_verifier, code_challenge


def wait_for_authorization_code(expected_state: str) -> str:
    received_query: dict[str, list[str]] = {}

    class CallbackHandler(http.server.BaseHTTPRequestHandler):
        def do_GET(self) -> None:
            received_query.update(
                urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            )
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write(b"Authorization received. You can close this tab.")

        def log_message(self, *args: object) -> None:
            pass

    with http.server.HTTPServer(("127.0.0.1", 8888), CallbackHandler) as server:
        server.handle_request()
    if received_query.get("state", [""])[0] != expected_state:
        sys.exit("State mismatch in the OAuth callback")
    if "code" not in received_query:
        sys.exit(f"Authorization failed: {received_query.get('error', ['unknown'])[0]}")
    return received_query["code"][0]


def exchange_code_for_tokens(client_id: str, code: str, code_verifier: str) -> dict:
    form = urllib.parse.urlencode(
        {
            "grant_type": "authorization_code",
            "code": code,
            "redirect_uri": REDIRECT_URI,
            "client_id": client_id,
            "code_verifier": code_verifier,
        }
    ).encode("ascii")
    request = urllib.request.Request(
        "https://accounts.spotify.com/api/token",
        data=form,
        headers={"Content-Type": "application/x-www-form-urlencoded"},
    )
    with urllib.request.urlopen(request) as response:
        return json.load(response)


def write_refresh_token(secrets_text: str, refresh_token: str) -> None:
    updated_text, replacement_count = re.subn(
        r'(#define\s+SPOTIFY_REFRESH_TOKEN\s+)"[^"]*"',
        lambda match: f'{match.group(1)}"{refresh_token}"',
        secrets_text,
    )
    if replacement_count == 0:
        sys.exit(
            f"SPOTIFY_REFRESH_TOKEN not found in {SECRETS_PATH}; add this line:\n"
            f'#define SPOTIFY_REFRESH_TOKEN "{refresh_token}"'
        )
    SECRETS_PATH.write_text(updated_text)


def main() -> None:
    secrets_text = SECRETS_PATH.read_text()
    client_id = read_client_id(secrets_text)
    code_verifier, code_challenge = create_pkce_pair()
    state = secrets.token_urlsafe(16)
    authorize_url = "https://accounts.spotify.com/authorize?" + urllib.parse.urlencode(
        {
            "client_id": client_id,
            "response_type": "code",
            "redirect_uri": REDIRECT_URI,
            "scope": " ".join(SCOPES),
            "code_challenge_method": "S256",
            "code_challenge": code_challenge,
            "state": state,
        }
    )
    print(f"Open this URL if the browser does not start:\n{authorize_url}\n")
    webbrowser.open(authorize_url)
    code = wait_for_authorization_code(state)
    tokens = exchange_code_for_tokens(client_id, code, code_verifier)
    write_refresh_token(secrets_text, tokens["refresh_token"])
    print(f"Wrote SPOTIFY_REFRESH_TOKEN to {SECRETS_PATH}")


if __name__ == "__main__":
    main()
