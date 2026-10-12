#!/usr/bin/env python3
"""Audit complete cache owner repeats, raw platform evidence and native result."""
import argparse,json,re
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
from oracle_smg_audio_rng_startup_1_1 import FROZEN

STARTUP=ROOT/'analysis/multiplayer/integration/sound-startup-native/final-manifest.json'
STARTUP_PIN='cbbac6e29511d5a27d4cc5908095bddb4c31399c1d3bbddd97c2fe9b54ea8d6a'

def prior_pins():
 assert sha(STARTUP)==STARTUP_PIN
 m=json.loads(STARTUP.read_text());pins=dict(FROZEN)
 pins[str(STARTUP.relative_to(ROOT))]=STARTUP_PIN
 for group in('owned_current_files','evidence_files'):
  for path,row in m[group].items():pins[path]=row['sha256']
 return pins

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory
 comparisons=[]
 for left,right in(('measured','repeated'),('paired','paired-repeat'),('','export-repeat')):
  base=d/left;other=d/right
  paths=[q for q in base.rglob('*')if q.is_file()and 'raw-host-providers'not in q.parts]if left else[d/'fixtures.tsv',d/'fixtures.json']
  for q in paths:
   rel=q.relative_to(base);copy=other/rel;assert copy.is_file()and q.read_bytes()==copy.read_bytes(),(q,copy)
   comparisons.append(dict(left=str(q.relative_to(d)),right=str(copy.relative_to(d)),sha256=sha(q)))
 r=json.loads((d/'measured/report.json').read_text());pair=json.loads((d/'paired/report.json').read_text());providers=json.loads((d/'paired/providers.json').read_text())
 images={'old':Xbe(OLD),'u5':Xbe(NEW)};addresses=set();rows=0
 for q in pair['owners']:
  for tag in images:
   listing=d/'paired/instructions'/f'{q["old"]}-{tag}.asm'
   for line in listing.read_text().splitlines():
    addr,raw,*_=line.split();assert images[tag].read(int(addr,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw)
    rows+=1
    if tag=='old':addresses.add(int(addr,16))
 for q in(d/'measured/instructions').glob('*.asm'):
  for line in q.read_text().splitlines():
   addr,raw,*_=line.split();assert images['old'].read(int(addr,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw)
 callback_expected={'delete_callback':('0021a736','0021f036'),'busy_callback':('0021a731','0021f031')}
 # The immutable push immediate itself binds the opaque callback identity;
 # paired complete callback bodies are retained as independent authority.
 for name,(old_site,u5_site)in callback_expected.items():
  q=providers['callbacks'][name]
  for tag,site,key in(('old',old_site,'old_identity'),('u5',u5_site,'u5_identity')):
   raw=images[tag].read(int(site,16),5);assert raw[0]==0x68 and int.from_bytes(raw[1:],'little')==int(q[key],16)
 for q in providers['names'].values():
  for tag in images:assert images[tag].read(int(q['old_va'if tag=='old'else'u5_va'],16),32).split(b'\0',1)[0].decode()==q['ascii']
 for rel in('include/halo2/sound_cache_startup.hpp','src/sound_cache_startup.cpp','tests/sound_cache_startup_tests.cpp'):
  assert (ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 cpp=(ROOT/'src/sound_cache_startup.cpp').read_text();sites={int(q,16)for q in re.findall(r'(?:write|clear|global)\(0x([0-9a-f]+)',cpp)}
 assert sites<=addresses,sorted(hex(q)for q in sites-addresses)
 # Eight explicit sequential13E8E0 sentinel stores are represented by this
 # fixed source loop, not a synthesized new original instruction address.
 assert all(0x13e97e+3*i in addresses for i in range(8))
 raw_files=[]
 for base in('measured','repeated'):
  for case in r['cases']:
   q=d/base/case['raw_host_provider_evidence_file'];calls=json.loads(q.read_text())
   vm=[x for x in calls if x.get('entry')=='0033785c'];assert [x['requested_size']for x in vm]==[0x208c,0x30fc]
   for x in vm:
    operation=x['raw_provider_operation'];assert operation['actual_errno']==0 and int(operation['actual_host_address'],16)>0
    assert x['provider_writes_only_raw_base_and_access']and not x['original_cache_constructor_result_supplied']
   # Import target identity is a retained compatibility kernel thunk; no
   # hardcoded target is required to identify the actual operation.
   protected=[x for x in calls if x.get('raw_provider_operation',{}).get('api')=='actual libc mprotect PROT_READ|PROT_WRITE']
   assert len(protected)==sum(x['service']=='raw_protect'for x in case['services'])
   for x in protected:assert x['raw_provider_operation']['actual_result']==0 and x['raw_provider_operation']['actual_errno']==0
   raw_files.append(dict(file=str(q.relative_to(d)),sha256=sha(q),raw_allocations=len(vm),actual_protections=len(protected)))
 pins=prior_pins()
 for rel,pin in pins.items():assert sha(ROOT/rel)==pin,rel
 logs={}
 for binary,log in(('sound_cache_startup_tests','normal.stdout'),('sound_cache_startup_tests_asan','sanitizer.stdout')):
  text=(d/'native'/log).read_text();assert 'PASS complete original21A6E0/13E8E0: 6 cases, 4090'in text and 'no map/device/voice admission'in text
  assert 'PASS genuine cache constructor wired through portable126B20 CPU owner'in text
  logs[binary]=dict(binary_sha256=sha(d/'native'/binary),stdout_sha256=sha(d/'native'/log),text=text.strip())
 audit=dict(schema='source-sound-cache-startup-native-audit-v1',audit_tool_sha256=sha(__file__),
   source_report_sha256=sha(d/'measured/report.json'),paired_report_sha256=sha(d/'paired/report.json'),providers_sha256=sha(d/'paired/providers.json'),
   complete_source_cases=len(r['cases']),original_ordered_stores=sum(len(q['stores'])for q in r['cases']),represented_native_ordered_stores=4090,
   complete_source_raw_block_extents=[0x3000,0x4000],whole_paired_bodies=len(pair['owners']),paired_original_instruction_rows_verified=rows,
   independent_byte_equal_files=comparisons,nondeterministic_actual_host_provider_evidence=raw_files,
   prior_pins_verified=pins,native_results=logs,limits=r['boundaries'])
 save(d/'audit.json',encoded(audit));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),independent_files=len(comparisons),paired_rows=rows,prior_pins=len(pins))))

if __name__=='__main__':main()
