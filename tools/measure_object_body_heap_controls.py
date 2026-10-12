#!/usr/bin/env python3
"""Execute complete paired heap owners on source-created storage and declared raw allocator controls."""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import OLD,NEW,PINS,Xbe,sha,save,_Code
STACK,STOP,NAME=0x02008000,0x03000000,0x70000000

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--owning',type=Path,required=True);p.add_argument('--pair',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 owning=json.loads(a.owning.read_text());pair=json.loads(a.pair.read_text());init=next(c for c in owning['calls']if c['entry']=='0018fc80');allocation=next(c for c in owning['calls']if c['entry']=='0018fce0');heap=init['heap'];state=owning['storage_states'][init['before']['storage']];initial=(a.owning.parent/state['file']).read_bytes();assert sha(a.owning.parent/state['file'])==state['sha256'];headers=allocation['before']['object_headers'];slotbase=int(headers['address'],16);slotraw=bytes.fromhex(headers['raw24576'])
 oldimage=Xbe(OLD);nameptr=int(init['stack'][0],16);name=oldimage.read(nameptr,32)
 targets={int(o['old_entry'],16):int(o['u5_entry'],16)for o in pair['owners']};maps=[{},{}];allowed=[set(),set()];clock={}
 for o in pair['owners']:
  rows=[(a.pair.parent/'instructions'/f'{o["old_entry"]}-{s}.asm').read_text().splitlines()for s in('old','u5')]
  for l,r in zip(*rows,strict=True):
   old,new=int(l.split()[0],16),int(r.split()[0],16)
   for side,pc in enumerate((old,new)):maps[side][pc]=old;allowed[side].add(pc)
  for c in o['direct_calls']:
   if c['old_target']=='00337a40':clock={0:0x337a40,1:int(c['u5_target'],16)}
 specs=[]
 for size in(0,1,15,16,17,31,32,63,64,1124,32767,0xffff0,0xffff1,0xfffff,0x100000,0xffffffff):specs.append((f'size_{size:08x}',[('allocate',0,size)]))
 specs += [
 ('resize_last',[('allocate',0,64),('pattern',0,80),('resize',0,128),('resize',0,17),('resize',0,0)]),
 ('resize_relocation',[('allocate',0,65),('pattern',0,80),('allocate',1,80),('resize',0,128)]),
 ('resize_failure',[('allocate',0,64),('allocate',1,0xfff90),('resize',0,128)]),
 ('free_all',[('allocate',0,64),('allocate',1,80),('allocate',2,96),('free',1,0),('free',0,0),('free',2,0)]),
 ('append_without_gap_flag',[('allocate',0,64),('allocate',1,80),('free',0,0),('allocate',2,32)]),
 ('head_gap',[('flag',0x3d,1),('allocate',0,64),('allocate',1,80),('free',0,0),('allocate',2,32)]),
 ('middle_gap',[('flag',0x3d,1),('allocate',0,64),('allocate',1,80),('allocate',2,96),('free',1,0),('allocate',3,48)]),
 ('middle_gap_too_small',[('flag',0x3d,1),('allocate',0,64),('allocate',1,80),('allocate',2,96),('free',1,0),('allocate',3,96)]),
 ('gap_flag_empty',[('flag',0x3d,1),('allocate',0,64)]),
 ('owner_tracking_disabled',[('flag',0x3c,1),('allocate',0,65),('pattern',0,80),('allocate',1,80),('resize',0,128),('free',0,0)]),
 ('exhaustion_then_free',[('allocate',0,0xffff0),('allocate',1,0),('free',0,0),('allocate',2,0xffff0)]),
 ('debug_allocation_stop',[('flag',0x3f,1),('allocate',0,64)]),
 ('debug_resize_stop',[('allocate',0,64),('flag',0x3f,1),('resize',0,128)]),
 ]
 for header_shift in(0,1,2,3,15):
  for name_shift in(0,1,2,3):specs.append((f'initializer_alignment_{header_shift}_{name_shift}',[],header_shift,name_shift))
 # Varied name bytes are explicit CRT controls only, never live asset defaults.
 for length in(0,1,2,3,4,7,15,31,32,40):
  for name_shift in(0,1,2,3):specs.append((f'initializer_length_{length}_{name_shift}',[],0,name_shift,length))
 states={};cases=[]
 def persist(raw):
  import hashlib
  digest=hashlib.sha256(raw).hexdigest();path=out/'storage'/f'{digest}.bin';path.parent.mkdir(exist_ok=True)
  if not path.exists():path.write_bytes(raw)
  states[digest]=dict(bytes=len(raw),file=str(path.relative_to(out)),sha256=digest);return digest
 for side,imagepath in enumerate((OLD,NEW)):
  assert sha(imagepath)==PINS[str(imagepath)];image=Xbe(imagepath)
  for spec in specs:
   title,actions=spec[:2];shift=spec[2]if len(spec)>2 else 0;ns=spec[3]if len(spec)>3 else 0;length=spec[4]if len(spec)>4 else None;actualheap=heap+shift;namestorage=name if length is None else bytes((0x41+i%26)for i in range(length))+bytes(64-length)
   g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
   for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
   pages=set(range(0x10000,0x810000,0x1000))
   for base,n in ((heap,len(initial)),(slotbase,len(slotraw)),(STACK-0x10000,0x20000),(STOP,0x1000),(NAME,0x1000)):
    for pg in range(base&~4095,(base+n+4095)&~4095,4096):
     if pg not in pages:g.mem_map(pg,4096);pages.add(pg)
   g.mem_write(heap,initial);g.mem_write(slotbase,slotraw);g.mem_write(NAME+ns,namestorage);rows=[];stores=[];calls=[];current=None;stopped=False
   def hook(_g,pc,n,user):
    nonlocal stopped
    if pc==clock[side]:stopped=True;g.emu_stop();return
    assert pc in allowed[side],(title,hex(pc))
    if pc in set(targets.values())if side else pc in targets:calls.append(dict(entry=f'{next(k for k,v in targets.items()if v==pc)if side else pc:08x}',instruction_count_position=len(stores)))
   def write(_g,kind,addr,n,value,user):
    if STACK-0x10000<=addr<STACK+0x10000:return
    stores.append(dict(instruction=f'{maps[side][g.reg_read(UC_X86_REG_EIP)]:08x}',address=f'{addr:08x}',bytes=n,before=bytes(g.mem_read(addr,n)).hex(),after=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
   g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
   def snapshot():return dict(heap_storage=persist(bytes(g.mem_read(heap,len(initial)))),slots=bytes(g.mem_read(slotbase,len(slotraw))).hex())
   def run(kind,index,value):
    nonlocal current,stopped
    slot=slotbase+12*index+8;before=snapshot();begin=len(stores);cb=len(calls);stopped=False;entry=dict(initialize=0x18fc80,allocate=0x18fce0,resize=0x18fd70,free=0x18fe60)[kind];regs={UC_X86_REG_EAX:0,UC_X86_REG_EBX:0,UC_X86_REG_ECX:0,UC_X86_REG_EDX:0,UC_X86_REG_ESI:0,UC_X86_REG_EDI:0,UC_X86_REG_EBP:0};args=[]
    if kind=='initialize':regs.update({UC_X86_REG_ESI:actualheap,UC_X86_REG_EBX:0x100000});args=[NAME+ns,0]
    elif kind=='allocate':regs[UC_X86_REG_ECX]=actualheap;args=[slot,value,0x10203040]
    elif kind=='resize':regs.update({UC_X86_REG_EAX:value,UC_X86_REG_EBX:slot});args=[actualheap,0x10203040,0x50607080]
    else:regs.update({UC_X86_REG_EAX:slot,UC_X86_REG_EDX:actualheap})
    for r,v in regs.items():g.reg_write(r,v)
    g.reg_write(UC_X86_REG_ESP,STACK);g.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));g.emu_start(targets[entry]if side else entry,STOP,count=200000)
    assert stopped or g.reg_read(UC_X86_REG_EIP)==STOP,(title,kind,hex(g.reg_read(UC_X86_REG_EIP)))
    rows.append(dict(kind=kind,index=index,value=value,heap=actualheap,output_slot=slot,resize_temporary_slot=STACK+8,name_pointer=NAME+ns,name_storage=namestorage.hex(),before=before,after=snapshot(),return_al=g.reg_read(UC_X86_REG_EAX)&255,return_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',stopped_before_clock=stopped,stores=stores[begin:],calls=calls[cb:]))
   run('initialize',0,0)
   for kind,index,value in actions:
    if kind=='flag':g.mem_write(actualheap+index,bytes([value]));rows.append(dict(kind=kind,offset=index,value=value));continue
    if kind=='pattern':
     addr=int.from_bytes(g.mem_read(slotbase+12*index+8,4),'little');raw=bytes((i*37+11)&255 for i in range(value));g.mem_write(addr,raw);rows.append(dict(kind=kind,index=index,address=addr,raw=raw.hex()));continue
    run(kind,index,value)
    if stopped:break
   cases.append(dict(image=side+1,name=title,heap_storage_address=heap,slot_storage_address=slotbase,rows=rows))
 half=len(cases)//2
 for l,r in zip(cases[:half],cases[half:],strict=True):
  assert l['name']==r['name']
  for x,y in zip(l['rows'],r['rows'],strict=True):
   assert x['kind']==y['kind']
   if 'after'in x:assert x['after']==y['after']and x['stores']==y['stores']and x['stopped_before_clock']==y['stopped_before_clock'],l['name']
 save(out/'report.json',dict(schema='original-object-body-heap-controls-v1',tool_sha256=sha(__file__),owning_sha256=sha(a.owning),pair_sha256=sha(a.pair),storage_states=states,cases=cases,source_capacity=0x100000,initial_storage_provenance=state,source_literal_name_provenance=dict(address=f'{nameptr:08x}',raw32=name.hex(),source_image_sha256=sha(OLD)),instruction_patches=[],supplied_allocator_results=[],scope='Source-created full heap/raw header storage. Each sequence executes original initializer then complete allocation/growth/free helpers. Flag, request sizes, private-frame aliases and synthetic allocator payload bytes are explicitly declared kernel controls, never Unit/world state. Debug branches stop before unavailable actual clock owner; no clock return supplied.'))
 print(json.dumps(dict(report_sha256=sha(out/'report.json'),cases=len(cases),actions=sum('after'in q for c in cases for q in c['rows']),stores=sum(len(q.get('stores',[]))for c in cases for q in c['rows']),storage_states=len(states))))
if __name__=='__main__':main()
