"""Observe real TU1.5 audio query and boot-mode reader without supplying state."""
from pathlib import Path
import sys,json,hashlib,socket,re
sys.path.insert(0,str(Path(__file__).resolve().parent))
import capture_live_sound_environment_tu15 as reference
from capture_live_sound_environment_tu15 import *
base_banks=reference.bank_state;base_abi=reference.abi;fs_base=None

def banks():
 r=base_banks();s=image.section('DSOUND');r.append(dict(label='actual SDK code and runtime state',address=s.va,size=s.vsize,writable=True,hex=read(s.va,s.vsize).hex()));r.append(dict(label='actual source thread IRQL',address=fs_base+0x24,size=1,writable=True,hex=read(fs_base+0x24,1).hex()));return sorted(r,key=lambda r:r['address'])
def abi_with_fs():
 a=base_abi();a['fs_base']=fs_base;return a
reference.bank_state=banks;reference.abi=abi_with_fs

def save(name):
 a=state();(out/name).write_text(json.dumps(a,separators=(',',':'))+'\n');return a

def capture_audio():
 global fs_base
 stop=breakpoint(0x37da9a);gdb.execute('continue');assert reg('eip')==0x37da9a;stop.delete();reference.stack_anchor=reg('esp')&~4095
 q=socket.socket(socket.AF_UNIX);q.connect(str(Path(os.environ['H2_SOUND_ENVIRONMENT_SESSION'])/'qmp.sock'));f=q.makefile('rwb',buffering=0);json.loads(f.readline())
 def call(command,args=None):
  f.write((json.dumps(dict(execute=command,**({'arguments':args}if args else{})))+'\n').encode())
  while True:
   result=json.loads(f.readline())
   if 'return'in result:return result['return']
   if 'error'in result:raise RuntimeError(result['error'])
 call('qmp_capabilities');registers=call('human-monitor-command',{'command-line':'info registers'});f.close();q.close();row=next(line for line in registers.splitlines()if re.match(r'FS\s*=',line));match=re.match(r'FS\s*=\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)',row);assert match;fs_base=int(match[1],16)
 text=image.section('.text');assert read(text.va,text.rsize)==image.section_bytes(text)
 parent=word(reg('esp'));save('helper-before.json')
 stop=breakpoint(0x2e101c);gdb.execute('continue');assert reg('eip')==0x2e101c;stop.delete();save('query-before.json');sp=reg('esp');ret=word(sp);args=[word(sp+4+i*4)for i in range(5)];assert ret==0x37dab2 and args[0]==9 and args[3:]==[4,0]
 stop=breakpoint(ret);gdb.execute('continue');assert reg('eip')==ret;stop.delete();save('query-after.json');status=reg('eax');kind=word(args[1]);value=word(args[2]);assert status==0 and kind==4
 stop=breakpoint(0x33c29d);gdb.execute('continue');assert reg('eip')==0x33c29d;stop.delete();save('mode-before.json');mode_pointer=word(0x41b784);actual_mode=word(mode_pointer);mode_ret=word(reg('esp'))
 stop=breakpoint(mode_ret);gdb.execute('continue');assert reg('eip')==mode_ret;stop.delete();save('mode-after.json');assert reg('eax')==actual_mode
 stop=breakpoint(parent);gdb.execute('continue');assert reg('eip')==parent;stop.delete();save('helper-after.json')
 eeprom=Path(os.environ['H2_SOUND_ENVIRONMENT_SESSION'])/'eeprom.bin';actual=eeprom.read_bytes();assert len(actual)==256
 report=dict(scope=__doc__,original_sha256=PIN,original_cold_boot=True,query_arguments=args,query_status=status,query_type=kind,query_audio_flags=value,helper_result=reg('eax'),helper_parent_return=parent,
 boot_smc_video_mode=dict(import_slot=0x41b784,ordinal=356,address=mode_pointer,actual_word=actual_mode,hex=read(mode_pointer,4).hex(),native_default_or_live_native_mode_claimed=False),
 eeprom_user_settings=dict(path=str(eeprom),section_offset=96,section_bytes=96,sha256=hashlib.sha256(actual[96:192]).hexdigest(),actual_audio_field_offset=56,audio_flags=int.from_bytes(actual[152:156],'little'),encrypted_or_factory_section_retained=False),
 fs_register_row=row,fs_base=fs_base,original_code_patches=[],original_memory_writes=[],original_register_writes=[],supplied_callee_returns=[],physical_xbox_or_native_mode_verified=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 assert report['eeprom_user_settings']['audio_flags']==value
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
