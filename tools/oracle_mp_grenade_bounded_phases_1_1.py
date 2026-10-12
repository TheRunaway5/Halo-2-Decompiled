#!/usr/bin/env python3
"""Bounded original unit/weapon clock phases on the genuine grenade owner.

This separate experiment does not resume or bypass the frozen full-world texture
stop. It composes only original animation, biped, weapon and pose roots; retains
the inherited fixed clock; and then delivers a declared Q command. No ready,
channel, count, allocation, release or graphics result is supplied.
"""
import argparse, json, os, struct, subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EIP, UC_X86_REG_ESP
import oracle_mp_grenade_action_1_1 as base

motion = base.motion
ROOT = base.ROOT
OUT = ROOT/'analysis/multiplayer/integration/grenade-bounded-phases'
ENTRIES = base.ENTRIES | {0x1d0ed0, 0xc7e90, 0xde960, 0x1021d0, 0x1019f0,
                         0x1098f0, 0x1099b0, 0x1099f0, 0x109a60,
                         0x1cf100, 0xbdf80, 0x112e10, 0xbd360, 0x10cdf0}
PINS = {**motion.FROZEN,
 'tools/oracle_mp_grenade_action_1_1.py': 'b4e3ae8604bbae5bb8219ddba754e9e48bf674d3fdbc2887b5aec9c8214e7341',
 'analysis/multiplayer/integration/grenade-action/manifest.json':'cb72c6ec1bfc4e5045c6bb537dc6f5e4fc91123616b18abbcfb5e5b0277493bf'}


class PhaseTrace(base.GrenadeTrace):
    def probe_returns(self, address):
        # Recursive action dispatch shares return addresses. A callback can only
        # return when the actual ESP agrees with its own unchanged RET ABI.
        current_sp=self.g.reg_read(UC_X86_REG_ESP)
        for ret,entry_sp,row in self.pending[:]:
            if ret!=address:continue
            routine=self.routines[int(row['entry'],16)]
            instructions=(self.code.at(a) for a in routine.code)
            deltas={4+(int(i.op_str,0) if i.op_str else 0)
                for i in instructions if i.mnemonic=='ret'}
            if current_sp-entry_sp not in deltas:continue
            row.update(return_order=self.order,return_cpu=self.cpu(),return_esp_delta=current_sp-entry_sp,
                after_packets=[dict(p,raw_hex=self.raw(int(p['pointer'],16),p['size']) if int(p['pointer'],16) else None)
                    for p in row['packets']])
            self.order+=1
            if int(row['entry'],16) in motion.STATE_ENTRIES:row['after_state']=self.state_id()
            self.pending.remove((ret,entry_sp,row))

    def snapshot(self):
        value = super().snapshot()
        value['carried_weapon_own_states'] = [dict(datum=o['datum'],
            state_178=f'{struct.unpack_from("<I", bytes.fromhex(o["body_hex"]), 0x178)[0]:08x}',
            blocking_17c=struct.unpack_from('<h', bytes.fromhex(o['body_hex']), 0x17c)[0])
            for o in value['objects'] if bytes.fromhex(o['body_hex'])[0xaa] == 2]
        return value


