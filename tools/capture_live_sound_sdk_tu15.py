"""Observe authentic whole TU1.5 SDK factory prefix and its actual platform calls."""
from pathlib import Path
import sys,json,hashlib,socket,re
sys.path.insert(0,str(Path(__file__).resolve().parent))
import capture_live_sound_environment_tu15 as reference
from capture_live_sound_environment_tu15 import *
base_banks=reference.bank_state
base_abi=reference.abi
thread_context=None
def abi_with_fs():
 r=base_abi();r["fs_base"]=thread_context["actual_fs_base"];return r
reference.abi=abi_with_fs

def banks():
 r=base_banks();fs=thread_context['actual_fs_base'];r.append(dict(label='actual stopped source thread IRQL byte',address=fs+0x24,size=1,writable=True,hex=read(fs+0x24,1).hex()));s=image.section('DSOUND');r.append(dict(label='actual SDK DSOUND code and mutable runtime state',address=s.va,size=s.vsize,writable=True,hex=read(s.va,s.vsize).hex()));return sorted(r,key=lambda r:r['address'])
reference.bank_state=banks

def save(name):
 s=state();(out/name).write_text(json.dumps(s,separators=(',',':'))+'\n');return s

def fs_context():
 # Read the actual emulator segment base; never infer a KPCR address.
 q=socket.socket(socket.AF_UNIX);q.connect(str(Path(os.environ['H2_SOUND_ENVIRONMENT_SESSION'])/'qmp.sock'));f=q.makefile('rwb',buffering=0);json.loads(f.readline())
 def call(command,args=None):
  f.write((json.dumps(dict(execute=command,**({'arguments':args}if args else{})))+'\n').encode())
  while True:
   value=json.loads(f.readline())
   if 'return'in value:return value['return']
   if 'error'in value:raise RuntimeError(value['error'])
 call('qmp_capabilities');registers=call('human-monitor-command',{'command-line':'info registers'});f.close();q.close()
 row=next(line for line in registers.splitlines()if re.match(r'FS\s*=',line));match=re.match(r'FS\s*=\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)',row);assert match,row;base=int(match[1],16)
 return dict(fs_register_row=row,actual_fs_base=base,actual_irql_byte=read(base+0x24,1).hex())

def capture_sdk():
 global thread_context
 stop=breakpoint(0x387877);gdb.execute('continue');assert reg('eip')==0x387877;stop.delete();reference.stack_anchor=reg('esp')&~4095
 verified=0
 for listing in sorted((ROOT/'work/sandbox-audio-v10/generated-sdk').glob('*.asm')):
  for row in listing.read_text().splitlines():
   pc,raw,*_=row.split();pc=int(pc,16);raw=bytes.fromhex(raw);assert read(pc,len(raw))==raw==image.read(pc,len(raw));verified+=1
 context=fs_context();thread_context=context;save('before.json');assert word(0x391278)==0 and word(0x3912a4)==0x390bc0
 calls=[]
 for number,slot,label in ((1,0x41b680,'enter'),(2,0x41b79c,'allocate'),(3,0x41b7ec,'capacity'),(4,0x41b79c,'allocate'),(5,0x41b7ec,'capacity')):
  target=word(slot);stop=breakpoint(target);gdb.execute('continue');assert reg('eip')==target;stop.delete();before=state();sp=reg('esp');return_pc=word(sp)
  args=[word(sp+4+i*4)for i in range(2 if label=='allocate'else 1)];returning=breakpoint(return_pc);gdb.execute('continue');assert reg('eip')==return_pc;returning.delete();after=state()
  row=dict(id=number,kind=label,import_slot=slot,entry=target,return_pc=return_pc,arguments=args,result=reg('eax'),before=before,after=after)
  (out/f'call-{number:02}.json').write_text(json.dumps(row,separators=(',',':'))+'\n');calls.append({k:v for k,v in row.items()if k not in('before','after')});print(label,number,[hex(a)for a in args],hex(reg('eax')),flush=True)
 stop=breakpoint(0x37da9a);gdb.execute('continue');assert reg('eip')==0x37da9a;stop.delete();save('saved-setting-before.json');sp=reg('esp');ret=word(sp)
 stop=breakpoint(ret);gdb.execute('continue');assert reg('eip')==ret;stop.delete();save('saved-setting-after.json')
 report=dict(scope=__doc__,original_sha256=PIN,original_cold_boot=True,verified_original_instruction_rows=verified,thread_context=context,calls=calls,saved_setting_result=reg('eax'),saved_setting_return=ret,
 original_code_patches=[],original_memory_writes=[],original_register_writes=[],supplied_callee_returns=[],native_or_physical_pcm_verified=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='calls'}),flush=True)
