#!/usr/bin/env python3
"""Audit whole paired driver bodies and repeated unchanged device-stop prefixes."""
import argparse,json,re
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
from audit_sound_map_activation_native import prior_pins as map_prior_pins
from oracle_sound_driver_startup_native import PRIOR,PRIOR_PIN,AUDIO_CAPACITY

MAP_MANIFEST=ROOT/'analysis/multiplayer/integration/sound-map-activation-native/final-manifest.json'
MAP_MANIFEST_PIN='8ae77db086ac162da8cde4fa17814e443d8043d460343842774bfd70d084eea0'

def prior_pins():
 assert sha(MAP_MANIFEST)==MAP_MANIFEST_PIN
 m=json.loads(MAP_MANIFEST.read_text());pins=map_prior_pins();pins.update(m['prior_pins'])
 pins[str(MAP_MANIFEST.relative_to(ROOT))]=MAP_MANIFEST_PIN
 for group in('owned_current_files','evidence_files'):
  for rel,q in m[group].items():pins[rel]=q['sha256']
 return pins

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();comparisons=[]
 for left,right in(('measured','repeated'),('paired','paired-repeat'),('','export-repeat')):
  base=d/left;other=d/right;files=[q for q in base.rglob('*')if q.is_file()]if left else[d/'fixtures.tsv',d/'fixtures.json']
  for q in files:
   rel=q.relative_to(base);copy=other/rel;assert q.read_bytes()==copy.read_bytes(),(q,copy)
   comparisons.append(dict(left=str(q.relative_to(d)),right=str(copy.relative_to(d)),sha256=sha(q)))
 r=json.loads((d/'measured/report.json').read_text());pair=json.loads((d/'paired/report.json').read_text());packet=json.loads((d/'paired/providers.json').read_text())
 images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0;addresses=set()
 for q in pair['owners']:
  for tag,x in images.items():
   for line in(d/'paired/instructions'/f'{q["old"]}-{tag}.asm').read_text().splitlines():
    at,raw,*_=line.split();assert x.read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw);rows+=1
    if tag=='old':addresses.add(int(at,16))
 assert len(pair['owners'])==12 and rows==2512
 for q in packet['constants'].values():
  for tag,x in images.items():
   if 'raw4_hex'in q:assert x.read(int(q['old_va'if tag=='old'else'u5_va'],16),4).hex()==q['raw4_hex']
   else:
    pointer=int.from_bytes(x.read(int(q['old_pointer_va'if tag=='old'else'u5_pointer_va'],16),4),'little')
    assert pointer==int(q['old_value_va'if tag=='old'else'u5_value_va'],16)
    assert x.read(pointer,12).hex()==q['raw12_hex']
 assert packet['source_selected_plugin_identity']['native_retained_mutable_plugin_owner_required']
 assert not packet['runtime_environment_packet']['copied_into_constants']
 for tag in('old','u5'):
  # Recover the paired MOV instruction by the retained row index, rather
  # than relying on a guessed relocation delta in the u5 image.
  old=(d/'paired/instructions/002205c0-old.asm').read_text().splitlines()
  index=next(i for i,line in enumerate(old)if line.startswith('002209f9 '))
  line=(d/'paired/instructions'/f'002205c0-{tag}.asm').read_text().splitlines()[index]
  at,raw,*_=line.split();raw=bytes.fromhex(raw)
  assert raw[0]==0xb9 and int.from_bytes(raw[1:],'little')==int(packet['source_selected_plugin_identity'][tag],16)
  assert images[tag].read(int(at,16),len(raw))==raw
 assert sha(PRIOR)==PRIOR_PIN and r['prior_report_sha256']==PRIOR_PIN
 prior=json.loads(PRIOR.read_text())['stopped_original_caller'];initial=bytes.fromhex(prior['arena_hex'])
 assert len(initial)==AUDIO_CAPACITY and not r['source_instruction_patches']and not r['source_platform_returns']
 assert len(r['cases'])==8
 for case in r['cases']:
  assert bytes.fromhex(case['initial_arena_hex'])==initial
  assert len(bytes.fromhex(case['output_arena_hex']))==AUDIO_CAPACITY
  assert not case['whole_constructor_returned']and len(case['stores'])==2746
  tag=case['image'];boundary=case['native_platform_boundary']
  assert not boundary['source_platform_return_supplied']and boundary['source_device_initialized_byte']==boundary['source_device_identity']==0
  driver=int(case['after']['driver_address'],16)
  assert boundary['actual_stack_arguments']==['00000000',f'{driver+0x2ab0:08x}','00000000']
  assert case['policy16_hex']==images[tag].read(int(case['policy_address'],16),16).hex()
  assert case['before']['arena_base']==prior['globals']['arena_base']and case['before']['arena_crc']==prior['globals']['arena_crc']
  residue=int(case['name'].rsplit('_',1)[1]);assert int(case['before']['arena_used'],16)==int(prior['globals']['arena_used'],16)+residue
  assert len(case['controls'])==(0 if residue==0 else 1)
  for line in(d/'measured'/case['listing']['file']).read_text().splitlines():
   at,raw,*_=line.split();assert images[tag].read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw)
  assert sha(d/'measured'/case['listing']['file'])==case['listing']['sha256']
  # Native scalar CRC output is represented. These source-owned cold cache
  # writes remain separately preserved, not discarded or promoted as native.
  assert len(case['auxiliary_crc_cache_stores'])==257
 for residue in range(4):
  left,right=r['cases'][residue],r['cases'][residue+4]
  assert left['output_arena_hex']==right['output_arena_hex']
  assert [q['canonical']for q in left['stores']]==[q['canonical']for q in right['stores']]
 for rel in('include/halo2/sound_driver_startup.hpp','src/sound_driver_startup.cpp','tests/sound_driver_startup_tests.cpp','include/halo2/sound_map_activation.hpp'):
  assert(ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 cpp=(ROOT/'src/sound_driver_startup.cpp').read_text()
 sites={int(x,16)for x in re.findall(r'(?:write|global)\(0x([0-9a-f]+)',cpp)}
 sites|={0x2205fd,0x12d7e4,0x1656ed,0x220c6d,0x220c82}
 sites|={0x2208d1+10*i for i in range(3)}|{0x220b8f+8*i for i in range(4)}
 assert sites<=addresses,sorted(hex(q)for q in sites-addresses)
 pins=prior_pins()
 for rel,pin in pins.items():assert sha(ROOT/rel)==pin,rel
 native={}
 for binary,log in(('sound_driver_startup_tests','normal'),('sound_driver_startup_tests_asan','sanitizer')):
  text=(d/'native'/f'{log}.stdout').read_text()
  assert '8 cases, 21968 exact ordered stores'in text and 'no saved-setting/caps/DSP/stream/plugin/admission return'in text
  assert text==(d/'native-repeat'/f'{log}.stdout').read_text()
  assert not(d/'native'/f'{log}.stderr').read_bytes()and not(d/'native-repeat'/f'{log}.stderr').read_bytes()
  native[binary]=dict(binary_sha256=sha(d/'native'/binary),stdout_sha256=sha(d/'native'/f'{log}.stdout'),text=text.strip())
 report=dict(schema='sound-driver-startup-native-audit-v1',audit_tool_sha256=sha(__file__),original_report_sha256=sha(d/'measured/report.json'),paired_report_sha256=sha(d/'paired/report.json'),providers_sha256=sha(d/'paired/providers.json'),original_prefix_cases=len(r['cases']),represented_prefix_stores=sum(len(q['stores'])for q in r['cases']),cold_crc_cache_stores_separately_retained=sum(len(q['auxiliary_crc_cache_stores'])for q in r['cases']),full_source_arena_bytes=AUDIO_CAPACITY,complete_static_paired_bodies=len(pair['owners']),paired_instruction_rows=rows,independent_byte_equal_files=comparisons,prior_pins_verified=pins,native_results=native,
  limits=r['bounds']+['Native whole caller, cleanup caller and storage/CRC scalar are statically mapped. Only the actual original allocation prefix is differentially executed; source CPU helper callbacks and all subsequent platform operations remain required and unproved as live composition.','Cold source CRC table initialization has a separately retained 257-store trace per case; the native scalar computation does not reproduce that guest cache.','Fresh standalone Linux mmap test snapshots/binaries do not assert freshness of unrelated shared archives or physical audio hardware.'])
 save(d/'audit.json',encoded(report));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),independent_files=len(comparisons),paired_rows=rows,prior_pins=len(pins))))
if __name__=='__main__':main()
