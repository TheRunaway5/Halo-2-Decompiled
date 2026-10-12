#!/usr/bin/env python3
"""Bind the actual TU1.5 entry selections of the compiled complete modules.

This is module composition, not binary execution or a function-address guess.
Each selected address comes from that module's compiled correspondence table.
The emitted manifest pins every consumed production source file.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
MODULES=(('World','physics_world_runtime_generated.cpp'),
         ('Pool','physics_pool_startup_runtime_generated.cpp'),
         ('Arena','physics_game_arena_runtime_generated.cpp'),
         ('MainHeap','physics_main_heap_runtime_generated.cpp'),
         ('Early','physics_early_initializer_runtime_generated.cpp'),
         ('Renderer','physics_renderer_storage_runtime_generated.cpp'),
         ('Process','source_process_heap_runtime_generated.cpp'),
         ('Phantom','physics_phantom_world_tu15_generated.cpp'),
         ('Damage','source_damage_delivery_runtime_generated.cpp'))
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True)
    a=ap.parse_args();rows=[];inputs={};seen=set()
    for kind,name in MODULES:
        p=ROOT/'src'/name;source=p.read_text();inputs[str(p.relative_to(ROOT))]=digest(p)
        if kind=='Phantom':
            # This TU1.5-only composition exposes actual selected entries.
            # Consume its literal binding arrays, including the vehicle graph.
            for table in ('new_entries','vehicle_entries','world_entries'):
                body=source.split(f'> {table}={{{{',1)[1].split('}};',1)[0]
                for value in re.findall(r'\{0x([0-9a-f]+)u,0x[0-9a-f]+u\}',body):
                    actual=int(value,16)
                    if actual in seen:continue
                    seen.add(actual);rows.append(dict(actual=actual,canonical=actual,module=kind))
            continue
        selected=source.split('static void run_')[0]
        mapping={int(x,16):int(y,16) for x,y in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',selected)}
        for value in re.findall(r'case 0x([0-9a-f]+)u:run_',source):
            canonical=int(value,16)
            if kind=='Damage':
                # Every damage selection must be an authenticated pair,
                # including any entry whose two literal addresses coincide.
                assert canonical in mapping,(name,hex(canonical))
            actual=mapping.get(canonical,canonical)
            # Full Xbox process startup remains separate from the explicit
            # native process heap constructor used by the live owner.
            if kind=='Process' and canonical in (0x2d7f8e,0x2dc5d2,0x2dc5ff):continue
            if actual in seen:continue
            seen.add(actual);rows.append(dict(actual=actual,canonical=canonical,module=kind))
    output='#pragma once\n#include <array>\n#include <cstdint>\nnamespace halo2::physics_live_entries_tu15 {\n'
    output+='enum class Module { '+', '.join(x for x,_ in MODULES)+' };\n'
    output+='struct Entry {std::uint32_t actual,canonical;Module module;};\n'
    output+=f'inline constexpr std::array<Entry,{len(rows)}> entries{{{{\n'
    output+=''.join(f'{{0x{r["actual"]:x}u,0x{r["canonical"]:x}u,Module::{r["module"]}}},\n' for r in rows)
    output+='}};\n} // namespace halo2::physics_live_entries_tu15\n'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(output)
    report=dict(image='English TU1.5',tool_sha256=digest(Path(__file__)),inputs=inputs,
        generated_sha256=digest(a.output),entries=rows,original_code_execution=False)
    a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(entries=len(rows),generated_sha256=digest(a.output))))
if __name__=='__main__':main()
