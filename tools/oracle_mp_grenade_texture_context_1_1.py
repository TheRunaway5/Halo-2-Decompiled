#!/usr/bin/env python3
"""Genuine texture-cache constructor/reset before the bounded grenade world.

Source-owned raw graphics arena capacity remains absent. No graphics backing,
ready/admission/cache result or gameplay success is supplied. The actual source
constructor and reset either return naturally or retain their next dependency.
"""
import argparse
import ctypes
import json
import mmap
import os
import subprocess
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
import oracle_mp_grenade_action_1_1 as prior
from oracle_mp_spawn_process_heap_world import KernelBoundary

motion=prior.motion
ROOT=prior.ROOT
CONSTRUCTORS={0x12d840,0x12d950,0x13e8e0,0x13f020,0x13eae0,0x12ea60,0x12e570}

class TextureContextTrace(prior.GrenadeTrace):
    def __init__(self,*args,**kwargs):
        self.texture_context_enabled=False
        self.texture_raw_allocations=[]
        self.texture_raw_backing=[]
        super().__init__(*args,**kwargs)

    def hook(self,guest,address,size,user):
        if self.texture_context_enabled and address==self.u(0x417ba8):
            stack=guest.reg_read(UC_X86_REG_ESP)
            values=[self.u(stack+4+n*4) for n in range(3)]
            self.new_invalid=dict(kind='unavailable_original_platform_service',instruction=f'{address:08x}',
                                  kernel_ordinal=address&0x7fffffff,original_data_thunk='00417ba8',
                                  original_return_site=f'{self.u(stack):08x}',original_args=[f'{v:08x}' for v in values],
                                  dependency='MmSetAddressProtect PAGE_READWRITE|PAGE_WRITECOMBINE request: no unsupported Xbox cache-policy/platform return supplied',
                                  source_or_platform_result_supplied=False)
            raise KernelBoundary()
        if self.texture_context_enabled and address==0x33785c:
            stack=guest.reg_read(UC_X86_REG_ESP)
            values=[self.u(stack+4+n*4) for n in range(4)]
            assert values[0]==0 and values[2:]==[0x101000,4] and 0<values[1]<=0x1000000,values
            capacity=(values[1]+4095)&~4095
            alias=self.pool_next
            assert all(alias+capacity<=lo or alias>hi for lo,hi,p in guest.mem_regions()),hex(alias)
            backing=mmap.mmap(-1,capacity,prot=mmap.PROT_READ|mmap.PROT_WRITE)
            host=ctypes.addressof(ctypes.c_char.from_buffer(backing))
            guest.mem_map_ptr(alias,capacity,3,host)
            self.pool_next+=capacity
            self.texture_raw_backing.append(backing)
            self.texture_raw_allocations.append(dict(phase=self.active,original_entry='0033785c',original_return_site=f'{self.u(stack):08x}',
                                                     original_args=[f'{v:08x}' for v in values],guest_raw_alias=f'{alias:08x}',capacity=capacity,
                                                     actual_host_mmap_storage=True,source_constructor_or_admission_result_supplied=False))
            destination=self.u(stack)
            guest.reg_write(UC_X86_REG_EAX,alias)
            guest.reg_write(UC_X86_REG_ESP,stack+20)
            guest.reg_write(UC_X86_REG_EIP,destination)
            return
        super().hook(guest,address,size,user)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=prior.Path,required=True)
    parser.add_argument('--frames',type=int,default=22)
    args=parser.parse_args();assert args.frames>=2
    pins={**motion.FROZEN,'tools/oracle_mp_command_locomotion_pose_1_1.py':motion.digest(ROOT/'tools/oracle_mp_command_locomotion_pose_1_1.py'),
          'tools/oracle_mp_grenade_action_1_1.py':motion.digest(ROOT/'tools/oracle_mp_grenade_action_1_1.py')}
    motion.ENTRIES.update(prior.ENTRIES|CONSTRUCTORS)
    motion.STATE_ENTRIES.update(prior.ENTRIES|CONSTRUCTORS)
    motion.CommandPoseTrace=TextureContextTrace
    process=subprocess.Popen(['wine',str(motion.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
                             env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.command.CONTEXT_OUT/'wine-prefix')))
    try:
        context=json.loads(process.stdout.readline())
        trace,correlation=motion.prepare(context)
        # Restore the exact source-created pre-command fixture, then append only
        # the original constructors. No owner/header/context byte is substituted.
        for lo,hi,permissions,raw in trace.source_fixture:trace.g.mem_write(lo,raw)
        trace.g.context_restore(trace.source_fixture_cpu)
        trace.texture_context_enabled=True
        trace.observe=True;trace.probe='source_texture_cache_context'
        constructor_stages=[]
        def cache_state():
            pointer=trace.u(0x4eda34)
            return dict(texture_cache=f'{pointer:08x}',texture_storage=f'{trace.u(0x4eda30):08x}',
                        original_arena_fields_hex=trace.raw(0x4ed9f0,0x30),
                        original_allocator_fbf8=f'{trace.u(0x46fbf8):08x}',
                        texture_pool=f'{trace.u(0x4eda24):08x}',predicted_texture_pool=f'{trace.u(0x4eda28):08x}',
                        page_allocator8c_hex=None if not pointer else trace.raw(pointer,0x8c))
        for name,entry in [('original_texture_cache_constructor',0x12d840),('original_texture_cache_map_reset',0x12d950)]:
            before=trace.state_id();prior_state=cache_state()
            ok=trace.stage(name,entry);trace.probe_returns(motion.STOP)
            row=dict(name=name,entry=f'{entry:08x}',before_state=before,after_state=trace.state_id(),before_cache=prior_state,after_cache=cache_state(),
                     completion=trace.player_stages[-1]['completion'],invalid=trace.new_invalid,stopped_eip=f'{trace.g.reg_read(UC_X86_REG_EIP):08x}')
            constructor_stages.append(row)
            print(json.dumps(dict(constructor=row)),flush=True)
            if not ok:break
        trace.observe=False
        cases=[]
        if all(row['completion']=='complete_original_return' for row in constructor_stages) and len(constructor_stages)==2:
            backing=[(lo,hi,p,bytes(trace.g.mem_read(lo,hi-lo+1))) for lo,hi,p in trace.g.mem_regions()]
            cpu=trace.g.context_save()
            for name,selected in [('constructor_unselected_Q',False),('source_selected_frag_Q',True)]:
                for lo,hi,p,raw in backing:trace.g.mem_write(lo,raw)
                trace.g.context_restore(cpu)
                cases.append(prior.run_case(trace,name,selected,args.frames))
        for path,digest in pins.items():assert motion.digest(ROOT/path)==digest,path
        report=dict(schema=1,tool_sha256=motion.digest(__file__),source_pins=pins,prior_runtime_correlation=correlation,
                    source_metadata=prior.metadata(trace),constructor_stages=constructor_stages,cases=cases,calls=trace.calls,states=trace.states,
                    reads=trace.reads,stores=trace.stores,points=trace.points,instructions=motion.listings(trace,args.output.parent),
                    inherited_services=trace.events,inherited_physical_services=trace.physical_services,
                    texture_raw_allocations=trace.texture_raw_allocations,
                    new_supplied_gameplay_fields=[],new_supplied_source_returns=[],instruction_patches=[],
                    boundaries=['Actual unchanged12D840 texture-cache constructor and12D950 reset run after the source-created player/world/allocator fixture. No graphics arena/storage/capacity/ready or cache-admission result is supplied.',
                                'Original graphics arena fields remain the exact uninitialized source context. Any naturally rejected texture allocation is a recorded absent-graphics boundary, not valid positive Xbox texture-cache admission.',
                                *json.loads((ROOT/'analysis/multiplayer/integration/grenade-action/exploratory/report.json').read_text())['boundaries']])
        motion.frozen_write(args.output,motion.encoded(report))
        print(json.dumps(dict(report=str(args.output),sha256=motion.digest(args.output),cases=[dict(name=c['name'],last=c['stages'][-1],final=c['final']) for c in cases])),flush=True)
    finally:
        if process.poll() is None:
            try:process.stdin.write('x');process.stdin.flush();process.wait(timeout=15)
            except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
        for stream in (process.stdin,process.stdout,process.stderr):stream.close()

if __name__=='__main__':main()
