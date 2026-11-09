#!/usr/bin/env python3
"""Read genuine initialized allocator owners without executing or supplying them."""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import struct
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if (seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
from validate_native_array_growth_tu15 import ARCHIVE,MEMBER,NEW_DEFAULT,NEW_PIN,Xbe,reference_inputs
from generate_physics_live_support_tu15 import complete_body
from oracle_smg_firing_1_1 import _Code


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    image=Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest()!=NEW_PIN:raise ValueError('Wrong original TU1.5 image')
    banks,proof=reference_inputs(image)
    manifest=json.loads(lzma.decompress(ARCHIVE.with_name('all-file-hashes.json.xz').read_bytes()))
    def member(name):
        key=MEMBER+name;raw=subprocess.check_output(['tar','--zstd','-xOf',str(ARCHIVE),key]);row=manifest[key]
        if len(raw)!=row['bytes'] or hashlib.sha256(raw).hexdigest()!=row['sha256']:raise ValueError('Initialized input member drift')
        return raw
    capture=json.loads(member('capture.json'));used={}
    def read(p,n):
        for first,raw in banks.items():
            if first<=p and p+n<=first+len(raw):return raw[p-first:p-first+n]
        for region in capture['regions']:
            if region['address']<=p and p+n<=region['address']+region['size']:
                name=region['before_file'];raw=member(name);banks[region['address']]=raw
                used[name]=manifest[MEMBER+name];return raw[p-region['address']:p-region['address']+n]
        if 0x10000<=p and p+n<=0x800000:return image.read(p,n)
        raise ValueError(f'No initialized owner bank at{p:x}')
    def word(p):return struct.unpack('<I',read(p,4))[0]
    # Globals are mutable initialized inputs; static vtables are authenticated
    # executable-image DATA. Do not replace mutable cells with original .data.
    def captured_word(p):
        for first,raw in banks.items():
            if first<=p and p+4<=first+len(raw):return struct.unpack_from('<I',raw,p-first)[0]
        return word(p)
    heaps={name:captured_word(cell) for name,cell in (('fixed',0x486068),('primary',0x48606c),('secondary',0x486070))}
    owners={}
    for name,heap in heaps.items():
        if not heap:owners[name]=dict(heap=0,available=False);continue
        table=word(heap)
        owners[name]=dict(heap=heap,vtable=table,allocation=word(table),release=word(table+4),
            virtual_words=[word(table+i*4) for i in range(6)])
        if name=='secondary':owners[name].update(start=word(heap+0x18),size=word(heap+0x1c))
    code=_Code(image,'.text');body=complete_body(code,0x149930);listing=[];calls=[];static_cells=set()
    for pc in sorted(body.code):
        ins=code.at(pc);listing.append(f'{pc:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}')
        if ins.mnemonic in ('call','jmp') and ins.operands and ins.operands[0].type==2:calls.append(dict(site=pc,target=ins.operands[0].imm&0xffffffff))
        for op in ins.operands:
            if op.type==3 and not op.mem.base and not op.mem.index:static_cells.add(op.mem.disp&0xffffffff)
    cells={};missing=[]
    for p in sorted(static_cells):
        region=next((r for r in capture['regions'] if r['address']<=p and p+4<=r['address']+r['size']),None)
        if region is None:missing.append(f'{p:x}');continue
        raw=member(region['before_file']);used[region['before_file']]=manifest[MEMBER+region['before_file']]
        cells[f'{p:x}']=struct.unpack_from('<I',raw,p-region['address'])[0]
    report=dict(schema='native-raw-allocator-owner-audit-v1',original_xbe_sha256=NEW_PIN,input_provenance=proof,
        additional_input_members=used,initialized_mode=captured_word(0x486064),owners=owners,
        lazy_default_owner=dict(entry=0x149930,listing=listing,listing_sha256=hashlib.sha256('\n'.join(listing).encode()).hexdigest(),
            literal_edges=calls,captured_static_cells=cells,uncaptured_static_cells=missing),
        original_or_native_execution=False,owner_outcomes_supplied=False,retained_large_artifacts=[],
        owned_scratch=os.environ['HALO2_TEST_SCRATCH'],
        source_sha256={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in
            (Path(__file__).resolve(),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/generate_physics_live_support_tu15.py')})
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(mode=report['initialized_mode'],owners=owners,lazy_default_cells=cells,literal_edges=calls)))


if __name__=='__main__':main()
