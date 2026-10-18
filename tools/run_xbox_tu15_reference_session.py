#!/usr/bin/env python3
"""Own a disposable original TU1.5 session for normal controller observations.

Run via tools/run_test.py. The session starts from a read-only, authenticated
normally played snapshot. STOP in the report directory ends it; a time bound
also guarantees cleanup. No guest code, registers or gameplay state are written.
All working disks, display data and raw readbacks stay in owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import resource
import select
import shutil
import socket
import subprocess
import time
import tomllib
from capture_xbox_tu15_grenade_reference import ROOT,TEXT_SHA,qmp,sha,wait_for,write_json
from xbox_tu15_reference_storage import pins,prepare_private_disk,verify_reference_disk

def live_text(qp):
    descriptor=os.memfd_create('tu15-live-code',os.MFD_CLOEXEC)
    try:
        qmp(qp,[dict(execute='memsave',arguments=dict(val=0x12000,size=3611948,filename=f'/proc/{os.getpid()}/fd/{descriptor}'))])
        raw=os.pread(descriptor,3611948,0);assert len(raw)==3611948
        digest=hashlib.sha256(raw).hexdigest();assert digest==TEXT_SHA
        return digest
    finally:os.close(descriptor)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('session','evidence','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--snapshot',default='tu15-needler-idle')
    parser.add_argument('--max-seconds',type=int,default=1200)
    parser.add_argument('--retain-snapshot',help='Explicitly retain a read-only normally played reference template at successful STOP')
    args=parser.parse_args();assert 30<=args.max_seconds<=3600
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);working=scratch/'session';working.mkdir()
    output=args.output.absolute();output.mkdir(parents=True,exist_ok=False)
    session=args.session.absolute();evidence=json.loads(args.evidence.read_text())
    assert evidence['passed']and evidence['full_live_TU15_code_authenticated']
    source_pins=pins(evidence);disk=session/'session-hdd.qcow2'
    original_sha=prepare_private_disk(session,working,evidence)
    config=(session/'xemu.toml').read_text();source_config=config;settings=tomllib.loads(config);image=Path(settings['sys']['files']['dvd_path'])
    assert sha(image)==source_pins[str(image)]
    shutil.copyfile(session/'eeprom.bin',working/'eeprom.bin')
    for name in ('session-hdd.qcow2','eeprom.bin'):config=config.replace(str(session/name),str(working/name))
    assert '[display]\n'not in config
    config=config.replace('[display.vulkan]',"[display]\nrenderer = 'OPENGL'\n\n[display.vulkan]")
    (working/'xemu.toml').write_text(config);shutil.copyfile(working/'xemu.toml',output/'launch-config.toml')
    shutil.copyfile(Path(__file__),output/'session-source.py')
    shutil.copyfile(ROOT/'tools/xbox_tu15_reference_storage.py',output/'reference-storage-source.py')
    processes=[];qp=None;completed=False;final_text=None
    try:
        read_fd,write_fd=os.pipe()
        with (working/'xvfb.log').open('w')as log:
            display_process=subprocess.Popen(['Xvfb','-displayfd',str(write_fd),'-screen','0','640x480x24','-nolisten','tcp','-noreset'],pass_fds=(write_fd,),stdout=log,stderr=subprocess.STDOUT)
        processes.append(display_process);os.close(write_fd)
        with os.fdopen(read_fd)as number:
            assert select.select([number],[],[],10)[0];line=number.readline(64);assert line.endswith('\n')
            display=':'+line.strip()
        assert display[1:].isdigit()
        with socket.socket()as reservation:reservation.bind(('127.0.0.1',0));port=reservation.getsockname()[1]
        env=dict(os.environ,DISPLAY=display,LIBGL_ALWAYS_SOFTWARE='1',__GLX_VENDOR_LIBRARY_NAME='mesa',
            __EGL_VENDOR_LIBRARY_FILENAMES='/usr/share/glvnd/egl_vendor.d/50_mesa.json',SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
            XDG_CONFIG_HOME=str(working/'profile'),XDG_DATA_HOME=str(working/'profile'),XDG_CACHE_HOME=str(working/'cache'))
        command=[str(ROOT/'.tools/xemu-app/squashfs-root/usr/bin/xemu'),'-config_path',str(working/'xemu.toml'),'-S',
            '-qmp','unix:qmp.sock,server=on,wait=off','-gdb',f'tcp:127.0.0.1:{port}','-nic','none','-no-reboot','-no-shutdown']
        with (working/'xemu.log').open('w')as log:xemu=subprocess.Popen(command,cwd=working,env=env,stdout=log,stderr=subprocess.STDOUT)
        processes.append(xemu);qp=Path(f'/proc/{xemu.pid}/cwd/qmp.sock');wait_for(qp.exists,15,[xemu])
        def ready():
            tree=qmp(qp,[dict(execute='human-monitor-command',arguments={'command-line':'info qtree'})])[-1]['return']
            return 'usb-xbox-gamepad'in tree and 'usb-hub'in tree
        wait_for(ready,15,[xemu])
        restored=qmp(qp,[dict(execute='human-monitor-command',arguments={'command-line':'loadvm '+args.snapshot}),dict(execute='stop')])
        assert not restored[1]['return'].strip();write_json(output/'restore.json',restored)
        launch=dict(command=command,pid=xemu.pid,xvfb_pid=display_process.pid,display=display,gdb_port=port,qmp=str(qp),
            snapshot=args.snapshot,original_session=str(session),original_session_sha256=original_sha,
            reference_image=str(image),reference_image_sha256=sha(image),initial_live_text_sha256=live_text(qp),
            core_limit=resource.getrlimit(resource.RLIMIT_CORE)[0],runner_scratch=str(scratch),
            tool_sha256=sha(Path(__file__)),helper_sha256=sha(ROOT/'tools/capture_xbox_tu15_grenade_reference.py'),
            guest_code_patches=[],guest_memory_patches=[],guest_register_writes=[],physical_xbox_verified=False,full_weapon_parity=False)
        assert launch['core_limit']==0;write_json(output/'launch.json',launch)
        print(json.dumps(dict(ready=True,output=str(output),display=display,qmp=str(qp),pid=xemu.pid)),flush=True)
        deadline=time.monotonic()+args.max_seconds
        while not(output/'STOP').exists():
            assert time.monotonic()<deadline,'Reference session time limit reached'
            for process in processes:assert process.poll()is None,'Reference subprocess exited'
            time.sleep(.1)
        qmp(qp,[dict(execute='stop')]);final_text=live_text(qp)
        if args.retain_snapshot:
            assert args.retain_snapshot.replace('-','').isalnum()
            result=qmp(qp,[dict(execute='human-monitor-command',arguments={'command-line':'savevm '+args.retain_snapshot})])
            assert not result[-1]['return'].strip();write_json(output/'save-reference-snapshot.json',result)
        completed=True
    finally:
        if qp and qp.exists():
            try:qmp(qp,[dict(execute='quit')])
            except(OSError,RuntimeError):pass
        for process in reversed(processes):
            if process.poll()is None:
                try:process.wait(timeout=8)
                except subprocess.TimeoutExpired:process.terminate()
            if process.poll()is None:
                try:process.wait(timeout=3)
                except subprocess.TimeoutExpired:process.kill();process.wait()
        for name in ('xemu.log','xvfb.log'):
            path=working/name
            if path.exists():(output/name).write_bytes(path.read_bytes()[-65536:])
        assert verify_reference_disk(session,evidence)==original_sha
        retained=[]
        if completed and args.retain_snapshot:
            target=output/'session-hdd.qcow2';subprocess.run(['cp','--reflink=always','--',str(working/disk.name),str(target)],check=True)
            assert sha(target)==sha(working/disk.name);target.chmod(0o444)
            shutil.copyfile(working/'eeprom.bin',output/'eeprom.bin')
            canonical=source_config
            for name in ('session-hdd.qcow2','eeprom.bin'):canonical=canonical.replace(str(session/name),str(output/name))
            (output/'xemu.toml').write_text(canonical)
            retained=[dict(path=str(target),bytes=target.stat().st_size,sha256=sha(target),read_only=True,
                reason='Required normally played original Lockout weapon reference VM snapshot for subsequent feature/visual comparisons')]
            write_json(output/'reference-evidence.json',dict(passed=True,full_live_TU15_code_authenticated=True,
                final_live_text_sha256=final_text,snapshot=args.retain_snapshot,source_session=str(session),
                source_evidence_sha256=sha(args.evidence),linked_files_sha256={str(p):sha(p)for p in (target,output/'eeprom.bin',output/'xemu.toml',image)},
                retained_large_artifacts=retained,physical_xbox_verified=False,full_weapon_parity=False))
        write_json(output/'cleanup.json',dict(completed=completed,all_processes_exited=all(p.poll()is not None for p in processes),
            original_session_unchanged=True,final_live_text_sha256=final_text,runner_scratch=str(scratch),
            working_disk=str(working/disk.name),large_artifacts_retained=retained,working_files_removed_by_runner=True,full_weapon_parity=False))

if __name__=='__main__':main()
