#!/usr/bin/env python3
"""Read actual TU1.5 third-person node/section/palette inputs for one view.

The original game reaches the model-node entry normally. Hardware breakpoints
observe its inputs, effective WORLD nodes and completed palette. No guest code,
pose, clock, camera or weapon state is written. GPU capture is optional and is
ended after the next scene boundary while the guest remains stopped.
"""
import argparse
import json
import hashlib
from pathlib import Path
import subprocess
import time

from capture_xbox_tu15_grenade_reference import ROOT, TEXT_SHA, qmp, host_capture, x11_window


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--owners', type=lambda x: int(x, 0), nargs='+', required=True)
    parser.add_argument('--view', type=int, required=True)
    parser.add_argument('--gpu', action='store_true')
    parser.add_argument('--engine-input', type=Path, action='append', default=[],
                        help='Retain provenance for a declared original-engine API fixture')
    args = parser.parse_args()
    session = args.session.resolve()
    out = args.output.resolve()
    assert ROOT in session.parents and ROOT in out.parents
    engine_inputs = []
    for path in args.engine_input:
        path = path.resolve()
        assert ROOT in path.parents
        record = json.loads(path.read_text())
        assert record['original_text_preserved'] and record['original_player_pose_preserved']
        assert not record['guest_code_patches'] and not record['guest_pose_writes']
        engine_inputs.append(dict(path=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                                  provenance=record['provenance']))
    assert 0 <= args.view < 4 and 1 <= len(args.owners) <= 3
    assert len(set(args.owners)) == len(args.owners) and not out.exists()
    out.mkdir(parents=True)
    launch = json.loads((session/'launch.json').read_text())
    prior_captures = {p.name for p in session.glob('*.rdc')}
    qmp_path = Path(launch['qmp'])
    qmp(qmp_path, [{'execute': 'stop'}])
    if args.gpu:
        # Retain the observed original GL context in the existing private
        # session. This service neither replaces nor adds a guest draw.
        host_capture(session, launch['pid'], True)
    script = out/'capture.gdb'
    script.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 15
target remote 127.0.0.1:{launch['gdb_port']}
python
import gdb,struct,json,pathlib,hashlib,sys
sys.path.insert(0,{str(Path(__file__).resolve().parent)!r})
from capture_xbox_tu15_grenade_reference import qmp
out=pathlib.Path({str(out)!r});inf=gdb.selected_inferior()
def read(p,n):return bytes(inf.read_memory(p,n))
def word(p):return struct.unpack('<I',read(p,4))[0]
def reg(n):return int(gdb.parse_and_eval('$'+n))&0xffffffff
def stop_at(pc,condition=None):
 b=gdb.Breakpoint('*'+hex(pc),gdb.BP_HARDWARE_BREAKPOINT,internal=True)
 if condition:b.condition=condition
 gdb.execute('continue');b.delete()
