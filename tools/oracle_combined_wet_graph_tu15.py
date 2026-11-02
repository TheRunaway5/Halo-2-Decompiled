#!/usr/bin/env python3
"""Observe native wet-chain boundaries in the unchanged complete original GP.

Offline reference only. Explicit raw mix inputs enter after authentic original
installation; module state/history/status and program words are never overlaid.
Logical reference DMA histories do not qualify physical format2 or a PCM scale.
"""
import argparse,hashlib,importlib.util,json,os,shutil,struct,subprocess
from pathlib import Path


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 root=Path.cwd();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve previous reference results')
 stage=root/'analysis/asset-setup/mcc-native-20261015/native-sound-crosstalk-record0-join-v3'
 spec=importlib.util.spec_from_file_location('original_wet_inputs',stage/'original-inputs.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 gp,title,header,installation=module.acquire_original()
 oracle=scratch/'oracle';core=scratch/'core';shutil.copytree(stage/'oracle',oracle);shutil.copytree(stage/'corrected-core-v3',core)
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
 pins={str(p.relative_to(stage)):sha(p) for d in ('oracle','corrected-core-v3') for p in (stage/d).rglob('*') if p.is_file()}
 source=oracle/'reference.c';before=sha(source);text=source.read_text()
 for old,new in [('[16][343]','[64][727]'),('i<343;','i<727;'),('c.frame_count<=16','c.frame_count<=64')]:
  assert text.count(old)==1,old;text=text.replace(old,new)
 text=text.replace('#include "join-observations.inc"','#include "join-observations.inc"\n#include "combined-wet-observations.inc"')
 text=text.replace('    record0_join_checkpoint(pc);','    record0_join_checkpoint(pc);\n    combined_wet_checkpoint(pc);')
 source.write_text(text)
 join=oracle/'join-observations.inc';join.write_text(join.read_text().replace('join_seen[16]','join_seen[64]'))
 shutil.copyfile(root/'tools/data/combined_wet_graph_observations.inc',oracle/'combined-wet-observations.inc')
 argv=['cc','-std=c11','-O1','-I'+str(oracle),'-I'+str(core),str(source),str(oracle/'observation.c'),str(core/'dsp_cpu.c'),str(core/'dsp.c'),str(core/'dsp_dma.c'),'-lcrypto','-o',str(scratch/'original-wet-reference')]
 built=subprocess.run(argv,capture_output=True);assert built.returncode==0,built.stderr.decode()[-3000:]
 rng=0x614cc89a
 def draw():
  nonlocal rng;rng=(rng*1664525+1013904223)&0xffffffff;return (rng>>8)&0xffffff
 zero=[0]*32
 def silence():return dict(fanout=zero.copy(),mix=[zero.copy() for _ in range(4)],output_baselines=[zero.copy() for _ in range(8)])
 cases=[]
 for channel in range(4):
  first=silence();first['mix'][channel][0]=0x400000
  cases.append(dict(name='isolated-bin-'+str(10+channel),frames=[first]+[silence() for _ in range(15)]))
 frames=[]
 for f in range(64):
  row=silence()
  if f<24:
   row['fanout']=[draw() for _ in range(32)];row['mix']=[[draw() for _ in range(32)] for _ in range(4)]
   row['output_baselines']=[[draw() for _ in range(32)] for _ in range(8)]
  frames.append(row)
 cases.append(dict(name='retained-random-then-tail',frames=frames));result=[]
 for case in cases:
  frames=[]
  for row in case['frames']:
   old=[0]*343;old[0]=128;old[311:343]=row['fanout']
   frames.append(old+sum(row['mix'],[])+sum(row['output_baselines'],[]))
  payload=b'GPXTLK10'+struct.pack('<4I',len(gp),len(title),0xc9000,len(frames))+gp+title+struct.pack('<'+'I'*(727*len(frames)),*sum(frames,[]))
  log=scratch/(case['name']+'.jsonl');err=scratch/(case['name']+'.stderr')
  with log.open('wb') as stdout,err.open('wb') as stderr:run=subprocess.run([str(scratch/'original-wet-reference'),'1000000'],input=payload,stdout=stdout,stderr=stderr,timeout=120)
  assert run.returncode==0,(case['name'],run.returncode,err.read_text()[-2000:],log.read_text()[-1000:])
  events=[json.loads(line) for line in log.read_text().splitlines()]
  install=next(v for v in events if v['event']=='installed_memory_at_entry');assert install['code_mismatches']==install['state_mismatches']==0
  finish=next(v for v in events if v['event']=='result');assert finish['code_input_mismatches_after_frame']==0 and not finish['whole_processor_parity']
  assert [v['index'] for v in events if v['event']=='effect_record_entry']==list(range(15))*len(frames)
  captures=[]
  for f,declared in enumerate(case['frames']):
   prep=[v for v in events if v['event']=='combined_wet_prepare' and v['frame']==f]
   reverb=[v for v in events if v['event']=='combined_wet_reverb' and v['frame']==f]
   assert len(prep)==5 and len(reverb)==4
   assert prep[0]['mix']==sum(declared['mix'],[])
   assert all(v['workspace_hash']==prep[1]['workspace_hash'] for v in prep[1:])
   assert prep[-1]['mix'][64:96]==reverb[0]['input'] and prep[-1]['mix'][32:64]==reverb[2]['input']
   assert prep[-1]['workspace_hash']==reverb[0]['workspace_hash'],'Intervening chorus changed the wet workspace'
   assert reverb[1]['workspace_hash']==reverb[2]['workspace_hash'],'Workspace not shared between both reverb records'
   assert reverb[0]['outputs']==sum(declared['output_baselines'][4:],[]) and reverb[2]['outputs']==sum(declared['output_baselines'][:4],[])
   assert all(v['modifiers']==[65535]*8 for v in prep+reverb)
   if captures:
    for index,begin in ((0,reverb[0]),(1,reverb[2])):
     assert begin['ring_hashes']==captures[-1]['reverb'][index]['ring_hashes'],'A different original record changed retained history'
   captures.append(dict(inputs=declared,prepare_status=[v['sr'] for v in prep[:4]],fanout_state=prep[-1]['fanout_state'],
    distortion_history=prep[-1]['distortion_history'],
    distortion_state=prep[-1]['distortion_state'],accumulator_states=prep[-1]['accumulator_states'],prepared_mix=prep[-1]['mix'],
    prepared_workspace_hash=prep[-1]['workspace_hash'],reverb=[dict(status=begin['sr'],state=end['state'],
     input=end['input'],outputs=end['outputs'],ring_hashes=end['ring_hashes'],workspace_hash=end['workspace_hash'])
     for begin,end in ((reverb[0],reverb[1]),(reverb[2],reverb[3]))]))
  result.append(dict(name=case['name'],frames=captures,original_events_sha256=sha(log),whole_original_graph_frames=len(frames)))
  print(case['name'],len(frames),'whole original graph frames',flush=True)
 report=dict(scope=__doc__,cases=result,original_xbe_sha256=module.NEW_PIN,original_gp_sha256=hashlib.sha256(gp).hexdigest(),
  decoded_title_sha256=hashlib.sha256(title).hexdigest(),original_program_patches=[],callee_or_availability_results_supplied=[],
  all15_original_records_execute_each_frame=True,state_history_status_overlays=False,original_ring_history_carries_without_overlay=True,
  source_pins=pins,reference_wrapper_before_sha256=before,reference_wrapper_after_sha256=sha(source),compile_command=argv,
  physical_dma_voice_producer_or_device_verified=False,retained_large_artifacts=[])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,separators=(',',':'))+'\n')


if __name__=='__main__':main()
