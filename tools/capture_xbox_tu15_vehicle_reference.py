#!/usr/bin/env python3
"""Private, normally controlled TU1.5 vehicle reference session; no guest patches.

Menu/gameplay inputs and screenshots are retained as evidence. This launcher
alone does not establish a camera match or physical Xbox framebuffer parity.
"""
import argparse
import json
import os
import re
import resource
from pathlib import Path
import socket
import subprocess
import time
from capture_xbox_tu15_grenade_reference import ROOT, TEXT_SHA, qmp, sha, wait_for, x11_window, write_json, host_capture


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('action', choices=['start', 'key', 'frame', 'stop', 'snapshot', 'state', 'resume', 'gpu_start', 'gpu_end'])
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--session', type=Path)
    p.add_argument('--disc-manifest', type=Path)
    p.add_argument('--snapshot', default='tu15-lockout-neutral')
    p.add_argument('--key')
    p.add_argument('--seconds', type=float, default=.15)
    p.add_argument('--name', default='frame')
    p.add_argument('--physical-ram', action='store_true', help='Retain full physical RAM at the final capture only')
    p.add_argument('--renderdoc', action='store_true', help='Retain actual original GPU commands in this private session')
    p.add_argument('--scene-boundaries', type=int, default=1)
    a = p.parse_args(); out = a.output.resolve()
    if a.action == 'start':
        assert a.session and a.disc_manifest and not out.exists()
        session = a.session.resolve(); manifest = json.loads(a.disc_manifest.read_text())
        assert manifest['reference'] == 'OG Xbox TU1.5' and manifest['all_packaged_bytes_verified']
        assert not manifest['xbe_media_patched']
        assert next(s for s in manifest['executable']['sections'] if s['name'] == '.text')['sha256'] == TEXT_SHA
        out.mkdir(parents=True)
        for name in ['base-hdd.qcow2', 'session-hdd.qcow2', 'eeprom.bin']:
            subprocess.run(['cp', '--reflink=auto', '--', str(session/name), str(out/name)], check=True)
        # QEMU's io_uring fd monitor can exhaust this host's small per-process
        # locked-memory allowance after its first context succeeds. A zero
        # allowance selects its supported epoll fallback consistently. This
        # policy is confined to this metadata-only qemu-img child process.
        subprocess.run(['qemu-img', 'rebase', '-u', '-f', 'qcow2', '-F', 'qcow2', '-b', str(out/'base-hdd.qcow2'), str(out/'session-hdd.qcow2')],
            preexec_fn=lambda:resource.setrlimit(resource.RLIMIT_MEMLOCK,(0,0)),check=True)
        config = (session/'xemu.toml').read_text()
        for name in ['session-hdd.qcow2', 'eeprom.bin']:
            config = config.replace(str(session/name), str(out/name))
        bindings = '''[input.keyboard_controller_scancode_map]
a = 40
b = 42
x = 27
y = 28
start = 21
back = 41
dpad_up = 82
dpad_down = 81
dpad_left = 80
dpad_right = 79
rstick_up = 12
rstick_left = 13
rstick_down = 14
rstick_right = 15'''
        bindings += '\nlstick_up = 26\nlstick_left = 7\nlstick_right = 9\nlstick_down = 22\nltrigger = 20\nrtrigger = 8\n\n'
        config = re.sub(r'^\[input\.keyboard_controller_scancode_map\][\s\S]*?(?=^\[|\Z)', bindings, config, flags=re.M)
        (out/'xemu.toml').write_text(config)
        read_fd, write_fd = os.pipe()
        xvfb = subprocess.Popen(['Xvfb', '-displayfd', str(write_fd), '-screen', '0', '1280x960x24', '-nolisten', 'tcp', '-noreset'],
            pass_fds=(write_fd,), stdout=(out/'xvfb.log').open('w'), stderr=subprocess.STDOUT, start_new_session=True)
        os.close(write_fd); display = ':'+os.read(read_fd, 64).decode().strip(); os.close(read_fd)
        with socket.socket() as port:
            port.bind(('127.0.0.1', 0)); gdb_port = port.getsockname()[1]
        # Use a short socket path; the retained output path exceeds UNIX limits.
        qmp_path = Path('/tmp')/('h2-vehicle-'+str(xvfb.pid)+'.sock')
        env = dict(os.environ, DISPLAY=display, LIBGL_ALWAYS_SOFTWARE='1', SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
            __GLX_VENDOR_LIBRARY_NAME='mesa', __EGL_VENDOR_LIBRARY_FILENAMES='/usr/share/glvnd/egl_vendor.d/50_mesa.json',
            XDG_CONFIG_HOME=str(out/'profile'), XDG_DATA_HOME=str(out/'profile'))
        if a.renderdoc:
            library=ROOT/'.tools/renderdoc/usr/lib/librenderdoc.so';helper=out/'gl-capture-control.so'
            subprocess.run(['cc','-shared','-fPIC','-O2','-I'+str(ROOT/'.tools/renderdoc/usr/include'),str(ROOT/'tools/xemu_gl_capture_control.c'),'-o',str(helper),'-ldl','-lpthread'],check=True)
            env.update(LD_PRELOAD=str(library)+':'+str(helper),HALO2_RENDERDOC_LIBRARY=str(library),HALO2_GL_CAPTURE_REQUEST=str(out/'gpu-request'),
                HALO2_CAPTURE_PREFIX=str(out/'vehicle'),HALO2_CAPTURE_ALLOW_DEBUGGER='1')
        argv = [str(ROOT/'.tools/xemu-app/squashfs-root/usr/bin/xemu'), '-config_path', str(out/'xemu.toml'), '-S',
            '-qmp', f'unix:{qmp_path},server=on,wait=off', '-gdb', f'tcp:127.0.0.1:{gdb_port}']
        xemu = subprocess.Popen(argv, env=env, stdout=(out/'xemu.log').open('w'), stderr=subprocess.STDOUT, start_new_session=True)
        write_json(out/'launch.json', dict(argv=argv, pid=xemu.pid, xvfb_pid=xvfb.pid, display=display, qmp=str(qmp_path), gdb_port=gdb_port,
            reference='OG Xbox TU1.5', physical_xbox_capture=False, guest_code_patches=[], guest_memory_patches=[],
            source_manifest=str(a.disc_manifest.resolve()), source_manifest_sha256=sha(a.disc_manifest), snapshot=a.snapshot))
        wait_for(qmp_path.exists, 15, [xemu])
        def ready():
            r=qmp(qmp_path,[{'execute':'human-monitor-command','arguments':{'command-line':'info qtree'}}])
            return 'usb-xbox-gamepad' in r[1]['return'] and 'usb-hub' in r[1]['return']
        wait_for(ready, 15, [xemu])
        r=qmp(qmp_path,[{'execute':'human-monitor-command','arguments':{'command-line':'loadvm '+a.snapshot}}, {'execute':'cont'}])
        assert not r[1]['return'].strip(), r
        write_json(out/'load.json',r)
        print(json.dumps(dict(pid=xemu.pid, display=display)))
        return
    state=json.loads((out/'launch.json').read_text()); qp=Path(state['qmp'])
    log=out/'actions.jsonl'
    record=dict(action=a.action, name=a.name, time=time.time())
    if a.action == 'key':
        assert a.key and 0<a.seconds<=10
        x11_window(state['display'], True, a.key, a.seconds)
        time.sleep(1)
        record.update(key=a.key,seconds=a.seconds)
    elif a.action == 'frame':
        assert a.name.replace('-','').replace('_','').isalnum()
        window=x11_window(state['display'])
        subprocess.run(['import', '-display', state['display'], '-window', str(window), str(out/(a.name+'.png'))], check=True, timeout=10)
        record['sha256']=sha(out/(a.name+'.png'))
    elif a.action == 'snapshot':
        assert a.name.replace('-','').replace('_','').isalnum()
        record['qmp']=qmp(qp,[{'execute':'human-monitor-command','arguments':{'command-line':'savevm '+a.name}}])
        assert not record['qmp'][1]['return'].strip(),record
    elif a.action == 'resume':
        record['qmp']=qmp(qp,[{'execute':'cont'}])
    elif a.action in ('gpu_start','gpu_end'):
        host_capture(out,state['pid'],a.action=='gpu_start')
    elif a.action == 'state':
        dest=out/a.name;assert not dest.exists();dest.mkdir()
        qmp(qp,[{'execute':'stop'}])
        script=dest/'guest.gdb'
        script.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 15
