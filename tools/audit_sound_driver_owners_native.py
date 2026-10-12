#!/usr/bin/env python3
"""Audit unchanged empty owners, unavailable controls and real raw storage."""
import argparse,json,re
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
from audit_sound_driver_startup_native import prior_pins as driver_prior_pins
DRIVER=ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/final-manifest.json'
DRIVER_PIN='e75f3b1fabc796cf5ddf7608fbcc7786ce6504f97ff180e93994d322349ab228'
STORAGE=ROOT/'analysis/multiplayer/integration/native-sound-storage-20261009/frozen/manifest.json'
STORAGE_PIN='893146d869aed77ba12ff9711c8523b0f9811cf28e2eb6f6cb5c3b618fad7a27'
def prior_pins():
 assert sha(DRIVER)==DRIVER_PIN and sha(STORAGE)==STORAGE_PIN
 d=json.loads(DRIVER.read_text());pins=driver_prior_pins();pins.update(d['prior_pins']);pins[str(DRIVER.relative_to(ROOT))]=DRIVER_PIN
 for group in('owned_current_files','evidence_files'):
  for rel,q in d[group].items():pins[rel]=q['sha256']
 # Depend on the measured real-storage source, not freshness of every archive
 # or all protected sources in the parent's independent release checkpoint.
 m=json.loads(STORAGE.read_text());pins[str(STORAGE.relative_to(ROOT))]=STORAGE_PIN
 for rel in('include/halo2/native_sound_storage.hpp','src/native_sound_storage.cpp'):
  snapshot=str(STORAGE.parent.relative_to(ROOT)/'source'/rel);pin=m['artifact_pins'][snapshot]
  assert sha(ROOT/rel)==pin;pins[snapshot]=pin;pins[rel]=pin
 return pins
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();same=[]
 for left,right in(('measured','repeated'),('paired','paired-repeat'),('','export-repeat')):
  base=d/left;other=d/right;files=[q for q in base.rglob('*')if q.is_file()]if left else[d/'fixtures.tsv',d/'fixtures.json']
  for path in files:
   copy=other/path.relative_to(base);assert path.read_bytes()==copy.read_bytes(),(path,copy)
   same.append(dict(left=str(path.relative_to(d)),right=str(copy.relative_to(d)),sha256=sha(path)))
 pair=json.loads((d/'paired/report.json').read_text());r=json.loads((d/'measured/report.json').read_text());packet=json.loads((d/'paired/providers.json').read_text());images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0;addresses=set()
 for owner in pair['owners']:
  for tag,x in images.items():
   for line in(d/'paired/instructions'/f'{owner["old"]}-{tag}.asm').read_text().splitlines():
    at,raw,*_=line.split();assert x.read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw);rows+=1
    if tag=='old':addresses.add(int(at,16))
 assert rows==750 and len(pair['owners'])==6 and not r['instruction_patches']and not r['source_platform_returns']
 witnesses=[];coverage={};all_stores=0
 for image in r['images']:
  tag=image['image'];assert len(image['cases'])==570
  for line in(d/'measured'/image['listing']['file']).read_text().splitlines():
   at,raw,*_=line.split();assert images[tag].read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw)
  assert sha(d/'measured'/image['listing']['file'])==image['listing']['sha256']
  for case in image['cases']:
   assert not case['source_return_supplied'];all_stores+=len(case['stores']);coverage[case['outcome']]=coverage.get(case['outcome'],0)+1
   if case['operation']=='cleanup_witness':
    assert case['outcome']=='returned'and len(case['stores'])==3
    witnesses.append(dict(image=tag,name=case['name'],ordered_stores=case['stores']))
   if case['platform_request']:
    q=case['platform_request'];assert not q['original_return_supplied']and q['device_identity']=='00000000'
    assert q['descriptor24_hex']=='180000001020000000000000000000000000000000000000'
   if case['outcome']=='invalid_storage':assert len(case['invalid'])==1 and case['invalid'][0]['address']in('00000000','00000001','00000008')
  queue=int(packet['source_static_queue_identity'][tag],16);region=next(q for q in image['initial_regions']if int(q['address'],16)==queue)
  assert bytes.fromhex(region['raw_hex'])==images[tag].read(queue,16)
 assert all_stores==520 and coverage=={'returned':330,'invalid_storage':708,'platform_stop':102}
 for rel in('include/halo2/sound_driver_owners.hpp','src/sound_driver_owners.cpp','tests/sound_driver_owners_tests.cpp','include/halo2/native_sound_storage.hpp','src/native_sound_storage.cpp'):
  assert(ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 cpp=(ROOT/'src/sound_driver_owners.cpp').read_text();sites={int(q,16)for q in re.findall(r'write\(0x([0-9a-f]+)',cpp)}|{0x2b4c21,0x2b48e6}
 assert sites<=addresses,sorted(hex(q)for q in sites-addresses)
 assert cpp.index('buffer_operation(SoundDriverStaticBufferOperation::RolloffFactor')<cpp.index('buffer_operation(SoundDriverStaticBufferOperation::DopplerFactor')
 pins=prior_pins()
 for rel,pin in pins.items():assert sha(ROOT/rel)==pin,rel
 native={}
 for binary,label in(('sound_driver_owners_tests','normal'),('sound_driver_owners_tests_asan','sanitizer')):
  text=(d/'native'/f'{label}.stdout').read_text();assert '1138 cases, 514 exact ordered writes, 328 natural/control returns, 708 unallocated storage stops, 102 exact create-buffer requests'in text
  assert text==(d/'native-repeat'/f'{label}.stdout').read_text()
  assert not(d/'native'/f'{label}.stderr').read_bytes()and not(d/'native-repeat'/f'{label}.stderr').read_bytes()
  native[binary]=dict(binary_sha256=sha(d/'native'/binary),stdout_sha256=sha(d/'native'/f'{label}.stdout'),text=text.strip())
 audit=dict(schema='sound-driver-owners-native-audit-v1',audit_tool_sha256=sha(__file__),original_report_sha256=sha(d/'measured/report.json'),paired_report_sha256=sha(d/'paired/report.json'),source_cases=1140,native_cases=1138,represented_native_ordered_stores=514,whole_cleanup_witnesses=witnesses,complete_static_paired_bodies=6,paired_instruction_rows=rows,source_coverage=coverage,independent_byte_equal_files=same,prior_pins_verified=pins,native_results=native,limits=r['limits']+['Complete downstream nonempty virtual/buffer calls and packet memmove/count-debit are statically ported; the controls stop at real unallocated backing or actual constructor entry. No successful nonempty playback/cache-pin/platform phase is executed or inferred.','The test uses the independently frozen production NativeSoundStorage source with actual OS backing. Only directly observed dependency sources are asserted; unrelated shared archives are not rebuilt or claimed fresh.'])
 save(d/'audit.json',encoded(audit));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),independent_files=len(same),paired_rows=rows,prior_pins=len(pins))))
if __name__=='__main__':main()
