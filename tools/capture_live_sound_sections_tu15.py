"""Observe real TU1.5 paired DSP section loads without guest mutations."""
import os,json,hashlib,struct,sys,gdb
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
path=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
pin='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd';assert hashlib.sha256(path.read_bytes()).hexdigest()==pin
image=Xbe(path);out=Path(os.environ['H2_SOUND_ENVIRONMENT_OUTPUT']);out.mkdir(parents=True,exist_ok=False)
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
word=lambda p:struct.unpack('<I',read(p,4))[0]
reg=lambda n:int(gdb.parse_and_eval('$'+n))&0xffffffff
bp=lambda p:gdb.Breakpoint(f'*0x{p:x}',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
base=image.base;table=struct.unpack_from('<I',image.data,0x120)[0];index=next(i for i,s in enumerate(image.sections)if s.name=='DSPImage');section=image.sections[index];header=table+index*56

def state():
 raw=read(header,56);head,tail=struct.unpack_from('<2I',raw,28)
 return dict(header=header,header56_hex=raw.hex(),references=struct.unpack_from('<I',raw,24)[0],
  head_reference_hex=read(head,2).hex(),tail_reference_hex=read(tail,2).hex(),
  resident_bytes_equal_owned_image=read(section.va,section.vsize)==image.section_bytes(section))

def capture_sections():
 entry=bp(0x1972c0);gdb.execute('continue');assert reg('eip')==0x1972c0;entry.delete()
 text=image.section('.text');assert read(text.va,text.rsize)==image.section_bytes(text)
 rows=[]
 for target,label in ((0x2df851,'load'),(0x2df86c,'free')):
  stop=bp(target);gdb.execute('continue');assert reg('eip')==target;stop.delete();sp=reg('esp');caller=word(sp);name=word(sp+4)
  assert read(name,9)==b'DSPImage\0';before=state();returning=bp(caller);gdb.execute('continue');assert reg('eip')==caller;returning.delete();after=state()
  rows.append(dict(kind=label,entry=target,caller=caller,name=name,result=reg('eax'),before=before,after=after))
  print(label,'references',before['references'],'->',after['references'],'result',hex(reg('eax')),flush=True)
 assert rows[0]['after']['references']==rows[0]['before']['references']+1
 assert rows[1]['after']['references']==rows[1]['before']['references']-1
 assert rows[1]['after']['references']==rows[0]['before']['references']
 assert all(r['before']['resident_bytes_equal_owned_image']and r['after']['resident_bytes_equal_owned_image']for r in rows)
 report=dict(original_sha256=pin,original_preload_flag=section.flags,section_va=section.va,section_bytes=section.vsize,
  rows=rows,original_cold_boot=True,original_code_patches=[],original_memory_writes=[],original_register_writes=[],supplied_callee_returns=[],
  kernel_vm_or_physical_pcm_claimed=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