target remote 127.0.0.1:{state['gdb_port']}
hbreak *0x132b0 if *(float*)0x4c6690!=0.0
continue
python
import gdb,json,struct,pathlib,hashlib
assert 1<={a.scene_boundaries}<=8
for _ in range({a.scene_boundaries}-1):gdb.execute('continue')
out=pathlib.Path({str(dest)!r});inf=gdb.selected_inferior()
def read(p,n):return bytes(inf.read_memory(p,n))
def word(p):return struct.unpack('<I',read(p,4))[0]
text=read(0x12000,3611948);assert hashlib.sha256(text).hexdigest()=={TEXT_SHA!r}
(out/'tu15-text.bin').write_bytes(text)
camera=read(0x4c6634,192);(out/'camera.bin').write_bytes(camera)
(out/'view-header.bin').write_bytes(read(0x491e20,0x298))
(out/'next-view-header.bin').write_bytes(read(word(int(gdb.parse_and_eval('$esp'))+4),0x298))
(out/'game-variant.bin').write_bytes(read(word(0x4f3218)+0x13c,0x130))
pool=word(0x4ecbd0);base=word(pool+0x44);objects=[]
cache_pool=word(0x515f14);cache_rows=word(cache_pool+0x44)
physics_pool=word(0x52be5c);physics_rows=word(physics_pool+0x44)
velocity_banks=[]
def retain(p,n):
 raw=read(p,n);velocity_banks.append(dict(address=p,bytes=raw.hex()));return raw
