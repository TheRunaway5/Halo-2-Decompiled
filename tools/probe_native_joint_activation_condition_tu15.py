#!/usr/bin/env python3
"""Read-only two-phase actual Warthog constraint membership comparison."""
import argparse,bisect,hashlib,json,os,subprocess,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class Before:
 def __init__(self,archive,expected):
  assert hashlib.file_digest(archive.open('rb'),'sha256').hexdigest()==expected;self.capture=json.loads((archive.parent/'capture-native-before.json').read_text());assert not self.capture['guest_abi_fabricated'] and not self.capture['physics_result_supplied'];wanted={r['before_file']:r for r in self.capture['regions']};self.banks={};stream=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name in wanted:
     row=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==row['size'];self.banks[row['address']]=raw
  assert stream.wait()==0 and len(self.banks)==len(wanted);self.starts=sorted(self.banks)
 def read(self,p,n):
  data=bytearray()
  while n:
   at=bisect.bisect_right(self.starts,p)-1;assert at>=0;lo=self.starts[at];off=p-lo;assert off<len(self.banks[lo]),hex(p);size=min(n,len(self.banks[lo])-off);data+=self.banks[lo][off:off+size];p+=size;n-=size
  return bytes(data)
 def word(self,p,n=4):return int.from_bytes(self.read(p,n),'little')
 def array(self,p,n):assert n<4096;return [self.word(p+i*4)for i in range(n)]
 def scan(self):
  w=self.word;world=w(0x52be48);active=self.array(w(world+8),w(world+12));inactive=self.array(w(world+20),w(world+24));pool=w(0x52be5c);extent=w(pool+0x38);assert extent<=4096 and w(pool+0x24)==160;rows=w(pool+0x44);bitmap=w(pool+0x48);objects=w(w(0x4ecbd0)+0x44);tags=w(0x4f0414);vehicles=[]
  for index in range(extent):
   if not w(bitmap+(index>>5)*4)&(1<<(index&31)):continue
   row=rows+index*160;salt=w(row,2)
   if not salt:continue
   datum=w(row+8);header=objects+(datum&0xffff)*12;object=w(header+8);tag=w(object);entry=tags+(tag&0xffff)*16;group=w(entry)
   if group!=0x76656869:continue
   count=w(row+0x74);assert count<=64;bank=w(row+0x70);bodies=[]
   for slot in range(count):
    body=w(bank+slot*96+0x40);island=w(body+0x44);part={'slot':slot,'body':body,'motion':w(body+0x3c),'island':island}
    if island:
     constraints=self.array(w(island+0x58),w(island+0x5c));part.update(active_byte=w(island+0x29,1),active_array_membership=island in active,inactive_array_membership=island in inactive,index=w(island+0x24),island_constraints=[{'constraint':c,'builder_owner':w(w(c)+0x24),'first_body':w(c+12),'second_body':w(c+16)}for c in constraints])
    bodies.append(part)
   authored=w(row+0x80);assert authored<=256;entries=w(row+0x7c);constraints=[{'kind':w(entries+i*12,2),'index':w(entries+i*12+2,2),'handle':w(entries+i*12+4),'constraint':w(entries+i*12+8)}for i in range(authored)]
   model_datum=w(w(entry+8)+0x38);model=w(tags+(model_datum&0xffff)*16+8);physics_datum=w(model+0x24);physics=w(tags+(physics_datum&0xffff)*16+8);rigid_count=w(physics+0x38);assert rigid_count<256;rigid=w(physics+0x3c);node_count=w(physics+0xc8);assert node_count<256;nodes=w(physics+0xcc)
   authored_physics={'model_datum':model_datum,'model':model,'physics_datum':physics_datum,'physics':physics,'rigid_count':rigid_count,'rigid_records':[{'index':i,'node16':w(rigid+i*0x90,2),'region16':w(rigid+i*0x90+2,2),'permutation16':w(rigid+i*0x90+4,2),'flags18':w(rigid+i*0x90+0x18,2),'motion_type1E':w(rigid+i*0x90+0x1e,2)}for i in range(rigid_count)],'nodes':[{'index':i,'name_string_id':w(nodes+i*12),'flags':w(nodes+i*12+4,2),'parent':w(nodes+i*12+6,2)}for i in range(node_count)]}
   limited_count=w(physics+0xe8);assert limited_count<256;limited=w(physics+0xec);authored_physics['limited_hinges']=[{'index':i,'name_string_id':w(limited+i*0x84),'nodeA':w(limited+i*0x84+4,2),'nodeB':w(limited+i*0x84+6,2),'edge':w(limited+i*0x84+0x70,2)}for i in range(limited_count)]
   vehicles.append({'authored_physics':authored_physics,'component':(salt<<16)|index,'component_address':row,'flags':w(row+4),'object':datum,'object_header_salt':w(header,2),'object_type_byte':w(header+3,1),'object_tag':tag,'definition':w(entry+8),'body_count':count,'authored_rows':constraints,'bodies':bodies})
  return {'phase':self.capture['boundary'],'input':self.capture['input'],'world':world,'active_islands':active,'inactive_islands':inactive,'vehicle_components':vehicles}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--joint-archive',required=True,type=Path);p.add_argument('--joint-sha256',required=True);p.add_argument('--process-archive',required=True,type=Path);p.add_argument('--process-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);first=Before(a.joint_archive,a.joint_sha256).scan();second=Before(a.process_archive,a.process_sha256).scan();files=['src/physics/native_tu15_world_integration.cpp','src/physics/native_tu15_island_integration.cpp','src/physics/native_tu15_constraint_step.cpp','src/physics/native_tu15_island_activation.cpp'];j={'passed':True,'scope':__doc__,'joint_archive_sha256':a.joint_sha256,'process_archive_sha256':a.process_sha256,'snapshots':[first,second],'source_conditions':['World319F90 iterates actual active-island array(world+8/count+C), subject to source deactivation decision','Island320B70 takes constraint solver route iff island+5C nonzero; actual constraints list=island+58','Constraint3261E0 dispatches each list entry current vtable+24;31D930 only if that actual owner is present'],'limits':['Snapshots are different actual call phases and gameplay times; neither is a continuous membership trace','No sufficient SDL trigger/deactivation outcome inferred; no flags, list membership or source DATA mutated'],'sources':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest()for p in files},'owned_scratch':str(scratch),'temporary_allocated_bytes_before_cleanup':sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),'raw_banks_written':False,'retained_large_artifacts':[]};(a.output/'report.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps({'vehicles':[x['vehicle_components']for x in j['snapshots']]},indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
