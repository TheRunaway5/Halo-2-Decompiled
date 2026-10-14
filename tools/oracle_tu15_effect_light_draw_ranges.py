#!/usr/bin/env python3
"""Replay the unchanged TU1.5 BSP sphere-light indexed-range owner.

Whole C35D0/31C70/40E80 prepare actual commands, then 3BAA0 sorts layer 12.
The post-successful material corridor 4D5D2..4D616 and whole 4DA90 select
original ranges. Only final 1CF30 device flush and 405080 indexed submission
are intercepted. Model commands are retained separately, not treated as BSP.
--include-models retains model ranges in the same original sorted sequence.
This is source range evidence, not material binding or GPU pixel proof.
"""
import argparse,hashlib,json,random,struct,sys,subprocess
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
NONE=0xffffffff;TU15='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--map',type=Path,required=True);p.add_argument('--native',type=Path);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--include-models',action='store_true');p.add_argument('--captured-command-bank',action='store_true',help='Use the bank actually retained at4A3C7 instead of rebuilding the collectors');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);state=json.loads((a.snapshot/'source-state.json').read_text());assert state['reference']=='OG Xbox TU1.5' and state['whole_live_text_matches_original'] and not state['guest_code_patches'];assert sha(s.ram)==state['ram_sha256'];x=Xbe(a.xbe);assert sha(x.data)==TU15
 text=x.section('.text');assert s.read(text.va,text.rsize)==x.read(text.va,text.rsize), 'Live TU1.5 text differs from the unchanged image'
 g=s.guest();stack,stop,out=0x700fe000,0x700fff00,0x70000000;g.mem_map(0x70000000,0x100000)
 def read(at,n):
  regions=list(g.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
  return bytes(g.mem_read(at,n))
 def val(at):return u(read(at,4))
 def write(at,raw):read(at,len(raw));g.mem_write(at,raw)
 def call(entry,end,args=()):
  g.mem_write(stack,pack([stop,*args]));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0);g.emu_start(entry,end,count=30000000);assert g.reg_read(R.UC_X86_REG_EIP)==end,hex(g.reg_read(R.UC_X86_REG_EIP))
 if not a.captured_command_bank:call(0xc35d0,stop);call(0x31c70,0x31177)
 header=0x4ce280;h=words(read(header,24));print("command count",h[1],flush=True)
 write(0x4c6834,pack([1]));call(0x3baa0,stop,[header,0,12,out]);selected=g.reg_read(R.UC_X86_REG_EAX);print("selected",selected,flush=True)
 context=val(0x4cd3a0);base=context+0x25c;original_mask=words(read(base,2048));bsp=val(0x4ecc18);instances=val(bsp+0x144)
 selected_records=[words(read(out+i*24,24))for i in range(selected)];ordered=[];models=[]
 if a.include_models:
  selection=dict(schema='tu15-full-light-selection-v1',snapshot_sha256=sha(s.ram),original_xbe_sha256=sha(x.data),
      header=h,commands=[words(read(h[5]+i*24,24))for i in range(h[1])],
      units=[words(read(0x4c6958+i*32,32))for i in range(val(0x4cd398))],
      records=selected_records,arena_address=base,arena=original_mask,retained_records_are_zero=True)
  selection['model_owners']=[]
  for rec in selected_records:
   command=rec[0]&65535;c=words(read(h[5]+command*24,24));index=c[3]>>8&65535;unit=words(read(0x4c6958+index*32,32))
   if unit[1]!=NONE:selection['model_owners'].append(dict(command=command,unit=index,model=unit[1],
       section=unit[3]>>9&511,object=val(unit[4]),shader=c[2],part=c[3]&255,lod=c[5]>>8&255,pass_index=c[5]&255,
       vertex_class=rec[2]&65535,flags=unit[0]))
  (a.output/'selection.json').write_text(json.dumps(selection,indent=2)+'\n')
 for rec in selected_records:
  command=rec[0]&65535;c=words(read(h[5]+command*24,24));index=c[3]>>8&65535;up=0x4c6958+index*32;unit=words(read(up,32));
  if unit[1]!=NONE:
   models.append(dict(command=command,unit=index,vertex_class=rec[2]&65535,words=unit))
   if not a.include_models:continue
  section=unit[3]>>9&511;kind=0
  if unit[0]&0x1000:kind=1;section=struct.unpack('<H',read(instances+(unit[3]>>18&2047)*88+52,2))[0]
  if unit[1]!=NONE:
   kind=2;mode=val(val(0x4f0414)+(unit[1]&65535)*16+8);metadata=val(mode+0x28)+section*92;geometry=val(metadata+0x34)
  else:metadata=val(bsp+(0x13c if kind else 0xa0))+section*(200 if kind else 176);geometry=val(metadata+0x50)
  part=c[3]&255;pp=val(geometry+4)+part*72;indices=val(geometry+0x24)
  ordered.append(dict(command=command,index=index,vertex_class=rec[2]&65535,unit=unit,up=up,kind=kind,section=section,geometry=geometry,part=part,pp=pp,indices=indices,flags=struct.unpack('<H',read(pp+2,2))[0],subparts=val(geometry+8)))
 executed={};intercepts={'0001cf30':0,'00405080':0};current=[None];submissions=[]
 def ret(arguments=0):
  sp=g.reg_read(R.UC_X86_REG_ESP);g.reg_write(R.UC_X86_REG_EIP,val(sp));g.reg_write(R.UC_X86_REG_ESP,sp+4+arguments*4)
 def hook(vm,pc,size,_):
  if pc in (0x1cf30,0x405080):
   intercepts[f'{pc:08x}']+=1
   if pc==0x405080:
    sp=vm.reg_read(R.UC_X86_REG_ESP);primitive,count,ptr=words(read(sp+4,12));first=(ptr-current[0]['indices'])//2;assert ptr>=current[0]['indices'] and (ptr-current[0]['indices'])%2==0
    indices=list(struct.unpack('<'+'H'*count,read(ptr,count*2)));submissions.append([primitive,first,count,*indices]);ret(3)
   else:ret()
   return
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Changed original instruction{pc:08x}';executed[pc]=raw
 g.hook_add(UC_HOOK_CODE,hook)
 variants=[('actual sorted TU1.5 BSP commands',{})]
 fixture=[0x474e524c,1,len(variants)*len(ordered)];cases=[];rng=random.Random(0x4d5d24daa0)
 for label,v in variants:
  for ordinal,row in enumerate(ordered):
   current[0]=row;unit=row['unit'].copy()
   if 'flags'in v:unit[0]=v['flags']
   if v.get('null_mask'):unit[5]=0
   part_flags=v.get('part_flags',row['flags']);subparts=0 if v.get('zero_subparts')else row['subparts']
   mask=[0]*512 if v.get('mask')=='zero' else [NONE]*512 if v.get('mask')=='one' else [rng.getrandbits(32)for _ in range(512)]if v.get('mask')=='random' else original_mask
   write(row['up'],pack(unit));write(row['pp']+2,struct.pack('<H',part_flags));write(row['geometry']+8,pack([subparts]));write(base,pack(mask));g.mem_write(stack+0x14,pack([row['pp']]));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_EBP,row['up']);g.reg_write(R.UC_X86_REG_ESI,row['geometry']);submissions.clear()
   observed=[]
   def args(vm,pc,size,_):observed.extend([vm.reg_read(R.UC_X86_REG_ECX)&255,vm.reg_read(R.UC_X86_REG_EAX)&255])
   handle=g.hook_add(UC_HOOK_CODE,args,begin=0x4da90,end=0x4da90);g.emu_start(0x4d5d2,0x4d616,count=1000000);g.hook_del(handle);assert g.reg_read(R.UC_X86_REG_EIP)==0x4d616 and len(observed)==2
   returned=g.reg_read(R.UC_X86_REG_EAX);assert returned==sum(d[2]for d in submissions)
   fixture.extend([row['command'],row['index'],row['vertex_class'],row['kind'],row['section'],*unit,row['part'],part_flags,subparts,base,*mask,*observed,returned,len(submissions),*[w for draw in submissions for w in draw]])
   cases.append(dict(label=label,mutation=v,sorted_ordinal=ordinal,command=row['command'],unit=row['index'],kind=row['kind'],section=row['section'],part=row['part'],topology=observed[0],filtered=observed[1],draws=len(submissions),indices=returned))
   write(row['up'],pack(row['unit']));write(row['pp']+2,struct.pack('<H',row['flags']));write(row['geometry']+8,pack([row['subparts']]))
 path=a.output/'fixture.bin';path.write_bytes(pack(fixture));(a.output/'tool.py').write_bytes(Path(__file__).read_bytes());report=dict(boundary=__doc__,command_bank_source='actual captured4A3C7 bank' if a.captured_command_bank else 'replayed C35D0/31C70 collectors',snapshot_sha256=sha(s.ram),original_xbe_sha256=sha(x.data),original_map_sha256=sha(a.map.read_bytes()),reference='OG Xbox TU1.5',original_sorted_commands=[r['command']for r in ordered],separate_model_commands=models,instruction_count=len(executed),instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),intercepted_services=intercepts,cases=cases,draws=sum(c['draws']for c in cases),indices=sum(c['indices']for c in cases),fixture_sha256=sha(path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(path.resolve()),str(a.map.resolve())],capture_output=True,text=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()
