"""Observe the actual kernel writer of boot SMC video mode in an owned cold boot.

Hardware execution/write breakpoints and read-only code/data observations only.
The modified Complex4627/xemu platform is named; physical retail equivalence
and a current native SMC/video-mode producer are not asserted.
"""
from pathlib import Path
import sys,json,hashlib,struct,gdb
sys.path.insert(0,str(Path(__file__).resolve().parent))
from capture_live_sound_environment_tu15 import *

def capture_mode_producer():
 stop=breakpoint(0x8005504c);gdb.execute('continue');assert reg('eip')==0x8005504c;stop.delete()
 base=0x80010000;assert read(base,2)==b'MZ';pe=base+word(base+0x3c);assert read(pe,4)==b'PE\0\0';coff=read(pe+4,20);count=struct.unpack_from('<H',coff,2)[0];optional_size=struct.unpack_from('<H',coff,16)[0];optional=read(pe+24,optional_size);assert struct.unpack_from('<H',optional)[0]==0x10b
 assert base+struct.unpack_from('<I',optional,16)[0]==reg('eip');export_rva=struct.unpack_from('<I',optional,96)[0];exports=read(base+export_rva,40);ordinal,count_exports,name_count,table_rva=struct.unpack_from('<IIII',exports,16);table=read(base+table_rva,count_exports*4);pointer=base+struct.unpack_from('<I',table,(356-ordinal)*4)[0]
 sections=[];section_table=pe+24+optional_size
 for index in range(count):
  raw=read(section_table+index*40,40);name=raw[:8].split(b'\0')[0].decode();size,rva=struct.unpack_from('<II',raw,8);flags=struct.unpack_from('<I',raw,36)[0]
  if not flags&0x20000000:continue
  payload=read(base+rva,size);filename=f'kernel-code-{index}.bin';(out/filename).write_bytes(payload);sections.append(dict(name=name,address=base+rva,bytes=size,flags=flags,file=filename,sha256=hashlib.sha256(payload).hexdigest()))
 initial=word(pointer);gdb.execute('set can-use-hw-watchpoints 1');watch=gdb.Breakpoint(f'*(unsigned int*)0x{pointer:x}',type=gdb.BP_WATCHPOINT,wp_class=gdb.WP_WRITE,internal=False);title=breakpoint(0x33c29d);request=breakpoint(0x80054f44);response=breakpoint(0x80054f49)
 inventory=gdb.execute('info breakpoints',to_string=True);assert 'hw watchpoint'in inventory.lower()or'hardware watchpoint'in inventory.lower(),inventory
 rows=[];previous=initial;transaction={}
 for _ in range(16):
  gdb.execute('continue');pc=reg('eip')
  if pc==0x33c29d:break
  if pc==0x80054f44:
   sp=reg('esp');args=[word(sp+4*i)for i in range(4)];assert args[:3]==[0x20,4,0]
   transaction['request']=dict(instruction=pc,target=0x80014645,args=args,gp=[reg(n)for n in('eax','ecx','edx','ebx','esp','ebp','esi','edi')],output_before=word(args[3]))
   continue
  if pc==0x80054f49:
   assert 'request'in transaction;transaction['response']=dict(instruction=pc,status=reg('eax'),output_word=word(transaction['request']['args'][3]),gp=[reg(n)for n in('eax','ecx','edx','ebx','esp','ebp','esi','edi')]);continue
  value=word(pointer);gp=[reg(n)for n in('eax','ecx','edx','ebx','esp','ebp','esi','edi')]
  row=dict(pc_after_store=pc,before=previous,after=value,gp_after=gp,window_address=pc-16,window48_hex=read(pc-16,48).hex());rows.append(row);previous=value;print('original boot-mode write',hex(pc),hex(row['before']),'->',hex(value),flush=True)
 else:raise RuntimeError('Unexpected repeated boot-mode writer observations')
 watch.delete();title.delete();request.delete();response.delete();assert word(0x41b784)==pointer and word(pointer)==previous
 text=image.section('.text');assert read(text.va,text.rsize)==image.section_bytes(text)
 report=dict(scope=__doc__,original_sha256=PIN,kernel_entry=base+struct.unpack_from('<I',optional,16)[0],boot_mode_export=dict(ordinal=356,address=pointer,initial_word=initial,current_word=word(pointer)),kernel_executable_sections=sections,writes=rows,smc_transaction=transaction,hardware_watchpoint_inventory=inventory,
 platform=dict(bios_path='/mnt/SSD4TB/Games/Xemu/bios/Complex_4627.bin',bios_sha256=hashlib.sha256(Path('/mnt/SSD4TB/Games/Xemu/bios/Complex_4627.bin').read_bytes()).hexdigest(),emulator_path=str(ROOT/'.tools/xemu-app/squashfs-root/usr/bin/xemu'),emulator_sha256=hashlib.sha256((ROOT/'.tools/xemu-app/squashfs-root/usr/bin/xemu').read_bytes()).hexdigest()),
 original_code_patches=[],original_memory_writes=[],original_register_writes=[],supplied_callee_returns=[],physical_retail_or_live_native_smc_mode_claimed=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(initial=initial,current=word(pointer),writes=len(rows),code_sections=len(sections))),flush=True)
