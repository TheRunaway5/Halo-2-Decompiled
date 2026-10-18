#!/usr/bin/env python3
"""Observe one original pose call in a disposable private session.

Run through tools/run_test.py. Working disks, RAM dumps, display and emulator
stay in owned scratch. Required RAM contents share compressed reference chunks;
retain only those chunks and small provenance/CPU/pose evidence.
"""
import argparse
import fcntl
import importlib.util
import json
import os
from pathlib import Path
import resource
import select
import shutil
import socket
import subprocess
import tomllib
from capture_xbox_tu15_grenade_reference import ROOT,qmp,sha,wait_for,write_json,x11_window
from xbox_tu15_reference_storage import pins,prepare_private_disk,verify_reference_disk

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--evidence',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--snapshot',default='tu15-needler-idle')
    parser.add_argument('--unit',type=lambda v:int(v,0),default=0xe2820013)
    parser.add_argument('--warm-views',type=int,default=2)
    parser.add_argument('--normal-fire',action='store_true',help='Hold the authored right trigger and observe the first real ammunition decrement before the pose call')
    parser.add_argument('--phase',choices=('models','update'),default='models')
    parser.add_argument('--weapon',type=lambda v:int(v,0),default=0xe2830014)
    parser.add_argument('--definition',type=lambda v:int(v,0),default=0xe34b01d6)
    args=parser.parse_args();assert 0<=args.warm_views<=5
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);working=scratch/'session';working.mkdir()
    output=args.output.absolute();output.mkdir(parents=True,exist_ok=False)
    session=args.session.absolute();evidence=json.loads(args.evidence.read_text())
    assert evidence['passed']and evidence['full_live_TU15_code_authenticated']
    source_pins=pins(evidence);disk=session/'session-hdd.qcow2'
    original_sha=prepare_private_disk(session,working,evidence)
    config=(session/'xemu.toml').read_text();settings=tomllib.loads(config)
    image=Path(settings['sys']['files']['dvd_path']);assert sha(image)==source_pins[str(image)]
    shutil.copyfile(session/'eeprom.bin',working/'eeprom.bin')
    for name in ('session-hdd.qcow2','eeprom.bin'):config=config.replace(str(session/name),str(working/name))
    assert '[display]\n'not in config
    config=config.replace('[display.vulkan]',"[display]\nrenderer = 'OPENGL'\n\n[display.vulkan]")
    (working/'xemu.toml').write_text(config);shutil.copyfile(working/'xemu.toml',output/'launch-config.toml')
    processes=[];qp=None;success=False
    try:
        read_fd,write_fd=os.pipe()
        with (working/'xvfb.log').open('w')as log:
            display_process=subprocess.Popen(['Xvfb','-displayfd',str(write_fd),'-screen','0','640x480x24','-nolisten','tcp','-noreset'],pass_fds=(write_fd,),stdout=log,stderr=subprocess.STDOUT)
        processes.append(display_process);os.close(write_fd)
        with os.fdopen(read_fd)as number:
            assert select.select([number],[],[],10)[0]
            # Xorg may write the digits and newline separately. Read the complete
            # protocol line before closing; an early close kills Xvfb with EPIPE.
            line=number.readline(64);assert line.endswith('\n'),repr(line)
            display=':'+line.strip()
        assert display[1:].isdigit()
        with socket.socket()as reservation:reservation.bind(('127.0.0.1',0));port=reservation.getsockname()[1]
        env=dict(os.environ,DISPLAY=display,LIBGL_ALWAYS_SOFTWARE='1',__GLX_VENDOR_LIBRARY_NAME='mesa',
            __EGL_VENDOR_LIBRARY_FILENAMES='/usr/share/glvnd/egl_vendor.d/50_mesa.json',SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
            XDG_CONFIG_HOME=str(working/'profile'),XDG_DATA_HOME=str(working/'profile'),XDG_CACHE_HOME=str(working/'cache'))
        command=[str(ROOT/'.tools/xemu-app/squashfs-root/usr/bin/xemu'),'-config_path',str(working/'xemu.toml'),'-S',
            '-qmp','unix:qmp.sock,server=on,wait=off','-gdb',f'tcp:127.0.0.1:{port}','-nic','none','-no-reboot','-no-shutdown']
        with (working/'xemu.log').open('w')as log:xemu=subprocess.Popen(command,cwd=working,env=env,stdout=log,stderr=subprocess.STDOUT)
        processes.append(xemu)
        qp=Path(f'/proc/{xemu.pid}/cwd/qmp.sock');wait_for(qp.exists,15,[xemu])
        def ready():
            tree=qmp(qp,[dict(execute='human-monitor-command',arguments={'command-line':'info qtree'})])[-1]['return']
            return 'usb-xbox-gamepad'in tree and 'usb-hub'in tree
        wait_for(ready,15,[xemu])
        restored=qmp(qp,[dict(execute='human-monitor-command',arguments={'command-line':'loadvm '+args.snapshot}),dict(execute='stop')])
        assert not restored[1]['return'].strip();write_json(output/'restore.json',restored)
        commands=qmp(qp,[dict(execute='query-commands')])[-1]['return'];assert any(c['name']=='query-memory-size-summary'for c in commands)
        memory=qmp(qp,[dict(execute='query-memory-size-summary')])[-1]['return'];ram=memory['base-memory'];assert 0<ram<=128*1024*1024 and not memory['plugged-memory']
        parameters=dict(output=str(working),qmp=str(qp),ram_bytes=ram,unit=args.unit,warm_views=args.warm_views,
            normal_fire=args.normal_fire,weapon=args.weapon,definition=args.definition,phase=args.phase)
        parameter_path=working/'parameters.json';write_json(parameter_path,parameters)
        observer=ROOT/'tools/read_xbox_tu15_first_person_case.py'
        launch=dict(command=command,pid=xemu.pid,xvfb_pid=display_process.pid,display=display,gdb_port=port,
            snapshot=args.snapshot,original_session=str(session),original_session_sha256=original_sha,
            reference_image_sha256=sha(image),memory_summary=memory,core_limit=resource.getrlimit(resource.RLIMIT_CORE)[0],
            tool_sha256=sha(Path(__file__)),observer_sha256=sha(observer),runner_scratch=str(scratch),
            guest_code_patches=[],guest_memory_writes=[],guest_register_writes=[],GPU_capture=False)
        assert launch['core_limit']==0;write_json(output/'launch.json',launch)
        script=working/'capture.gdb'
        script.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 20
