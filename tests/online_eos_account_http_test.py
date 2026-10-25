"""Loopback native account-auth worker; no successful credential/cloud writes.

Requires an actual SDK-enabled executable and private untrusted client config.
All config outputs, private room admission and SDK cache belong to run_test.
"""
import asyncio
import json
import os
from pathlib import Path
import secrets
import sys
import time
import uuid

from aiohttp import web, WSMsgType
from aiohttp.test_utils import TestServer


async def main():
    assert len(sys.argv) == 3, "Supply native executable and case"
    binary = str(Path(sys.argv[1]).resolve())
    mode = sys.argv[2]
    assert mode in ("denied", "unavailable", "stalled")
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    assert scratch.is_dir() and scratch.name.startswith("halo2-test-")
    owned = scratch / ("account-http-" + mode)
    owned.mkdir()
    descriptor = dict(room=str(uuid.uuid4()), bearer=secrets.token_urlsafe(32),
                      socket=secrets.token_hex(16), peer_key=secrets.token_hex(32))
    private = owned / "admission.json"
    private.write_text(json.dumps(descriptor))
    private.chmod(0o600)
    route = "/v1/matches/" + descriptor["room"]
    requests, controls = [], []
    auth_started, socket_open, release_http = asyncio.Event(), asyncio.Event(), asyncio.Event()
    stalled_body_pending = asyncio.Event()
    held_requests = 0
    pending_transports = []
    protocol_error = False

    def authenticated(request):
        return request.headers.get("Authorization") == "Bearer " + descriptor["bearer"]

    async def issuer(request):
        nonlocal held_requests, protocol_error
        valid = (authenticated(request) and request.content_type == "application/json"
                 and await request.read() == b"{}")
        protocol_error |= not valid
        if not valid:
            return web.json_response({"error": "Fixture request admission mismatch"}, status=400)
        requests.append(time.monotonic())
        auth_started.set()
        if mode == "denied":
            return web.json_response({"error": "Fixture account admission revoked"}, status=401)
        if mode == "unavailable":
            return web.json_response({"error": "Fixture issuer temporarily unavailable"}, status=503)
        response = web.StreamResponse(status=200, headers={"Content-Type": "application/json"})
        await response.prepare(request)
        held_requests += 1
        pending_transports.append((request.transport, time.monotonic()))
        stalled_body_pending.set()
        try:
            # No credential bytes or EOF are supplied. Only cancellation can
            # finish the client's in-progress HTTP operation promptly.
            await release_http.wait()
        finally:
            held_requests -= 1
        return response

    async def proof(request):
        nonlocal protocol_error
        protocol_error = True
        return web.json_response({"error": "No SDK proof is possible in this fixture"}, status=400)

    async def signal(request):
        nonlocal protocol_error
        if not authenticated(request):
            protocol_error = True
            return web.Response(status=401)
        socket = web.WebSocketResponse()
        await socket.prepare(request)
        socket_open.set()

        async def ping():
            while not socket.closed:
                await socket.send_json({"ping": "0123456789abcdef"})
                await asyncio.sleep(.05)

        heartbeat = asyncio.create_task(ping())
        try:
            async for message in socket:
                if message.type == WSMsgType.TEXT:
                    value = json.loads(message.data)
                    assert isinstance(value, dict)
                    # Record only control names, never private signal data.
                    controls.extend(value.keys())
                elif message.type == WSMsgType.BINARY:
                    protocol_error = True
        finally:
            heartbeat.cancel()
            await asyncio.gather(heartbeat, return_exceptions=True)
        return socket

    app = web.Application()
    app.router.add_post(route + "/eos-token", issuer)
    app.router.add_post(route + "/eos-proof", proof)
    app.router.add_get(route + "/signal", signal)
    env = os.environ.copy()
    env.pop("H2_EOS_DIAGNOSTICS", None)
    env.update(H2_EOS_CACHE_DIR=str(owned / "eos-cache"),
               XDG_DATA_HOME=str(owned / "data"), XDG_CONFIG_HOME=str(owned / "config"),
               XDG_CACHE_HOME=str(owned / "cache"), SDL_AUDIODRIVER="dummy",
               HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
    process = None
    stderr_task = None

    async def discard_output(stream):
        while await stream.read(65536):
            pass

    async with TestServer(app) as server:
        try:
            process = await asyncio.create_subprocess_exec(binary,
                str(server.make_url("/")).rstrip("/"), str(private), mode,
                env=env, stdin=asyncio.subprocess.PIPE, stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE)
            stderr_task = asyncio.create_task(discard_output(process.stderr))

            async def read():
                # The real SDK can write its own stdout banner. Only the
                # fixture's explicitly tagged numeric/boolean messages enter
                # this protocol; discard all arbitrary SDK output.
                async with asyncio.timeout(12):
                    for _ in range(256):
                        line = await process.stdout.readline()
                        assert line, "Native worker ended without a bounded result"
                        if not line.startswith(b"H2EOSAUTHFIXTURE "):
                            continue
                        value = json.loads(line[len(b"H2EOSAUTHFIXTURE "):])
                        assert isinstance(value, dict)
                        return value
                raise AssertionError("Native worker exceeded the diagnostic message bound")

            async def command(action):
                process.stdin.write((json.dumps({"action": action}) + "\n").encode())
                await process.stdin.drain()
                return await read()

            assert (await read())["constructed"]
            await asyncio.wait_for(auth_started.wait(), 10)
            if mode == "denied":
                evidence = await command("denial")
                assert evidence["typed_server_denial"]
            else:
                await asyncio.wait_for(socket_open.wait(), 10)
                if mode == "unavailable":
                    async with asyncio.timeout(5):
                        while len(requests) < 2:
                            await asyncio.sleep(.01)
                    assert requests[1] - requests[0] >= .2, "Native retries bypassed Rust backoff"
                else:
                    await asyncio.wait_for(stalled_body_pending.wait(), 5)
                    assert held_requests >= 1, "HTTP body was not actually pending"
                before = await command("snapshot")
                assert before["connecting"] and not before["failed"] and before["master_connected"]
                await asyncio.sleep(.15)
                evidence = await command("snapshot")
                assert evidence["sdk_ticks"] > before["sdk_ticks"], "HTTP held the SDK/transport mutex"
                assert not evidence["failed"] and evidence["master_connected"]
                assert evidence["host_candidates"] == 0
                assert "pong" in controls, "Control socket stopped servicing while auth HTTP waited"
                assert "peer_ready" not in controls and "peer_signal" not in controls, "Pre-ACK identity announced QoS or a peer signal"
            pending_age_ms = None
            if mode == "stalled":
                # A handler can remain held after Curl's 3s timeout closed its
                # connection. Require a genuinely fresh still-live request at
                # stop, leaving >2s of HTTP timeout to distinguish cancellation.
                async with asyncio.timeout(5):
                    while True:
                        live = [(transport, started) for transport, started in pending_transports
                                if transport is not None and not transport.is_closing()]
                        if len(live) == 1 and time.monotonic() - live[0][1] < .4:
                            pending_age_ms = int((time.monotonic() - live[0][1]) * 1000)
                            break
                        await asyncio.sleep(.01)
                assert pending_age_ms is not None and pending_age_ms < 400
            stopped = await command("stop")
            assert stopped["stopped"] and stopped["teardown_ms"] < 2000
            assert await asyncio.wait_for(process.wait(), 5) == 0, "Native worker or transport teardown failed"
            for values in (evidence, stopped):
                assert all(values[key] == 0 for key in
                    ("device_calls", "login_calls", "create_user_calls", "socket_calls"))
            assert not protocol_error and requests
            assert "peer_ready" not in controls and "peer_signal" not in controls
            result = dict(passed=True, case=mode, native=evidence, teardown=stopped,
                          issuer_requests=len(requests), private_bearer_and_empty_body_verified=True,
                          pre_ack_peer_ready=0, pre_ack_peer_signals=0,
                          live_stalled_request_age_ms_at_stop=pending_age_ms,
                          no_external_credential_or_cloud_user_write=True,
                          kind2_proof_http_exercised=False,
                          scope="Actual native Session, Rust auth, licensed SDK polling and loopback HTTP/WebSocket ownership; no account grant, gameplay or WAN acceptance")
            (owned / "result.json").write_text(json.dumps(result, indent=2) + "\n")
            print("EOS_ACCOUNT_HTTP_RESULT " + json.dumps(result, sort_keys=True))
            print("PASS native EOS account HTTP " + mode + ": private room request, Rust admission, pre-ACK QoS gate and cancellable worker lifetime")
            print("LIMIT denied/unavailable issuer only; no successful SDK account grant or kind2 proof HTTP, no playable session")
        finally:
            release_http.set()
            if process and process.returncode is None:
                process.kill()
                await process.wait()
            if stderr_task:
                # SDK output is discarded incrementally, not retained/echoed.
                await stderr_task


if __name__ == "__main__":
    asyncio.run(main())
