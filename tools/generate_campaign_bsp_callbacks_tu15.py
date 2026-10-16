#!/usr/bin/env python3
"""Recover BSP callbacks for the gameplay systems constructed by the native world."""
import argparse
import json
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import sha

CONSTRUCTED = (3,14,17,18,19,20,21,22,23,25,27,28,30,32,34,35,37,39,
               40,45,46,47,48,49,51,54,58,59,63)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('src/campaign_bsp_callbacks_tu15.hpp'))
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    assert image.read(0x31d400, 1) == b'\xc3'
    rows = []
    for index in CONSTRUCTED:
        load, unload = struct.unpack('<II', image.read(0x44aed0 + index * 0x24 + 0x10, 8))
        if any(value not in (0, 0x31d400) for value in (load, unload)):
            rows.append((index, load, unload))
    header = '''#pragma once
#include <array>
#include <cstdint>
namespace halo2 {
// Authenticated original44AED0 table. Native world owns these constructed
// gameplay systems; native rendering owns its separate BSP lifecycle.
struct CampaignBspCallbacksTu15 {unsigned row;std::uint32_t load,unload;};
inline constexpr std::array<CampaignBspCallbacksTu15,%d> campaign_bsp_callbacks_tu15{{
''' % len(rows)
    header += ''.join('  {%du,0x%xu,0x%xu},\n' % row for row in rows)
    header += '}};\n}\n'
    args.output.write_text(header)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(dict(source_sha256=NEW_PIN,
        load_order='13A860 ascending', unload_order='13A886 descending',
        rows=rows, header_sha256=sha(args.output)), indent=2) + '\n')

if __name__ == '__main__':
    main()
