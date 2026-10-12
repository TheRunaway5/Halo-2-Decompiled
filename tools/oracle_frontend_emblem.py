#!/usr/bin/env python3
"""Freeze original14CCF0 emblem inputs and the retained live22A664 pixel bank.
The authored executable runs unchanged with supplied valid appearance bytes;
only host arenas/pointers are supplied. No native formula computes expected words.
"""
from pathlib import Path
import hashlib,json,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from oracle_multiplayer_hud_positions import Retail,PIN
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBP
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/customization'
r=Retail();appearance=r.alloc(bytes(16));value=r.alloc(bytes(4));hud=r.alloc(bytes(0x220));r.w(0x510c4c,hud);r.w(hud+0x1b4,0xffffffff)
names=[0x0d0005ff,0x170005fa,0x170005fb,0x180005fc,0x1d0005fd,0x1d0005fe]
rows=[]
cases=[(f,f%32,f%16)for f in range(64)]+[(b+32,b,15-b%16)for b in range(32)]+[(63,31,flags)for flags in range(16)]
for foreground,background,flags in cases:
 raw=bytes([10,0,0,10,0,foreground,background,flags,0])+bytes(7)
 r.g.mem_write(appearance,raw)
 for name in names:
  r.run(0x14ccf0,registers=[(UC_X86_REG_EAX,name),(UC_X86_REG_ECX,appearance),(UC_X86_REG_EDX,value)])
  rows.append(f'{foreground} {background} {flags} {name:08x} {r.r(value):08x} 3f800000')
for opacity in [0,.25,.5,.75,1]:
 word=struct.unpack('<I',struct.pack('<f',opacity))[0];r.w(0x4e696c,word)
 for name in [0x0c0005cb,0]:
  r.run(0x22ace7,stop=0x22ae1a,argv=[0,name])
  result=r.r(r.g.reg_read(UC_X86_REG_EBP)-4)
  rows.append(f'0 0 0 {name:08x} {result:08x} {word:08x}')
(OUT/'frontend-emblem-inputs-original.tsv').write_text('# original14CCF0 and22ACE7 pinned XBE '+PIN+'\n# foreground background flags input result_word opacity_word\n'+'\n'.join(rows)+'\n')
live=json.loads((OUT/'original-emblem-draw.json').read_text())
program=bytes.fromhex(live['pixel_program_hex']);assert len(program)==240
(OUT/'frontend-emblem-live-program.bin').write_bytes(program)
report={'xbe_sha256':PIN,'input_cases':len(rows),'primary_method':'unchanged14CCF0 and22ACE7','live_source':'original-emblem-draw.json','live_program_sha256':hashlib.sha256(program).hexdigest(),'live_vertex_slot20':live['ui_vertex_slot20'],'live_appearance':live['appearance_hex']}
(OUT/'frontend-emblem-original.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
