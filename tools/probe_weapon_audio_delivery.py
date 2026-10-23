#!/usr/bin/env python3
"""Observe real shot/reload requests and SDL submissions in a selected runtime.

Run through tools/run_test.py. GDB observes existing call boundaries without
changing game state. MCC supplies runtime content; no Xbox caches or synthetic
sound requests are injected. This does not measure physical speakers or prove
TU1.5 waveform/timing parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
from mcc_content import MccCache
from run_test import stop_process_tree


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


GDB = r'''
set pagination off
set confirm off
set print thread-events off
set debuginfod enabled off
set breakpoint pending on
handle SIGPIPE nostop noprint pass
python
import gdb, json, collections
calls = collections.Counter()
accepted = collections.Counter()
fp = collections.Counter()
source_calls = collections.Counter()
source_returns = collections.Counter()
active = collections.defaultdict(list)
errors = []
def reg(name): return int(gdb.parse_and_eval('$' + name))
def word(at): return int.from_bytes(gdb.selected_inferior().read_memory(at, 4).tobytes(), 'little')
class EndImpulse(gdb.FinishBreakpoint):
    def __init__(self, thread):
        super().__init__(gdb.newest_frame(), internal=True)
        self.thread = thread
    def retire(self):
        if active[self.thread]: active[self.thread].pop()
    def stop(self): self.retire(); return False
    def out_of_scope(self): self.retire()
class Impulse(gdb.Breakpoint):
    def enter(self, key):
        calls[key] += 1
        thread = gdb.selected_thread().global_num
        active[thread].append(key)
        EndImpulse(thread)
        return False
    def stop(self): return self.enter((reg('esi') & 0xffffffff, reg('edx')))
class SourceImpulse(Impulse):
    # A fixed-extent span uses one pointer register in the x86-64 ABI.
    def stop(self): return self.enter((reg('esi') & 0xffffffff, reg('ecx')))
class FirstPerson(gdb.Breakpoint):
    def stop(self):
        event, binding = reg('rcx'), reg('rdx')
        kind = int.from_bytes(gdb.selected_inferior().read_memory(event, 2).tobytes(), 'little')
        if kind == 1 and word(event+4) != 0xffffffff and word(binding) != 0xffffffff and word(binding+12) != 0xffffffff and word(binding+8) & 6 == 6:
            fp[word(event+4)] += 1
        return False
class EndSubmit(gdb.FinishBreakpoint):
    def __init__(self, key):
        super().__init__(gdb.newest_frame(), internal=True)
        self.key = key
    def stop(self):
        if reg('eax') == 0: accepted[self.key] += 1
        else: errors.append('SDL submission rejected: ' + str(self.key))
        return False
class Submit(gdb.Breakpoint):
    def stop(self):
        stack = active[gdb.selected_thread().global_num]
        if stack: EndSubmit(stack[-1])
        return False
class EndSource(gdb.FinishBreakpoint):
    def __init__(self, entry, abi):
        super().__init__(gdb.newest_frame(), internal=True)
        self.entry, self.abi = entry, abi
    def stop(self):
        source_returns[(self.entry, word(self.abi))] += 1
        return False
class Source(gdb.Breakpoint):
    def stop(self):
        entry = reg('esi')
        source_calls[entry] += 1
        EndSource(entry, reg('rdx'))
        return False
Impulse('halo2::MultiplayerAudio::impulse(unsigned int, float, halo2::MultiplayerAudioKind, unsigned int, unsigned int)', internal=True)
Impulse('halo2::MultiplayerAudio::impulse_at(unsigned int, halo2::Vec3, float, halo2::MultiplayerAudioKind, unsigned int)', internal=True)
FirstPerson('halo2::MultiplayerAudio::first_person(unsigned int, halo2::FirstPersonViewBinding const&, halo2::FirstPersonAnimationEvent const&)', internal=True)
SourceImpulse('halo2::MultiplayerAudio::source_impulse', internal=True)
Submit('halo2::SdlSoundPlatform::submit_impl(unsigned long, halo2::SdlSoundPacket)', internal=True)
# The direct installed-route check observes native requests/SDL admission.
# Original instruction tracing belongs to its separate bounded source probes.
end
run
python
rows = [dict(sound=f'{sound:08x}', kind=kind, requests=count, accepted=accepted[(sound,kind)]) for (sound,kind),count in sorted(calls.items())]
print('WEAPON_AUDIO_OBSERVATION=' + json.dumps(dict(requests=rows, first_person_events={f'{k:08x}':v for k,v in fp.items()}, source_calls={f'{k:08x}':v for k,v in source_calls.items()}, source_returns=[dict(entry=f'{k:08x}',result=f'{r:08x}',count=v) for (k,r),v in source_returns.items()], errors=errors)))
end
'''


def classify_observation(map_path, observation):
    """Classify captured unsigned sound ABI identities against real metadata."""
    metadata = {}
    for source in (map_path.parent/'shared.map', map_path):
        cache = MccCache(source)
        try:
            for datum, tag in cache.tags.items():
                if tag['group']=='snd!' and tag['offset'] is not None:
                    at=tag['offset'];metadata[datum]=(cache.u16(at),cache.data[at+2],tag['name'])
        finally: cache.close()
    relevant=[];classes={};mode_errors=[]
    for row in observation['requests'] if observation else []:
        # GDB may expose esi as a signed register value despite the uint32 ABI.
        datum=int(row['sound'],16)&0xffffffff;row['sound']=f'{datum:08x}'
        info=metadata.get(datum)
        if info is None:mode_errors.append('Unbound actual sound datum: '+row['sound']);continue
        flags,sound_class,name=info;row.update(sound_class=sound_class,name=name,remastered_only=bool(flags&0x800))
        if flags&0x800:
            if row['accepted']:mode_errors.append('Remastered-only sound reached SDL: '+name)
            continue
        relevant.append(row);classes[sound_class]=classes.get(sound_class,0)+row['accepted']
    return bool(relevant and all(row['requests']==row['accepted'] for row in relevant)),classes,mode_errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--players', type=int, choices=(1, 2, 4), default=1)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--ticks', type=int, default=260)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    runtime, map_path = args.runtime.resolve(), args.map.resolve()
    with map_path.open('rb') as stream:
        if struct.unpack('<II', stream.read(8)) != (0x68656164, 13):
            raise ValueError('Actual runtime probe requires an MCC version13 cache')
    inputs = scratch / 'fire-reload.inputs'
    inputs.write_text('5 0 1 0\n35 0 0 0\n40 0 0 1\n41 0 0 0\n'
                      '140 0 1 0\n170 0 0 0\n180 0 0 1\n181 0 0 0\n')
    debugger = scratch / 'observe.gdb'
    debugger.write_text(GDB)
    command = ['xvfb-run', '-a', '-s', '-screen 0 800x600x24', 'gdb', '-q',
               '-batch', '-x', str(debugger), '--args', str(runtime),
               '--multiplayer', '--map', str(map_path), '--shared-maps', str(map_path.parent),
               '--local-players', str(args.players), '--mp-starting-weapon', '5',
               '--mp-input-script', str(inputs), '--mp-capture-tick', str(args.ticks),
               '--mp-reference-tick-cadence', '--viewport', '640', '480',
               '--screenshot', str(scratch / 'last.ppm')]
    environment = dict(os.environ, SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
        HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'vulkan-cache'),
        MESA_SHADER_CACHE_DIR=str(scratch/'mesa-cache'),
        __GL_SHADER_DISK_CACHE_PATH=str(scratch/'nvidia-cache'))
    # Keep input rows inside the declared finite capture, including minimized
    # single fire/reload runs.
    inputs.write_text(''.join(line+'\n' for line in inputs.read_text().splitlines()
                              if int(line.split()[0])<args.ticks))
    process=subprocess.Popen(command,env=environment,stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT,text=True,start_new_session=True)
    try:
        output,_=process.communicate(timeout=args.timeout)
        code,timed_out=process.returncode,False
    except subprocess.TimeoutExpired:
        stop_process_tree(process);output,_=process.communicate();code,timed_out=None,True
    finally:
        stop_process_tree(process)
    lines = output.splitlines()
    observation = next((json.loads(line.split('=', 1)[1]) for line in lines
                        if line.startswith('WEAPON_AUDIO_OBSERVATION=')), None)
    audio_lines = [line for line in lines if any(key in line.lower() for key in ('audio:', 'sound ', 'sound:', 'audio: ', 'exited '))]
    delivered,classes,mode_errors=classify_observation(map_path,observation)
    # The pinned MCC schema names class4 weapon_fire and class6 weapon_reload.
    # These are actual resident metadata identities, not filename inference.
    fire_events,reload_events=classes.get(4,0),classes.get(6,0)
    report = dict(command=command, exit_code=code, timed_out=timed_out, observation=observation,
                  passed=bool(code == 0 and delivered and fire_events>0 and reload_events >= (2 if args.ticks>180 else 1) and
                              not mode_errors and
                              not observation['errors'] and any('exited normally' in line for line in lines)),
                  accepted_weapon_fire=fire_events,accepted_weapon_reload=reload_events,mode_errors=mode_errors,
                  runtime_sha256=sha(runtime), map_sha256=sha(map_path), tool_sha256=sha(Path(__file__)),
                  audio_lines=audio_lines[-80:], output_tail=lines[-12:],
                  runner_scratch=str(scratch), large_artifacts_retained=[],
                  physical_speakers_verified=False, full_tu15_audio_parity=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'observation', 'audio_lines', 'output_tail', 'runner_scratch')}, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
