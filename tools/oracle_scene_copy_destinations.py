#!/usr/bin/env python3
"""Whole original mode1 copies into genuine captured targets19,21,22.

External destination arguments and explicitly declared camera rectangles vary.
Captured resource records/readiness, source target, raster rectangle, device,
programs and command capacity are never replaced. CPU output is not GPU
submission, sampling, framebuffer or normal camouflage/water-flow evidence.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_scene_copy_rectangles import RectangleTrace,compact_result,inventory
from oracle_camouflage_scene_copy import Snapshot,SNAPSHOT,RAM_PIN,ROOT,PIN,Xbe,read_rows,FUNCTIONS_CSV,uint,words,packed,STACK,STOP

OUT=ROOT/'analysis/multiplayer/game-variants/scene-copy-destinations-20261012'
FIXTURE=ROOT/'tests/fixtures/scene_copy_destinations_original.json'
PRIOR=ROOT/'analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json'
PRIOR_PIN='be070885fdade38af04685a6c330a07017ec4d6a8cfc66da760678a8c55dd7d0'
def sha(raw):return hashlib.sha256(raw).hexdigest()
class DestinationTrace(RectangleTrace):
 def __init__(self,snapshot,image,rows,destination):
  self.destination=destination;super().__init__(snapshot,image,rows)
 def put(self,address,raw):
  # The inherited fresh-call harness declares this exact external call stack.
  # Change only its destination argument, before original352E0 starts.
  if address==STACK and raw==packed([STOP,0xffffffff,1]):
   raw=packed([STOP,self.destination,1])
  return super().put(address,raw)
 def run_target(self):
  index=19 if self.destination==0xffffffff else self.destination
  address=0x4b4b58+index*0x98
  before=self.block(address,0x98)
  result=self.run_copy();result['arguments']=[self.destination,1]
  result['destination_record_before']=before
  result['destination_record_after']=self.block(address,0x98)
  return result
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT);ap.add_argument('--fixture',type=Path,default=FIXTURE)
 args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=True)
 snapshot=Snapshot(SNAPSHOT);assert sha(snapshot.ram)==RAM_PIN and sha(PRIOR.read_bytes())==PRIOR_PIN
 image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
 rows=read_rows(FUNCTIONS_CSV);cameras=[q for q in inventory()if q['name']=='captured_full' or q['name'].startswith('camera_only:')]
 inputs=[dict(name='captured_none',destination_argument=0xffffffff,camera_rectangle_shorts=cameras[0]['camera_rectangle_shorts'],provenance='captured camera with declared external NONE argument')]
 inputs.extend(dict(name=f'target{destination}:{q["name"]}',destination_argument=destination,camera_rectangle_shorts=q['camera_rectangle_shorts'],provenance='declared external destination and camera input; actual captured target record retained') for destination in (19,21,22)for q in cameras)
 assert len(inputs)==37
 results=[];detailed=[];executed={};resources=[]
 for destination in (19,21,22):
  address=0x4b4b58+destination*0x98;raw=snapshot.read(address,0x98)
  width,height,backing=struct.unpack_from('<3I',raw,0x80)
  assert backing and not raw[0x95] and struct.unpack_from('<i',raw,0x78)[0]>0
  count=width*height*4;physical=snapshot.physical(backing)
  resources.append(dict(index=destination,address=address,record_hex=raw.hex(),width=width,height=height,physical_backing=physical,physical_bytes=count,physical_captured_sha256=sha(snapshot.ram[physical:physical+count])))
 for cell in inputs:
  trace=DestinationTrace(snapshot,image,rows,cell['destination_argument'])
  camera_raw=struct.pack('<4h',*cell['camera_rectangle_shorts']);before=trace.read(0x4b9dd0,8)
  writes=[]
  if before!=camera_raw:
   trace.put(0x4b9dd0,camera_raw);writes.append(dict(address=0x4b9dd0,before_hex=before.hex(),declared_hex=camera_raw.hex()))
  result=trace.run_target();executed.update(trace.executed)
  assert result['before']['camera_rectangle_shorts']==cell['camera_rectangle_shorts']==result['after']['camera_rectangle_shorts']
  assert result['before']['raster_rectangle_shorts']==[0,0,480,640]==result['after']['raster_rectangle_shorts']
  assert result['before']['current_target']==0==result['after']['current_target']
  if result['complete_original_return']:
   compact=compact_result(result)
   quad=next(q for q in result['events']if q['entry']==0x34420)
   rect=struct.unpack('<4f',bytes.fromhex(quad['destination_rectangle']['raw_hex']))
   assert rect[0]==rect[2]==0 and rect[1]>0 and rect[3]>0
   compact['destination_dimensions']=[int(rect[1]),int(rect[3])]
  else:compact=dict(complete_original_return=False,stop_before=result['stop_before'])
  results.append(compact);detailed.append(dict(input_name=cell['name'],declared_camera_writes=writes,original=result))
 fixture=dict(schema='original-scene-copy-destinations-v1',boundary=__doc__,inputs=inputs,versions=dict(retail=dict(image_sha256=PIN,results=results)),instruction_patches=[],supplied_returns=[])
 args.fixture.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(schema='original-scene-copy-destinations-report-v1',boundary=__doc__,prior_report_sha256=PRIOR_PIN,fixture_sha256=sha(args.fixture.read_bytes()),snapshot_ram_sha256=RAM_PIN,image_sha256=PIN,
  resources=resources,cases=detailed,complete_original_calls=sum(q['complete_original_return']for q in results),stopped_original_calls=sum(not q['complete_original_return']for q in results),
  executed_original=[dict(address=pc,bytes_hex=image.read(pc,size).hex())for pc,size in sorted(executed.items())],
  dependencies={str(p.relative_to(ROOT)):sha(p.read_bytes())for p in(Path(__file__).resolve(),ROOT/'tools/oracle_scene_copy_rectangles.py',ROOT/'tools/oracle_camouflage_scene_copy.py',ROOT/'tools/oracle_camouflage_pass_services.py',ROOT/'tools/differential_source_object_admission_snapshot.py')},
  instruction_patches=[],supplied_returns=[],limits=['External fresh calls over authentic weather-entry RAM; no recovered normal copy schedule or live camera producer.',
   'Only actual source640x480 and ready captured targets19=320x240,21/22=512x256; retained near1/64,far1024 and mode1 flags.',
   'GPU-resident dirty surfaces, command acceptance/completion, raster centers, interpolation/filtering and framebuffer output are outside this CPU oracle.'])
 path=args.output/'report.json';path.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(cases=len(inputs),complete=report['complete_original_calls'],stopped=report['stopped_original_calls'],distinct_instructions=len(executed),fixture_sha256=report['fixture_sha256'],report_sha256=sha(path.read_bytes())),indent=2))
if __name__=='__main__':main()
