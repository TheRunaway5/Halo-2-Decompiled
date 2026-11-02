#!/usr/bin/env python3
"""Offline unchanged TU1.5 player-effect tick; all banks are declared inputs."""
import argparse,gzip,hashlib,json,os,struct
from pathlib import Path
import oracle_native_campaign_screen_flash_sample_tu15 as source
import validate_native_geometry_tu15 as physical

BASE=0x1000000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=source.Xbe(source.NEW_DEFAULT)
    assert sha(source.NEW_DEFAULT)==source.NEW_PIN
    source.ENTRIES=(0x156480,);meta=source.metadata(image);owned=source.prepare(image,meta,scratch)
    rows=[]
    for rate in [15,30,60]:
      for present in [0,1,5,10,15]:
       for ticks in [-2,0,1,4]:
        for flags in [[0,3,255][len(rows)%3]]:
         n=len(rows);bank,clock,feedback,users=BASE+0x100,BASE+0x800,BASE+0x900,BASE+0xd00
         data=bytearray((i*13+n*7)&255 for i in range(0x2000))
         def put(at,value,width=4):data[at-BASE:at-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
         put(bank,100);put(bank+4,ticks,2);put(bank+6,n&1,1)
         put(bank+0x24,ticks,2);put(bank+0x28,flags)
         put(clock,0,2);put(clock+2,rate,2);put(clock+4,struct.unpack('<I',struct.pack('<f',1/rate))[0]);put(clock+8,103)
         for user in range(4):
          put(users+0xc+user*4,0x10001+user if present&(1<<user) else 0xffffffff)
          slot=bank+0x30+user*0xa0
          for off in [0x7c,0x7e,0x80,0x82]:put(slot+off,ticks+user%2,2)
          put(slot+0x88,flags,1);put(slot+0x89,[0,1,128,255][user],1)
          for off in range(0x84,0x88):put(slot+off,[0,1,128,255][(user+off)%4],1)
         banks=[{'address':BASE,'before':data.hex()},
          *[{'address':at,'before':struct.pack('<I',value).hex()} for at,value in
            [(0x51df5c,bank),(0x51df54,clock),(0x50ebf8,feedback),(0x4f54f0,users)]],
          {'address':0x46a000,'before':image.read(0x46a000,0x2000).hex()}]
         gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=physical.SP
         rows.append({'id':n,'name':f'rate{rate}_users{present}_ticks{ticks}_flags{flags}',
          'entry':0x156480,'gp':gp,'matrix':0,'source':0,'banks':banks,
          'before_fp':[0x27f,0,0x1f80|((n%4)<<13)],'live_x87':[],'input_changes':[]})
    rows=physical.physical(owned,rows,'complete')
    assert all(r['completed'] for r in rows)
    fixture={'image_sha256':source.NEW_PIN,'backend':'unchanged-tu15-player-effect-tick-physical-v1',
      'instruction_patches':[],'supplied_original_outcomes':False,'owners':meta['owners'],'cases':rows}
    raw=json.dumps(fixture,separators=(',',':')).encode();packed=gzip.compress(raw,mtime=0)
    assert gzip.decompress(packed)==raw and len(packed)<1048576,len(packed)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(packed)
    report={'passed':True,'cases':len(rows),'image_sha256':source.NEW_PIN,'owners':meta['owners'],
      'original_instructions':sum(r['original_instructions'] for r in rows),
      'reads':sum(len(r['reads']) for r in rows),'stores':sum(len(r['writes']) for r in rows),
      'fixture':str(a.output),'fixture_bytes':len(packed),'fixture_sha256':sha(a.output),
      'instruction_patches':[],'supplied_original_outcomes':False,'retained_large_artifacts':[]}
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    code=source.common._Code(image,'.text')
    listing=''.join(f"{s['instruction']:08x} {code.at(s['instruction']).mnemonic} {code.at(s['instruction']).op_str}\n" for s in meta['sites'])
    a.report.with_suffix('.asm').write_text(listing);print(json.dumps(report))
if __name__=='__main__':main()
