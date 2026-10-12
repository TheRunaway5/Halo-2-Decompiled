#!/usr/bin/env python3
"""Complete unchanged D15E0 image selection over explicit source state.

The 64 retained MAP runtime records and their globals/pools are literal
snapshot bytes. Five additional caller-state variants are independently
observed outputs of earlier calls in the unchanged whole MAP oracle.
Separate declared handle/readiness banks exercise complete ready and
last-ready fallback branches; fresh headers stop at original 12C820. General
12CE00 controls stop at actual 12D5A0 allocation or 120CE0 pump entries.
No service result, original instruction patch, GPU or guest work is supplied.
"""
from pathlib import Path
import argparse,collections,hashlib,itertools,json,re,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed,fw
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH,STACK,STOP=0x20000000,0x200fe000,0x200ff000
RECORD,POOL,IMAGES,CACHE,LRUPOOL,LRU=0x20001000,0x20040000,0x20041000,0x20048000,0x20049000,0x2004a000
GLOBALS=[(0x4e6454,4),(0x4e6464,4),(0x4e6468,1),(0x4e646c,4),
 (0x4e6470,4),(0x4e6474,4),(0x4e647a,1),(0x4e647b,1),(0x4e647c,4),
 (0x4e6484,4),(0x4e6488,4),(0x4e28f0,4),(0x46883c,1),(0x46883d,1),(0x468841,1),(0x485ae4,12)]
