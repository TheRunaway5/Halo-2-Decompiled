"""Observe genuine TU1.5 driver creation and capabilities without guest writes."""
from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parent))
import capture_live_sound_environment_tu15 as reference
from capture_live_sound_environment_tu15 import *

def save(name):
 s=state();(out/name).write_text(json.dumps(s,separators=(',',':'))+'\n');return s

def capture_device():
 stop=breakpoint(0x224ec0);gdb.execute('continue');assert reg('eip')==0x224ec0;stop.delete()
 reference.stack_anchor=reg('esp')&~4095
 text=image.section('.text');assert read(text.va,text.rsize)==image.section_bytes(text)
 save('before.json')
 stop=breakpoint(0x387877);gdb.execute('continue');assert reg('eip')==0x387877;stop.delete();save('create-before.json')
 sp=reg('esp');return_pc=word(sp);args=[word(sp+4+i*4)for i in range(3)];driver=word(0x52d674)
 assert return_pc==0x224f2b and args==[0,driver+0x2ab0,0] and word(args[1])==0 and read(driver,1)==b'\0'
 stop=breakpoint(return_pc);gdb.execute('continue');assert reg('eip')==return_pc;stop.delete();save('create-after.json')
 result=reg('eax');device=word(args[1]);assert result==0 and device and reg('esp')==sp+16
 stop=breakpoint(0x38568e);gdb.execute('continue');assert reg('eip')==0x38568e;stop.delete();save('capabilities-before.json')
 sp=reg('esp');caps_pc=word(sp);caps_args=[word(sp+4+i*4)for i in range(2)];assert caps_pc==0x224f4b and caps_args==[device,driver+0x2aa0]
 stop=breakpoint(caps_pc);gdb.execute('continue');assert reg('eip')==caps_pc;stop.delete();save('capabilities-after.json')
 report=dict(scope=__doc__,original_sha256=PIN,original_cold_boot=True,driver=driver,creation_arguments=args,creation_result=result,device=device,
 capabilities_arguments=caps_args,capabilities_result=reg('eax'),capabilities16_hex=read(driver+0x2aa0,16).hex(),
 original_code_patches=[],original_memory_writes=[],original_register_writes=[],supplied_callee_returns=[],physical_pcm_verified=False,
 tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),reference_helper_sha256=hashlib.sha256(Path(reference.__file__).read_bytes()).hexdigest())
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
