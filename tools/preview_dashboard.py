#!/usr/bin/env python3
"""Serve the dashboard locally with Firebase Hosting's clean URLs and rewrites."""
import argparse
import json
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
HOSTING = json.loads((ROOT / 'firebase.json').read_text())['hosting']
PUBLIC = ROOT / HOSTING['public']
REWRITES = {rule['source']: rule['destination'] for rule in HOSTING.get('rewrites', [])
            if '*' not in rule['source'] and 'destination' in rule}


class PreviewHandler(SimpleHTTPRequestHandler):
    def translate_path(self, path):
        route = unquote(urlsplit(path).path)
        if route in REWRITES:
            return super().translate_path(REWRITES[route])
        resolved = super().translate_path(path)
        if HOSTING.get('cleanUrls') and not Path(resolved).exists():
            html = Path(resolved + '.html')
            if html.is_file():
                return str(html)
        return resolved


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8765)
    args = parser.parse_args()
    handler = partial(PreviewHandler, directory=str(PUBLIC))
    server = ThreadingHTTPServer(('127.0.0.1', args.port), handler)
    print(f'Dashboard preview: http://127.0.0.1:{args.port}', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        server.server_close()