def execute(t, name, neutral_phases):
    t.probe=name; t.observe=True
    begin={k:len(getattr(t,k)) for k in ('calls','states','reads','stores','points')}
    stages=[]; rows=[]; inputs=[]
    def run(label,entry,args=(),regs=()):
        before=t.state_id(); ok=t.stage(name+'_'+label,entry,args,regs); t.probe_returns(motion.STOP)
        stages.append(dict(name=label,entry=f'{entry:08x}',before_state=before,after_state=t.state_id(),
            completion=t.player_stages[-1]['completion'],invalid=t.new_invalid,stopped_eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}'))
        return ok
    def pointer(datum):
        return t.u(t.u(t.u(0x4e78d0)+0x44)+(datum&0xffff)*12+8)
    def blend(label,datum):
        p=pointer(datum); offset=struct.unpack('<h',t.g.mem_read(p+0x12a,2))[0]
        if offset==-1 or t.u(p+offset+0x68)==0xffffffff:return True
        return run(label,0x1d0ed0,regs=((UC_X86_REG_ESI,p+offset),))
    assert run('source_available_grenade_selector',0xcd310,
        regs=((UC_X86_REG_EAX,t.spawned_unit),(UC_X86_REG_ECX,0xffffffff),(UC_X86_REG_EBX,0)))
    selector=struct.unpack('<h',(t.g.reg_read(UC_X86_REG_EAX)&0xffff).to_bytes(2,'little'))[0]
    unit_pointer=pointer(t.spawned_unit)
    weapon=t.u(unit_pointer+0x218) # Actual source unit inventory slot zero.
    assert weapon!=0xffffffff
    for phase in range(neutral_phases+1):
        before=t.state_id()
        if not run(f'phase_{phase}_neutral_command_initializer',0x14e780,regs=((UC_X86_REG_EDX,motion.command.COMMAND),)):break
        flags=0x04000000 if phase==neutral_phases else 0
        initial=t.raw(motion.command.COMMAND,0x5c)
        t.w(motion.command.COMMAND,flags); t.g.mem_write(motion.command.COMMAND+0x22,struct.pack('<h',selector))
        inputs.append(dict(phase=phase,flags=f'{flags:08x}',selector=selector,before5c_hex=initial,after5c_hex=t.raw(motion.command.COMMAND,0x5c),
            declared_fields=['incoming command+0 flags','incoming command+22 grenade selector']))
        record=t.u(0x4d6d44)+0x80+(t.spawned_player&0xffff)*0x88
        sequence=[('submission',0x852a0,(record,motion.command.COMMAND),()),
          ('assembly',0x69350,(t.u(0x4d6d44),motion.command.PACKET),()),
          ('player',0x14eae0,(motion.command.PACKET,),())]
        if not all(run(f'phase_{phase}_{label}',e,args,regs) for label,e,args,regs in sequence):break
        after_player=t.state_id()
        if not blend(f'phase_{phase}_source_unit_blend',t.spawned_unit):break
        if not run(f'phase_{phase}_source_biped_control',0xc7e90,(t.spawned_unit,),((UC_X86_REG_EAX,0xc7e90),)):break
        if not run(f'phase_{phase}_source_biped_motion_animation',0xde960,(t.spawned_unit,),((UC_X86_REG_EAX,0xde960),)):break
        if not blend(f'phase_{phase}_source_weapon_blend',weapon):break
        if not run(f'phase_{phase}_source_weapon_own_update',0x1021d0,(weapon,),((UC_X86_REG_EAX,0x1021d0),)):break
        after_weapon=t.state_id()
        if not run(f'phase_{phase}_source_unit_pose',0xbdf80,(t.spawned_unit,)):break
        if not run(f'phase_{phase}_source_weapon_pose',0xbdf80,(weapon,)):break
        if not run(f'phase_{phase}_source_projectile_postpass',0x1019f0):break
        rows.append(dict(phase=phase,before_state=before,after_player=after_player,after_weapon=after_weapon,after_pose=t.state_id()))
        print(json.dumps(dict(case=name,phase=phase,grenade=t.grenade_state(),weapon_state=t.u(pointer(weapon)+0x178))),flush=True)
    t.observe=False
    unreturned=[r['id'] for _,_,r in t.pending]
    if unreturned:assert stages[-1]['completion']=='blocked_original_instruction'
    t.pending=[]
    return dict(name=name,neutral_phases_before_Q=neutral_phases,actual_weapon_datum=f'{weapon:08x}',source_selector=selector,
        rows=rows,inputs=inputs,stages=stages,ranges={k:[v,len(getattr(t,k))] for k,v in begin.items()},unreturned_original_calls=unreturned,final=t.snapshot())


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--neutral-phases',type=int,default=1); args=parser.parse_args()
    assert 0<=args.neutral_phases<=60
    for name,digest in PINS.items():assert motion.digest(ROOT/name)==digest,name
    motion.ENTRIES.update(ENTRIES); motion.STATE_ENTRIES.update(ENTRIES); motion.CommandPoseTrace=PhaseTrace
    process=subprocess.Popen(['wine',str(motion.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
      env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.command.CONTEXT_OUT/'wine-prefix')))
    try:
        context=json.loads(process.stdout.readline()); t,correlation=motion.prepare(context)
        for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
        t.g.context_restore(t.source_fixture_cpu)
        removed=t.services.pop(0x115650)
        cases=[execute(t,'genuine_owner_bounded_neutral_then_Q',args.neutral_phases)]
        for name,digest in PINS.items():assert motion.digest(ROOT/name)==digest,name
        report=dict(schema=1,tool_sha256=motion.digest(__file__),source_pins=PINS,prior_runtime_correlation=correlation,
          removed_inherited_boundary=dict(entry='00115650',description=removed[0],return_value=removed[1],stack_cleanup=removed[2],removed_only_after_unchanged_preparation=True),
          source_metadata=base.metadata(t),cases=cases,calls=t.calls,states=t.states,reads=t.reads,stores=t.stores,points=t.points,
          instructions=motion.listings(t,args.output.parent),inherited_services=t.events,inherited_physical_services=t.physical_services,
          new_supplied_gameplay_fields=[],new_supplied_source_returns=[],instruction_patches=[],
          return_observer='Actual caller address and ESP delta checked against the unchanged callee RET ABI; recursive action return-address aliases do not manufacture callback completion.',
          boundaries=['A separate composition of the unchanged original biped/weapon animation, control, motion and pose roots on the genuine allocated/joined owner. It does not resume or bypass the separately frozen complete-world texture stop.',
           'The inherited original fixed game clock and dt remain untouched. Repeated bounded phases are not a naturally advancing match-clock, original recursive-world traversal or recovered first-person presentation proof.',
           'Only declared incoming command flags and selector are supplied. Original ready-state, action, grenade admission, animation/channel, inventory, count, allocation and release are not replaced.',
           'Only the inherited 115650 negative-animation service is removed after unchanged preparation, so actual source channel selection executes if reached. All other original platform/resource boundaries remain recorded.'])
        motion.frozen_write(args.output,motion.encoded(report));print(json.dumps(dict(report=str(args.output),sha256=motion.digest(args.output),last=cases[-1]['stages'][-1])),flush=True)
    finally:
        if process.poll() is None:
            try:process.stdin.write('x');process.stdin.flush();process.wait(timeout=15)
            except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
        for stream in (process.stdin,process.stdout,process.stderr):stream.close()

if __name__=='__main__':main()
