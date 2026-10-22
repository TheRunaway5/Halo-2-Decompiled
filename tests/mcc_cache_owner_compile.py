"""Compile changed Cache consumers with their actual production CMake flags.

This checks production objects, constructor/private-call symbols and the current
Cache layout only. It does not link or execute Cache, authenticate the older
selected runtime ABI, or establish gameplay.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from source_layout import source_file


def compile_command(row, output):
    command = row.get("arguments") or shlex.split(row["command"])
    result, skip = [], False
    for argument in command:
        if skip:
            skip = False
        elif argument in ("-o", "-MF", "-MT", "-MQ"):
            skip = True
        elif argument not in ("-c", "-MD", "-MMD", "-MP"):
            result.append(argument)
    return result + ["-c", "-o", str(output)]


def symbols(path, *flags):
    result = subprocess.run(["nm", *flags, str(path)], check=True, text=True,
                            capture_output=True, timeout=30)
    return {line.split()[-1] for line in result.stdout.splitlines() if line.split()}


def layout_symbols(path):
    result = subprocess.run(["nm", "-S", "--defined-only", str(path)], check=True,
                            text=True, capture_output=True, timeout=30)
    return {parts[-1]: int(parts[-3], 16) for line in result.stdout.splitlines()
            if len(parts := line.split()) == 4 and parts[-1] in ("cache_layout", "cache_alignment")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--commands", type=Path, required=True)
    parser.add_argument("--root", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    if not scratch.is_absolute() or not scratch.is_dir():
        raise RuntimeError("Use the owned CTest/run_test lifecycle")
    root = args.root.resolve(strict=True)
    rows = json.loads(args.commands.read_text())
    selected = {}
    for name in ("cache.cpp", "mcc_cache.cpp", "mcc_native_shaders.cpp", "mcc_native_library.cpp"):
        source = source_file(root, name)
        matches = [row for row in rows if Path(row["file"]).resolve() == source and
                   "CMakeFiles/halo2_cache.dir/" in " ".join(row.get("arguments") or [row["command"]])]
        if len(matches) != 1:
            raise RuntimeError("Missing unique production compile command for " + name)
        selected[name] = matches[0]
    objects = {}
    for name, row in selected.items():
        source = Path(row["file"])
        before = hashlib.sha256(source.read_bytes()).hexdigest()
        objects[name] = scratch / (name + ".o")
        subprocess.run(compile_command(row, objects[name]), cwd=row["directory"], check=True, timeout=180)
        if hashlib.sha256(source.read_bytes()).hexdigest() != before:
            raise RuntimeError("Source changed during compilation: " + name)
    caller = scratch / "cache-owner-constructor-caller.cpp"
    caller.write_text('#include "halo2/cache.hpp"\n'
        'void constructor_calls(const std::filesystem::path& p) {\n'
        ' halo2::Cache a(p); halo2::Cache b(p,p,{}); halo2::Cache c(p,p,{},p);\n}\n')
    row = selected["cache.cpp"]
    caller_object = scratch / "constructor-caller.o"
    command = compile_command(row, caller_object)
    command = [str(caller) if argument == row["file"] else argument for argument in command]
    subprocess.run(command, cwd=row["directory"], check=True, timeout=180)
    required = {name for name in symbols(caller_object, "--undefined-only") if name.startswith("_ZN5halo25CacheC")}
    provided = symbols(objects["cache.cpp"], "--defined-only")
    if len(required) != 2 or not required <= provided:
        raise RuntimeError("Old and explicit-context constructor symbols are not both provided")
    for consumer, provider, fragment in (
            ("cache.cpp", "mcc_cache.cpp", "initialize_mccE"),
            ("mcc_cache.cpp", "mcc_native_shaders.cpp", "initialize_mcc_native_shadersE")):
        calls = {name for name in symbols(objects[consumer], "--undefined-only") if fragment in name}
        if len(calls) != 1 or not calls <= symbols(objects[provider], "--defined-only"):
            raise RuntimeError("Private Cache initializer consumer/provider mismatch: " + fragment)
    header = (root / "include/halo2/cache.hpp").read_text()
    baseline = header.replace("struct MccPreparedMapOwner;\n", "", 1).replace(
        "    // Explicit prepared-cache context for mod-owned map/shared directories.\n"
        "    // The existing constructor remains available for conventional stock paths.\n"
        "    Cache(const std::filesystem::path& path,const std::filesystem::path& shared_directory,\n"
        "          std::shared_ptr<const OriginalData> gameplay_data,\n"
        "          const std::filesystem::path& prepared_cache_root);\n", "", 1).replace(
        "void initialize_mcc(const MccPreparedMapOwner*);", "void initialize_mcc();", 1).replace(
        "void initialize_mcc_native_shaders(const MccPreparedMapOwner*);", "void initialize_mcc_native_shaders();", 1)
    if hashlib.sha256(baseline.encode()).hexdigest() != "821d66c77a9316ccbc0691459aea102f026291d8016914c03763170385e029cd":
        raise RuntimeError("Cache header contains an undeclared change beyond the reviewed method declarations")
    baseline_directory = scratch / "baseline/halo2"
    baseline_directory.mkdir(parents=True)
    (baseline_directory / "cache.hpp").write_text(baseline)
    probe = scratch / "cache-layout.cpp"
    probe.write_text('#include "halo2/cache.hpp"\n'
                     'unsigned char cache_layout[sizeof(halo2::Cache)];\n'
                     'unsigned char cache_alignment[alignof(halo2::Cache)];\n')
    layouts = []
    for kind in ("baseline", "current"):
        output = scratch / (kind + "-layout.o")
        command = compile_command(row, output)
        command = [str(probe) if argument == row["file"] else argument for argument in command]
        if kind == "baseline":
            command.insert(1, "-I" + str(baseline_directory.parent))
        subprocess.run(command, cwd=row["directory"], check=True, timeout=180)
        layouts.append(layout_symbols(output))
    if not layouts[0] or layouts[0] != layouts[1]:
        raise RuntimeError("Current Cache size/alignment changed")
    print("Four production objects, old/new constructor symbols and both private call pairs pass; current Cache layout", layouts[0])
    print("No full link, selected-runtime ABI or gameplay claim; objects remain owned test scratch")


if __name__ == "__main__":
    main()
