#!/usr/bin/env python3
"""Execute the supplied Vista MP combat path unchanged over actual PC tags.

Private resident metadata bytes and a table assembled from exact local/shared
entries are declared ABI inputs; the PC loader/preceding fixups are not run.
No tuning field, source callback, service return, executable byte or game file
is changed. Only report/oracle evidence files are written.
"""
import argparse,bisect,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_MXCSR
from inspect_pc_combat_tag_sample import PcCache,SOURCE,sha
from pc_postprocess_inspect import PE,VISTA,ROOT,OUT
BASE,TABLE,STACK,STOP=0x10000000,0x20000000,0x21000000,0x22000000
ENTRY,CALLER=0x46efdb,0x428403
PIN='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
def h(n):return f'{n:08x}'
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 assert sha(VISTA)==PIN
 pe=PE(VISTA);source,calls=pe.trace(ENTRY);assert len(source)==573
 for q in (ENTRY,CALLER):(a.output/f'{q:08x}.asm').write_text(pe.listing(q))
 level=PcCache(ROOT/'Halo 2 Vista installer/maps/lockout.map');shared=PcCache(ROOT/'Halo 2 Vista installer/maps/shared.map')
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(pe.base,(pe.image_size+4095)&~4095)
 for s in pe.sections:u.mem_write(s['va'],pe.data[s['file_offset']:s['file_offset']+s['raw_size']])
 # Each selected tag is copied verbatim to its authored relative address plus
 # one declared loader base. No tag field/pointer/reference is rewritten.
 residents={};table_sources=[]
 index=level.index;table=level.u(index+8)-(level.u(index)-index-32);count=level.u(index+24)
 raw_table=bytearray(level.data[table:table+count*16])
 for index,t in level.tags.items():
  c,tag=level,t
  if t['offset']is None:
   tag=shared.tags.get(index)
   if tag is None or tag['offset']is None:continue
   assert (tag['group'],tag['name'],tag['datum'])==(t['group'],t['name'],t['datum'])
   c=shared
   si=shared.index;st=shared.u(si+8)-(shared.u(si)-si-32)
   raw_table[index*16:index*16+16]=shared.data[st+index*16:st+index*16+16]
   table_sources.append(dict(index=index,source='shared',raw_entry_hex=bytes(raw_table[index*16:index*16+16]).hex()))
  residents[index]=(c,tag)
 # Bring shared resident entries reachable through another shared tag, retaining
 # identical original table slots and IDs. None are made up from names.
 for index,t in shared.tags.items():
  if index in residents or t['offset']is None:continue
  assert index<count
  old=level.tags.get(index)
  if old:assert(old['group'],old['name'],old['datum'])==(t['group'],t['name'],t['datum'])
  si=shared.index;st=shared.u(si+8)-(shared.u(si)-si-32)
  raw_table[index*16:index*16+16]=shared.data[st+index*16:st+index*16+16];residents[index]=(shared,t)
  table_sources.append(dict(index=index,source='shared',raw_entry_hex=bytes(raw_table[index*16:index*16+16]).hex()))
 intervals=[]
 for c,t in residents.values():
  if not t['size']:continue
  start=BASE+t['offset']+c.magic;end=start+t['size'];intervals.append((start&~4095,(end+4095)&~4095))
 merged=[]
 for start,end in sorted(intervals):
  if merged and start<=merged[-1][1]:merged[-1]=(merged[-1][0],max(end,merged[-1][1]))
  else:merged.append((start,end))
 for start,end in merged:u.mem_map(start,end-start)
 regions=[]
 for index,(c,t)in residents.items():
  if not t['size']:continue
  va=BASE+t['offset']+c.magic;raw=bytes(c.data[t['offset']:t['offset']+t['size']]);u.mem_write(va,raw)
  regions.append((va,va+t['size'],c,t))
 regions.sort(key=lambda q:(q[0],q[1]));starts=[q[0]for q in regions]
 for left,right in zip(regions,regions[1:]):assert left[1]<=right[0],(left[0:2],left[3],right[0:2],right[3])
 u.mem_map(TABLE,(len(raw_table)+4095)&~4095);u.mem_write(TABLE,bytes(raw_table));u.mem_map(STACK,0x10000);u.mem_map(STOP,4096)
 matg=next((c,t)for c,t in residents.values()if t['group']=='matg')
 scenario=level.named('scnr','scenarios\\multi\\lockout\\lockout')
 bindings=[(0x87cd50,TABLE),(0x87cd54,BASE),(0x882290,BASE),(0x879e70,BASE+matg[1]['offset']+matg[0].magic),(0x879e74,BASE+scenario['offset']+level.magic)]
 for slot,value in bindings:u.mem_write(slot,struct.pack('<I',value))
 sp=STACK+0x8000;u.mem_write(sp,struct.pack('<I',STOP));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 executed=[];stores=[]
 def code(g,p,n,user):
  assert p in source,('outside literal function',h(p))
  i=source[p];assert pe.read(p,i.size)==bytes(g.mem_read(p,i.size))
  assert i.mnemonic!='call',('unexpected source service',h(p),i.op_str)
  executed.append(p)
 def write(g,access,p,n,v,user):
  row=dict(instruction=h(g.reg_read(UC_X86_REG_EIP)),address=h(p),size=n,before_hex=bytes(g.mem_read(p,n)).hex(),after_hex=(v&((1<<(n*8))-1)).to_bytes(n,'little').hex())
  if STACK<=p<STACK+0x10000:row['region']='declared_abi_stack'
  else:
   idx=bisect.bisect_right(starts,p)-1;start,end,c,t=regions[idx];assert start<=p and p+n<=end
   row.update(region='actual_resident_metadata',tag=dict(group=t['group'],name=t['name'],datum=t['datum']),tag_relative_offset=p-start,file_offset=t['offset']+p-start,owner=str(c.path))
  stores.append(row)
 u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write);u.emu_start(ENTRY,STOP,count=5000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==sp+4
 original=json.loads(SOURCE.read_text());xbox=next(q for q in original['images']if q['image_label']=='owned_1_5')
 xs=[s for s in xbox['ordered_stores']if s['region']=='original_cache_metadata'];ps=[s for s in stores if s['region']=='actual_resident_metadata']
 assert len(xs)==len(ps)==41
 comparison=[]
 for x,p in zip(xs,ps):
  assert(x['tag']['group'],x['tag']['name'],x['after_hex'])==(p['tag']['group'],p['tag']['name'],p['after_hex'])
  comparison.append(dict(tag=p['tag'],vista_instruction=p['instruction'],xbox_instruction=x['instruction'],vista_tag_relative_offset=p['tag_relative_offset'],xbox_tag_relative_offset=x['tag_relative_offset'],raw_vista_hex=p['before_hex'],processed_hex=p['after_hex'],equal_output=True))
 changed=[]
 for start,end,c,t in regions:
  raw=bytes(c.data[t['offset']:t['offset']+t['size']]);processed=bytes(u.mem_read(start,end-start))
  if raw!=processed:
   d=a.output/'tag-buffers';d.mkdir(exist_ok=True);stem=t['datum']+'-'+t['group'].replace('!','_')
   (d/(stem+'-raw.bin')).write_bytes(raw);(d/(stem+'-processed.bin')).write_bytes(processed)
   changed.append(dict(tag=t,owner=str(c.path),raw_sha256=__import__('hashlib').sha256(raw).hexdigest(),processed_sha256=__import__('hashlib').sha256(processed).hexdigest()))
 r=dict(scope='Complete unchanged Vista function at 0046EFDB, actual MP path over exact private resident Lockout/shared metadata. The source caller, loader, preceding group fixups, campaign branch, and gameplay are not executed.',
 executable_path=str(VISTA),executable_sha256=PIN,tool_sha256=sha(Path(__file__)),helper_sha256=sha(ROOT/'tools/pc_postprocess_inspect.py'),reader_sha256=sha(ROOT/'tools/inspect_pc_combat_tag_sample.py'),xbox_report_sha256=sha(SOURCE),cache_inputs=[level.provenance,shared.provenance],entry=h(ENTRY),owning_caller=h(CALLER),literal_function_instruction_count=len(source),executed_instruction_addresses=[h(q)for q in executed],function_calls_outside_measured_mp_path=[dict(site=h(p),target=t)for p,t in calls],
 declared_abi_global_bindings=[dict(slot=h(p),value=h(v))for p,v in bindings],resident_table_original_local_file_offset=table,resident_table_shared_entries=table_sources,resident_table_sha256=__import__('hashlib').sha256(raw_table).hexdigest(),original_services_or_callback_returns_supplied=[],ordered_stores=stores,metadata_store_count=len(ps),actual_changed_store_count=sum(q['before_hex']!=q['after_hex']for q in ps),changed_tags=changed,xbox_processed_field_comparison=comparison,all_41_ordered_destination_values_equal_to_xbox=True,stack_cleanup_bytes=4)
 (a.output/'report.json').write_text(json.dumps(r,indent=2,allow_nan=False)+'\n')
 print(json.dumps(dict(stores=len(ps),changes=r['actual_changed_store_count'],tags=len(changed),instructions=len(executed),all_41_values_equal=True,report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
