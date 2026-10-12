#!/usr/bin/env python3
"""Complete original unit-constructor-tail instruction bodies and direct calls."""
import argparse,json
from pathlib import Path
from oracle_mp_unit_constructor_tail_1_1 import ROOT,DIRECT,PURE,LOWER
from oracle_smg_firing_1_1 import Xbe,_Code,_trace,PIN
from oracle_mp_unit_constructors_1_1 import save,sha,encoded
import csv
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
image_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(image_path)==PIN
x=Xbe(image_path);code=_Code(x,'.text');inventory=ROOT/'analysis/map-pack-update/functions.tsv';starts={int(q['address'],16)for q in csv.DictReader(inventory.open(),delimiter='\t')};result=[]
for e in DIRECT+PURE+LOWER:
 r=_trace(code,e,starts|set(DIRECT+PURE+LOWER));ins=[code.at(pc)for pc in sorted(r.code)]
 for i in ins:assert x.read(i.address,len(i.bytes))==i.bytes
 name=f'instructions/{e:08x}.asm';save(a.output/name,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
 result.append(dict(entry=f'{e:08x}',rows=len(ins),file=name,sha256=sha(a.output/name),direct_calls=[dict(site=f'{i.address:08x}',target=f'{i.operands[0].imm:08x}')for i in ins if i.mnemonic=='call'and i.operands[0].type==2]))
value=dict(schema='original-unit-constructor-tail-complete-listings-v1',tool_sha256=sha(__file__),source_xbe_sha256=PIN,inventory_sha256=sha(inventory),owners=result,instruction_patches=[],new_guest_inputs=[])
save(a.output/'report.json',encoded(value));print(json.dumps(dict(report=str(a.output/'report.json'),sha256=sha(a.output/'report.json'),owners=len(result))))
