#!/usr/bin/env python3
"""Exercise strict typed-process BEFORE authentication and genuine1479 DATA."""
import argparse,copy,hashlib,io,json,lzma,os,struct,subprocess,tarfile
from pathlib import Path
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,SCHEMA,BOUNDARY,sha
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-304350-driving-vehicle-camera/required-native-contact-process-304350-before/actual-native-before.tar.zst';source=NativeContactProcessBeforeInput(archive,'5ef6aba031fc184d390fcc65ad6da055e16a279212232da4252a36e0b69d5c9c',scratch/'actual304',0x304350);assert source.receipt['verified_bank_files']==4033 and source.receipt['verified_files']==4034 and source.receipt['before_bytes']==89795481
  census={}
  for name in ('first','second'):
   payload=source.input[name];shape=source.word(payload);table=source.word(shape);census[name]=dict(payload=payload,shape=shape,table=table,bounds_owner=source.word(table+0x18),query_owner=source.word(table+0x30),resolver_owner=source.word(table+0x34),class_getter=source.word(table+0x14),virtual0_owner=source.word(table),motion=source.word(payload+8))
  container=source.word(census['second']['shape']+12);table=source.word(container);census['second_underlying_collection']=dict(shape=container,table=table,class_getter=source.word(table+0x14),bounds_owner=source.word(table+0x18),query_owner=source.word(table+0x30),resolver_owner=source.word(table+0x34))
  from oracle_smg_firing_1_1 import Xbe
  from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
  for row in census.values():
   code=image.read(row['class_getter'],6);row['class_getter_source_hex']=code.hex()
   if code[:1]==b'\xb8'and code[5:]==b'\xc3':row['source_constant_class']=int.from_bytes(code[1:5],'little')
  report['actual_input_receipt']=source.receipt;report['actual_typed_input']=source.input;report['actual_dispatch_before']=census
  meta=dict(schema=SCHEMA,reference='English TU1.5 only',boundary=BOUNDARY,guest_abi_fabricated=False,physics_result_supplied=False,source_memory_mutated=False,input=dict(owner=0x304350,agent=0x1000,first=0x1004,second=0x1008,configuration=0x100c,output=0x1010,preserved_x87_depth=0,floating_point=dict(x87_control_word=0x27f,x87_status=0x20,mxcsr=0x1fa0)),regions=[dict(address=0x1000,size=8,before_file='before-4096.bin'),dict(address=0x1008,size=16,before_file='before-4104.bin')],bank_bytes=24)
  def build(name,change=None,file_change=None,special=None,manifest_change=None):
   folder=scratch/name;folder.mkdir();data=copy.deepcopy(meta)
   if change:change(data)
   files={'capture-native-before.json':json.dumps(data).encode(),'before-4096.bin':bytes(range(8)),'before-4104.bin':bytes(range(8,24))}
   manifest={n:dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())for n,raw in files.items()}
   if manifest_change:manifest_change(manifest)
   if file_change:file_change(files)
   tar=folder/'input.tar'
   with tarfile.open(tar,'w')as output:
    for n,raw in files.items():info=tarfile.TarInfo(n);info.size=len(raw);output.addfile(info,io.BytesIO(raw))
    if special:
     info=tarfile.TarInfo(special[0]);info.type=special[1];info.linkname=special[2];output.addfile(info)
   target=folder/'actual-native-before.tar.zst';subprocess.run(['zstd','-q','-T1',str(tar),'-o',str(target)],check=True);(folder/'all-file-hashes.json.xz').write_bytes(lzma.compress(json.dumps(manifest).encode()));return target
  tiny=build('valid');ref=NativeContactProcessBeforeInput(tiny,sha(tiny),scratch/'valid-extract',0x304350);assert ref.read(0x1006,4)==bytes(range(6,10))and ref.word(0x1004,8)==int.from_bytes(bytes(range(4,12)),'little')
  negatives=[('abi_flag',lambda x:x.update(guest_abi_fabricated=True)),('mutation',lambda x:x.update(source_memory_mutated=True)),('result',lambda x:x.update(physics_result_supplied=True)),('schema',lambda x:x.update(schema='frame-before-v1')),('owner',lambda x:x['input'].update(owner=0x2e5320)),('gp_fields',lambda x:x['input'].update(gp=[0]*8)),('depth',lambda x:x['input'].update(preserved_x87_depth=1)),('fp',lambda x:x['input']['floating_point'].update(xmm=[0]*4)),('total',lambda x:x.update(bank_bytes=23)),('overlap',lambda x:x['regions'][1].update(address=0x1004)),('missing_bank',lambda x:x.update(regions=x['regions'][:1])),('region_fields',lambda x:x['regions'][0].update(after_file='after.bin'))]
  rejected=[]
  for name,change in negatives:
   path=build(name,change);bad=False
   try:NativeContactProcessBeforeInput(path,sha(path),scratch/(name+'-extract'),0x304350)
   except ValueError:bad=True
   assert bad,name;rejected.append(name)
  for name,options in [('file_hash',dict(file_change=lambda files:files.update({'before-4096.bin':b'12345678'}))),('extra_file',dict(file_change=lambda files:files.update({'unexpected.bin':b'0'}))),('symlink',dict(special=('before-8192.bin',tarfile.SYMTYPE,'/etc/passwd'))),('traversal',dict(special=('../escape',tarfile.DIRTYPE,''))),('hardlink',dict(special=('before-8192.bin',tarfile.LNKTYPE,'before-4096.bin')))]:
   path=build(name,**options);bad=False
   try:NativeContactProcessBeforeInput(path,sha(path),scratch/(name+'-extract'),0x304350)
   except ValueError:bad=True
   assert bad,name;rejected.append(name)
  for name,change,reason in [('aggregate_budget',lambda m:[m[n].update(bytes=192*1024*1024)for n in ('before-4096.bin','before-4104.bin')],'aggregate'),('metadata_budget',lambda m:m['capture-native-before.json'].update(bytes=1024*1024+1),'metadata'),('archive_extent',lambda m:m['before-4096.bin'].update(bytes=7),'header size')]:
   path=build(name,manifest_change=change);destination=scratch/(name+'-extract');bad=False
   try:NativeContactProcessBeforeInput(path,sha(path),destination,0x304350)
   except ValueError as e:bad=reason in str(e)
   assert bad and not destination.exists(),name+' must refuse BEFORE extraction';rejected.append(name)
  bad=False
  try:NativeContactProcessBeforeInput(tiny,'0'*64,scratch/'wrong-archive',0x304350)
  except ValueError:bad=True
  assert bad;rejected.append('archive_hash')
  bad=False
  try:ref.read(0xfff,1)
  except ValueError:bad=True
  assert bad;rejected.append('unmapped_read')
  report.update(passed=True,negative_controls_rejected=rejected,cross_bank_reads_verified=True,source_sha256={n:sha(ROOT/n)for n in ('tools/native_contact_process_before_input_tu15.py','tools/validate_native_contact_process_before_input_tu15.py')},old_before_input_sha256=sha(ROOT/'tools/build_native_tu15_constraint_before_reference.py'),old_before_input_modified=False)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
