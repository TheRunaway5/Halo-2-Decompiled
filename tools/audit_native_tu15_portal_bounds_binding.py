#!/usr/bin/env python3
"""Audit the original writable portal alias and its native CRT constructor.

This offline audit exports a bounded provenance report, never runtime results.
Use tools/run_test.py; no original instructions execute in the game.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original English TU1.5 image differs')
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');data=image.section('.data')
    if not data.flags&1:raise ValueError('Original pointer has no writable data owner')
    for address,size in ((0x474d3c,4),(0x474d40,4),(0x55655c,24)):
        if not data.va<=address<address+size<=data.va+data.vsize:
            raise ValueError('Portal binding or destination is outside original .data')
    if image.read(0x474d3c,8)!=struct.pack('<II',0x55655c,0x55655c):
        raise ValueError('Original portal and BSP aliases differ')
    if image.read(0x55655c,24)!=bytes(24):raise ValueError('Original cold destination differs')
    if image.read(0x46d990,4)!=struct.pack('<I',0x380fa0):raise ValueError('Original CRT constructor row differs')
    expected={0x176462:'8b0d3c4d4700',0x17647f:'8b39',0x176488:'8b7904',
              0x17648e:'8b7908',0x176494:'8b490c'}
    reads=[]
    for pc,raw in expected.items():
        ins=code.at(pc)
        if ins.bytes.hex()!=raw:raise ValueError('Original portal consumer changed')
        reads.append(dict(pc=pc,bytes=raw,mnemonic=ins.mnemonic,operands=ins.op_str))
    constructor=[]
    for pc in (0x380fa0,0x380fa8,0x380fb0,0x380fb8,0x380fc0,0x380fc8,0x380fd0,0x380fd8,0x380fe0):
        ins=code.at(pc);constructor.append(dict(pc=pc,bytes=ins.bytes.hex(),mnemonic=ins.mnemonic,operands=ins.op_str))
    if [(row['mnemonic'],row['operands']) for row in constructor]!=[
            ('movss','xmm0, dword ptr [0x45695c]'),('movss','xmm1, dword ptr [0x44aac8]'),
            *[('movss',f'dword ptr [0x{0x55655c+4*i:x}], xmm{i%2}') for i in range(6)],('ret','')]:
        raise ValueError('Complete original bounds constructor differs')
    if image.read(0x45695c,4)!=struct.pack('<I',0x7f7fffff) or image.read(0x44aac8,4)!=struct.pack('<I',0xff7fffff):
        raise ValueError('Original bounds operands differ')
    native=ROOT/'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'
    text=native.read_text().split('static void run_380fa0(WorldState &s) {',1)[1].split('\n}',1)[0]
    if any(f's.services.instruction(0x{row["pc"]:x}u)' not in text for row in constructor):
        raise ValueError('Existing native constructor omitted an original instruction site')
    startup=ROOT/'src/physics/physics_live_world_tu15.cpp'
    if startup.read_text().count('call_actual(0x380fa0u);')!=1:
        raise ValueError('Native world does not invoke the actual constructor once')
    report=dict(passed=True,original_sha256=NEW_PIN,pointer_address=0x474d3c,
        pointer_hex=image.read(0x474d3c,4).hex(),pointer_value=0x55655c,
        original_section='.data',original_section_flags=data.flags,writable=True,
        shared_existing_BSP_binding=0x474d40,destination_bytes=24,
        destination_is_cold_storage_not_a_completed_result=True,consumer_reads=reads,
        actual_CRT_row=0x46d990,actual_constructor=constructor,
        native_constructor_sha256=sha(native),native_startup_sha256=sha(startup),
        runtime_opcode_execution=False,produced_bounds_imported=False,full_vehicle_parity=False,
        tool_sha256=sha(Path(__file__)))
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,pointer='474D3C -> 55655C',constructor='380FA0',report=str(args.report))))


if __name__=='__main__':main()
