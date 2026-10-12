#!/usr/bin/env python3
"""Original indexed activity table and blend operands from complete body matches."""
import argparse,json,struct
from pathlib import Path
from oracle_mp_title_update_display import OLD,NEW,Xbe,_Code
from oracle_mp_unit_constructors_1_1 import sha,save,encoded
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--correspondence',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();proof=json.loads(a.correspondence.read_text());rows=[]
 for path,label,side in ((OLD,'TU1.1','old'),(NEW,'authenticated-u5','u5')):
  image=Xbe(path);code=_Code(image,'.text');out={}
  for entry,kind in ((0x110bf0,'activity_pairs'),(0x114f70,'blend')):
   match=next(r for r in proof['owners']if r['old']==f'{entry:08x}');assert match['unique_full_body_match']
   lines=(a.correspondence.parent/'instructions'/f'{entry:08x}-{side}.asm').read_text().splitlines()
   ins=[code.at(int(line.split()[0],16))for line in lines]
   if kind=='activity_pairs':
    sites=[i for i in ins if i.mnemonic=='cmp'and i.op_str.startswith('edx, dword ptr [ecx*8 + ')]
    assert len(sites)==1;i=sites[0];va=i.operands[1].mem.disp;raw=image.read(va,32)
    out[kind]=dict(source_address=f'{va:08x}',reader=f'{i.address:08x}',raw20_hex=raw.hex(),pairs=[list(struct.unpack_from('<II',raw,n*8))for n in range(4)])
   else:
    for dest,name in (('xmm1','normal_blend_bits'),('xmm0','special_blend_bits')):
     sites=[i for i in ins if i.mnemonic=='movss'and i.op_str.startswith(dest+', dword ptr [0x')];assert len(sites)==1
     i=sites[0];va=i.operands[1].mem.disp;raw=image.read(va,4)
     out[name]=dict(source_address=f'{va:08x}',reader=f'{i.address:08x}',raw4_hex=raw.hex(),bits=struct.unpack('<I',raw)[0])
  rows.append(dict(image=label,image_sha256=sha(path),**out))
 assert rows[0]['activity_pairs']['pairs']==rows[1]['activity_pairs']['pairs']
 for n in('normal_blend_bits','special_blend_bits'):assert rows[0][n]['bits']==rows[1][n]['bits']
 value=dict(schema='unit-constructor-tail-literal-operands-v1',tool_sha256=sha(__file__),correspondence_sha256=sha(a.correspondence),images=rows,boundaries=['Literal executable operands only. No unit storage, animation/resource readiness, player-transition runtime byte or successful callbacks are provided.'])
 save(a.output,encoded(value));print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),images=rows)))
if __name__=='__main__':main()
