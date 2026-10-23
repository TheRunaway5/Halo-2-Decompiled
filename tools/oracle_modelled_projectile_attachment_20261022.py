#!/usr/bin/env python3
"""Original TU1.5 modelled Needler root-marker/MGS2 collection oracle.

The declared object storage has actual authored Needler metadata and a completed
single WORLD root. No projectile birth, physics, visibility or GPU result is
supplied or certified by this isolated attachment consumer.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import ROOT, Cache, WEAPON, WI
from oracle_weapon_graph_timing_tu15 import Original
from oracle_projectile_light_attachment_tu15 import mapping, QUEUE


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--report',type=Path,required=True)
    args=ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):ap.error('Use tools/run_test.py')
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    tag=next(t for t in cache.tags.values() if t['group']=='proj' and t['name'].endswith('\\needler_shard\\needler_shard'))
    assert cache.u(tag['offset']+0x38)!=0xffffffff
    entries,absolute,owners=mapping('TU1.5')
    t=Original();t.g.mem_map(QUEUE,0x20000)
    t.w(WEAPON,int(tag['datum'],16));t.w(WEAPON+4,0);t.w(WEAPON+0x14,0xffffffff)
    t.g.mem_write(WEAPON+0xaa,b'\x05');t.sh(WEAPON+0x116,0x500)
    # One actual model root, no current animation/resource result substituted.
    t.sh(WEAPON+0x114,52);t.sh(WEAPON+0x11a,0x534)
    root=struct.pack('<13f',1,1,0,0,0,1,0,0,0,1,2,3,4)
    t.g.mem_write(WEAPON+0x500,root)
    t.w(absolute[0x4b6288],QUEUE);t.w(absolute[0x4b6280],0)
    t.g.mem_write(absolute[0x4b9da0],struct.pack('<3f',0,0,0))
    t.g.mem_write(absolute[0x4b9dac],struct.pack('<3f',1,0,0))
    t.run(entries[0x41980],(0,WI))
    count=t.u(absolute[0x4b6280]);assert count==1,count
    raw=bytes(t.g.mem_read(QUEUE,112))
    assert raw[32:36]==struct.pack('<I',0x4f000)
    # Original42840 payload root position/forward/up (the native consumer's
    # matching layout is covered by projectile_light_attachment_tests).
    expected=struct.pack('<9f',2,3,4,1,0,0,0,0,1)
    assert raw[44:80]==expected,(raw.hex(),expected.hex())
    assert raw[80:100]==struct.pack('<5f',1,1,1,1,1)
    for pc,raw_bytes in t.reached.items():assert t.image.read(pc,len(bytes.fromhex(raw_bytes)))==bytes.fromhex(raw_bytes)
    result={'passed':True,'scope':__doc__,'model_present':True,'root_marker':0,
        'actual_definition':tag,'light_volume_commands':count,'command_hex':raw.hex(),
        'owners':owners,'original_xbe_sha256':hashlib.sha256(t.image.data).hexdigest(),
        'original_cache_sha256':hashlib.sha256(cache.data).hexdigest(),
        'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'instruction_patches':[],'supplied_results':[],
        'reached_instruction_bytes':{f'{pc:08x}':b for pc,b in sorted(t.reached.items())}}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'passed':True,'commands':count,'model_present':True,'report':str(args.report)}))


if __name__=='__main__':main()
