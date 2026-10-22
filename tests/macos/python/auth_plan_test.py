#!/usr/bin/env python3
"""Human-approved Steam QR authentication and metadata-only download planning.

Run through run_tests.py with HALO2_MACOS_STEAM_AUTH_PLAN=ON. Credentials and
QR pixels remain in subprocess memory. The confirmation button cannot start
a download in this diagnostic; no game payload is requested.
"""
import json
import os
from pathlib import Path
import subprocess
import sys

PROBE = r'''
import hashlib,json,os,pathlib,sys,time,tkinter
data,resources,status=(pathlib.Path(v).resolve() for v in sys.argv[1:4])
sys.path.insert(0,str(resources/'tools'))
os.environ['H2_INSTALLATION_LAYOUT']=json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',data=str(data),resources=str(resources)))
from setup_assets_ui import SourceSelector
from steam_content import file_plan,profile
def progress(state):
 status.write_text(json.dumps(dict(state=state,time=time.time()))+'\n')
window=tkinter.Tk();selector=SourceSelector(window,data,auto_detect=False)
callbacks=[]
window.report_callback_exception=lambda *args:callbacks.append(type(args[1]).__name__)
try:
 assert hasattr(selector,'steam_button'),'Steam helper unavailable'
 selector.steam_button.invoke();dialog=selector.steam
 assert dialog is not None
 # This human-assisted test ends at the plan. Even an accidental click in
 # the window cannot send a download command to the helper.
 dialog.confirm.configure(command=lambda:None,text='Plan verification only')
 dialog.window.lift();dialog.window.focus_force()
 progress('waiting_for_qr')
 deadline=time.monotonic()+330;previous=None
 while time.monotonic()<deadline:
  window.update();time.sleep(.02)
  if callbacks:raise RuntimeError('GUI callback failed: '+','.join(callbacks))
  if selector.steam is None:raise RuntimeError('Steam setup was closed before its plan')
  if dialog.process.failure:
   progress('failed_'+dialog.process.failure)
   raise RuntimeError('Steam helper failed: '+json.dumps(dict(code=dialog.process.failure,
    reason=dialog.process.failure_reason,details=dialog.process.policy_details)))
  state='qr_ready' if dialog.image is not None else dialog.process.state
  if state!=previous:progress(state);previous=state
  plan=dialog.process.plan
  if plan is not None:
   dialog.confirm.configure(state='disabled')
   owned=data/'downloads/steam-mcc'/plan['source_id']
   inventory=(owned/'selected-files.json').read_bytes()
   assert hashlib.sha256(inventory).hexdigest()==plan['inventory_sha256']
   files=json.loads(inventory);marker=json.loads((owned/'owner.json').read_text());roles=profile()['depots']
   depots=dict(depots=[dict(depot_id=d['depot_id'],manifest_id=d['manifest_id'],role=roles[str(d['depot_id'])]) for d in marker['depots']])
   manifests=[dict(depot_id=d['depot_id'],manifest_id=d['manifest_id'],files=[dict(name=f['path'],bytes=f['bytes'],compressed_bytes=f['download_bytes'],sha1=f['sha1']) for f in files if f['depot_id']==d['depot_id']]) for d in marker['depots']]
   independent=file_plan(depots,manifests)
   assert len(independent['files'])==plan['files'] and independent['bytes']==plan['bytes'] and independent['download_bytes']==plan['download_bytes']
   required={p.casefold() for p in profile()['required_files']}
   result=dict(schema='H2_STEAM_AUTHENTICATED_PLAN_V1',entitlement_verified=plan['entitlement_verified'],
    source_id=plan['source_id'],depots=depots['depots'],files=plan['files'],bytes=plan['bytes'],download_bytes=plan['download_bytes'],
    required_free_bytes=plan['required_free_bytes'],available_free_bytes=plan['available_free_bytes'],
    inventory_sha256=plan['inventory_sha256'],independent_plan_sha256=independent['plan_sha256'],
    required_files=[f for f in files if f['path'].casefold() in required],account_identifier_retained=False,
    qr_or_credentials_retained=False,payload_downloaded=False,download_command_sent=False)
   progress('plan_verified')
   # Give the person scanning the QR a moment to see the result in the UI.
   until=time.monotonic()+3
   while time.monotonic()<until:window.update();time.sleep(.02)
   child=dialog.process.process;dialog.finish()
   result.update(helper_reaped=child.poll() is not None,reader_reaped=dialog.process.reader is None,passed=True)
   selector.close();print(json.dumps(result),flush=True);break
 else:raise TimeoutError('Waiting for Steam approval or manifest planning expired')
finally:
 if selector.steam is not None:selector.steam.finish()
 try:window.destroy()
 except tkinter.TclError:pass
'''


def main():
    sdk, repo, launcher, graphics, steam, output = map(Path, sys.argv[1:7])
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    app = scratch/'Halo 2 Steam sign-in.app'
    report = dict(schema='H2_MACOS_STEAM_HUMAN_PLAN_TEST_V1', scratch=str(scratch),
                  payload_downloaded=False, retained_large_artifacts=[])
    status = repo/'build/test-results/macos-steam-auth/status.json'
    status.parent.mkdir(parents=True, exist_ok=True)
    status.write_text(json.dumps(dict(state='packaging'))+'\n')
    try:
        molten = graphics/'sdk/MoltenVK'
        subprocess.run([sys.executable, '-B', str(repo/'tools/macos/package_launcher.py'),
            '--root', str(repo), '--launcher', str(launcher), '--python-sdk', str(sdk), '--steam-sdk', str(steam),
            '--app', str(app), '--arch', 'arm64', '--arch', 'x86_64',
            '--library', str(molten/'MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib'),
            '--license', 'MoltenVK='+str(molten/'LICENSE'), '--license', 'SDL2='+str(graphics/'SDL2-LICENSE.txt')],check=True)
        resources=app/'Contents/Resources'
        env={k:v for k,v in os.environ.items() if not k.startswith(('DYLD_','PYTHON','TCL_','TK_','DOTNET_'))}
        env.update(PATH='/usr/bin:/bin',DOTNET_ROOT='/missing-host-dotnet',DOTNET_EnableDiagnostics='0')
        result=subprocess.run([str(resources/'python/bin/python3'),'-c',PROBE,str(scratch/'data'),str(resources),str(status)],
            cwd=scratch,env=env,capture_output=True,text=True,timeout=360)
        report.update(exit_code=result.returncode,stderr=result.stderr[-4000:])
        if result.returncode:raise RuntimeError('Steam plan check did not finish: '+result.stderr[-2000:])
        report['result']=json.loads(result.stdout)
        report['passed']=report['result']['passed']
    finally:
        output.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report.get('passed') else 1


if __name__=='__main__':raise SystemExit(main())
