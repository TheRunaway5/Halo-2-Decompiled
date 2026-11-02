#!/usr/bin/env python3
"""Identify actual1478 joint bodies via source2001/component/object DATA only."""
import argparse,bisect,hashlib,json,os,struct,subprocess,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH']);assert hashlib.file_digest(a.archive.open('rb'),'sha256').hexdigest()==a.archive_sha256
 capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());typed=capture['input'];assert typed['owner']==0x31d930 and not capture['guest_abi_fabricated'] and not capture['physics_result_supplied'];wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
 with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
  for member in tar:
   name=Path(member.name).name
   if name in wanted:
    row=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==row['size'];banks[row['address']]=raw
 assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
 def read(p,n):
  out=bytearray()
  while n:
   at=bisect.bisect_right(starts,p)-1;assert at>=0;lo=starts[at];off=p-lo;assert off<len(banks[lo]),hex(p);size=min(n,len(banks[lo])-off);out+=banks[lo][off:off+size];p+=size;n-=size
  return bytes(out)
 def word(p,n=4):return int.from_bytes(read(p,n),'little')
 def optional(p,n=4):
  try:return word(p,n)
  except AssertionError:return None
 components=word(word(0x52be5c)+0x44);headers=word(word(0x4ecbd0)+0x44);tags=word(0x4f0414);constraint=typed['constraint'];rows=[]
 for offset in [12,16]:
  body=word(constraint+offset);count=word(body+0x34);assert count<128;properties=[{'key':word(word(body+0x30)+i*8),'value':word(word(body+0x30)+i*8+4)}for i in range(count)];component=next((r['value']for r in properties if r['key']==0x2001),None);slot=next((r['value']for r in properties if r['key']==0x2002),None);row={'constraint_field':offset,'body':body,'motion':word(body+0x3c),'properties':properties,'component_datum':component,'body_slot':slot,'body_island':optional(body+0x44)}
  if component is not None and component!=0xffffffff:
   address=components+(component&0xffff)*160;datum=word(address+8);header=headers+(datum&0xffff)*12;object=word(header+8);tag=word(object);tagrow=tags+(tag&0xffff)*16;body_count=word(address+0x74);constraint_count=word(address+0x80);assert body_count<256 and constraint_count<256
   row.update(component_address=address,component_salt16=word(address,2),component_flags=word(address+4),object_datum=datum,object_header=header,object_header_hex=read(header,12).hex(),object_header_type=word(header+3,1),object_address=object,object_tag_datum=tag,object_type_word_AA=word(object+0xaa,2),object_flags=word(object+4),object_parent=word(object+0x14),object_tag_record_words=list(struct.unpack('<4I',read(tagrow,16))),definition=word(tagrow+8),component_body_count=body_count,component_authored_constraint_count=constraint_count,component_child_constraint_count=word(address+0x8c))
   row['component_bodies']=[{'slot':i,'body':word(word(address+0x70)+i*96+0x40)}for i in range(body_count)]
   row['authored_constraint_rows']=[{'index':i,'kind':word(word(address+0x7c)+i*12,2),'kind_index':word(word(address+0x7c)+i*12+2,2),'handle':word(word(address+0x7c)+i*12+4),'constraint':word(word(address+0x7c)+i*12+8)}for i in range(constraint_count)]
  rows.append(row)
 j={'passed':True,'scope':__doc__,'archive_sha256':a.archive_sha256,'input':typed,'constraint':constraint,'constraint_table':word(constraint),'constraint_fields_0_20':list(struct.unpack('<9I',read(constraint,36))),'bodies':rows,'captured_before_bytes':sum(map(len,banks.values())),'hypothesis_only':'Object type/name/asset interpretation requires authenticated tag metadata; no corpse/vehicle label inferred from constraint alone.','dataflow_sources':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest()for p in ['src/physics/native_tu15_joint_constraint_build.cpp','src/physics/native_tu15_contact_query.cpp','src/physics/native_tu15_contact_body_update.cpp','src/physics/native_tu15_constraint_body_slots.cpp']},'owned_scratch':str(owner),'temporary_allocated_bytes_before_cleanup':sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file()),'retained_large_artifacts':[],'raw_banks_written':False};(a.output/'report.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(j,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