target remote 127.0.0.1:{port}
python
import os,sys,runpy
sys.path.insert(0,{str(ROOT/'tools')!r})
os.environ['HALO2_FP_CASE_CONFIG']={str(parameter_path)!r}
runpy.run_path({str(observer)!r},run_name='__main__')
end
delete
detach
quit
''')
        with (working/'capture.log').open('w')as log:
            observer_process=subprocess.Popen(['gdb','-q','-nx','-batch','-x',str(script)],stdout=log,stderr=subprocess.STDOUT)
            processes.append(observer_process)
            if args.normal_fire:
                assert settings['input']['keyboard_controller_scancode_map']['rtrigger']==8
                wait_for((working/'NORMAL_FIRE_READY').exists,60,[observer_process,xemu])
                def complete_capture():
                    assert observer_process.wait(timeout=90)==0
                x11_window(display,press=True,key='e',duration=complete_capture)
            else:assert observer_process.wait(timeout=150)==0
        for name in ('before-cpu.json','after-cpu.json','call.json','capture.gdb','parameters.json'):
            shutil.copyfile(working/name,output/name)
        for path in working.glob('model-*-nodes.bin'):shutil.copyfile(path,output/path.name)
        for path in working.glob('*-first-person-state.bin'):shutil.copyfile(path,output/path.name)
        base=ROOT/'analysis/weapons-tu15-goal-20261013'
        spec=importlib.util.spec_from_file_location('retained_content',base/'archive-content-retention-v1.py')
        store=importlib.util.module_from_spec(spec);spec.loader.exec_module(store)
        records={};verified=set()
        with (store.STORE/'pack.lock').open('a')as lock:
            fcntl.flock(lock,fcntl.LOCK_EX)
            for label in ('before','after'):
                path=working/(label+'-physical.bin');assert path.stat().st_size==ram
                digest=sha(path)
                with path.open('rb')as source:store.store_file(source,digest,ram,verified)
                records[label]=dict(raw_bytes=ram,raw_sha256=digest,content_recipe=str(store.content_path('files',digest,'.json')),
                    content_recipe_sha256=sha(store.content_path('files',digest,'.json')),exact_reconstruction_verified=True)
        write_json(output/'physical-memory-retention.json',dict(files=records,reason='Required original live first-person API entry/exit inputs; exact shared compressed contents retained'))
        shutil.copyfile(observer,output/'observer-source.py');shutil.copyfile(Path(__file__),output/'capture-source.py')
        shutil.copyfile(ROOT/'tools/capture_xbox_tu15_grenade_reference.py',output/'session-helper-source.py')
        authority=ROOT/'analysis/weapons-tu15-goal-20261013/first-person-original-call-v1'
        source_pins=json.loads((authority/'x87-tag-authority.json').read_text())
        for name in ('xemu-gdbstub.c','xemu-cpu-dump.c'):
            path=authority/name;assert sha(path)==source_pins['source_files_sha256'][str(path.relative_to(ROOT))]
            shutil.copyfile(path,output/name)
        write_json(output/'x87-tag-authority.json',dict(
            pinned_source_authority=str(authority/'x87-tag-authority.json'),
            pinned_source_authority_sha256=sha(authority/'x87-tag-authority.json'),
            source_files_sha256={name:sha(output/name)for name in ('xemu-gdbstub.c','xemu-cpu-dump.c')},
            raw_capture_sha256={label:sha(output/(label+'-cpu.json'))for label in ('before','after')},
            explanation=source_pins['explanation'],reference_commit=source_pins['reference_commit']))
        success=True
    finally:
        if qp and qp.exists():
            try:qmp(qp,[dict(execute='quit')])
            except (OSError,RuntimeError):pass
        for process in reversed(processes):
            if process.poll()is None:
                try:process.wait(timeout=8)
                except subprocess.TimeoutExpired:process.terminate()
            if process.poll()is None:
                try:process.wait(timeout=3)
                except subprocess.TimeoutExpired:process.kill();process.wait()
        for name in ('xemu.log','xvfb.log','capture.log'):
            path=working/name
            if path.exists():(output/name).write_bytes(path.read_bytes()[-65536:])
        unchanged=verify_reference_disk(session,evidence)==original_sha;assert unchanged
        write_json(output/'cleanup.json',dict(capture_completed=success,all_processes_exited=all(p.poll()is not None for p in processes),
            original_session_unchanged=unchanged,working_disk=str(working/disk.name),runner_scratch=str(scratch),
            working_files_removed_by_runner=True,physical_memory_shared_compressed=success,disc_image_copied=False,
            new_persistent_executables=False,physical_xbox_verified=False,full_weapon_parity=False))
    call=json.loads((output/'call.json').read_text())
    print(json.dumps(dict(passed=success,entry=call['entry'],models=len(call['models']),tick_before=call['tick_before'],tick_after=call['tick_after'],
        original_RAM_bytes=ram,output=str(output),runner_scratch=str(scratch))),flush=True)

if __name__=='__main__':main()
