"""Exercise the compiled C++ client against two real signed/federated masters."""
import json
import gzip
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import threading

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
try:
    import aiohttp  # The process integration requires the server runtime too.
    from multiplayer.master_server.cli import Instance, enroll, initialize_demo, request
except ImportError:
    print("Install multiplayer/master_server/requirements.txt for real HTTP integration")
    sys.exit(77)


def port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def eventually(function):
    end = time.monotonic() + 7
    while time.monotonic() < end:
        try:
            if function():
                return
        except (OSError, RuntimeError):
            pass
        time.sleep(0.05)
    raise AssertionError("Master convergence timed out")


def main():
    binary = str(Path(sys.argv[1]).resolve())
    mode = "empty"
    paths = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def do_GET(self):
            paths.append(self.path)
            raw = b'{"servers":[]}'
            self.send_response(302 if mode == "redirect" else 200)
            if mode == "redirect":
                self.send_header("Location", "/forbidden")
            if mode == "gzip":
                raw = gzip.compress(raw)
                self.send_header("Content-Encoding", "gzip")
            if mode == "oversized":
                raw = b" " * (4 * 1024 * 1024 + 1)
            self.send_header("Content-Length", str(len(raw)))
            self.end_headers()
            try:
                self.wfile.write(raw)
            except (BrokenPipeError, ConnectionResetError):
                pass

    fixture = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=fixture.serve_forever, daemon=True)
    thread.start()
    url = f"http://127.0.0.1:{fixture.server_port}"
    try:
        env = os.environ | {"http_proxy": "http://127.0.0.1:1", "HTTP_PROXY": "http://127.0.0.1:1", "NO_PROXY": "", "no_proxy": ""}
        result = subprocess.run([binary, "--probe", url], capture_output=True, text=True, timeout=7, env=env)
        assert result.returncode == 0 and json.loads(result.stdout) == [], "Ambient proxy affected discovery"
        for mode in ("redirect", "gzip", "oversized"):
            paths.clear()
            result = subprocess.run([binary, "--probe", url], capture_output=True, text=True, timeout=7)
            assert result.returncode != 0 and paths == ["/v1/servers"], f"Transport accepted {mode} or followed another endpoint"
    finally:
        fixture.shutdown()
        fixture.server_close()
        thread.join(timeout=2)
    with tempfile.TemporaryDirectory(prefix="halo2-native-discovery-") as tmp:
        folder = Path(tmp)
        a, b = folder / "a", folder / "b"
        first_url, second_url = (f"http://127.0.0.1:{port()}" for _ in range(2))
        initialize_demo(a, b, first_url, second_url)
        processes, logs = [], []

        def start(state, url):
            log = (state / "process.log").open("a")
            logs.append(log)
            env = os.environ | {"H2_MASTER_STATE": str(state), "H2_MASTER_BIND": "127.0.0.1", "H2_MASTER_PORT": url.rsplit(":", 1)[1],
                  "H2_MASTER_PUBLIC_URL": url, "H2_MASTER_ALLOW_HTTP": "1", "H2_MASTER_ADMIN_TOKEN_FILE": str(state / "admin-token"),
                  "H2_MASTER_LEASE_SECONDS": "4", "H2_MASTER_SYNC_SECONDS": "0.15"}
            process = subprocess.Popen([sys.executable, "-m", "multiplayer.master_server"], cwd=ROOT, env=env, stdout=log, stderr=log)
            processes.append(process)
            eventually(lambda: request(url, "GET", "/v1/health")["ready"])
            return process

        def native(*urls):
            return json.loads(subprocess.check_output([binary, "--probe", *urls], cwd=ROOT, timeout=10, text=True))

        try:
            first = start(a, first_url)
            start(b, second_url)
            data = json.loads((ROOT / "multiplayer/master_server/examples/server.json").read_text())
            instance = Instance(folder / "simulated-instance.json")
            enroll(instance, first_url, (a / "admin-token").read_text().strip(), True)
            instance.send("POST", data=data)
            eventually(lambda: len(request(second_url, "GET", "/v1/servers")["servers"]) == 1)
            rows = native(first_url, second_url)
            assert rows[0]["name"] == data["name"] and rows[0]["map"] == data["map"]
            instance.send("PATCH", data={"players": 4, "map": "ascension", "gametype": "Team Slayer"})
            eventually(lambda: request(second_url, "GET", "/v1/servers")["servers"][0]["players"] == 4)
            first.kill()
            first.wait(timeout=3)
            rows = native(first_url, second_url)
            assert rows[0]["players"] == 4 and rows[0]["map"] == "ascension" and rows[0]["gametype"] == "Team Slayer"
            # With no further heartbeats, the backup must not keep the crashed origin alive.
            eventually(lambda: request(second_url, "GET", "/v1/servers")["servers"] == [])
            assert native(first_url, second_url) == []
            first = start(a, first_url)
            assert native(first_url, second_url) == []
            instance.send("POST", data=data)
            assert native(first_url, second_url)[0]["name"] == data["name"]
            instance.send("DELETE")
            assert native(first_url, second_url) == []
            print("PASS C++ HTTP discovery: proxy/redirect/compression/size boundaries, signed registration, metadata updates, federated failover, expiry, recovery and removal")
        finally:
            for process in processes:
                if process.poll() is None:
                    process.terminate()
            for process in processes:
                process.wait(timeout=7)
            for log in logs:
                log.close()


if __name__ == "__main__":
    main()
