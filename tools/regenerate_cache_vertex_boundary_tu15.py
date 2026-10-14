#!/usr/bin/env python3
"""Re-emit the authenticated183E17 native CPU resource boundary only.

The complete surrounding bodies remain byte-for-byte generated code. The
shared generator owns the same rule for later full regeneration.
"""
import hashlib,json,sys
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
c=_Code(x,'.text');i=next(c.md.disasm(x.read(0x183e17,5),0x183e17,count=1))
assert i.mnemonic=='call' and i.operands[0].imm==0x439c0
old='s.push(0x183e17u,selected(0x183e1cu,s.image));run_439c0(s);'
new='s.push(0x183e17u,selected(0x183e1cu,s.image));s.required(0x183e17u,selected(0x439c0u,s.image),false);'
accepted='s.push(0x183e17u,selected(0x183e1cu,s.image));s.required(0x183e17u,0x439c0u,false);'
rows=[]
for name in ('src/physics_live_support_tu15_generated.cpp','src/source_map_world_support_tu15_generated.cpp'):
 p=Path(name);s=p.read_text();assert s.count(old)+s.count(new)+s.count(accepted)==1
 original=s.replace(new,old).replace(accepted,old);s=s.replace(old,new);p.write_text(s)
 rows.append(dict(path=name,prior_call_emission_sha256=hashlib.sha256(original.encode()).hexdigest(),generated_cpp_sha256=hashlib.sha256(s.encode()).hexdigest()))
out=Path('analysis/weapon-effects-20261013/live-tu15/cache-vertex-boundary-generation.json')
out.write_text(json.dumps(dict(original_sha256=NEW_PIN,call_pc='00183e17',call_bytes=i.bytes.hex(),target='000439c0',files=rows),indent=2)+'\n')
print('Re-emitted authenticated native CPU-view boundary in',len(rows),'complete generated bodies')
