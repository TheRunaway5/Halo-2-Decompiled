#!/usr/bin/env python3
"""Declared archive controls for current guarded event versus stale RAM."""
import argparse,copy,hashlib,json,lzma,os,subprocess
from pathlib import Path
from native_created_body_contact_before_input_tu15 import NativeCreatedBodyContactBeforeInput,SCHEMA,BOUNDARY,FIELDS,sha
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);paths=[Path(__file__),ROOT/'tools/native_created_body_contact_before_input_tu15.py',ROOT/'tools/native_contact_process_before_input_tu15.py'];pins={str(p.relative_to(ROOT)):sha(p)for p in paths};r=dict(passed=False,owned_scratch=str(scratch),source_sha256=pins,retained_large_artifacts=[],actual_input=False,original_execution=False,native_admission=False)
 try:
  event=0x600e1000;values={'id':0x1234,**{n:0x12340000+i for i,n in enumerate(FIELDS)}};values['flag_word']=7
  fields=b''.join(values[n].to_bytes(4,'little')for n in FIELDS)
  files={'before-268435456.bin':b'A'*4096,'before-1611538432.bin':b'<'*65536,'event-id-2.bin':b'4\x12','event-fields-4-35.bin':fields}
  # Use the exact producer's checked schema, with a deliberately different
  # bank byte at the same numeric event identity.
  bank_address=0x600e0000;files.pop('before-1611538432.bin');files[f'before-{bank_address}.bin']=b'<'*65536
  metadata=dict(schema=SCHEMA,reference='English TU1.5 only',boundary=BOUNDARY,input=dict(owner=0x281710,listener=0x10000010,event=event,floating_point=dict(x87_control_word=0x27f,x87_status=0x20,mxcsr=0x1fa0),preserved_x87_depth=0),event_fields=values,event_data_fragments=[dict(address=event,size=2,before_file='event-id-2.bin'),dict(address=event+4,size=32,before_file='event-fields-4-35.bin')],event_padding_provided=False,event_backing_bank_is_current_event=False,bank_bytes=69632,regions=[dict(address=0x10000000,size=4096,before_file='before-268435456.bin'),dict(address=bank_address,size=65536,before_file=f'before-{bank_address}.bin')],guest_abi_fabricated=False,source_memory_mutated=False,physics_result_supplied=False)
  def archive(label,data,blobs=files,alter_hash=False):
   d=scratch/label;d.mkdir();all_files={**blobs,'capture-native-before.json':(json.dumps(data)+'\n').encode()}
   for n,v in all_files.items():(d/n).write_bytes(v)
   manifest={n:dict(bytes=len(v),sha256=hashlib.sha256(v).hexdigest())for n,v in all_files.items()};(d/'all-file-hashes.json.xz').write_bytes(lzma.compress(json.dumps(manifest).encode()))
   if alter_hash:(d/'event-id-2.bin').write_bytes(b'5\x12')
   p=d/'actual-native-before.tar.zst';subprocess.run(['tar','--zstd','-cf',str(p),'-C',str(d),*all_files],check=True,capture_output=True);return p
  p=archive('valid',metadata);data=NativeCreatedBodyContactBeforeInput(p,sha(p),scratch/'extracted');assert data.event_fields['flag_word']==7 and data.read(event,2)==b'4\x12'and data.read(event+4,32)==fields and data.word(event+0x20)==7 and data.bank_read(event+0x20,4)==b'<<<<'
  controls=['current-event-fragments-vs-stale-bank','flagNON1-not-assumed0','complete-file-authentication']
  for p,n in((event,4),(event+2,1),(event+1,4),(event,36),(event+0x40,4),(data.private_first,4)):
   try:data.read(p,n);raise AssertionError('Unprovided event read accepted')
   except ValueError:pass
  controls+=['padding-read-refused','cross-fragment-read-refused','unprovided-sidecar-refused-despite-bank-backing']
  try:NativeCreatedBodyContactBeforeInput(scratch/'valid/actual-native-before.tar.zst','0'*64,scratch/'wrongHash');raise AssertionError('Wrong archive hash accepted')
  except ValueError:pass
  assert not(scratch/'wrongHash').exists();controls.append('archive-hash-before-extraction')
  for label,change in (('paddingClaim',lambda m:m.update(event_padding_provided=True)),('rawBackingClaim',lambda m:m.update(event_backing_bank_is_current_event=True)),('GPfield',lambda m:m['input'].update(gp=[0]*8)),('fieldMismatch',lambda m:m['event_fields'].update(flag_word=8)),('wrongDepth',lambda m:m['input'].update(preserved_x87_depth=1))):
   m=copy.deepcopy(metadata);change(m);p=archive(label,m)
   try:NativeCreatedBodyContactBeforeInput(p,sha(p),scratch/(label+'-out'));raise AssertionError('Invalid created-body input accepted')
   except ValueError:pass
   controls.append(label)
  m=copy.deepcopy(metadata);m['event_fields']['flag_word']=1;blobs=dict(files);blobs['event-fields-4-35.bin']=fields[:-4]+(1).to_bytes(4,'little');p=archive('flag1',m,blobs)
  try:NativeCreatedBodyContactBeforeInput(p,sha(p),scratch/'flag1-out');raise AssertionError('Rejected flag1 accepted as current frontier')
  except ValueError:pass
  controls.append('flag1-excluded')
  p=archive('badFileHash',metadata,alter_hash=True)
  try:NativeCreatedBodyContactBeforeInput(p,sha(p),scratch/'badFileHash-out');raise AssertionError('Changed authenticated fragment accepted')
  except ValueError:pass
  controls.append('fragment-file-hash')
  assert all(sha(ROOT/n)==h for n,h in pins.items());r.update(passed=True,controls=controls,source_inputs_unchanged=True,valid_receipt=data.receipt)
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
