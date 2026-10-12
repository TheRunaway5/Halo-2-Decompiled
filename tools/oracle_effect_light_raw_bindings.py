#!/usr/bin/env python3
"""Retain actual sorted sphere pass/program/palette inputs from original RAM.

Unchanged40E90/3BAB0 establish the thirteen commands. Literal VRTX bytes and
declarations are read independently from the retained Xbox RAM. Static shader
text comes from the pinned original Xemu decoder, as a verification fixture
only. Resource residency,1CAA0 success and original GPU output remain separate.
"""
import argparse, hashlib, importlib.util, json, struct, sys
from pathlib import Path
from unicorn import x86_const as R
from unicorn import UC_HOOK_CODE
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
def sha(b): return hashlib.sha256(b).hexdigest()
def pack(v): return struct.pack('<'+'I'*len(v),*v)
def words(b): return list(struct.unpack('<'+'I'*(len(b)//4),b))
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
 x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 source=ROOT/'analysis/frontend/title-camera-matrix-audit/generate_world_vertex_straight_line.py'
 spec=importlib.util.spec_from_file_location('retained_xemu_decoder',source);decoder_module=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder_module)
 ref=ROOT/'analysis/rendering-reference/vsh-prog.c'
 pinned=ROOT/'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/hw/xbox/nv2a/pgraph/glsl/vsh-prog.c'
 assert ref.read_bytes()==pinned.read_bytes()
 decoder=decoder_module.Decoder(ref.read_text())
 g=s.guest();g.mem_map(0x70000000,0x100000);stack,stop,out=0x700fe000,0x700fff00,0x70000000
 executed={}
 def trace(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Original instruction changed at{pc:08x}';executed[pc]=raw
 g.hook_add(UC_HOOK_CODE,trace)
 def read(at,n):
  regions=list(g.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
  return bytes(g.mem_read(at,n))
 def val(at):return u(read(at,4))
 def call(entry,end=stop,args=()):
  g.mem_write(stack,pack([stop,*args]));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x23f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0);g.emu_start(entry,end,count=30000000);assert g.reg_read(R.UC_X86_REG_EIP)==end
 call(0xc15a0);call(0x31c80,0x31197);read(0x4ba014,4);g.mem_write(0x4ba014,pack([1]));call(0x3bab0,args=[0x4c1a60,0,12,out]);assert g.reg_read(R.UC_X86_REG_EAX)==13
 bsp=val(0x4e0348);table=val(0x4e3b44);commands=val(0x4c1a60+20);instances=val(bsp+0x144)
 manifest=['H2_SPHERE_RAW_BINDINGS_V1'];programs={};rows=[];files={}
 def save(name,raw):
  path=a.output/name
  if path.exists():assert path.read_bytes()==raw
  else:path.write_bytes(raw)
  files[name]=dict(bytes=len(raw),sha256=sha(raw))
 for ordinal in range(13):
  rec=words(read(out+ordinal*24,24));command=rec[0]&65535;c=words(read(commands+command*24,24));unit=words(read(0x4ba138+(c[3]>>8&65535)*32,32));assert unit[1]==0xffffffff
  kind=int(bool(unit[0]&0x1000));section=unit[3]>>9&511;instance=unit[3]>>18&2047 if kind else 0xffffffff
  if kind:section=struct.unpack('<H',read(instances+instance*88+52,2))[0]
  part=c[3]&255;shader=c[2];lod=c[5]>>8&255;assert c[5]&255==0
  implementation=rec[4];vertex=val(implementation+0x100);classification=rec[2]&65535
  vr=val(table+(vertex&65535)*16+8);assert classification<val(vr+4);entry=val(vr+8)+classification*28
  code=read(val(entry+16),val(entry+12));semantics=read(val(entry+8),val(entry+4)*2)
  key=f'{vertex:08x}-class{classification}';save(key+'.tokens.bin',code);save(key+'.declaration.bin',semantics)
  count=struct.unpack_from('<H',code,2)[0];assert code[:2]==b'\x78\x20' and len(code)==4+count*16
  body='';calls=[]
  for i in range(count):
   token=struct.unpack_from('<4I',code,4+i*16);text,helpers=decoder.token(token);body+=f'  /* Slot {i}: '+ ' '.join(f'0x{v:08X}'for v in token)+' */\n'+text+'\n';calls+=helpers
  save(key+'.semantic.glsl',body.encode());programs[key]=dict(vertex=vertex,classification=classification,slots=count,helper_calls=len(calls))
  palette=b''
  if kind:
   # Original4D830 instance matrix layout; retain the separate scale field
   # independently rather than premultiplying it into the uploaded rows.
   matrix=read(instances+instance*88,52);f=struct.unpack('<13f',matrix)
   palette=struct.pack('<12f',*(f[1+row+col*3]for row in range(3)for col in range(4)))
  save(f'command-{command}-palette.bin',palette)
  row=[command,kind,section,instance,part,shader,lod,implementation,vertex,classification,key]
  manifest.append(' '.join(map(str,row)));rows.append(row)
 save('manifest.tsv',('\n'.join(manifest)+'\n').encode())
 program_manifest=['vertex_shader\tclassification\tprogram\tsemantic\tslots\thelper_calls']
 for key,row in sorted(programs.items()):program_manifest.append(f"{row['vertex']}\t{row['classification']}\t{key}.tokens.bin\t{key}.semantic.glsl\t{row['slots']}\t{row['helper_calls']}")
 save('programs.tsv',('\n'.join(program_manifest)+'\n').encode())
 report=dict(boundary=__doc__,snapshot_sha256=sha(s.ram),original_xbe_sha256=sha(x.data),map_sha256=sha((ROOT/'assets/disc/maps/lockout.map').read_bytes()),decoder_tool_sha256=sha(source.read_bytes()),original_decoder_sha256=sha(ref.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),executed_instructions=len(executed),instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),intercepted_instructions=0,programs=programs,rows=rows,files=files)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');(a.output/'tool.py').write_bytes(Path(__file__).read_bytes());print('PASS',len(rows),'original sorted raw inputs /',len(programs),'actual sphere programs')
if __name__=='__main__':main()
