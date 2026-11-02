#!/usr/bin/env python3
"""Compare unchanged TU1.5 stream construction with native CPU and SDL services.

The complete2B8370 CPU caller and literal codec methods execute unchanged.
3875C2 creation/3858F8 pause are explicit native test ABI boundaries backed by
actual manual SDL voices, not executions or qualified results of the retail SDK.
Legacy constructor cases declare descriptors/formats and raw stream rows.
--format-chain executes original224E20/225EF0 and the private conversion over
immutable1.0/2.0 operands with contiguous logical x87 headroom. Raw Unicorn C1
differences are retained and that bit has an independent native opcode oracle.
Driver/World admission, dead physical register and Xbox equivalence remain open.
"""
import argparse,collections,ctypes,hashlib,json,os,random
from pathlib import Path

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bridge',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--case',type=int,action='append');p.add_argument('--format-chain',action='store_true',help='Exercise original224E20/225EF0 and private conversion, including live logical x87 state');args=p.parse_args()
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 try:
  from oracle_sound_stream_packets_tu15 import Xbe,NEW_DEFAULT,NEW_PIN,REGS,STACK,STOP
  from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
  from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW,UC_X86_REG_XMM0
 except ModuleNotFoundError:print('Constructor reference requires the declared Unicorn/Capstone Python');return 77
 if not NEW_DEFAULT.is_file():print('Authenticated TU1.5 image unavailable');return 77
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 library=ctypes.CDLL(str(args.bridge.resolve(strict=True)))
 for name in ['h2_construct_native','h2_construct_provider','h2_format_x87_primitives']:
  method=getattr(library,name);method.argtypes=[ctypes.c_char_p];method.restype=ctypes.c_char_p
 def invoke(name,row):
  result=json.loads(getattr(library,name)(json.dumps(row,separators=(',',':')).encode()))
  if 'error'in result:raise ValueError((name,result['error']))
  return result
 driver,desc,format_address=0x4000000,0x4100000,0x4200000
 create,pause,device_binding,voice_base=0x3875c2,0x3858f8,0x7600000,0x7100000
 pcm,adpcm=0x48b8e8,0x48b908
 assert int.from_bytes(image.read(int.from_bytes(image.read(pcm,4),'little'),4),'little')==0x2b7f40
 assert int.from_bytes(image.read(int.from_bytes(image.read(adpcm,4),'little'),4),'little')==0x2e4d60
 rng=random.Random(0x2b8370);categories=[0,1,2,3,7,8,255,256,65535,65536,65538,0xffff0002,0x80000000,0xffffffff];specs=[]
 for codec in [adpcm,pcm]:
  for flags in range(256):specs.append(dict(entry=0x2b8370,codec=codec,flags=flags,category=categories[flags%len(categories)]))
  for index in range(82):specs.append(dict(entry=0x2b8370,codec=codec,index=index,flags=(index*17)&255,category=index%3))
  for category in categories:
   for previous in [False,True]:specs.append(dict(entry=0x2b8370,codec=codec,category=category,removed_before_create=True,previous_voice=previous))
  for alias in ['descriptor-is-stream','descriptor-is-codec-slot']:
   for category in categories:specs.append(dict(entry=0x2b8370,codec=codec,category=category,alias=alias))
  for fault in ['stream','codec','vtable','descriptor','driver']:
   for category in [0,1,2,0xffffffff]:specs.append(dict(entry=0x2b8370,codec=codec,category=category,fault=fault))
 for n in range(64):specs.append(dict(entry=0x2b7f40,codec=pcm,category=n,fault='codec'if n%7==0 else None))
 if args.format_chain:
  specs=[]
  for category in range(256):specs.append(dict(entry=0x224e20,codec=adpcm,category=category,depth=category%7,top=category%8))
  for category in [0,1,2,3,0xffff0002,0xffffffff]:
   for precision in [0,2,3]:
    for rounding in range(4):
     for depth in [0,3,6]:specs.append(dict(entry=0x224e20,codec=adpcm,category=category,depth=depth,top=(depth+rounding)&7,cw=0x7f|(precision<<8)|(rounding<<10)))
  for alias in ['queue-is-format','queue-is-saved-ecx']:
   for category in [0,1,2,0xffffffff]:specs.append(dict(entry=0x224e20,codec=adpcm,category=category,depth=3,alias=alias))
  for fault in ['format','output']:
   for category in [0,1,2,0xffff0002]:specs.append(dict(entry=0x224e20,codec=adpcm,category=category,depth=6,fault=fault))
  for index in range(82):specs.append(dict(entry=0x225ef0,codec=adpcm,index=index,category=index%3,depth=index%7,top=index%8))
  for category in [0,1,2]:
   for precision in [0,2,3]:
    for rounding in range(4):
     for depth in [0,3,6]:specs.append(dict(entry=0x225ef0,codec=adpcm,category=category,depth=depth,top=(depth+rounding)&7,cw=0x7f|(precision<<8)|(rounding<<10)))
  for category in [0,1,2,0xffff0002]:
   for previous in [False,True]:specs.append(dict(entry=0x225ef0,codec=adpcm,category=category,depth=3,removed_before_create=True,previous_voice=previous))
  for fault in ['driver','vtable']:
   for category in [0,1,2]:
    for depth in [0,6]:specs.append(dict(entry=0x225ef0,codec=pcm if category==2 else adpcm,category=category,depth=depth,fault=fault))
 if args.case is not None and any(n<0 or n>=len(specs)for n in args.case):p.error('Case outside declared matrix')
 sites=collections.Counter();counts=collections.Counter();calls_count=collections.Counter();returns=faults=stores_count=created=pauses=successful_pauses=0
 input_digest=hashlib.sha256();trace_digest=hashlib.sha256();raw_fp_digest=hashlib.sha256();primitive_digest=hashlib.sha256();c1_checked=raw_c1_differences=0
 report=dict(passed=False,goal_complete=False,scope=__doc__,original_xbe_sha256=NEW_PIN,source_instruction_patches=[],native_service_boundaries=[create,pause],selected_cases=args.case,complete_declared_matrix=args.case is None,
  native_test_abi=dict(device='Declared binding to actual opened manual SDL device',creation='Actual SDL create_voice with unity gain/rate and initially paused routes; write output token only on actual success',pause='Actual SDL parameters with paused=false; attempts include failed creation and retained prior output slots',returns='EAX actual native HRESULT mapping; other GPRs preserved by this declared ABI',format='Caller-supplied authenticated-shape PCM/ADPCM format, not execution of225EF0 or224E20'),
  original_driver_world_cache_voice_admission=False,production_native_handoff=False,retail_sdk_dsp_xbox_equivalence=False,
  format_chain=args.format_chain,private_conversion_scope='Original immutable1.0/2.0 format operands with masked controls and contiguous logical x87 stack/headroom; no global32CC0C or dead physical register claim'if args.format_chain else None)
 if args.format_chain:report['native_test_abi']['format']='Original224E20 format and225EF0 caller execute; private32CC0C frame and logical x87 state are compared'
 args.output.parent.mkdir(parents=True,exist_ok=True)
 try:
  for ordinal,spec in enumerate(specs):
   if args.case is not None and ordinal not in args.case:continue
   rng=random.Random(0x2b8370+ordinal)
   record=driver+12+spec.get('index',0)*0x34;descriptor=record if spec.get('alias')=='descriptor-is-stream'else record+0x30 if spec.get('alias')=='descriptor-is-codec-slot'else desc
   codec=spec['codec'];category=spec.get('category',0);fields=[]
   def w(at,n,v):fields.append([at,n,v&((1<<(8*n))-1)])
   for at in range(STACK-0x80,STACK+0x20,4):w(at,4,rng.getrandbits(32))
   for at,v in [(0x52d674,driver),(driver+0x2ab0,device_binding),(record+0x24,voice_base+16 if spec.get('previous_voice')else 0),(pcm,int.from_bytes(image.read(pcm,4),'little')),(adpcm,int.from_bytes(image.read(adpcm,4),'little'))]:w(at,4,v)
   for object_address in [pcm,adpcm]:
    vtable=int.from_bytes(image.read(object_address,4),'little')
    for n in range(9):w(vtable+n*4,4,int.from_bytes(image.read(vtable+n*4,4),'little'))
   w(pcm+4,4,rng.getrandbits(32));w(record+1,1,rng.randrange(256));w(record+2,1,spec.get('flags',0));w(record+3,1,rng.randrange(256));w(record+0x28,4,rng.getrandbits(32));w(record+0x1c,4,rng.getrandbits(32));w(record+0x20,4,rng.getrandbits(32))
   for n,v in enumerate([rng.getrandbits(32),2,format_address,0x227140,spec.get('index',0),0]):w(descriptor+n*4,4,v)
   channels=2 if codec==pcm or ordinal&1 else 1
   for at,n,v in ([(0,2,1),(2,2,2),(4,4,44100),(8,4,176400),(12,2,4),(14,2,16),(16,2,0),(18,2,0xa5a5)]if codec==pcm else [(0,2,0x69),(2,2,channels),(4,4,44100),(8,4,36*channels*689),(12,2,36*channels),(14,2,4),(16,2,2),(18,2,64)]):w(format_address+at,n,v)
   w(STACK,4,STOP);w(STACK+4,4,spec.get('index',0)if spec['entry']==0x225ef0 else category if spec['entry']==0x2b8370 else record)
   gp=[rng.getrandbits(32)for _ in REGS];gp[4]=STACK;gp[0]=record;gp[1]=codec;gp[7]=descriptor
   if spec['entry']==0x224e20:
    gp[0]=category;gp[6]=format_address;gp[7]=format_address if spec.get('alias')=='queue-is-format'else STACK-4 if spec.get('alias')=='queue-is-saved-ecx'else desc
   elif spec['entry']==0x225ef0:gp[3]=category
   fault=spec.get('fault')
   if fault=='stream':gp[0]=0
   elif fault=='codec':gp[1]=0
   elif fault=='vtable':w(codec,4,0)
   elif fault=='descriptor':gp[7]=0
   elif fault=='driver':w(0x52d674,4,0)
   elif fault=='format':gp[6]=0
   elif fault=='output':gp[7]=0
   xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)];fp=[[0x27f,0x37f,0x77f,0xb7f,0xf7f][ordinal%5],[0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0][ordinal%5],((ordinal%8)<<11)|(ordinal&0x3f)]
   regions=[[0x463000,4096],[0x48b000,4096],[0x52d000,4096],[0x2000000,0x10000],[driver,0x4000],[desc,4096],[format_address,4096]]
   if args.format_chain:
    regions.append([0x46a000,4096]);w(0x46a0d0,4,0x3f800000);w(0x46a11c,4,0x40000000)
    fp[0]=spec.get('cw',fp[0]);fp[2]=(spec.get('top',0)<<11)|0x4500|(0x200 if ordinal&1 else 0)|(ordinal&0x3f)
   row=spec|dict(fields=fields,gp=gp,xmm=xmm,fp=fp,regions=regions)
   if args.format_chain:row.update(format_abi=True,x87_depth=spec.get('depth',0))
   input_digest.update((json.dumps(row,separators=(',',':'))+'\n').encode())
   report['case_in_progress']=ordinal;args.output.write_text(json.dumps(report,indent=2)+'\n')
   if ordinal%128==0 or args.case is not None:print(json.dumps(dict(case_in_progress=ordinal,spec=spec)),flush=True)
   native=invoke('h2_construct_native',row);invoke('h2_construct_provider',dict(op='reset',row=row))
   g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
   for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
   text=image.section('.text');g.mem_protect(text.va&~4095,((text.va%4096+text.vsize+4095)//4096)*4096,5)
   for at,n in regions:
    if at>=0x810000:g.mem_map(at,n)
    g.mem_write(at,bytes(n))
   g.mem_map(STOP,4096)
   for at,n,v in fields:g.mem_write(at,v.to_bytes(n,'little'))
   for reg,v in zip(REGS,gp):g.reg_write(reg,v)
   for n,words in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+n,sum(v<<(k*32)for k,v in enumerate(words)))
   for reg,v in zip([UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW],fp):g.reg_write(reg,v)
   g.reg_write(UC_X86_REG_EFLAGS,0x202);stores=[];calls=[];bad=[];executed=set();started=False;reference_delta=[]
   if args.format_chain:
    import unicorn.x86_const as R
    values=[(0x8000000000000000,0x3fff),(0x8000000000000000,0xbfff),(0x8000000000000000,0x4000),(0xc000000000000000,0x4000),(0x8000000000000000,0x4001),(0xa000000000000000,0x4001),(0xc000000000000000,0x4001)]
    tag=0xffff;top=fp[2]>>11&7
    for n in range(row['x87_depth']):
     physical=(top+n)&7;tag&=~(3<<(physical*2));g.reg_write(R.UC_X86_REG_FP0+physical,values[n])
    g.reg_write(R.UC_X86_REG_FPTAG,tag)
   def u(at,n=4):return int.from_bytes(g.mem_read(at,n),'little')
   def instruction(_g,at,n,_):
    nonlocal started
    assert image.read(at,n)==bytes(g.mem_read(at,n))
    if at in [create,pause]or(started and at in [0x224e20,0x2b8370,0x2b7f40,0x2e4d60]):
     sp=g.reg_read(REGS[4]);argc=4 if at==create else 2 if at==pause else 0 if at==0x224e20 else 1;arguments=[u(sp+4+k*4)for k in range(argc)]
     call=dict(target=at,gp=[g.reg_read(r)for r in REGS],**{'return':u(sp)},args=arguments)
     if at==create:
      call['descriptor']=[u(arguments[1]+k*4)for k in range(6)];call['format']=[u(call['descriptor'][2]+k,1)for k in range(20)]
     calls.append(call)
     if at in [create,pause]:
      reply=invoke('h2_construct_provider',dict(op='service',request=call))
      if 'output_value'in reply:g.mem_write(arguments[2],int(reply['output_value']).to_bytes(4,'little'))
      g.reg_write(REGS[0],reply['eax']);g.reg_write(REGS[4],sp+4+4*argc);g.reg_write(UC_X86_REG_EIP,call['return']);return
    started=True;executed.add(at)
   def write(_g,kind,at,n,v,_):
    if 0x2000000<=at<0x2010000:
     if g.reg_read(UC_X86_REG_EIP)==0x32cc53:reference_delta.append(v&0xffffffff)
     return
    try:before=u(at,n)
    except UcError:return
    stores.append([g.reg_read(UC_X86_REG_EIP),at,n,before,v&((1<<(8*n))-1)])
   def invalid(_g,kind,at,n,v,_):bad.append([at,n]);return False
   g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
   try:g.emu_start(spec['entry'],STOP,count=10000)
   except UcError:
    if not bad:raise
   if not bad:assert g.reg_read(UC_X86_REG_EIP)==STOP
   original=invoke('h2_construct_provider',dict(op='receipt'));original.update(gp=[g.reg_read(r)for r in REGS],xmm=[[g.reg_read(UC_X86_REG_XMM0+n)>>(k*32)&0xffffffff for k in range(4)]for n in range(8)],fp=[g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]],calls=calls,stores=stores,fault=bad[0]if bad else None,
    pages=sorted([[at+k,hashlib.sha256(g.mem_read(at+k,4096)).hexdigest()]for at,n in regions for k in range(0,n,4096)]))
   if args.format_chain:
    tag=g.reg_read(R.UC_X86_REG_FPTAG);depth=sum(((tag>>(n*2))&3)!=3 for n in range(8));top=(g.reg_read(UC_X86_REG_FPSW)>>11)&7
    live=[]
    for n in range(depth):
     mantissa,exponent=g.reg_read(R.UC_X86_REG_FP0+((top+n)&7));live.append((int(mantissa).to_bytes(8,'little')+int(exponent).to_bytes(2,'little')).hex())
    original.update(x87_live=live,x87_depth=depth,x87_tag=tag)
   if args.format_chain:
    raw_fp_digest.update(json.dumps(original['fp'],separators=(',',':')).encode())
    if 0x224e4b in executed:
     # Keep raw Unicorn status evidence. Independently execute the exact x87
     # primitives for this immutable operand/control profile. Only C1 is
     # supplied by that separate source-opcode oracle; no bit is ignored.
     value=0x40000000 if category==1 else 0x3f800000
     primitive=invoke('h2_format_x87_primitives',dict(cw=fp[0],sw=fp[2],value=value))
     assert primitive['stored']==value and primitive['integer']==(2 if category==1 else 1)
     assert reference_delta==[primitive['delta']],(ordinal,reference_delta,primitive)
     actual_sw=primitive['status'][-1];raw_sw=original['fp'][2]
     assert (actual_sw&~0x200)==(raw_sw&~0x200),(ordinal,actual_sw,raw_sw)
     raw_c1_differences+=(actual_sw&0x200)!=(raw_sw&0x200);c1_checked+=1
     original['fp'][2]=(raw_sw&~0x200)|(actual_sw&0x200)
     primitive_digest.update(json.dumps(primitive,separators=(',',':')).encode())
   for key in original:
    if key=='gp'and bad:continue
    if original[key]!=native[key]:report.update(case=ordinal,input=row,first_difference=key,native=native[key],original=original[key]);raise AssertionError(f'Constructor case {ordinal} {spec} differs: {key}')
   for call in calls:calls_count[f'{call["target"]:08x}']+=1
   trace_digest.update(json.dumps(original['native_service_trace'],separators=(',',':')).encode());created+=original['actual_created_voices'];pauses+=original['actual_pause_calls'];successful_pauses+=original['actual_successful_pauses'];returns+=not bad;faults+=bool(bad);stores_count+=len(stores);sites.update(executed);counts[f'{spec["entry"]:08x}']+=1
   invoke('h2_construct_provider',dict(op='close'))
  report.pop('case_in_progress',None);report.update(passed=True,cases=returns+faults,declared_cases=len(specs),entries=dict(counts),returns=returns,faults=faults,ordered_stores=stores_count,instruction_sites=len(sites),executed_instructions_lower_bound=sum(sites.values()),callee_entries=dict(calls_count),actual_created_voices=created,actual_pause_calls=pauses,actual_successful_pauses=successful_pauses,
   input_sha256=input_digest.hexdigest(),native_service_trace_sha256=trace_digest.hexdigest(),all_general_registers_on_return=True,all_page_hashes_including_source_stack_checked=True,all_callee_entry_gprs_and_arguments_checked=True,fp_simd_preserved=not args.format_chain,return_fp_simd_and_live_logical_x87_compared=args.format_chain,
   reference_fp_status_policy='Unchanged original execution for every bit except C1; fresh independent native original-opcode probe supplies C1 for complete1.0/2.0 conversion paths, with identical delta words and other status bits'if args.format_chain else None,
   independent_x87_c1_cases=c1_checked,raw_unicorn_c1_differences=raw_c1_differences,raw_unicorn_fp_state_sha256=raw_fp_digest.hexdigest()if args.format_chain else None,independent_x87_primitive_sha256=primitive_digest.hexdigest()if args.format_chain else None,raw_unicorn_c1_equivalence=raw_c1_differences==0,xbox_fp_status_equivalence=False)
 finally:
  invoke('h2_construct_provider',dict(op='close'));data=json.dumps(report,indent=2)+'\n';assert len(data.encode())<=1024*1024;args.output.write_text(data)
 print(json.dumps({k:v for k,v in report.items()if k not in ['input','native','original','scope']}));return 0
if __name__=='__main__':raise SystemExit(main())
