#!/usr/bin/env python3
"""Capture an unchanged Xbox TU1.5 grenade light pass in a private xemu session.

Starts from an existing normally played Lockout snapshot. Hardware breakpoints
hold the guest while its source state and the PGRAPH OpenGL capture are joined.
The original executable is authenticated from live memory, not its filename.
This is emulator evidence, not a physical Xbox framebuffer capture.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import select
import socket
import subprocess
import tempfile
import time
import tomllib

ROOT = Path(__file__).resolve().parents[1]
TEXT_SHA = 'd9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe'


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def qmp(path, commands):
    with socket.socket(socket.AF_UNIX) as connection:
        connection.settimeout(30)
        connection.connect(str(path))
        stream = connection.makefile('rwb', buffering=0)
        stream.readline()
        results = []
        for index, command in enumerate([{'execute': 'qmp_capabilities'}] + commands):
            stream.write((json.dumps(dict(command, id=index)) + '\n').encode())
            while True:
                line = stream.readline()
                if not line:
                    raise RuntimeError('QMP disconnected')
                reply = json.loads(line)
                if reply.get('id') == index:
                    break
            if 'error' in reply:
                raise RuntimeError(reply)
            results.append(reply)
        return results


def wait_for(predicate, timeout, processes=()):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        for process in processes:
            if process.poll() is not None:
                raise RuntimeError(f'Process {process.pid} exited: {process.returncode}')
        time.sleep(.05)
    raise TimeoutError(f'Condition not reached within {timeout} seconds')


def x11_window(display, press=False, key='q', duration=.35):
    x = ctypes.CDLL('libX11.so.6')
    x.XOpenDisplay.argtypes = [ctypes.c_char_p]
    x.XOpenDisplay.restype = ctypes.c_void_p
    d = x.XOpenDisplay(display.encode())
    if not d:
        raise RuntimeError('Cannot open private display')
    x.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
    x.XDefaultRootWindow.restype = ctypes.c_ulong
    x.XQueryTree.argtypes = [ctypes.c_void_p, ctypes.c_ulong,
        ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_ulong),
        ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)), ctypes.POINTER(ctypes.c_uint)]
    x.XFetchName.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_char_p)]
    x.XFree.argtypes = [ctypes.c_void_p]
    x.XCloseDisplay.argtypes = [ctypes.c_void_p]
    children = ctypes.POINTER(ctypes.c_ulong)()
    a, b, count = ctypes.c_ulong(), ctypes.c_ulong(), ctypes.c_uint()
    x.XQueryTree(d, x.XDefaultRootWindow(d), ctypes.byref(a), ctypes.byref(b),
                ctypes.byref(children), ctypes.byref(count))
    matches = []
    for i in range(count.value):
        name = ctypes.c_char_p()
        x.XFetchName(d, children[i], ctypes.byref(name))
        if name.value and name.value.startswith(b'xemu'):
            matches.append(children[i])
        if name.value:
            x.XFree(name)
    x.XFree(children)
    if len(matches) != 1:
        x.XCloseDisplay(d)
        raise RuntimeError(f'Expected exactly one private xemu window: {matches}')
    if press:
        xt = ctypes.CDLL('libXtst.so.6')
        x.XSetInputFocus.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
        x.XStringToKeysym.argtypes = [ctypes.c_char_p]
        x.XStringToKeysym.restype = ctypes.c_ulong
        x.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        x.XKeysymToKeycode.restype = ctypes.c_uint
        x.XFlush.argtypes = [ctypes.c_void_p]
        xt.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
        x.XSetInputFocus(d, matches[0], 1, 0)
        code = x.XKeysymToKeycode(d, x.XStringToKeysym(key.encode()))
        if not code:
            x.XCloseDisplay(d)
            raise RuntimeError('Unknown private controller key: ' + key)
        xt.XTestFakeKeyEvent(d, code, 1, 0)
        x.XFlush(d)
        try:
            time.sleep(duration)
        finally:
            xt.XTestFakeKeyEvent(d, code, 0, 0)
            x.XFlush(d)
    x.XCloseDisplay(d)
    return matches[0]


def host_capture(out, pid, start):
    script = out / ('host-start.gdb' if start else 'host-end.gdb')
    if start:
        code = f'''
rows=[]
for thread in gdb.selected_inferior().threads():
 if thread.name!='xemu':continue
 thread.switch()
 context=int(gdb.parse_and_eval('(void*)halo2_gl_capture_current_context()'))
 rows.append(dict(num=thread.num,tid=thread.ptid[1],context=context))
candidates=[r for r in rows if r['tid']!={pid} and r['context']]
assert len(candidates)==1,candidates
chosen=candidates[0]
next(t for t in gdb.selected_inferior().threads() if t.num==chosen['num']).switch()
result=int(gdb.parse_and_eval('(unsigned)halo2_gl_capture_start_current_context()'))
assert result==1,result
pathlib.Path({str(out / 'contexts.json')!r}).write_text(json.dumps(dict(threads=rows,chosen=chosen,result=result),indent=2)+'\\n')
'''
    else:
        code = f'''
chosen=json.loads(pathlib.Path({str(out / 'contexts.json')!r}).read_text())['chosen']
next(t for t in gdb.selected_inferior().threads() if t.ptid[1]==chosen['tid']).switch()
context=int(gdb.parse_and_eval('(void*)halo2_gl_capture_current_context()'))
result=int(gdb.parse_and_eval('(unsigned)halo2_gl_capture_end_observed_context()'))
assert result==1,result
pathlib.Path({str(out / 'capture-ended.json')!r}).write_text(json.dumps(dict(current_context=context,captured_context=chosen['context'],result=result))+'\\n')
'''
    script.write_text(f'set pagination off\nset confirm off\nattach {pid}\npython\n'
                      'import gdb,json,pathlib\n' + code + '\nend\ndetach\nquit\n')
    with script.with_suffix('.log').open('w') as log:
        subprocess.run(['gdb', '-q', '-nx', '-batch', '-x', str(script)],
                       stdout=log, stderr=subprocess.STDOUT, timeout=30, check=True)
    assert (out / ('contexts.json' if start else 'capture-ended.json')).exists()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--disc-manifest', type=Path, required=True)
    parser.add_argument('--snapshot', default='tu15-lockout-neutral')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--pre-inputs', type=Path,
                        help='Declared normal keyboard-controller holds before throwing; JSON key/seconds rows, no guest memory changes')
    parser.add_argument('--warm-frames',type=int,default=3,
                        help='Reach this many unchanged original frame boundaries before delivering normal input')
    parser.add_argument('--throw-hold',type=float,default=.35)
    args = parser.parse_args()
    if not 0<=args.warm_frames<=30:raise ValueError('Warm frames must be in0..30')
    if not 0<args.throw_hold<=2:raise ValueError('Throw hold must be in(0,2] seconds')
    session, out = args.session.resolve(), args.output.resolve()
    manifest = json.loads(args.disc_manifest.read_text())
    assert manifest['reference'] == 'OG Xbox TU1.5'
    assert manifest['all_packaged_bytes_verified'] and not manifest['xbe_media_patched']
    text = next(s for s in manifest['executable']['sections'] if s['name'] == '.text')
    assert text['sha256'] == TEXT_SHA
    out.mkdir(parents=True, exist_ok=False)
    config = tomllib.loads((session / 'xemu.toml').read_text())
    pre_inputs=json.loads(args.pre_inputs.read_text()) if args.pre_inputs else []
    if not isinstance(pre_inputs,list) or any(not isinstance(row,dict) or row.get('key') not in ('i','j','k','l','w','a','s','d') or
            not isinstance(row.get('seconds'),(float,int)) or not 0<float(row['seconds'])<=5 for row in pre_inputs):
        raise ValueError('Pre-inputs require normal movement/look key holds of at most five seconds each')
    processes = []
    with tempfile.TemporaryDirectory(prefix='h2-tu15-ref-') as temporary:
        runtime = Path(temporary)
        qmp_path = runtime / 'qmp.sock'
        try:
            for filename in ('base-hdd.qcow2', 'session-hdd.qcow2', 'eeprom.bin'):
                subprocess.run(['cp', '--reflink=auto', '--', str(session / filename), str(out / filename)], check=True)
            subprocess.run(['qemu-img', 'rebase', '-u', '-f', 'qcow2', '-F', 'qcow2',
                            '-b', str(out / 'base-hdd.qcow2'), str(out / 'session-hdd.qcow2')], check=True)
            config['sys']['files'].update(eeprom_path=str(out / 'eeprom.bin'),
                hdd_path=str(out / 'session-hdd.qcow2'), dvd_path=manifest['image'])
            config['display'].pop('renderer', None)
            # Preserve the known-good source session syntax, changing paths only.
            config_text = (session / 'xemu.toml').read_text()
            for filename in ('session-hdd.qcow2', 'eeprom.bin'):
                config_text = config_text.replace(str(session / filename), str(out / filename))
            if pre_inputs:
                # Literal SDL scancodes, verified field names in the installed
                # xemu configuration schema. Only host controller bindings change.
                config_text=config_text.replace('[input.keyboard_controller_scancode_map]',
                    '[input.keyboard_controller_scancode_map]\nrstick_up = 12\nrstick_left = 13\nrstick_down = 14\nrstick_right = 15')
            (out / 'xemu.toml').write_text(config_text)
            library = ROOT / '.tools/renderdoc/usr/lib/librenderdoc.so'
            helper = out / 'gl-capture-control.so'
            subprocess.run(['cc', '-shared', '-fPIC', '-O2',
                '-I' + str(ROOT / '.tools/renderdoc/usr/include'),
                str(ROOT / 'tools/xemu_gl_capture_control.c'), '-o', str(helper), '-ldl', '-lpthread'], check=True)
            read_fd, write_fd = os.pipe()
            xvfb_log = (out / 'xvfb.log').open('w')
            xvfb = subprocess.Popen(['Xvfb', '-displayfd', str(write_fd), '-screen', '0', '1280x960x24',
                '-nolisten', 'tcp', '-noreset'], pass_fds=(write_fd,), stdout=xvfb_log, stderr=subprocess.STDOUT)
            processes.append(xvfb)
            os.close(write_fd)
            display_line=b'';display_deadline=time.monotonic()+10
            while b'\n' not in display_line:
                remaining=display_deadline-time.monotonic()
                assert remaining>0 and select.select([read_fd],[],[],remaining)[0], 'Private display did not start'
                chunk=os.read(read_fd,64)
                assert chunk,'Private display ended before publishing its number'
                display_line+=chunk
            assert display_line.strip().isdigit(),display_line
            display = ':' + display_line.decode().strip()
            os.close(read_fd)
            with socket.socket() as port:
                port.bind(('127.0.0.1', 0))
                gdb_port = port.getsockname()[1]
            env = dict(os.environ, DISPLAY=display, LIBGL_ALWAYS_SOFTWARE='1',
                SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
                __GLX_VENDOR_LIBRARY_NAME='mesa',
                __EGL_VENDOR_LIBRARY_FILENAMES='/usr/share/glvnd/egl_vendor.d/50_mesa.json',
                XDG_CONFIG_HOME=str(out / 'profile'),
                XDG_DATA_HOME=str(out / 'profile'),
                LD_PRELOAD=str(library) + ':' + str(helper), HALO2_RENDERDOC_LIBRARY=str(library),
                HALO2_GL_CAPTURE_REQUEST=str(runtime / 'request'), HALO2_CAPTURE_PREFIX=str(out / 'tu15-light'),
                HALO2_CAPTURE_ALLOW_DEBUGGER='1')
            xemu_log = (out / 'xemu.log').open('w')
            argv = [str(ROOT / '.tools/xemu-app/squashfs-root/usr/bin/xemu'), '-config_path',
                str(out / 'xemu.toml'), '-S', '-qmp', f'unix:{qmp_path},server=on,wait=off',
                '-gdb', f'tcp:127.0.0.1:{gdb_port}']
            xemu = subprocess.Popen(argv, env=env, stdout=xemu_log, stderr=subprocess.STDOUT)
            processes.append(xemu)
            write_json(out / 'launch.json', dict(argv=argv, pid=xemu.pid, display=display,
                reference='OG Xbox TU1.5', physical_xbox_capture=False, guest_code_patches=[]))
            wait_for(qmp_path.exists, 15, [xemu])
            # The UI hotplugs the configured keyboard controller after QMP
            # opens. Loading before that point races the saved USB topology.
            topology=[]
            def controller_ready():
                rows=qmp(qmp_path,[{'execute':'human-monitor-command','arguments':{'command-line':'info qtree'}}])
                topology[:] = rows
                return 'usb-xbox-gamepad' in rows[1]['return'] and 'usb-hub' in rows[1]['return']
            wait_for(controller_ready,15,[xemu])
            write_json(out/'virtual-input-topology.json',topology)
            reply = qmp(qmp_path, [{'execute': 'human-monitor-command',
                'arguments': {'command-line': 'loadvm ' + args.snapshot}}, {'execute': 'stop'}])
            assert not reply[1]['return'].strip(), reply
            write_json(out / 'load-neutral.json', reply)
            light_break=2 if args.warm_frames else 1
            upload_break=light_break+1
            warmup=''
            if args.warm_frames:
                warmup=f'''hbreak *0x132b0
python
import gdb,json,pathlib,struct
warm=[]
for frame in range({args.warm_frames}):
 gdb.execute('continue')
 assert int(gdb.parse_and_eval('$eip'))==0x132b0
 warm.append(dict(frame=frame,pc=0x132b0,camera_hex=bytes(gdb.selected_inferior().read_memory(0x4c6634,192)).hex()))
pathlib.Path({str(out/'warm-original-frames.json')!r}).write_text(json.dumps(warm,indent=2))
end
delete 1
'''
            guest_script = out / 'guest.gdb'
            guest_script.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 15
target remote 127.0.0.1:{gdb_port}
{warmup}
hbreak *0x4a1d0 if (unsigned int)$edx!=0xffffffff && *(unsigned int*)(*(unsigned int*)(*(unsigned int*)0x4ecbdc+0x44)+($edx & 65535)*0x110+4)==0xe63a04c5
echo WAITING_FOR_NORMAL_GRENADE\\n
continue
python
import gdb,json,struct,pathlib,hashlib,socket
out=pathlib.Path({str(out)!r})
inferior=gdb.selected_inferior()
registers={{r:int(gdb.parse_and_eval('$'+r)) & 0xffffffff for r in ['eax','ecx','edx','esp','eip']}}
identity=registers['edx']
stack=list(struct.unpack('<5I',inferior.read_memory(registers['esp'],20)))
(out/'entry.json').write_text(json.dumps(dict(registers=registers,stack=stack),indent=2)+'\\n')
end
disable {light_break}
hbreak *0x4a3c7
continue
python
assert int(gdb.parse_and_eval('$eip'))==0x4a3c7
pool=struct.unpack('<I',inferior.read_memory(0x4ecbdc,4))[0]
base=struct.unpack('<I',inferior.read_memory(pool+0x44,4))[0]
light=bytes(inferior.read_memory(base+(identity&65535)*0x110,0x110))
assert struct.unpack_from('<I',light,4)[0]==0xe63a04c5
(out/'light.bin').write_bytes(light)
(out/'camera.bin').write_bytes(bytes(inferior.read_memory(0x4c6634,192)))
code=bytes(inferior.read_memory({text['address']},{text['file_size']}))
assert hashlib.sha256(code).hexdigest()=={TEXT_SHA!r}
(out/'tu15-text.bin').write_bytes(code)
s=socket.socket(socket.AF_UNIX);s.settimeout(30);s.connect({str(qmp_path)!r});f=s.makefile('rwb',buffering=0);f.readline();replies=[]
commands=[dict(execute='qmp_capabilities'),dict(execute='stop'),dict(execute='human-monitor-command',arguments={{'command-line':'info registers'}}),dict(execute='pmemsave',arguments={{'val':0,'size':67108864,'filename':str(out/'physical-ram.bin')}}),dict(execute='query-status')]
for n,c in enumerate(commands):
 c['id']=n;f.write((json.dumps(c)+'\\n').encode())
 while True:
  reply=json.loads(f.readline())
  if reply.get('id')==n:break
 assert 'error' not in reply,reply
 replies.append(reply)
f.close();s.close()
(out/'source-state.json').write_text(json.dumps(dict(reference='OG Xbox TU1.5',qmp=replies,registers_before=replies[2]['return'],light_identity=identity,light_definition=0xe63a04c5,guest_pc=0x4a3c7,code_sha256=hashlib.sha256(code).hexdigest(),whole_live_text_matches_original=True,ram_sha256=hashlib.sha256((out/'physical-ram.bin').read_bytes()).hexdigest(),guest_code_patches=[]),indent=2)+'\\n')
(out/'SOURCE_READY').write_text('guest remains stopped and attached\\n')
end
''')
            guest_log = (out / 'guest.log').open('w')
            guest = subprocess.Popen(['gdb', '-q', '-nx', '-x', str(guest_script)],
                stdin=subprocess.PIPE, stdout=guest_log, stderr=subprocess.STDOUT, text=True)
            processes.append(guest)
            wait_for(lambda: 'WAITING_FOR_NORMAL_GRENADE' in (out / 'guest.log').read_text(), 40, [guest, xemu])
            # GDB owns guest execution until the capture closes. A parallel
            # QMP cont would race the pending GDB continue command.
            def window_ready():
                try:
                    return x11_window(display)
                except RuntimeError:
                    return False
            wait_for(window_ready, 15, [xemu])
            write_json(out/'declared-pre-inputs.json',pre_inputs)
            for row in pre_inputs:
                x11_window(display,press=True,key=row['key'],duration=float(row['seconds']))
            if pre_inputs:
                subprocess.run(['import','-display',display,'-window',str(x11_window(display)),
                    str(out/'before-throw.png')],check=True,timeout=10)
            window = x11_window(display, press=True,duration=args.throw_hold)
            try:
                wait_for((out / 'SOURCE_READY').exists, 40, [guest, xemu])
            except TimeoutError:
                # Preserve the original capture failure even when a stopped
                # remote-debug condition also prevents a QMP diagnostic reply.
                diagnostics={}
                try:
                    subprocess.run(['import','-display',display,'-window',str(x11_window(display)),
                        str(out/'missing-light-diagnostic.png')],check=True,timeout=10)
                except Exception as error:
                    diagnostics['screenshot_error']=repr(error)
                try:
                    write_json(out/'missing-light-qmp.json',qmp(qmp_path,[{'execute':'query-status'}]))
                except Exception as error:
                    diagnostics['qmp_error']=repr(error)
                write_json(out/'capture-failure.json',dict(boundary='waiting for original grenade source state',
                    accepted_reference=False,diagnostics=diagnostics))
                raise
            print('Authenticated TU1.5 grenade source state; starting PGRAPH capture', flush=True)
            host_capture(out, xemu.pid, True)
            guest.stdin.write(f'disable {upload_break}\nhbreak *0x132b0\ncontinue\n')
            guest.stdin.flush()
            wait_for(lambda: f'Breakpoint {upload_break+1},' in (out / 'guest.log').read_text(), 25, [guest, xemu])
            time.sleep(.25)
            host_capture(out, xemu.pid, False)
            captures = list(out.glob('tu15-light*.rdc'))
            assert len(captures) == 1, captures
            subprocess.run(['import', '-display', display, '-window', str(x11_window(display)),
                            str(out / 'original-frame.png')], check=True, timeout=10)
            write_json(out / 'manifest.json', dict(reference='OG Xbox TU1.5',
                physical_xbox_capture=False, vista_resources=False, guest_code_patches=[],
                source_state='source-state.json', capture=captures[0].name,
                capture_sha256=sha(captures[0]), image='original-frame.png',
                whole_live_text_matches_original=True, hardware_breakpoints_only=True,
                capture_boundary=dict(start=0x4a3c7, end=0x132b0),
                snapshot=args.snapshot, normal_input='X11 q key, authored left trigger binding',
                declared_pre_inputs=pre_inputs,pre_input_timing='host key hold seconds; no claim of fixed guest input cadence',
                warm_original_frame_boundaries=args.warm_frames,
                throw_hold_seconds=args.throw_hold,
                disc_manifest_sha256=sha(args.disc_manifest), tool_sha256=sha(Path(__file__))))
            guest.stdin.write('detach\nquit\n')
            guest.stdin.flush()
            guest.wait(timeout=5)
            qmp(qmp_path, [{'execute': 'quit'}])
            print(json.dumps(dict(capture=str(captures[0]), source='OG Xbox TU1.5', authenticated=True)), flush=True)
        finally:
            for process in reversed(processes):
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5)


if __name__ == '__main__':
    main()
