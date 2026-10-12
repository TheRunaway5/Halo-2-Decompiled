#!/usr/bin/env python3
"""Export complete raw-submit inputs/stores and literal serialized map memory."""
import argparse,json,hashlib
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
from oracle_sound_submit_native import CACHE,CACHE_PIN,AUDIO_ARENA,AUDIO_CAPACITY,REQUEST
from inspect_multiplayer import Cache
ORDER=('system','tag_table','current_matg','current_ugh','current_scenario','effect_pool','controller_pool','controller_root')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--pair',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.report.read_text());pair=json.loads(a.pair.read_text());assert sha(CACHE)==CACHE_PIN
 c=Cache(CACHE);resident=[t for t in c.tags.values()if t['offset']is not None];base=(min(t['offset']for t in resident)+c.magic)&~4095;end=(max(t['offset']+t['size']for t in resident)+c.magic+4095)&~4095;offset=base-c.magic;metadata=bytes(c.data[offset:min(len(c.data),offset+end-base)]);metadata+=bytes(end-base-len(metadata));table=bytes.fromhex(r['images'][0]['metadata']['table_hex']);literal=table+metadata
 save(a.output/'literal-lockout-regions.bin',literal)
 lines=['SOUND_SUBMIT_NATIVE_V1',f'LITERAL\t{r["cache_sha256"]}\t{base:08x}\t{len(table)}\t{len(metadata)}']
 tables={q['image']:q for q in pair['effect_callback_tables']};writes=0;counts=[]
 for im in r['images']:
  assert im['metadata']['table_hex']==r['images'][0]['metadata']['table_hex'];initial=bytes.fromhex(im['arena_initial_hex']);assert len(initial)==AUDIO_CAPACITY
  lines.append('IMAGE\t'+im['image']+'\t'+tables[im['image']]['address']+'\t'+'\t'.join(im['state'][k]for k in ORDER));lines.append('ARENA\t'+initial.hex())
  for case in im['cases']:
   arena=bytearray(initial);request=bytearray.fromhex(case['request_before_hex']);lines.append('CASE\t'+case['sound']['datum']+'\t'+case['request_before_hex']+'\t'+case['request_after_hex']+'\t'+case['return_eax']);assert case['outcome']=='returned'and not case['invalid']
   for q in case['stores']:
    at=int(q['address'],16);n=q['bytes'];region=arena if AUDIO_ARENA<=at<AUDIO_ARENA+AUDIO_CAPACITY else request;pos=at-(AUDIO_ARENA if region is arena else REQUEST)
    assert int.from_bytes(region[pos:pos+n],'little')==int(q['before'],16);region[pos:pos+n]=int(q['after'],16).to_bytes(n,'little')
    lines.append('STORE\t'+'\t'.join(str(q[k])for k in('instruction','address','bytes','before','after')));writes+=1
   assert hashlib.sha256(arena).hexdigest()==case['arena_sha256'] and request.hex()==case['request_after_hex'];lines.append('END')
  counts.append(len(im['cases']))
 save(a.output/'fixtures.tsv',('\n'.join(lines)+'\n').encode());save(a.output/'fixtures.json',encoded(dict(schema='sound-submit-source-fixtures-v1',exporter_sha256=sha(__file__),report_sha256=sha(a.report),pair_sha256=sha(a.pair),cache_sha256=CACHE_PIN,fixture_sha256=sha(a.output/'fixtures.tsv'),literal_regions_sha256=sha(a.output/'literal-lockout-regions.bin'),cases=sum(counts),case_counts=counts,ordered_writes=writes,independently_reconstructed_full_regions=True,metadata_bytes=len(literal),scope='Complete raw160 caller backing/source-created unavailable startup arena/literal current-map ABI inputs; no ready/admission/source/native-platform result. Full arena reconstruction verifies each original outputSHA.')))
 c.data.close();c.stream.close();print(json.dumps(dict(fixture_sha256=sha(a.output/'fixtures.tsv'),cases=sum(counts),ordered_writes=writes,metadata_bytes=len(literal))))
if __name__=='__main__':main()
