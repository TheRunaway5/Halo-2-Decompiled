#!/usr/bin/env python3
"""Capture original TU1.5 graphics at natural rendering entries.

Restores a retained normally played private state and observes it through
hardware breakpoints. No guest code, memory, register or game result is set.
The capture still needs draw-owner and GPU/source-boundary analysis before a
matched native comparison; no physical Xbox or whole-weapon parity is claimed.
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
import tomllib

from capture_xbox_tu15_grenade_reference import (
    ROOT, TEXT_SHA, qmp, sha, wait_for, write_json, x11_window,
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--census', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--snapshot', default='tu15-needler-idle')
    parser.add_argument('--unit', type=lambda x: int(x, 0), required=True)
    parser.add_argument('--weapon', type=lambda x: int(x, 0), required=True)
    parser.add_argument('--definition', type=lambda x: int(x, 0), required=True)
    parser.add_argument('--draw-parts', type=Path,
                        help='Capture from an actual indexed submission matching the first authored part')
    parser.add_argument('--observe-first-person', action='store_true',
                        help='Read completed actual168DED model records before indexed submissions')
    parser.add_argument('--observe-palettes', action='store_true',
                        help='Read actual first-person13A2E0 selected sections, WORLD inputs and returned palettes')
    parser.add_argument('--warm-views', type=int, default=3)
    args = parser.parse_args()
    assert 1 <= args.warm_views <= 8 and not args.output.exists()
    assert args.snapshot.replace('-', '').isalnum()
    session, out = args.session.absolute(), args.output.absolute()
    evidence = json.loads(args.evidence.read_text())
    assert evidence['passed'] and evidence['full_live_TU15_code_authenticated']
    pins = evidence['linked_files_sha256']
    assert sha(args.census) == pins[str(args.census.absolute())]
    census = json.loads(args.census.read_text())
    assert census['whole_live_text_sha256'] == TEXT_SHA
    assert any(p['current_unit'] == args.unit for p in census['players'])
    resident = next(o for o in census['objects'] if o['datum'] == args.weapon)
    assert resident['type'] == 2 and resident['definition'] == args.definition
    draw_part = None
    if args.draw_parts:
        authored = json.loads(args.draw_parts.read_text())
        assert authored['weapon'] == args.definition and authored['map_sha256'] == census['map_sha256']
        draw_part = authored['parts'][0]
        assert draw_part['index_count'] == len(draw_part['index_u16'])
    assert not args.observe_palettes or args.draw_parts
    palette_models = sorted({p['model'] for p in authored['parts']}) if args.observe_palettes else []
    capture_entry = 0x405080 if draw_part else 0x132b0
    breakpoint = f'hbreak *{capture_entry:#x}'
    if draw_part:
        breakpoint += f" if *(unsigned*)($esp+8)=={draw_part['index_count']}"
    original_disk = session / 'session-hdd.qcow2'
    original_sha = sha(original_disk)
    assert original_sha == pins[str(original_disk)]
    config = (session / 'xemu.toml').read_text()
    settings = tomllib.loads(config)
    image = Path(settings['sys']['files']['dvd_path'])
    assert sha(image) == pins[str(image)]
    out.mkdir(parents=True)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    subprocess.run(['cp', '--reflink=always', '--', str(original_disk),
                    str(out / 'session-hdd.qcow2')], check=True)
    os.chmod(out / 'session-hdd.qcow2', 0o600)
    assert sha(out / 'session-hdd.qcow2') == original_sha
    shutil.copy2(session / 'eeprom.bin', out / 'eeprom.bin')
    os.chmod(out / 'eeprom.bin', 0o600)
    for name in ('session-hdd.qcow2', 'eeprom.bin'):
        config = config.replace(str(session / name), str(out / name))
    # OPENGL is this installed xemu version's default and is confirmed by the
    # actual backend log. Setting it explicitly also makes launch reviewable.
    assert '[display]\n' not in config
    config = config.replace('[display.vulkan]', "[display]\nrenderer = 'OPENGL'\n\n[display.vulkan]")
    (out / 'xemu.toml').write_text(config)
    shutil.copy2(out / 'xemu.toml', out / 'launch-config.toml')
    library = ROOT / '.tools/renderdoc/usr/lib/librenderdoc.so'
    helper = out / 'gl-capture-control.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-O2',
                    '-I' + str(ROOT / '.tools/renderdoc/usr/include'),
                    str(ROOT / 'tools/xemu_gl_capture_control.c'), '-o', str(helper),
                    '-ldl', '-lpthread'], check=True)
    processes = []
    success = False
    qp = None
    xemu = None
    try:
        read_fd, write_fd = os.pipe()
        with (out / 'xvfb.log').open('w') as log:
            xvfb = subprocess.Popen(['Xvfb', '-displayfd', str(write_fd), '-screen', '0',
                                      '640x480x24', '-nolisten', 'tcp', '-noreset'],
                                     pass_fds=(write_fd,), stdout=log, stderr=subprocess.STDOUT)
        processes.append(xvfb)
        os.close(write_fd)
        try:
            assert select.select([read_fd], [], [], 10)[0]
            display = ':' + os.read(read_fd, 64).decode().strip()
        finally:
            os.close(read_fd)
        assert display[1:].isdigit()
        with socket.socket() as reservation:
            reservation.bind(('127.0.0.1', 0))
            gdb_port = reservation.getsockname()[1]
        qp = Path('/tmp') / f'h2-weapon-gpu-{xvfb.pid}.sock'
        assert not qp.exists()
        env = dict(os.environ, DISPLAY=display, LIBGL_ALWAYS_SOFTWARE='1',
                   SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
                   __GLX_VENDOR_LIBRARY_NAME='mesa',
                   __EGL_VENDOR_LIBRARY_FILENAMES='/usr/share/glvnd/egl_vendor.d/50_mesa.json',
                   XDG_CONFIG_HOME=str(out / 'profile'), XDG_DATA_HOME=str(out / 'profile'),
                   LD_PRELOAD=str(library) + ':' + str(helper), HALO2_RENDERDOC_LIBRARY=str(library),
                   HALO2_GL_CAPTURE_REQUEST=str(out / 'capture-request'),
                   HALO2_CAPTURE_PREFIX=str(out / 'original-weapon'), HALO2_CAPTURE_ALLOW_DEBUGGER='1')
        # The existing capture bridge selects the observed xemu render thread
        # by name. Its AppRun symlink changes that name without changing bytes.
        command = [str(ROOT / '.tools/xemu-app/squashfs-root/usr/bin/xemu'),
                   '-config_path', str(out / 'xemu.toml'), '-S', '-qmp',
                   f'unix:{qp},server=on,wait=off', '-gdb', f'tcp:127.0.0.1:{gdb_port}',
                   '-nic', 'none', '-no-reboot', '-no-shutdown']
        with (out / 'xemu.log').open('w') as log:
            xemu = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
        processes.append(xemu)
        launch = dict(command=command, argv=command, pid=xemu.pid, xvfb_pid=xvfb.pid,
                      display=display, qmp=str(qp), gdb_port=gdb_port, snapshot=args.snapshot,
                      reference='OG Xbox TU1.5', physical_xbox_capture=False,
                      original_session=str(session), original_session_sha256=original_sha,
                      source_evidence_sha256=sha(args.evidence), reference_image_sha256=sha(image),
                      configuration_sha256=sha(out / 'launch-config.toml'), core_limit_bytes=0,
                      read_only_backing_disk_reused=True, disc_image_copied=False,
                      renderdoc_library_sha256=sha(library), helper_sha256=sha(helper),
                      tool_sha256=sha(Path(__file__)), guest_code_patches=[], guest_memory_patches=[])
        write_json(out / 'launch.json', launch)
        wait_for(qp.exists, 15, [xemu])

        def controller_ready():
            tree = qmp(qp, [{'execute': 'human-monitor-command',
                             'arguments': {'command-line': 'info qtree'}}])[-1]['return']
            return 'usb-xbox-gamepad' in tree and 'usb-hub' in tree

        wait_for(controller_ready, 15, [xemu])
        restored = qmp(qp, [{'execute': 'human-monitor-command',
                            'arguments': {'command-line': 'loadvm ' + args.snapshot}},
                           {'execute': 'stop'}])
        assert not restored[1]['return'].strip(), restored
        write_json(out / 'restore.json', restored)
        script = out / 'guest.gdb'
        script.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 15
target remote 127.0.0.1:{gdb_port}
{breakpoint}
python
import gdb,hashlib,json,pathlib,struct,sys
sys.path.insert(0,{str(ROOT / 'tools')!r})
from capture_xbox_tu15_grenade_reference import host_capture,qmp
out=pathlib.Path({str(out)!r});inf=gdb.selected_inferior()
def read(p,n):return bytes(inf.read_memory(p,n))
def word(p):return struct.unpack('<I',read(p,4))[0]
text=read(0x12000,3611948);assert hashlib.sha256(text).hexdigest()=={TEXT_SHA!r}
expected_indices={draw_part['index_u16'] if draw_part else None!r}
observe_fp={args.observe_first_person!r}
indexed_break=gdb.breakpoints()[0]
fp_break=gdb.Breakpoint('*0x168ded',gdb.BP_HARDWARE_BREAKPOINT,internal=True) if observe_fp else None
palette_models={palette_models!r}
palette_break=gdb.Breakpoint('*0x13a2e0',gdb.BP_HARDWARE_BREAKPOINT,internal=True) if palette_models else None
if palette_break:palette_break.condition='('+' || '.join('$edx=='+hex(m) for m in palette_models)+') && *(unsigned char*)($esp+16)!=0'
poses=[]
palettes=[]
def observation_enabled(enabled):
 indexed_break.enabled=enabled
 if fp_break:fp_break.enabled=enabled
 if palette_break:palette_break.enabled=enabled
def observe_palette():
 sp=int(gdb.parse_and_eval('$esp'));ret,nodes,selected,lod,fp,palette=struct.unpack('<6I',read(sp,24))
 model=int(gdb.parse_and_eval('$edx'))&0xffffffff;owner=int(gdb.parse_and_eval('$ecx'))&0xffffffff
 assert model in palette_models and fp&255 and lod<6
 metadata=word(word(0x4f0414)+(model&65535)*16+8);count=word(metadata+0x48);regions=word(metadata+0x1c)
 assert 0<count<=64 and regions<=16
 ordinal=len(palettes);before=read(nodes,count*52);selection=list(read(selected,regions))
 (out/f'palette-{{ordinal}}-input-nodes.bin').write_bytes(before)
 observation_enabled(False)
 stop=gdb.Breakpoint('*'+hex(ret),gdb.BP_HARDWARE_BREAKPOINT,internal=True)
 effective=gdb.Breakpoint('*0x13a357',gdb.BP_HARDWARE_BREAKPOINT,internal=True);effective.condition='$ebp=='+hex(sp-4)
 effective_nodes=None
 try:
  gdb.execute('continue');pc=int(gdb.parse_and_eval('$eip'))
  if pc==0x13a357:
   pointer=int(gdb.parse_and_eval('$eax'))&0xffffffff;effective_nodes=read(pointer,count*52)
   (out/f'palette-{{ordinal}}-effective-nodes.bin').write_bytes(effective_nodes)
   effective.enabled=False;gdb.execute('continue');pc=int(gdb.parse_and_eval('$eip'))
  assert pc==ret
  base_count,region_count=struct.unpack('<HH',read(palette,4));assert region_count==regions
  ranges=[struct.unpack('<HH',read(palette+4+i*4,4)) for i in range(regions)]
  matrices=max([base_count]+[first+length for first,length in ranges]);assert matrices<=256
  raw=read(palette,0x44+matrices*48);name=f'palette-{{ordinal}}-returned.bin';(out/name).write_bytes(raw)
  palettes.append(dict(entry=0x13a2e0,return_pc=ret,owner=owner,model=model,fp=fp&255,lod=lod,node_count=count,selected_sections=selection,base_count=base_count,region_ranges=ranges,matrix_count=matrices,tick=word(word(0x51df54)+8),input_nodes_sha256=hashlib.sha256(before).hexdigest(),effective_nodes_observed=effective_nodes is not None,effective_nodes_equal_input=effective_nodes==before if effective_nodes is not None else None,palette_file=name,palette_sha256=hashlib.sha256(raw).hexdigest()))
  gdb.write('Observed completed original palette '+str(ordinal)+' model '+hex(model)+' matrices '+str(matrices)+'\\n')
 finally:
  effective.delete();stop.delete();observation_enabled(True)
def observe_pose():
 sp=int(gdb.parse_and_eval('$esp'));ret,view,unit,capacity,output=struct.unpack('<5I',read(sp,20))
 assert 0x12000<=ret<0x383d2c and view<4 and 0<capacity<=16
 tick_before=word(word(0x51df54)+8)
 observation_enabled(False)
 stop=gdb.Breakpoint('*'+hex(ret),gdb.BP_HARDWARE_BREAKPOINT,internal=True)
 try:
  gdb.execute('continue');assert int(gdb.parse_and_eval('$eip'))==ret
  count=int(gdb.parse_and_eval('$eax'))&0xffffffff;assert count<=capacity
  rows=[];ordinal=len(poses)
  for index in range(count):
   p=output+index*0xd0c;model=word(p);metadata=word(word(0x4f0414)+(model&65535)*16+8)
   nodes=word(metadata+0x48);assert 0<nodes<=64
   data=read(p+0xc,nodes*52);name=f'fp-pose-{{ordinal}}-model-{{index}}-nodes.bin'
   (out/name).write_bytes(data)
   rows.append(dict(model=model,owner=word(p+4),mode=word(p+8),node_count=nodes,nodes_file=name,nodes_sha256=hashlib.sha256(data).hexdigest()))
  pose=dict(entry=0x168ded,return_pc=ret,view=view,unit=unit,capacity=capacity,output=output,count=count,tick_before=tick_before,tick_after=word(word(0x51df54)+8),models=rows,camera_words=list(struct.unpack('<9I',read(0x4c65c0,36))))
  poses.append(pose)
  gdb.write('Observed completed original FP pose '+str(ordinal)+' models '+str(count)+'\\n')
 finally:
  stop.delete();observation_enabled(True)
def matching_entry():
 while True:
  gdb.execute('continue');pc=int(gdb.parse_and_eval('$eip'))
  if observe_fp and pc==0x168ded:observe_pose();continue
  if palette_break and pc==0x13a2e0:observe_palette();continue
  assert pc=={capture_entry};break
 if expected_indices is None:return True
 sp=int(gdb.parse_and_eval('$esp'));count=word(sp+8);pointer=word(sp+12)
 return list(struct.unpack('<'+'H'*count,read(pointer,count*2)))==expected_indices
for _ in range({args.warm_views}):
 while not matching_entry():pass
def body(datum,kind):
 pool=word(0x4ecbd0);header=read(word(pool+0x44)+(datum&65535)*12,12)
 assert struct.unpack_from('<H',header)[0]==datum>>16
 pointer=struct.unpack_from('<I',header,8)[0];size=struct.unpack_from('<H',header,6)[0]
 data=read(pointer,size);assert data[0xaa]==kind
 return pointer,data
unit,unit_data=body({args.unit},0);weapon,weapon_data=body({args.weapon},2)
assert word(weapon)=={args.definition} and word(weapon+0x14)=={args.unit}
(out/'unit-before.bin').write_bytes(unit_data);(out/'weapon-before.bin').write_bytes(weapon_data)
(out/'camera-before.bin').write_bytes(read(0x4c6634,192))
(out/'view-before.bin').write_bytes(read(0x491e20,0x298))
cache_token=word(unit+0xcc);assert cache_token!=0xffffffff
cache=word(word(0x515f14)+0x44)+(cache_token&65535)*0x100
(out/'unit-render-cache.bin').write_bytes(read(cache,256))
clock=word(0x51df54);before_tick=word(clock+8)
pose_before=len(poses)-1
if observe_fp:
 assert poses and poses[-1]['unit']=={args.unit} and poses[-1]['tick_after']==before_tick
host_capture(out,{xemu.pid},True)
gdb.execute('stepi')
while not matching_entry():pass
if expected_indices is not None:
 for attempt in range(32):
  sp=int(gdb.parse_and_eval('$esp'));count=word(sp+8);pointer=word(sp+12)
  matched=list(struct.unpack('<'+'H'*count,read(pointer,count*2)))==expected_indices
  if matched and word(clock+8)>before_tick:break
  matching_entry()
 else:raise RuntimeError('No subsequent authored draw after source clock advanced')
(out/'camera-after.bin').write_bytes(read(0x4c6634,192))
(out/'view-after.bin').write_bytes(read(0x491e20,0x298))
(out/'weapon-after.bin').write_bytes(body({args.weapon},2)[1])
text=read(0x12000,3611948);assert hashlib.sha256(text).hexdigest()=={TEXT_SHA!r}
host_capture(out,{xemu.pid},False)
(out/'first-person-poses.json').write_text(json.dumps(dict(poses=poses,before_capture_pose=pose_before,after_capture_pose=len(poses)-1),indent=2)+'\\n')
(out/'model-palettes.json').write_text(json.dumps(dict(palettes=palettes),indent=2)+'\\n')
(out/'source.json').write_text(json.dumps(dict(reference='OG Xbox TU1.5',live_text_sha256={TEXT_SHA!r},unit={args.unit},weapon={args.weapon},definition={args.definition},source_tick_before=before_tick,source_tick_after=word(clock+8),capture_entry_before={capture_entry},capture_entry_after={capture_entry},actual_authored_indices_matched=expected_indices is not None,actual_first_person_producer_observed=observe_fp,view_header_source=0x491e20,guest_code_patches=[],guest_memory_patches=[],guest_register_writes=[],controller_inputs=[],GPU_source_boundary_analysis_complete=False),indent=2)+'\\n')
qmp(pathlib.Path({str(qp)!r}),[{{'execute':'stop'}}])
end
delete
detach
quit
''')
        with (out / 'guest.log').open('w') as log:
            subprocess.run(['gdb', '-q', '-nx', '-batch', '-x', str(script)],
                           stdout=log, stderr=subprocess.STDOUT, timeout=60, check=True)
        qmp(qp, [{'execute': 'stop'}])
        captures = list(out.glob('*.rdc'))
        assert len(captures) == 1
        window = x11_window(display)
        subprocess.run(['import', '-display', display, '-window', str(window),
                        str(out / 'original.png')], check=True, timeout=10)
        write_json(out / 'capture.json', dict(capture=captures[0].name,
            capture_sha256=sha(captures[0]), source_sha256=sha(out / 'source.json'),
            guest_script_sha256=sha(script), full_weapon_parity=False))
        qmp(qp, [{'execute': 'quit'}])
        assert xemu.wait(timeout=15) == 0
        assert sha(original_disk) == original_sha
        success = True
        print(json.dumps(dict(capture=str(captures[0]), bytes=captures[0].stat().st_size,
                              code_authenticated=True, native_pixels_compared=False)))
    finally:
        for process in reversed(processes):
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)
        if sha(original_disk) == original_sha and all(p.poll() is not None for p in processes):
            disposable = out / 'session-hdd.qcow2'
            if disposable.exists():
                disposable.unlink()
            if qp is not None:
                qp.unlink(missing_ok=True)
            write_json(out / 'cleanup.json', dict(capture_completed=success,
                original_session_unchanged=True, all_processes_exited=True,
                unnecessary_working_disk_removed=True, base_disk_copied=False,
                disc_image_copied=False))


if __name__ == '__main__':
    main()