ENTRIES=[0xd15e0,0x12ccf0,0x12ce00,0x12c990,0x12cb10,0x12c820,0x359e0,0x12d5a0,0x120ce0,0x2d2084,0x125d60]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--binary',type=Path);args=parser.parse_args()
 snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
 image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
 source_path=ROOT/'analysis/multiplayer-shading/map-light-sample-original.json'
 source=json.loads(source_path.read_text());assert source['source_xbe_sha256']==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000)
 pages={};dirty=set();instructions={};entries=collections.Counter();calls=[];writes=[];boundaries=[];active_lru=[0]
 def page(address):
  p=address&~4095
  if p not in pages:
   raw=snapshot.read(p,4096);u.mem_map(p,4096);u.mem_write(p,raw);pages[p]=raw
 def read(address,size):
  for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=p<SYNTH+0x100000:page(p)
  return bytes(u.mem_read(address,size))
 def get(address):return uint(read(address,4))
 def put(address,raw):
  read(address,len(raw));u.mem_write(address,raw)
  for p in range(address&~4095,((address+len(raw)-1)&~4095)+4096,4096):
   if not SYNTH<=p<SYNTH+0x100000:dirty.add(p)
 def integer(address,value,size=4):put(address,int(value&((1<<(size*8))-1)).to_bytes(size,'little'))
 def missing(g,access,address,size,value,data):
  for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(p)
  return True
 def observe(g,pc,size,data):
  assert read(pc,size)==image.read(pc,size),(hex(pc),'changed original instruction')
  instructions[pc]=size
  if pc not in ENTRIES:return
  entries[f'{pc:08X}']+=1;sp=g.reg_read(UC_X86_REG_ESP)
  event=dict(entry=f'{pc:08X}',eax=g.reg_read(UC_X86_REG_EAX),ecx=g.reg_read(UC_X86_REG_ECX),
    edx=g.reg_read(UC_X86_REG_EDX),ebx=g.reg_read(UC_X86_REG_EBX),esi=g.reg_read(UC_X86_REG_ESI),
    xmm0=g.reg_read(UC_X86_REG_XMM0)&0xffffffff,xmm1=g.reg_read(UC_X86_REG_XMM1)&0xffffffff,
    xmm2=g.reg_read(UC_X86_REG_XMM2)&0xffffffff,stack=words(read(sp+4,20)))
  calls.append(event)
  if pc in[0x12c820,0x12d5a0,0x120ce0,0x2d2084,0x125d60]:
   boundaries.append(event);g.emu_stop()
 def write(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(p)
  base=active_lru[0]
  if base<=address<base+512*24 and (address-base)%24==0x14:
   writes.append(dict(slot=(address-base)//24,before=get(address),after=value&0xffffffff))
 u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,observe);u.hook_add(UC_HOOK_MEM_WRITE,write)
 gdtr=re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',snapshot.report['registers_before'])
 ga,gl=(int(v,16)for v in gdtr.groups());read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,selector in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,0x10),(UC_X86_REG_DS,0x10),(UC_X86_REG_ES,0x10),(UC_X86_REG_FS,0x20)]:u.reg_write(reg,selector)
 def reset():
  for p in dirty:u.mem_write(p,pages[p])
  dirty.clear();u.mem_write(SYNTH,bytes(0x50000));calls.clear();writes.clear();boundaries.clear()
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 def bank(record):
  raw=read(record,116);pool=get(0x4e6454);images=get(pool+0x44);cache=get(0x4e6464);lru=get(get(cache+0x64)+0x44)
  active_lru[0]=lru;partitions=[]
  for i in range(3):
   handle=uint(raw,0x28+i*4);slot=handle&65535
   row=dict(handle=handle,resource_pointer=uint(raw,0x1c+i*4),resource_size=uint(raw,0x34+i*4))
   if handle!=0xffffffff:
    image_row=read(images+slot*40,40);row.update(texture_token=images+slot*40+0x14,ready=bool(image_row[3]),
      last_use=get(lru+slot*24+0x14),image_row_hex=image_row.hex())
   else:row.update(texture_token=0,ready=False,last_use=0,image_row_hex=None)
   partitions.append(row)
  return dict(record=record,raw_record_hex=raw.hex(),flags_high=raw[0xf],flags_low=raw[0xe],byte_bias=raw[0x16],category=raw[0x17],
   cutoff=uint(raw,0x58),cached_texture=uint(raw,0x50),cached_until=uint(raw,0x70),
   inline_control=uint(raw,0x5c),inline_pending_size=uint(raw,0x64),inline_texture_token=record+0x5c,
   globals={f'{a:08X}':read(a,n).hex()for a,n in GLOBALS},use_clock=get(cache+0x38),partitions=partitions)
 def run(record,request,label,entry=0xd15e0,flags=0):
  before=bank(record);u.mem_write(STACK,packed([STOP]if entry==0xd15e0 else[STOP,record,flags]))
  u.reg_write(UC_X86_REG_EAX,record);u.reg_write(UC_X86_REG_XMM2,request);u.reg_write(UC_X86_REG_XMM1,request)
  u.emu_start(entry,STOP,count=2000000);pc=u.reg_read(UC_X86_REG_EIP)
  assert pc==STOP or boundaries,(label,hex(pc))
  if pc==STOP:assert u.reg_read(UC_X86_REG_ESP)==STACK+(4 if entry==0xd15e0 else 12)
  result=None if boundaries else u.reg_read(UC_X86_REG_EAX)
  after=read(record,116);header=None if not result else words(read(result,20))
  output=dict(returned_texture_token=result,selected_header=header,after_record_hex=after.hex(),cached_texture=uint(after,0x50),
    cached_until=uint(after,0x70),last_use=[get(active_lru[0]+(p['handle']&65535)*24+0x14)if p['handle']!=0xffffffff else 0 for p in before['partitions']],
    fallback_mark=read(0x4e647a,1)[0],lru_write_order=writes.copy(),source_calls=calls.copy(),boundary=boundaries.copy())
  if entry==0xd15e0:assert all(b['entry']=='0012C820'for b in boundaries),'literal CPU path unexpectedly entered allocation/pump'
  return dict(label=label,entry=f'{entry:08X}',request=request,flags=flags,input=before,output=output)
 def declared(request=0.,ready=(False,False,False),present=(False,False,False),pointers=(0,1,2),sizes=(8192,4096,2048),
    cutoff=4.,quality=True,bias=0.,byte_bias=0,force_zero=False,force_one=False,software=False,available=True,
    inline='existing',category=1,mode=0,filter_mode=0,cached=0,deadline=0,clock=17,use_clock=91,flags_high=2,flags_low=3,
    throttle_tasks=0,throttle_loads=0,entry=0xd15e0,request_flags=0,bitmap_type=0,fallback_marked=False,handles=None,label='declared CPU ready/fallback control'):
  reset();raw=bytearray(116);struct.pack_into('<6H',raw,4,64,64,1,bitmap_type,16,(flags_high<<8)|flags_low)
  raw[0x16]=byte_bias;raw[0x17]=category
  struct.pack_into('<3I',raw,0x1c,*pointers);struct.pack_into('<3I',raw,0x28,*(handles if handles is not None else[0xe0000001+i if present[i]else 0xffffffff for i in range(3)]))
  struct.pack_into('<3I',raw,0x34,*sizes);struct.pack_into('<I',raw,0x50,cached);struct.pack_into('<f',raw,0x58,cutoff)
  struct.pack_into('<I',raw,0x70,deadline&0xffffffff)
  if inline=='existing':struct.pack_into('<5I',raw,0x5c,0x40001,0x01000080,0,0x03310f29,0)
  elif inline in['cold','negative']:struct.pack_into('<3I4H',raw,0x5c,0 if inline=='cold'else 0x80000000,0x80,128,16,8,8,1)
  else:assert inline=='absent'
  u.mem_write(RECORD,bytes(raw));integer(POOL+0x44,IMAGES);integer(CACHE+0x64,LRUPOOL);integer(CACHE+0x38,use_clock);integer(LRUPOOL+0x44,LRU)
  for i in range(3):
   at=IMAGES+(i+1)*40;integer(at+3,int(ready[i]),1);integer(at+8,2);integer(at+0xc,0x400000+i)
   u.mem_write(at+0x14,packed([0x40001,0x08100000+i*0x10000,0,0x06610f29,0]));integer(LRU+(i+1)*24+0x14,123+i)
  for at,value,size in[(0x4e6454,POOL,4),(0x4e6464,CACHE,4),(0x4e6468,available,1),(0x4e646c,0x08100000,4),
    (0x4e6470,mode,4),(0x4e6474,filter_mode,4),(0x4e647a,fallback_marked,1),(0x4e647b,software,1),(0x4e647c,fw(bias),4),
    (0x4e6484,throttle_loads,4),(0x4e6488,clock,4),(0x4e28f0,throttle_tasks,4),
    (0x46883c,force_one,1),(0x46883d,force_zero,1),(0x468841,quality,1)]:integer(at,int(value),size)
  return run(RECORD,fw(request),label,entry,request_flags)
 records=[];actual_records={};observed_versions=collections.defaultdict(set)
 for case in source['records']:
  for record in case.get('runtime_records',[]):
   pointer=int(record['pointer']);raw=bytes.fromhex(record['raw'])
   observed_versions[pointer].add(raw);actual_records[pointer]=snapshot.read(pointer,116)
   original=actual_records[pointer]
   assert raw[:0x50]==original[:0x50]and raw[0x54:0x5c]==original[0x54:0x5c]
 assert len(actual_records)==64
 for pointer in sorted(actual_records):
  reset();case=run(pointer,fw(0.),'literal retained MAP CPU request');records.append(case)
  assert all(b['entry']=='0012C820'for b in case['output']['boundary'])
 for pointer,versions in sorted(observed_versions.items()):
  for raw in sorted(versions):
   if raw==actual_records[pointer]:continue
   reset();put(pointer,raw);records.append(run(pointer,fw(0.),'independently observed prior whole-MAP caller-state variant'))
 assert len(records)==69
 # Every handle/readiness combination includes absent, pending, requested
 # ready, and last-ready precedence, with an independent inline candidate.
 for request in[-2.,0.,.9999999403953552,1.,1.0000001192092896,1.9999998807907104,2.,2.000000238418579,4.,8.]:
  for ready,present in itertools.product(itertools.product([False,True],repeat=3),repeat=2):
   records.append(declared(request=request,ready=ready,present=present))
 for request in[-2.,0.,.75,1.,1.75,2.,4.]:
  for quality,bias,byte_bias in[(False,2.,255),(True,0.,100),(True,-2.,255),(True,.75,0)]:
   for cutoff in[0.,.5,1.,2.,4.]:
    for zero,one in[(False,False),(True,False),(False,True),(True,True)]:
     records.append(declared(request=request,quality=quality,bias=bias,byte_bias=byte_bias,cutoff=cutoff,
       force_zero=zero,force_one=one,ready=(True,True,True),present=(True,True,True),
       label='raw cutoff versus adjusted request and sequential override control'))
 for request,bias in[(3.,-2.),(.5,2.)]:
  for zero,one in[(False,False),(True,False),(False,True),(True,True)]:
   records.append(declared(request=request,bias=bias,cutoff=2.,force_zero=zero,force_one=one,
    ready=(False,False,True),present=(True,True,True),label='independent raw cutoff gate with a pending adjusted partition'))
 for inline,available,software,category in itertools.product(['existing','cold','negative','absent'],[False,True],[False,True],[0,1,3,4,12,17,18]):
  records.append(declared(inline=inline,available=available,software=software,category=category,
    label='complete existing/cold inline and software-category control'))
 for mode,filter_mode,category,inline,ready in itertools.product([0,1,2],[0,1,2],[1,3],['existing','absent'],[False,True]):
  records.append(declared(mode=mode,filter_mode=filter_mode,category=category,inline=inline,ready=(ready,)*3,present=(True,)*3,
    label='filtered force-mode/default/last-ready control'))
 for size in[0,1,1023,1024,1025,4096,0xffffffff]:
  for pointers in[(0,1,2),(0xffffffff,1,2),(0,0xffffffff,2),(0,1,0xffffffff)]:
   for request in[0.,1.,2.]:
    records.append(declared(request=request,pointers=pointers,sizes=(8192,size,size),cutoff=0.,inline='absent',
      ready=(True,True,True),present=(True,True,True),label='source authored eligibility and 1024-byte edge'))
 for ready,request,cutoff in itertools.product([(False,False,True),(True,False,False),(True,True,True)],[0.,1.,2.,4.],[0.,4.]):
  records.append(declared(request=request,cutoff=cutoff,ready=ready,present=(True,True,True),
    handles=(0xe0000001,0xf0000001,0xe0000003),label='identical low16 image/LRU row with different upper handle salts'))
 for clock,deadline in[(17,16),(17,17),(17,18),(0x7fffffff,0x7fffffff),(0x80000000,0x80000001),(0xffffffff,0)]:
  for flags_high in[0,2]:
   for request in[0.,1.,2.,4.]:
    records.append(declared(request=request,clock=clock,deadline=deadline,cached=IMAGES+40+0x14,
      flags_high=flags_high,ready=(True,True,True),present=(True,True,True),label='complete CPU cached deadline/direct/overflow edge'))
 for bitmap_type,fallback_marked,flags_low in itertools.product([0,1,2],[False,True],[3,131]):
  records.append(declared(bitmap_type=bitmap_type,fallback_marked=fallback_marked,flags_low=flags_low,mode=1,
    inline='absent',label='type-specific default and persistent fallback-marker control'))
 # General request controls recover the actual service entry/ABI without
 #inventing an allocation result, priority update, asynchronous completion,
 # or wait-pump result. CPU flags zero controls above never reach these.
 for request_flags in[0,1,2,3,4,6,7,8,10,16,18]:
  for state in['missing','pending','ready']:
   for tasks,loads in[(0,0),(25,9),(26,9),(25,10)]:
    for mode in[0,1,2]:
     records.append(declared(entry=0x12ce00,request_flags=request_flags,inline='absent',mode=mode,
      throttle_tasks=tasks,throttle_loads=loads,present=(state!='missing',False,False),ready=(state=='ready',False,False),
      label='general 12CE00 allocation/pump/throttle boundary'))
 completed=[case for case in records if not case['output']['boundary']]
 fixture=ROOT/'tests/fixtures/retail_source_bitmap_cpu_selection.tsv'
 rows=['# CPU44 input words: request/context14/state12/partitions3x6 | original10 output words. Fresh-header branches stop at original12C820.']
 for case in records:
  if case['entry']!='000D15E0':continue
  state=case['input'];context=state['globals'];out=case['output']
  def global_word(address):return int.from_bytes(bytes.fromhex(context[f'{address:08X}']),'little')
  defaults=words(bytes.fromhex(context['00485AE4']))
  row=[case['request'],global_word(0x468841),global_word(0x4e647c),global_word(0x4e6488),state['use_clock'],
    global_word(0x4e647b),global_word(0x4e6468),global_word(0x46883d),global_word(0x46883c),
    global_word(0x4e6470),global_word(0x4e6474),global_word(0x4e647a),*defaults,
    state['flags_high'],state['byte_bias'],state['cutoff'],state['cached_texture'],state['cached_until'],state['category'],
    state['inline_control'],state['inline_pending_size'],state['inline_texture_token'],state['flags_low'],
    struct.unpack_from('<H',bytes.fromhex(state['raw_record_hex']),0xa)[0]]
  for p in state['partitions']:row.extend([p['resource_pointer'],p['resource_size'],p['handle'],p['texture_token'],int(p['ready']),p['last_use']])
  boundary=int(bool(out['boundary']));request_entered=int(any(c['entry']=='0012CE00'for c in out['source_calls']))
  requested=next((c['eax']for c in out['source_calls']if c['entry']=='0012C990'),3)
  expected=[out['returned_texture_token']or 0,boundary,request_entered,requested,out['cached_texture'],out['cached_until'],*out['last_use'],int(bool(out['fallback_mark']))]
  assert len(row)==44 and len(expected)==10
  case['native_probe_input']=row;case['native_probe_output']=expected
  rows.append(' '.join(f'{v&0xffffffff:08X}'for v in row)+' | '+' '.join(f'{v&0xffffffff:08X}'for v in expected))
 fixture.write_text('\n'.join(rows)+'\n')
 retained=records[:64];outcomes=collections.Counter()
 for case in completed:
  token=case['output']['returned_texture_token'];record=case['input']['record']
  selected='null'if token==0 else'inline'if token==record+0x5c else'partition'
  if selected=='partition':
   p=next((i for i,p in enumerate(case['input']['partitions'])if p['texture_token']==token),None)
   selected=f'partition{p}'if p is not None else'cached/default'
  outcomes[selected]+=1
 report=dict(boundary=__doc__,source_xbe_sha256=PIN,tool_sha256=sha(Path(__file__).read_bytes()),
  fixture_sha256=sha(fixture.read_bytes()),native_fixture_cases=len(rows)-1,native_fixture_words=(len(rows)-1)*10,
  fpcw='023F',mxcsr='1F80',execution_configuration='Supplied CPU control configuration; not inferred live scheduling.',
  snapshot_ram_sha256=sha(snapshot.ram),snapshot_manifest_sha256=sha((snapshot.directory/'export.json').read_bytes()),
  map_report_sha256=sha(source_path.read_bytes()),retained_globals={f'{a:08X}':snapshot.read(a,n).hex()for a,n in GLOBALS},
  retained_record_count=64,observed_whole_MAP_caller_variants=5,cases=len(records),complete_cases=len(completed),boundary_cases=len(records)-len(completed),
  complete_retained_cases=sum(not c['output']['boundary']for c in retained),outcomes=dict(outcomes),
  source_entries=dict(entries),unique_instructions=len(instructions),
  instruction_sha256=sha(b''.join(packed([pc])+image.read(pc,n)for pc,n in sorted(instructions.items()))),
  abi={
   'D15E0':dict(registers={'EAX':'bitmap record','XMM2':'unadjusted request'},stack_arguments=[],returns='EAX texture header; RET',
     first_gate='Signed bitmap+70 > 4E6488 and bitmap+50 nonzero accepts the cached pointer before other state.'),
   '12CCF0':dict(registers={'ESI':'bitmap record','EDX':'request flags (CPU zero)','XMM1':'unadjusted request'},stack_arguments=[],returns='EAX texture header; RET'),
   '12CE00':dict(registers={'XMM1':'unadjusted request'},stack_arguments=['bitmap record','request flags (CPU zero)'],returns='EAX texture header; RET 8'),
   '12CB10':dict(registers={'EDX':'bitmap record','XMM0':'adjusted request'},stack_arguments=[],returns='EAX partition index; RET'),
   '12C990':dict(registers={'EAX':'partition index','CL':'flags bit1 / allow allocation','EBX':'priority'},
     stack_arguments=['bitmap record','wait boolean','positive filtered force-mode boolean','waited output pointer','allocation-failed output pointer'],returns='EAX texture header; RET 14 hex'),
   '12D5A0_boundary':dict(registers={'ECX':'bitmap record','AL':'wait boolean'},stack_arguments=['partition index','positive filtered force-mode boolean','priority']),
   '120CE0_boundary':dict(registers={'ESI':'image row +C asynchronous ticket'},stack_arguments=['priority'])},
  field_rules={
   'record':'Flags low/high +E/+F; bias byte +16; category +17; authored locations +1C[3]; runtime handles +28[3]; authored sizes +34[3]; cached token +50; cutoff +58; inline Common +5C / pending signed size +64; signed cache deadline +70.',
   'banks':'4E6454->+44 image rows of 40 bytes: readiness byte +3, priority +8, asynchronous ticket +C, inline header +14. 4E6464->+64->+44 LRU rows of 24 bytes: timestamp +14; source use clock 4E6464->+38. Handles use low16 without a salt comparison.',
   'selection':'After the first lookup misses, 12CE00 recomputes adjusted XMM0 with separately rounded bias arithmetic. 46883D forces XMM0 zero, then 46883C forces XMM0 one. Its cutoff gate at 12CF1E still compares the original raw XMM1.',
   'authored_eligibility':'12CB10 starts with partition0 and visits indices 0..2. Location must differ from NONE; size must be nonzero; indices1/2 require signed size >1024. Thresholds are 0/1/2. Its chosen index does not prove a handle is ready.',
   'fallback':'A ready requested partition returns immediately. Otherwise the original fallback scan touches handles in order0/1/2 and each ready header replaces the current candidate, including an inline candidate; the last ready header wins. The ready-only request branch and final scan do not gate on authored location/size.',
   'mode':'Positive 4E6470 passes only category filter0 or filter1 with bitmap category3. The force boolean becomes true for any such positive mode; the wait boolean becomes (mode==1). A no-candidate wait path uses 485AE4/8/EC by signed type0/1/other and sets 4E647A.',
   'CPU_boundary':'D15E0 supplies flags0. 12C990 therefore receives CL0, enters its ready-only path at 12CABD, and cannot allocate, escalate asynchronous priority, or wait/pump.',
   'header_boundary':'Availability4E6468 and nonzero Common or positive signed pending size admit the inline candidate. Common<=0 reaches original12C820 and is stopped before construction; no header result is supplied.',
   'general_request_boundaries':'Allocation/pump controls are direct12CE00 inputs and are excluded from the CPU native fixture. Their reported state is partial at the actual source service entry, never a completion result.'},
  services_replaced=[],source_instruction_patches=[],records=records)
 if args.binary:
  native_cases=[case for case in records if 'native_probe_input'in case]
  stream=''.join(' '.join(f'{v&0xffffffff:X}'for v in case['native_probe_input'])+'\n'for case in native_cases)
  native=subprocess.run([str(args.binary.resolve()),'--probe'],input=stream,capture_output=True,text=True)
  assert native.returncode==0,native.stderr
  outputs=[[int(v,16)for v in line.split()]for line in native.stdout.splitlines()]
  assert len(outputs)==len(native_cases)
  for index,(case,actual)in enumerate(zip(native_cases,outputs)):
   assert actual==case['native_probe_output'],(index,case['label'],case['native_probe_input'],actual,case['native_probe_output'])
  report['native_validation']=dict(binary_sha256=sha(args.binary.read_bytes()),return_code=0,cases=len(outputs),checked_words=sum(map(len,outputs)))
 output=ROOT/'analysis/multiplayer-shading/source-bitmap-cpu-selection-original.json';output.write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='records'},indent=2)+'\n')
 print(json.dumps({k:report[k]for k in['cases','complete_cases','boundary_cases','retained_record_count','complete_retained_cases','outcomes','source_entries','unique_instructions']},indent=2))
if __name__=='__main__':main()
