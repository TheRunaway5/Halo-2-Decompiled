#!/usr/bin/env python3
"""Compare current-row 37A60 submission against the complete unchanged caller and SDK.

Entry is derived from the retained original 36B50 snapshot, with no service,
instruction, GPU-readiness or MMIO replacement. Controls are explicit input
experiments, not original observed histories or original rendered references.
"""
import argparse,hashlib,importlib.util,json,random,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
REF=ROOT/'analysis/multiplayer-shading/weather-plate-program-admission'
sys.path.insert(0,str(ROOT/'tools'))
spec=importlib.util.spec_from_file_location('original_plate_caller',REF/'derive-caller.py');original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from audit_rendering import Cache

def words(b):return list(struct.unpack('<'+'I'*(len(b)//4),b))
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def fw(v):return words(struct.pack('<f',v))[0]
def fput(b,at,v):struct.pack_into('<f',b,at,v)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--write-fixture',type=Path);args=p.parse_args()
 snapshot=original.load(ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-full-entry');image=original.Xbe(ROOT/'assets/disc/default.xbe')
 assert hashlib.sha256(image.data).hexdigest()==original.PIN
 prefix=original.Replay(snapshot,image);original.initialize(prefix,snapshot.report['registers_before']);pr=prefix.run(0x36b50,0x37a60,4_000_000);assert pr['complete'],pr
 sp=prefix.u.reg_read(UC_X86_REG_ESP);stop,plate=words(prefix.read(sp,8));actual=prefix.read(plate,0x3a8);context=prefix.u.context_save();seed={at:prefix.read(at,4096) for at in prefix.pages}
 constants={a:words(image.read(a,4))[0] for a in [0x45dbbc,0x45dbc0,0x45dbcc,0x45dd40]};assert list(constants.values())==[fw(.5),fw(1),fw(-1),fw(16777215)]
 # Literal authored Warlock fields with explicitly controlled copied view rows.
 c=Cache(ROOT/'assets/map-pack/maps/warlock.map');tag=next(t for t in c.tags.values() if t['group']=='weat');_,at=c.tag(tag['datum'],'weat');authored=c.block(at+8,0x3a8)[0];warlock=bytearray(c.data[authored:authored+0x3a8]);warlock[0xcc:]=actual[0xcc:];warlock[:24]=actual[:24] # Preserve actual retained bitmap services in the Lockout snapshot.
 cases=[('actual-derived-entry',actual,words(prefix.read(0x485600,4))[0],prefix.read(0x485648,8),words(prefix.read(0x485658,8))+words(prefix.read(0x485ad4,8)),False)]
 rects=[(0,0,480,640),(0,0,720,1280),(0,0,360,640),(360,640,720,1280),(10,20,491,661)]
 for name,template in [('lockout',actual),('warlock-authored-arithmetic-retained-lockout-textures-controlled-rows',warlock)]:
  for slot in range(4):
   for rect in rects:
    for variant in range(4):
     b=bytearray(template);v=slot*0xb8
     for layer in range(3):
      fput(b,v+0x11c+layer*8,(-23.25+layer*13.125)*(variant+1));fput(b,v+0x120+layer*8,(11.5-layer*7.75)*(variant+1))
      fput(b,v+0x134+layer*4,[.25,1.5,-.75,0.][variant]*(layer+1))
      fput(b,v+0xec+layer*4,[-.25,.25,.75,1.25][(variant+layer)%4]);fput(b,0x98+layer*4,[-.125,.5,1.5][layer])
     fput(b,v+0x148,[0,.25,1,2][variant]);fput(b,v+0x14c,[1,2,.5,1][variant])
     if variant==0:fput(b,0x84,struct.unpack_from('<f',b,0x80)[0]) # Original NoDraw, scroll still emitted.
     banks=[fw(.015625),fw(1024),fw(.015625),fw(1024)]
     if variant==2:banks[2:]=[fw(.125),fw(512)]
     if variant==3:banks[0]=words(b[0x80:0x84])[0] # Source near/depth-disabled profile, quad retained.
     cases.append((f'{name}-slot{slot}-rect{rect}-variant{variant}',b,slot,struct.pack('<4h',*rect),banks,True))
 inputs=[];expected=[];records=[];seen=set()
 for index,(label,b,slot,rect,banks,control) in enumerate(cases):
  replay=original.Replay(snapshot,image,seed,context,stop)
  for at,raw in [(plate,b),(0x485600,pack([slot])),(0x485648,rect),(0x485658,pack(banks[:2])),(0x485ad4,pack(banks[2:]))]:replay.read(at,len(raw));replay.u.mem_write(at,bytes(raw))
  result=replay.run(0x37a60,stop,1_000_000);assert result['complete'],(label,result)
  comp=next(e for e in replay.events if e['PC']=='0003A5C0');ret=next(e for e in replay.events if e['PC']=='00037DB6');success=int(ret['registers']['EAX'],16)&255
  out=[success,0,0]+[0]*60;compositor=words(bytes.fromhex(comp['source_compositor52_hex']));compositor[0]&=255
  if success:
   depth=int(ret['depth_word'],16);source_depth=struct.unpack('<f',pack([depth]))[0];near=struct.unpack('<f',pack([banks[0]]))[0]
   out[1]=1 if source_depth>near else 2;out[2]=depth;out[3:63]=[int(w,16) for w in ret['program_words']]
  else:assert words(replay.read(0x484f68,240))==words(prefix.read(0x484f68,240))
  out+=compositor
  scroll={e['constant_index']:[int(w,16) for w in e['constant_XYZW_words']] for e in replay.events if e['PC']=='003F71D0'};assert set(scroll)=={21,22,23}
  out+=sum((scroll[i] for i in [21,22,23]),[])
  registers=[[0]*4 for _ in range(5)];vertices=[]
  for e in replay.events:
   if e['PC']=='003FB280':
    register,x,y=[int(w,16) for w in e['vertex_register_XY_words']];registers[register]=[x,y,0,fw(1)]
   elif e['PC']=='003FB2D0':
    registers[0]=[int(w,16) for w in e['position_XYZW_words']];vertices+=sum(registers,[])
  assert len(vertices)==(80 if success else 0);out+=vertices if success else [0]*80;assert len(out)==168
  inp=words(b)+[slot]+words(rect)+banks+[prefix.u.reg_read(UC_X86_REG_FPCW),prefix.u.reg_read(UC_X86_REG_MXCSR)];assert len(inp)==243
  inputs.append(inp);expected.append(out);seen.update(replay.seen);records.append(dict(label=label,controlled_input=control,profile=out[1],complete=True,instructions=result['instructions'],original_return_AL=replay.u.reg_read(original.UC_X86_REG_EAX)&255))
  if index%40==0:print(json.dumps({'original_cases':index+1,'total':len(cases)}),flush=True)
 payload=''.join(' '.join(f'{w:08x}' for w in row)+'\n' for row in inputs);native=subprocess.run([str(args.binary.resolve())],input=payload,text=True,capture_output=True,check=True);actual_rows=[[int(w,16) for w in line.split()] for line in native.stdout.splitlines()];assert len(actual_rows)==len(expected)
 mismatches=[dict(case=i,label=records[i]['label'],word=j,source=f'{want:08x}',native=f'{have:08x}') for i,(wantrow,haverow) in enumerate(zip(expected,actual_rows)) for j,(want,have) in enumerate(zip(wantrow,haverow)) if want!=have]
 assert all(len(row)==168 for row in actual_rows)
 report=dict(passed=not mismatches,cases=len(cases),compared_words=len(cases)*168,mismatches=mismatches[:80],original_XBE_sha256=original.PIN,native_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),prefix=pr,distinct_original_instructions=len(seen),constants={f'{a:08x}':f'{v:08x}' for a,v in constants.items()},records=records,boundary=__doc__)
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
 if args.write_fixture and not mismatches:
  args.write_fixture.write_text('# Complete unchanged 37A60/SDK submission; 243 input and168 output words.\n'+''.join('# '+r['label']+'\n'+' '.join(f'{w:08x}' for w in inp)+' | '+' '.join(f'{w:08x}' for w in out)+'\n' for r,inp,out in zip(records,inputs,expected)))
 print(json.dumps({k:report[k] for k in ['passed','cases','compared_words','distinct_original_instructions','mismatches']}),flush=True);raise SystemExit(0 if report['passed'] else 1)
if __name__=='__main__':main()
