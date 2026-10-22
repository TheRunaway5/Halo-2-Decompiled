#!/usr/bin/env python3
"""Run actual bundled Python extensions and native Tk with its SDK hidden."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

PROBE = r'''
import bz2, compression.zstd, ctypes, hashlib, io, json, lzma, os, pathlib, platform
import sqlite3, ssl, sys, tkinter, urllib.request, zlib
from PIL import Image, features
runtime=pathlib.Path(sys.argv[1]).resolve()
assert platform.machine()==sys.argv[2], platform.machine()
assert pathlib.Path(sys.base_prefix).resolve()==runtime,(sys.base_prefix,runtime)
assert pathlib.Path(sys.executable).resolve().is_relative_to(runtime),sys.executable
assert sys.flags.no_user_site,sys.flags
assert sys.dont_write_bytecode
assert all(pathlib.Path(p).resolve().is_relative_to(runtime) for p in sys.path if p),sys.path
raw=b'actual bundled compression and sqlite test'*30
for codec in [bz2,lzma,zlib,compression.zstd]: assert codec.decompress(codec.compress(raw))==raw
with sqlite3.connect(':memory:') as db: assert db.execute('select 6*7').fetchone()[0]==42
image=Image.new('RGBA',(13,17),(9,23,47,255)); image_tests={}
for format in ['PNG','DDS','TIFF']:
 stream=io.BytesIO(); image.save(stream,format=format); stream.seek(0)
 decoded=Image.open(stream).convert('RGBA'); assert decoded.tobytes()==image.tobytes()
 image_tests[format]=hashlib.sha256(stream.getvalue()).hexdigest()
assert features.check_module('freetype2')
context=ssl.create_default_context()
assert context.verify_mode==ssl.CERT_REQUIRED and context.check_hostname
assert context.cert_store_stats()['x509_ca']>0
with urllib.request.urlopen('https://pypi.org/pypi/certifi/json',context=context,timeout=30) as response:
 assert response.status==200 and json.load(response)['info']['name']=='certifi'
window=tkinter.Tk(); window.title('Halo 2 bundled Python verification')
label=tkinter.Label(window,text='Universal Python and private Tk'); label.pack()
window.update_idletasks(); window.update()
assert window.winfo_exists() and window.winfo_width()>1
tcl=str(window.tk.call('info','patchlevel')); tk=str(window.tk.call('package','provide','Tk'))
tcl_path=pathlib.Path(str(window.tk.call('info','library'))).resolve()
tk_path=pathlib.Path(str(window.tk.call('set','tk_library'))).resolve()
assert tcl_path.is_relative_to(runtime) and tk_path.is_relative_to(runtime),(tcl_path,tk_path)
window.destroy()
resources=pathlib.Path(sys.argv[4]).resolve(); data=pathlib.Path(sys.argv[3]).resolve()
sys.path.insert(0,str(resources/'tools'))
os.environ['H2_INSTALLATION_LAYOUT']=json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',data=str(data),resources=str(resources)))
import setup_assets, mcc_onboarding, mcc_loading_presentation
from setup_assets_ui import SourceSelector
from launcher_ui import LauncherHome
setup_window=tkinter.Tk()
selector=SourceSelector(setup_window,data,auto_detect=False)
setup_window.update_idletasks();setup_window.update()
assert selector.entry.winfo_exists() and selector.browse_button.winfo_exists()
selector.close()
home_window=tkinter.Tk()
home=LauncherHome(home_window,data,launcher_executable=resources.parent/'MacOS/Halo2-Launcher')
home_window.update_idletasks();home_window.update()
assert home.precompiled and home.window.winfo_exists()
home.close()
assert not data.exists(),'Opening setup/home or inspecting the launcher wrote user data'
dyld=ctypes.CDLL(None)
dyld._dyld_image_count.restype=ctypes.c_uint32
dyld._dyld_get_image_name.argtypes=[ctypes.c_uint32];dyld._dyld_get_image_name.restype=ctypes.c_char_p
images=[os.fsdecode(dyld._dyld_get_image_name(i)) for i in range(dyld._dyld_image_count())]
external=[p for p in images if not p.startswith(('/System/Library/','/usr/lib/')) and not pathlib.Path(p).resolve().is_relative_to(runtime)]
assert not external,external
print(json.dumps(dict(arch=platform.machine(),python=sys.version,prefix=sys.base_prefix,executable=sys.executable,
 ssl=ssl.OPENSSL_VERSION,certificates=context.cert_store_stats(),pillow=Image.__version__,image_tests=image_tests,
 tcl=tcl,tk=tk,tcl_path=str(tcl_path),tk_path=str(tk_path),images=images,
 actual_setup_and_home_windows=True,passed=True)))
'''

STEAM_PROBE = r'''
import json,os,pathlib,platform,sys,time,tkinter
data=pathlib.Path(sys.argv[1]).resolve();resources=pathlib.Path(sys.argv[2]).resolve()
sys.path.insert(0,str(resources/'tools'))
os.environ['H2_INSTALLATION_LAYOUT']=json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',data=str(data),resources=str(resources)))
from setup_assets_ui import SourceSelector
window=tkinter.Tk();selector=SourceSelector(window,data,auto_detect=False)
callbacks=[]
window.report_callback_exception=lambda *args:callbacks.append(str(args[1]))
child=None
try:
 assert hasattr(selector,'steam_button'),'Packaged Steam helper was not offered'
 selector.steam_button.invoke();dialog=selector.steam
 assert dialog is not None
 child=dialog.process.process
 deadline=time.monotonic()+80
 while dialog.image is None and time.monotonic()<deadline:
  window.update();time.sleep(.01)
  if dialog.process.failure:raise RuntimeError('Packaged Steam sign-in challenge failed')
 assert dialog.image is not None,'Packaged QR challenge was not rendered'
 dimensions=(dialog.image.width(),dialog.image.height())
 assert str(dialog.confirm.cget('state'))=='disabled'
 assert selector.working and str(selector.entry.cget('state'))=='disabled'
 start=time.monotonic();dialog.cancel_button.invoke();elapsed=time.monotonic()-start
 assert child.poll()==2 and dialog.process.process is None and dialog.process.reader is None
 assert selector.steam is None and dialog.image is None and not selector.working
 assert str(selector.entry.cget('state'))=='normal' and selector.value.get()==''
 assert not list(data.rglob('*')),'QR session wrote account or download state'
 assert not callbacks,callbacks
 selector.close()
 print(json.dumps(dict(architecture=platform.machine(),actual_packaged_qr_window=True,
  qr_dimensions=dimensions,download_confirmation_disabled=True,account_authenticated=False,
  payload_downloaded=False,helper_reaped=True,reader_reaped=True,cancel_seconds=round(elapsed,3),
  manual_import_restored=True,passed=True)))
finally:
 if selector.steam is not None:selector.steam.finish()
 try:window.destroy()
 except tkinter.TclError:pass
'''

LOADING_PROBE = r'''
import hashlib,json,os,pathlib,platform,subprocess,sys
resources,work=(pathlib.Path(v).resolve() for v in sys.argv[1:3])
assert platform.machine()==sys.argv[3]
sys.path.insert(0,str(resources/'tools'))
data=work/'unused-user-data'
os.environ['H2_INSTALLATION_LAYOUT']=json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',data=str(data),resources=str(resources)))
from audit_rendering import FormatError
from mcc_loading_presentation import (CODEBOOK,MEDIA_SHA,V3_FRAMES,_v3_pcm,converter_dependencies,preflight_v3,read_media)
owner=converter_dependencies(data)
source=pathlib.Path(sys.argv[4]) if len(sys.argv)>4 else None
if source:
 preflight=preflight_v3(data,source)
 payload,_=read_media(source)
 assert hashlib.sha256(payload).hexdigest()==MEDIA_SHA
else:
 payload=b'invalid fixture';preflight=None
work.mkdir();wem=work/'source.wem';wem.write_bytes(payload);wav=work/'output.wav'
environment={k:v for k,v in os.environ.items() if not k.startswith(('LD_','DYLD_')) and k!='GLIBC_TUNABLES'}
command=['/usr/bin/arch','-'+platform.machine(),'/usr/bin/env','DYLD_PRINT_LIBRARIES=1',
 str(resources/owner['converter']['path']),str(wem),str(wav),str(resources/CODEBOOK),str(V3_FRAMES)]
result=subprocess.run(command,env=environment,capture_output=True,text=True,timeout=120)
loaded=[line for line in result.stderr.splitlines() if 'libvorbis' in line or 'libogg' in line]
assert all(any(str(resources/('macos/loading-converter/lib/'+name)) in line for line in loaded)
 for name in ('libogg.0.dylib','libvorbis.0.dylib'))
report=dict(arch=platform.machine(),owner=owner,source_preflight=preflight,loaded_libraries=loaded,
 helper_exit=result.returncode,user_data_unchanged=not data.exists())
if source:
 assert result.returncode==0
 pcm=wav.read_bytes();report['pcm_sha256']=hashlib.sha256(pcm[44:]).hexdigest()
 try:
  _v3_pcm(pcm);report['canonical_pcm']=True
 except FormatError as error:
  assert 'PCM differs' in str(error)
  report.update(canonical_pcm=False,pcm_gate_error=str(error))
else:
 assert result.returncode!=0 and not wav.exists()
 report['invalid_media_rejected']=True
assert not data.exists()
report['passed']=True
print(json.dumps(report))
'''


def hashes(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file() and not p.is_symlink()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('sdk', 'repo', 'launcher', 'graphics_sdk', 'output'):
        parser.add_argument(name, type=Path)
    parser.add_argument('arches', nargs='+', choices=('arm64', 'x86_64'))
    parser.add_argument('--steam-sdk', type=Path)
    parser.add_argument('--loading-converter', type=Path)
    parser.add_argument('--vorbis-sdk', type=Path)
    parser.add_argument('--loading-source', type=Path)
    args = parser.parse_args()
    sdk, repo, launcher, graphics_sdk = (getattr(args, name).resolve(strict=True)
        for name in ('sdk', 'repo', 'launcher', 'graphics_sdk'))
    output, arches = args.output, args.arches
    report = dict(schema='H2_MACOS_PYTHON_RELOCATION_V1', runs=[], retained_large_artifacts=[])
    try:
        with tempfile.TemporaryDirectory(prefix='macos-python-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
            work = Path(directory)
            relocated = work / 'Relocated launcher Ω with spaces.app'
            molten = graphics_sdk / 'sdk/MoltenVK'
            subprocess.run([sys.executable, '-B', str(repo / 'tools/macos/package_launcher.py'),
                '--root', str(repo), '--launcher', str(launcher), '--python-sdk', str(sdk), '--app', str(relocated),
                *[value for arch in arches for value in ('--arch', arch)],
                '--library', str(molten / 'MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib'),
                '--license', 'MoltenVK=' + str(molten / 'LICENSE'),
                '--license', 'SDL2=' + str(graphics_sdk / 'SDL2-LICENSE.txt'),
                *(['--steam-sdk', str(args.steam_sdk)] if args.steam_sdk else []),
                *(['--loading-converter', str(args.loading_converter), '--vorbis-sdk', str(args.vorbis_sdk)]
                  if args.loading_converter else [])], check=True)
            resources = relocated / 'Contents/Resources'
            runtime = resources / 'python'
            # The script itself is an explicit test input; execute its source
            # with -c so sys.path has no unrelated script or checkout entry.
            before = hashes(relocated)
            hidden = work / 'original-dependencies'
            sdk.parent.rename(hidden)
            hidden_graphics = work / 'original-graphics'
            graphics_sdk.rename(hidden_graphics)
            try:
                environment = {k: v for k, v in os.environ.items() if not k.startswith(('DYLD_', 'PYTHON', 'TCL_', 'TK_', 'SSL_', 'OPENSSL_'))}
                environment.update(PATH='/usr/bin:/bin', PYTHONHOME='/missing-host-python',
                    PYTHONPATH='/missing-host-packages', PYTHONUSERBASE='/missing-user-packages',
                    TCL_LIBRARY='/missing-host-tcl', TK_LIBRARY='/missing-host-tk')
                for arch in arches:
                    data = work / ('User data ' + arch)
                    launcher_environment = dict(environment, H2_USER_DATA_DIR=str(data),
                        SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
                    info = subprocess.run(['/usr/bin/arch', '-' + arch,
                        str(relocated / 'Contents/MacOS/Halo2-Launcher'), '--launcher-info'],
                        cwd=work, env=launcher_environment, capture_output=True, text=True, timeout=20)
                    report.setdefault('launchers', []).append(dict(arch=arch, exit=info.returncode,
                        stdout=info.stdout[-8192:], stderr=info.stderr[-8192:]))
                    if info.returncode or 'Project: ' + str(data) + '\n' not in info.stdout:
                        raise RuntimeError('Actual packaged launcher failed: ' + info.stderr)
                    argv = ['/usr/bin/arch', '-' + arch, str(runtime / 'bin/python3'), '-c', PROBE,
                        str(runtime), arch, str(data), str(resources)]
                    result = subprocess.run(argv, cwd=runtime, env=environment, text=True,
                        capture_output=True, timeout=60)
                    row = dict(arch=arch, command=argv[:4] + ['<PROBE source in relocation_test.py>', *argv[5:]],
                        exit=result.returncode, stdout=result.stdout[-32768:], stderr=result.stderr[-8192:])
                    report['runs'].append(row)
                    if result.returncode:
                        raise RuntimeError('Relocated Python runtime failed: ' + result.stderr[-8192:])
                    row['result'] = json.loads(result.stdout)
                    if args.loading_converter:
                        argv = ['/usr/bin/arch', '-' + arch, str(runtime / 'bin/python3'), '-c', LOADING_PROBE,
                            str(resources), str(work / ('Loading ' + arch)), arch,
                            *([str(args.loading_source)] if args.loading_source else [])]
                        result = subprocess.run(argv, cwd=runtime, env=environment,
                            capture_output=True, text=True, timeout=150)
                        row['loading_converter'] = dict(exit=result.returncode, stderr=result.stderr[-8192:])
                        if result.returncode:
                            raise RuntimeError('Packaged loading converter failed: ' + result.stderr[-8192:])
                        row['loading_converter']['result'] = json.loads(result.stdout)
                    if args.steam_sdk:
                        argv = ['/usr/bin/arch', '-' + arch, str(runtime / 'bin/python3'), '-c', STEAM_PROBE,
                            str(data), str(resources)]
                        steam_environment = dict(environment, DOTNET_ROOT='/missing-host-dotnet',
                            DOTNET_MULTILEVEL_LOOKUP='0', DOTNET_EnableDiagnostics='0')
                        result = subprocess.run(argv, cwd=runtime, env=steam_environment,
                            capture_output=True, text=True, timeout=100)
                        row['steam_ui'] = dict(exit=result.returncode, stderr=result.stderr[-8192:])
                        if result.returncode:
                            raise RuntimeError('Packaged Steam UI failed: ' + result.stderr[-8192:])
                        row['steam_ui']['result'] = json.loads(result.stdout)
                if hashes(relocated) != before:
                    raise RuntimeError('Python changed its runtime files during use')
                subprocess.run(['codesign', '--verify', '--deep', '--strict',
                    str(relocated)], check=True)
                report.update(passed=True, unchanged_runtime=True, images=len(json.loads((hidden / 'python/manifest.json').read_text())['images']))
            finally:
                hidden_graphics.rename(graphics_sdk)
                hidden.rename(sdk.parent)
    finally:
        output.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
