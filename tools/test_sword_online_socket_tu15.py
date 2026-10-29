#!/usr/bin/env python3
"""Canonical sword capture over real, privately admitted ONLINE socket peers.

The normal local master/signaling/quality policy runs against two owned native
OnlineMatchSession processes. Only external Discord verification and the finite
two-owner playlist are fixtures. No public service or account is contacted.
Raw descriptors/logs/database remain inside the enclosing run_test.py owner.
"""
import argparse
import asyncio
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import uuid
from collections.abc import Mapping

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, os.environ.get("H2_QUALIFICATION_MASTER_ROOT", str(ROOT)))
from aiohttp import web
from aiohttp.test_utils import TestClient, TestServer
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.tests.test_matchmaking_http import catalog


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--shared-maps", type=Path)
    parser.add_argument("--mode", action="append", choices=("lunge-host", "lunge-client",
        "sword-rapid-near-host", "sword-rapid-near-client", "sword-rapid-far-host", "sword-rapid-far-client",
        "sword-close-host", "sword-close-client", "sword-cancel-neutral-host",
        "sword-cancel-neutral-client", "sword-cancel-jump-host", "sword-cancel-jump-client",
        "sword-cancel-switch-host", "sword-cancel-switch-client", "sword-flight-swap-host",
        "sword-flight-swap-client"))
    parser.add_argument("--timeout-seconds", type=float, default=240)
    parser.add_argument("--report-output", type=Path)
    return parser.parse_args()


def bounded_log(path):
    data = path.read_bytes()
    return dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
        tail=data[-48 * 1024:].decode(errors="replace"), truncated=len(data) > 48 * 1024)


def plain_data(value):
    """Copy immutable native value views into the bounded JSON receipt."""
    if isinstance(value, Mapping):
        return {str(key): plain_data(item) for key, item in value.items()}
    if isinstance(value, (tuple, list)):
        return [plain_data(item) for item in value]
    return value


def save(destination, report):
    if destination is None:
        return
    data = json.dumps(report, indent=2) + "\n"
    if len(data.encode()) > 1024 * 1024:
        raise RuntimeError("Owned ONLINE socket report exceeds the1MiB bounded-report contract")
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(data)


