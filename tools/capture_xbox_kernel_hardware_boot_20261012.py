#!/usr/bin/env python3
"""Capture named kernel data from a fresh, isolated owned xemu boot.

Uses an HDD overlay and private EEPROM copy. No DVD/title is selected, no guest
code or data is patched, and no gameplay result is supplied. Complex4627 is a
modified BIOS; this is emulator resource evidence, not physical Xbox parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = ROOT/'tools/capture_xbox_kernel_hardware_20261012.py'

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    inputs = json.loads(a.inputs.read_text())
    a.output = a.output.resolve()
    a.output.mkdir(parents=True, exist_ok=False)
    for key in ('bootrom_path', 'flashrom_path', 'eeprom_path', 'xemu'):
        assert sha(inputs[key]['path']) == inputs[key]['sha256'], key
    hdd = Path(inputs['hdd_backing']['path'])
    before = hdd.stat()
    assert before.st_size == inputs['hdd_backing']['bytes']
    assert before.st_mtime_ns == inputs['hdd_backing']['mtime_ns']
    shutil.copyfile(inputs['eeprom_path']['path'], a.output/'eeprom.bin')
    subprocess.run(['qemu-img','create','-f','qcow2','-F','qcow2','-b',str(hdd),
        str(a.output/'session-hdd.qcow2')], check=True, capture_output=True)
    config = '''[general]
show_welcome = false
[input]
auto_bind = false
background_input_capture = false
[display.window]
startup_size = '640x480'
[audio]
volume_limit = 0
[sys.files]
'''
    for key, value in dict(bootrom_path=inputs['bootrom_path']['path'],
        flashrom_path=inputs['flashrom_path']['path'], eeprom_path=str(a.output/'eeprom.bin'),
        hdd_path=str(a.output/'session-hdd.qcow2'), dvd_path='').items():
        config += key+' = '+json.dumps(value)+'\n'
    (a.output/'xemu.toml').write_text(config)
    (a.output/'inputs.json').write_text(json.dumps(inputs, sort_keys=True, indent=2)+'\n')
    with tempfile.TemporaryDirectory(prefix='h2-khw-') as tmp:
        sockpath = str(Path(tmp)/'qmp.sock')
        with socket.socket() as probe:
            probe.bind(('127.0.0.1',0)); port = probe.getsockname()[1]
        with (a.output/'xemu.log').open('w') as log:
            launch_env = os.environ.copy()
            launch_env['SDL_VIDEODRIVER'] = 'x11'
            process = subprocess.Popen([inputs['xemu']['path'], '-config_path',str(a.output/'xemu.toml'),
                '-S','-qmp','unix:'+sockpath+',server=on,wait=off',
                '-gdb','tcp:127.0.0.1:'+str(port)], stdout=log, stderr=subprocess.STDOUT, env=launch_env)
            link = None
            try:
                deadline = time.monotonic()+20
                while not Path(sockpath).exists():
                    assert process.poll() is None, 'xemu exited before QMP was ready'
                    assert time.monotonic() < deadline, 'xemu QMP startup timed out'
                    time.sleep(.1)
                link = socket.socket(socket.AF_UNIX); link.settimeout(20); link.connect(sockpath)
                stream = link.makefile('rwb',buffering=0)
                greeting = json.loads(stream.readline())
                def q(command):
                    stream.write((json.dumps(dict(execute=command))+'\n').encode())
                    while True:
                        result = json.loads(stream.readline())
                        if 'error' in result: raise RuntimeError(result)
                        if 'return' in result: return result['return']
                q('qmp_capabilities'); q('cont'); time.sleep(5); q('stop')
                status = q('query-status'); assert not status['running']
                env = os.environ.copy(); env['HALO2_KERNEL_DATA_OUTPUT'] = str(a.output/'kernel')
                capture = subprocess.run(['gdb','-q','-nx','-batch','-ex','set architecture i386',
                    '-ex','target remote 127.0.0.1:'+str(port), '-ex','source '+str(CAPTURE),
                    '-ex','detach'],env=env,capture_output=True,text=True,timeout=20)
                (a.output/'capture.log').write_text(capture.stdout+capture.stderr)
                assert capture.returncode == 0
                report = json.loads((a.output/'kernel/report.json').read_text())
                assert report['tool_sha256'] == sha(CAPTURE)
                q('quit'); process.wait(timeout=20)
            finally:
                if link is not None: link.close()
                if process.poll() is None:
                    process.terminate()
                    try: process.wait(timeout=10)
                    except subprocess.TimeoutExpired: process.kill(); process.wait(timeout=10)
    for key in ('bootrom_path', 'flashrom_path', 'eeprom_path', 'xemu'):
        assert sha(inputs[key]['path']) == inputs[key]['sha256'], key
    after = hdd.stat()
    assert (after.st_size,after.st_mtime_ns)==(before.st_size,before.st_mtime_ns)
    result = dict(schema=1, tool_sha256=sha(__file__), capture_tool_sha256=sha(CAPTURE),
        input_manifest_sha256=sha(a.inputs), kernel_report_sha256=sha(a.output/'kernel/report.json'),
        reference='owned Complex4627 modified BIOS in xemu 0.8.136',
        qmp_version=greeting['QMP'], paused_status=status, own_process_exit=process.returncode,
        original_resources_unchanged=True, isolated_hdd_overlay=True, private_eeprom_copy=True,
        dvd_selected=False, title_executed=False, guest_writes=[], code_patches=[],
        physics_results_supplied=[], physical_xbox_proof=False)
    (a.output/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(records=report['records'],original_resources_unchanged=True,
        title_executed=False,physical_xbox_proof=False)))

if __name__ == '__main__': main()
