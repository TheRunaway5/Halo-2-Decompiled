#!/usr/bin/env python3
"""Prove the native TU1.5 device update callbacks (1179C0 machine update, 10B840 device update).

Run through tools/run_test.py. Extracts the two unmodified translated bodies
from the checked-in scene device constructor unit (pinned), the unit's own
operand resolver, sign extension and single spill, and the verification
references the production binder generates; compiles
tests/native_tu15_device_update_tests.cpp against them, and compares every
seeded case (ordered store events with preimages, child call log, bank,
registers, flags, XMM, x87 status, MXCSR, cursor; faults by exception, cursor
and store prefix) in Release, census and verify modes and an ASan/UBSan
build. Requires complete reachable branch coverage. Checks the production
binding (install, idempotence, refusal of changed bodies, non-identity
operands, missing children, changed dispatch routes and dispatched targets
that gained a body, references without an unrouted child). Then screens
deliberate defects: each mutant is an out-of-tree copy of the logic or the
adapter placed first on the include path and must be rejected (default mode,
then verify mode, whose replay also orders every read). Source hashes are
rechecked after the screen.
"""
import argparse
import concurrent.futures
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_device_update.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_device_update_systems.hpp'
FRAMEWORK = ROOT / 'src/physics/runtime/native_tu15_object_tick.hpp'
FRAMEWORK_SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_object_tick_systems.hpp'
TEST = ROOT / 'tests/native_tu15_device_update_tests.cpp'
HELPER = ROOT / 'tools/native_device_update_replacements.py'
OWNERS = ('1179c0', '10b840')
UNREACHABLE = set()
# (name, file, anchor, replacement). The logic lives in SYSTEMS and the ABI
# in the HEADER adapter; each mutant is an out-of-tree copy of its file.
MUTANTS = [
    # 1179C0 logic
    ('machine-floor', SYSTEMS, 'const auto floor=flags&object_flags::powered_floor?one:0u;',
     'const auto floor=flags&object_flags::powered_floor?one:0x80000000u;'),
    ('machine-power-test', SYSTEMS, 'if(!x87::above(epsilon,power) || (flags&object_flags::powered_floor)) {',
     'if(x87::above(power,epsilon) || (flags&object_flags::powered_floor)) {'),
    ('machine-cluster-stride', SYSTEMS, 'const auto offset=handle_index(cluster_handle)*std::uint32_t(sizeof(ClusterRecord));',
     'const auto offset=handle_index(cluster_handle)*0x90u;'),
    ('machine-power-choice', SYSTEMS, 'frame.power_cluster(m,record,sse::above(value,floor)?value:floor);',
     'frame.power_cluster(m,record,sse::below(floor,value)?value:floor);'),
    ('machine-animation-graph', SYSTEMS, 'memory.load32(channel_graph,at(m.object,offsetof(Device,position_channel)))!=none &&',
     'memory.load32(channel_graph,at(m.object,offsetof(Device,power_channel)))!=none &&'),
    ('machine-animation-sign', SYSTEMS,
     'm.animation=widen16(memory.load16(animation_offset_again,at(m.object,offsetof(Device,animation_offset))))+m.object;',
     'm.animation=memory.load16(animation_offset_again,at(m.object,offsetof(Device,animation_offset)))+m.object;'),
    ('machine-gear', SYSTEMS, '==machine_types::gear) {', '==machine_types::door) {'),
    ('machine-inverse', SYSTEMS, 'const auto inverse=x87::difference(rate_one_value,', 'const auto inverse=x87::sum(rate_one_value,'),
    ('machine-unpowered-rate', SYSTEMS,
     'x87::load(memory.load32(unpowered_rate,at(m.definition,offsetof(DeviceDefinition,unpowered_rate)))));',
     'x87::load(memory.load32(unpowered_rate,at(m.definition,offsetof(DeviceDefinition,unpowered_acceleration)))));'),
    ('machine-velocity-clear', SYSTEMS, 'memory.store32(velocity_clear,at(m.object,offsetof(Device,position_velocity)),0);',
     'memory.store32(velocity_clear,at(m.object,offsetof(Device,position_velocity)),0x80000000u);'),
    ('machine-rate-magnitude', SYSTEMS, 'if(x87::above(x87::magnitude(rate),epsilon)) {', 'if(x87::above(rate,epsilon)) {'),
    ('machine-step-add', SYSTEMS, 'auto position=sse::add(step,memory.load32(turn_position',
     'auto position=sse::subtract(step,memory.load32(turn_position'),
    ('machine-wrap', SYSTEMS, 'const bool wraps=!sse::below(position,one);', 'const bool wraps=sse::above(position,one);'),
    ('machine-wrap-order', SYSTEMS,
     '                const bool animated=frame.animated();\n'
     '                position=sse::subtract(position,one);\n'
     '                memory.store32(wrap_store,at(m.object,offsetof(Device,position)),position);\n',
     '                position=sse::subtract(position,one);\n'
     '                memory.store32(wrap_store,at(m.object,offsetof(Device,position)),position);\n'
     '                const bool animated=frame.animated();\n'),
    ('machine-wrap-value', SYSTEMS, 'frame.advance_channel(m,0x3f800000u);', 'frame.advance_channel(m,0u);'),
    ('machine-entity-row', SYSTEMS, 'at(entity_rows+m.row,offsetof(ObjectHeader,object))',
     'at(entity_rows+m.row,offsetof(ObjectHeader,unused_here))'),
    ('machine-entity-none', SYSTEMS, 'if(entity!=none)frame.mark_entity(m,entity);', 'if(entity)frame.mark_entity(m,entity);'),
    ('machine-group-value', SYSTEMS, 'return at(element(groups,handle_index(group),sizeof(DeviceGroup)),offsetof(DeviceGroup,value));',
     'return at(element(groups,handle_index(group),sizeof(DeviceGroup)),offsetof(DeviceGroup,initial));'),
    ('machine-door-locked', SYSTEMS, '&machine_flags::door_locked)) {', '&machine_flags::door_stays_open)) {'),
    ('machine-phase', SYSTEMS, 'if(!(phase&3u)) {', 'if(!(phase&1u)) {'),
    ('machine-mode', SYSTEMS, 'if(memory.load8(mode,at(game_globals,offsetof(GameGlobals,mode)))!=4u) {',
     'if(memory.load8(mode,at(game_globals,offsetof(GameGlobals,mode)))!=5u) {'),
    ('machine-radius', SYSTEMS, 'const bool no_radius=sse::above(epsilon,', 'const bool no_radius=sse::below(epsilon,'),
    ('machine-vehicle-bit', SYSTEMS, '>>20)&1u))', '>>21)&1u))'),
    ('machine-unit-state', SYSTEMS, 'offsetof(Unit,flags)))>>2)&1u;', 'offsetof(Unit,flags)))>>1)&1u;'),
    ('machine-unit-flags', SYSTEMS, 'offsetof(UnitDefinition,flags)))>>14)&1u)', 'offsetof(UnitDefinition,flags)))>>13)&1u)'),
    ('machine-player', SYSTEMS, 'one_sided=memory.load32(unit_player,at(m.unit,offsetof(Unit,player)))!=none;',
     'one_sided=memory.load32(unit_player,at(m.unit,offsetof(Unit,player)))==none;'),
    ('machine-door-zero', SYSTEMS, 'if(sse::equal(position,memory.load32(door_zero,device_globals::zero))) {',
     'if(!sse::above(position,memory.load32(door_zero,device_globals::zero))) {'),
    ('machine-marker-sign', SYSTEMS, 'if(signed_value(widen16(frame.find_marker(m)))<=0) {', 'if(signed_value(frame.find_marker(m))<=0) {'),
    ('machine-side-order', SYSTEMS,
     '                    side=sse::add(side,sse::multiply(frame.marker_forward(2),dz));\n'
     '                    side=sse::add(side,sse::multiply(frame.marker_forward(1),dy));\n',
     '                    side=sse::add(side,sse::multiply(frame.marker_forward(1),dy));\n'
     '                    side=sse::add(side,sse::multiply(frame.marker_forward(2),dz));\n'),
    ('machine-side-operand', SYSTEMS, 'const auto dx=sse::subtract(unit_x_value,frame.marker_position(0));',
     'const auto dx=sse::subtract(frame.marker_position(0),unit_x_value);'),
    ('machine-behind', SYSTEMS, 'behind=!sse::above(side,', 'behind=sse::below(side,'),
    ('machine-open-ticks', SYSTEMS, 'memory.store32(open_ticks_open,at(m.object,offsetof(Device,open_ticks)),0xfffffffdu);',
     'memory.store32(open_ticks_open,at(m.object,offsetof(Device,open_ticks)),0xfffffffcu);'),
    ('machine-open-value', SYSTEMS, 'if(group!=none)frame.set_group(m,group,0x3f800000u);',
     'if(group!=none)frame.set_group(m,group,0x3f000000u);'),
    ('machine-occlusion-moving', SYSTEMS,
     'if(memory.load32(occlusion_flags,at(m.object,offsetof(Device,device_flags)))&device_flags::moving) {',
     'if(memory.load32(occlusion_flags,at(m.object,offsetof(Device,device_flags)))&device_flags::initializing) {'),
    ('machine-occlusion-open', SYSTEMS, 'if(!open)open=!sse::equal(memory.load32(occlusion_position,',
     'if(!open)open=sse::equal(memory.load32(occlusion_position,'),
    ('machine-portal-sar', SYSTEMS, 'const auto word=element(bits,std::uint32_t(signed_value(portal)>>5),4);',
     'const auto word=element(bits,portal>>5,4);'),
    ('machine-portal-bit', SYSTEMS, 'open?current|bit:current&~bit', 'open?current|bit:current'),
    ('machine-clamp-nan', SYSTEMS, 'if(sse::above(0u,value))value=0u;', 'if(sse::below(value,0u))value=0u;'),
    ('machine-clamp-high', SYSTEMS, 'else if(sse::above(value,one))value=one;', 'else if(sse::above(value,one))value=value;'),
    ('machine-cursor', SYSTEMS, 'frame.keep_portal_cursor(cursor+2u);', 'frame.keep_portal_cursor(cursor+4u);'),
    ('machine-portal-end', SYSTEMS,
     'end=widen16(memory.load16(portal_count_again,at(m.object,offsetof(Device,portal_count))))+first;',
     'end=widen16(memory.load16(portal_count_again,at(m.object,offsetof(Device,portal_count))))+index;'),
    ('machine-timer-equal', SYSTEMS, 'if(sse::equal(memory.load32(timer_position,at(m.object,offsetof(Device,position))),one) &&',
     'if(!sse::below(memory.load32(timer_position,at(m.object,offsetof(Device,position))),one) &&'),
    ('machine-timer-stays', SYSTEMS, '&machine_flags::door_stays_open)) {', '&machine_flags::one_sided)) {'),
    ('machine-ticks', SYSTEMS, 'const auto ticks=memory.load32(open_ticks,at(m.object,offsetof(Device,open_ticks)))+1u;',
     'const auto ticks=memory.load32(open_ticks,at(m.object,offsetof(Device,open_ticks)))+2u;'),
    ('machine-elapsed', SYSTEMS, 'if(sse::above(elapsed,memory.load32(door_open_time', 'if(sse::below(elapsed,memory.load32(door_open_time'),
    ('machine-close-value', SYSTEMS, 'if(close!=none)frame.set_group(m,close,0);', 'if(close!=none)frame.set_group(m,close,0x80000000u);'),
    ('machine-moved-bit', SYSTEMS, 'm.moved=(flags>>2)&1u;', 'm.moved=(flags>>3)&1u;'),
    ('machine-moved-clear', SYSTEMS, 'value&~device_flags::moved);', 'value&~device_flags::scripted_position);'),
    ('machine-parent-moved', SYSTEMS, 'if(header_flags&native_object_update::header_flags::moved)m.moved=1;',
     'if(header_flags&native_object_update::header_flags::late_update)m.moved=1;'),
    ('machine-parent-walk', SYSTEMS, 'm.parent=memory.load32(parent_parent,at(data,offsetof(ObjectData,parent)));',
     'm.parent=memory.load32(parent_parent,at(data,offsetof(ObjectData,first_child)));'),
    ('machine-result', SYSTEMS, 'return {Exit::moved,m.moved};', 'return {Exit::moved,1};'),
    # 10B840 logic
    ('device-moving-clear', SYSTEMS, 'const auto flags=memory.load32(flags_load,at(d.object,offsetof(Layout,device_flags)))&~device_flags::moving;',
     'const auto flags=memory.load32(flags_load,at(d.object,offsetof(Layout,device_flags)))&~device_flags::moved;'),
    ('device-script-done', SYSTEMS, 'if(!(flags&device_flags::position_script_done)) {', 'if(!(flags&device_flags::power_script_done)) {'),
    ('device-script-velocity', SYSTEMS, 'const auto velocity=sse::divide(change,', 'const auto velocity=sse::multiply(change,'),
    ('device-script-flag', SYSTEMS,
     'done?script_flags_value|device_flags::position_script_done:script_flags_value&~device_flags::position_script_done);',
     'done?script_flags_value|device_flags::position_script_done:script_flags_value);'),
    ('device-at-target', SYSTEMS, 'const bool at_target=sse::equal(target,memory.load32(position_compare,d.value));',
     'const bool at_target=!sse::below(target,memory.load32(position_compare,d.value));'),
    ('device-initializing', SYSTEMS, '&& !(flags&device_flags::initializing)) {', '&& !(flags&device_flags::moving)) {'),
    ('device-follow-group', SYSTEMS, '            d.group=frame.group();\n', ''),
    ('device-inverse', SYSTEMS, 'const auto inverse=sse::subtract(one_value,', 'const auto inverse=sse::add(one_value,'),
    ('device-acceleration-weight', SYSTEMS, 'acceleration=sse::add(acceleration,sse::multiply(unpowered,inverse));',
     'acceleration=sse::add(acceleration,sse::multiply(unpowered,one_value));'),
    ('device-speed-rate', SYSTEMS, 'auto speed=memory.load32(position_rate,at(device_definition,offsetof(DeviceDefinition,position_rate)));',
     'auto speed=memory.load32(position_rate,at(device_definition,offsetof(DeviceDefinition,power_rate)));'),
    ('device-direction', SYSTEMS, 'const bool up=sse::above(memory.load32(direction_velocity,d.velocity),0u);',
     'const bool up=!sse::below(memory.load32(direction_velocity,d.velocity),0u);'),
    ('device-moving', SYSTEMS, 'const bool moving=sse::above(acceleration,0u) && sse::above(speed,0u);',
     'const bool moving=sse::above(acceleration,0u);'),
    ('device-delay', SYSTEMS, 'if(sse::below(waited,memory.load32(delay_time,at(device_definition,offsetof(DeviceDefinition,delay))))) {',
     'if(sse::above(memory.load32(delay_time,at(device_definition,offsetof(DeviceDefinition,delay))),waited)) {'),
    ('device-closed', SYSTEMS, 'if(sse::equal(position,0u) && !sse::above(position,memory.load32(closed_target,',
     'if(sse::equal(position,0u) && sse::below(position,memory.load32(closed_target,'),
    ('device-delay-tick', SYSTEMS, 'const auto ticks=(delay_ticks+1u)&0xffffu;', 'const auto ticks=(delay_ticks+2u)&0xffffu;'),
    ('device-first-wait', SYSTEMS, 'if(ticks==1u) {', 'if(ticks==2u) {'),
    ('device-clamp', SYSTEMS, 'const auto clamped=frame.direction()?speed:sse::subtract(0u,speed);',
     'const auto clamped=frame.direction()?speed:sse::subtract(speed,0u);'),
    ('device-loops', SYSTEMS, 'const bool loops=memory.load8(loop_flags,at(device_definition,offsetof(DeviceDefinition,flags)))&1u;',
     'const bool loops=memory.load8(loop_flags,at(device_definition,offsetof(DeviceDefinition,flags)))&2u;'),
    ('device-arrival', SYSTEMS, 'effect=was_up?memory.load32(opened_effect', 'effect=!was_up?memory.load32(opened_effect'),
    ('device-turn', SYSTEMS, 'if(!sse::below(0u,sse::multiply(now,before))) {', 'if(sse::above(0u,sse::multiply(now,before))) {'),
    ('device-opening', SYSTEMS, 'const bool opening=sse::above(memory.load32(new_velocity_again,d.velocity),before);',
     'const bool opening=sse::below(memory.load32(new_velocity_again,d.velocity),before);'),
    ('device-moved', SYSTEMS, '&device_flags::initializing))\n        return Approach::updated;',
     '&device_flags::moving))\n        return Approach::updated;'),
    ('device-power-target', SYSTEMS, 'const auto target_value=memory.load32(power_target,at(d.power_group,offsetof(DeviceGroup,value)));',
     'const auto target_value=memory.load32(power_target,at(d.power_group,offsetof(DeviceGroup,initial)));'),
    ('device-power-rest', SYSTEMS,
     'if(sse::equal(velocity,memory.load32(power_velocity_zero,device_globals::zero)))return Approach::idle;',
     'if(!sse::above(velocity,memory.load32(power_velocity_zero,device_globals::zero)))return Approach::idle;'),
    ('device-power-scripted', SYSTEMS, '} else if(!(power_flags_value&device_flags::scripted_position)) {', '} else {'),
    ('device-power-idle-row', SYSTEMS, 'case Approach::idle:d.row=frame.row();break;', 'case Approach::idle:break;'),
    ('device-final-group', SYSTEMS, 'memory.load32(final_group,at(d.object,offsetof(Layout,position_group)))==none)',
     'memory.load32(final_group,at(d.object,offsetof(Layout,power_group)))==none)'),
    ('device-machine', SYSTEMS, 'if(is_machine(memory.load8(object_type', 'if(is_vehicle(memory.load8(object_type'),
    ('device-countdown', SYSTEMS, 'counting=memory.load8(countdown,at(data,offsetof(ObjectData,countdown)))!=0;',
     'counting=memory.load8(countdown,at(data,offsetof(ObjectData,type)))!=0;'),
    ('device-initializing-clear', SYSTEMS, 'value&~device_flags::initializing);', 'value&~device_flags::moving);'),
    ('device-row', SYSTEMS, 'd.row=handle_index(d.handle)*std::uint32_t(sizeof(ObjectHeader));', 'd.row=handle_index(d.handle)*8u;'),
    ('device-block-sign', SYSTEMS, 'frame.keep_block(widen16(memory.load16(animation_offset_again', 'frame.keep_block((memory.load16(animation_offset_again'),
    ('device-machine-mask', SYSTEMS, 'constexpr bool is_machine(std::uint32_t type){return (type&31u)==7u;}',
     'constexpr bool is_machine(std::uint32_t type){return (type&15u)==7u;}'),
    # 1179C0 adapter
    ('adapter-machine-frame', HEADER, 'gp[esp]=this->flags.minus(gp[esp],0xc8u,4);', 'gp[esp]=this->flags.minus(gp[esp],0xc4u,4);'),
    ('adapter-machine-exit-flags', HEADER, 'gp[esp]=this->flags.plus(gp[esp],0xc8u,4);', 'gp[esp]+=0xc8u;'),
    ('adapter-machine-ret', HEADER, 'gp[esp]=this->flags.plus(gp[esp],0xc8u,4);\n        this->ret(last,4);',
     'gp[esp]=this->flags.plus(gp[esp],0xc8u,4);\n        this->ret(last,0);'),
    ('adapter-handle-slot', HEADER, 'const auto value=this->read(0x1179c6u,gp[esp]+0xccu,4);', 'const auto value=this->read(0x1179c6u,gp[esp]+0xc8u,4);'),
    ('adapter-cluster-args', HEADER, 'gp[ecx]=record;this->push(0x117a8au,gp[eax]);this->push(0x117a8bu,gp[ecx]);',
     'gp[ecx]=record;this->push(0x117a8au,gp[ecx]);this->push(0x117a8bu,gp[eax]);'),
    ('adapter-floor-xorps', HEADER, 'else {x[0]=_mm_setzero_ps();this->mark(a_floor_zero);}', 'else {this->mark(a_floor_zero);}'),
    ('adapter-cluster-movaps', HEADER, 'if(this->above()){x[0]=x[1];this->mark(a_power_above);}', 'if(this->above()){this->mark(a_power_above);}'),
    ('adapter-or-eax', HEADER, 'case object_again:gp[eax]=f.logic(0xffffffffu,4);break;', 'case object_again:break;'),
    ('adapter-power-c1', HEADER, 'case power_epsilon:this->clear_c1();break;', 'case power_epsilon:break;'),
    ('adapter-fistp-c1', HEADER,
     'const auto rounded=physics_contact_runtime_detail::integer_spill(logic::x87::load(loaded),this->s.fp.status);',
     'std::uint16_t ignored{};const auto rounded=physics_contact_runtime_detail::integer_spill(logic::x87::load(loaded),ignored);'),
    ('adapter-wrap-subss', HEADER, 'case wrap_store:x[0]=subtract(x[0],x[2]);return 0x117b74u;', 'case wrap_store:return 0x117b74u;'),
    ('adapter-wrap-xorps', HEADER, 'case wrap_one:x[3]=_mm_setzero_ps();return 0x117ba6u;', 'case wrap_one:return 0x117ba6u;'),
    ('adapter-door-xorps', HEADER, 'case door_one:x[3]=_mm_setzero_ps();return 0x117c49u;', 'case door_one:return 0x117c49u;'),
    ('adapter-query-capacity', HEADER, 'this->push(0x117cbeu,0x10u);', 'this->push(0x117cbeu,0x20u);'),
    ('adapter-query-buffer', HEADER, 'gp[ecx]=gp[esp]+0x9cu;', 'gp[ecx]=gp[esp]+0x98u;'),
    ('adapter-query-order', HEADER, 'this->push(0x117cdbu,3);this->push(0x117cddu,1);', 'this->push(0x117cdbu,1);this->push(0x117cddu,3);'),
    ('adapter-units-count', HEADER, 'this->store(0x117cebu,gp[esp]+0x1cu,4,gp[eax]);', 'this->store(0x117cebu,gp[esp]+0x18u,4,gp[eax]);'),
    ('adapter-marker-ebx', HEADER, 'keep(m,scan);gp[ebx]=gp[esp]+0x34u;', 'keep(m,scan);gp[ebx]=gp[esp]+0x30u;'),
    ('adapter-forward-esi', HEADER, 'gp[edi]=this->flags.logic(0,4);gp[esi]=gp[esp]+0x64u;', 'gp[edi]=this->flags.logic(0,4);gp[esi]=gp[esp]+0x68u;'),
    ('adapter-forward-order', HEADER, 'this->store(0x117e03u,gp[esp]+0x88u,4,gp[ecx]);this->store(0x117e0au,gp[esp]+0x8cu,4,gp[edx]);',
     'this->store(0x117e03u,gp[esp]+0x88u,4,gp[edx]);this->store(0x117e0au,gp[esp]+0x8cu,4,gp[ecx]);'),
    ('adapter-side-last', HEADER, 'case side_zero:x[0]=multiply(x[0],x[1]);x[3]=add(x[3],x[0]);return 0x117e6eu;',
     'case side_zero:x[0]=multiply(x[0],x[1]);return 0x117e6eu;'),
    ('adapter-occlusion-bl', HEADER, '            if(f.pf)set_low_byte(gp[ebx],1);\n            break;', '            break;'),
    ('adapter-ticks-convert', HEADER, 'case timer_time:x[0]=convert(x[0],gp[eax]);return 0x118044u;', 'case timer_time:return 0x118044u;'),
    ('adapter-finish-shift', HEADER, 'gp[ebx]=this->flags.right(gp[ebx],2,4,false);', 'gp[ebx]=this->flags.right(gp[ebx],3,4,false);'),
    ('adapter-notify-slot', HEADER, 'this->store(0x118091u,gp[esp]+0x17u,1,1);', 'this->store(0x118091u,gp[esp]+0x13u,1,1);'),
    ('adapter-machine-take', HEADER, '        if(w.esi)m.*w.esi=gp[esi];\n', ''),
    ('adapter-machine-seats', HEADER, 'static constexpr Seats turned{nullptr,&Kept::object,&Kept::definition,&Kept::animation};',
     'static constexpr Seats turned{nullptr,&Kept::object,nullptr,&Kept::animation};'),
    ('adapter-lahf', HEADER, 'gp[eax]=(gp[eax]&0xffff00ffu)|(((this->flags.bits()|2u)&0xffu)<<8);',
     'gp[eax]=(gp[eax]&0xffff00ffu)|(((this->flags.bits())&0xffu)<<8);'),
    # 10B840 adapter
    ('adapter-device-frame', HEADER, 'gp[esp]=this->flags.minus(gp[esp],0x20u,4);', 'gp[esp]=this->flags.minus(gp[esp],0x1cu,4);'),
    ('adapter-device-handle', HEADER, 'gp[esi]=this->read(0x10b846u,gp[esp]+0x30u,4);', 'gp[esi]=this->read(0x10b846u,gp[esp]+0x2cu,4);'),
    ('adapter-device-row-shift', HEADER, 'case object:gp[ebx]=this->flags.shift(gp[ebx],2,4);return 0x10b864u;', 'case object:return 0x10b864u;'),
    ('adapter-device-edi-push', HEADER, 'this->push(0x10b8b3u,gp[edi]);', 'this->push(0x10b8b3u,gp[esi]);'),
    ('adapter-script-test', HEADER, 'case script_old_position:this->test8(gp[ecx]&0x20u);', 'case script_old_position:this->test8(gp[ecx]&0x8u);'),
    ('adapter-script-args', HEADER, 'gp[ecx]=gp[esp]+0x20u;this->push(0x10b8d9u,gp[ecx]);', 'gp[ecx]=gp[esp]+0x24u;this->push(0x10b8d9u,gp[ecx]);'),
    ('adapter-script-channel', HEADER, 'gp[eax]=gp[ebp]+0x18cu;gp[ecx]=gp[ebp]+0x14cu;keep(d,scripted);',
     'gp[eax]=gp[ebp]+0x1acu;gp[ecx]=gp[ebp]+0x14cu;keep(d,scripted);'),
    ('adapter-divide', HEADER, 'case tick_length:x[0]=divide(x[0],scalar(v));this->test8(gp[eax]);break;',
     'case tick_length:x[0]=multiply(x[0],scalar(v));this->test8(gp[eax]);break;'),
    ('adapter-script-done', HEADER, 'script_done=!this->flags.zf;', 'script_done=this->flags.zf;'),
    ('adapter-channel-eax', HEADER, 'case channel_graph:gp[ecx]=v;f.minus(v,logic::none,4);gp[eax]=gp[ebp]+0x18cu;break;',
     'case channel_graph:gp[ecx]=v;f.minus(v,logic::none,4);break;'),
    ('adapter-follow-esi', HEADER, 'gp[ecx]=position;this->push(0x10b9ddu,gp[ecx]);gp[esi]=gp[eax];', 'gp[ecx]=position;this->push(0x10b9ddu,gp[ecx]);'),
    ('adapter-unpowered-xorps', HEADER, 'x[2]=multiply(scalar(v),x[1]);x[1]=_mm_setzero_ps();x[5]=add(x[5],x[2]);',
     'x[2]=multiply(scalar(v),x[1]);x[5]=add(x[5],x[2]);'),
    ('adapter-direction-slot', HEADER, 'this->store(0x10ba5du,gp[esp]+0x13u,1,1);', 'this->store(0x10ba5du,gp[esp]+0x12u,1,1);'),
    ('adapter-delay-cmp', HEADER, 'case delay_store:gp[eax]=gp[ecx]+1u;this->flags.minus(gp[eax]&0xffffu,1,2);return 0x10bad1u;',
     'case delay_store:gp[eax]=gp[ecx]+1u;return 0x10bad1u;'),
    ('adapter-step-esi', HEADER, 'keep(d,stepping);gp[esi]=gp[ebx];', 'keep(d,stepping);gp[esi]=gp[edi];'),
    ('adapter-step-slot', HEADER, 'this->store(0x10bb53u,gp[esp]+0x38u,4,gp[ecx]);', 'this->store(0x10bb53u,gp[esp]+0x34u,4,gp[ecx]);'),
    ('adapter-loops-dl', HEADER, 'set_low_byte(gp[edx],this->flags.logic(loops?1u:0u,1));', 'this->flags.logic(loops?1u:0u,1);'),
    ('adapter-turn-product', HEADER, 'x[2]=multiply(x[2],x[0]);this->compare(x[1],x[2],false);', 'this->compare(x[1],x[2],false);'),
    ('adapter-effect-handle', HEADER, 'gp[edx]=this->read(0x10bbc1u,gp[esp]+0x34u,4);', 'gp[edx]=this->read(0x10bbc1u,gp[esp]+0x30u,4);'),
    ('adapter-report-ebx', HEADER, 'gp[ebx]=this->read(0x10bbe3u,gp[esp]+0x34u,4);this->push(0x10bbe7u,gp[ebx]);',
     'gp[edx]=this->read(0x10bbe3u,gp[esp]+0x34u,4);this->push(0x10bbe7u,gp[edx]);'),
    ('adapter-advance-order', HEADER, 'this->push(0x10bc0fu,gp[ebx]);this->push(0x10bc10u,0xc1630u);',
     'this->push(0x10bc0fu,0xc1630u);this->push(0x10bc10u,gp[ebx]);'),
    ('adapter-row-slot', HEADER, 'gp[ebx]=this->read(pc,gp[esp]+0x1cu,4);', 'gp[ebx]=this->read(pc,gp[esp]+0x18u,4);'),
    ('adapter-power-script-xorps', HEADER, 'this->x()[0]=_mm_setzero_ps();\n        this->push(0x10bc42u,0);', 'this->push(0x10bc42u,0);'),
    ('adapter-valid-esi', HEADER, 'keep(d,seats);gp[esi]=gp[ebp]+0x1acu;gp[eax]=gp[esi];', 'keep(d,seats);gp[esi]=gp[ebp]+0x18cu;gp[eax]=gp[esi];'),
    ('adapter-step-power-esi', HEADER, 'keep(d,powered);gp[esi]=gp[ebp]+0x138u;', 'keep(d,powered);gp[esi]=gp[ebp]+0x134u;'),
    ('adapter-old-power-slot', HEADER, 'this->store(0x10bd39u,gp[esp]+0x40u,4,before);', 'this->store(0x10bd39u,gp[esp]+0x3cu,4,before);'),
    ('adapter-final-pop', HEADER, 'case final_flags:gp[eax]=v;this->test8(v&8u);gp[edi]=this->pop(0x10bd9au);break;',
     'case final_flags:gp[eax]=v;this->test8(v&8u);break;'),
    ('adapter-notify-push', HEADER, 'gp[eax]=this->read(0x10bdccu,gp[esp]+0x30u,4);this->push(0x10bdd0u,1);',
     'gp[eax]=this->read(0x10bdccu,gp[esp]+0x30u,4);this->push(0x10bdd0u,0);'),
    ('adapter-device-take', HEADER, '        if(w.ebx)d.*w.ebx=gp[ebx];\n', ''),
    ('adapter-device-seats', HEADER, 'static constexpr Seats stepping{&Kept::velocity,&Kept::object,nullptr,&Kept::value};',
     'static constexpr Seats stepping{nullptr,&Kept::object,nullptr,&Kept::value};'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper():
    spec = importlib.util.spec_from_file_location('native_device_update_replacements', HELPER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def prelude(text):
    """The unit's operand resolver, sign extension and single spill, verbatim."""
    parts = [helper().RESOLVER.search(text)[0]]
    for pattern in (r'std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?\n\}',
                    r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}'):
        found = re.search(pattern, text, re.S)
        if not found:
            raise ValueError(f'Scene device unit lacks {pattern[:40]}')
        parts.append(found[0])
    return '\n'.join(parts) + '\n'


def compile_harness(scratch, name, includes, sanitizers=False):
    binary = scratch / name
    command = ['c++', '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
               *[f'-I{path}' for path in includes], f'-I{ROOT / "include"}', f'-I{ROOT / "src"}',
               f'-I{scratch}', str(TEST), '-o', str(binary)]
    if sanitizers:
        command[1:1] = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True, cwd=scratch)
    return binary


def run(binary, cases, mode=''):
    command = [str(binary), str(cases)] + ([mode] if mode else [])
    return json.loads(subprocess.run(command, check=True, capture_output=True, text=True).stdout)


def branch_names(text, group):
    block = re.sub(r'//[^\n]*', '', re.search(r'enum ' + group + r' : unsigned \{(.*?)\};', text, re.S)[1])
    return [name for name in re.sub(r'\s', '', block).split(',') if name and not name.endswith('_count')]


def check_binding(module, text):
    output, installed = module.install(text, text)
    if installed != list(OWNERS) or any(module.prefix(name) not in output for name in OWNERS):
        raise ValueError(f'Device update binding did not install: {installed}')
    if module.install(text, output)[1]:
        raise ValueError('Device update binding is not idempotent')
    if output.count(module.REFERENCES_BEGIN) != 1 or \
            output.index(module.REFERENCES_BEGIN) > output.index('static void run_1179c0(WorldState &s) {'):
        raise ValueError('Device update references are not in the unit prefix')
    original = module.body(text, '10b840')[0]
    changed = text.replace(original, original.replace('selected(0x148u,s.image)', 'selected(0x14au,s.image)', 1), 1)
    if changed == text or '10b840' in module.install(changed, changed)[1]:
        raise ValueError('Device update binding accepted a changed 10B840 body')
    child = module.body(text, 'bd080')[0]
    childless = text.replace(child, '', 1)
    if '1179c0' in module.install(childless, childless)[1]:
        raise ValueError('Device update binding accepted 1179C0 without its BD080 body')
    target, site, back = module.DISPATCHED['1179c0'][2]
    route = module.route(target, site, back)
    rerouted = text.replace(route, f's.push(0x{site:x}u,0x{back:x}u);run_{target}(s);', 1)
    if rerouted == text or '1179c0' in module.install(rerouted, rerouted)[1]:
        raise ValueError('Device update binding accepted 1179C0 with a changed BF050 route')
    bodied = text.replace(original, original + '\nstatic void run_b8760(WorldState &s) {\n}', 1)
    if '10b840' in module.install(bodied, bodied)[1]:
        raise ValueError('Device update binding accepted 10B840 with a B8760 body in the unit')
    resolver = module.RESOLVER.search(text)[0]
    remapped = text.replace(resolver, resolver.replace('case 0x1d0u:return 0x1d0u;', 'case 0x1d0u:return 0x1d4u;', 1), 1)
    if remapped == text or '1179c0' in module.install(remapped, remapped)[1]:
        raise ValueError('Device update binding accepted a non-identity operand')
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=12000)
    parser.add_argument('--mutant-cases', type=int, default=3000)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sources = [UNIT, HEADER, SYSTEMS, FRAMEWORK, FRAMEWORK_SYSTEMS, TEST, HELPER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = helper()
    if module.admitted(text) != list(OWNERS):
        raise ValueError(f'Canonical device update bodies drifted from their pins: {module.admitted(text)}')
    (scratch / 'device_prelude.inc').write_text(prelude(text))
    for name in OWNERS:
        found = module.body(text, name)
        (scratch / f'device_{name}.inc').write_text(found[0] + '\n')
        (scratch / f'device_{name}_reference.inc').write_text(module.reference(found, name))
    check_binding(module, text)
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins=module.PINS)
    release = compile_harness(scratch, 'device-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    header_text = HEADER.read_text()
    names, hit = [], set()
    for group, key in (('MachineBranch', 'machine_coverage'), ('DeviceBranch', 'device_coverage')):
        group_names = branch_names(header_text, group)
        words = [result[key][i] | verify[key][i] for i in range(2)]
        names += group_names
        hit |= {name for index, name in enumerate(group_names) if words[index // 64] >> (index % 64) & 1}
    missing = sorted(set(names) - hit - UNREACHABLE)
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit),
                  unreachable=sorted(UNREACHABLE), missing=missing)
    if missing:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
        raise ValueError(f'Uncovered native device update branches: {missing}')
    if not any(census['machine_coverage']) or not any(census['device_coverage']):
        raise ValueError('Census mode never admitted the native closures')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'device-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 5, 1000))
        report['sanitized_verify'] = run(sanitized, 500, 'verify')
        sanitized.unlink()
    release.unlink()

    texts = {path: path.read_text() for path in (HEADER, SYSTEMS)}

    def screen(mutant):
        name, target, old, new = mutant
        source = texts[target]
        if source.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / target.name).write_text(source.replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        # The default mode compares stores, registers and the bank; the verify
        # mode also orders every read against the stores.
        runs = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        if runs.returncode == 0:
            runs = subprocess.run([str(binary), str(max(args.mutant_cases // 2, 500)), 'verify'],
                                  capture_output=True, text=True)
        binary.unlink()
        return dict(name=name, rejected=runs.returncode != 0, diagnostic=runs.stderr.strip()[-200:])
    mutants = []
    if not args.skip_mutants:
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            mutants = list(pool.map(screen, MUTANTS))
    report['mutants'] = mutants
    after = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    report['source_sha256'] = after
    if after != before:
        raise ValueError('Sources changed during the mutation screen')
    survivors = [m['name'] for m in mutants if not m['rejected']]
    report['passed'] = not survivors
    report['survivors'] = survivors
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    if survivors:
        raise ValueError(f'Mutants survived: {survivors}')
    print(f"PASS {result['compared']} native device update comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['children']} genuine children, {result['faults']} faults), census {census['compared']}, "
          f"verify {verify['compared']}, {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
