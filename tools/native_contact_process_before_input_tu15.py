#!/usr/bin/env python3
"""Strict authentication of typed2E5320/304350 BEFORE inputs.

This schema is independent from the older component/frame BeforeInput. It
contains ordinary semantic arguments and scalar FP policy, never a GP/SIMD
register file, an expected result or a captured after-image. Extracted banks
belong to the caller's run_test.py scratch owner.
"""
import bisect,hashlib,json,lzma,os,re,subprocess
from pathlib import Path

SCHEMA='native-tu15-contact-process-typed-before-v1'
BOUNDARY='Actual native World typed contact-process owner BEFORE unchanged remaining-owner dispatch; no GP/SIMD ABI'
OWNERS=frozenset((0x2e5320,0x304350))
def sha(path):
 with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def require(value,message):
 if not value:raise ValueError(message)
def u32(value):return type(value)is int and 0<=value<=0xffffffff

def _canonical_member(name):
 require(not name.startswith('/')and '\\'not in name,'Archive member must be relative')
 value=name[2:]if name.startswith('./')else name
 require(value in ('','.')or re.fullmatch(r'(?:before-[0-9]+\.bin|capture-native-before\.json)',value)is not None,'Archive contains undeclared path')
 return ''if value in ('','.')else value

class NativeContactProcessBeforeInput:
 def __init__(self,archive,archive_sha256,destination,expected_owner):
  archive=Path(archive).resolve();destination=Path(destination).resolve();require(expected_owner in OWNERS,'Expected owner must name the typed process schema')
  require(re.fullmatch('[0-9a-f]{64}',archive_sha256)is not None and sha(archive)==archive_sha256,'Required BEFORE archive hash mismatch')
  owner=os.environ.get('HALO2_TEST_SCRATCH');require(owner and destination.is_relative_to(Path(owner).resolve())and destination!=Path(owner).resolve(),'BEFORE extraction requires a child of the active scratch owner')
  require(not destination.exists(),'BEFORE destination already exists')
  sidecar=archive.with_name('all-file-hashes.json.xz');packed=sidecar.read_bytes();decompressor=lzma.LZMADecompressor();raw=decompressor.decompress(packed,max_length=4*1024*1024);require(decompressor.eof and not decompressor.unused_data,'Invalid or oversized BEFORE file manifest');manifest=json.loads(raw)
  require(type(manifest)is dict and 1<len(manifest)<=8193,'Invalid BEFORE file manifest extent')
  canonical={}
  for name,proof in manifest.items():
   n=_canonical_member(name);require(n and n not in canonical,'Duplicate BEFORE manifest member');require(type(proof)is dict and set(proof)=={'bytes','sha256'},'Invalid BEFORE file proof');require(type(proof['bytes'])is int and 0<proof['bytes']<=192*1024*1024 and re.fullmatch('[0-9a-f]{64}',proof['sha256'])is not None,'Invalid BEFORE file size/hash');canonical[n]=proof
  require('capture-native-before.json'in canonical and canonical['capture-native-before.json']['bytes']<=1024*1024,'BEFORE metadata exceeds bounded payload')
  require(sum(p['bytes']for n,p in canonical.items()if n!='capture-native-before.json')<=192*1024*1024,'BEFORE aggregate payload exceeds bounded budget')
  names=subprocess.check_output(['tar','--zstd','-tf',str(archive)],text=True).splitlines();members=[_canonical_member(n)for n in names];require(len(members)==len(set(members)),'Duplicate archive member');require(set(members)-{''}==set(canonical),'Archive file set differs from authenticated BEFORE manifest')
  listing=subprocess.check_output(['tar','--zstd','-tvf',str(archive)],text=True).splitlines();require(len(listing)==len(names)and all(line and line[0]==('d'if not member else'-')for line,member in zip(listing,members)),'BEFORE archive contains links or special files')
  for line,member in zip(listing,members):
   parts=line.split(maxsplit=5);require(len(parts)==6 and parts[2].isdigit(),'Invalid BEFORE archive header')
   require(int(parts[2])==(canonical[member]['bytes']if member else 0),'BEFORE archive header size differs from bounded manifest')
  destination.mkdir();subprocess.run(['tar','--zstd','--no-same-owner','--no-same-permissions','-xf',str(archive),'-C',str(destination)],check=True)
  actual={str(p.relative_to(destination))for p in destination.rglob('*')if p.is_file()};require(actual==set(canonical),'Extracted BEFORE file set differs')
  for name,proof in canonical.items():
   p=destination/name;require(p.is_file()and not p.is_symlink()and p.stat().st_size==proof['bytes']and sha(p)==proof['sha256'],'BEFORE member size/hash mismatch: '+name)
  data=json.loads((destination/'capture-native-before.json').read_text());require(type(data)is dict and set(data)=={'schema','reference','boundary','guest_abi_fabricated','physics_result_supplied','source_memory_mutated','input','regions','bank_bytes'},'Unknown typed-process metadata fields')
  require(data['schema']==SCHEMA and data['reference']=='English TU1.5 only'and data['boundary']==BOUNDARY,'Wrong typed-process BEFORE authority/schema')
  require(all(data[k]is False for k in ('guest_abi_fabricated','physics_result_supplied','source_memory_mutated')),'Typed BEFORE may not contain fabricated ABI, outcomes or Source mutation')
  entry=data['input'];require(type(entry)is dict and set(entry)=={'owner','agent','first','second','configuration','output','floating_point','preserved_x87_depth'},'Unknown typed process formal fields')
  require(entry['owner']==expected_owner and all(u32(entry[k])for k in ('owner','agent','first','second','configuration','output')),'Wrong typed-process owner/formals');require(type(entry['preserved_x87_depth'])is int and entry['preserved_x87_depth']==0,'Typed process BEFORE requires depth zero; nonempty x87 values were not captured')
  fp=entry['floating_point'];require(type(fp)is dict and set(fp)=={'x87_control_word','x87_status','mxcsr'}and all(type(v)is int and 0<=v<=0xffff for v in fp.values()),'Invalid actual scalar FP policy')
  regions=data['regions'];require(type(regions)is list and 0<len(regions)<=8192,'Invalid BEFORE bank extent');ordered=sorted(regions,key=lambda r:r['address']);last=0;files=set();total=0
  for row in ordered:
   require(type(row)is dict and set(row)=={'address','size','before_file'},'Invalid BEFORE bank fields');address,size,name=row['address'],row['size'],row['before_file'];require(u32(address)and type(size)is int and 0<size and address+size<=0x100000000 and address>=last,'Overlapping or invalid BEFORE bank');require(name==f'before-{address}.bin'and name not in files and name in canonical and canonical[name]['bytes']==size,'Bank identity differs from authenticated file');last=address+size;files.add(name);total+=size
  require(files|{'capture-native-before.json'}==set(canonical),'Manifest includes non-BEFORE files');require(type(data['bank_bytes'])is int and data['bank_bytes']==total<=192*1024*1024,'BEFORE total byte count differs')
  require(sha(archive)==archive_sha256,'BEFORE archive changed during authentication')
  self.path=destination;self.data=data;self.input=entry;self.floating_point=fp;self.regions=ordered;self.addresses=[r['address']for r in ordered]
  self.receipt=dict(schema=SCHEMA,owner=expected_owner,archive=str(archive),archive_sha256=archive_sha256,metadata_sha256=sha(destination/'capture-native-before.json'),file_manifest_sha256=sha(sidecar),verified_files=len(canonical),verified_bank_files=len(ordered),before_bytes=total,all_manifest_files_verified=True,captured_guest_ABI=False,captured_after_consumed=False,physics_result_supplied=False,source_memory_mutated=False,preserved_x87_depth=entry['preserved_x87_depth'])
 def read(self,address,size):
  require(u32(address)and type(size)is int and 0<=size and address+size<=0x100000000,'Invalid BEFORE read extent');result=bytearray()
  while size:
   index=bisect.bisect_right(self.addresses,address)-1;require(index>=0,'Unmapped BEFORE read');row=self.regions[index];offset=address-row['address'];require(0<=offset<row['size'],'Unmapped BEFORE read gap');count=min(size,row['size']-offset)
   with(self.path/row['before_file']).open('rb')as stream:stream.seek(offset);part=stream.read(count)
   require(len(part)==count,'BEFORE bank truncated after authentication');result.extend(part);address+=count;size-=count
  return bytes(result)
 def word(self,address,width=4):
  require(width in (1,2,4,8),'Unsupported BEFORE word extent');return int.from_bytes(self.read(address,width),'little')
