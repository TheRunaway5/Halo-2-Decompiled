#!/usr/bin/env python3
"""Qualify the opt-in Rust/Caddy route in run_test.py-owned storage.

This is host process/TLS qualification. It does not start Docker, publish a
service, replace a selected runtime, or assert genuine provider/played parity.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import time
import traceback
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
CADDY_VERSION = "2.11.7"
CADDY_ARCHIVE_SHA512 = "a7a433a1b133efc3c8d10eb0b99d52a24b5ef5c322dc77f5282182b1c0402139ab83f3a99f0c52409df77d20123fb0b523edad8a66d8f5e49136197bf61ef0e7"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def redact_failure(value):
    # aiohttp exception reprs include request headers, even for private
    # fixtures. Keep status/stack diagnostics without retaining bearer grants.
    return re.sub(r"(?i)(bearer\s+)[^\s'\"<>),]+", r"\1<REDACTED>", value)


def download(url, limit):
    request = urllib.request.Request(url, headers={"User-Agent": "Halo2-private-delivery-qualification"})
    with urllib.request.urlopen(request, timeout=45) as response:
        data = response.read(limit + 1)
    if len(data) > limit:
        raise RuntimeError("Pinned dependency download exceeded its bound")
    return data


def caddy(scratch, report):
    if sys.platform != "linux" or os.uname().machine != "x86_64":
        raise RuntimeError("This delivery cohort declares Linux amd64 only")
    base = f"https://github.com/caddyserver/caddy/releases/download/v{CADDY_VERSION}/"
    name = f"caddy_{CADDY_VERSION}_linux_amd64.tar.gz"
    checksums = download(base + f"caddy_{CADDY_VERSION}_checksums.txt", 256 * 1024)
    matches = [line.split()[0] for line in checksums.decode().splitlines() if line.split()[-1] == name]
    if matches != [CADDY_ARCHIVE_SHA512]:
        raise RuntimeError("Pinned Caddy archive is missing a unique release checksum")
    archive = scratch / name
    archive.write_bytes(download(base + name, 100 * 1024 * 1024))
    if hashlib.sha512(archive.read_bytes()).hexdigest() != CADDY_ARCHIVE_SHA512:
        raise RuntimeError("Pinned Caddy release checksum mismatch")
    with tarfile.open(archive) as bundle:
        member = bundle.getmember("caddy")
        if not member.isfile() or member.size > 100 * 1024 * 1024:
            raise RuntimeError("Unexpected Caddy release archive")
        stream = bundle.extractfile(member)
        binary = scratch / "caddy"
        binary.write_bytes(stream.read())
    binary.chmod(0o700)
    version = subprocess.check_output([str(binary), "version"], text=True).strip()
    if not version.startswith("v" + CADDY_VERSION + " "):
        raise RuntimeError("Unexpected Caddy executable version")
    report["caddy"] = dict(version=version, archive_url=base + name,
                           checksum_url=base + f"caddy_{CADDY_VERSION}_checksums.txt",
                           archive_sha512=CADDY_ARCHIVE_SHA512, archive_sha256=digest(archive),
                           binary_sha256=digest(binary), retained=False)
    return binary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "build/test-results/rust-matchmaking-delivery")
    parser.add_argument("--legacy-state-only", action="store_true",
                        help="Qualify populated legacy state backup, Rust adoption and host process rollback")
    parser.add_argument("--owned-controls-retry-file", type=Path,
                        help="Private investigation only: wait at most10min after a control failure for an explicit hash-pinned fixture refresh")
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = scratch / "source"
    source.mkdir()
    for relative in ("rust/online_protocol", "rust/online_security_ffi", "rust/matchmaking_service", "multiplayer/master_server"):
        shutil.copytree(ROOT / relative, source / relative,
                        ignore=shutil.ignore_patterns("target", "__pycache__", "*.so", ".local", ".env", ".env.*"))
    for name in ("test_rust_matchmaking_delivery.py", "rust_service_delivery_controls.py",
                 "rust_service_state_controls.py", "rust_service_room_controls.py",
                 "test_rust_matchmaking_service.py", "rust_service_steam_provider_fixture.cpp"):
        target = source / "tools" / name
        target.parent.mkdir(exist_ok=True)
        shutil.copyfile(ROOT / "tools" / name, target)
    pins = {str(p.relative_to(source)): digest(p) for p in source.rglob("*") if p.is_file()}
    drift = [name for name, pin in pins.items() if digest(ROOT / name) != pin]
    (output / "source-manifest.json").write_text(json.dumps(dict(files=pins, drift_during_freeze=drift), indent=2) + "\n")
    if drift:
        raise RuntimeError("Delivery inputs changed while freezing: " + ", ".join(drift))
    report = dict(passed=False, source=str(source), scratch=str(scratch), commands=[],
                  retained_large_artifacts=[], production_activated=False, docker_image_executed=False,
                  mode="legacy-state-only" if args.legacy_state_only else "host-route")
    environment = dict(os.environ, CARGO_TARGET_DIR=str(scratch / "cargo"), CARGO_INCREMENTAL="0",
                       RUSTUP_TOOLCHAIN="1.96.0", RUSTUP_AUTO_INSTALL="0")

    def run(command, name, env=environment):
        result = subprocess.run(command, cwd=source, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        data = result.stdout
        if len(data) > 1024 * 1024:
            data = data[-1024 * 1024:]
        (output / (name + ".log")).write_bytes(data)
        report["commands"].append(dict(argv=command, returncode=result.returncode, log=name + ".log"))
        if result.returncode:
            raise RuntimeError(name + " failed: " + data.decode(errors="replace")[-2500:])
        return result.stdout

    try:
        caddy_binary = caddy(scratch, report)
        manifest = str(source / "rust/matchmaking_service/Cargo.toml")
        run(["cargo", "test", "--release", "--locked", "--offline", "--jobs", "2", "--manifest-path", manifest], "rust-service-tests")
        run(["cargo", "build", "--release", "--locked", "--offline", "--jobs", "2", "--manifest-path", manifest], "rust-service-build")
        run(["cargo", "build", "--release", "--locked", "--offline", "--jobs", "2", "--manifest-path",
             str(source / "rust/online_security_ffi/Cargo.toml")], "directory-production-abi")
        binary = scratch / "cargo/release/halo2-matchmaking-service"
        library = scratch / "cargo/release/libhalo2_online_security_ffi.so"
        report["binary"] = dict(sha256=digest(binary), bytes=binary.stat().st_size, retained=False)
        report["directory_abi"] = dict(sha256=digest(library), bytes=library.stat().st_size, retained=False)
        test_env = dict(environment, H2_SECURITY_LIBRARY=str(library), HALO2_MATCHMAKING_TEST_INPUTS="0")
        run([sys.executable, "-B", "-m", "unittest", "multiplayer.master_server.tests.test_discovery_only",
             "multiplayer.master_server.tests.test_http", "multiplayer.master_server.tests.test_registry"],
            "directory-regressions", test_env)
        shim = None
        if not args.legacy_state_only:
            shim = scratch / "provider.so"
            run(["g++", "-std=c++20", "-O1", "-shared", "-fPIC", str(source / "tools/rust_service_steam_provider_fixture.cpp"),
                 "-o", str(shim), "-ldl", "-lcrypto"], "private-provider-fixture")
        report["attempts"] = []
        relative = "tools/" + ("rust_service_state_controls.py" if args.legacy_state_only else "rust_service_delivery_controls.py")
        for attempt in range(1, 5):
            attempt_output = output / ("attempt-%02d" % attempt)
            attempt_output.mkdir()
            spec = importlib.util.spec_from_file_location("delivery_controls_" + str(attempt), source / relative)
            controls = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(controls)
            entry = dict(attempt=attempt, controls_sha256=digest(source / relative), output=str(attempt_output), passed=False)
            report["attempts"].append(entry)
            try:
                report["delivery"] = controls.qualify(source, scratch / ("delivery-%02d" % attempt), binary, library,
                                                       caddy_binary, shim, attempt_output)
                entry["passed"] = report["delivery"]["passed"]
                break
            except Exception as error:
                entry["error"] = redact_failure(repr(error))
                (attempt_output / "failure.txt").write_text(redact_failure(traceback.format_exc()))
                (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
                if not args.owned_controls_retry_file or attempt == 4:
                    raise
                print(json.dumps(dict(control_failed=entry, waiting_for=str(args.owned_controls_retry_file))), flush=True)
                deadline = time.monotonic() + 600
                while not args.owned_controls_retry_file.exists():
                    if time.monotonic() >= deadline:
                        raise RuntimeError("Owned control investigation expired") from error
                    time.sleep(0.2)
                raw = args.owned_controls_retry_file.read_bytes()
                if len(raw) > 4096:
                    raise RuntimeError("Invalid owned refresh request")
                action = json.loads(raw)
                args.owned_controls_retry_file.unlink()
                if action == {"quit": True}:
                    raise RuntimeError("Owned control investigation closed") from error
                if set(action) != {"refresh_controls_sha256"} or digest(ROOT / relative) != action["refresh_controls_sha256"]:
                    raise RuntimeError("Owned fixture refresh hash mismatch")
                entry["accepted_control_refresh"] = dict(before_sha256=digest(source / relative),
                                                         after_sha256=action["refresh_controls_sha256"],
                                                         compiler_inputs_changed=False)
                shutil.copyfile(ROOT / relative, source / relative)
                (source / relative).touch()
                pins[relative] = action["refresh_controls_sha256"]
        (output / "source-manifest-final.json").write_text(json.dumps(dict(files=pins, accepted_control_refreshes=report["attempts"]), indent=2) + "\n")
        report["source_drift_after_validation"] = [name for name, pin in pins.items() if digest(ROOT / name) != pin]
        report["passed"] = report["delivery"]["passed"]
        print(json.dumps(dict(passed=report["passed"], checks=report["delivery"]["checks"], report=str(output / "report.json"))))
        return 0 if report["passed"] else 1
    except BaseException as error:
        report["error"] = redact_failure(str(error))
        if isinstance(error, Exception):
            raise RuntimeError(redact_failure(repr(error))) from None
        raise
    finally:
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    sys.exit(main())
