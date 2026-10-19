"""Synthetic read-only admission/routing controls; no game/runtime execution."""
import contextlib
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

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import mcc_sandbox_launcher as shortcut
import native_gameplay_test as admission


class Dispatched(Exception):
    pass


class MccSandboxLauncherTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="mcc-sandbox-", dir=os.environ.get("HALO2_TEST_SCRATCH"))
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.cache = self.root / "assets/imported/mcc-test"
        self.maps = self.cache / "halo2/h2_maps_win64_dx11"
        for name in shortcut.mcc_onboarding.REQUIRED:
            path = self.cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(struct.pack("<II", 0x68656164, 13) + b"synthetic MCC fixture"
                             if path.suffix == ".map" else b"synthetic imported fixture")
        self.provenance = dict(schema="H2_MCC_TU15_PREPARATION_V1", external_installation_requirement="Halo MCC only",
                               runtime_active=False, full_tu15_parity=False)
        (self.cache / "provenance.json").write_text(json.dumps(self.provenance))
        (self.root / "assets/mcc-source.tsv").write_text("H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-test\n")
        self.runtime = self.root / "build/halo2_native"
        self.shaders = self.runtime.parent / "shaders"
        self.shaders.mkdir(parents=True)
        names = ["world.vert.spv", "world.frag.spv", "hud.frag.spv"] + [f"fixture-{i}.spv" for i in range(94)]
        for name in names:
            (self.shaders / name).write_bytes(name.encode())
        self.runtime.write_bytes(b"synthetic admission owner\0" + os.fsencode(self.shaders) + b"\0")
        self.runtime.chmod(0o700)
        self.launcher = self.root / ("Halo2-Launcher.exe" if os.name == "nt" else "Halo2-Launcher")
        self.launcher.write_text("synthetic launcher; execution is mocked")
        self.launcher.chmod(0o700)
        self.publish_native()
        self.info = (f"Project: {self.root}\nRuntime: {self.runtime}\nAsset source: Halo MCC only\n"
                     f"MCC cache prepared: yes\nMCC cache: {self.cache}\nMCC maps: {self.maps}\n"
                     "Native MCC gameplay ready: no\nNative MCC gameplay test: available\nNative MCC frontend: unavailable\n"
                     "Play: native Lockout gameplay test\n")
        self.help = "MCC setup: --setup\n--native-test opens the qualified native Lockout test.\n"
        self.completed = patch.object(shortcut.subprocess, "run", side_effect=self.info_call).start()
        self.addCleanup(patch.stopall)
        self.execute = patch.object(shortcut.os, "execv", side_effect=Dispatched).start()

    @staticmethod
    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def info_call(self, command, **kwargs):
        self.assertEqual(command[0], str(self.launcher))
        self.assertIn(command[1], ("--launcher-info", "--help"))
        self.assertEqual(len(command), 2)
        self.assertEqual(kwargs["cwd"], self.root)
        self.assertNotIn("shell", kwargs)
        return subprocess.CompletedProcess(command, 0, self.info if command[1] == "--launcher-info" else self.help, "")

    def capability(self, frontend=False):
        report = dict(schema=admission.FRONTEND_SCHEMA if frontend else admission.SCHEMA,
            capability=admission.FRONTEND_CAPABILITY if frontend else admission.CAPABILITY,
            runtime=dict(path=str(self.runtime), sha256=self.digest(self.runtime)),
            shaders=dict(binding="compiled-default", directory=str(self.shaders),
                         files={p.name: self.digest(p) for p in self.shaders.iterdir()}),
            map=dict(path=admission.FRONTEND_MAP if frontend else admission.MAP,
                     sha256=self.digest(self.maps / ("mainmenu.map" if frontend else "lockout.map"))),
            validation=dict(passed=True, scope=admission.FRONTEND_SCOPE if frontend else admission.SCOPE,
                            report_sha256="1" * 64))
        if frontend:
            resources = ["halo2/h2_maps_win64_dx11/shared.map", "halo2/h2_maps_win64_dx11/textures.dat",
                         "halo2/h2_maps_win64_dx11/sounds_neutral.dat", "halo2/h2_fonts/font_table.txt"]
            resources += ["presentation/prompts/" + name for name in
                          ("manifest.json", "prompt-source.tsv", "prompts.tsv")]
            resources += [f"presentation/prompts/atlases/{index}.dds" for index in range(6)]
            for name in resources:
                path = self.cache / name
                if not path.exists():
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(b"synthetic imported audio bank")
            resources += [path.relative_to(self.cache).as_posix()
                          for path in (self.cache / "halo2/h2_fonts").rglob("*") if path.is_file()]
            report["resources"] = {name: self.digest(self.cache / name) for name in resources}
        return report

    def publish_native(self):
        self.runtime.with_name(self.runtime.name + ".native-test.json").write_text(json.dumps(self.capability()))

    def publish_frontend(self):
        self.runtime.with_name(self.runtime.name + ".frontend-test.json").write_text(json.dumps(self.capability(True)))
        self.info = self.info.replace("Native MCC frontend: unavailable", "Native MCC frontend: available")

    def call(self, *args):
        stdout, stderr = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            status = shortcut.main(self.root, list(args))
        return status, stdout.getvalue(), stderr.getvalue()

    def test_default_dispatches_exact_regular_native_test_and_preserves_files(self):
        before = {p: p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        with self.assertRaises(Dispatched):
            self.call("--mute", "--hidden", "--frames", "6", "--viewport", "640", "480")
        self.execute.assert_called_once_with(str(self.launcher), [str(self.launcher), "--native-test",
            "--hidden", "--mute", "--frames", "6", "--viewport", "640", "480"])
        self.assertEqual(before, {p: p.read_bytes() for p in self.root.rglob("*") if p.is_file()})

    def test_separate_frontend_menu_keeps_original_native_test_owner(self):
        self.publish_frontend()
        frontend = self.root / "frontend/halo2_native"
        frontend.parent.mkdir()
        frontend.write_bytes(b"separate frontend owner\0" + os.fsencode(self.shaders) + b"\0")
        frontend.chmod(0o700)
        report = self.capability(True)
        report["runtime"] = dict(path=str(frontend), sha256=self.digest(frontend))
        sidecar = frontend.with_name(frontend.name + ".frontend-test.json")
        sidecar.write_text(json.dumps(report))
        self.runtime.with_name(self.runtime.name + ".frontend-test.json").unlink()
        (self.root / "frontend-runtime-selection.json").write_text(json.dumps(dict(
            schema="H2_LOCAL_FRONTEND_RUNTIME_SELECTION_V1", runtime="frontend/halo2_native",
            runtime_sha256=self.digest(frontend), capability_sha256=self.digest(sidecar))))
        self.info += f"Frontend runtime: {frontend}\n"
        with self.assertRaises(Dispatched):
            self.call("--menu")
        self.execute.assert_called_once_with(str(self.launcher), [str(self.launcher), "--launch-game", "--menu"])
        self.execute.reset_mock()
        with self.assertRaises(Dispatched):
            self.call()
        self.execute.assert_called_once_with(str(self.launcher), [str(self.launcher), "--native-test"])
        self.execute.reset_mock()
        self.info = self.info.replace(f"Frontend runtime: {frontend}", f"Frontend runtime: {self.runtime}")
        self.assertEqual(self.call("--menu")[0], 1)
        self.execute.assert_not_called()

    def test_menu_needs_its_separate_frontend_capability(self):
        self.assertEqual(self.call("--menu")[0], 1)
        self.execute.assert_not_called()
        self.publish_frontend()
        with self.assertRaises(Dispatched):
            self.call("--menu", "--validation")
        self.execute.assert_called_once_with(str(self.launcher), [str(self.launcher), "--launch-game", "--menu", "--validation"])

    def test_legacy_default_uses_only_its_actual_qualified_lockout_play_action(self):
        self.help = "MCC setup: --setup\n--launch-game skips the launcher home.\n"
        with self.assertRaises(Dispatched):
            self.call()
        self.execute.assert_called_once_with(str(self.launcher), [str(self.launcher), "--launch-game"])

    def test_legacy_play_cannot_mean_preview_or_frontend_and_never_admits_menus(self):
        self.help = "--launch-game skips the launcher home.\n"
        original = self.info
        for play in ("MCC content test build", "MCC title screen and menus", "native gameplay"):
            with self.subTest(play=play):
                self.info = original.replace("Play: native Lockout gameplay test", "Play: " + play)
                self.assertEqual(self.call()[0], 1)
        self.info = original
        self.publish_frontend()
        self.assertEqual(self.call()[0], 1)
        self.assertEqual(self.call("--menu")[0], 1)
        self.execute.assert_not_called()

    def test_missing_or_stale_native_capability_never_falls_back_to_preview(self):
        sidecar = self.runtime.with_name(self.runtime.name + ".native-test.json")
        sidecar.unlink()
        self.assertEqual(self.call()[0], 1)
        self.publish_native()
        self.runtime.write_bytes(self.runtime.read_bytes() + b"changed")
        self.assertEqual(self.call()[0], 1)
        self.execute.assert_not_called()

    def test_frontend_admission_does_not_require_gameplay_capability(self):
        self.publish_frontend()
        self.runtime.with_name(self.runtime.name + ".native-test.json").unlink()
        self.assertEqual(self.call()[0], 1)
        with self.assertRaises(Dispatched):
            self.call("--menu")

    def test_every_required_header_rejects_xbox_and_vista_even_with_matching_capability_hash(self):
        for name in shortcut.MAP_NAMES:
            path = self.maps / name
            original = path.read_bytes()
            for version in (8, 10):
                with self.subTest(name=name, version=version):
                    path.write_bytes(struct.pack("<II", 0x68656164, version) + original[8:])
                    self.publish_native()
                    status, _, error = self.call()
                    self.assertEqual(status, 1)
                    self.assertIn("MCC version13", error)
            path.write_bytes(original)
        self.execute.assert_not_called()

    def test_wrong_provenance_and_missing_selection_fail_closed(self):
        for change in ({"schema": "foreign"}, {"external_installation_requirement": "Xbox ISO"}):
            with self.subTest(change=change):
                (self.cache / "provenance.json").write_text(json.dumps({**self.provenance, **change}))
                self.assertEqual(self.call()[0], 1)
        (self.root / "assets/mcc-source.tsv").unlink()
        self.assertEqual(self.call()[0], 1)
        self.execute.assert_not_called()

    def test_foreign_cache_maps_project_or_runtime_in_actual_launcher_info_rejects(self):
        original = self.info
        for old, new in ((f"MCC cache: {self.cache}", f"MCC cache: {self.root}"),
                         (f"MCC maps: {self.maps}", f"MCC maps: {self.cache}"),
                         (f"Project: {self.root}", f"Project: {self.cache}"),
                         (f"Runtime: {self.runtime}", "Runtime: not built")):
            with self.subTest(field=old):
                self.info = original.replace(old, new)
                self.assertEqual(self.call()[0], 1)
        self.info = original + f"Runtime: {self.runtime}\n"
        self.assertEqual(self.call()[0], 1)
        self.execute.assert_not_called()

    def test_asset_runtime_source_and_mode_options_are_not_passthrough(self):
        for option in ("--runtime", "--installation", "--map", "--shared-maps", "--menu-maps",
                       "--mp-source-object-world", "--gameplay-xbe", "--xbox-source", "--vista-source",
                       "--menu-vista-settings", "--menu-ui-assets", "--hud-ui-assets", "--pause-ui-assets",
                       "--mcc-source", "--preview-mcc", "--campaign", "--host", "--join", "--screenshot"):
            with self.subTest(option=option), self.assertRaises(SystemExit):
                self.call(option, "/foreign")
        self.execute.assert_not_called()
        self.completed.assert_not_called()

    def test_zero_negative_overflow_and_unmatched_viewport_are_rejected_before_inspection(self):
        for args in (("--frames", "0"), ("--frames", "-1"), ("--frames", "4294967296"),
                     ("--viewport", "0", "480"), ("--viewport", "640"), ("--viewport", "640", "8192")):
            with self.subTest(args=args), self.assertRaises(SystemExit):
                self.call(*args)
        self.completed.assert_not_called()
        self.execute.assert_not_called()

    def test_check_is_bounded_read_only_json_and_reports_real_admission(self):
        status, output, error = self.call("--check")
        self.assertEqual((status, error), (0, ""))
        report = json.loads(output)
        self.assertEqual(report["runtime"], str(self.runtime))
        self.assertEqual(report["cache"], str(self.cache))
        self.assertTrue(report["native_test_available"])
        self.assertFalse(report["frontend_available"])
        self.assertFalse(report["runtime_launched"])
        self.assertLess(len(output), 4096)
        self.runtime.with_name(self.runtime.name + ".native-test.json").unlink()
        status, output, _ = self.call("--check")
        self.assertEqual(status, 1)
        self.assertFalse(json.loads(output)["passed"])
        self.execute.assert_not_called()

    def test_launcher_info_delegates_actual_info_and_policy_without_game(self):
        self.runtime.with_name(self.runtime.name + ".native-test.json").unlink()
        status, output, error = self.call("--launcher-info")
        self.assertEqual((status, error), (0, ""))
        self.assertTrue(output.startswith(self.info))
        self.assertIn(shortcut.POLICY, output)
        self.assertIn("Sandbox native test: unavailable", output)
        self.execute.assert_not_called()


if __name__ == "__main__":
    unittest.main()
