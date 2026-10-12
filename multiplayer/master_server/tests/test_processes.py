"""Two real master processes, independent SQLite banks and an abrupt simulator exit."""
import os
import socket
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path

from multiplayer.master_server.cli import Instance, ApiError, initialize_demo, enroll, request, discover
from .test_registry import DATA

ROOT = Path(__file__).resolve().parents[3]


def free_port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def eventually(function, timeout=7):
    deadline = time.monotonic() + timeout
    last = None
    while time.monotonic() < deadline:
        try:
            result = function()
            if result:
                return result
        except (OSError, ApiError) as error:
            last = error
        time.sleep(0.05)
    raise AssertionError(f"Bounded convergence timeout ({last})")


class ProcessTests(unittest.TestCase):
    def test_two_masters_failure_recovery_and_simulator_expiration(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            a, b = folder / "a", folder / "b"
            a_url, b_url = (f"http://127.0.0.1:{free_port()}" for _ in range(2))
            identities = initialize_demo(a, b, a_url, b_url)
            processes = []
            logs = []
            def start(state, url):
                log = (state / "test-process.log").open("a")
                logs.append(log)
                env = os.environ | {"H2_MASTER_STATE": str(state), "H2_MASTER_BIND": "127.0.0.1",
                     "H2_MASTER_PORT": url.rsplit(":", 1)[1], "H2_MASTER_PUBLIC_URL": url,
                     "H2_MASTER_ALLOW_HTTP": "1", "H2_MASTER_ADMIN_TOKEN_FILE": str(state / "admin-token"),
                     "H2_MASTER_LEASE_SECONDS": "4", "H2_MASTER_SYNC_SECONDS": "0.15"}
                process = subprocess.Popen([sys.executable, "-m", "multiplayer.master_server"], cwd=ROOT,
                                           env=env, stdout=log, stderr=log)
                processes.append(process)
                eventually(lambda: request(url, "GET", "/v1/health")["ready"])
                return process
            try:
                first, second = start(a, a_url), start(b, b_url)
                instance = Instance(folder / "simulator.json")
                token = (a / "admin-token").read_text().strip()
                enroll(instance, a_url, token, True)
                instance.send("POST", data=DATA)
                eventually(lambda: len(request(a_url, "GET", "/v1/servers")["servers"]) == 1)
                eventually(lambda: len(request(b_url, "GET", "/v1/servers")["servers"]) == 1)
                instance.send("PATCH", data={"map": "ascension", "players": 4})
                eventually(lambda: request(b_url, "GET", "/v1/servers")["servers"][0]["map"] == "ascension")
                instance.send("POST", "/heartbeat")
                first.kill(); first.wait(timeout=3)
                fallback = discover([a_url, b_url])
                self.assertEqual(b_url, fallback["master_used"])
                self.assertEqual(instance.value["server_id"], fallback["servers"][0]["server_id"])
                self.assertTrue(request(b_url, "GET", "/v1/health")["ready"])
                first = start(a, a_url)
                self.assertEqual(identities["a"]["id"], request(a_url, "GET", "/v1/health")["master_id"])
                instance.send("POST", "/heartbeat")
                eventually(lambda: request(a_url, "GET", "/v1/servers")["servers"] == request(b_url, "GET", "/v1/servers")["servers"])
                instance.send("DELETE")
                eventually(lambda: request(b_url, "GET", "/v1/servers")["servers"] == [])
                data_path = folder / "metadata.json"
                import json
                data_path.write_text(json.dumps(DATA))
                simulator_log = (folder / "simulator.log").open("w")
                logs.append(simulator_log)
                simulator = subprocess.Popen([sys.executable, "-m", "multiplayer.master_server.cli", "run", "--identity", str(instance.path),
                    "--metadata", str(data_path), "--allow-http", "--heartbeat", "0.25", "--update-every", "0.25",
                    "--maps", "lockout", "ascension", "--gametypes", "Slayer", "Team Slayer", "--crash-after", "2.5"],
                    cwd=ROOT, stdout=simulator_log, stderr=simulator_log)
                processes.append(simulator)
                eventually(lambda: bool(request(b_url, "GET", "/v1/servers")["servers"]))
                first.kill(); first.wait(timeout=3)
                time.sleep(0.4)
                self.assertIsNone(simulator.poll(), "Simulator must survive its origin becoming unavailable")
                self.assertEqual(b_url, discover([a_url, b_url])["master_used"])
                first = start(a, a_url)
                eventually(lambda: bool(request(a_url, "GET", "/v1/servers")["servers"]) and
                           request(a_url, "GET", "/v1/servers")["servers"] == request(b_url, "GET", "/v1/servers")["servers"])
                self.assertEqual(23, simulator.wait(timeout=5))
                eventually(lambda: request(a_url, "GET", "/v1/servers")["servers"] == [] and
                                   request(b_url, "GET", "/v1/servers")["servers"] == [], timeout=7)
                # Persisted signed expired snapshots continue to sync, without renewal.
                time.sleep(0.4)
                self.assertEqual([], request(b_url, "GET", "/v1/servers")["servers"])
                self.assertIsNone(first.poll()); self.assertIsNone(second.poll())
            finally:
                for process in processes:
                    if process.poll() is None:
                        process.terminate()
                for process in processes:
                    process.wait(timeout=7)
                for log in logs:
                    log.close()
