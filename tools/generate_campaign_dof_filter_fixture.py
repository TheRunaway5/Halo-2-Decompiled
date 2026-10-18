"""Complete unchanged21A60/21180/20F10 + CRT pow arithmetic.
Only void native graphics state/submission boundaries are recorded. Their ABI
returns preserve incoming scalar values; no allocation, target readiness,
gameplay/SDK result, instruction patch, texture pixels or GPU work is supplied.
"""
from pathlib import Path
import argparse,sys,json,struct,hashlib,traceback
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',type=Path,required=True)
parser.add_argument('--repository-root',type=Path,default=Path(__file__).resolve().parents[1])
parser.add_argument('--output',type=Path)
parser.add_argument('--campaign-state-fixture',type=Path,
 help='Optional pinned output from generate_campaign_dof_fixture.py; use with --maps-directory')
parser.add_argument('--maps-directory',type=Path)
cli_args=parser.parse_args();ROOT=cli_args.repository_root.resolve()
if bool(cli_args.campaign_state_fixture)!=bool(cli_args.maps_directory):
 parser.error('--campaign-state-fixture and --maps-directory must be supplied together')
fixture_output=cli_args.output or ROOT/'tests/fixtures/retail_cinematic_dof_filter_tu15.json'
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
XBE=cli_args.xbe;x=Xbe(XBE);PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd';assert hashlib.sha256(x.data).hexdigest()==PIN
P=dict(status='RUNNING',scope=__doc__,xbe_sha256=PIN,cases=[],void_boundaries={hex(p):n for p,n in {0x14f40:0,0x15760:4,0x222b0:0,0x1ccd0:0,0x4044b0:0,0x34a80:44}.items()})
u=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
for s in x.sections:
 for p in range(s.va&~4095,(s.va+s.vsize+4095)&~4095,4096):
  if p not in pages:u.mem_map(p,4096);pages.add(p)
 u.mem_write(s.va,x.section_bytes(s))
for p,n in [(0x90000000,0x100000),(0x60000000,0x10000),(0x70000000,4096)]:u.mem_map(p,n)
def put(p,v):u.mem_write(p,struct.pack('<I',v&0xffffffff))
def get(p):return struct.unpack('<I',u.mem_read(p,4))[0]
def fw(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def fv(w):return struct.unpack('<f',struct.pack('<I',w))[0]
put(0x411568,0x90000000);put(0x90000000,0x90080000);put(0x90000004,0x900f0000);put(0x4920d8,3)
services={0x14f40:0,0x15760:4,0x222b0:0,0x1ccd0:0,0x4044b0:0,0x34a80:44};seen=set();programs=[];draws=[];bindings=[];events=[]
def hook(vm,pc,n,_):
 assert bytes(vm.mem_read(pc,n))==x.read(pc,n);seen.add(pc)
 if pc not in services:return
 sp=vm.reg_read(UC_X86_REG_ESP);row=dict(entry=hex(pc),callsite=hex(get(sp)-5))
 if pc in (0x1ccd0,0x4044b0):
  raw=bytes(vm.mem_read(0x491788,240));programs.append(raw.hex());row['program']=raw.hex()
 if pc==0x14f40:
  bindings.append(dict(target=vm.reg_read(UC_X86_REG_EDI),stage=vm.reg_read(UC_X86_REG_EBX)));row.update(bindings[-1])
 elif pc==0x222b0:row.update(state=vm.reg_read(UC_X86_REG_ESI),value=vm.reg_read(UC_X86_REG_EDI))
 elif pc==0x15760:row.update(stage=vm.reg_read(UC_X86_REG_ECX),state=get(sp+4))
 elif pc==0x34a80:
  raw=bytes(vm.mem_read(sp+4,44));words=list(struct.unpack('<11I',raw));draws.append(dict(arguments=words,program=bytes(vm.mem_read(0x491788,240)).hex()));row.update(arguments=words)
 events.append(row);vm.reg_write(UC_X86_REG_EIP,get(sp));vm.reg_write(UC_X86_REG_ESP,sp+4+services[pc])
u.hook_add(UC_HOOK_CODE,hook)
def execute(entry,words=(),first=0):
 put(0x60008000,0x70000000)
 for i,w in enumerate(words):put(0x60008004+4*i,w)
 for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):u.reg_write(reg,0)
 u.reg_write(UC_X86_REG_ESP,0x60008000);u.reg_write(UC_X86_REG_XMM0,first)
 u.reg_write(UC_X86_REG_EFLAGS,0x202);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
 u.emu_start(entry,0x70000000,count=100000)
 assert u.reg_read(UC_X86_REG_EIP)==0x70000000
 assert u.reg_read(UC_X86_REG_ESP)==0x60008004+4*len(words)
def capture_filter(words,provenance=None):
 programs.clear();draws.clear();bindings.clear();events.clear();execute(0x21a60,words)
 row=dict(arguments=words,programs=programs.copy(),draws=draws.copy(),bindings=bindings.copy(),events=events.copy(),actual_pow_executed=0x37ce0c in seen)
 if provenance is not None:row['completed_state']=provenance
 P['cases'].append(row)
def set_clock(value):u.mem_write(0x4920c0,struct.pack('<d',value))
OWNER=0x9003f000
def start_interface(words,epoch):
 u.mem_write(OWNER,bytes(0x3fc));u.mem_write(OWNER+0xac,b'\1');put(0x515ec0,OWNER)
 set_clock(epoch);execute(0x1fc10,words[1:],words[0])
