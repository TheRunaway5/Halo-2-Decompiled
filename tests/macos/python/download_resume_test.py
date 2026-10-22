#!/usr/bin/env python3
"""User-authorized MCC transfer into persistent game-input storage.

The enclosing run_tests.py owns the disposable app/SDKs. Downloaded game
inputs stay in the explicitly approved application-support directory. This
test cancels a real transfer, reaps it, signs in again and verifies chunk reuse
and the helper's complete SHA-1 admission. It does not start preparation.
"""
import json
import os
from pathlib import Path
import subprocess
import sys

PROBE = r'''
import json,os,pathlib,signal,sys,time,tkinter
resources,status,output=(pathlib.Path(v).resolve() for v in sys.argv[1:4])
data=pathlib.Path.home()/'Library/Application Support/net.halo2forever'
source_id='e46c404c25077cdd5c064c12'
owned=data/'downloads/steam-mcc'/source_id
sys.path.insert(0,str(resources/'tools'))
os.environ['H2_INSTALLATION_LAYOUT']=json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',data=str(data),resources=str(resources)))
from setup_assets_ui import SourceSelector
report=dict(schema='H2_MACOS_STEAM_DOWNLOAD_RESUME_V1',data_root=str(data),source_owner=str(owned),
 source_id=source_id,payload_retention_authorized=True,preparation_started=False,runs=[])
def storage():
 files=[p for p in owned.rglob('*') if p.is_file() and not p.is_symlink()]
 return dict(files=len(files),logical_bytes=sum(p.stat().st_size for p in files),allocated_bytes=sum(p.stat().st_blocks*512 for p in files))
def progress(state,**values):
 status.write_text(json.dumps(dict(state=state,time=time.time(),source_owner=str(owned),**values))+'\n')
def save():output.write_text(json.dumps(report,indent=2)+'\n')
def interrupted(number,frame):raise KeyboardInterrupt()
signal.signal(signal.SIGTERM,interrupted)
window=tkinter.Tk();selector=SourceSelector(window,data,auto_detect=False)
callbacks=[];completed=[]
window.report_callback_exception=lambda *args:callbacks.append(type(args[1]).__name__)
selector.start=lambda:completed.append(selector.value.get())
selector.prepare.configure(command=lambda:None)
try:
 for phase in ('seed','resume'):
  selector.steam_button.invoke();dialog=selector.steam
  assert dialog is not None
  dialog.window.lift();dialog.window.focus_force()
  # The human approved this exact plan in chat. The observer starts it only
  # after rechecking the source identity and all three size/count bounds.
  dialog.confirm.configure(command=lambda:None,text='Authorized download test')
  row=dict(phase=phase);report['runs'].append(row);save()
  observed={};original=dialog.process._event
  def watch(value):
   original(value)
   if value.get('event')=='progress':observed.update(bytes=value['bytes'],total=value['total'])
  dialog.process._event=watch
  started=False;deadline=time.monotonic()+7200;previous=None;last_status=0
  progress(phase+'_waiting_for_qr')
  while time.monotonic()<deadline:
   window.update();time.sleep(.02)
   if callbacks:raise RuntimeError('GUI callback failed: '+','.join(callbacks))
   if dialog.process.failure:raise RuntimeError('Steam helper failed: '+json.dumps(dict(code=dialog.process.failure,reason=dialog.process.failure_reason,details=dialog.process.policy_details)))
   if not started and dialog.process.plan is not None:
    plan=dialog.process.plan
    assert (plan['source_id'],plan['files'],plan['bytes'],plan['download_bytes'])==(source_id,883,42179329726,28103673648),'Valve content plan changed; download was not confirmed'
    assert plan['entitlement_verified'] and plan['available_free_bytes']>=plan['required_free_bytes']
    row['plan']={k:v for k,v in plan.items() if k not in ('token','event')}
    dialog.download();started=True;row['started']=True;save()
   state=phase+('_qr_ready' if dialog.image is not None else '_'+dialog.process.state)
   now=time.monotonic()
   if state!=previous or now-last_status>=2:
    progress(state,downloaded_uncompressed_bytes=observed.get('bytes',0),remaining_transfer_uncompressed_bytes=observed.get('total',0),verified_reused_bytes=dialog.process.reused_bytes)
    previous=state;last_status=now
   if phase=='seed' and observed.get('bytes',0)>=32*1024*1024:
    child=dialog.process.process;start=time.monotonic();dialog.cancel_button.invoke()
    row.update(cancel_seconds=round(time.monotonic()-start,3),exit_code=child.returncode,
     helper_reaped=child.poll() is not None,reader_reaped=dialog.process.reader is None,
     validated_transferred_uncompressed_bytes=observed['bytes'],storage=storage())
    assert row['helper_reaped'] and row['reader_reaped'] and row['storage']['allocated_bytes']>0
    assert not (owned/'complete.json').exists(),'Cancellation did not precede completion'
    report['partial_preserved']=True;save();progress('seed_cancelled_and_reaped')
    # Retire transient events from the old process before opening the new one.
    while not selector.events.empty():selector.events.get_nowait()
    break
   if phase=='resume' and completed:
    assert dialog.process.process is None and dialog.process.reader is None
    assert dialog.process.reused_bytes>0,'The resumed transfer did not validate and reuse any existing chunks'
    receipt=json.loads((owned/'complete.json').read_text())
    assert receipt['source_id']==source_id and receipt['files']==883 and receipt['bytes']==42179329726
    row.update(verified_reused_bytes=dialog.process.reused_bytes,verified_reused_files=dialog.process.reused_files,
     helper_reaped=True,reader_reaped=True,complete=dialog.process.complete,receipt=receipt,storage=storage())
    report.update(passed=True,all_selected_file_sha1_verified=True,source=completed[-1],retained_reference_inputs=[dict(path=str(owned),kind='user-authorized Steam-owned MCC source',**storage())])
    save();progress('complete',verified_reused_bytes=dialog.process.reused_bytes,storage=storage());break
   if selector.steam is None and not completed:raise RuntimeError('Steam setup was closed before completing this phase')
  else:raise TimeoutError('Steam transfer test deadline expired; partial game data is preserved')
 assert report.get('passed')
 selector.close()
finally:
 if selector.steam is not None:selector.steam.finish()
 report['storage_after_stop']=storage();save()
 try:window.destroy()
 except tkinter.TclError:pass
'''


