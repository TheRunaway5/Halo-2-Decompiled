#!/usr/bin/env python3
"""Read-only genuine typed2E5320 DATA census, no source behavior supplied."""
import argparse,bisect,hashlib,json,os,struct,subprocess,tarfile
from pathlib import Path

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert os.environ.get('HALO2_TEST_SCRATCH');assert hashlib.file_digest(a.archive.open('rb'),'sha256').hexdigest()==a.archive_sha256
 capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());typed=capture['input'];assert typed['owner']==0x2e5320 and typed['preserved_x87_depth']==0 and not capture['guest_abi_fabricated'] and not capture['physics_result_supplied'];wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
 with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
  for member in tar:
   name=Path(member.name).name
   if name in wanted:
    row=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==row['size'];banks[row['address']]=raw
 assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
 def read(p,n):
  out=bytearray()
  while n:
   at=bisect.bisect_right(starts,p)-1;assert at>=0;lo=starts[at];off=p-lo;assert off<len(banks[lo]),hex(p);size=min(n,len(banks[lo])-off);out+=banks[lo][off:off+size];p+=size;n-=size
  return bytes(out)
 def word(p):return int.from_bytes(read(p,4),'little')
 rows=[]
 for name in ['first','second']:
  payload=typed[name];shape=word(payload);table=word(shape);root=payload;chain=[]
  while word(root+12):chain.append(root);root=word(root+12);assert len(chain)<64
  body=word(root+32);motion=word(body+60);row={'which':name,'payload':payload,'shape':shape,'table':table,'type_owner':word(table+20),'support_owner':word(table+48),'center_owner':word(table+56),'encoding_owner':word(table+60),'root_payload':root,'parents':chain,'body':body,'motion':motion,'motion_linear_vec4_words':list(struct.unpack('<4I',read(motion+64,16))),'shape_words0_34':list(struct.unpack('<14I',read(shape,56)))};rows.append(row)
 agent=typed['agent'];report={'passed':True,'scope':__doc__,'archive_sha256':a.archive_sha256,'actual_input':typed,'bank_bytes':sum(map(len,banks.values())),'body_shapes':rows,'agent':{'table':word(agent),'manager':word(agent+8),'cache_count':word(agent+24),'plane_vec4_words':list(struct.unpack('<4I',read(agent+32,16))),'prior_motion_mask':word(agent+240)},'profiling_cursor':word(0x48c974),'profiling_limit':word(0x48c978),'retained_large_artifacts':[],'raw_banks_written':False};(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
