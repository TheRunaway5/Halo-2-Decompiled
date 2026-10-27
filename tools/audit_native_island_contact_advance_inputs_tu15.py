#!/usr/bin/env python3
"""Read-only actual BEFORE motion/collision owner and input-field census."""
import argparse,bisect,hashlib,json,os,struct,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert sha(a.capture)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN;scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();unpack=scratch/'inputs';unpack.mkdir()
 with tarfile.open(a.capture,'r:*')as archive:
  for m in archive.getmembers():
   target=(unpack/m.name).resolve();assert (target==unpack or unpack in target.parents)and(m.isfile()or m.isdir())
  archive.extractall(unpack,filter='data')
 paths=[p for p in unpack.rglob('capture.json')if json.loads(p.read_text()).get('entry')==0x3205b0];assert len(paths)==1;capture=paths[0];d=json.loads(capture.read_text());regions=sorted(d['regions'],key=lambda r:r['address']);addresses=[r['address']for r in regions];read_banks={}
 def read(p,n):
  r=regions[bisect.bisect_right(addresses,p)-1];assert r['address']<=p and p+n<=r['address']+r['size'];path=capture.parent/r['before_file'];read_banks[r['address']]=dict(address=r['address'],bytes=r['size'],file=r['before_file'],sha256=sha(path))
  with path.open('rb')as stream:stream.seek(p-r['address']);b=stream.read(n)
  assert len(b)==n;return b
 def word(p):return struct.unpack('<I',read(p,4))[0]
 def value(p):return dict(address=p,bits=word(p),float_value=struct.unpack('<f',read(p,4))[0])
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');sp=d['before']['gp'][4];island=d['before']['gp'][1];config,record,output,time=[word(sp+4+n*4)for n in range(4)];motions=[];owners={}
 def owner(p):
  if p not in owners:
   b=complete_body(code,p);listing=[f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}'for pc in sorted(b.code)];owners[p]=dict(entry=p,sites=len(b.code),calls=[line for line in listing if ' call 'in line],listing=listing,listing_sha256=hashlib.sha256(('\n'.join(listing)+'\n').encode()).hexdigest())
  return p
 for n in(0,4):
  collidable=word(record+n);body=word(collidable+0x10);motion=word(body+0x3c);table=word(motion);target=word(table+0x74);motions.append(dict(collidable=collidable,body=body,motion=motion,vtable=table,advance_owner=owner(target),cached_time=value(motion+0x7c),before_words=[word(motion+n)for n in range(0,256,4)]))
 agent=word(record+8);table=word(agent);process=owner(word(table+0x10));world=word(island+0x20);policy=word(world+0x254);report=dict(passed=True,source_sha256=NEW_PIN,archive_sha256=a.archive_sha256,before_only=True,captured_after_consumed=False,actual_inputs=dict(island=island,configuration=config,agent_entry=record,output=output,time_bits=time,configuration_words=[value(config+n)for n in range(0,24,4)],motions=motions,agent=agent,process_owner=process,world=world,threshold=value(world+0x24c),policy=policy,policy_mode=word(policy+0xc),policy_threshold=value(policy+0x10)),owners=list(owners.values()),read_bank_hashes=list(read_banks.values()),owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[]);a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('Actual motion/process owners',[(hex(x['entry']),x['sites'])for x in owners.values()])
if __name__=='__main__':main()