def main():
    sdk,repo,launcher,graphics,steam,output=map(Path,sys.argv[1:7])
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    app=scratch/'Halo 2 owned-content download.app'
    status=repo/'build/test-results/macos-steam-download/status.json'
    status.parent.mkdir(parents=True,exist_ok=True)
    status.write_text(json.dumps(dict(state='packaging'))+'\n')
    molten=graphics/'sdk/MoltenVK'
    subprocess.run([sys.executable,'-B',str(repo/'tools/macos/package_launcher.py'),
        '--root',str(repo),'--launcher',str(launcher),'--python-sdk',str(sdk),'--steam-sdk',str(steam),
        '--app',str(app),'--arch','arm64','--arch','x86_64',
        '--library',str(molten/'MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib'),
        '--license','MoltenVK='+str(molten/'LICENSE'),'--license','SDL2='+str(graphics/'SDL2-LICENSE.txt')],check=True)
    resources=app/'Contents/Resources'
    env={k:v for k,v in os.environ.items() if not k.startswith(('DYLD_','PYTHON','TCL_','TK_','DOTNET_'))}
    env.update(PATH='/usr/bin:/bin',DOTNET_ROOT='/missing-host-dotnet',DOTNET_EnableDiagnostics='0')
    result=subprocess.run([str(resources/'python/bin/python3'),'-c',PROBE,str(resources),str(status),str(output)],
        cwd=scratch,env=env,capture_output=True,text=True,timeout=7500)
    if result.returncode:
        report=json.loads(output.read_text()) if output.exists() else dict(passed=False)
        report.update(passed=False,exit_code=result.returncode,stderr=result.stderr[-6000:])
        output.write_text(json.dumps(report,indent=2)+'\n')
        raise RuntimeError('Steam transfer test stopped: '+result.stderr[-2000:])


if __name__=='__main__':main()
