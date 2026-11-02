#!/usr/bin/env python3
"""Authenticate genuine typed native3205B0 DATA without inventing a call frame."""
import json,lzma,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,build_constraint_reference,sha,save
class ContactTimeBeforeInput(BeforeInput):
 def __init__(self,archive,archive_sha,destination):
  archive=archive.resolve();assert sha(archive)==archive_sha;self.path=destination;destination.mkdir()
  names=subprocess.check_output(['tar','--zstd','-tf',str(archive)],text=True).splitlines();assert all(not Path(n).is_absolute()and'..'not in Path(n).parts for n in names)
  types=subprocess.check_output(['tar','--zstd','-tvf',str(archive)],text=True).splitlines();assert all(n[0]in'-d'for n in types);subprocess.run(['tar','--zstd','-xf',str(archive),'-C',str(destination)],check=True)
  self.data=json.loads((destination/'capture-native-before.json').read_text());assert self.data['schema']=='native-tu15-contact-time-typed-before-v1'and self.data['reference']=='English TU1.5 only';assert not self.data['guest_abi_fabricated']and not self.data['physics_result_supplied']and not self.data['captured_after_consumed']and not self.data['source_memory_mutated'];assert self.data['input']['preserved_x87_depth']==0
  self.regions=sorted(self.data['regions'],key=lambda r:r['address']);self.addresses=[r['address']for r in self.regions];manifest=json.loads(lzma.decompress(archive.with_name('all-file-hashes.json.xz').read_bytes()))
  for row in self.regions:
   name=row['before_file'];assert not Path(name).is_absolute()and'..'not in Path(name).parts;proof=manifest.get('./'+name,manifest.get(name));assert proof is not None and(destination/name).stat().st_size==row['size']and sha(destination/name)==proof['sha256']
  self.receipt=dict(archive=str(archive),archive_sha256=archive_sha,metadata_sha256=sha(destination/'capture-native-before.json'),verified_bank_files=len(self.regions),before_bytes=sum(r['size']for r in self.regions),captured_guest_ABI=False,boundary=self.data['boundary'])
