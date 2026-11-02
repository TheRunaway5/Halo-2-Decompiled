#!/usr/bin/env python3
"""Record the unchanged whole original GP graph with declared bins6..9 inputs.

The pinned reference core remains an offline oracle only. Original title/GP
words come from authenticated XBE transforms; only external mix samples and
four explicit gain parameters are supplied. No state/history, module status,
instructions, voice availability or callee result is overlaid.
"""
import argparse,gzip,hashlib,importlib.util,json,os,shutil,struct,subprocess
from pathlib import Path


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 root=Path.cwd();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve previous reference results')
 stage=root/'analysis/asset-setup/mcc-native-20261015/native-sound-crosstalk-record0-join-v3'
 spec=importlib.util.spec_from_file_location('original_dry_inputs',stage/'original-inputs.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 gp,title,header,installation=module.acquire_original()
 oracle=scratch/'oracle';core=scratch/'core';shutil.copytree(stage/'oracle',oracle);shutil.copytree(stage/'corrected-core-v3',core)
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
 pins={str(p.relative_to(stage)):sha(p) for d in ('oracle','corrected-core-v3') for p in (stage/d).rglob('*') if p.is_file()}
 source=oracle/'reference.c';before=sha(source);text=source.read_text()
 assert text.count('[16][343]')==1 and text.count('i<343;')==1
 text=text.replace('[16][343]','[16][507]').replace('i<343;','i<507;')
 text=text.replace('#include "join-observations.inc"','#include "join-observations.inc"\n#include "combined-dry-observations.inc"')
 text=text.replace('    record0_join_checkpoint(pc);','    record0_join_checkpoint(pc);\n    combined_dry_checkpoint(pc);')
 source.write_text(text);shutil.copyfile(root/'tools/data/combined_dry_graph_observations.inc',oracle/'combined-dry-observations.inc')
 argv=['cc','-std=c11','-O1','-I'+str(oracle),'-I'+str(core),str(source),str(oracle/'observation.c'),str(core/'dsp_cpu.c'),str(core/'dsp.c'),str(core/'dsp_dma.c'),'-lcrypto','-o',str(scratch/'original-dry-reference')]
 built=subprocess.run(argv,capture_output=True);assert built.returncode==0,built.stderr.decode()[-3000:]
 rng=0x981d23ac
 def draw():
  nonlocal rng;rng=(rng*1664525+1013904223)&0xffffffff;return (rng>>8)&0xffffff
 zero=[0]*32;cases=[]
 for channel in range(4):
  audio=[zero.copy() for _ in range(4)];audio[channel][0]=0x400000
  cases.append(dict(name='isolated-bin-'+str(6+channel),frames=[dict(audio=audio,modulation=zero,gains=[0]*4)]+[
   dict(audio=[zero]*4,modulation=zero,gains=[0]*4) for _ in range(3)]))
 cases.append(dict(name='retained-nonzero-gain',frames=[dict(audio=[[draw() for _ in range(32)] for _ in range(4)],
  modulation=[draw()>>1 for _ in range(32)],gains=[0x200000,0x400000,3003121,0x600000]) for _ in range(16)]))
 result=[]
 for case in cases:
  frames=[([0]*343+sum(v['audio'],[])+v['modulation']+v['gains']) for v in case['frames']]
  payload=b'GPXTLK10'+struct.pack('<4I',len(gp),len(title),0xc9000,len(frames))+gp+title+struct.pack('<'+'I'*(507*len(frames)),*sum(frames,[]))
  log=scratch/(case['name']+'.jsonl');err=scratch/(case['name']+'.stderr')
  with log.open('wb') as stdout,err.open('wb') as stderr:run=subprocess.run([str(scratch/'original-dry-reference'),'1000000'],input=payload,stdout=stdout,stderr=stderr,timeout=60)
  assert run.returncode==0,(case['name'],run.returncode,err.read_text()[-1000:],log.read_text()[-1000:])
  events=[json.loads(line) for line in log.read_text().splitlines()]
  install=next(v for v in events if v['event']=='installed_memory_at_entry');assert install['code_mismatches']==install['state_mismatches']==0
  finish=next(v for v in events if v['event']=='result');assert finish['code_input_mismatches_after_frame']==0 and not finish['whole_processor_parity']
  assert [v['index'] for v in events if v['event']=='effect_record_entry']==list(range(15))*len(frames)
  captures=[]
  for f,declared in enumerate(case['frames']):
   nodes=[v for v in events if v['event']=='combined_dry_node' and v['frame']==f]
   begins=[next(v for v in nodes if v['record']==r and v['point']=='begin') for r in range(4,8)]
   ends=[next(v for v in nodes if v['record']==r and v['point']=='end') for r in range(4,8)]
   boundaries=[v for v in events if v['event']=='combined_dry_boundary' and v['frame']==f]
   cross=[v for v in events if v['event']=='crosstalk_snapshot' and v['frame']==f];assert len(cross)==2 and len(boundaries)==3
   assert boundaries[0]['base_outputs']==boundaries[1]['base_outputs'],'Intervening reverb changed dry output baselines'
   assert boundaries[0]['history']==(captures[-1]['history'] if captures else [0]*1601),'Other graph records changed the retained chorus owner'
   assert [v['audio'] for v in begins]==declared['audio'] and all(v['modulation']==declared['modulation'] for v in begins)
   assert cross[0]['inputs']==sum((v['output'] for v in ends),[]),'Intervening records changed the chorus->crosstalk join'
   assert all(v['modifiers']==[65535]*8 for v in begins+cross)
   captures.append(dict(inputs=declared,base_outputs=boundaries[0]['base_outputs'],chorus_status=[v['sr'] for v in begins],
    crosstalk_status=cross[0]['sr'],chorus_flags=[v['state'][4] for v in ends],
    cursors=[v['state'][10]-v['state'][2] for v in ends],history=boundaries[1]['history'],
    chorus_outputs=[v['output'] for v in ends],crosstalk_state=cross[1]['state_words'],outputs=boundaries[2]['base_outputs']))
  result.append(dict(name=case['name'],frames=captures,original_events_sha256=sha(log),whole_original_graph_frames=len(frames)))
  print(case['name'],len(frames),'whole original graph frames',flush=True)
 report=dict(scope=__doc__,cases=result,original_xbe_sha256=module.NEW_PIN,original_gp_sha256=hashlib.sha256(gp).hexdigest(),
  decoded_title_sha256=hashlib.sha256(title).hexdigest(),original_program_patches=[],callee_or_availability_results_supplied=[],
  all15_original_records_execute_each_frame=True,parameter_and_raw_mix_inputs_are_declared=True,state_history_status_overlays=False,
  original_chorus_history_carries_without_overlay=True,
  source_pins=pins,reference_wrapper_before_sha256=before,reference_wrapper_after_sha256=sha(source),compile_command=argv,
  hardware_voice_producer_or_device_verified=False,retained_large_artifacts=[])
 raw=json.dumps(report,separators=(',',':'))+'\n';assert len(raw.encode())<1024*1024
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(raw)


if __name__=='__main__':main()