text=read(0x12000,3611948)
assert hashlib.sha256(text).hexdigest()=={TEXT_SHA!r}
(out/'tu15-text.bin').write_bytes(text)
rows=[];remaining={args.owners!r}
while remaining:
 condition='('+' || '.join('$ecx=='+hex(o) for o in remaining)+') && *(unsigned*)0x4c6870=={args.view} && *(unsigned char*)($esp+16)==0'
 # Whole original13A2E0 is137800's TU1.5 model-node entry. Its literal
 # ECX/EDX plus five stack arguments identify the actual owner/model/bank.
 stop_at(0x13a2e0,condition)
 sp=reg('esp');owner=reg('ecx');model=reg('edx')
 ret=word(sp);nodes,selected,lod,fp,palette=struct.unpack('<5I',read(sp+4,20))
 assert owner in remaining and fp&255==0 and lod<6
 metadata=word(word(0x4f0414)+(model&65535)*16+8)
 count=word(metadata+0x48);assert 0<count<=256
 before=read(nodes,count*52)
 # Literal13A357 consumes EAX after original BF640 and its early gates.
 # The EBP condition keeps nested original calls outside this observation.
 stop_at(0x13a357,'$ebp=='+hex(sp-4))
 effective=reg('eax');raw=read(effective,count*52)
 (out/(f'owner-{{owner:08x}}-nodes.bin')).write_bytes(raw)
 sections=list(read(selected,word(metadata+0x1c)))
 stop_at(ret)
 base_count,regions=struct.unpack('<HH',read(palette,4));assert regions<=16
 ranges=[struct.unpack('<HH',read(palette+4+i*4,4))for i in range(regions)]
 matrices=max([base_count]+[first+n for first,n in ranges]);assert matrices<=256
 (out/(f'owner-{{owner:08x}}-palette.bin')).write_bytes(read(palette,0x44+matrices*48))
 headers=word(word(0x4ecbd0)+0x44);header=read(headers+(owner&65535)*12,12)
 body=struct.unpack_from('<I',header,8)[0];extent=struct.unpack_from('<H',header,6)[0]
 data=read(body,extent);(out/(f'owner-{{owner:08x}}-body.bin')).write_bytes(data)
 token=word(body+0xcc);assert token!=0xffffffff
 cache=word(word(0x515f14)+0x44)+(token&65535)*0x100
 (out/(f'owner-{{owner:08x}}-cache.bin')).write_bytes(read(cache,256))
 (out/'camera.bin').write_bytes(read(0x4c6634,192))
 (out/'view-header.bin').write_bytes(read(0x491e20,0x298))
 (out/'render-policy.json').write_text(json.dumps(dict(ordinary_maximum=word(0x4c683c),
  feature_bytes=list(read(0x4c6840,6)),coverage_word=word(0x4c6848),secondary_word=word(0x4c684c),
  effect_word=word(0x473644),owner='Complete original TU1.5 2C470',guest_code_patches=[],guest_memory_patches=[]),indent=2)+'\\n')
 rows.append(dict(owner=owner,model=model,definition=word(body),node_count=count,lod=lod,
  sections=sections,palette_base_count=base_count,palette_matrices=matrices,palette_ranges=ranges,
  changed_nodes=[i for i in range(count)if before[i*52:(i+1)*52]!=raw[i*52:(i+1)*52]],
  eye_words=list(struct.unpack('<3I',read(0x4c6690,12))),
  lighting_words=list(struct.unpack('<21I',read(cache+0x54,84))),
  colors=list(struct.unpack('<4I',read(cache+0xa8,16))),color_count=read(cache+0xb8,1)[0]))
 remaining.remove(owner)
stop_at(0x132b0)
(out/'next-view-header.bin').write_bytes(read(word(reg('esp')+4),0x298))
(out/'game-clock.bin').write_bytes(read(word(0x51df54),0x24))
result=qmp(pathlib.Path({str(qmp_path)!r}),[{{'execute':'stop'}}])
(out/'receipt.json').write_text(json.dumps(dict(reference='OG Xbox TU1.5',physical_xbox_capture=False,
 live_text_sha256=hashlib.sha256(text).hexdigest(),view={args.view},rows=rows,
 guest_code_patches=[],guest_memory_patches=[],original_engine_inputs={engine_inputs!r},gpu_requested={args.gpu!r},pause=result,
 scope='Original completed render inputs and optional GPU commands; no native pixel acceptance'),indent=2)+'\\n')
end
detach
quit
''')
    try:
        with (out/'capture.log').open('w') as log:
            subprocess.run(['gdb', '-q', '-nx', '-batch', '-x', str(script)],
                           stdout=log, stderr=subprocess.STDOUT, timeout=45, check=True)
    finally:
        qmp(qmp_path, [{'execute': 'stop'}])
        if args.gpu:
            host_capture(session, launch['pid'], False)
    window = x11_window(launch['display'])
    subprocess.run(['import', '-display', launch['display'], '-window', str(window),
                    str(out/'original.png')], timeout=10, check=True)
    receipt = json.loads((out/'receipt.json').read_text())
    receipt['completed_at'] = time.time()
    receipt['gpu_captures'] = [str(p) for p in session.glob('*.rdc')
                               if p.name not in prior_captures]
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(output=str(out),owners=args.owners,view=args.view)))


if __name__ == '__main__':
    main()
