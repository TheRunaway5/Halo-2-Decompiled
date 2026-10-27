#!/usr/bin/env python3
"""Compare captured live native sword movement with unchanged TU1.5 execution.

Raw captures must stay in the enclosing tools/run_test.py scratch owner. Only
the bounded summary is retained. No native output supplies an original callee
return; the complete original 1F39D0/1F3B80/10F080/30BD0 closure executes.
"""
import argparse
import hashlib
import json
import os
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
import unicorn.x86_const as R


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replay(image, allowed, instruction_bytes, case):
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    pages = set()

    def mount(address, size):
        for page in range(address & ~4095, (address + size + 4095) & ~4095, 4096):
            if page not in pages:
                machine.mem_map(page, 4096)
                pages.add(page)

    for section in image.sections:
        raw = image.section_bytes(section)
        mount(section.va, len(raw))
        machine.mem_write(section.va, raw)
    captured_bytes = set()
    for bank in case["initial_banks"]:
        raw = bytes.fromhex(bank["hex"])
        for offset, value in enumerate(raw):
            address = bank["address"] + offset
            if address in instruction_bytes:
                assert value == instruction_bytes[address], \
                    f"Captured bank would replace authenticated instruction byte {address:08x}"
            assert address not in captured_bytes, "Captured initial banks overlap"
            captured_bytes.add(address)
        mount(bank["address"], len(raw))
        machine.mem_write(bank["address"], raw)
    entry = case["entry_abi"]
    gp = [getattr(R, "UC_X86_REG_" + name) for name in
          ("EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI")]
    for register, value in zip(gp, entry["gp"]):
        machine.reg_write(register, value)
    for n, lanes in enumerate(entry["xmm"]):
        machine.reg_write(R.UC_X86_REG_XMM0+n, sum(lane << (k*32) for k, lane in enumerate(lanes)))
    fp = entry.get("floating_point", case["fp"])
    for register, value in ((R.UC_X86_REG_EFLAGS, entry["eflags"]),
            (R.UC_X86_REG_FPCW, fp["x87_control_word"]),
            (R.UC_X86_REG_MXCSR, fp["mxcsr"]),
            (R.UC_X86_REG_FPSW, entry["x87_status"]), (R.UC_X86_REG_FPTAG, 0xffff)):
        machine.reg_write(register, value)
    top = (entry["x87_status"] >> 11) & 7
    tags = 0xffff
    for n, encoded in enumerate(entry["x87"]):
        physical = (top+n) & 7
        raw = bytes.fromhex(encoded)
        machine.reg_write(getattr(R, f"UC_X86_REG_FP{physical}"), struct.unpack("<QH", raw))
        tags &= ~(3 << (physical*2))
    machine.reg_write(R.UC_X86_REG_FPTAG, tags)
    assert not entry["active_mmx"], "Active MMX capture unsupported"
    trace, stores = [], []

    def instruction(_, pc, size, user):
        assert pc in allowed, f"Unexpected original instruction/callee {pc:08x}"
        assert bytes(machine.mem_read(pc, size)) == image.read(pc, size), \
            f"Authenticated original instruction bytes changed at {pc:08x}"
        trace.append(pc)

    def read(_, access, address, size, value, user):
        assert all(address+n in captured_bytes for n in range(size)), \
            f"Original read lacks captured input bytes at {address:08x}+{size}"

    def write(_, access, address, size, value, user):
        assert all(address+n in captured_bytes for n in range(size)), \
            f"Original write lacks captured preimage bytes at {address:08x}+{size}"
        stores.append(dict(instruction=machine.reg_read(R.UC_X86_REG_EIP), address=address,
                           before=bytes(machine.mem_read(address, size)).hex(),
                           written=int(value).to_bytes(size, "little", signed=False).hex()))

    machine.hook_add(UC_HOOK_CODE, instruction)
    machine.hook_add(UC_HOOK_MEM_READ, read)
    machine.hook_add(UC_HOOK_MEM_WRITE, write)
    stop = struct.unpack("<I", bytes(machine.mem_read(entry["gp"][4], 4)))[0]
    machine.emu_start(case["entry"], stop, count=50000)
    assert machine.reg_read(R.UC_X86_REG_EIP) == stop, "Original movement did not return"
    expected = case["exit_abi"]
    assert not expected["active_mmx"], "Native movement unexpectedly leaves active MMX"
    assert expected["fs_base"] == entry["fs_base"], "Native movement changes unused FS base"
    expected_fp = expected.get("floating_point", fp)
    assert machine.reg_read(R.UC_X86_REG_FPCW) == expected_fp["x87_control_word"], \
        "Original/native x87 control word differs"
    assert machine.reg_read(R.UC_X86_REG_MXCSR) == expected_fp["mxcsr"], \
        "Original/native SSE control/exception state differs"
    assert trace == case["instructions"], "Original/native instruction sequence differs"
    assert [machine.reg_read(r) for r in gp] == expected["gp"], "Original/native GP registers differ"
    assert (machine.reg_read(R.UC_X86_REG_EFLAGS) & 0xcd5) == (expected["eflags"] & 0xcd5), \
        "Original/native defined arithmetic flags differ"
    for n, lanes in enumerate(expected["xmm"]):
        actual = machine.reg_read(R.UC_X86_REG_XMM0+n)
        assert [(actual >> (k*32)) & 0xffffffff for k in range(4)] == lanes, \
            f"Original/native SSE register {n} differs"
    assert stores == case["stores"], "Original/native ordered stores differ"
    for bank in case["final_banks"]:
        raw = bytes.fromhex(bank["hex"])
        assert bytes(machine.mem_read(bank["address"], len(raw))) == raw, \
            f"Original/native retained bytes differ at {bank['address']:08x}"
    sw = machine.reg_read(R.UC_X86_REG_FPSW)
    tags = machine.reg_read(R.UC_X86_REG_FPTAG)
    depth = sum(((tags >> (n*2)) & 3) != 3 for n in range(8))
    assert depth == expected["x87_depth"], "Original/native live x87 depth differs"
    assert (sw & 0x383f) == (expected["x87_status"] & 0x383f), \
        "Original/native x87 TOP or exception status differs"
    for n, encoded in enumerate(expected["x87"]):
        mantissa, exponent = machine.reg_read(getattr(R, f"UC_X86_REG_FP{((sw >> 11)+n)&7}"))
        assert struct.pack("<QH", mantissa, exponent).hex() == encoded, "Original/native live x87 value differs"
    return dict(index=case["index"], clock_tick=case["clock_tick"], passed=True,
                movement_state_address=case["movement_state_address"],
                movement_parent=case["movement_parent"],
                movement_active_byte=case["movement_active_byte"],
                physical_component=case["physical_component"], unit_datum=case["unit_datum"],
                physical_mode=case["physical_mode"],
                output_flags_before=case["output_flags_before"],
                output_flags_after=case["output_flags_after"],
                instructions=len(trace), stores=len(stores),
                retained_bytes=sum(len(bytes.fromhex(b["hex"])) for b in case["final_banks"]),
                original_x87_status=sw, native_x87_status=expected["x87_status"],
                x87_control_exception_top_and_live_values_compared=True,
                sse_control_exception_state_compared=True,
                x87_condition_bits_compared=False,
                fs_base_preserved_not_consumed=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--report-output", type=Path, required=True)
    parser.add_argument("--require-span-tick", type=int,
                        help="Require actual movement calls before and at/after this observed cancellation tick")
    parser.add_argument("--require-termination-transition", action="store_true",
                        help="Require continuing movement followed by original result bit4 termination")
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve()
    assert args.capture.resolve().is_relative_to(scratch), "Raw movement captures require owned scratch"
    assert sha(NEW_DEFAULT) == NEW_PIN, "TU1.5 executable authority differs"
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, ".text")
    entries = {0x1f39d0, 0x1f3b80, 0x10f080, 0x30bd0}
    allowed = set().union(*(_trace(code, entry, entries).code for entry in entries))
    assert all("fs:" not in code.at(pc).op_str for pc in allowed), \
        "Movement closure requires an FS segment owner before it can be replayed"
    instruction_bytes = {pc+n: value for pc in allowed
                         for n, value in enumerate(bytes(code.at(pc).bytes))}
    rows = [json.loads(line) for line in args.capture.read_text().splitlines() if line.strip()]
    assert rows and rows[0]["type"] == "header" and rows[0]["original_sha256"] == NEW_PIN
    assert len(rows) > 1, "Capture contains no actual live movement calls"
    result = dict(scope=__doc__, original_sha256=NEW_PIN, capture_sha256=sha(args.capture),
                  capture_bytes=args.capture.stat().st_size, tool_sha256=sha(Path(__file__)),
                  wrapper_sha256=sha(Path(__file__).with_name("capture_sword_movement_tu15.cpp")),
                  cases=[], passed=False, scratch=str(scratch),
                  qualification="Per-call movement parity; the enclosing canonical case separately owns cancellation/flying acceptance",
                  cancellation_span_tick=args.require_span_tick,
                  require_termination_transition=args.require_termination_transition)
    try:
        assert all(row.get("type") != "truncated" for row in rows[1:]), "Live movement capture was truncated"
        assert all(row["movement_active_byte"] in (0, 1) for row in rows[1:]), "Malformed movement active byte"
        assert any(row["movement_active_byte"] == 1 for row in rows[1:]), "Capture lacks an active movement input"
        assert all(0x1f3b2f in row["instructions"] for row in rows[1:]), "Capture lacks the actual physical movement consumer"
        contexts = {}
        for row in rows[1:]:
            key = (row["unit_datum"], row["physical_component"], row["movement_parent"])
            contexts.setdefault(key, []).append(row)
        qualified_contexts = []
        for key, cohort in contexts.items():
            ticks = [row["clock_tick"] for row in cohort]
            spans = args.require_span_tick is None or min(ticks) < args.require_span_tick <= max(ticks)
            continuing = False
            terminated = False
            for row in cohort:
                if row["output_flags_after"] & 4:
                    terminated |= continuing
                else:
                    continuing = True
            if spans and (not args.require_termination_transition or terminated):
                qualified_contexts.append(dict(unit_datum=key[0], physical_component=key[1],
                    movement_parent=key[2], minimum_tick=min(ticks), maximum_tick=max(ticks),
                    calls=len(cohort), termination_transition=terminated))
        assert qualified_contexts, "No same-unit movement context satisfies the requested cancellation span/termination coverage"
        result["qualified_contexts"] = qualified_contexts
        for case in rows[1:]:
            result["cases"].append(replay(image, allowed, instruction_bytes, case))
        result["passed"] = True
    except Exception as error:
        result["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        args.report_output.parent.mkdir(parents=True, exist_ok=True)
        args.report_output.write_text(json.dumps(result, indent=2)+"\n")
    print(f"PASS original/native live sword movement: {len(result['cases'])} actual calls")


if __name__ == "__main__":
    main()
