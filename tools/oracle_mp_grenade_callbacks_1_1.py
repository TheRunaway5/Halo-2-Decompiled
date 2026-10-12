#!/usr/bin/env python3
"""Separate unchanged grenade callback ABIs; no completed begin is supplied."""
import argparse,json,os,struct,subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_mp_grenade_bounded_phases_1_1 as begin
from oracle_mp_spawn_process_heap_world import KernelBoundary

motion=begin.motion;ROOT=begin.ROOT
OUT=ROOT/'analysis/multiplayer/integration/grenade-callbacks'
ENTRIES=begin.ENTRIES|{0x115800,0xa87e0,0xb9c20,0xb8820,0xb8a30,0x11e720,0x30bd0}
PINS={**begin.PINS,
 'tools/oracle_mp_grenade_bounded_phases_1_1.py':'0d10fc01bfb10aad5341346f8de29241e2f19f412668b1fd5fccb9cf22de2b82',
 'analysis/multiplayer/integration/grenade-bounded-phases/manifest.json':'636e172a83834e2695e3890fd0677ab7472c2c60477cbdc96c50c8d235b2929f'}

class CallbackTrace(begin.PhaseTrace):
    stop_creation=False
    def hook(self,g,address,size,user):
        super().hook(g,address,size,user)
        if self.observe and self.stop_creation and address==0xb8a30:
            sp=g.reg_read(UC_X86_REG_ESP);pointer=self.u(sp+4)
            self.new_invalid=dict(instruction=f'{address:08x}',explicit_source_allocation_request_boundary=True,
                request_pointer=f'{pointer:08x}',descriptor_c4_hex=self.raw(pointer,0xc4),return_address=f'{self.u(sp):08x}',
                allocation_result_supplied=False,instructions_patched=False)
            raise KernelBoundary('Actual original creation request; allocator is not replaced with a result')

