#!/usr/bin/env python3
"""Readonly semantic census of one genuine native322F70 typed BEFORE archive."""
import argparse,bisect,hashlib,json,os,subprocess,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
 capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());assert capture['schema']=='native-tu15-contact-point-typed-before-v1'and capture['input']['owner']==0x322f70 and not capture['guest_abi_fabricated']and not capture['physics_result_supplied'];wanted={r['before_file']:r for r in capture['regions']};banks={};process=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
 with tarfile.open(fileobj=process.stdout,mode='r|')as stream:
  for member in stream:
   name=Path(member.name).name
   if name not in wanted:continue
   region=wanted[name];raw=stream.extractfile(member).read();assert len(raw)==region['size'];banks[region['address']]=raw
 assert process.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
 def word(p,size=4):
  index=bisect.bisect_right(starts,p)-1;assert index>=0
  first=starts[index];assert first<=p and p+size<=first+len(banks[first]),(hex(p),size)
  return int.from_bytes(banks[first][p-first:p-first+size],'little')
 manager=capture['input']['manager'];collector=manager+0x10;world=word(manager+8);first=word(manager+0x1c);second=word(manager+0x20);owners=set()
 def listeners(obj,offset,slot):
  data,count=word(obj+offset),word(obj+offset+4);assert count<=128;rows=[]
  for n in range(count):
   receiver=word(data+4*n)
   if not receiver:rows.append(dict(index=n,receiver=0));continue
   table=word(receiver);owner=word(table+slot);owners.add(owner);rows.append(dict(index=n,receiver=receiver,table=table,owner=owner))
  return dict(object=obj,offset=offset,data=data,count=count,slot=slot,rows=rows)
 groups=dict(world_removed=listeners(world,0xb4,4),first_removed=listeners(first,0x60,4),second_removed=listeners(second,0x60,4),world_updated=listeners(world,0x60,0xc))
 receiver=word(collector+0x14);table=word(receiver);update_owner=word(table+0xc);detach_owner=word(table+4);owners|={update_owner,detach_owner};point=capture['input']['point'];point_map=word(collector+0x20);count=word(collector+0x94)
 actual=dict(manager=manager,manager_table=word(manager),collector=collector,collector_table=word(collector),world=world,world_flag=word(world+0x12c,1),first=first,second=second,
             collector_references=word(collector+6,2),point=point,map=point_map,map_count=word(collector+0x24),point_index=word(point_map+point,1),point_count=count,record_data=word(collector+0x90),record_capacity=word(collector+0x98),active_record_count=word(collector+0x38),collector_flags=word(collector+0x50),receiver=receiver,receiver_table=table,update_owner=update_owner,detach_owner=detach_owner,first_island=word(first+0x44),second_island=word(second+0x44))
 def property_value(body,property_id=0x2001):
  count=word(body+0x34);assert count<=256;rows=[];value=0
  if count:
   data=word(body+0x30)
   for n in range(count):
    key=word(data+n*8);entry=word(data+n*8+4);rows.append([key,entry])
    if key==property_id:value=entry;break
  return dict(body=body,count=count,rows=rows,value=value)
 component_owner=word(0x52be5c);component_data=word(component_owner+0x44);component_rows=[]
 for n,body in enumerate((first,second)):
  lookup=property_value(body)
  if n==0:
   record=component_data+(lookup['value']&0xffff)*160;flags=word(record+4,1)
   component_rows.append(dict(property=lookup,record=record,flags=flags,child_1d8880_selected=bool(flags&1),source_subject='first body notification'))
  else:component_rows.append(dict(property=lookup,component_record_not_read='Unselected branch in the actual first-body281D40 notification'))
 actual['point_listener_component_lookup']=dict(owner=component_owner,data=component_data,rows=component_rows)
 selected=component_rows[0];record=selected['record'];contact_count=word(record+0x8c);assert contact_count<=128
 subject_part=property_value(first,0x2002)['value'];other_part=property_value(second,0x2002)['value'];other_datum=component_rows[1]['property']['value'];contact_rows=[]
 contact_data=word(record+0x88)if contact_count else 0
 def signed_byte(value):return value-256 if value>=128 else value
 def signed_word(value):return value-(1<<32)if value&(1<<31)else value
 for n in range(contact_count):
  row=contact_data+n*72;point_id=word(row,2);datum=word(row+0x14);part=signed_byte(word(row+0x45,1));other=signed_byte(word(row+0x46,1));flags=word(row+0x44,1);effect=word(row+8)
  matches=point_id==point and datum==other_datum and part==signed_word(subject_part)and other==signed_word(other_part)
  contact_rows.append(dict(index=n,address=row,point=point_id,other_datum=datum,subject_part=part,other_part=other,flags=flags,effect=effect,matches=matches,release_effect_22e1b0=matches and effect!=0xffffffff,release_part_1e0a60=matches and bool(flags&0x20)))
 selected['gameplay_contact_rows']=dict(data=contact_data,count=contact_count,subject_part=signed_word(subject_part),other_part=signed_word(other_part),other_datum=other_datum,rows=contact_rows)
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');a.output.mkdir(parents=True,exist_ok=False);owner_rows=[]
 for owner in sorted(owners):
  body=complete_body(code,owner);assert len(body.code)<=2048;lines=[]
  for pc in sorted(body.code):
   instruction=code.at(pc);lines.append(f'{pc:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
  listing=''.join(lines);name=f'{owner:08x}.txt';(a.output/name).write_text(listing);owner_rows.append(dict(owner=owner,source_sites=len(body.code),listing=name,listing_sha256=hashlib.sha256(listing.encode()).hexdigest()))
 report=dict(passed=True,original_sha256=NEW_PIN,archive_sha256=a.archive_sha256,archive=str(a.archive),actual_typed_input=capture['input'],source_banks=len(banks),source_bank_bytes=sum(map(len,banks.values())),current=actual,listeners=groups,callback_owners=owner_rows,source_memory_mutated=False,source_code_or_expected_after_supplied=False,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},owned_scratch=os.environ['HALO2_TEST_SCRATCH'],temporary_allocated_bytes_before_cleanup=0,retained_large_artifacts=[],scope='Genuine typed before DATA census and authenticated owner source only. No captured source GP/SIMD ABI or fabricated physics outcome; no component/live closure claim.')
 (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(current=actual,listeners=groups,callback_owners=owner_rows)))
if __name__=='__main__':main()
