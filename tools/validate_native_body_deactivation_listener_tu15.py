#!/usr/bin/env python3
"""Physical TU1.5 body-deactivation listener, native growth and true child stops.

Use run_test.py; all banks, binaries and compiler products use its scratch.
Allocation and component-motion results are never supplied to the native side.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM

OWNERS = (0x2815f0, 0x2e3250)
BASE = 0x1000000
LISTENER, BODY, PROPERTY, MANAGER, COMPONENTS, QUEUE, ALLOCATOR, TABLE = (
    BASE + offset for offset in (0x100, 0x300, 0x500, 0x800, 0x1000, 0x3000, 0x4000, 0x4100))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def metadata(image):
    code = _Code(image, ".text")
    registers = {name: n for n, name in enumerate(("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"))}
    sites, counts, listings = [], [], {}
    for entry in OWNERS:
        body = complete_body(code, entry)
        counts.append(len(body.code))
        listing = []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            listing.append(f"{pc:x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n")
            operands = []
            for operand in instruction.operands:
                if operand.type != X86_OP_MEM or instruction.mnemonic == "lea":
                    continue
                assert not operand.mem.segment
                access = operand.access
                assert access & (CS_AC_READ | CS_AC_WRITE)
                operands.append(dict(base=registers.get(instruction.reg_name(operand.mem.base), -1),
                                     index=registers.get(instruction.reg_name(operand.mem.index), -1),
                                     scale=operand.mem.scale, displacement=operand.mem.disp & 0xffffffff,
                                     bytes=operand.size, read=bool(access & CS_AC_READ), write=bool(access & CS_AC_WRITE)))
            sites.append(dict(instruction=pc, operands=operands))
        listings[hex(entry)] = hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert counts == [63, 46]
    return dict(sites=sites, body_sites=counts, listing_sha256=listings,
                boundaries=[0x149da0, 0x1d7260], regions=[])


def cases():
    rows = []

    def make(name, deferred=0, count=0, capacity=8, datum=1, property_count=1,
             property_index=0, flags=0, bodies=0, refs=3):
        banks = {BASE: bytearray(0x5000), 0x52be5c: bytearray(4), 0x48c938: bytearray(4)}

        def put(address, value, width=4):
            first = next(p for p, data in banks.items() if p <= address and address + width <= p + len(data))
            banks[first][address-first:address-first+width] = (value & ((1 << (width * 8))-1)).to_bytes(width, "little")

        put(LISTENER + 0x18, deferred, 1)
        put(LISTENER + 8, QUEUE)
        put(LISTENER + 12, count)
        put(LISTENER + 16, capacity)
        put(BODY + 6, refs, 2)
        put(BODY + 0x30, PROPERTY)
        put(BODY + 0x34, property_count)
        for n in range(4):
            put(PROPERTY + n * 8, 0x2001 if n == property_index else 0x2002)
            put(PROPERTY + n * 8 + 4, datum)
        put(0x52be5c, MANAGER)
        put(MANAGER + 0x44, COMPONENTS)
        for n in range(4):
            put(COMPONENTS + n * 160 + 4, flags)
            put(COMPONENTS + n * 160 + 0x74, bodies)
        put(0x48c938, ALLOCATOR)
        put(ALLOCATOR, TABLE)
        put(TABLE + 0x10, 0x149da0)
        n = len(rows)
        rows.append(dict(name=name, entry=0x2815f0, receiver=LISTENER, arguments=[BODY],
                         banks=[dict(address=p, before=data.hex()) for p, data in sorted(banks.items())],
                         before_fp=[(0x27f, 0x37f, 0x7f)[n % 3] | ((n % 4) << 10),
                                    ((n % 8) << 11) | (0x453f if n % 3 == 0 else 0),
                                    (0x1f80, 0x5fa5, 0xffc0)[n % 3]],
                         stop_pc=physical.STOP, boundary=None))

    for deferred in (0, 1, 255):
        for property_count in (0, 1, 4, 0xffffffff, 0x80000000):
            for datum in (0, 0xabcd0001, 0xffffffff):
                make(f"datum_{deferred}_{property_count}_{datum}", deferred=deferred,
                     property_count=property_count, datum=datum)
    for property_index in (0, 1, 3, 7):
        for flags in (0, 0x20, 0xffffffff):
            for count in (0, 0xffffffff, 0x80000000, 3):
                make(f"properties_{property_index}_{flags}_{count}", property_count=4,
                     property_index=property_index, flags=flags, bodies=count)
    for count in (0, 1, 4):
        for borrowed in (0, 0x80000000):
            make(f"growth_{count}_{borrowed}", deferred=1, count=count, capacity=count | borrowed)
    for count in (1, 4, 7):
        make(f"nonempty_queue_{count}", deferred=1, count=count, capacity=8, datum=0xffffffff)
    for refs in (0, 1, 0xfffe, 0xffff):
        make(f"reference_{refs}", deferred=1, refs=refs, datum=0xffffffff)
    make("fault_seed_queue", deferred=1, property_count=4, property_index=3, flags=0x20)
    make("fault_seed_component", deferred=0, property_count=4, property_index=3, flags=0x20, bodies=2)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--production-binding", action="store_true")
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve()
    report = dict(passed=False, owned_scratch=str(scratch), retained_large_artifacts=[])
    files = [ROOT / "src/physics/native_tu15_body_deactivation_listener.cpp",
             ROOT / "include/halo2/native_tu15_body_deactivation_listener.hpp",
             ROOT / "include/halo2/native_tu15_body_deactivation_binding.hpp",
             ROOT / "tests/native_tu15_body_deactivation_listener_tests.cpp", Path(__file__),
             ROOT / "tools/validate_native_island_agent_phase_tu15.py",
             ROOT / "tools/observe_native_constraint_step_tu15.cpp"]
    for unit in ("activation_payload_filter", "activation_services", "array_growth"):
        files += [ROOT / ("src/physics/native_tu15_" + unit + ".cpp"), ROOT / ("include/halo2/native_tu15_" + unit + ".hpp")]
    pins = {str(path.relative_to(ROOT)): sha(path) for path in files}
    try:
        assert sha(NEW_DEFAULT) == NEW_PIN
        image = Xbe(NEW_DEFAULT)
        meta = metadata(image)

        def boundary(case, actual):
            owner = actual["actual_stop"]
            if owner == physical.STOP:
                return None
            gp, arguments = actual["boundary_gp"], actual["boundary_stack"]
            if owner == 0x149da0:
                return dict(owner=owner, receiver=gp[1], bytes=arguments[1], memory_class=arguments[2])
            assert owner == 0x1d7260
            return dict(owner=owner, component=arguments[1], index=gp[0])

        physical.boundary = boundary
        rows = physical.physical(image, meta, cases(), scratch)
        prefixes, atomic_write_faults = [], set()
        code = _Code(image, ".text")
        for seed in (row for row in rows if row["name"].startswith("fault_seed")):
            for category in ("reads", "writes"):
                seen, occurrences = set(), {}
                for event in seed[category]:
                    width = event["bytes"] if category == "reads" else len(event["written"]) // 2
                    key = event["address"], width
                    occurrences[key] = occurrences.get(key, 0) + 1
                    if event["instruction"] in seen:
                        continue
                    seen.add(event["instruction"])
                    instruction = code.at(event["instruction"])
                    if category == "writes" and any(op.type == X86_OP_MEM and op.access & CS_AC_READ and op.access & CS_AC_WRITE for op in instruction.operands):
                        atomic_write_faults.add(event["instruction"])
                        continue
                    row = copy.deepcopy(seed)
                    row.update(name=seed["name"] + "_" + category + "_" + hex(event["instruction"]),
                               stop_pc=event["instruction"], fault=dict(address=key[0], bytes=width,
                               occurrence=occurrences[key], access="read" if category == "reads" else "write"))
                    prefixes.append(row)
            for n, event in enumerate(seed["writes"]):
                row = copy.deepcopy(seed)
                row.update(name=seed["name"] + "_observer_" + str(n),
                           stop_pc=event["instruction"] + code.at(event["instruction"]).size, observer_fault=n+1)
                prefixes.append(row)
        rows += physical.physical(image, meta, prefixes, scratch)
        fixture = scratch / "listener.json"
        save(fixture, dict(image_sha256=NEW_PIN, required_backend="unchanged-tu15-native-body-deactivation-listener-v1",
                           body_sites=meta["body_sites"], cases=rows))
        project = scratch / "project"
        project.mkdir()
        shutil.copytree(ROOT / "include", project / "include")
        source = ROOT / "src/physics/native_tu15_body_deactivation_listener.cpp"
        text = source.read_text()
        (project / "native.cpp").write_text(text)
        shutil.copyfile(ROOT / "tests/native_tu15_body_deactivation_listener_tests.cpp", project / "driver.cpp")
        variants = {}
        for name, needle, replacement in (
            ("wrong_reference", 'm.read(body.identity+6u,2)+1u', 'm.read(body.identity+6u,2)+2u'),
            ("wrong_flag", '&0x20u', '&0x40u'),
            ("wrong_queue", 'data+insertion*4u', 'data+insertion*8u')):
            assert text.count(needle) == 1
            variants[name] = text.replace(needle, replacement)
            (project / (name + ".cpp")).write_text(variants[name])
        for unit in ("activation_payload_filter", "activation_services", "array_growth"):
            shutil.copyfile(ROOT / ("src/physics/native_tu15_" + unit + ".cpp"), project / (unit + ".cpp"))
        (project / "negative.py").write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.stdout,r.stderr)\nprint(r.stderr[-4096:])\n')
        cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(NativeDeactivationListener LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
        for name in ("native", *variants):
            cmake += 'add_executable(listener_' + name + ' $<TARGET_OBJECTS:driver> ' + name + '.cpp activation_payload_filter.cpp activation_services.cpp array_growth.cpp)\ntarget_include_directories(listener_' + name + ' PRIVATE include)\ntarget_compile_features(listener_' + name + ' PRIVATE cxx_std_20)\n'
            command = '' if name == 'native' else '"' + sys.executable + '" "' + str(project / 'negative.py') + '" '
            cmake += 'add_test(NAME listener_' + name + ' COMMAND "' + sys.executable + '" "' + str(ROOT / 'tools/run_test.py') + '" -- ' + command + '$<TARGET_FILE:listener_' + name + '> "' + str(fixture) + '"' + (' --production-binding' if args.production_binding else '') + ')\n'
        (project / "CMakeLists.txt").write_text(cmake)
        results = []
        for compiler in ("g++", "clang++"):
            command = [sys.executable, str(ROOT / "tools/run_tests.py"), "--source", str(project),
                       "--scratch-root", str(scratch / "builds"), "--output", str(args.output / compiler),
                       "--jobs", "1", "--test-jobs", "1", "--filter", "^listener_", "--cmake-arg=-DCMAKE_CXX_COMPILER=" + compiler]
            for name in ("native", *variants):
                command += ["--target", "listener_" + name]
            result = subprocess.run(command)
            results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
        unchanged = all(sha(ROOT / path) == digest for path, digest in pins.items())
        report.update(passed=all(item["exit_code"] == 0 for item in results) and unchanged, compiler_results=results,
                      source_snapshot_unchanged=unchanged,
                      frozen_compiler_source_hashes={str(path.relative_to(project)): sha(path) for path in project.rglob("*") if path.is_file()},
                      production_binding=args.production_binding,
                      original_sha256=NEW_PIN, body_sites=meta["body_sites"], listing_sha256=meta["listing_sha256"],
                      cases=len(rows), full_returns=sum(row["actual_stop"] == physical.STOP and not row.get("fault") and not row.get("observer_fault") for row in rows),
                      child_entry_prefixes=sum(row["boundary"] is not None for row in rows),
                      ordered_reads=sum(len(row["reads"]) for row in rows), ordered_stores=sum(len(row["writes"]) for row in rows),
                      negative_controls=list(variants), source_hashes=pins,
                      unqualified_atomic_rmw_write_fault_sites=sorted(atomic_write_faults),
                      instruction_patches=[], supplied_callee_results=False,
                      remaining=["Native returned1D7260 component motion closure", "Full allocation return composition",
                                 "Mutable returned callbacks and wider aliases", "Atomic RMW write-fault service parity", "Visible World integration"])
    except BaseException as error:
        report["error"] = str(error) or repr(error)
        raise
    finally:
        report["temporary_allocated_bytes_before_cleanup"] = sum(path.stat().st_blocks * 512 for path in scratch.rglob("*") if path.is_file())
        save(args.output / "report.json", report)
    print(json.dumps({key: report[key] for key in ("passed", "cases", "full_returns", "ordered_reads", "ordered_stores")}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
