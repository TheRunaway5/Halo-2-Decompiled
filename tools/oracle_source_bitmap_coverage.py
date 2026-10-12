#!/usr/bin/env python3
"""Run original per-part bitmap coverage over immutable actual Zanzibar banks.

Complete4D9C0 ->1BD50 ->1BBF0 ->16B90 prefix stops at16CB3 immediately after
both coverage stores. Actual tag/pass/D3D state is retained; no callee/result
is replaced. Direct16B90 controls use the same actual energy-blade metadata.
Later material/animation/stage/residency submission is an explicit boundary.
"""
from pathlib import Path
import argparse,collections,hashlib,json,random,re,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed,fw
from audit_rendering import Cache
from audit_original_energy_blade_draw import resources
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH,PART,STACK,STOP=0x20000000,0x20001000,0x200fe000,0x200ff000
CONTEXT=0x485b48
ENTRIES=[0x4d9c0,0x1bd50,0x1bbf0,0x16b90,0x16cb3,0x1b0a0,0x143c0,0x12310,0x12ce00]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);a=p.parse_args()
 s=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};dirty=set();instructions={};entries=collections.Counter();calls=[]
 phase=['coverage'];requests=[];service_entries=[];upstream_banks=[]
 def page(address):
  at=address&~4095
  if at not in pages:
   raw=s.read(at,4096);u.mem_map(at,4096);u.mem_write(at,raw);pages[at]=raw
 def read(address,size):
  for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=at<SYNTH+0x100000:page(at)
  return bytes(u.mem_read(address,size))
 def put(address,raw):
  read(address,len(raw));u.mem_write(address,raw)
  for at in range(address&~4095,((address+len(raw)-1)&~4095)+4096,4096):
   if not SYNTH<=at<SYNTH+0x100000:dirty.add(at)
 def missing(g,access,address,size,value,data):
  for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(at)
  return True
 def code(g,pc,size,data):
  assert read(pc,size)==x.read(pc,size),(hex(pc),'changed source instruction');instructions[pc]=size
  if pc in ENTRIES:
   entries[f'{pc:08X}']+=1
   if pc==0x16b90:calls.append(dict(entry=f'{pc:08X}',arguments=words(read(g.reg_read(UC_X86_REG_ESP)+4,28))))
   if pc==0x1b0a0:
    upstream_banks.append(dict(stage_bank_hex=read(CONTEXT+0x1320,256).hex(),
     mask=uint(read(CONTEXT+0x1420,4)),coverage=words(read(CONTEXT+0x28,8)),
     full_context_hex=read(CONTEXT,0x1800).hex()))
   if pc==0x143c0:
    requests.append(dict(stage=uint(read(g.reg_read(UC_X86_REG_ESP)+4,4)),
     bitmap=g.reg_read(UC_X86_REG_EAX),image=g.reg_read(UC_X86_REG_ECX)&65535,
     fractional_request=g.reg_read(UC_X86_REG_XMM2)&0xffffffff))
   if pc==0x12ce00:
    sp=g.reg_read(UC_X86_REG_ESP);record=uint(read(sp+4,4))
    service_entries.append(dict(entry=f'{pc:08X}',record=f'{record:08X}',flags=uint(read(sp+8,4)),
     raw_request=g.reg_read(UC_X86_REG_XMM1)&0xffffffff,record_hex=read(record,116).hex()))
  stop={'coverage':0x16cb3,'stages':0x1b0a0,'request':0x12ce00}[phase[0]]
  if pc==stop:g.emu_stop()
 def write(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(at)
 u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
 ga,gl=(int(v,16)for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',s.report['registers_before']).groups());read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,v in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,16),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),(UC_X86_REG_FS,32)]:u.reg_write(reg,v)
 def original(row,mode='coverage'):
  for at in dirty:u.mem_write(at,pages[at])
  dirty.clear();u.mem_write(SYNTH,bytes(0x10000));calls.clear();requests.clear();service_entries.clear();upstream_banks.clear();phase[0]=mode
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
  if row[0]:
   u.mem_write(STACK,packed([STOP,0,0xe2da0165,0,0,0]));u.mem_write(PART+0x2c,packed(row[2:3]));put(0x4670e4,packed(row[3:4]))
   u.reg_write(UC_X86_REG_ECX,PART);u.reg_write(UC_X86_REG_EDX,15);u.reg_write(UC_X86_REG_EAX,row[1]);entry=0x4d9c0
  else:
   u.mem_write(STACK,packed([STOP,CONTEXT,0xe2da0165,0,15,0,0,row[1]]));entry=0x16b90
  u.emu_start(entry,STOP,count=2000000);assert u.reg_read(UC_X86_REG_EIP)=={'coverage':0x16cb3,'stages':0x1b0a0,'request':0x12ce00}[mode]
  assert len(calls)==1;args=calls[0]['arguments'];assert args[:6]==[CONTEXT,0xe2da0165,0,15,0,0]
  return [args[6],*words(read(CONTEXT+0x28,8))]
 c=Cache(ROOT/'assets/disc/maps/zanzibar.map');_,model=c.tag(0xe2d90164,'mode');authored=[]
 for section,where in enumerate(c.block(model+0x24,92)):
  raw,_=resources(c,where)
  for part in range(len(raw)//72):
   w=uint(raw,part*72+0x2c);v=struct.unpack('<f',packed([w]))[0]
   authored.append(dict(section=section,part=part,raw_scale=f'{w:08X}',resolved_scale=f'{fw(v if v>0 else 1):08X}',part_sha256=sha(raw[part*72:part*72+72])))
 actual_multiplier=uint(s.read(0x4670e4,4));rows=[];labels=[]
 scales=sorted(set(int(v['resolved_scale'],16)for v in authored));multipliers=[actual_multiplier,fw(.3),fw(.25)]
 for scale in scales:
  for multiplier in multipliers:
   for byte in range(256):rows.append([1,byte,scale,multiplier]);labels.append('literal energy-blade part / explicit coverage byte/player policy')
 for metric in[0,0x80000000,1,2,0x007fffff,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc00001,0xffc00001,0x7f800001,0xff800001]:
  rows.append([0,metric,0,0]);labels.append('direct IEEE/zero/unordered/subnormal coverage')
 for power in range(-126,128):
  word=fw(2.**power)
  for metric in[word-1,word,word+1]:rows.append([0,metric,0,0]);labels.append('adjacent reciprocal/log exponent boundary')
 rng=random.Random(0x16b904d9c0)
 for _ in range(4096):rows.append([0,rng.getrandbits(32),0,0]);labels.append('explicit raw metric control')
 all_expected=[original(row)for row in rows]
 upstream=[]
 actual_scale=int(authored[0]['resolved_scale'],16)
 for byte in[0,1,255]:
  row=[1,byte,actual_scale,actual_multiplier];out=original(row,'stages')
  upstream.append(dict(input=row,source_output=out,complete_before_request=upstream_banks.copy(),
   boundary='Actual1B0A0 entry; complete material/animation/stage producers run, residency has not started.'))
 request_out=original([1,255,actual_scale,actual_multiplier],'request')
 request_boundary=dict(input=[1,255,actual_scale,actual_multiplier],source_output=request_out,
  complete_before_request=upstream_banks.copy(),ordered_requests=requests.copy(),actual_request_entry=service_entries.copy(),
  stage_bank_at_boundary_hex=read(CONTEXT+0x1320,256).hex(),
  scope='Stage0 actual ready-cache call returns and commits its minimum. Stage1 stops at actual12CE00(flags3). No allocator/readiness outcome supplied.')
 outside=[];admitted=[];expected=[];admitted_labels=[]
 for row,label,out in zip(rows,labels,all_expected):
  if out[1]==0:
   outside.append(dict(input=row,source_output=out,label=label,
    boundary='FYL2X zero-argument exception is not represented by native host-log arithmetic. No result fitting.'))
  else:admitted.append(row);expected.append(out);admitted_labels.append(label)
 rows,labels=admitted,admitted_labels
 encoded=[' '.join(f'{v:08X}'for v in row)for row in rows]
 result=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
 actual=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()];assert len(actual)==len(expected)
 for i,(native,source)in enumerate(zip(actual,expected)):
  assert native==source,(i,labels[i],rows[i],native,source)
 fixture=ROOT/'tests/fixtures/retail_source_bitmap_coverage.tsv'
 fixture.write_text('# kind(0direct/1part),metric-or-byte,actualpartscale,playerShaderMultiplier | original metric/reciprocal/adjustment\n'+''.join(encoded[i]+' | '+' '.join(f'{v:08X}'for v in expected[i])+'\n'for i in range(len(rows))))
 report=dict(source_xbe_sha256=PIN,tool_sha256=sha(Path(__file__).read_bytes()),native_sha256=sha(a.binary.read_bytes()),fixture_sha256=sha(fixture.read_bytes()),
  snapshot_ram_sha256=sha(s.ram),snapshot_manifest_sha256=sha((s.directory/'export.json').read_bytes()),cache_sha256=sha(c.data),
  cases=len(rows),checked_words=len(rows)*3,original_cases=len(all_expected),unsupported_exception_controls=outside,
  source_entries=dict(entries),unique_instructions=len(instructions),
  instruction_sha256=sha(b''.join(x.read(pc,size)for pc,size in sorted(instructions.items()))),
  configuration=dict(fpcw='023F',mxcsr='1F80',scope='Supplied execution inputs, not inferred live render scheduler.'),
  constants={f'{at:08X}':x.read(at,4).hex()for at in[0x45dbd8,0x45dbc0,0x45de10,0x44a04c]},
  retained_initial_globals={f'{at:08X}':s.read(at,size).hex()for at,size in[(0x4670bc,1),(0x4670e4,4),(0x485b48,0x60)]},
  actual_metadata=dict(shader='E2DA0165',template='E2DB0166',layer=15,lod=0,pass_ordinal=0,implementation=0,parts=authored),
  complete_stage_producer_probes=upstream,actual_request_boundary_probe=request_boundary,
  upstream_context_inputs=dict(context_ordinal=0,packed_context_hex=s.read(0x4ba138,32).hex(),
   shader_clock_hex=s.read(0x4858a0,8).hex(),shader_clock_source_address='4858A0',
   scope='Explicit retained context0 and clock, not proven original8787 owner/phase or a native activation service.'),
  label_counts=dict(collections.Counter(labels)),services_replaced=[],source_instruction_patches=[],
  source_boundary='Primary arithmetic corpus stops at16CB3, immediately after complete16B90 coverage stores +28/+2C. Complete earlier4D9C0/1BD50/1BBF0 executes actual retained tag/D3D banks. Separately labelled diagnostics continue through pass setup, animation and stage bindings to1B0A0, and one reaches actual12CE00. No allocation outcome or cache activation is supplied. No GPU/guest IO.')
 path=ROOT/'analysis/multiplayer-shading/source-bitmap-coverage-original.json';path.write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2))
if __name__=='__main__':main()
