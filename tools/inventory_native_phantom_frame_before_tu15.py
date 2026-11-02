#!/usr/bin/env python3
"""Read-only phantom container census from a protected whole-frame BEFORE."""
import argparse,bisect,hashlib,json,os,struct,subprocess,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_only_before=True,selected_callback_claimed=False)
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
  capture=json.loads((a.archive.parent/'manifests/capture.json').read_text());wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name in wanted:r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
  def read(p,n):
   at=bisect.bisect_right(starts,p)-1
   if at<0:return None
   first=starts[at];return banks[first][p-first:p-first+n]if first<=p and p+n<=first+len(banks[first])else None
  def word(p):
   raw=read(p,4);return int.from_bytes(raw,'little')if raw is not None else None
  image=Xbe(NEW_DEFAULT);tables={}
  for section in image.sections:
   raw=image.section_bytes(section)
   for owner in(0x315300,0x315b00):
    needle=struct.pack('<I',owner);offset=raw.find(needle)
    while offset>=0:
     table=section.va+offset-0x1c
     if not(table&3)and read(table+0x1c,4)==needle:tables[table]=owner
     offset=raw.find(needle,offset+1)
  rows=[]
  for first,raw in banks.items():
   for table,owner in tables.items():
    needle=struct.pack('<I',table);offset=raw.find(needle)
    while offset>=0:
     phantom=first+offset;offset=raw.find(needle,offset+1)
     if phantom&15:continue
     header=phantom+(0x80 if owner==0x315300 else 0xc0);count,cap,data=word(header+4),word(header+8),word(header)
     if count is None or cap is None or data is None or count>65536 or count>(cap&0x7fffffff):continue
     listener_count,listener_data=word(phantom+0x40),word(phantom+0x3c)
     if listener_count is None or listener_data is None or listener_count>1024:continue
     values=[];listeners=[]
     for n in range(count):values.append(word(data+4*n))
     for n in range(listener_count):
      listener=word(listener_data+4*n);vtable=word(listener)if listener else None;listeners.append(dict(listener=listener,table=vtable,added_owner=word(vtable)if vtable else None,removed_owner=word(vtable+4)if vtable else None))
     rows.append(dict(phantom=phantom,table=table,add_owner=owner,header=header,data=data,count=count,capacity=cap,items=values,listener_count=listener_count,listeners=listeners))
  report.update(passed=True,archive_sha256=a.archive_sha256,original_sha256=NEW_PIN,frame_entry=capture['entry'],banks=len(banks),before_bytes=sum(map(len,banks.values())),authenticated_tables={hex(k):hex(v)for k,v in tables.items()},candidates=rows,scope='Authenticated currentvtable+1C andplausiblealignedphantomDATA, allcurrentcount/cap/listenerfieldsreadverbatim. Candidates may be dormant; neither actualcallbackselection noralgorithmcauseisinferred.')
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(x.stat().st_blocks*512 for x in scratch.rglob('*')if x.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 print(json.dumps(report,indent=2));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
