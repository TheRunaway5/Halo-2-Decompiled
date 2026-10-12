#!/usr/bin/env python3
"""Audit map prefix/pair/export repetitions and mandatory platform stops."""
import argparse,json,re
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
from audit_sound_cache_startup_native import prior_pins as startup_pins
from oracle_sound_map_activation_native import CACHE,CACHE_PIN,PRIOR_PIN
from inspect_multiplayer import Cache
CACHE_MANIFEST=ROOT/'analysis/multiplayer/integration/sound-cache-startup-native/final-manifest.json'
CACHE_MANIFEST_PIN='da6e6f3c7bed6abebe8f9b518e36aeeff866cd11eb76238c5770b6ffb30b5777'
def prior_pins():
 assert sha(CACHE_MANIFEST)==CACHE_MANIFEST_PIN
 m=json.loads(CACHE_MANIFEST.read_text());pins=startup_pins();pins.update(m['prior_pins']);pins[str(CACHE_MANIFEST.relative_to(ROOT))]=CACHE_MANIFEST_PIN
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
 r=json.loads((d/'measured/report.json').read_text());pair=json.loads((d/'paired/report.json').read_text());packet=json.loads((d/'paired/providers.json').read_text());images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0;addresses=set()
 assert r['prior_startup_report_sha256']==PRIOR_PIN
 for q in pair['owners']:
  for tag,x in images.items():
   for line in(d/'paired/instructions'/f'{q["old"]}-{tag}.asm').read_text().splitlines():
    at,raw,*_=line.split();assert x.read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw);rows+=1
    if tag=='old':addresses.add(int(at,16))
 for listing in(d/'measured/instructions').glob('*.asm'):
  for line in listing.read_text().splitlines():
   at,raw,*_=line.split();assert images['old'].read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw)
 for q in packet['constants'].values():
  for key,x in images.items():assert x.read(int(q['old_va'if key=='old'else'u5_va'],16),4).hex()==q['raw4_hex']
 assert not packet['runtime_environment_packet']['copied_into_constants']
 assert sha(CACHE)==CACHE_PIN;c=Cache(CACHE)
 for case in r['cases']:
  assert not case['whole_owner_returned']and not case['platform_callbacks_executed']
  assert case['before']['driver_address']==case['after']['state']['driver_address']=='00000000'
  for region in case['regions']:
   if 'tag'in region:
    t=region['tag'];assert c.tags[int(t['datum'],16)]==t
    literal=bytearray(c.data[t['offset']:t['offset']+t['size']]);address=int(region['address'],16)
    for control in case['controls']:
     at=int(control['address'],16)-address
     if 0<=at<len(literal):assert literal[at:at+4]==int(control['before'],16).to_bytes(4,'little');literal[at:at+4]=int(control['after'],16).to_bytes(4,'little')
    assert bytes(literal).hex()==region['raw_hex']
  for tag in case['tag_table']:
   t=c.tags[int(tag['datum'],16)];assert t['offset']+c.magic==int(tag['address'],16)and(int(tag['datum'],16)&0xffff)==int(tag['index'],16)
  for clock in case['clock_calls']:assert clock['provider']=='actual clock_gettime_ns(CLOCK_BOOTTIME)'and((clock['observed_ns']//1000000)&0xffffffff)==clock['tick_ms_u32']
 c.data.close();c.stream.close()
 for rel in('include/halo2/sound_map_activation.hpp','src/sound_map_activation.cpp','tests/sound_map_activation_tests.cpp'):assert (ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 sites={int(x,16)for x in re.findall(r'(?:write|global)\(0x([0-9a-f]+)',(ROOT/'src/sound_map_activation.cpp').read_text())};assert sites<=addresses
 pins=prior_pins()
 for rel,pin in pins.items():assert sha(ROOT/rel)==pin,rel
 native={}
 for binary,log in(('sound_map_activation_tests','normal'),('sound_map_activation_tests_asan','sanitizer')):
  text=(d/'native'/f'{log}.stdout').read_text();assert '3 cases, 38 exact ordered stores, 2 recorded real clock inputs'in text and 'no map/plugin/effect/voice admission'in text
  assert text==(d/'native-repeat'/f'{log}.stdout').read_text()
  assert not(d/'native'/f'{log}.stderr').read_bytes()and not(d/'native-repeat'/f'{log}.stderr').read_bytes()
  native[binary]=dict(binary_sha256=sha(d/'native'/binary),stdout_sha256=sha(d/'native'/f'{log}.stdout'),text=text.strip())
 report=dict(schema='sound-map-activation-native-audit-v1',audit_tool_sha256=sha(__file__),original_report_sha256=sha(d/'measured/report.json'),paired_report_sha256=sha(d/'paired/report.json'),providers_sha256=sha(d/'paired/providers.json'),source_cases=len(r['cases']),exact_prefix_stores=sum(len(q['stores'])for q in r['cases']),complete_static_paired_bodies=len(pair['owners']),paired_instruction_rows=rows,independent_byte_equal_files=comparisons,prior_pins_verified=pins,native_results=native,
   limits=r['bounds']+['Complete represented native CPU owner is statically instruction-mapped; only the actual stopped source prefix and two declared metadata controls are differentially executed. No platform callbacks return or whole original map activation completes.','Recorded host clock/data-import routing is explicit; fresh runtime environmental packet and saved output setting remain unavailable live-owner inputs.'])
 save(d/'audit.json',encoded(report));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),independent_files=len(comparisons),paired_rows=rows,prior_pins=len(pins))))
if __name__=='__main__':main()
