#!/usr/bin/env python3
"""Unchanged126930/21B060 and1279D0 control composition in paired images.

Actual Lockout/Triplicate metadata, explicit caller controls/base pitch and a
constant zero class-fade row are ABI inputs. No callee is intercepted. This
verifies these CPU owners, not original scheduler admission or spatial audio.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86_const import X86_OP_MEM,X86_OP_IMM
from oracle_native_footstep_selection import ROOT,Cache,Xbe
from oracle_particle_physics_movement_20261010 import OLD,NEW
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R

def run(path,map_path):
 x=Xbe(path);c=Cache(map_path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 def locate(prefix):
  matches=[]
  for s in x.sections:
   raw=x.section_bytes(s);start=0
   while (at:=raw.find(prefix,start))>=0:matches.append(s.va+at);start=at+1
  assert len(matches)==1,(prefix.hex(),matches)
  return matches[0]
 gain_candidates=[]
 for section in x.sections:
  data=x.section_bytes(section);start=0
  while (at:=data.find(bytes.fromhex('51578bd6e8'),start))>=0:
   if data[at+9:at+13]==bytes.fromhex('8bd68bf8'):gain_candidates.append(section.va+at)
   start=at+1
 assert len(gain_candidates)==1,gain_candidates
 gain=gain_candidates[0];ran=locate(bytes.fromhex('5153550fbe690a568b35'))
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True
 def ins(at,n):return list(md.disasm(bytes(g.mem_read(at,n)),at))
 calls=[i.operands[0].imm for i in ins(gain,0xaa) if i.mnemonic=='call']
 getter,fade=calls[0],calls[-1]
 globals_=[o.mem.disp for i in ins(getter,0x70) for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index]
 matg_va,table_va,ugh_va=globals_[:3]
 fades_va=next(o.mem.disp for i in ins(fade,16) for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index)
 page=c.magic&~4095;g.mem_map(page,(len(c.data)+c.magic-page+4095)//4096*4096);g.mem_write(c.magic,bytes(c.data))
 table,stack,stop,raw,source,fade_rows=0x1000000,0x10f0000,0x10fe000,0x10f1000,0x10f2000,0x10f3000
 g.mem_map(table,0x100000)
 for datum,t in c.tags.items():
  if t['offset'] is not None:g.mem_write(table+(datum&65535)*16+8,struct.pack('<I',t['offset']+c.magic))
 matg=next(t['offset'] for t in c.tags.values() if t['group']=='matg');ugh=next(t['offset'] for t in c.tags.values() if t['group']=='ugh!')
 for a,v in ((matg_va,matg+c.magic),(table_va,table),(ugh_va,ugh+c.magic),(fades_va,fade_rows)):g.mem_write(a,struct.pack('<I',v))
 for n in range(64):g.mem_write(fade_rows+n*16+12,b'\x01')
 # Exact caller instruction span ending at the original range result join.
 composition=locate(bytes.fromhex('0fbe470b8b0d'))
 join=composition+0x44
 rows=[]
 def start(entry,end=stop):
  g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  g.emu_start(entry,end,count=100000);assert g.reg_read(R.UC_X86_REG_EIP)==end
 def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
 for datum,t in c.tags.items():
  if t['group']!='snd!' or t['offset'] is None:continue
  at=t['offset'];count=c.data[at+10]
  if not count:continue
  # Keep authored multi-range rocks and representative single-range impacts,
  # explosives and weapons; no input/result is edited after execution.
  if count==1 and not any(n in t['name'] for n in ('grenade','materials','smg')):continue
  for control in (0.,.01,.1,.25,.5,.75,1.):
   for base in (-1200.,0.,1200.):
    g.mem_write(stack,struct.pack('<3I',stop,0,word(control)));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_ESI,at+c.magic)
    start(gain);volume=g.reg_read(R.UC_X86_REG_EAX)
    g.mem_write(raw+0x10,struct.pack('<f',control));g.mem_write(source+0x90,struct.pack('<f',base))
    for reg,v in ((R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EDI,at+c.magic),(R.UC_X86_REG_EBP,raw),(R.UC_X86_REG_EBX,source)):g.reg_write(reg,v)
    start(composition,join);pitch=g.reg_read(R.UC_X86_REG_XMM1)&0xffffffff;selected=g.reg_read(R.UC_X86_REG_EAX)&65535
    rows.append(('C',datum,word(control),word(base),pitch,selected,volume))
  for pitch in (-32000.,-1200.,-600.,0.,300.,600.,1200.,32000.):
   for previous in range(-1,count):
    g.mem_write(stack,struct.pack('<I',stop));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_ECX,at+c.magic);g.reg_write(R.UC_X86_REG_EAX,previous&0xffffffff);g.reg_write(R.UC_X86_REG_XMM1,word(pitch))
    start(ran);rows.append(('R',datum,previous,word(pitch),g.reg_read(R.UC_X86_REG_EAX)&65535))
 return rows,{'gain':hex(gain),'range':hex(ran),'caller_composition':hex(composition),'caller_join':hex(join),'globals':[hex(v) for v in (matg_va,table_va,ugh_va,fades_va)]}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 pins=['aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377','21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd']
 for path,pin in zip((OLD,NEW),pins):assert hashlib.sha256(path.read_bytes()).hexdigest()==pin
 maps=[ROOT/'assets/disc/maps/lockout.map',Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c/4d53006400000003/triplicate.map')]
 rows=[];reports=[]
 for index,path in enumerate(maps):
  first,entries=run(OLD,path);other,entries_new=run(NEW,path);assert first==other
  reports.append({'map':str(path),'map_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'cases_per_image':len(first),'entries':[entries,entries_new]})
  for row in first:rows.append(' '.join([row[0],str(index),f'{row[1]:08x}']+[str(v) if row[0]=='R' and n==0 else f'{v&0xffffffff:08x}' for n,v in enumerate(row[2:])]))
 data='\n'.join(rows)+'\n';(a.output/'fixtures.tsv').write_text(data)
 report=dict(boundary=__doc__,maps=reports,xbe_sha256=pins,intercepted_instructions=0,paired_results_byte_exact=True,fixture_sha256=hashlib.sha256(data.encode()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
