"""Observe two whole original3806BE sample-page acquisitions during Xbox boot.

All original pool allocation, page locks, physical translations, device FIFO
and MMIO execute normally. Hardware breakpoints only observe. Sample payload
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
 output=Path(os.environ['H2_SOUND_SAMPLE_PAGES_OUTPUT']);output.mkdir(parents=True,exist_ok=False);reports=[]
 for attempt in range(512):
  reach(0x3806be);manager=reg('ecx');stack=reg('esp');caller=word(stack);buffer=word(stack+4);size=word(stack+8)
  pages=((buffer&0xfff)+size+0xfff)&0xffffffff;pages>>=12
  if not 0<pages<=16:gdb.execute('stepi');continue
  directory=output/f'acquire-{len(reports)}';directory.mkdir();regions=[]
  def backing(name,address,size):regions.append(dict(name=name,address=f'{address:08x}',size=size,before_hex=read(address,size).hex()))
  backing('manager',manager,0x20);backing('available_count',word(manager+0x1c),4)
  nodes=set()
  for head in (manager+4,manager+0xc):
   node=word(head);seen=set()
   while node!=head:
    assert node not in seen and len(seen)<65536,(hex(head),hex(node));seen.add(node);nodes.add(node);node=word(node)
  pool=word(manager+0x14);assert nodes and min(nodes)==pool
  assert all((node-pool)%20==0 for node in nodes)
  extent=max(nodes)-pool+20;assert extent==len(nodes)*20,'Pool nodes do not cover a contiguous actual allocation'
  backing('node_pool',pool,extent)
  unchanged=[]
  for folder in ('sdk-sample-pages-dependencies-base-v1','sdk-sample-page-ownership-base-v1'):
   for p in (ROOT/'analysis/audio-spatial-20261011'/folder).glob('*.asm'):
    for line in p.read_text().splitlines():
     pc,raw,*_=line.split();encoded=bytes.fromhex(raw);assert image.read(int(pc,16),len(encoded))==read(int(pc,16),len(encoded));unchanged.append(dict(pc=pc,bytes=raw))
  report=dict(boundary=__doc__,entry='003806be',image_sha256=sha(image_path),tool_sha256=sha(__file__),registers={n:reg(n)for n in ('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','mxcsr')},manager=f'{manager:08x}',buffer=f'{buffer:08x}',bytes=size,page_base=f'{buffer&0xfffff000:08x}',pages=pages,regions=regions,unchanged_instructions=unchanged,pool_extent_from_actual_list_nodes=True,kernel_calls=[],observed_device_writes=[],instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[])
  (directory/'entry.json').write_text(json.dumps(report,indent=2)+'\n')
  choices=[bp(pc)for pc in (0x380746,0x380327,caller)];gdb.execute('continue');pc=reg('eip')
  for b in choices:b.delete()
  if pc==0x380746:report['path']='cached';reach(caller)
  elif pc==0x380327:
   report['path']='allocated';reach(0x380718);report['allocated_node']=f'{reg("eax"):08x}'
   if reg('eax'):
    reach(0x380419);mapping_stack=reg('esp');first=word(mapping_stack+4);mapped_buffer=word(mapping_stack+8);mapped_pages=word(mapping_stack+12);assert mapped_pages==pages
    reach(0x380431);args=[word(reg('esp')+i*4)for i in range(3)];assert args==[mapped_buffer,pages<<12,0]
    reach(0x380437);report['kernel_calls'].append(dict(name='lock_pages',arguments=[f'{a:08x}'for a in args],actual_return_eax=f'{reg("eax"):08x}'))
    for index in range(pages):
     reach(0x380443);argument=word(reg('esp'));assert argument==(mapped_buffer+index*0x1000)&0xffffffff
     reach(0x380449);physical=reg('eax');report['kernel_calls'].append(dict(name='physical_address',arguments=[f'{argument:08x}'],actual_return_eax=f'{physical:08x}'))
     reach(0x380460);report['observed_device_writes'].append(dict(pc='00380460',address='fe820804',value=f'{reg("edi"):08x}'));assert reg('edi')==first+index
     reach(0x380469);report['observed_device_writes'].append(dict(pc='00380469',address='fe820808',value=f'{reg("esi"):08x}'));assert reg('esi')==physical
   reach(caller)
  else:assert pc==caller;report['path']='unavailable'
  assert reg('esp')==stack+12
  for region in regions:region['after_hex']=read(int(region['address'],16),region['size']).hex()
  report.update(actual_return_pc=f'{caller:08x}',actual_return_eax=f'{reg("eax"):08x}',original_whole_acquisition_returned_normally=True,device_writes_executed_normally=True,native_or_physical_DSP_claimed=False)
  (directory/'report.json').write_text(json.dumps(report,indent=2)+'\n');reports.append(dict(path=report['path'],report_sha256=sha(directory/'report.json')))
  print(f'Original acquisition {len(reports)} {report["path"]}, {pages} pages, {len(report["observed_device_writes"])} device words',flush=True)
  if len(reports)==2:break
 else:raise RuntimeError('Two bounded sample-page acquisitions not reached')
 (output/'report.json').write_text(json.dumps(dict(boundary=__doc__,captures=reports,original_cold_boot=True,instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[]),indent=2)+'\n')
