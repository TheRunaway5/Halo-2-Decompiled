#!/usr/bin/env python3
"""Real loopback HTTP, actual native Curl; provider identity is a fixture."""
import argparse
import hashlib
import http.server
import json
import os
from pathlib import Path
import subprocess
import threading


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'), 'Use the owned test lifecycle'
    report = {'passed': False, 'scope': 'Native account controller, not external authentication or Xbox Live', 'cases': []}
    try:
        for name, binary, expected in [('before', args.reference, 0), ('rust', args.candidate, 1)]:
            calls = []

            class Handler(http.server.BaseHTTPRequestHandler):
                def log_message(self, *unused):
                    pass

                def respond(self):
                    body = self.rfile.read(int(self.headers.get('Content-Length', '0')))
                    assert self.path == '/v1/account/session'
                    if self.command == 'POST':
                        assert json.loads(body) == {'screen_name': 'Curl Pilot'}
                        assert self.headers.get('Authorization') == 'Bearer native-fixture-oauth'
                        status, value = 201, {'token': 'curl-private-owned-grant', 'expires_in': 900}
                    else:
                        assert self.command == 'DELETE'
                        assert self.headers.get('Authorization') == 'Bearer curl-private-owned-grant'
                        status, value = 200, {}
                    calls.append(self.command)
                    encoded = json.dumps(value).encode()
                    self.send_response(status)
                    self.send_header('Content-Type', 'application/json')
                    self.send_header('Content-Length', str(len(encoded)))
                    self.end_headers()
                    self.wfile.write(encoded)

                do_POST = respond
                do_DELETE = respond

            server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                origin = f'http://127.0.0.1:{server.server_port}'
                command = [str(binary), '--http'] + (['--expect-before'] if name == 'before' else [])
                result = subprocess.run(command, env=dict(os.environ, HALO2_ACCOUNT_CLIENT_HTTP_ORIGIN=origin),
                                        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=20)
                assert result.returncode == 0, f'{name} native Curl scenario failed: {result.stderr[-2000:]}'
                assert calls.count('POST') == 1 and calls.count('DELETE') == expected, f'{name} completed-grant ownership differed: {calls}'
                report['cases'].append({'owner': name, 'http_calls': calls,
                                        'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
                                        'stdout': result.stdout, 'exit_code': result.returncode})
            finally:
                server.shutdown()
                thread.join(timeout=3)
                server.server_close()
        report['passed'] = True
    finally:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS real native Curl: original leaks late complete grant; Rust retires it with uncancelled DELETE')


if __name__ == '__main__':
    main()
