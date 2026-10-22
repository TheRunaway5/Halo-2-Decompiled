"""Isolated prebuilt download/activation tests; no game, compiler or live assets."""
from contextlib import ExitStack
import copy
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.error import HTTPError, URLError

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import update_runtime as updater

subprocess_run = subprocess.run
subprocess_popen = subprocess.Popen


def digest(data):
    return hashlib.sha256(data).hexdigest()


def elf(marker=b"fixture"):
    """Tiny authenticated section-layout fixture, deliberately never executed."""
    names = b"\0.shstrtab\0.rodata\0"
    data = b"\0./bin/shaders\0" + marker
    table = 64 + len(names) + len(data)
    header = struct.pack("<16sHHIQQQIHHHHHH", b"\x7fELF\x02\x01\x01" + b"\0" * 9,
                         3, 62, 1, 0, 0, table, 0, 64, 0, 0, 64, 3, 1)
    sections = bytes(64)
    sections += struct.pack("<IIQQQQIIQQ", 1, 3, 0, 0, 64, len(names), 0, 0, 1, 0)
    sections += struct.pack("<IIQQQQIIQQ", names.index(b".rodata"), 1, 2, 0,
                            64 + len(names), len(data), 0, 0, 1, 0)
    return header + names + data + sections


class UpdateRuntimeTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-prebuilt-update-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "installation with spaces"
        self.root.mkdir()
        self.build = self.root / "existing build"
        self.shaders = {name: ("existing shader " + name).encode() for name in
            ["world.vert.spv", "world.frag.spv", "hud.frag.spv", *[f"program-{i}.spv" for i in range(94)]]}
        for name, data in self.shaders.items():
            path = self.build / "shaders" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        self.protected = {}
        for name, data in {"assets/imported/map.map": b"private map", "saves/profile": b"saved game",
                           "cache/warm": b"warm cache", "CMakeLists.txt": b"local dirty source",
                           "build/workbench": b"selected workbench", "Halo2-Launcher": b"root launcher"}.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            self.protected[name] = data
        self.messages, self.calls = [], []
        self.remote = {}
        self.stack = ExitStack()
        self.addCleanup(self.stack.close)
        self.stack.enter_context(patch.object(updater, "platform_key", return_value="linux-x86_64"))
        self.stack.enter_context(patch.object(updater, "request", side_effect=self.request))
        # Source builds and subprocess-based Git updates are outside this API.
        self.stack.enter_context(patch("subprocess.Popen", side_effect=AssertionError("Unexpected process")))
        self.stack.enter_context(patch("subprocess.run", side_effect=AssertionError("Unexpected process")))
        self.publish(1)

    def request(self, url):
        self.calls.append(url)
        value = self.remote.get(url)
        if isinstance(value, Exception):
            raise value
        if value is None:
            raise AssertionError("Unexpected URL: " + url)
        return io.BytesIO(value)

    def publish(self, release_id, payload=None, shaders=None, launchers=True, metadata=None):
        payload = elf(str(release_id).encode()) if payload is None else payload
        data = {"halo2_native": updater.SLOT_WRAPPER, "bin/halo2_native.real": payload,
                "lib/ld-linux-x86-64.so.2": b"unchanged loader", "lib/library.so": b"unchanged library"}
        if launchers:
            data.update({"Halo2-Launcher": elf(b"launcher"), "Halo2-Title-Launcher": elf(b"title"),
                         "onboarding/Halo2-Launcher": elf(b"onboarding"),
                         "halo2_native.frontend-test.json": b'{"schema":"test-only"}\n'})
        data.update(metadata or {})
        data.update({"bin/shaders/" + name: value for name, value in (shaders or self.shaders).items()})
        self.release = dict(id=release_id, draft=False, prerelease=False, assets=[])
        files = {}
        self.file_urls = {}
        for index, (name, body) in enumerate(data.items()):
            asset = f"linux-x86_64-{index}"
            url = f"https://github.com/{updater.REPOSITORY}/releases/download/v{release_id}/{asset}"
            self.release["assets"].append(dict(name=asset, browser_download_url=url, size=len(body)))
            files[name] = dict(asset=asset, bytes=len(body), sha256=digest(body), executable=(
                name in updater.LAUNCHERS or name in {"halo2_native", "bin/halo2_native.real", "lib/ld-linux-x86-64.so.2"}))
            self.remote[url] = body
            self.file_urls[name] = url
        self.manifest = dict(schema=updater.MANIFEST_SCHEMA, release_id=str(release_id), platforms={"linux-x86_64": dict(
            layout="linux-packaged-slot-v1", runtime="halo2_native", compiled_shader_default="./bin/shaders", files=files)})
        self.manifest_url = f"https://github.com/{updater.REPOSITORY}/releases/download/v{release_id}/{updater.MANIFEST_ASSET}"
        self.release["assets"].append(dict(name=updater.MANIFEST_ASSET, browser_download_url=self.manifest_url))
        self.refresh_manifest()

    def refresh_manifest(self):
        body = json.dumps(self.manifest).encode()
        self.remote[self.manifest_url] = body
        asset = next(row for row in self.release["assets"] if row["name"] == updater.MANIFEST_ASSET)
        asset.update(size=len(body), digest="sha256:" + digest(body))
        self.remote[updater.RELEASE_API] = json.dumps(self.release).encode()

    def update(self):
        updater.update_runtime(self.root, self.build, self.messages.append)

    def assert_preserved(self):
        for name, data in self.protected.items():
            self.assertEqual((self.root / name).read_bytes(), data)
        for name, data in self.shaders.items():
            self.assertEqual((self.build / "shaders" / name).read_bytes(), data)
        self.assertFalse(list(self.root.rglob(".staging-*")))
        self.assertFalse(list(self.root.glob(".runtime-selection-*")))

    def test_downloads_prebuilt_binaries_copies_shaders_and_selects_only_complete_release(self):
        self.update()
        selected = updater.selected_runtime(self.root)
        self.assertEqual(selected, self.root / "bin/releases/1/halo2_native")
        self.assertEqual(selected.read_bytes(), updater.SLOT_WRAPPER)
        self.assertEqual(updater.selected_launcher(self.root), selected.parent / "Halo2-Launcher")
        self.assertEqual(updater.selected_launcher(self.root, "onboarding/Halo2-Launcher"), selected.parent / "onboarding/Halo2-Launcher")
        self.assertNotIn(self.file_urls["bin/shaders/world.vert.spv"], self.calls)
        self.assertIn(self.file_urls["bin/halo2_native.real"], self.calls)
        self.assertIn("no source build", self.messages[-1])
        self.assert_preserved()

    def test_latest_release_downloads_only_changed_files_and_keeps_previous_slot(self):
        self.update()
        before = (self.root / "runtime-selection.json").read_bytes()
        self.calls.clear()
        self.publish(2)
        self.update()
        binary_requests = set(self.calls) - {updater.RELEASE_API, self.manifest_url}
        self.assertEqual(binary_requests, {self.file_urls["bin/halo2_native.real"]})
        self.assertNotEqual((self.root / "runtime-selection.json").read_bytes(), before)
        self.assertTrue((self.root / "bin/releases/1/halo2_native").is_file())
        self.assertEqual(updater.selected_runtime(self.root), self.root / "bin/releases/2/halo2_native")
        self.assert_preserved()

    def test_already_current_release_never_redownloads_binary_files(self):
        self.update()
        self.calls.clear()
        self.update()
        self.assertEqual(self.calls, [updater.RELEASE_API, self.manifest_url])
        self.assert_preserved()

    def test_no_published_release_fails_before_any_installation_write(self):
        self.remote[updater.RELEASE_API] = HTTPError(updater.RELEASE_API, 404, "Not Found", {}, None)
        with self.assertRaisesRegex(RuntimeError, "No prebuilt release has been published"):
            self.update()
        self.assertFalse((self.root / "bin").exists())
        self.assertFalse((self.root / "runtime-selection.json").exists())
        self.assert_preserved()

    def test_missing_platform_manifest_asset_digest_or_file_asset_fails_before_mutation(self):
        original_release, original_manifest = copy.deepcopy(self.release), copy.deepcopy(self.manifest)
        for problem in ["platform", "manifest", "digest", "file"]:
            with self.subTest(problem=problem):
                self.release, self.manifest = copy.deepcopy(original_release), copy.deepcopy(original_manifest)
                if problem == "platform":
                    self.manifest["platforms"] = {"windows-arm64": {}}
                elif problem == "manifest":
                    self.release["assets"] = [row for row in self.release["assets"] if row["name"] != updater.MANIFEST_ASSET]
                    self.remote[updater.RELEASE_API] = json.dumps(self.release).encode()
                elif problem == "file":
                    self.manifest["platforms"]["linux-x86_64"]["files"]["bin/halo2_native.real"]["asset"] = "missing"
                if problem != "manifest":
                    self.refresh_manifest()
                if problem == "digest":
                    self.release["assets"][-1]["digest"] = "sha256:" + "0" * 64
                    self.remote[updater.RELEASE_API] = json.dumps(self.release).encode()
                with self.assertRaises(RuntimeError):
                    self.update()
                self.assertFalse((self.root / "bin").exists())
        self.assert_preserved()

    def test_malformed_metadata_types_fail_cleanly_before_mutation(self):
        valid_release = copy.deepcopy(self.release)
        for value in [[], None, "release", 1]:
            with self.subTest(release=value):
                self.remote[updater.RELEASE_API] = json.dumps(value).encode()
                with self.assertRaises(RuntimeError):
                    self.update()
                self.assertFalse((self.root / "bin").exists())
        for value in [[], None, "manifest", 1]:
            with self.subTest(manifest=value):
                self.manifest = value
                self.release = copy.deepcopy(valid_release)
                self.refresh_manifest()
                with self.assertRaises(RuntimeError):
                    self.update()
                self.assertFalse((self.root / "bin").exists())
        self.publish(1)
        self.release["assets"][0]["browser_download_url"] = ["https://github.com/untrusted"]
        self.remote[updater.RELEASE_API] = json.dumps(self.release).encode()
        with self.assertRaisesRegex(RuntimeError, "asset URL"):
            self.update()
        self.assertFalse((self.root / "bin").exists())
        self.assert_preserved()

    def test_network_error_or_tampered_download_keeps_valid_previous_selection(self):
        self.update()
        previous = (self.root / "runtime-selection.json").read_bytes()
        for problem in [URLError("connection interrupted"), b"corrupt"]:
            self.publish(2)
            self.remote[self.file_urls["bin/halo2_native.real"]] = problem
            with self.assertRaises((RuntimeError, URLError)):
                self.update()
            self.assertEqual((self.root / "runtime-selection.json").read_bytes(), previous)
            self.assertIsNotNone(updater.selected_runtime(self.root))
            self.assertFalse((self.root / "bin/releases/2").exists())
            self.assert_preserved()

    def test_activation_replace_failure_rolls_back_new_slot_without_touching_old_selection(self):
        self.update()
        previous = (self.root / "runtime-selection.json").read_bytes()
        self.publish(2)
        with patch.object(updater.os, "replace", side_effect=OSError("fixture activation failure")):
            with self.assertRaisesRegex(OSError, "fixture activation failure"):
                self.update()
        self.assertEqual((self.root / "runtime-selection.json").read_bytes(), previous)
        self.assertIsNotNone(updater.selected_runtime(self.root))
        self.assertFalse((self.root / "bin/releases/2").exists())
        self.assert_preserved()

    def test_cancellation_after_atomic_activation_keeps_new_complete_slot(self):
        self.update()
        self.publish(2)
        replace = updater.os.replace
        def activate_then_cancel(*args):
            replace(*args)
            raise KeyboardInterrupt()
        with patch.object(updater.os, "replace", side_effect=activate_then_cancel):
            with self.assertRaises(KeyboardInterrupt):
                self.update()
        self.assertEqual(updater.selected_runtime(self.root), self.root / "bin/releases/2/halo2_native")
        self.assert_preserved()

    def test_shader_difference_refuses_update_instead_of_replacing_original_bank(self):
        changed = dict(self.shaders, **{"world.vert.spv": b"changed shader"})
        self.publish(1, shaders=changed)
        with self.assertRaisesRegex(RuntimeError, "same existing 97 shaders"):
            self.update()
        self.assertFalse((self.root / "bin").exists())
        self.assert_preserved()

    def test_unsafe_paths_wrong_repository_and_aliases_are_rejected(self):
        original = copy.deepcopy(self.manifest)
        for unsafe in ["../saves/profile", "/tmp/escape", "lib/../../escape", "lib\\bad", "assets/imported/map.map"]:
            with self.subTest(path=unsafe):
                self.manifest = copy.deepcopy(original)
                files = self.manifest["platforms"]["linux-x86_64"]["files"]
                files[unsafe] = files.pop("lib/library.so")
                self.refresh_manifest()
                with self.assertRaises(RuntimeError):
                    self.update()
                self.assertFalse((self.root / "bin").exists())
        self.manifest = original
        self.refresh_manifest()
        self.release["assets"][0]["browser_download_url"] = "https://github.com/untrusted/project/releases/download/v1/file"
        self.remote[updater.RELEASE_API] = json.dumps(self.release).encode()
        with self.assertRaisesRegex(RuntimeError, "official repository"):
            self.update()
        self.publish(1)
        outside = self.root.parent / "outside"
        outside.mkdir()
        (self.root / "bin").symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(RuntimeError, "symlinks"):
            self.update()
        self.assertEqual(list(outside.iterdir()), [])
        self.assert_preserved()

    def test_nonrelocatable_payload_and_old_wrapper_fail_before_selection(self):
        self.publish(1, payload=elf().replace(b"./bin/shaders", b"/old/shaders/"))
        with self.assertRaises(ValueError):
            self.update()
        self.assertIsNone(updater.selected_runtime(self.root))
        self.publish(1)
        self.remote[self.file_urls["halo2_native"]] = b"#!/bin/sh\nexit 0\n"
        with self.assertRaises(RuntimeError):
            self.update()
        self.assertIsNone(updater.selected_runtime(self.root))
        self.assert_preserved()

    def test_selected_runtime_rejects_stale_files_metadata_modes_and_symlinks(self):
        self.update()
        runtime = updater.selected_runtime(self.root)
        library = runtime.parent / "lib/library.so"
        saved = library.read_bytes()
        library.write_bytes(b"tampered")
        self.assertIsNone(updater.selected_runtime(self.root))
        library.write_bytes(saved)
        receipt = runtime.parent / updater.RECEIPT
        saved_receipt = receipt.read_bytes()
        receipt.write_bytes(b'{}')
        self.assertIsNone(updater.selected_runtime(self.root))
        receipt.write_bytes(saved_receipt)
        runtime.chmod(0o644)
        self.assertIsNone(updater.selected_runtime(self.root))
        runtime.chmod(0o755)
        self.assertIsNotNone(updater.selected_runtime(self.root))
        library.unlink()
        library.symlink_to(self.root / "saves/profile")
        self.assertIsNone(updater.selected_runtime(self.root))
        self.assertIsNone(updater.selected_launcher(self.root))
        self.assert_preserved()

    def test_persisted_receipt_rechecks_schema_paths_required_files_and_boolean_modes(self):
        self.update()
        slot = updater.selected_runtime(self.root).parent
        receipt_path, selection_path = slot / updater.RECEIPT, self.root / "runtime-selection.json"
        original_receipt, original_selection = receipt_path.read_bytes(), selection_path.read_bytes()
        for problem in ["target-list", "file-list", "omitted-runtime", "integer-mode", "unowned-path"]:
            with self.subTest(problem=problem):
                receipt = json.loads(original_receipt)
                if problem == "target-list":
                    receipt["target"] = []
                elif problem == "file-list":
                    receipt["target"]["files"] = []
                elif problem == "omitted-runtime":
                    receipt["target"]["files"].pop("halo2_native")
                elif problem == "integer-mode":
                    receipt["target"]["files"]["halo2_native"]["executable"] = 1
                else:
                    receipt["target"]["files"]["saves/profile"] = receipt["target"]["files"]["lib/library.so"]
                encoded = json.dumps(receipt).encode()
                receipt_path.write_bytes(encoded)
                selection = json.loads(original_selection)
                selection["manifest_sha256"] = digest(encoded)
                selection_path.write_text(json.dumps(selection))
                self.assertIsNone(updater.selected_runtime(self.root))
        receipt_path.write_bytes(original_receipt)
        selection_path.write_bytes(original_selection)
        self.assertIsNotNone(updater.selected_runtime(self.root))
        self.assert_preserved()

    def test_exact_file_modes_and_unshared_owners_are_required_for_selection(self):
        self.update()
        runtime = updater.selected_runtime(self.root)
        library = runtime.parent / "lib/library.so"
        library.chmod(0o666)
        self.assertIsNone(updater.selected_runtime(self.root))
        library.chmod(0o644)
        self.assertIsNotNone(updater.selected_runtime(self.root))
        os.link(library, self.root / "shared-library-owner")
        self.assertIsNone(updater.selected_runtime(self.root))
        (self.root / "shared-library-owner").unlink()
        self.assertIsNotNone(updater.selected_runtime(self.root))
        selection = self.root / "runtime-selection.json"
        saved = selection.read_bytes()
        selection.write_bytes(b" " * 65537)
        self.assertIsNone(updater.selected_runtime(self.root))
        selection.write_bytes(saved)
        self.assert_preserved()

    def test_undeclared_files_and_nonbinary_optional_launchers_are_rejected(self):
        self.update()
        runtime = updater.selected_runtime(self.root)
        unexpected = runtime.parent / "lib/undeclared.so"
        unexpected.write_bytes(b"undeclared")
        self.assertIsNone(updater.selected_runtime(self.root))
        unexpected.unlink()
        self.assertIsNotNone(updater.selected_runtime(self.root))
        self.publish(2)
        body = b"not a genuine launcher"
        row = self.manifest["platforms"]["linux-x86_64"]["files"]["Halo2-Launcher"]
        row.update(bytes=len(body), sha256=digest(body))
        next(asset for asset in self.release["assets"] if asset["name"] == row["asset"])["size"] = len(body)
        self.remote[self.file_urls["Halo2-Launcher"]] = body
        self.refresh_manifest()
        with self.assertRaisesRegex(RuntimeError, "ELF64 x86-64"):
            self.update()
        self.assertEqual(updater.selected_runtime(self.root), runtime)
        self.assert_preserved()

    def test_optional_launchers_are_not_invented(self):
        self.publish(1, launchers=False)
        self.update()
        self.assertIsNotNone(updater.selected_runtime(self.root))
        self.assertIsNone(updater.selected_launcher(self.root))
        with self.assertRaisesRegex(RuntimeError, "Unknown"):
            updater.selected_launcher(self.root, "../../arbitrary")

    def test_cli_verification_and_selection_outputs(self):
        with patch.object(sys, "argv", ["update_runtime.py", "--root", str(self.root), "--verify-selection"]), patch("sys.stdout", new_callable=io.StringIO) as output:
            self.assertEqual(updater.main(), 1)
            self.assertEqual(output.getvalue(), "")
        self.update()
        for option in ["--selected-runtime", "--verify-selection", "--selected-launcher"]:
            argv = ["update_runtime.py", "--root", str(self.root), option]
            if option == "--selected-launcher":
                argv.append("Halo2-Title-Launcher")
            with patch.object(sys, "argv", argv), patch("sys.stdout", new_callable=io.StringIO) as output:
                self.assertEqual(updater.main(), 0)
                self.assertEqual(bool(output.getvalue()), option != "--verify-selection")

    def test_release_capabilities_validate_in_fresh_safe_path_python(self):
        cache = self.root / "assets/imported/capability-fixture"
        resources = {"halo2/h2_maps_win64_dx11/lockout.map": b"fixture Lockout",
            "halo2/h2_maps_win64_dx11/mainmenu.map": b"fixture mainmenu",
            "halo2/h2_maps_win64_dx11/shared.map": b"fixture shared",
            "halo2/h2_maps_win64_dx11/textures.dat": b"fixture bitmap partition",
            "halo2/h2_maps_win64_dx11/sounds_neutral.dat": b"fixture classic audio partition",
            "halo2/h2_fonts/font_table.txt": b"fixture font selector",
            "halo2/h2_fonts/font.bin": b"fixture font pixels"}
        for name in ("manifest.json", "glow-mask.dds", "loading.wav"):
            resources["presentation/loading-v3/" + name] = b"fixture declared MCC V3 loading owner"
        for name, data in resources.items():
            path = cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        native = dict(schema="H2_NATIVE_GAMEPLAY_TEST_V1", capability="mcc-lockout-source-object-world-test",
            runtime=dict(path="halo2_native", sha256=digest(updater.SLOT_WRAPPER),
                payload=dict(path="bin/halo2_native.real", sha256=digest(elf(b"1"))),
                loader=dict(path="lib/ld-linux-x86-64.so.2", sha256=digest(b"unchanged loader"))),
            shaders=dict(binding="release-slot-compiled-default", directory="bin/shaders", compiled_default="./bin/shaders",
                         files={name:digest(data) for name,data in self.shaders.items()}),
            map=dict(path="halo2/h2_maps_win64_dx11/lockout.map", sha256=digest(resources["halo2/h2_maps_win64_dx11/lockout.map"])),
            validation=dict(passed=True, scope="mcc-lockout-native-gameplay-test", report_sha256="1" * 64))
        frontend = copy.deepcopy(native)
        frontend.update(schema="H2_NATIVE_FRONTEND_TEST_V1", capability="mcc-native-frontend",
            map=dict(path="halo2/h2_maps_win64_dx11/mainmenu.map", sha256=digest(resources["halo2/h2_maps_win64_dx11/mainmenu.map"])),
            resources={name:digest(data) for name,data in resources.items() if not name.endswith(("lockout.map", "mainmenu.map"))})
        frontend["validation"]["scope"] = "mcc-native-title-menu"
        frontend["validation"]["mcc_loading_presentation_profile"] = "mcc-logo-glow-mcc-track-oneshot-v1"
        frontend["validation"]["mcc_prompt_font_profile"] = "mcc-native-control-fonts-v1"
        self.publish(1, metadata={"halo2_native.native-test.json":json.dumps(native).encode(),
                                  "halo2_native.frontend-test.json":json.dumps(frontend).encode()})
        self.update()
        runtime = updater.selected_runtime(self.root)
        helper = Path(updater.__file__).with_name("native_gameplay_test.py")
        # These are Python admission readers, never the synthetic ELF fixtures.
        command = [sys.executable, "-P", "-B", str(helper), "--root", str(self.root),
                   "--runtime", str(runtime), "--cache", str(cache)]
        # setUp prohibits processes for updater calls; allow this explicit,
        # bounded fresh-Python CLI integration check only.
        with patch("subprocess.run", subprocess_run), patch("subprocess.Popen", subprocess_popen):
            for flags in [[], ["--frontend"]]:
                result = subprocess_run(command + flags, capture_output=True, text=True,
                    env={**os.environ, "PYTHONSAFEPATH":"1"}, timeout=10)
                self.assertEqual(result.returncode, 0, result.stderr)
            (cache / "halo2/h2_maps_win64_dx11/textures.dat").write_bytes(b"changed fixture pixels")
            result = subprocess_run(command + ["--frontend"], capture_output=True, text=True,
                env={**os.environ, "PYTHONSAFEPATH":"1"}, timeout=10)
            self.assertEqual(result.returncode, 1, result.stderr)
        self.assert_preserved()


