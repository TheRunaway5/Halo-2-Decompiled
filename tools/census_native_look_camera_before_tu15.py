#!/usr/bin/env python3
"""Read current on-foot camera DATA from the protected typed look BEFORE."""
import argparse,bisect,json,os,struct,subprocess,tarfile
from pathlib import Path
from validate_native_look_actual_tu15 import ROOT,sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],physics_source_mutated=False)
 try:
  assert sha(a.archive)==a.archive_sha256;capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());assert capture['input']['owner']==0x18a300 and not capture['physics_result_supplied'];wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as archive:
   for member in archive:
    name=Path(member.name).name
    if name in wanted:r=wanted[name];raw=archive.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);addresses=sorted(banks)
  def word(at,width=4):
   index=bisect.bisect_right(addresses,at)-1;assert index>=0;first=addresses[index];raw=banks[first];offset=at-first;assert 0<=offset and offset+width<=len(raw);return int.from_bytes(raw[offset:offset+width],'little')
  unit=capture['caller']['unit'];objects=word(0x4ecbd0);headers=word(objects+0x44);body=word(headers+(unit&0xffff)*12+8);tags=word(0x4f0414);tag=word(body);definition=word(tags+(tag&0xffff)*16+8)
  fields={hex(offset):word(definition+offset)for offset in(0x218,0x21c,0x224,0x228,0x22c,0x230,0x234,0x238)}
  report.update(passed=True,archive_sha256=a.archive_sha256,unit=unit,body=body,definition=definition,definition_fields_bits=fields,body_fields={'scale_bits':word(body+0xa0),'parent':word(body+0x14),'flags10A':word(body+0x10a,1),'byteAA':word(body+0xaa,1),'blend_bits':word(body+0x2c4),'camera_adjustment_enabled':word(body+0x3c8,1),'transition_mode':word(body+0x3dc,1),'transition_active':word(body+0x3f8,1),'transition_flags':word(body+0x348,2),'forward_bits':[word(body+0x168+n*4)for n in range(3)]},actual_camera_boundary_captured=False,source_boundary='Typed18A300 BEFORE; camera child path selection remains an independent source execution proof')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(q.stat().st_blocks*512 for q in scratch.rglob('*')if q.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