async def one_case(args, owner, mode):
    case = owner / mode
    case.mkdir()
    fixture = catalog()
    # Explicit custom validation offering: no claim about downloaded Xbox
    # hopper contents, rotation or timing. Real measured host policy remains.
    fixture["playlists"] = [next(row for row in fixture["playlists"] if row["id"] == "casual")]
    fixture["playlists"][0].update(minimum_players=2, maximum_players=2, maximum_party=1)
    selected = case / "playlists.json"
    selected.write_text(json.dumps(fixture))
    verified = []
    requests = dict(signal=0, relay=0)
    room_id = None
    live_coordinator = None

    def observe_coordinator(application):
        nonlocal live_coordinator
        if room_id is None:
            return
        api = next(iter(application.values())).matchmaking
        try:
            room = api.transport.rooms[room_id]
            measured = room.measured_hosts()
            if not measured or not room.direct_udp:
                return
            elected = min(measured, key=measured.__getitem__)
            live_coordinator = dict(host=room.host, direct_udp=room.direct_udp,
                elected_host=room.index(elected), measured_host_score=list(measured[elected]),
                reciprocal_direct_reports=plain_data(list(room.direct_ready.values())),
                observed_before_native_room_retirement=True)
        except (Problem, KeyError):
            # The actual coordinator retires the setup when its final native
            # endpoint leaves. Keep only an already completed live observation.
            return

    async def verify(session, token, application):
        if token not in ("sword-owned-one", "sword-owned-two") or application != "1558522073272426519":
            raise Problem(401, "Local sword fixture authorization denied")
        verified.append(token)
        username = "sword_owned_two" if token == "sword-owned-two" else "sword_owned_one"
        return VerifiedOwner("discord", str(123456789012345678 + (token == "sword-owned-two")), username)

    @web.middleware
    async def observe(request, handler):
        if request.path.endswith("/signal"):
            requests["signal"] += 1
        if request.path.endswith("/relay"):
            requests["relay"] += 1
        observe_coordinator(request.app)
        response = await handler(request)
        observe_coordinator(request.app)
        return response

    app = create_app(Settings(state=case, public_url="http://127.0.0.1:8080", allow_http=True,
        rate_per_minute=100000, matchmaking_catalog=str(selected)), verify)
    app.middlewares.insert(0, observe)
    result = dict(mode=mode, passed=False, network_scope="same-host actual process/socket peers",
        raw_storage="enclosing managed scratch", external_account_authentication="local verification fixture")
    process = None
    log_path = case / "native.log"
    transport_path = case / "transport.json"
    try:
        async with TestClient(TestServer(app)) as client:
            headers = []
            for index, token in enumerate(("sword-owned-one", "sword-owned-two")):
                response = await client.post("/v1/matchmaking/session",
                    headers={"Authorization": "Bearer " + token}, json=dict(
                        screen_name="Owned Sword " + str(index + 1), maps=["lockout"], language=0,
                        compatibility="halo2-native-queue-v12"))
                if response.status != 201:
                    raise RuntimeError("Local sword session admission failed: " + await response.text())
                headers.append({"Authorization": "Bearer " + (await response.json())["token"]})
            for header in headers:
                response = await client.post("/v1/matchmaking/ticket", headers=header,
                    json=dict(playlist_id="casual", revision="lab-1", request_id=str(uuid.uuid4())))
                if response.status != 201:
                    raise RuntimeError("Local sword queue admission failed: " + await response.text())
            descriptors = []
            for header in headers:
                response = await client.get("/v1/matchmaking/transport", headers=header)
                if response.status != 200:
                    raise RuntimeError("Local sword transport admission failed: " + await response.text())
                descriptors.append(await response.json())
            if {row["slot"] for row in descriptors} != {0, 1} or descriptors[0]["id"] != descriptors[1]["id"]:
                raise RuntimeError("Actual local sword room did not admit exactly two independent owners")
            room_id = descriptors[0]["id"]
            paths = {}
            for d in descriptors:
                path = case / ("descriptor-" + str(d["slot"]) + ".json")
                path.write_text(json.dumps(d))
                path.chmod(0o600)
                paths[d["slot"]] = path
            env = os.environ | dict(HALO2_SWORD_ONLINE_ORIGIN=str(client.make_url("/")).rstrip("/"),
                HALO2_SWORD_ONLINE_DESCRIPTOR_0=str(paths[0]), HALO2_SWORD_ONLINE_DESCRIPTOR_1=str(paths[1]),
                HALO2_SWORD_ONLINE_REPORT=str(transport_path), H2_NET_PROFILE="complete",
                H2_EOS_PROFILES="0", HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
            command = [str(args.binary), str(args.map), str(args.shared_maps), "native-mcc", str(case / "source"), mode]
            result["command"] = command
            # Inherit the enclosing run_test.py process group. Its supervisor
            # therefore stops both native endpoint processes on cancellation.
            # The exec child additionally uses a parent-death SIGTERM lease.
            with log_path.open("wb") as output:
                process = await asyncio.create_subprocess_exec(*command, stdout=output,
                    stderr=asyncio.subprocess.STDOUT, env=env)
                try:
                    await asyncio.wait_for(process.wait(), args.timeout_seconds)
                except asyncio.TimeoutError:
                    result["timeout"] = True
                    process.kill()
                    await process.wait()
            result["native_exit_code"] = process.returncode
            result["native_log"] = bounded_log(log_path)
            if process.returncode != 0:
                raise RuntimeError("Actual ONLINE socket sword capture failed")
            if not transport_path.is_file() or transport_path.stat().st_size > 32 * 1024:
                raise RuntimeError("Native capture omitted its bounded actual transport evidence")
            transport = json.loads(transport_path.read_text())
            if transport.get("schema") != "H2_TU15_OWNED_ONLINE_SOCKET_V1":
                raise RuntimeError("Unexpected actual socket evidence schema")
            for endpoint in transport["endpoints"]:
                if endpoint["submitted_packets"] <= 0 or endpoint["received_packets"] <= 0:
                    raise RuntimeError("Native gameplay did not traverse both actual ONLINE socket endpoints")
            for endpoint in (transport["local"], transport["remote"]):
                if endpoint["phase"] != 2 or not endpoint["direct_udp"] or not endpoint["master_connected"]:
                    raise RuntimeError("Native gameplay did not retain its real ONLINE Running/direct admission")
                if len(endpoint["quality"]) != 2 or any(row["upload_bytes_per_second"] <= 0 for row in endpoint["quality"]):
                    raise RuntimeError("Actual ONLINE quality evidence is missing a measured endpoint")
            if live_coordinator is None:
                raise RuntimeError("No completed live coordinator observation preceded native room retirement")
            if live_coordinator["elected_host"] != live_coordinator["host"]:
                raise RuntimeError("The local sword room did not preserve its actual measured host election")
            if transport["host"] != live_coordinator["host"] or requests["relay"] or requests["signal"] < 2:
                raise RuntimeError("Native sword gameplay escaped its real direct-signaling room path")
            if set(verified) != {"sword-owned-one", "sword-owned-two"}:
                raise RuntimeError("Both owned local authentication fixtures were not exercised")
            result.update(passed=True, transport=transport, coordinator=live_coordinator | dict(
                signaling_requests=requests["signal"], gameplay_relay_requests=requests["relay"]))
    except Exception as error:
        result["failure"] = str(error)
        if log_path.is_file():
            result["native_log"] = bounded_log(log_path)
    finally:
        if process is not None and process.returncode is None:
            process.kill()
            await process.wait()
    print(("PASS " if result["passed"] else "FAIL ") + mode + ": actual owned ONLINE session sockets", flush=True)
    if not result["passed"]:
        print(result.get("failure", "Unknown native failure"), flush=True)
        print(result.get("native_log", {}).get("tail", ""), flush=True)
    return result


async def main():
    args = arguments()
    if not sys.platform.startswith("linux"):
        raise RuntimeError("Owned separate-process sword socket qualification currently requires Linux")
    if not (0 < args.timeout_seconds <= 600):
        raise ValueError("--timeout-seconds requires a finite budget no greater than600")
    args.binary = args.binary.resolve(strict=True)
    args.map = args.map.resolve(strict=True)
    args.shared_maps = (args.shared_maps or args.map.parent).resolve(strict=True)
    if args.report_output is not None and not args.report_output.is_absolute():
        raise ValueError("--report-output must name an explicit absolute bounded-report path")
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    report = dict(schema="H2_TU15_OWNED_ONLINE_SWORD_V1", passed=False, cases=[],
        limits=["Two owned processes on one host, actual ICE/DTLS/SCTP/UDP sockets and real local signaling/quality policy.",
            "External account verification and the finite two-owner custom playlist are local fixtures.",
            "No public account, matchmaking service, WAN, NAT traversal or packet-impairment claim.",
            "Client native codec remains on the parent executor; the remote session/socket owner is an independent exec process."])
    with tempfile.TemporaryDirectory(prefix="sword-online-sockets-", dir=scratch) as temporary:
        owner = Path(temporary)
        for mode in args.mode or ["lunge-host", "lunge-client"]:
            report["cases"].append(await one_case(args, owner, mode))
            save(args.report_output, report)
        report["passed"] = all(row["passed"] for row in report["cases"])
        save(args.report_output, report)
    report["private_coordinator_and_descriptors_removed"] = not owner.exists()
    save(args.report_output, report)
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
