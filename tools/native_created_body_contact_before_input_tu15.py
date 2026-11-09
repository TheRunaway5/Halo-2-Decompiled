#!/usr/bin/env python3
"""Authenticate readonly281710 BEFORE banks and distinct provided event DATA.

The event's numeric identity may overlap stale captured backing RAM. read()
therefore uses authenticated semantic fragments inside the36-byte event and
rejects its unprovided padding. bank_read() deliberately exposes only backing
RAM for independent full-bank comparators; it is never a current-event reader.
"""
import bisect,json,lzma,os,re,subprocess
from pathlib import Path
from native_contact_process_before_input_tu15 import sha,require,u32
SCHEMA='native-tu15-created-body-contact-typed-before-v1'
BOUNDARY='Actual native281710 callback entry, before unchanged flag read/refusal. Owner-selected outer bank view and SAME guarded event DATA are distinct.'
FIELDS=('manager','first_payload','second_payload','current_body','point','properties','relative_velocity_bits','flag_word')
# This schema's sole production hook is the frozen moving binding: creation
# scratch=p+5000, genuine322D20 event=scratch+30, one owned32KiB Host sidecar.
# This guard is a source-owned resource contract, not a captured GP image.
MOVING_EVENT_OFFSET=0x5030
MOVING_ARENA_BYTES=0x8000
def canonical(name):
 require(not name.startswith('/')and'\\'not in name,'BEFORE archive member must be relative')
 value=name[2:]if name.startswith('./')else name
 require(value in ('','.')or re.fullmatch(r'(?:before-[0-9]+\.bin|capture-native-before\.json|event-id-2\.bin|event-fields-4-35\.bin)',value)is not None,'Unknown created-body BEFORE file')
 return ''if value in ('','.')else value
