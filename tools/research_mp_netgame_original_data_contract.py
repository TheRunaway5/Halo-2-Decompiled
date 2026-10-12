#!/usr/bin/env python3
"""Map measured original operands to the semantic OriginalData provider ABI."""
import argparse,hashlib,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded
from research_mp_original_data_u5_audit import fnv

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/netgame-original-data'
REPORT_PIN='2d3035514210f9a22158ced7dd7e7df4ef295a234e74ba734a3edcbd439af8d5'
# Semantic XYZ coordinates retain actual original operands, including shared Z.
COMMON={'MapPack11':(0x465518,0x465514,0x446df8,0x465510,0x46550c,0x446df8,0x46504c,0x46509c),
        'TitleUpdate15':(0x46a5c8,0x46a5cc,0x44a8a0,0x46a5c0,0x46a5c4,0x44a8a0,0x46a0ec,0x46a13c)}
RELIC=(0x46a5bc,0x46a5b8,0x46a5b4,0x46a5b0,0x46a4fc,0x46a5ac,0x46a178)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);out=p.parse_args().output_dir
 assert sha(out/'report.json')==REPORT_PIN
 report=json.loads((out/'report.json').read_text());images=[]
 for image in report['images']:
  rows={int(q['address'],16):q for q in image['constants_in_unique_first_instruction_order']}
  names=[f'beaver_creek_targets[{row}][{axis}]'for row in range(2)for axis in range(3)]+['distance_squared_threshold','beaver_creek_x_delta']
  addresses=list(COMMON[image['image']]);relic_present=image['image']=='TitleUpdate15'
  if relic_present:
   names+=[f'relic.targets[{row}][{axis}]'for row in range(2)for axis in range(3)]+['relic.z_delta'];addresses+=RELIC
  mapped=[]
  for field,address in zip(names,addresses):
   source=rows[address]
   mapped.append(dict(field=field,address=source['address'],raw_file_offset=source['raw_file_offset'],raw4_hex=source['source_raw4_hex'],source_first_instruction=source['first_instruction'],source_read_sites=source['read_sites']))
  packet=b''.join(bytes.fromhex(q['raw4_hex'])for q in mapped)
  images.append(dict(image=image['image'],relic_present=relic_present,semantic_fields=mapped,semantic_packet_bytes=len(packet),semantic_packet_sha256=hashlib.sha256(packet).hexdigest(),semantic_packet_fnv1a64=fnv(packet),
                     original_unique_operand_packet_sha256=image['packet_sha256'],original_unique_operand_packet_fnv1a64=image['packet_fnv1a64']))
 assert images[0]['semantic_fields'][2]['raw4_hex']==images[0]['semantic_fields'][5]['raw4_hex']
 assert [q['raw4_hex']for q in images[0]['semantic_fields']]==[q['raw4_hex']for q in images[1]['semantic_fields'][:8]]
 result=dict(schema=1,tool_sha256=sha(__file__),original_operand_report_sha256=REPORT_PIN,
  member='OriginalData::multiplayer_netgame_postprocess_constants',type='optional<MultiplayerNetgamePostprocessConstants>',
  header='halo2/multiplayer_netgame_postprocess.hpp',base_optional_absent=True,images=images,
  no_native_outputs_used=True,no_generated_tuning_words=True,
  scope='Semantic mapping of measured original raw operands only. Separate native postprocessor and root enum/Cache/runtime integration are outside this provider evidence.')
 save(out/'provider-contract.json',encoded(result));print(json.dumps(dict(sha256=sha(out/'provider-contract.json'),images=len(images))))
if __name__=='__main__':main()
