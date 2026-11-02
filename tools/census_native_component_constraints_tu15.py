#!/usr/bin/env python3
"""Read actual BEFORE body/component constraint counts without mutating inputs."""
import argparse,bisect,json,os,struct,subprocess
from pathlib import Path,PurePosixPath
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path);p.add_argument('--archive',type=Path);p.add_argument('--archive-sha256');p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert bool(a.capture)^bool(a.archive);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();provenance={}
 if a.archive:
  assert a.archive_sha256 and sha(a.archive)==a.archive_sha256;members=subprocess.run(['tar','--zstd','-tf',str(a.archive)],capture_output=True,text=True,check=True).stdout.splitlines();assert len(members)<10000
  for name in members:
   path=PurePosixPath(name);assert not path.is_absolute()and'..'not in path.parts
   assert str(path)=='.'or path.name in('capture.json','capture-legacy-provenance.json')or(path.name.startswith('before-')and path.name.endswith('.bin'))
  root=scratch/'banks';root.mkdir();subprocess.run(['tar','--zstd','-xf',str(a.archive),'--no-same-owner','--no-same-permissions','-C',str(root)],check=True);capture=root/'capture.json';provenance=dict(archive=str(a.archive.resolve()),archive_sha256=a.archive_sha256,archive_bytes=a.archive.stat().st_size)
 else:capture=a.capture.resolve();root=capture.parent
 j=json.loads(capture.read_text());regions=sorted(j['regions'],key=lambda r:r['address']);starts=[r['address']for r in regions]
 def raw(address,size):
  n=bisect.bisect_right(starts,address)-1;assert n>=0;(row)=regions[n];at=address-row['address'];assert 0<=at and at+size<=row['size'],(hex(address),size,row)
  path=root/row['before_file'];assert path.resolve().is_relative_to(root.resolve())
  with path.open('rb')as f:f.seek(at);result=f.read(size);assert len(result)==size;return result
 def word(address):return struct.unpack('<I',raw(address,4))[0]
 world=word(0x52be48);manager=word(0x52be5c);components=word(manager+0x44);object_pool=word(0x4ecbd0);headers=word(object_pool+0x44);seen=set();rows=[]
 for role,bankoff,countoff in(('active',8,12),('inactive',0x14,0x18)):
  bank=word(world+bankoff);count=word(world+countoff);assert count<=4096
  for ordinal in range(count):
   island=word(bank+ordinal*4);bodies=word(island+0x3c);body_count=word(island+0x40);assert body_count<=65536
   for n in range(body_count):
    body=word(bodies+n*4);properties=word(body+0x30);property_count=word(body+0x34);assert property_count<=65536;datum=next((word(properties+i*8+4)for i in range(property_count)if word(properties+i*8)==0x2001),None)
    if datum in(None,0xffffffff)or datum in seen:continue
    seen.add(datum);component=components+(datum&0xffff)*160;flags=word(component+4);obj=word(component+8);header=headers+(obj&0xffff)*12;object_data=word(header+8)
    rows.append(dict(role=role,island_ordinal=ordinal,body=body,body_datum=datum,component=component,flags=hex(flags),flag_800=bool(flags&0x800),physical_rows=word(component+0x74),constraint_rows=word(component+0x80),object=obj,object_type=raw(header+3,1)[0],object_data=object_data,definition=word(object_data)))
 extracted=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());report=dict(read_only_actual_before_inputs=True,source_metadata_sha256=sha(capture),source_entry=j.get('entry'),provenance=provenance,world=world,rows=rows,positive_constraints=[r for r in rows if r['constraint_rows']],owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=extracted,retained_large_artifacts=[],tool_sha256=sha(Path(__file__)));save(a.output,report);print(json.dumps(dict(components=len(rows),positive_constraints=report['positive_constraints'],vehicle_components=[r for r in rows if r['object_type']==1],temporary_allocated_bytes=extracted),indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
