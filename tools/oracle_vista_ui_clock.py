#!/usr/bin/env python3
"""Execute actual 60CA7D clock accumulation, stopping before unrelated UI work.

The binary32 input delta is an explicit test input. Original fld/fmul, x87
control-word truncation, fistp64 and low-word addition execute unchanged.
Native steady-clock sampling is a separate platform boundary.
"""
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

from oracle_native_vista_menu_services import Original
from oracle_vista_pcr_screen import u32, w32
from unicorn.x86_const import UC_X86_REG_FPCW

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "analysis/frontend/vista-session-flow-audit/local-session-inputs-original"))
from audit import PE, EXE, EXE_SHA


def main():
    original = PE(EXE, EXE_SHA)
    machine = Original(original)
    machine.stops = {0x60CAB3}
    literal = struct.unpack("<f", original.data[original.file(0x79DE40-original.base):][:4])[0]
    assert literal == 1000.0
    rng = random.Random(0x60CA7D)
    samples = [0x3A83126E, 0x3C83126E, 0x3F800000, 0x41200000, 0x41200001, 0x4728C000]
    samples += [rng.randrange(0, 0x4F000000) for _ in range(1000)]
    rows = []
    default_control = machine.u.reg_read(UC_X86_REG_FPCW)
    for control in (0x27F, 0x37F):
        for bits in samples:
            delta = struct.unpack("<f", struct.pack("<I", bits))[0]
            machine.u.reg_write(UC_X86_REG_FPCW, control)
            w32(machine.u, 0xD71900, 0xFFFFFFF0)
            result = machine.invoke(0x60CA7D, 0, [bits])
            assert result["pc"] == 0x60CAB3
            actual = u32(machine.u, 0xD71900)
            expected = (0xFFFFFFF0 + int(float(delta) * 1000.0)) & 0xFFFFFFFF
            assert actual == expected, (control, bits, actual, expected)
            rows.append([control, bits, actual])
    report = dict(exe_sha256=EXE_SHA, cases=len(rows),
                  default_unicorn_fpcw=default_control,
                  tested_control_words=[0x27F, 0x37F],
                  case_results_sha256=hashlib.sha256(json.dumps(rows, separators=(",", ":")).encode()).hexdigest(),
                  span=original.span(0x60CA7D, 0x60CAB3), literal_address=0x79DE40,
                  literal_value=literal, boundaries=__doc__)
    output = ROOT / "analysis/frontend/vista-session-flow-audit/native-ui-clock-original"
    output.mkdir(parents=True, exist_ok=True)
    (output / "original.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: report[key] for key in ("cases", "case_results_sha256", "default_unicorn_fpcw")}))


if __name__ == "__main__":
    main()