def complete_state(clock):
 set_clock(clock);execute(0x1fd00)
 assert u.reg_read(UC_X86_REG_EAX)==OWNER
 return bytes(u.mem_read(OWNER,0x3fc))
def completed_filter(bank,provenance):
 # Source output fields, never the untouched A5 raw-setter bank.
 words=[struct.unpack_from('<I',bank,offset)[0]for offset in (0x44,0x48,0x40)]
 provenance={**provenance,'active':bool(bank[0x3c]),'interface_after':bank.hex()}
 capture_filter(words,provenance)
def expand_campaign():
 state_bytes=cli_args.campaign_state_fixture.read_bytes()
 expected='754c33b530454168ccf4c307f6ac21b821742c07090a4800f38f1af5cbe93df9'
 assert hashlib.sha256(state_bytes).hexdigest()==expected,'Unexpected campaign state fixture; regenerate authenticated original state controls'
 state=json.loads(state_bytes)
 assert state['status']=='PASS_UNCHANGED_DOF_AUTHORED64_PARSER_RAW_SETTER_UPDATE_ADMISSION_AND_DEPTH'
 assert state['xbe_sha256']==PIN and len(state['authored'])==64 and len(state['update_cases'])==36
 sys.path.insert(0,str(ROOT/'tools'))
 from inspect_multiplayer import Cache
 authenticated=[]
 for mapname in dict.fromkeys(row['map']for row in state['authored']):
  c=Cache(cli_args.maps_directory/f'{mapname}.map');nodes=c.u(c.scenario+0x23c)-c.magic
  for row in (row for row in state['authored']if row['map']==mapname):
   raw_nodes={int(key,16):bytes.fromhex(raw)for key,raw in row['raw_nodes'].items()}
   for datum,raw in raw_nodes.items():
    offset=nodes+(datum&65535)*20
    assert bytes(c.data[offset:offset+20])==raw and struct.unpack_from('<H',raw)[0]==datum>>16
   root=int(row['datum'],16);assert struct.unpack_from('<H',raw_nodes[root],2)[0]==706
   function=struct.unpack_from('<I',raw_nodes[root],16)[0];node=struct.unpack_from('<I',raw_nodes[function],8)[0];words=[]
   for expected_word in row['arguments']:
    raw=raw_nodes[node];assert struct.unpack_from('<H',raw,4)[0]==6
    value=struct.unpack_from('<I',raw,16)[0];assert value==int(expected_word,16)
    words.append(value);node=struct.unpack_from('<I',raw,8)[0]
   assert node==0xffffffff and len(raw_nodes)==9
   assert hashlib.sha256(b''.join(raw_nodes.values())).hexdigest()==row['metadata_sha256']
   authenticated.append(dict(map=mapname,datum=row['datum'],arguments=words,metadata_sha256=row['metadata_sha256']))
   epoch=0.;duration=max(fv(words[3]),fv(words[6]),0.)
   start_interface(words,epoch)
   for time_name,delta in [('start',0.),('mid',duration*.5),('end',duration+1/30)]:
    completed_filter(complete_state(epoch+delta),dict(kind='authored64',map=mapname,datum=row['datum'],metadata_sha256=row['metadata_sha256'],setter_arguments=words,epoch=epoch,delta=delta,time=time_name))
  c.data.close();c.stream.close()
 previous=None
 for index,row in enumerate(state['update_cases']):
  identity=(row['epoch'],tuple(row['arguments']))
  if identity!=previous:start_interface(row['arguments'],row['epoch']);previous=identity
  bank=complete_state(row['epoch']+row['delta'])
  assert bank.hex()==row['after'],('Completed full interface drift',index)
  completed_filter(bank,dict(kind='original36_timed_update',index=index,setter_arguments=row['arguments'],epoch=row['epoch'],delta=row['delta'],full_original_bank_match=True))
 P['campaign_coverage']=dict(state_fixture_sha256=expected,authored_cache_requests=len(authenticated),authored_completed_samples=len(authenticated)*3,timed_completed_samples=36,authenticated=authenticated,initial_interface='zeroed0x3FC with+ACactive1',source_chain=['1FC10','1FD00','21A60','21180','20F10','actual CRT pow'],bank_outputs_used=['+44far','+48near','+40focus'],full_timed_banks_match=True)
try:
 for far,near,focus in [(0.,0.,1.5),(1.,0.,1.5),(0.,1.,5.),(.5,.75,2.),(1.5,2.5,1.),(5.,5.,12.),(.001,.001,1.),(-1.,1.,3.),(1.,-1.,3.),(.1,.9,2048.)]:
  words=list(map(fw,(far,near,focus)));capture_filter(words)
 if cli_args.campaign_state_fixture:expand_campaign()
 P['executed_instruction_count']=len(seen);P['actual_pow_executed']=0x37ce0c in seen;P['status']='PASS_UNCHANGED_ORIGINAL_DOF_FILTER_COMPOSITE_CPU_RECIPE'
except Exception as e:P['status']='FAIL_RETAINED';P['error']=repr(e);P['last_case']=dict(arguments=words,events=events.copy());traceback.print_exc()
payload=(json.dumps(P,indent=2)+'\n').encode()
fixture_output.parent.mkdir(parents=True,exist_ok=True)
if fixture_output.exists():
 if fixture_output.read_bytes()!=payload:raise ValueError('Preserve existing differing DoF filter oracle')
else:fixture_output.write_bytes(payload)
print(json.dumps({k:len(v) if isinstance(v,list) else v for k,v in P.items() if k!='last_case'},indent=2))
raise SystemExit(P['status']=='FAIL_RETAINED')