class PlatformAndTransportTests(unittest.TestCase):
    def test_verifier_cli_imports_its_packaged_sibling_with_safe_python_path(self):
        with tempfile.TemporaryDirectory(prefix="halo2-update-cli-") as root:
            result = subprocess.run([sys.executable, "-P", "-B", str(Path(updater.__file__)),
                "--root", root, "--verify-selection"], capture_output=True, text=True,
                env={**os.environ, "PYTHONSAFEPATH": "1"}, timeout=10)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertEqual(result.stderr, "")

    def test_only_current_supported_platform_is_selected(self):
        with patch.object(updater.platform, "system", return_value="Linux"), patch.object(updater.platform, "machine", return_value="x86_64"):
            self.assertEqual(updater.platform_key(), "linux-x86_64")
        for system, machine in [("Linux", "aarch64"), ("Windows", "AMD64"), ("Windows", "ARM64"), ("Darwin", "arm64")]:
            with patch.object(updater.platform, "system", return_value=system), patch.object(updater.platform, "machine", return_value=machine):
                with self.assertRaisesRegex(RuntimeError, "No supported prebuilt"):
                    updater.platform_key()

    def test_untrusted_redirect_is_closed_and_rejected(self):
        response = io.BytesIO(b"not a GitHub asset")
        response.geturl = lambda: "https://untrusted.invalid/binary"
        with patch.object(updater, "urlopen", return_value=response):
            with self.assertRaisesRegex(RuntimeError, "trusted hosts"):
                updater.request(updater.RELEASE_API)
        self.assertTrue(response.closed)


if __name__ == "__main__":
    unittest.main()
