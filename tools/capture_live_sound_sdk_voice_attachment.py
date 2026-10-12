"""Observe whole original382B7E buffered and382BE3 streamed voice attachment.

Original dirty flags, buffer assignment, cursor packing, pool allocation,
page locks, physical translations, device FIFO and MMIO execute normally.
Hardware breakpoints only observe. Sample payload
bytes are not CPU inputs to these owners and are not claimed waveform proof.
"""
import hashlib,json,os,struct,sys
from pathlib import Path
import gdb
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
image_path=ROOT/'assets/disc/default.xbe';sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image=Xbe(image_path);inferior=gdb.selected_inferior();read=lambda a,n:bytes(inferior.read_memory(a,n))
word=lambda a:struct.unpack('<I',read(a,4))[0];reg=lambda n:int(gdb.parse_and_eval('$'+n))&0xffffffff
bp=lambda pc:gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
def reach(pc):
 b=bp(pc);gdb.execute('continue');assert reg('eip')==pc,(hex(reg('eip')),hex(pc));b.delete()
def capture():
 output=Path(os.environ['H2_SOUND_VOICE_ATTACHMENT_OUTPUT']);output.mkdir(parents=True,exist_ok=False);reports=[];counts={'buffer':0,'stream':0}
 for attempt in range(512):
  choices=[bp(pc)for pc in (0x382b7e,0x382be3)];gdb.execute('continue');entry=reg('eip')
  for b in choices:b.delete()
  assert entry in (0x382b7e,0x382be3);kind='buffer'if entry==0x382b7e else'stream'
  voice=reg('ecx');data=word(voice+0x80);device=word(voice+8);manager=device+0x60;stack=reg('esp');caller=word(stack)
  buffer=word(data+0xbc);size=word(data+0xc0);pages=(((buffer&0xfff)+size+0xfff)&0xffffffff)>>12
  if counts[kind]>=(2 if kind=='buffer'else 1)or int.from_bytes(read(voice+0x12,2),'little')&0x100 or(kind=='buffer'and not 0<pages<=16):gdb.execute('stepi');continue
  directory=output/f'{kind}-{counts[kind]}';directory.mkdir();regions=[]
  def backing(name,address,size):
   if not any(int(r['address'],16)<=address and address+size<=int(r['address'],16)+r['size']for r in regions):regions.append(dict(name=name,address=f'{address:08x}',size=size,before_hex=read(address,size).hex()))
  backing('voice',voice,0x158);backing('data',data,0xe0);backing('device',device,0x540);backing('available_count',word(manager+0x1c),4)
  format_data=word(voice+0x70)
  if format_data!=data:backing('format_data',format_data,0xe0)
  nodes=set()
  for head in (manager+4,manager+0xc):
   node=word(head);seen=set()
   while node!=head:
    assert node not in seen and len(seen)<65536;seen.add(node);nodes.add(node);node=word(node)
  pool=word(manager+0x14);assert nodes and min(nodes)==pool and all((node-pool)%20==0 for node in nodes)
  extent=max(nodes)-pool+20;assert extent==len(nodes)*20;backing('node_pool',pool,extent);backing('stream_samples_pointer',0x387330,4)
  unchanged=[]
  for folder in ('sdk-sample-pages-dependencies-base-v1','sdk-sample-page-ownership-base-v1','sdk-voice-attachment-dependencies-base-v1','sdk-image-pairing-v21/instructions'):
   for p in (ROOT/'analysis/audio-spatial-20261011'/folder).glob('*.asm'):
    if folder.endswith('/instructions')and not p.name.endswith('-base.asm'):continue
    for line in p.read_text().splitlines():
     pc,raw,*_=line.split();encoded=bytes.fromhex(raw);assert image.read(int(pc,16),len(encoded))==read(int(pc,16),len(encoded));unchanged.append(dict(pc=pc,bytes=raw))
  report=dict(boundary=__doc__,entry=f'{entry:08x}',kind=kind,image_sha256=sha(image_path),tool_sha256=sha(__file__),registers={n:reg(n)for n in ('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','mxcsr')},voice=f'{voice:08x}',buffer=f'{buffer:08x}',bytes=size,page_base=f'{buffer&0xfffff000:08x}',pages=pages,regions=regions,unchanged_instructions=unchanged,pool_extent_from_actual_list_nodes=True,kernel_calls=[],allocation_calls=[],observed_device_writes=[],instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[])
  (directory/'entry.json').write_text(json.dumps(report,indent=2)+'\n')
  if kind=='buffer':
   reach(0x3806be);assert reg('ecx')==manager and word(reg('esp'))==0x382bb3
   choices=[bp(pc)for pc in (0x380746,0x380327,0x382bb3)];gdb.execute('continue');pc=reg('eip')
   for b in choices:b.delete()
   if pc==0x380746:report['path']='cached';reach(0x382bb3)
   elif pc==0x380327:
    report['path']='allocated';reach(0x380718);report['allocated_node']=f'{reg("eax"):08x}';assert reg('eax')
    reach(0x380419);mapping_stack=reg('esp');first=word(mapping_stack+4);mapped_buffer=word(mapping_stack+8);mapped_pages=word(mapping_stack+12);assert mapped_pages==pages
    reach(0x380431);args=[word(reg('esp')+i*4)for i in range(3)];assert args==[mapped_buffer,pages<<12,0]
    reach(0x380437);report['kernel_calls'].append(dict(name='lock_pages',arguments=[f'{a:08x}'for a in args],actual_return_eax=f'{reg("eax"):08x}'))
    for index in range(pages):
     reach(0x380443);argument=word(reg('esp'));reach(0x380449);physical=reg('eax');report['kernel_calls'].append(dict(name='physical_address',arguments=[f'{argument:08x}'],actual_return_eax=f'{physical:08x}'))
     reach(0x380460);report['observed_device_writes'].append(dict(pc='00380460',address='fe820804',value=f'{reg("edi"):08x}'));assert reg('edi')==first+index
     reach(0x380469);report['observed_device_writes'].append(dict(pc='00380469',address='fe820808',value=f'{reg("esi"):08x}'));assert reg('esi')==physical
    reach(0x382bb3)
   else:raise RuntimeError('Genuine buffer attachment did not acquire pages')
   report['acquired_entry']=f'{reg("eax"):08x}';reach(0x3824ee);report['cursor_argument']=f'{word(reg("esp")+4):08x}';reach(caller);assert reg('eax')==0
  else:
   report['path']='stream';reach(0x37f43e);assert word(reg('esp')+4)==32;reach(0x382c02);report['stream_bytes']=reg('eax')
   reach(0x37c8ab);source=reg('ecx');input_buffer=word(reg('esp')+4);input_bytes=word(reg('esp')+8);report['stream_buffer']=f'{input_buffer:08x}';report['stream_buffer_bytes']=input_bytes
   if not word(source+8)&0x80000000 and word(source+0xbc):
    reach(0x37d9ba);report['allocation_calls'].append(dict(name='release_allocation',arguments=[f'{word(reg("esp")+4):08x}']));reach(0x37c8d3)
   if input_bytes and not input_buffer:
    reach(0x37d952);allocation_stack=reg('esp');arguments=[word(allocation_stack+4+i*4)for i in range(3)];return_pc=word(allocation_stack);reach(return_pc);report['allocation_calls'].append(dict(name='allocate',arguments=[f'{a:08x}'for a in arguments],actual_return_eax=f'{reg("eax"):08x}'))
   reach(0x3824ee);report['cursor_argument']=f'{word(reg("esp")+4):08x}';reach(caller)
  assert reg('esp')==stack+4
  for region in regions:region['after_hex']=read(int(region['address'],16),region['size']).hex()
  report.update(actual_return_pc=f'{caller:08x}',actual_return_eax=f'{reg("eax"):08x}',original_whole_attachment_returned_normally=True,device_writes_executed_normally=True,native_or_physical_DSP_claimed=False)
  (directory/'report.json').write_text(json.dumps(report,indent=2)+'\n');counts[kind]+=1;reports.append(dict(kind=kind,path=report['path'],report_sha256=sha(directory/'report.json')))
  (output/'report.json').write_text(json.dumps(dict(boundary=__doc__,captures=reports,original_cold_boot=True,complete=counts=={'buffer':2,'stream':1},instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[]),indent=2)+'\n')
  print(f'Original {kind} attachment {counts[kind]} {report["path"]}, {len(report["observed_device_writes"])} device words',flush=True)
  if counts=={'buffer':2,'stream':1}:break
 else:raise RuntimeError('Required whole attachments not reached')
