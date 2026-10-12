#!/usr/bin/env python3
"""Import authenticated immutable stream operands; do not import mutable queue state."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
PAIR=ROOT/'analysis/multiplayer/integration/sound-driver-stream-native/paired'
ABI=ROOT/'analysis/multiplayer/integration/sound-driver-stream-native/method-abi'
PAIR_PIN='781933decd99123c63996cdea1900a527247db159750f9dd74711dbe8d254646'
ABI_PIN='5d892b7f3c223c6274cbf70ae43c4a3b496c8e2bdd3c8d27117c3992bbe4be9b'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(PAIR/'report.json')==PAIR_PIN and sha(ABI/'report.json')==ABI_PIN
 pair=json.loads((PAIR/'report.json').read_text());abi=json.loads((ABI/'report.json').read_text());lines=[];images=[]
 for index,(side,path)in enumerate((('old',OLD),('u5',NEW)),1):
  assert sha(path)==PINS[str(path)];xbe=Xbe(path);code=_Code(xbe,'.text');operands=[]
  def add(field,address,expected=None):
   raw=xbe.read(address,4);value=int.from_bytes(raw,'little')
   if expected is not None:assert value==expected
   operands.append(dict(field=field,address=f'{address:08x}',raw_hex=raw.hex(),value=f'{value:08x}'));return value
  owner=next(q for q in pair['owners']if q['old']=='00220520')
  for field,old_va in(('one_bits',0x465030),('stereo_bits',0x46507c)):
   word=next(q for q in owner['memory']if int(q['old_va'],16)==old_va)
   address=int(word['old_va'if side=='old'else'new_va'],16);add(field,address,int.from_bytes(bytes.fromhex(word['old_hex'if side=='old'else'new_hex']),'little'))
  for field,owner_entry,old_pc,expected in[
    ('pcm_queue','00220520',0x2205b3,int(abi['images'][index-1]['owners'][0]['queue'],16)),
    ('adpcm_queue','00220520',0x220580,int(abi['images'][index-1]['owners'][1]['queue'],16))]:
   left=(PAIR/'instructions'/f'{owner_entry}-old.asm').read_text().splitlines();right=(PAIR/'instructions'/f'{owner_entry}-{side}.asm').read_text().splitlines()
   at=next(i for i,row in enumerate(left)if int(row.split()[0],16)==old_pc)
   instruction=code.at(int(right[at].split()[0],16));assert instruction.mnemonic=='mov'and instruction.imm_size==4
   add(field,instruction.address+instruction.imm_offset,expected)
  for field,queue in zip(('pcm_initialize_method','adpcm_initialize_method'),abi['images'][index-1]['owners'][:2],strict=True):
   add(field,int(queue['vtable'],16),int(queue['entry'],16))
  left=(PAIR/'instructions/002215f0-old.asm').read_text().splitlines();right=(PAIR/'instructions'/f'002215f0-{side}.asm').read_text().splitlines()
  at=next(i for i,row in enumerate(left)if row.startswith('00221661 '));instruction=code.at(int(right[at].split()[0],16))
  assert instruction.mnemonic=='mov'and instruction.imm_size==4
  add('completion_callback',instruction.address+instruction.imm_offset)
  lines.append('\t'.join(['C',str(index),*[q['value']for q in operands]]))
  images.append(dict(path=str(path),sha256=sha(path),operands=operands))
 fixture=a.output/'constants.tsv';save(fixture,('\n'.join(lines)+'\n').encode())
 report=dict(schema='sound-driver-stream-immutable-operands-v1',exporter_sha256=sha(__file__),paired_report_sha256=PAIR_PIN,method_abi_sha256=ABI_PIN,images=images,immutable_operand_bytes=56,fixture_sha256=sha(fixture),mutable_queue_or_driver_bytes_imported=False,limits=['Base mapping absent; queue/driver/scratch backing and device/stream ownership remain separate.'])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(fixture_sha256=sha(fixture),report_sha256=sha(a.output/'report.json'),images=images,immutable_operand_bytes=56)))
if __name__=='__main__':main()
