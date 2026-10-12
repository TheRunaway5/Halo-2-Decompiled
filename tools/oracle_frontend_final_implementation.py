#!/usr/bin/env python3
"""Execute unchanged retail layer3 collector override after15D70 selector.
This freezes4CC70→4CE1F, including literal dispatch table and outputbytewrite.
Only supplied collector/descriptor storage is relocated; no function service,
source instruction, classification or policy output is replaced.
"""
from pathlib import Path
import json,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from oracle_multiplayer_hud_positions import Retail,PIN,STACK
from unicorn.x86_const import UC_X86_REG_ESI,UC_X86_REG_EBX
ROOT=Path(__file__).resolve().parents[1];D=ROOT/'analysis/frontend/customization'
r=Retail();record=r.alloc(bytes(32));out=r.alloc(bytes(32));rows=[]
for classification in range(8):
 for initial in range(5):
  flags=classification<<29;r.w(record,flags);r.w(STACK+0x34,out)
  r.run(0x4cc70,stop=0x4ce1f,registers=[(UC_X86_REG_ESI,record),(UC_X86_REG_EBX,initial)])
  selected=int.from_bytes(r.g.mem_read(out+4,1),'little')
  rows.append(dict(classification=classification,initial=initial,selected=selected))
assert len(rows)==40
assert all(x['selected']==(x['classification'] if x['classification']<5 else 3)for x in rows)
live=json.loads((D/'original-preview-final-implementation-spartan.json').read_text())
actual=[x for x in live['rows']if x['pc']=='0004ce0f'and x['layer']==3]
assert len(actual)==4 and all(x['classification']==2 and x['registers']['ebx']=='00000002'for x in actual)
report=dict(method=__doc__,xbe_sha256=PIN,live_fixture='original-preview-final-implementation-spartan.json',
 source_start='0004CC70',source_stop='0004CE1F',cases=rows,live_four_part_override_count=len(actual),
 bound_object='E27F0010',bound_object_definition='E23400BF',bound_shader='E23700C2',bound_implementation=2)
(D/'frontend-final-implementation-original.json').write_text(json.dumps(report,indent=2)+'\n')
print('40 original layer3 classification overrides exact; four live appearance parts3→2 then finalimpl2')
