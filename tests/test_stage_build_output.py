"""Local-build staging transactions use disposable roots, never live dist/."""
from pathlib import Path
import hashlib
import json
import os
import platform
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
import stage_build_output as staging


def elf(payload=b"compiled shader directory: /existing/native/shaders"):
    header = bytearray(64)
    header[:7] = b"\x7fELF\x02\x01\x01"
    struct.pack_into("<HHI", header, 16, 3, 62, 1)
    struct.pack_into("<Q", header, 32, 64)
    struct.pack_into("<HHH", header, 52, 64, 56, 1)
    program = struct.pack("<IIQQQQQQ", 1, 5, 0, 0x400000, 0x400000, 120+len(payload), 120+len(payload), 0x1000)
    return bytes(header)+program+payload


class StageBuildOutputTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="build-stage-", dir=os.environ.get("HALO2_TEST_SCRATCH"))
        self.addCleanup(self.temporary.cleanup)
        self.base = Path(self.temporary.name)
        self.root = self.base/"project with spaces"
        self.root.mkdir()
        self.build = self.base/"build with spaces"
        self.build.mkdir()
        self.out = self.root/"dist/builds/linux-x86_64"

    def binary(self, target="halo2_native", payload=None):
        relative = Path(staging.OUTPUTS[target])
        path = self.build/relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(elf(target.encode() if payload is None else payload))
        path.chmod(0o755)
        return path

    def stage(self, path=None, target="halo2_native"):
        return staging.stage(self.root, path or self.binary(target), target)

    def manifest(self):
        return json.loads((self.out/staging.MANIFEST).read_text())

    def assert_no_temporaries(self):
        self.assertFalse([p for p in self.root.rglob("*") if p.name.startswith((".binary-", ".previous-", ".build-output-"))])

    def test_all_four_targets_have_distinct_outputs_and_verified_manifest(self):
        originals = {}
        for target in staging.OUTPUTS:
            source = self.binary(target)
            before = source.stat()
            result = self.stage(source, target)
            self.assertTrue(result["binary_changed"])
            destination = self.out/staging.OUTPUTS[target]
            self.assertEqual(source.read_bytes(), destination.read_bytes())
            self.assertEqual((source.stat().st_mtime_ns, source.stat().st_ino), (before.st_mtime_ns, before.st_ino))
            originals[target] = source
        report = self.manifest()
        self.assertFalse(report["packaged_release"])
        self.assertEqual(set(report["files"]), set(staging.OUTPUTS))
        for target, source in originals.items():
            row = report["files"][target]
            self.assertEqual(row["sha256"], hashlib.sha256(source.read_bytes()).hexdigest())
            self.assertEqual(row["source"], str(source))
            self.assertEqual(row["path"], staging.OUTPUTS[target])
        self.assertNotEqual((self.out/"Halo2-Launcher").read_bytes(), (self.out/"onboarding/Halo2-Launcher").read_bytes())
        self.assert_no_temporaries()

    def test_identical_content_is_not_recopied_or_manifest_rewritten(self):
        source = self.binary(payload=b"preserve /existing/native/shaders byte for byte")
        self.stage(source)
        paths = [self.out/"halo2_native", self.out/staging.MANIFEST]
        before = [(p.stat().st_ino, p.stat().st_mtime_ns, p.read_bytes()) for p in paths]
        result = self.stage(source)
        self.assertFalse(result["binary_changed"])
        self.assertFalse(result["manifest_changed"])
        self.assertEqual(before, [(p.stat().st_ino, p.stat().st_mtime_ns, p.read_bytes()) for p in paths])

    def test_source_path_only_updates_manifest_without_binary_copy(self):
        source = self.binary()
        self.stage(source)
        copy = self.base/"another build/halo2_native"
        copy.parent.mkdir()
        copy.write_bytes(source.read_bytes())
        copy.chmod(0o755)
        before = (self.out/"halo2_native").stat().st_ino
        result = self.stage(copy)
        self.assertFalse(result["binary_changed"])
        self.assertTrue(result["manifest_changed"])
        self.assertEqual(before, (self.out/"halo2_native").stat().st_ino)
        self.assertEqual(self.manifest()["files"]["halo2_native"]["source"], str(copy))

    def test_changed_binary_is_copied_without_rewriting_compiled_bindings(self):
        source = self.binary(payload=b"old /compiled/shaders")
        self.stage(source)
        source.write_bytes(elf(b"new /the/exact/compiled/shaders"))
        result = self.stage(source)
        self.assertTrue(result["binary_changed"])
        self.assertEqual((self.out/"halo2_native").read_bytes(), source.read_bytes())
        self.assert_no_temporaries()

    def test_unsupported_platforms_skip_even_missing_source_and_root(self):
        for platform in ["linux-arm64", "windows-x86_64", "windows-arm64", "macos-arm64"]:
            result = staging.stage(self.base/"absent", self.base/"not-present.exe", "halo2_native", platform)
            self.assertEqual(result["status"], "skipped")
            self.assertFalse((self.base/"absent").exists())
        self.assertFalse((self.root/"dist").exists())

    def test_allowlist_and_basename_rejected_before_output_creation(self):
        source = self.binary()
        with self.assertRaises(staging.StageError):
            staging.stage(self.root, source, "shader_tool")
        with self.assertRaises(staging.StageError):
            staging.stage(self.root, source, "halo2_launcher")
        self.assertFalse((self.root/"dist").exists())

    def test_elf_architecture_header_bounds_and_executable_mode(self):
        cases = []
        for offset, value in [(0,b"FAIL"),(4,b"\x01"),(5,b"\x02"),(6,b"\x00"),(18,b"\xb7\x00"),(20,b"\x02\x00\x00\x00"),(52,b"\x30\x00"),(54,b"\x20\x00")]:
            raw = bytearray(elf());raw[offset:offset+len(value)] = value;cases.append(bytes(raw))
        cases += [elf()[:63]]
        raw = bytearray(elf());struct.pack_into("<Q",raw,32,1<<60);cases.append(bytes(raw))
        for raw in cases:
            with self.subTest(raw=raw[:24]):
                source = self.binary();source.write_bytes(raw)
                with self.assertRaises(staging.StageError):self.stage(source)
                self.assertFalse((self.root/"dist").exists())
        source=self.binary();source.chmod(0o644)
        with self.assertRaises(staging.StageError):self.stage(source)
        self.assertFalse((self.root/"dist").exists())

    def test_source_symlink_and_symlink_parent_are_refused(self):
        source=self.binary();link=self.base/"links";link.mkdir()
        (link/"halo2_native").symlink_to(source)
        with self.assertRaises(OSError):self.stage(link/"halo2_native")
        parent=self.base/"build alias";parent.symlink_to(self.build,target_is_directory=True)
        with self.assertRaises(OSError):self.stage(parent/"halo2_native")
        self.assertFalse((self.root/"dist").exists())

    def test_every_destination_directory_symlink_is_refused(self):
        source=self.binary()
        for parts in [("dist",),("dist","builds"),("dist","builds","linux-x86_64")]:
            with self.subTest(parts=parts):
                root=self.base/("root-"+str(len(parts)));root.mkdir()
                bad=root.joinpath(*parts);bad.parent.mkdir(parents=True,exist_ok=True)
                outside=self.base/("outside-"+str(len(parts)));outside.mkdir();(outside/"keep").write_text("unchanged")
                bad.symlink_to(outside,target_is_directory=True)
                with self.assertRaises(OSError):staging.stage(root,source,"halo2_native")
                self.assertEqual([p.name for p in outside.iterdir()],["keep"])

    def test_destination_binary_manifest_and_onboarding_symlinks_are_refused(self):
        for name,target in [("halo2_native","halo2_native"),(staging.MANIFEST,"halo2_native"),("onboarding","halo2_onboarding")]:
            with self.subTest(name=name):
                root=self.base/("target-"+name.replace(".","-"));root.mkdir();out=root/"dist/builds/linux-x86_64";out.mkdir(parents=True)
                outside=self.base/("external-"+name.replace(".","-"));outside.write_text("untouched")
                (out/name).symlink_to(outside)
                with self.assertRaises(OSError):staging.stage(root,self.binary(target),target)
                self.assertEqual(outside.read_text(),"untouched")

    def test_invalid_existing_manifest_is_preserved(self):
        source=self.binary();self.stage(source)
        manifest=self.out/staging.MANIFEST;manifest.write_text('{"schema":"foreign manifest"}')
        old=(self.out/"halo2_native").read_bytes();source.write_bytes(elf(b"replacement"))
        with self.assertRaises(staging.StageError):self.stage(source)
        self.assertEqual((self.out/"halo2_native").read_bytes(),old)
        self.assertEqual(manifest.read_text(),'{"schema":"foreign manifest"}')
        self.assert_no_temporaries()

    def test_source_change_during_copy_preserves_previous_pair(self):
        source=self.binary();self.stage(source)
        old=(self.out/"halo2_native").read_bytes();manifest=(self.out/staging.MANIFEST).read_bytes()
        source.write_bytes(elf(b"new input"))
        copy=staging._copy_binary
        def changed(src,dest,expected):
            source.write_bytes(elf(b"changed mid-copy"))
            return copy(src,dest,expected)
        with patch.object(staging,"_copy_binary",side_effect=changed):
            with self.assertRaises(staging.StageError):self.stage(source)
        self.assertEqual((self.out/"halo2_native").read_bytes(),old)
        self.assertEqual((self.out/staging.MANIFEST).read_bytes(),manifest)
        self.assert_no_temporaries()

    def test_manifest_publish_failure_rolls_binary_back(self):
        source=self.binary();self.stage(source)
        old=(self.out/"halo2_native").read_bytes();manifest=(self.out/staging.MANIFEST).read_bytes()
        source.write_bytes(elf(b"new binary before failing manifest"))
        replace=os.replace
        def fail(src,dest,*args,**kwargs):
            if dest==staging.MANIFEST:raise OSError("injected manifest publication failure")
            return replace(src,dest,*args,**kwargs)
        with patch.object(staging.os,"replace",side_effect=fail):
            with self.assertRaisesRegex(OSError,"injected"):self.stage(source)
        self.assertEqual((self.out/"halo2_native").read_bytes(),old)
        self.assertEqual((self.out/staging.MANIFEST).read_bytes(),manifest)
        self.assert_no_temporaries()

    def test_first_manifest_publish_failure_removes_uncommitted_binary(self):
        source=self.binary()
        replace=os.replace
        def fail(src,dest,*args,**kwargs):
            if dest==staging.MANIFEST:raise OSError("injected first manifest failure")
            return replace(src,dest,*args,**kwargs)
        with patch.object(staging.os,"replace",side_effect=fail):
            with self.assertRaisesRegex(OSError,"injected"):self.stage(source)
        self.assertFalse((self.out/"halo2_native").exists())
        self.assertFalse((self.out/staging.MANIFEST).exists())
        self.assert_no_temporaries()

    def test_concurrent_cli_targets_do_not_lose_manifest_entries(self):
        jobs=[]
        for target in staging.OUTPUTS:
            source=self.binary(target)
            jobs.append(subprocess.Popen([sys.executable,"-B",str(TOOLS/"stage_build_output.py"),"--root",str(self.root),"--binary",str(source),"--target",target,"--platform","linux-x86_64"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True))
        try:
            for process in jobs:
                out,error=process.communicate(timeout=10)
                self.assertEqual(process.returncode,0,error)
                self.assertEqual(json.loads(out)["status"],"staged")
        finally:
            for process in jobs:
                if process.poll()is None:process.kill();process.wait()
        self.assertEqual(set(self.manifest()["files"]),set(staging.OUTPUTS))
        self.assert_no_temporaries()

    def test_sigterm_during_copy_cleans_temps_and_preserves_existing_pair(self):
        source=self.binary();self.stage(source)
        old=(self.out/"halo2_native").read_bytes();manifest=(self.out/staging.MANIFEST).read_bytes()
        source.write_bytes(elf(b"new candidate"));ready=self.base/"copy-ready"
        code="import sys,time;sys.path.insert(0,sys.argv[1]);import stage_build_output as s;from pathlib import Path\ndef slow(*args):\n Path(sys.argv[2]).touch();time.sleep(30)\ns._copy_binary=slow;s.main(sys.argv[3:])"
        process=subprocess.Popen([sys.executable,"-B","-c",code,str(TOOLS),str(ready),"--root",str(self.root),"--binary",str(source),"--target","halo2_native","--platform","linux-x86_64"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        try:
            end=time.monotonic()+5
            while not ready.exists()and process.poll()is None and time.monotonic()<end:time.sleep(.01)
            self.assertTrue(ready.exists())
            process.terminate();process.communicate(timeout=5)
            self.assertEqual(process.returncode,143)
        finally:
            if process.poll()is None:process.kill();process.wait()
        self.assertEqual((self.out/"halo2_native").read_bytes(),old)
        self.assertEqual((self.out/staging.MANIFEST).read_bytes(),manifest)
        self.assert_no_temporaries()

    def test_sigterm_between_publications_is_deferred_until_pair_is_complete(self):
        source=self.binary();self.stage(source)
        source.write_bytes(elf(b"accepted complete new pair"))
        code="import sys,os,signal;sys.path.insert(0,sys.argv[1]);import stage_build_output as s\nreplace=os.replace\ndef interrupted(src,dest,*args,**kwargs):\n result=replace(src,dest,*args,**kwargs)\n if dest=='halo2_native':os.kill(os.getpid(),signal.SIGTERM)\n return result\ns.os.replace=interrupted;s.main(sys.argv[2:])"
        process=subprocess.run([sys.executable,"-B","-c",code,str(TOOLS),"--root",str(self.root),"--binary",str(source),"--target","halo2_native","--platform","linux-x86_64"],capture_output=True,text=True,timeout=5)
        self.assertEqual(process.returncode,143,process.stderr)
        self.assertEqual((self.out/"halo2_native").read_bytes(),source.read_bytes())
        self.assertEqual(self.manifest()["files"]["halo2_native"]["sha256"],hashlib.sha256(source.read_bytes()).hexdigest())
        self.assert_no_temporaries()

    @unittest.skipUnless(sys.platform == "linux" and platform.machine().lower() in ("x86_64", "amd64") and
                         shutil.which("cmake") and shutil.which("c++"), "Native Linux x86_64 CMake/C++ required")
    def test_real_cmake_hook_stages_four_elf_targets_and_scratch_default_is_off(self):
        def fixture(root):
            root.mkdir(parents=True, exist_ok=True)
            (root/"tools").mkdir()
            (root/"cmake").mkdir()
            shutil.copyfile(TOOLS/"stage_build_output.py",root/"tools/stage_build_output.py")
            shutil.copyfile(TOOLS.parent/"cmake/BuildOutput.cmake",root/"cmake/BuildOutput.cmake")
            lines=["cmake_minimum_required(VERSION 3.24)","project(BuildOutputFixture LANGUAGES CXX)",
                   "find_package(Python3 REQUIRED COMPONENTS Interpreter)"]
            for target in staging.OUTPUTS:
                (root/(target+".cpp")).write_text('#include <cstdio>\nint main(){return std::puts("'+target+'")<0;}\n')
                lines.append("add_executable("+target+" "+target+".cpp)")
            lines += ['set_target_properties(halo2_launcher PROPERTIES OUTPUT_NAME "Halo2-Launcher")',
                      'set_target_properties(halo2_title_launcher PROPERTIES OUTPUT_NAME "Halo2-Title-Launcher")',
                      'set_target_properties(halo2_onboarding PROPERTIES OUTPUT_NAME "Halo2-Launcher" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/onboarding")',
                      'include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildOutput.cmake")']
            (root/"CMakeLists.txt").write_text("\n".join(lines)+"\n")
        def run(command,env):
            result=subprocess.run(command,env=env,capture_output=True,text=True,timeout=60)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        environment=os.environ.copy()
        environment["HALO2_TEST_SCRATCH"]=environment.get("HALO2_TEST_SCRATCH",str(self.base))
        environment.pop("CMAKE_TOOLCHAIN_FILE",None)
        for enabled in [True,False]:
            root=self.base/("explicit staging source" if enabled else "default scratch source")
            build=self.base/("explicit staging build" if enabled else "default scratch build")
            fixture(root)
            configure=[shutil.which("cmake"),"-S",str(root),"-B",str(build),
                       "-DPython3_EXECUTABLE="+sys.executable,"-DCMAKE_CXX_COMPILER="+shutil.which("c++")]
            if shutil.which("ninja"):configure += ["-G","Ninja"]
            if enabled:configure.append("-DHALO2_STAGE_BUILD_OUTPUT=ON")
            run(configure,environment)
            cache=(build/"CMakeCache.txt").read_text()
            self.assertIn("HALO2_STAGE_BUILD_OUTPUT:BOOL="+("ON" if enabled else "OFF"),cache)
            run([shutil.which("cmake"),"--build",str(build),"--parallel","2"],environment)
            if not enabled:
                self.assertFalse((root/"dist").exists(),"Disposable CMake validation retained staged binaries")
                continue
            output=root/"dist/builds/linux-x86_64"
            manifest=json.loads((output/staging.MANIFEST).read_text())
            self.assertEqual(manifest["schema"],staging.SCHEMA)
            self.assertEqual(set(manifest["files"]),set(staging.OUTPUTS))
            self.assertFalse(manifest["packaged_release"])
            self.assertEqual([p.name for p in (root/"dist/builds").iterdir()],["linux-x86_64"])
            for target,relative in staging.OUTPUTS.items():
                original=build/relative
                copied=output/relative
                self.assertEqual(copied.read_bytes(),original.read_bytes())
                self.assertEqual(copied.read_bytes()[:7],b"\x7fELF\x02\x01\x01")
                self.assertEqual(struct.unpack_from("<H",copied.read_bytes(),18)[0],62)
                record=manifest["files"][target]
                self.assertEqual(record["path"],relative)
                self.assertEqual(record["source"],str(original))
                self.assertEqual(record["sha256"],hashlib.sha256(original.read_bytes()).hexdigest())
                self.assertEqual(record["bytes"],original.stat().st_size)
            self.assertNotEqual((output/"Halo2-Launcher").read_bytes(),(output/"onboarding/Halo2-Launcher").read_bytes())


if __name__=="__main__":
    if os.environ.get("HALO2_TEST_SCRATCH"):
        print("Owned test scratch: "+os.environ["HALO2_TEST_SCRATCH"],flush=True)
    unittest.main()
