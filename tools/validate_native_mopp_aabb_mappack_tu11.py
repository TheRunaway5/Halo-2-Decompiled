#!/usr/bin/env python3
"""Qualify native MOPP image operands on unchanged MapPack1.1 code.

Both owning contexts and their raw/specialized production prefixes compare
against the genuine older image; data pointers and real callee targets keep
their older image identities rather than borrowing TU1.5 outcomes.
"""
import copy
import inspect
import json
from pathlib import Path
import sys
import validate_native_mopp_aabb_abi_tu15 as base
from oracle_smg_firing_1_1 import PIN

ORIGINAL_CORPUS=base.corpus
LEGACY_REGIONS=(*base.REGIONS[:2],(0x487000,0x1000))


def corpus(image):
    rows=ORIGINAL_CORPUS(image)
    for row in rows:
        row['entry']=0x305e40
        row['stop_before_entries']=[{0x2e3250:0x2df780,0x2e3080:0x2df5b0,0x2e30e0:0x2df610,0x309e9e:0x30639e}[pc]for pc in row['stop_before_entries']]
        bank=row['banks'][2];assert bank['address']==0x48c000
        data=bytearray.fromhex(bank['before']);bank['address']=0x487000
        if row['name'].startswith('diagnostic_entry_'):
            data[0x69c:0x6a0]=data[0x99c:0x9a0]
            heap=bytearray.fromhex(row['banks'][0]['before']);heap[0x2904:0x2908]=(0x2df5b0).to_bytes(4,'little');row['banks'][0]['before']=heap.hex()
        bank['before']=data.hex()
    return rows


def main():
    source=inspect.getsource(base.main)
    start=source.index('        image=Xbe(args.xbe);')
    end=source.index("        fixture=scratch/'mopp-aabb-abi.json';",start)
    # Only the genuine-original metadata/observation block changes address
    # namespace. Bindings and their whole TU1.5/paired-source pins stay exact.
    physical=source[start:end]
    for actual in(0x309940,0x309960,0x309967,0x309e9e,0x309f2c):physical=physical.replace(hex(actual),hex(actual-0x3b00))
    physical=physical.replace('regions=REGIONS','regions=LEGACY_REGIONS')
    source=source[:start]+physical+source[end:]
    old='dict(cases=rows,io_sites=io,reference_tables=dict(address=0x309f30,bytes=image.read(0x309f30,0xec).hex()))'
    new='dict(cases=rows,legacy_image=True,io_sites=[pc+0x3b00 for pc in io],reference_tables=dict(address=0x306430,bytes=image.read(0x306430,0xec).hex()))'
    if source.count(old)!=1:raise ValueError('Shared MOPP fixture seam changed')
    source=source.replace(old,new)
    source=source.replace('owner(s,0x2e3250u,','owner(s,selected(0x2df780u,s.image),')
    source=source.replace("proof_files=('","proof_files=('tools/validate_native_mopp_aabb_mappack_tu11.py','",1)
    source=source.replace("report.update(passed=True,result=modes", "report.update(production_legacy_admitted=True,original_image_sha256=PIN,passed=True,result=modes")
    env=dict(base.__dict__,PIN=PIN,corpus=corpus,LEGACY_REGIONS=LEGACY_REGIONS,__doc__=__doc__)
    exec(source,env)
    return env['main']()


if __name__=='__main__':raise SystemExit(main())
