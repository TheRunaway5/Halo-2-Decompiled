#!/usr/bin/env python3
"""Measure original gain/pitch helpers and device-application CPU boundaries.

The three genuine whole Xbox-in-xemu calls must agree first. On the host,
unchanged helpers run completely; application fragments run after the real
lock boundary, before FIFO reads, and up to each pending device write. No
FIFO/kernel/DSP result is supplied. Declared ABI states are numerical controls,
not more initialized game calls. Recursive pitch uses explicit caller fragments
before/after each child call, with each child CPU body measured separately.
"""
import argparse
import atexit
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from original_audio_cpu import OriginalAudioCpu

sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
unpack = lambda raw, at=0: struct.unpack_from('<I', raw, at)[0]
put = lambda raw, at, value: struct.pack_into('<I', raw, at, value & 0xffffffff)
ALIASES = {name: 0x1001000 + i * 0x1000 for i, name in enumerate((
    'voice', 'data', 'parameters', 'parent_link', 'parent_owner', 'parent_data',
    'parent_parameters', 'filter_override'))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', action='append', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    image_path = ROOT / 'assets/disc/default.xbe'
    assert sha(image_path) == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    captures = {}
    for path in args.capture:
        capture = json.loads((path / 'report.json').read_text())
        assert capture['image_sha256'] == sha(image_path)
        assert capture['device_writes_executed_normally']
        assert not any(capture[key] for key in ('instruction_patches', 'guest_memory_writes',
                                               'guest_register_writes', 'supplied_callee_returns'))
        captures[capture['kind']] = (path, capture)
    assert set(captures) == {'gain', 'pitch', 'filter'}
    layout = args.output / 'user-layout.c'
    layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc', str(layout), '-o', str(args.output / 'user-layout')], check=True)
    subprocess.run([str(args.output / 'user-layout')], check=True)
    image = Xbe(image_path)
    host = OriginalAudioCpu(image, args.output / 'original32.elf', 0x27f, 0x1f80)
    atexit.register(host.close)
    code = _Code(image, 'DSOUND')
    scratch, frame, stack, stop = 0x100d000, 0x100e000, 0x100f000, 0x3000000
    rows, visited = [], set()
    commands = {
        'gain': {0x381d2c: (0xfe8202f8, 'rdx'), 0x381d36: (0xfe8202fc, 1),
                 0x381d40: (0xfe820360, 'rdx'), 0x381d4a: (0xfe820364, 'rdx'),
                 0x381d54: (0xfe820368, 'rdx'), 0x381d5a: (0xfe8202fc, 0)},
        'pitch': {0x381de6: (0xfe8202f8, 'rdx'), 0x381def: (0xfe82037c, 'rdx')},
        'filter': {0x38180d: (0xfe8202f8, 'rsi'), 0x381813: (0xfe8202fc, 'rdx'),
                   0x38181c: (0xfe820318, 'rsi'), 0x381825: (0xfe820374, 'rsi'),
                   0x38182e: (0xfe820378, 'rsi'), 0x381834: (0xfe8202fc, 0)}}
    specs = {'gain': (0x381cf9, 0x381d13, 0x381d20, 0x381d6f, 'rsi'),
             'pitch': (0x381d92, 0x381dcd, 0x381dda, 0x381e00, 'rsi'),
             'filter': (0x381727, 0x3817f4, 0x381801, 0x381849, 'rbx')}

    def run(entry, boundary, traced=True, **registers):
        stops = tuple(boundary) if isinstance(boundary, (list, tuple)) else (boundary,)
        # Single-step observations need no enabled hardware breakpoints.
        host.trace(6, host.debug_offset + 7 * 8, 0)
        if traced:
            result, stores, executed = host.run_traced(entry, stack, code,
                ALIASES['voice'], 0x8000, stop_addresses=stops, **registers)
            visited.update(executed)
            return result, stores
        assert len(stops) <= 4
        for i, pc in enumerate(stops): host.trace(6, host.debug_offset + i * 8, pc)
        host.trace(6, host.debug_offset + 7 * 8, sum(1 << (2*i) for i in range(len(stops))))
        host.prepare(entry, stack, registers);host.trace(7);result = host.wait_stop()
        assert result.rip in stops
        return result, []

    def normalise(capture, field):
        state = {region['label']: bytearray.fromhex(region[field]) for region in capture['regions']}
        put(state['voice'], 0x70, ALIASES['data'])
        links = [('data', 0xb8, 'parameters'), ('data', 0xb0, 'parent_link'),
                 ('parent_link', 0x20, 'parent_owner'), ('parent_owner', 0x80, 'parent_data'),
                 ('parent_data', 0xb8, 'parent_parameters')]
        for name, offset, target in links:
            if name in state and unpack(state[name], offset):
                assert target in state, (name, target)
                put(state[name], offset, ALIASES[target])
        return state

    def measure(kind, label, state, override=False, actual=None, helper=False,
                voice_identity=None, edits=()):
        for name, raw in state.items(): host.write(ALIASES[name], raw)
        host.write(frame - 0x80, bytes([0xa5]) * 0x100)
        voice = ALIASES['voice'] if voice_identity is None else voice_identity
        output_before = bytes((i * 17 + 91) & 255 for i in range(36 if kind == 'gain' else 4))
        writes, stores = [], []
        if helper:
            assert kind in ('gain', 'pitch')
            host.write(scratch, output_before)
            host.write(stack, struct.pack('<II', stop, scratch))
            result, _ = run(0x380b97 if kind == 'gain' else 0x380deb, stop, rcx=voice)
            assert result.rsp == stack + 8
            output_after = host.read(scratch, len(output_before))
        else:
            first, fifo, after_fifo, unlock, voice_reg = specs[kind]
            filter_arg = ALIASES['filter_override'] if override else 0
            host.write(frame + 8, struct.pack('<I', filter_arg))
            result, prefix_stores = run(first, (fifo, unlock), rbp=frame,
                rsp=frame-0x40, **{voice_reg: voice})
            stores.extend(prefix_stores)
            if result.rip == fifo:
                entry = after_fifo
                while True:
                    inputs = {name: getattr(result, name) for name in
                              ('rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'rsp', 'eflags')}
                    result, fragment_stores = run(entry, (*commands[kind], unlock), **inputs)
                    stores.extend(fragment_stores)
                    if result.rip == unlock: break
                    address, value = commands[kind][result.rip]
                    value = getattr(result, value) & 0xffffffff if isinstance(value, str) else value
                    writes.append(dict(address=f'{address:08x}', value=f'{value:08x}'))
                    for edit in edits:
                        if edit['after_command']==len(writes):
                            host.write(edit['address'],bytes.fromhex(edit['hex']))
                    # Start the next unchanged CPU fragment after the pending
                    # MMIO instruction. No device result or callee return is
                    # injected; these explicit fragments do not execute MMIO.
                    entry = result.rip + code.at(result.rip).size
            output_after = None
            if actual is not None:
                assert writes == [{k: w[k] for k in ('address', 'value')}
                                   for w in actual['observed_device_writes']], label
                expected = normalise(actual, 'after_hex')
                assert all(host.read(ALIASES[name], len(raw)) == raw for name, raw in expected.items()), label
        after = {name: host.read(ALIASES[name], len(raw)).hex() for name, raw in state.items()}
        rows.append(dict(operation=kind + ('_helper' if helper else ''), label=label,
            actual_whole_capture=actual is not None, filter_override=override,
            voice_identity=f'{voice:08x}',expected_lock_count=0 if helper else 1,external_edits=list(edits),
            regions=[dict(name=name, address=f'{ALIASES[name]:08x}', before_hex=raw.hex(),
                          after_hex=after[name]) for name, raw in state.items()],
            output_before_hex=output_before.hex() if helper else None,
            output_after_hex=output_after.hex() if helper else None, writes=writes,
            stores=[dict(instruction=f'{pc:08x}', address=f'{address:08x}', bytes=size,
                         before=f'{old:08x}', after=f'{new:08x}')
                    for pc, address, size, old, new in stores]))
        return rows[-1]

    for kind, (_, capture) in captures.items():
        state = normalise(capture, 'before_hex')
        measure(kind, f'actual_{kind}_commands', state,
                override=bool(capture.get('filter_override') and int(capture['filter_override'],16)), actual=capture)
        if kind in ('gain', 'pitch'):
            measure(kind, f'actual_{kind}_helper_cpu', state, helper=True)
            raw = bytes.fromhex(capture['actual_helper_output_hex'])
            expected = bytes.fromhex(rows[-1]['output_after_hex'])
            indices = [i + j * 3 for i in range(state['voice'][0x64]) for j in range(3)] if kind == 'gain' else [0]
            assert all(expected[i*4:i*4+4] == raw[i*4:i*4+4] for i in indices)

    def seed(kind):
        state = normalise(captures[kind][1], 'before_hex')
        sizes = {'parameters': 0x38, 'parent_link': 0x24, 'parent_owner': 0x84,
                 'parent_data': 0xbc, 'parent_parameters': 0x38, 'filter_override': 0x18}
        for name, size in sizes.items(): state.setdefault(name, bytearray(size))
        put(state['data'], 0xb0, 0)
        # Captured ordinary calls can legitimately have no parameter object.
        # Declared spatial controls require their explicitly backed identity.
        put(state['data'], 0xb8, ALIASES['parameters'])
        return state

    def emit(kind, label, state, override=False):
        measure(kind, label, state, override=override)
        if kind in ('gain', 'pitch'):
            measure(kind, label+'_helper', state, helper=True)

    words = [0, 1, 15, 16, 99, 100, 101, 8190, 8191, 8192, 0x7fffffff,
             0x80000000, 0xffffffff, (-10000)&0xffffffff, (-10001)&0xffffffff,
             (-32767)&0xffffffff, (-32768)&0xffffffff]
    for count in (1,2,3):
        for routes in (0,1,2,3,4,5,6,7,8,9,16,0xffffffff):
            for enabled in (0,0x1000000):
                s=seed('gain');s['voice'][0x64]=count
                put(s['data'],0x24,routes);put(s['data'],8,enabled)
                emit('gain',f'gain_count_routes_{count}_{routes:08x}_{enabled:08x}',s)
    for slot in range(8):
        for route in range(32):
            s=seed('gain');s['data'][0x28+slot]=route;put(s['data'],8,0x1000000)
            put(s['data'],0x24,8);put(s['data'],0x1c,0)
            for offset in range(0x30,0xb0,4): put(s['data'],offset,(-offset*13)&0xffffffff)
            for offset in range(4,0x24,4): put(s['parameters'],offset,(-offset*17)&0xffffffff)
            emit('gain',f'gain_route_{slot}_{route}',s)
    for name, offsets in [('data',(0x1c,0x30,0x34)),('parameters',tuple(range(4,0x24,4)))]:
        for offset in offsets:
            for value in words:
                s=seed('gain');put(s['data'],8,0x1000000);put(s[name],offset,value)
                emit('gain',f'gain_word_{name}_{offset:02x}_{value:08x}',s)
    for flags in (0,0x1000000,0x2000,0x80000,0x100000,0x182000,0xffffffff):
        for absent in (False,True):
            s=seed('pitch');put(s['data'],8,flags)
            if absent: put(s['data'],0xb8,0)
            # Special pitch sources traverse this initialized empty list;
            # measure only the complete helper until recursive CPU proof.
            if flags&0x182000:
                measure('pitch',f'pitch_special_helper_{flags:08x}_{absent}',s,helper=True)
            else: emit('pitch',f'pitch_flags_{flags:08x}_{absent}',s)
    for name, offset in [('data',0x18),('parameters',0x24),('parent_data',0x18),('parent_parameters',0x24)]:
        for value in words:
            for parent_mode in range(4):
                s=seed('pitch');put(s['data'],8,0x1000000);put(s[name],offset,value)
                if parent_mode:
                    put(s['data'],0xb0,ALIASES['parent_link'])
                    if parent_mode>=2: put(s['parent_link'],0x20,ALIASES['parent_owner'])
                    put(s['parent_owner'],0x80,ALIASES['parent_data'])
                    if parent_mode>=3: put(s['parent_data'],0xb8,ALIASES['parent_parameters'])
                emit('pitch',f'pitch_word_{name}_{value:08x}_{parent_mode}',s)
    for active in (0,1):
        for count in (0,1,2,3):
            for flags in (0,0x10,0x200000,0x1000000,0x1200010):
                for override in (False,True):
                    s=seed('filter');s['voice'][0x12]=active;s['voice'][0x64]=count
                    put(s['data'],8,flags)
                    emit('filter',f'filter_state_{active}_{count}_{flags:08x}_{override}',s,override)
    for name, offsets in [('voice',(0x18,0x38,0x3c)),('parameters',(0x30,0x34)),
                           ('filter_override',tuple(range(0,0x18,4)))]:
        for offset in offsets:
            for value in words:
                s=seed('filter');put(s['data'],8,0x1200010);put(s[name],offset,value)
                emit('filter',f'filter_word_{name}_{offset:02x}_{value:08x}',s,name=='filter_override')
    rng=random.Random(0x4d435058)
    for kind in ('gain','pitch','filter'):
        for i in range(128):
            s=seed(kind);s['voice'][0x64]=i%3+1
            put(s['data'],8,(i%2)*0x1000000)
            for offset in range(0x18,0x24,4): put(s['data'],offset,rng.getrandbits(32))
            for offset in range(0x30,0xb0,4): put(s['data'],offset,rng.getrandbits(32))
            for offset in range(4,0x38,4): put(s['parameters'],offset,rng.getrandbits(32))
            if kind=='gain':
                put(s['data'],0x24,rng.randrange(0,10))
                for slot in range(8):s['data'][0x28+slot]=rng.randrange(32)
            for offset in range(0,0x18,4):put(s['filter_override'],offset,rng.getrandbits(32))
            emit(kind,f'varied_{kind}_{i}',s,kind=='filter' and i%2==0)
    for kind in ('gain','pitch'):
        s=seed(kind);s['voice'][0x12]=0;s['voice'][0x64]=0
        measure(kind,f'inactive_{kind}',s)
    s=seed('pitch');s['voice'][0x64]=0
    measure('pitch','zero_count_pitch',s)
    # Declared external retained changes at command boundaries check original
    # reload behavior. This is not a claimed real backend concurrency event.
    for kind in ('gain','pitch','filter'):
        for replacement in (0,1,2):
            s=seed(kind);s['voice'][0x64]=3
            edits=[dict(after_command=6 if kind!='pitch' else 2,
                        address=ALIASES['voice']+0x64,hex=f'{replacement:02x}')]
            measure(kind,f'reload_count_{kind}_{replacement}',s,edits=edits)
        if kind=='filter':
            s=seed(kind)
            edits=[dict(after_command=3,address=ALIASES['voice']+0x38,hex='78563412')]
            measure(kind,'reload_filter_coefficient',s,edits=edits)

    # Original list controller fragments retain its real ESI/EDI link state.
    # Each reached child body is measured at its own post-lock CPU boundary.
    # No child/kernel return is provided to original code: the post-call
    # fragment uses only the unchanged original callee-saved list registers.
    for children in (0,1,2):
        for inactive in (False,True):
            s=seed('pitch');put(s['data'],8,0x182000)
            sentinel=ALIASES['voice']+0x44
            identities=[0x1009000+i*0x2000 for i in range(children)]
            links=[address+0x5c for address in identities]+[sentinel]
            put(s['voice'],0x44,links[0])
            for i,address in enumerate(identities):
                name=f'child_voice_{i}';data_name=f'child_data_{i}'
                ALIASES[name]=address;ALIASES[data_name]=address+0x1000
                s[name]=bytearray(s['voice']);s[data_name]=bytearray(s['data'])
                put(s[name],0x70,ALIASES[data_name]);put(s[name],0x5c,links[i+1])
                s[name][0x64]=i+1;s[name][0x12]=0 if inactive and i==0 else 1
                put(s[data_name],8,0x1000000);put(s[data_name],0x18,0x1234+i*23)
            for name,raw in s.items():host.write(ALIASES[name],raw)
            parent,_=run(0x381d92,(0x381da8,0x381e00),rsi=ALIASES['voice'],rbp=frame)
            measured=[];child_writes=[]
            while parent.rip!=0x381e00:
                child=parent.rcx&0xffffffff;assert child in identities
                measured.append(child)
                child_row=measure('pitch',f'recursive_child_{children}_{inactive}_{len(measured)}',s,voice_identity=child)
                child_writes.extend(child_row['writes'])
                saved={name:getattr(parent,name) for name in ('rbx','rsi','rdi','rbp','rsp','eflags')}
                parent,_=run(0x381dad,(0x381da8,0x381e00),**saved)
            assert measured==identities
            rows.append(dict(operation='pitch',label=f'recursive_pitch_{children}_{inactive}',
                actual_whole_capture=False,filter_override=False,voice_identity=f'{ALIASES["voice"]:08x}',
                expected_lock_count=children+1,external_edits=[],output_before_hex=None,output_after_hex=None,
                regions=[dict(name=name,address=f'{ALIASES[name]:08x}',before_hex=raw.hex(),after_hex=raw.hex())
                         for name,raw in s.items()],writes=child_writes,stores=[]))
    for pc in visited:
        instruction=code.at(pc)
        assert image.read(pc,instruction.size)==host.read(pc,instruction.size)==instruction.bytes
    host.close()
    fixture=args.output/'fixtures.json'
    fixture.write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_DEVICE_CPU_V1',aliases=ALIASES,cases=rows),separators=(',',':'))+'\n')
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),original_image_sha256=sha(image_path),
        capture_sha256={str(path/'report.json'):sha(path/'report.json') for path,_ in captures.values()},
        host_runner_sha256=sha(ROOT/'tools/original_audio_cpu.py'),fixture_sha256=sha(fixture),
        genuine_whole_command_calls=3,declared_cases=len(rows)-3,unchanged_executed_instructions=len(visited),
        ordered_retained_stores=sum(len(row['stores']) for row in rows),
        command_words=sum(len(row['writes']) for row in rows),supplied_callee_returns=[],
        instruction_patches=[],native_or_physical_DSP_claimed=False,
        recursive_pitch_cpu_caller_fragment_cases=6,recursive_pitch_initialized_capture=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