def callback(t,name,entry,changes):
    t.probe=name;t.observe=True;start={k:len(getattr(t,k))for k in ('calls','states','reads','stores','points')}
    unit=t.u(t.u(t.u(0x4e78d0)+0x44)+(t.spawned_unit&0xffff)*12+8)
    component=unit+struct.unpack('<h',t.g.mem_read(unit+0x346,2))[0];declared=[]
    for region,offset,value in changes:
        p=(component if region=='action' else unit)+offset;before=t.raw(p,1);t.g.mem_write(p,bytes([value&255]))
        declared.append(dict(region=region,offset=f'{offset:x}',before_hex=before,after_hex=t.raw(p,1),explicit_branch_control=True))
    before=t.state_id();args=(t.spawned_unit,0)if entry==0xe99b0 else (t.spawned_unit,)
    ok=t.stage(name,entry,args);t.probe_returns(motion.STOP);after=t.state_id()
    result=dict(name=name,entry=f'{entry:08x}',arguments=[f'{v:08x}'for v in args],before_state=before,after_state=after,
        completion=t.player_stages[-1]['completion'],returned_al=t.g.reg_read(UC_X86_REG_EAX)&255 if ok else None,
        explicit_branch_controls=declared,invalid=t.new_invalid,ranges={k:[v,len(getattr(t,k))]for k,v in start.items()},
        unreturned_original_calls=[r['id']for _,_,r in t.pending])
    t.pending=[];t.observe=False;return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    for name,h in PINS.items():assert motion.digest(ROOT/name)==h,name
    motion.ENTRIES.update(ENTRIES);motion.STATE_ENTRIES.update(ENTRIES);motion.CommandPoseTrace=CallbackTrace
    proc=subprocess.Popen(['wine',str(motion.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
        env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.command.CONTEXT_OUT/'wine-prefix')))
    try:
        data=json.loads(proc.stdout.readline());t,correlation=motion.prepare(data)
        for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
        t.g.context_restore(t.source_fixture_cpu);t.services.pop(0x115650)
        original_begin=begin.execute(t,'genuine_owner_bounded_neutral_then_Q',1)
        assert original_begin['stages'][-1]['invalid']['instruction']=='0016d2c3'
        assert 0xb8a30 not in t.services
        backing=[(lo,hi,permissions,bytes(t.g.mem_read(lo,hi-lo+1)))for lo,hi,permissions in t.g.mem_regions()]
        cpu=t.g.context_save();t.stop_creation=True;cases=[]
        definitions=[('genuine_partial_update_counter_one',0xe99b0,[]),
          ('partial_direct_create_prefix',0xe8d60,[]),
          ('explicit_phase_one_counter_one',0xe99b0,[('action',0xe,1)]),
          ('explicit_phase_one_counter_126',0xe99b0,[('action',0xe,126)]),
          ('explicit_phase_one_counter_127',0xe99b0,[('action',0xe,127)]),
          ('explicit_phase_one_counter_255',0xe99b0,[('action',0xe,255)]),
          ('explicit_phase_two_before_timer',0xe99b0,[('action',0xc,2),('action',0xe,6)]),
          ('explicit_phase_two_at_timer_empty_held',0xe99b0,[('action',0xc,2),('action',0xe,7)]),
          ('explicit_phase_two_signed_counter_wrap',0xe99b0,[('action',0xc,2),('action',0xe,127)]),
          ('explicit_phase_two_signed_negative_timer',0xe99b0,[('action',0xc,2),('action',0xe,254),('action',0xf,255)]),
          ('explicit_phase_three_actual_throw_channel',0xe99b0,[('action',0xc,3)]),
          ('explicit_phase_zero_counter_wrap',0xe99b0,[('action',0xc,0),('action',0xe,255)]),
          ('explicit_unselected_prepare',0xe8d60,[('unit',0x23c,255)]),
          ('explicit_zero_count_prepare',0xe8d60,[('unit',0x23e,0)]),
          ('explicit_negative_count_prepare',0xe8d60,[('unit',0x23e,128)])]
        for name,entry,changes in definitions:
            for lo,hi,permissions,raw in backing:t.g.mem_write(lo,raw)
            t.g.context_restore(cpu);result=callback(t,name,entry,changes);cases.append(result)
            print(json.dumps(dict(case=name,completion=result['completion'],invalid=result['invalid'])),flush=True)
        for name,h in PINS.items():assert motion.digest(ROOT/name)==h,name
        report=dict(schema=1,tool_sha256=motion.digest(__file__),source_pins=PINS,prior_runtime_correlation=correlation,original_begin=original_begin,
            source_metadata=begin.base.metadata(t),cases=cases,calls=t.calls,states=t.states,reads=t.reads,stores=t.stores,points=t.points,
            instructions=motion.listings(t,a.output.parent),inherited_services=t.events,inherited_physical_services=t.physical_services,
            new_supplied_service_returns=[],instruction_patches=[],
            boundaries=['These are direct original callback ABI compositions against genuinely source-produced partial action state. The blocked original begin callback does not return; normal retained-action dispatch/admission is not supplied or claimed.',
             'The unmodified counter-one case and direct prepare prefix change no gameplay input fields. Separately labeled one-byte phase/counter/timer/current/count controls test exact original branches; they are not naturally produced gameplay state.',
             'Actual original constructor requests are stopped at B8A30 entry after source descriptor production. No allocation, effect, animation, attachment, release or creation success is supplied.',
             'All older fixed clock/local-session/resource/raw-storage/feedback/platform limitations remain explicit.'])
        motion.frozen_write(a.output,motion.encoded(report));print(json.dumps(dict(report=str(a.output),sha256=motion.digest(a.output),cases=len(cases))))
    finally:
        if proc.poll() is None:
            try:proc.stdin.write('x');proc.stdin.flush();proc.wait(timeout=15)
            except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=5)
        for stream in (proc.stdin,proc.stdout,proc.stderr):stream.close()

if __name__=='__main__':main()
