#!/usr/bin/env python3
"""Compare reached fatal-cleanup owners on independently prepared original banks.

Preparation uses the existing bounded original Spartan type/animation initializer
or authored netgame item allocator. Arena storage, inactive world/physical banks
and supplied preparation services are recorded boundaries. Compared execution
runs in a fresh Unicorn instance with NO preparation service hooks. Unknown native
owners stop before their first original instruction with the actual ABI. This
does not establish original player birth, active physics or natural hit admission.
"""
import argparse
import json
import re
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
from oracle_spartan_live_pose_1_1 import LivePoseTrace, UI, UNIT, HEADERS, ROOT, PIN, CACHE_PIN
from oracle_vehicle_update_20261011 import capture
from export_mp_physics_contact_lifecycle import segments
from generate_physics_world_bootstrap_runtime_20261011 import sha

MODULES=('unit_animation_selection_runtime.cpp','unit_weapon_routing.cpp',
    'unit_object_runtime_generated.cpp','physics_vehicle_motion_generated.cpp',
    'death_object_phases_generated.cpp','source_damage_delivery_runtime_generated.cpp')
STACK,STOP=0x10ee000,0x10ef000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--state-initializer',action='store_true',
        help='Compare the complete call-free 1EFF60 state initializer on declared dirty storage')
    mode.add_argument('--scale-reset',action='store_true',
        help='Compare F9900 on the independently initialized original Spartan bank')
    mode.add_argument('--item-scale-reset',type=int,choices=(0,1,12),
        help='Compare F9900 on an original allocated authored netgame item')
    mode.add_argument('--item-entity-detach',type=int,choices=(0,1,12),
        help='Compare A8840 on an original allocated authored netgame item')
    mode.add_argument('--item-drop-flags',type=int,choices=(0,1,12),
        help='Compare FB360 on an original allocated authored netgame item')
    mode.add_argument('--unit-action-interrupt',type=lambda value:int(value,0),choices=(0x16,0x1b),
        help='Compare an actual fatal cleanup descriptor callback on the initialized original unit')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    target=UI;body=UNIT
    item_row=next((value for value in (args.item_scale_reset,args.item_entity_detach,args.item_drop_flags) if value is not None),None)
    if item_row is not None:
        from oracle_mp_inventory_world_1_1 import WorldTrace
        original=WorldTrace(item_row,False)
        original.action('prepare_authored_netgame_item',0x15c110,(original.entries,item_row))
        target=original.u(original.entries+item_row*8)
        body=original.u(HEADERS+(target&0xffff)*12+8)
        preparation=dict(kind='bounded original authored item allocation',
            tool_sha256=sha(ROOT/'tools/oracle_mp_inventory_world_1_1.py'),
            row=item_row,active_world=False,
            object=original.object_state(target),heap_requests=original.heap_requests,
            supplied_preparation_services={f'{p:08x}':value for p,value in original.services.items()})
    else:
        original=LivePoseTrace();preparation=original.initialize()
    source=original.g
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for start,end,permissions in source.mem_regions():
        g.mem_map(start,end-start+1,permissions)
        g.mem_write(start,bytes(source.mem_read(start,end-start+1)))
    for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI','EFLAGS','FPCW','FPSW','FPTAG','MXCSR'):
        reg=getattr(R,'UC_X86_REG_'+name);g.reg_write(reg,source.reg_read(reg))
    for n in range(8):
        for prefix in ('XMM','FP'):
            reg=getattr(R,f'UC_X86_REG_{prefix}{n}');g.reg_write(reg,source.reg_read(reg))
    scope=[];sites=set()
    for name in MODULES:
        path=ROOT/'src'/name
        entries={int(p,16) for p in re.findall(r'services\.instruction\(0x([0-9a-f]+)u\)',path.read_text())}
        assert entries;sites.update(entries)
        scope.append(dict(path=str(path.relative_to(ROOT)),sha256=sha(path),instruction_sites=len(entries)))
    scale_reset=args.scale_reset or args.item_scale_reset is not None
    entry=(0x1eff60 if args.state_initializer else 0xa8840 if args.item_entity_detach is not None else
        0xfb360 if args.item_drop_flags is not None else 0xe9a50 if args.unit_action_interrupt==0x16 else
        0xeb830 if args.unit_action_interrupt==0x1b else 0xf9900 if scale_reset else 0xcbdc0)
    arguments=([] if args.state_initializer or args.item_entity_detach is not None or args.item_drop_flags is not None else
        [target,args.unit_action_interrupt] if args.unit_action_interrupt is not None else
        [target,0,0] if scale_reset else [target,0xffffffff,0,0,0])
    g.mem_write(STACK,struct.pack('<'+'I'*(len(arguments)+1),STOP,*arguments));g.reg_write(R.UC_X86_REG_ESP,STACK)
    if args.state_initializer:
        g.mem_write(0x10fd000,bytes((n*29+17)&255 for n in range(0x74)))
        g.reg_write(R.UC_X86_REG_EAX,0x10fd000)
    elif args.item_entity_detach is not None or args.item_drop_flags is not None:
        g.reg_write(R.UC_X86_REG_EAX,target)
    initial_abi=capture(g);trace=[];events=[];exit_entry=[]
    cpu_estimates=[];pending_estimate=[]
    def instruction(_g,p,_n,_u):
        if pending_estimate:
            site,kind,destination,inputs=pending_estimate.pop()
            output=g.reg_read(destination)
            for lane,value in enumerate(inputs):
                cpu_estimates.append(dict(instruction=site,kind=kind,input=value,
                    output=(output>>(lane*32))&0xffffffff))
        if p not in sites:exit_entry.append(p);g.emu_stop();return
        trace.append(p)
        ins=original.code.at(p)
        if ins.mnemonic in ('rcpss','rsqrtss','rsqrtps'):
            destination,operand=ins.op_str.split(', ')
            assert destination.startswith('xmm') and operand.startswith('xmm')
            value=g.reg_read(getattr(R,'UC_X86_REG_'+operand.upper()))
            inputs=[(value>>(lane*32))&0xffffffff for lane in range(4 if ins.mnemonic=='rsqrtps' else 1)]
            pending_estimate.append((p,0 if ins.mnemonic=='rcpss' else 1,
                getattr(R,'UC_X86_REG_'+destination.upper()),inputs))
    def read(_g,_a,p,n,_x,_u):
        events.append(dict(kind='R',address=p,raw=bytes(g.mem_read(p,n)).hex()))
    def write(_g,_a,p,n,value,_u):
        events.append(dict(kind='W',instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,
            before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
    retained=[(0x10fd000,0x74)] if args.state_initializer else [(body,original.u(HEADERS+(target&0xffff)*12+6,2))]
    before={p:bytes(g.mem_read(p,n)) for p,n in retained}
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
    def invalid(_g,access,p,n,_value,_user):
        (args.output/'missing-preparation-bank.json').write_text(json.dumps(dict(
            instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,size=n,access=access,
            executed_instructions=len(trace),boundary=__doc__),indent=2)+'\n')
        return False
    g.hook_add(UC_HOOK_MEM_INVALID,invalid)
    g.emu_start(entry,0,count=300000);assert len(exit_entry)==1
    first={};current={}
    for event in events:
        raw=bytes.fromhex(event['raw'] if event['kind']=='R' else event['before']);p=event['address']
        for n,value in enumerate(raw):
            if p+n not in current:first[p+n]=current[p+n]=value
            assert current[p+n]==value
        if event['kind']=='W':
            for n,value in enumerate(bytes.fromhex(event['written'])):current[p+n]=value
    for p,raw in before.items():
        after=bytes(g.mem_read(p,len(raw)))
        for n,value in enumerate(raw):
            first.setdefault(p+n,value);assert first[p+n]==value
            current.setdefault(p+n,after[n]);assert current[p+n]==after[n]
    complete=exit_entry[0]==STOP;pending=None
    if not complete:
        sp=g.reg_read(R.UC_X86_REG_ESP);stack=bytes(g.mem_read(sp,32))
        pending=dict(instruction=trace[-1],target=exit_entry[0],tail=False,frame_esp=sp,stack32=stack.hex())
        for n,value in enumerate(stack):
            first.setdefault(sp+n,value);current.setdefault(sp+n,value);assert current[sp+n]==value
    case=dict(id=0,entry=entry,entry_abi=initial_abi,exit=capture(g),exit_entry=exit_entry[0],
        return_address=STOP,complete=complete,pending=pending,arguments=arguments,
        initial_banks=[dict(address=p,hex=raw.hex()) for p,raw in segments(first)],
        final_banks=[dict(address=p,hex=raw.hex()) for p,raw in segments(current)],
        instructions=trace,writes=[e for e in events if e['kind']=='W'],cpu_estimates=cpu_estimates,
        declared_input=dict(preparation='bounded original authored item allocation' if item_row is not None else 'bounded original Spartan initializer',
            native_confirmed_fatal_cleanup_arguments=entry==0xcbdc0,
            scale_reset_on_original_initialized_unit=args.scale_reset,
            item_scale_reset_row=args.item_scale_reset,
            item_entity_detach_row=args.item_entity_detach,
            item_drop_flags_row=args.item_drop_flags,
            unit_action_interrupt=args.unit_action_interrupt,
            dirty_state_initializer_storage=args.state_initializer),
        retained_regions=[dict(address=p,size=n) for p,n in retained],metadata_overlays=[])
    fixture=dict(schema=1,image='1.1',cases=[case],native_composition_scope=scope,
        image_sha256=PIN,cache_sha256=CACHE_PIN,instruction_patches=[],native_expected_data_used=False)
    path=args.output/'disc.json';path.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(tool_sha256=sha(Path(__file__)),preparation_tool_sha256=sha(ROOT/'tools/oracle_spartan_live_pose_1_1.py'),
        fixture_sha256=sha(path),preparation=preparation,native_composition_scope=scope,
        boundary=__doc__,instructions=len(trace),writes=len(case['writes']),complete=complete,
        pending=pending,cpu_estimates=len(cpu_estimates),
        native_replay_cpu_estimate_boundary='Literal recorded RCP/RSQRT input/output; no software inverse or owner result',
        compared_execution_service_hooks=0,native_expected_data_used=False,instruction_patches=[])
    (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(instructions=len(trace),writes=len(case['writes']),complete=complete,pending=pending)))


if __name__=='__main__':main()
