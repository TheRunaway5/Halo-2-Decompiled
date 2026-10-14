#!/usr/bin/env python3
"""Retain the actual English TU1.5 seated-camera bodies and source contracts."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code
from generate_physics_live_support_tu15 import complete_body
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = (0x158340, 0x158450, 0x158900, 0x1888a0, 0x18a0c0,
           0x189fd0, 0x245ea0, 0x245c70, 0x2455f0, 0x175380,
           0x381710, 0x381730, 0x381750, 0x381770,
           0x153f20, 0xcb3d0, 0xcb090, 0xcb520, 0xcb7a0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, ".text")
    selection = code.at(0x15850d)
    assert selection.bytes.hex() == "baa05e2400"
    assert code.at(0x153fad).bytes.hex() == "e81e74f7ff"
    args.output.mkdir(parents=True, exist_ok=True)
    rows = []
    for entry in ENTRIES:
        body = complete_body(code, entry)
        instructions = [code.at(address) for address in sorted(body.code)]
        text = "".join(f"{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n"
                       for i in instructions)
        file = args.output / f"{entry:08x}.asm"
        file.write_text(text)
        rows.append({"entry": f"{entry:08x}", "instructions": len(instructions),
                     "listing": file.name, "sha256": hashlib.sha256(file.read_bytes()).hexdigest()})
    report = {"reference": "English retail TU1.5 only", "xbe_sha256": NEW_PIN,
              "camera_selection_site": "0015850d", "camera_selection_hex": selection.bytes.hex(),
              "seat_selection_site": "00153fad", "seat_selection_wrapper": "000cb3d0",
              "routines": rows,
              "scope": "Static original bodies and camera selection. Live rendering and UDP evidence are separate. Native local input delivery and surrounding camera mode transition ownership must be assessed independently."}
    (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"routines": len(rows), "output": str(args.output)}))


if __name__ == "__main__":
    main()
