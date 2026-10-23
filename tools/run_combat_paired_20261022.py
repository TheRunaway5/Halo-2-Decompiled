#!/usr/bin/env python3
"""Same-source-frame SDL/Vulkan counterfactual captures, inside run_test.py.

The original query ring is issued once. Repeat and suppressed captures borrow
identical completed CPU draw inputs; the existing renderer-owned postprocess
checkpoint is restored between draws. No simulation or GPU result is supplied.
"""
import hashlib,json,os,re,shutil,subprocess,sys,tempfile
from pathlib import Path
from PIL import Image,ImageChops
from validate_weapon_runtime_tu15 import private_display
from run_combat_presentation_audit_20261022 import graphics_cache,keep_log

def main():
 exe,mode,map_,output,subject,*extra=sys.argv[1:]
 assert mode in ('zoom','vehicle','fire')
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=Path(output).resolve();report.parent.mkdir(parents=True,exist_ok=True)
 scratch=Path(tempfile.mkdtemp(prefix='paired-'+report.stem+'-',dir=owner))
 detail=report.with_name(report.stem+'-actual.json');log=scratch/'runtime.log';capture=scratch/'baseline.ppm'
 glow=mode!='zoom'
 env=dict(os.environ,HALO2_COMBAT_AUDIT='1',HALO2_COMBAT_AUDIT_CAPTURE=str(capture),HALO2_COMBAT_AUDIT_PAIR_DIRECTORY=str(scratch),XDG_CACHE_HOME=graphics_cache(scratch),SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy')
 for name in ['HALO2_COMBAT_AUDIT_HIDE_ARMS','HALO2_COMBAT_AUDIT_HIDE_FOREGROUND','HALO2_COMBAT_AUDIT_HIDE_GLOW']:env.pop(name,None)
 if glow:env['HALO2_COMBAT_AUDIT_PAIR_GLOW']='1'
 if mode=='vehicle':
  env.update(HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=subject,HALO2_VEHICLE_HUD_GPU_NAME=subject,HALO2_TEST_GHOST_PROJECTILE_RENDER='1',HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=str(detail),HALO2_COMBAT_AUDIT_PIXEL_CAPTURE='1')
  command=[exe,map_,str(Path(map_).parent),'native-mcc',str(capture)]
 else:command=[exe,mode,map_,str(detail),subject,*(extra or ['0'])]
 with private_display(scratch) as display,log.open('w') as stream:
  env['DISPLAY']=display
  try:code=subprocess.run(command,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=360).returncode
  except subprocess.TimeoutExpired:code=124
 keep_log(log,report.with_suffix('.log'))
 result={'audit_completed':False,'exit_code':code,'scope':__doc__,'subject':subject,'command':command,'executable_sha256':hashlib.sha256(Path(exe).read_bytes()).hexdigest()}
 if code or not all((scratch/(n+'.ppm')).is_file() for n in ['baseline','repeat','control']):
  result['reason']='Actual runtime did not produce all paired captures';report.write_text(json.dumps(result,indent=2)+'\n');return code or 2
 data=json.loads(detail.read_text());images=[];bounds=None
 pilot=int(os.environ.get('HALO2_COMBAT_AUDIT_PILOT_VIEW','0'))
 rectangles=re.findall(r'\[COMBAT-AUDIT\] view='+str(pilot)+r' .* viewport=([0-9.,-]+)',log.read_text())
 assert rectangles
 left,top,width,height=map(float,rectangles[-1].split(','));bounds=tuple(round(v) for v in [left,top,left+width,top+height])
 for name in ['baseline','repeat','control']:
  with Image.open(scratch/(name+'.ppm')) as im:images.append(im.convert('RGB').crop(bounds))
 baseline,repeat,control=images;equal=lambda a,b:sum(x!=y for x,y in zip(a.getdata(),b.getdata()))
 noise=equal(baseline,repeat);difference=equal(baseline,control);diff=ImageChops.difference(baseline,control)
 brighter=sum(sum(a)>sum(b) for a,b in zip(baseline.getdata(),control.getdata()))
 darker=sum(sum(a)<sum(b) for a,b in zip(baseline.getdata(),control.getdata()))
 result.update(audit_completed=True,same_source_frame=True,paired_frame_query_issuance_count=1,repeat_changed_pixels=noise,changed_pixels=difference,difference_bounds=diff.getbbox(),pilot_view=pilot,view_rectangle=bounds,original_Xbox_pixels_compared=False)
 result.update(brighter_pixels=brighter,darker_pixels=darker)
 if mode=='zoom':
  selector=data['final_zoom'];assert selector==data['views'][-1]['zoom']
  result.update(actual_zoom=selector,foreground_invisible=difference==0)
  passed=noise==0 and (difference==0 if selector!=-1 else difference>0)
 else:
  actual=data['cases'][0] if mode=='vehicle' else data
  result['projectile_render']=actual.get('projectile_render',{k:actual[k] for k in ['live_projectiles','attached_projectiles','raw_source_attached_projectiles','attached_model_projectiles','glow_projectiles','glow_draws'] if k in actual})
  # The original non-fuzzy light program contributes RGB with final alpha0.
  # A zero/unuploaded program drew dark quads and passed an absolute delta.
  passed=noise==0 and brighter>0 and darker==0
  if mode=='fire' and subject=='needler':
   attached=set(actual['raw_source_attached_projectiles'])
   models=bool(attached) and attached<=set(actual['attached_projectiles']) and attached<=set(actual['attached_model_projectiles'])
   glows=bool(attached) and attached<=set(actual['glow_projectiles'])
   result.update(attached_models_presented=models,attached_glows_presented=glows);passed&=models and glows
 result['passed']=bool(passed)
 for name,im in [('baseline',baseline),('control',control),('difference',diff)]:im.save(scratch/(name+'.png'))
 destination=os.environ.get('HALO2_COMBAT_AUDIT_REFERENCE_DIR')
 if destination and noise==0:
  paths=[scratch/(n+'.png') for n in ['baseline','control','difference']]
  assert max(p.stat().st_size for p in paths)<=1024*1024 and sum(p.stat().st_size for p in paths)<=2*1024*1024
  destination=Path(destination);destination.mkdir(parents=True,exist_ok=True);result['retained_reference_images']=[]
  for p in paths:
   target=destination/(report.stem+'-'+p.name);shutil.copyfile(p,target)
   result['retained_reference_images'].append({'path':str(target),'bytes':target.stat().st_size,'sha256':hashlib.sha256(target.read_bytes()).hexdigest()})
 report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