class NativeCreatedBodyContactBeforeInput:
 def __init__(self,archive,archive_sha256,destination):
  archive=Path(archive).resolve();destination=Path(destination).resolve()
  require(re.fullmatch('[0-9a-f]{64}',archive_sha256)is not None and sha(archive)==archive_sha256,'Created-body BEFORE archive hash mismatch')
  root=os.environ.get('HALO2_TEST_SCRATCH');require(root and destination.is_relative_to(Path(root).resolve())and destination!=Path(root).resolve(),'Created-body extraction requires an owned child directory');require(not destination.exists(),'Created-body extraction destination exists')
  sidecar=archive.with_name('all-file-hashes.json.xz');decompressor=lzma.LZMADecompressor();raw=decompressor.decompress(sidecar.read_bytes(),max_length=4*1024*1024);require(decompressor.eof and not decompressor.unused_data,'Invalid created-body BEFORE file manifest');manifest=json.loads(raw)
  require(type(manifest)is dict and 3<len(manifest)<=8195,'Invalid created-body BEFORE file count');proofs={}
  for name,proof in manifest.items():
   name=canonical(name);require(name and name not in proofs,'Duplicate created-body BEFORE proof');require(type(proof)is dict and set(proof)=={'bytes','sha256'},'Invalid created-body file proof');require(type(proof['bytes'])is int and 0<proof['bytes']<=192*1024*1024 and re.fullmatch('[0-9a-f]{64}',proof['sha256'])is not None,'Invalid created-body file size/hash');proofs[name]=proof
  require({'capture-native-before.json','event-id-2.bin','event-fields-4-35.bin'}<=set(proofs),'Created-body semantic event files absent')
  require(proofs['capture-native-before.json']['bytes']<=1024*1024 and sum(p['bytes']for p in proofs.values())<=193*1024*1024,'Created-body aggregate input exceeds bounds')
  names=subprocess.check_output(['tar','--zstd','-tf',str(archive)],text=True).splitlines();members=[canonical(n)for n in names];require(len(members)==len(set(members))and set(members)-{''}==set(proofs),'Created-body archive file set differs')
  listing=subprocess.check_output(['tar','--zstd','-tvf',str(archive)],text=True).splitlines();require(len(listing)==len(members),'Invalid created-body archive listing')
  for line,member in zip(listing,members):
   parts=line.split(maxsplit=5);require(len(parts)==6 and parts[2].isdigit()and line[0]==('-'if member else'd')and int(parts[2])==(proofs[member]['bytes']if member else 0),'Created-body archive contains special files or wrong size')
  destination.mkdir();subprocess.run(['tar','--zstd','--no-same-owner','--no-same-permissions','-xf',str(archive),'-C',str(destination)],check=True)
  require({str(p.relative_to(destination))for p in destination.rglob('*')if p.is_file()}==set(proofs),'Created-body extracted file set differs')
  for name,proof in proofs.items():
   p=destination/name;require(p.is_file()and not p.is_symlink()and p.stat().st_size==proof['bytes']and sha(p)==proof['sha256'],'Created-body file authentication differs: '+name)
  data=json.loads((destination/'capture-native-before.json').read_text())
  require(type(data)is dict and set(data)=={'schema','reference','boundary','input','event_fields','event_data_fragments','event_padding_provided','event_backing_bank_is_current_event','bank_bytes','regions','guest_abi_fabricated','source_memory_mutated','physics_result_supplied'},'Unknown created-body metadata field')
  require(data['schema']==SCHEMA and data['reference']=='English TU1.5 only'and data['boundary']==BOUNDARY,'Wrong created-body BEFORE schema/authority')
  require(all(data[k]is False for k in ('event_padding_provided','event_backing_bank_is_current_event','guest_abi_fabricated','source_memory_mutated','physics_result_supplied')),'Created-body BEFORE contains an unprovided/ABI/outcome claim')
  entry=data['input'];require(type(entry)is dict and set(entry)=={'owner','listener','event','floating_point','preserved_x87_depth'},'Unknown created-body input fields');require(entry['owner']==0x281710 and all(u32(entry[k])for k in ('owner','listener','event'))and entry['listener']and entry['event']and entry['event']+0x24<=2**32,'Invalid created-body ordinary inputs');require(type(entry['preserved_x87_depth'])is int and entry['preserved_x87_depth']==0,'Created-body BEFORE lacks live nonempty x87 DATA')
  private_first=entry['event']-MOVING_EVENT_OFFSET
  require(private_first>0 and private_first%16==0 and private_first+MOVING_ARENA_BYTES<=2**32,'Created-body event contradicts its source-owned moving workspace')
  fp=entry['floating_point'];require(type(fp)is dict and set(fp)=={'x87_control_word','x87_status','mxcsr'}and all(type(v)is int and 0<=v<=0xffff for v in fp.values()),'Invalid created-body scalar FP fields')
  fragments=data['event_data_fragments'];expected=[dict(address=entry['event'],size=2,before_file='event-id-2.bin'),dict(address=entry['event']+4,size=32,before_file='event-fields-4-35.bin')];require(fragments==expected,'Created-body event fragment extents differ')
  event_id=(destination/'event-id-2.bin').read_bytes();fields=(destination/'event-fields-4-35.bin').read_bytes();require(len(event_id)==2 and len(fields)==32,'Created-body event fragment length differs')
  values={'id':int.from_bytes(event_id,'little'),**{name:int.from_bytes(fields[n*4:n*4+4],'little')for n,name in enumerate(FIELDS)}};require(type(data['event_fields'])is dict and all(type(v)is int for v in data['event_fields'].values())and data['event_fields']==values,'Created-body semantic fields differ from authenticated fragments');require(values['flag_word']!=1,'Created-body capture requires actual NON1 branch; no literal0 is inferred')
  regions=data['regions'];require(type(regions)is list and 0<len(regions)<=8192,'Invalid created-body bank count');ordered=sorted(regions,key=lambda r:r['address']);last=0;files=set();total=0
  for row in ordered:
   require(type(row)is dict and set(row)=={'address','size','before_file'},'Unknown created-body bank fields');address,size,name=row['address'],row['size'],row['before_file'];require(u32(address)and type(size)is int and 0<size and address+size<=2**32 and address>=last,'Created-body bank overlaps/wraps');require(name==f'before-{address}.bin'and name not in files and name in proofs and proofs[name]['bytes']==size,'Created-body bank file differs');last=address+size;files.add(name);total+=size
  require(files|{'capture-native-before.json','event-id-2.bin','event-fields-4-35.bin'}==set(proofs),'Created-body input has extra files');require(type(data['bank_bytes'])is int and data['bank_bytes']==total<=192*1024*1024,'Created-body bank byte total differs');require(sha(archive)==archive_sha256,'Created-body archive changed during authentication')
  self.path=destination;self.data=data;self.input=entry;self.event_fields=values;self.floating_point=fp;self.regions=ordered;self.addresses=[r['address']for r in ordered];self.fragments=[(entry['event'],event_id),(entry['event']+4,fields)];self.private_first=private_first;self.private_end=private_first+MOVING_ARENA_BYTES
  self.receipt=dict(schema=SCHEMA,archive=str(archive),archive_sha256=archive_sha256,metadata_sha256=sha(destination/'capture-native-before.json'),file_manifest_sha256=sha(sidecar),verified_files=len(proofs),verified_bank_files=len(ordered),before_bytes=total,verified_current_event_bytes=34,event_padding_provided=False,current_event_read_from_backing=False,unprovided_moving_arena_guard=[self.private_first,self.private_end],captured_guest_ABI=False,captured_after_consumed=False,physics_result_supplied=False,source_memory_mutated=False)
 def bank_read(self,address,size):
  require(u32(address)and type(size)is int and 0<=size and address+size<=2**32,'Invalid created-body backing read');result=bytearray()
  while size:
   index=bisect.bisect_right(self.addresses,address)-1;require(index>=0,'Unmapped created-body backing read');row=self.regions[index];offset=address-row['address'];require(0<=offset<row['size'],'Unmapped created-body backing gap');count=min(size,row['size']-offset)
   with(self.path/row['before_file']).open('rb')as stream:stream.seek(offset);part=stream.read(count)
   require(len(part)==count,'Created-body bank truncated');result.extend(part);address+=count;size-=count
  return bytes(result)
 def read(self,address,size):
  require(u32(address)and type(size)is int and 0<=size and address+size<=2**32,'Invalid created-body current read')
  if size and address<self.private_end and self.private_first<address+size:
   for first,raw in self.fragments:
    if first<=address and address+size<=first+len(raw):return raw[address-first:address-first+size]
   raise ValueError('Created-body read touches unprovided moving DATA/padding or crosses its semantic lifetime')
  return self.bank_read(address,size)
 def word(self,address,width=4):
  require(width in (1,2,4,8),'Invalid created-body word width');return int.from_bytes(self.read(address,width),'little')
