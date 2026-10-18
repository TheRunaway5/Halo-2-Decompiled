#!/usr/bin/env python3
"""Generate the original default builtin3 program; authenticate its debug globals."""
from pathlib import Path
import hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from oracle_smg_firing_1_1 import Xbe
reference=ROOT/'analysis/pstencil/2003-presentation/accumulation-construction-v1/reference.json'
j=json.loads(reference.read_text());image=Xbe(Path(j['original']))
assert hashlib.sha256(Path(j['original']).read_bytes()).hexdigest()==j['original_sha256']
for table,address,name in [(0x4c2f94,0x48f494,b'ssc_texture_diffuse'),(0x4c2fa0,0x48f498,b'ssc_texture_specular')]:
 ptr,kind,dest=struct.unpack('<3I',image.read(table,12));assert kind==6 and dest==address
 assert image.read(ptr,len(name)+1)==name+b'\0' and image.read(address,4)==struct.pack('<f',1.)
row=next(x for x in j['fixtures']if x['builtin']==3 and x['global_48f494']==1 and x['global_48f498']==1)
w=row['program_words'];assert hashlib.sha256(struct.pack('<60I',*w)).hexdigest()==row['program_sha256']
comment='// Original game964B0 builtin3, SSC texture diffuse/specular defaults1.\n// Program SHA256 '+row['program_sha256']+'\n'
rows=['    '+','.join(f'0x{x:08x}u'for x in w[n:n+8]) for n in range(0,60,8)]
(ROOT/'src/pstencil/texture_accumulation_2003_program.inc').write_text(comment+'constexpr std::array<std::uint32_t,60> texture_program_2003{{\n'+',\n'.join(rows)+'\n}};\n')
(ROOT/'shaders/pstencil/texture_accumulation_2003_program.glsl').write_text(comment+'const uint originalTextureProgram[60]=uint[60](\n'+',\n'.join(rows)+'\n);\n')
print(row['program_sha256'])