retain(0x4ecbd0,4);retain(pool,0x4c);retain(base,word(pool+0x20)*12)
retain(0x52be5c,4);retain(physics_pool,0x4c);retain(0x474d04,4);retain(word(0x474d04),12)
for index in range(word(pool+0x38)):
 header=read(base+index*12,12);salt=struct.unpack_from('<H',header)[0]
 if not salt:continue
 body=struct.unpack_from('<I',header,8)[0]
 if not body:continue
 extent=struct.unpack_from('<H',header,6)[0]
 assert 0x118<=extent<=65535
 data=read(body,extent)
 object=(salt<<16)|index
 (out/(f'object-{{object:08x}}.bin')).write_bytes(data)
 row=dict(object=object,header=header.hex(),body_address=body,definition=struct.unpack_from('<I',data)[0],type=data[0xaa])
 if data[0xaa]==1:
  velocity_banks.append(dict(address=body,bytes=data.hex()))
  component=struct.unpack_from('<I',data,0xb4)[0]
  if component!=0xffffffff:
   cb=retain(physics_rows+(component&65535)*0xa0,0xa0)
   count=struct.unpack_from('<I',cb,0x74)[0];primary=struct.unpack_from('<b',cb,0x18)[0]
   if 0<=primary<count:
    table=struct.unpack_from('<I',cb,0x70)[0];rb=retain(table+primary*0x60,0x60)
    rigid=retain(struct.unpack_from('<I',rb,0x40)[0],0x50)
    if rigid[0x40]==0:retain(struct.unpack_from('<I',rigid,0x3c)[0],0x50)
 if data[0xaa] in (0,1,2):
  token=struct.unpack_from('<I',data,0xcc)[0]
  if token!=0xffffffff:
   cached=read(cache_rows+(token&65535)*0x100,0x100)
   assert struct.unpack_from('<I',cached,4)[0]==object
   (out/(f'cache-{{object:08x}}.bin')).write_bytes(cached)
   row['lighting_words']=list(struct.unpack_from('<21I',cached,0x54))
 objects.append(row)
clock=read(word(0x51df54),0x24);(out/'game-clock.bin').write_bytes(clock)
(out/'game-options.bin').write_bytes(read(word(0x4f3218),0x2e0))
player_pool=word(0x4f54f4);player_rows=word(player_pool+0x44);players=[]
units={{o['object'] for o in objects if o['type']==0}}
for index in range(word(player_pool+0x20)):
 row=player_rows+index*0x21c;data=read(row,0x21c)
 current=struct.unpack_from('<I',data,0x2c)[0]
 if current not in units:continue
 player=(struct.unpack_from('<H',data)[0]<<16)|index
 (out/(f'player-{{player:08x}}.bin')).write_bytes(data)
 players.append(dict(player=player,unit=current,configuration=data[0x44:0xd4].hex()))
(out/'velocity-banks.json').write_text(json.dumps(velocity_banks)+'\\n')
(out/'state.json').write_text(json.dumps(dict(reference='OG Xbox TU1.5',physical_xbox_capture=False,whole_live_text_matches_original=True,text_sha256=hashlib.sha256(text).hexdigest(),pc=int(gdb.parse_and_eval('$eip')),camera_words=list(struct.unpack('<48I',camera)),pool=pool,objects=objects,players=players,clock_words=list(struct.unpack('<9I',clock)),guest_code_patches=[],guest_memory_patches=[]),indent=2)+'\\n')
end
delete 1
detach
quit
''')
        with (dest/'guest.log').open('w') as f:subprocess.run(['gdb','-q','-nx','-batch','-x',str(script)],stdout=f,stderr=subprocess.STDOUT,timeout=45,check=True)
        qmp(qp,[{'execute':'stop'}])
        if a.physical_ram:
            qmp(qp,[{'execute':'pmemsave','arguments':{'val':0,'size':67108864,'filename':str(dest/'physical-ram.bin')}}])
        window=x11_window(state['display'])
        subprocess.run(['import','-display',state['display'],'-window',str(window),str(dest/'original.png')],check=True,timeout=10)
        record.update(directory=str(dest),whole_live_text_matches_original=True)
    elif a.action == 'stop':
        qmp(qp,[{'execute':'quit'}]); os.kill(state['xvfb_pid'],15)
    with log.open('a') as f:f.write(json.dumps(record)+'\n')
    print(json.dumps(record))


if __name__=='__main__':main()
