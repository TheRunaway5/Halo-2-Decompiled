#!/usr/bin/env python3
"""Execute original Vista combat postprocessing on all supplied MP scenarios.

This separately extends the immutable Lockout oracle. Original game inputs and
all previous oracle tools/reports remain untouched. No source or service return
is supplied, and only private metadata copies are changed by source execution.
"""
import argparse,bisect,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_MXCSR
from inspect_pc_combat_tag_sample import PcCache,SOURCE,sha
from pc_postprocess_inspect import PE,VISTA,ROOT,OUT
from pc_postprocess_vista_oracle import BASE,TABLE,STACK,STOP,ENTRY,CALLER,PIN,h

def scenario_header(path):
 """Read authored scenario datum/type without loading whole non-MP caches."""
 with path.open('rb')as f:
  head=f.read(2048);assert head[:4]==b'daeh' and struct.unpack_from('<I',head,4)[0]==8
  index=struct.unpack_from('<I',head,16)[0];magic=struct.unpack_from('<I',head,32)[0]-index
  f.seek(index);ih=f.read(32);classes,_,table,datum=struct.unpack_from('<4I',ih)
  if datum==0xffffffff:return dict(path=str(path.resolve()),scenario_datum='ffffffff',scenario_type=None)
  table_file=table-(classes-index-32);f.seek(table_file+(datum&0xffff)*16);entry=f.read(16)
  group,actual,address,size=struct.unpack('<4sIII',entry);assert group==b'rncs'and actual==datum and address and size>=18
  offset=address-magic;f.seek(offset+16);type_=struct.unpack('<h',f.read(2))[0]
  return dict(path=str(path.resolve()),scenario_datum=h(datum),scenario_type=type_,scenario_file_offset=offset,scenario_entry_hex=entry.hex())

def execute_level(level,shared,output,pe=None):
 """Return ordered source stores with actual input-owner offsets and bytes."""
 output=Path(output);output.mkdir(parents=True,exist_ok=True)
 assert sha(VISTA)==PIN
 pe=pe or PE(VISTA);source,calls=pe.trace(ENTRY);assert len(source)==573
 for q in(ENTRY,CALLER):(output/f'{q:08x}.asm').write_text(pe.listing(q))
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
 scenario_id=level.u(level.index+12)
 scenario=level.tags[scenario_id&0xffff]
 assert int(scenario['datum'],16)==scenario_id and scenario['group']=='scnr' and scenario['offset'] is not None
 assert struct.unpack_from('<h',level.data,scenario['offset']+0x10)[0]==1
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
   d=output/'tag-buffers';d.mkdir(exist_ok=True);stem=t['datum']+'-'+t['group'].replace('!','_')
   (d/(stem+'-raw.bin')).write_bytes(raw);(d/(stem+'-processed.bin')).write_bytes(processed)
   changed.append(dict(tag=t,owner=str(c.path),raw_sha256=__import__('hashlib').sha256(raw).hexdigest(),processed_sha256=__import__('hashlib').sha256(processed).hexdigest()))
 r=dict(scope='Complete unchanged Vista function at 0046EFDB, actual MP path over this supplied level and shared metadata. The source caller, loader, preceding group fixups, campaign branch, and gameplay are not executed.',
 level_name=level.path.stem,scenario_index_header_datum=h(scenario_id),scenario_tag=scenario,executable_path=str(VISTA),executable_sha256=PIN,tool_sha256=sha(Path(__file__)),helper_sha256=sha(ROOT/'tools/pc_postprocess_inspect.py'),reader_sha256=sha(ROOT/'tools/inspect_pc_combat_tag_sample.py'),xbox_report_sha256=sha(SOURCE),cache_inputs=[level.provenance,shared.provenance],entry=h(ENTRY),owning_caller=h(CALLER),literal_function_instruction_count=len(source),executed_instruction_addresses=[h(q)for q in executed],function_calls_outside_measured_mp_path=[dict(site=h(p),target=t)for p,t in calls],
 declared_abi_global_bindings=[dict(slot=h(p),value=h(v))for p,v in bindings],resident_table_original_local_file_offset=table,resident_table_shared_entries=table_sources,resident_table_sha256=__import__('hashlib').sha256(raw_table).hexdigest(),original_services_or_callback_returns_supplied=[],ordered_stores=stores,metadata_store_count=len(ps),actual_changed_store_count=sum(q['before_hex']!=q['after_hex']for q in ps),changed_tags=changed,xbox_processed_field_comparison=comparison,all_41_ordered_destination_values_equal_to_xbox=True,stack_cleanup_bytes=4)

 (output/'report.json').write_text(json.dumps(r,indent=2,allow_nan=False)+'\n')
 return r

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--maps',type=Path,default=ROOT/'Halo 2 Vista installer/maps');a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 inventory=[scenario_header(p)for p in sorted(a.maps.glob('*.map'))]
 selected=[r for r in inventory if r['scenario_type']==1];assert len(selected)==23
 shared=PcCache(a.maps/'shared.map');pe=PE(VISTA);rows=[]
 for header in selected:
  path=Path(header['path']);level=PcCache(path);report=execute_level(level,shared,a.output/path.stem,pe)
  assert report['scenario_index_header_datum']==header['scenario_datum']
  rows.append(dict(map=path.stem,scenario_header=header,report_relative_path=path.stem+'/report.json',report_sha256=sha(a.output/path.stem/'report.json'),metadata_store_count=report['metadata_store_count'],actual_changed_store_count=report['actual_changed_store_count'],changed_tags=len(report['changed_tags']),executed_instructions=len(report['executed_instruction_addresses']),original_services_or_callback_returns_supplied=[]))
  print(json.dumps(rows[-1]),flush=True)
 r=dict(scope='Complete unchanged Vista MP combat path over every supplied multiplayer scenario selected by actual tag-index-header scenario ID and signed scenario type 1. Loader/preceding fixups/live gameplay remain outside this fixture.',tool_sha256=sha(Path(__file__)),original_lockout_tool_sha256=sha(ROOT/'tools/pc_postprocess_vista_oracle.py'),helper_sha256=sha(ROOT/'tools/pc_postprocess_inspect.py'),reader_sha256=sha(ROOT/'tools/inspect_pc_combat_tag_sample.py'),executable_path=str(VISTA),executable_sha256=PIN,source_field_contract_sha256=sha(SOURCE),inventory=inventory,multiplayer_map_count=len(rows),maps=rows,metadata_store_count=sum(r['metadata_store_count']for r in rows),actual_changed_store_count=sum(r['actual_changed_store_count']for r in rows),executed_original_instructions=sum(r['executed_instructions']for r in rows),all_maps_41_destination_values_equal_to_xbox=True,original_services_or_callback_returns_supplied=[])
 (a.output/'report.json').write_text(json.dumps(r,indent=2,allow_nan=False)+'\n')
 print(json.dumps(dict(maps=len(rows),stores=r['metadata_store_count'],changed=r['actual_changed_store_count'],instructions=r['executed_original_instructions'],report_sha256=sha(a.output/'report.json'))),flush=True)
if __name__=='__main__':main()
