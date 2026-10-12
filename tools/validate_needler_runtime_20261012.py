#!/usr/bin/env python3
"""Actual SDL/Vulkan weapon checks; explicit diagnostic placement, dummy audio.

Requires owned Lockout/shared caches and an owned gameplay update image. Does
not establish original framebuffer, physical audible output or full game parity.
"""
import argparse,json,os,subprocess
from pathlib import Path

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--build',type=Path,required=True)
 p.add_argument('--xbe',type=Path,required=True)
 p.add_argument('--output',type=Path,required=True)
 p.add_argument('--hud',type=Path,required=True)
 p.add_argument('--resume',action='store_true',help='Keep completed passing cases after an interrupted runner')
 p.add_argument('--cases',nargs='+',default=['needler-ammo','needler-fire','sword-ready','sword-strike','sword-world','smg-dual'])
 a=p.parse_args();root=Path.cwd();a.output.mkdir(parents=True,exist_ok=True)
 env=os.environ.copy();env.update(SDL_AUDIODRIVER='dummy',HALO2_ANIMATION_VALIDATION='1',
  HALO2_TEST_HUD_UI_ASSETS=str(a.hud.resolve()),
  VK_LAYER_PATH=str(root/'.tools/validation/usr/share/vulkan/explicit_layer.d'),
  LD_LIBRARY_PATH=str(root/'.tools/validation/usr/lib'))
 prefix=['xvfb-run','-a','-s','-screen 0 1024x768x24']
 resources=[str(root/'assets/disc/maps/lockout.map'),str(root/'assets/disc/maps'),str(a.xbe.resolve())]
 results_path=a.output/'results.json'
 results=json.loads(results_path.read_text()) if a.resume and results_path.exists() else []
 assert all(r['exit_code']==0 and r['validation_errors']==0 for r in results)
 done={r['case']for r in results}
 for case in a.cases:
  if case in done:continue
  interrupted=a.output/(case+'.log')
  if a.resume and interrupted.exists():interrupted.rename(a.output/(case+'-interrupted.log'))
  if case=='smg-dual':cmd=prefix+[str(a.build.resolve()/'native_weapon_runtime_tests')]+resources
  else:
   assert case in ('needler-ammo','needler-fire','sword-ready','sword-strike','sword-world')
   weapon,phase=case.split('-');capture=str((a.output/(case+'.bmp')).resolve()) if phase in ('fire','world') else ''
   cmd=prefix+[str(a.build.resolve()/'native_covenant_weapon_runtime_tests')]+resources+[weapon,capture]
   if phase!='strike':cmd.append(phase)
  log=a.output/(case+'.log')
  with log.open('w')as f:r=subprocess.run(cmd,cwd=root,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  text=log.read_text();errors=text.count('Validation Error:')
  row=dict(case=case,command=cmd,exit_code=r.returncode,validation_errors=errors,log=str(log.resolve()))
  results.append(row);print(json.dumps(row),flush=True)
  (a.output/'results.json').write_text(json.dumps(results,indent=2)+'\n')
  if r.returncode or errors:return 1
 return 0
if __name__=='__main__':raise SystemExit(main())
