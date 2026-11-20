#!/usr/bin/env python3
"""Prove a group F native TU1.5 closure (tools/native_frame_misc_replacements.py).

Run through tools/run_test.py: validate_native_frame_misc_tu15.py --owner NAME.
Extracts the unmodified translated body from its checked-in unit (pinned),
the extra translated bodies the harness needs, and the verification
reference the production binder generates. Compiles the owner's harness
against them and compares every seeded case (ordered stores with preimages,
child log, bank, registers, flags, XMM, x87, MXCSR, cursor; faults by
exception, cursor and store prefix) in Release, census and verify modes and
an ASan/UBSan build. Requires complete branch coverage (declared
unreachable branches excepted, each with its reason), checks the production
binding (install, idempotence, refusal of a changed body and of child-route
changes) and screens deliberate defects in out-of-tree header copies.
Source hashes are rechecked after the screen.
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
HELPER = ROOT / 'tools/native_frame_misc_replacements.py'
SHARED = ROOT / 'src/physics/runtime/native_tu15_object_tick.hpp'
HARNESS = ROOT / 'tests/native_tu15_frame_misc_harness.hpp'
# The shared data_array search and datum lookup (native_object_update).
OBJECT_UPDATE = 'src/physics/runtime/native_tu15_object_tick_systems.hpp'

# name -> owner proof: unit, boundary adapter (header, which holds the Branch
# enum), game logic (systems), harness, extra translated bodies (written to
# extra.inc), unreachable branches with reasons, mutants (file, name, anchor,
# replacement); the file is 'header', 'systems' or a shared source path.
OWNERS = {
    '1306b0': dict(
        unit='src/rendering/source_geometry_cache_frame_tu15_generated.cpp',
        header='src/physics/runtime/native_tu15_geometry_cache_count.hpp',
        systems='src/physics/runtime/native_tu15_geometry_cache_count_systems.hpp',
        test='tests/native_tu15_geometry_cache_count_tests.cpp',
        extra=(),
        unreachable={
            'scan_negative': 'EDX < [array+38] <= 0x7FFFFFFF before every INC, so it never becomes negative',
            'scan_invalid': 'the found index is the non-negative EDX that passed JS/JGE, never -1',
        },
        mutants=[
            ('systems', 'state-global', 'predicted_geometry=0x4f2d60u;', 'predicted_geometry=0x4f2d64u;'),
            ('systems', 'enable-byte', 'at(predicted,offsetof(DataArray,valid))', 'at(predicted,offsetof(DataArray,alignment_bits))'),
            ('header', 'enable-width', 'case predicted_valid:set_low_byte(gp[edx],v);', 'case predicted_valid:gp[edx]=v;'),
            ('header', 'disabled-flags', 'gp[eax]=0;this->test8(v);break;', 'gp[eax]=0;this->test8(1u);break;'),
            ('header', 'push-order', 'this->push(0x1306c6u,gp[ebp]);this->push(0x1306c7u,gp[esi]);',
             'this->push(0x1306c6u,gp[esi]);this->push(0x1306c7u,gp[ebp]);'),
            (OBJECT_UPDATE, 'count-signed', 'if(signed_value(index)>=signed_value(limit))return {SearchEnd::past_limit,index};',
             'if(index>=limit)return {SearchEnd::past_limit,index};'),
            ('header', 'bound-flags', 'past_limit:this->compare32(search.index,gp[edi]);', 'past_limit:this->compare32(gp[edi],search.index);'),
            (OBJECT_UPDATE, 'bit-shift', 'std::uint32_t(signed_value(index)>>5)', 'std::uint32_t(signed_value(index)>>4)'),
            (OBJECT_UPDATE, 'bit-mask', '&(1u<<(index&31u))))', '&(1u<<(index&15u))))'),
            (OBJECT_UPDATE, 'vector-stride', 'element(bits,std::uint32_t(signed_value(index)>>5),4)',
             'element(bits,std::uint32_t(signed_value(index)>>5),2)'),
            (OBJECT_UPDATE, 'scan-end', 'if(signed_value(index)>=signed_value(limit))return {SearchEnd::bits_exhausted,index};',
             'if(signed_value(index)>signed_value(limit))return {SearchEnd::bits_exhausted,index};'),
            ('header', 'end-flags', 'bits_exhausted:this->compare32(search.index,gp[edi]);', 'bits_exhausted:'),
            (OBJECT_UPDATE, 'stride-read', 'memory.load32(size,at(array,offsetof(DataArray,datum_size)));',
             'memory.load32(size,at(array,offsetof(DataArray,maximum_count)));'),
            (OBJECT_UPDATE, 'record-order',
             '    const auto datum_size=memory.load32(size,at(array,offsetof(DataArray,datum_size)));\n'
             '    return datum_size*index+memory.load32(data,at(array,offsetof(DataArray,data)));',
             '    const auto datum_data=memory.load32(data,at(array,offsetof(DataArray,data)));\n'
             '    return memory.load32(size,at(array,offsetof(DataArray,datum_size)))*index+datum_data;'),
            ('systems', 'null-record', '        if(!read)return {Exit::null_datum,pending,search};\n', ''),
            ('header', 'null-flags', 'gp[ecx]=this->flags.plus(gp[ecx]*gp[edx],v,4);', 'gp[ecx]=gp[ecx]*gp[edx]+v;this->flags.logic(0,4);'),
            ('systems', 'record-byte', 'offsetof(GeometryRead,ready))))continue;', 'offsetof(GeometryRead,priority))))continue;'),
            ('systems', 'record-width', 'if(memory.load8(ready,', 'if(memory.load16(ready,'),
            ('systems', 'count-skip', '        ++pending;\n', ''),
            ('header', 'pop-order', 'gp[edi]=this->pop(0x1306fbu);gp[esi]=this->pop(0x1306fcu);',
             'gp[esi]=this->pop(0x1306fbu);gp[edi]=this->pop(0x1306fcu);'),
            ('header', 'exit-ret', 'gp[ebx]=this->pop(0x1306feu);\n        this->ret(0x1306ffu,0);',
             'gp[ebx]=this->pop(0x1306feu);\n        this->ret(0x1306ffu,4);'),
            ('header', 'exit-cursor', 'this->ret(0x1306ffu,0);this->publish(0x1306ffu);\n    }\n};',
             'this->ret(0x1306ffu,0);this->publish(0x1306feu);\n    }\n};'),
        ]),
    'd7230': dict(
        unit='src/rendering/models/source_model_render_tu15_generated.cpp',
        header='src/physics/runtime/native_tu15_attachment_update.hpp',
        systems='src/physics/runtime/native_tu15_attachment_update_systems.hpp',
        test='tests/native_tu15_attachment_update_tests.cpp',
        extra=(), unreachable={},
        mutants=[
            (OBJECT_UPDATE, 'object-table', 'inline constexpr std::uint32_t object_headers=0x4ecbd0u;',
             'inline constexpr std::uint32_t object_headers=0x4ecbf0u;'),
            (OBJECT_UPDATE, 'handle-mask', 'constexpr std::uint32_t handle_index(std::uint32_t handle){return handle&0xffffu;}',
             'constexpr std::uint32_t handle_index(std::uint32_t handle){return handle&0x7fffu;}'),
            ('systems', 'first-link', 'at(walk.object,offsetof(WidgetOwner,first_widget))', 'at(walk.object,offsetof(WidgetOwner,first_widget)-4u)'),
            ('header', 'empty-flags', 'this->compare32(gp[eax],logic::none);', 'this->test32(gp[eax]);'),
            ('header', 'saved-order', 'this->push(0xd7255u,gp[esi]);this->push(0xd7256u,gp[edi]);',
             'this->push(0xd7255u,gp[edi]);this->push(0xd7256u,gp[esi]);'),
            ('systems', 'type-sign', 'widen16(memory.load16(widget_type,at(walk.widget,offsetof(Widget,type))))',
             'memory.load16(widget_type,at(walk.widget,offsetof(Widget,type)))'),
            ('systems', 'type-stride', 'offsetof(Widget,type)))),\n                          sizeof(WidgetType));',
             'offsetof(Widget,type)))),\n                          0x34u);'),
            ('systems', 'mode-width', 'if(render_mode==2u)', 'if(mode==2u)'),
            ('systems', 'mode-two-flag', '&request_flags::mode_two)', '&request_flags::mode_one)'),
            ('systems', 'mode-one-flag', '&request_flags::mode_one)', '&request_flags::mode_two)'),
            ('header', 'flag-slot', 'gp[eax]=this->read(0xd7288u,gp[esp]+0x10u,4);', 'gp[eax]=this->read(0xd7288u,gp[esp]+0x14u,4);'),
            ('header', 'argument-order', 'this->push(0xd72b1u,gp[eax]);this->push(0xd72b2u,gp[edx]);',
             'this->push(0xd72b1u,gp[edx]);this->push(0xd72b2u,gp[eax]);'),
            ('systems', 'argument-constant', 'limit=0x44200000u;', 'limit=0x44000000u;'),
            ('systems', 'owner-slot', 'constexpr std::uint32_t center_of(std::uint32_t object){return at(object,offsetof(WidgetOwner,center));}',
             'constexpr std::uint32_t center_of(std::uint32_t object){return at(object,offsetof(WidgetOwner,center)+4u);}'),
            ('header', 'update-flags', 'case render_callback:gp[ecx]=v;this->test32(v);break;', 'case render_callback:gp[ecx]=v;break;'),
            ('systems', 'update-target', 'memory.load32(shader_routine,at(walk.type,offsetof(WidgetType,shader)))',
             'memory.load32(shader_routine,at(walk.type,offsetof(WidgetType,render_callbacks[1])))'),
            ('header', 'target-order', 'child(0xd72c1u,0xd72c4u,shader);', 'child(0xd72c1u,0xd72c5u,shader);'),
            ('header', 'tail-argument', 'this->push(0xd72c4u,gp[ebx]);', 'this->push(0xd72c4u,gp[ebp]);'),
            ('header', 'tail-skip', '        child(0xd72c5u,0xd72cau,logic::submission::render_queue);\n', ''),
            ('systems', 'mode-slot', 'memory.load32(direct_routine,at(walk.type,offsetof(WidgetType,render)))',
             'memory.load32(direct_routine,at(walk.type,offsetof(WidgetType,shader)))'),
            ('header', 'mode-flags', 'case direct_routine:gp[esi]=v;this->test32(v);break;', 'case direct_routine:gp[esi]=v;break;'),
            ('systems', 'mode-argument', 'memory.load32(direct_data,at(walk.widget,offsetof(Widget,data)))',
             'memory.load32(direct_data,at(walk.widget,offsetof(Widget,next)))'),
            ('systems', 'next-link', 'memory.load32(next_widget,at(walk.widget,offsetof(Widget,next)))',
             'memory.load32(next_widget,at(walk.widget,offsetof(Widget,data)))'),
            ('header', 'pop-order', 'gp[edi]=this->pop(0xd72e0u);gp[esi]=this->pop(0xd72e1u);',
             'gp[esi]=this->pop(0xd72e0u);gp[edi]=this->pop(0xd72e1u);'),
            ('header', 'exit-ret', 'this->ret(0xd72e3u,4);', 'this->ret(0xd72e3u,0);'),
        ]),
    '1eee80': dict(
        unit='src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp',
        header='src/physics/runtime/native_tu15_engine_player_update.hpp',
        systems='src/physics/runtime/native_tu15_engine_player_update_systems.hpp',
        test='tests/native_tu15_engine_player_update_tests.cpp',
        extra=(),
        unreachable={
            'award_negative': 'EBX runs 0..0x26C in steps of 0x14, so TEST EBX,EBX never has SF set',
        },
        mutants=[
            ('header', 'frame', 'gp[esp]=this->flags.minus(gp[esp],std::uint32_t(sizeof(Locals)),4);',
             'gp[esp]=this->flags.minus(gp[esp],std::uint32_t(sizeof(Locals))+4u,4);'),
            ('systems', 'player-stride', 'at(rows,handle_index(player_handle)*player_size)', 'at(rows,(player_handle&0x7fffu)*player_size)'),
            ('systems', 'player-sign', 'auto record_index=widen16(memory.load16(engine_record,at(player,offsetof(Player,engine_record))));',
             'auto record_index=memory.load16(engine_record,at(player,offsetof(Player,engine_record)));'),
            ('systems', 'campaign-mode', 'inline constexpr std::int32_t campaign_mode=1;', 'inline constexpr std::int32_t campaign_mode=2;'),
            ('systems', 'campaign-word', 'suppressed=option==3u || option==2u;', 'suppressed=option==3u;'),
            ('header', 'partial-cl', 'if(!campaign){set_low_byte(gp[ecx],0);', 'if(!campaign){gp[ecx]=0u;'),
            ('systems', 'engine-enable', 'at(engine_players,offsetof(EnginePlayers,enabled))', 'at(engine_players,offsetof(EnginePlayers,enabled)-1u)'),
            ('systems', 'record-stride', 'const auto offset=record_index*std::uint32_t(sizeof(EnginePlayer));', 'const auto offset=record_index*0x1acu;'),
            ('systems', 'local-slot', 'memory.store32(record_offset_store,local(Local::record_offset),offset);',
             'memory.store32(record_offset_store,local(Local::slot_cursor),offset);'),
            ('systems', 'countdown-197',
             'if(tick(memory,countdown_1,countdown_1_store,countdown(1))==0)memory.store8(countdown_2_clear,countdown(2),0);',
             'tick(memory,countdown_1,countdown_1_store,countdown(1));'),
            ('systems', 'countdown-19b', '    tick(memory,countdown_6,countdown_6_store,countdown(6));\n', ''),
            ('systems', 'empty-greater', 'signed_value(scan.current)>signed_value(scan.slot)', 'signed_value(scan.current)>=signed_value(scan.slot)'),
            ('systems', 'empty-last', 'scan.slot==last_slot', 'scan.slot==last_slot-1u'),
            ('header', 'offer-flags', 'this->compare32(gp[ebp],logic::none);this->compare32(gp[ebp],gp[ebx]);',
             'this->compare32(gp[ebp],logic::none);this->compare32(gp[ebx],gp[ebp]);'),
            ('systems', 'offer-reload', 'record_index=memory.load32(scan_record_reload,local(Local::engine_record));',
             'record_index=memory.load32(scan_record_reload,local(Local::engine_record)-4u);'),
            ('systems', 'timer-sign', 'return std::int16_t(memory.load16(check,timer))>0;', 'return memory.load16(check,timer)>0;'),
            ('systems', 'timer-clear-order',
             '                memory.store16(cooling_timer_clear,scan.timer,0);\n'
             '                memory.store16(cooling_state_clear,at(slot_of_timer(scan.timer),offsetof(Slot,state)),slot_state::idle);\n',
             '                memory.store16(cooling_state_clear,at(slot_of_timer(scan.timer),offsetof(Slot,state)),slot_state::idle);\n'
             '                memory.store16(cooling_timer_clear,scan.timer,0);\n'),
            ('systems', 'roster-bound', 'signed_value(slot.slot)<signed_value(memory.load32(definition_count,',
             'signed_value(slot.slot)<=signed_value(memory.load32(definition_count,'),
            ('systems', 'roster-stride', 'slot.slot,\n                                    sizeof(SlotDefinition));',
             'slot.slot,\n                                    16u);'),
            ('systems', 'slot-reset', '                memory.store16(previous_timer_clear,at(old,offsetof(Slot,timer)),0);\n', ''),
            ('systems', 'state-bound', 'if(state<=slot_state::cooling)advance(', 'if(state<=slot_state::cooling+1u)advance('),
            ('header', 'state-table', 'this->read(0x1ef02fu,0x1ef260u+state*4u,4)', 'this->read(0x1ef02fu,0x1ef264u+state*4u,4)'),
            ('header', 'state0-flags', 'case start_state:this->compare32(gp[ebx],gp[edx]);return 0x1ef038u;', 'case start_state:return 0x1ef038u;'),
            ('systems', 'state0-result', 'memory.store16(start_timer,timer_of(),ticks);', 'memory.store32(start_timer,timer_of(),ticks);'),
            ('header', 'state1-flags', 'case current_state:this->compare32(v,logic::slot_state::cooling);break;', 'case current_state:break;'),
            ('header', 'state1-argument', 'this->push(site,gp[esi]);this->push(site+1u,gp[ebx]);', 'this->push(site,gp[ebx]);this->push(site+1u,gp[esi]);'),
            ('systems', 'state1-reset', '        memory.store32(starting_reset_clear,at(slot.record,offsetof(EnginePlayer,current)),none);\n', ''),
            ('header', 'count-carry', 'case active_ticks:case ending_ticks:this->flags.increment(v,4,false);break;',
             'case active_ticks:case ending_ticks:this->flags.cf=false;this->flags.increment(v,4,false);break;'),
            ('systems', 'state2-timer', 'at(time,offsetof(GameTime,ticks_per_second))', 'at(time,offsetof(GameTime,ticks_per_second)+2u)'),
            ('systems', 'state2-factor', 'memory.load16(hold_seconds,', 'memory.load8(hold_seconds,'),
            ('systems', 'state2-order', '        memory.store16(active_ending_state,state_of(),slot_state::ending);\n', ''),
            ('systems', 'award-reset', '        memory.store32(award_ticks_clear,at(slot.record,offsetof(EnginePlayer,current_ticks)),0);\n', ''),
            ('systems', 'state3-wait', 'return std::int16_t(memory.load16(check,timer))>0;', 'return std::int16_t(memory.load16(check,timer))>=0;'),
            ('systems', 'state4-compare', 'return std::int16_t(memory.load16(check,timer))>0;', 'return std::int16_t(memory.load16(check,timer))>1;'),
            ('systems', 'award-masks', 'at(masks,offsetof(EnginePlayer,award_mask_b))', 'at(masks,offsetof(EnginePlayer,award_mask_a))'),
            ('systems', 'award-sum', 'if(!(masked_a && masked_b) &&', 'if(!(masked_b && !masked_a) &&'),
            ('systems', 'award-range', 'pass.slot-first_awarded_slot<=awarded_slot_span',
             'signed_value(pass.slot-first_awarded_slot)<=signed_value(awarded_slot_span)'),
            ('header', 'award-bytes', 'gp[ecx]=this->read(0x1ef1f0u,0x1ef26du+slot,1);', 'gp[ecx]=this->read(0x1ef1f0u,0x1ef26cu+slot,1);'),
            ('header', 'award-default', '        if(target!=0x1ef1feu)throw std::runtime_error("Required original solver dispatch target unavailable");\n', ''),
            ('header', 'award-flags', 'gp[eax]=gp[esi]-logic::first_awarded_slot;this->compare32(gp[eax],logic::awarded_slot_span);',
             'gp[eax]=gp[esi]-logic::first_awarded_slot;this->compare32(gp[esi],logic::awarded_slot_span);'),
            ('systems', 'award-roster', 'signed_value(pass.slot)<signed_value(memory.load32(award_count,',
             'signed_value(pass.slot)<=signed_value(memory.load32(award_count,'),
            ('systems', 'award-index', '+pass.definition_offset;', '+pass.slot;'),
            ('systems', 'award-cursor', 'memory.store32(cursor_store,local(Local::slot_cursor),cursor+std::uint32_t(sizeof(Slot)));',
             'memory.store32(cursor_store,local(Local::record_offset),cursor+std::uint32_t(sizeof(Slot)));'),
            ('systems', 'award-bound', 'while(signed_value(pass.definition_offset)<signed_value(slot_count*std::uint32_t(sizeof(SlotDefinition))));',
             'while(signed_value(pass.definition_offset)<0x26c);'),
            ('header', 'exit-flags', 'gp[esp]=this->flags.plus(gp[esp],std::uint32_t(sizeof(Locals)),4);', 'gp[esp]+=std::uint32_t(sizeof(Locals));'),
            ('header', 'exit-pops', 'gp[edi]=this->pop(0x1ef255u);gp[esi]=this->pop(0x1ef256u);', 'gp[esi]=this->pop(0x1ef255u);gp[edi]=this->pop(0x1ef256u);'),
        ]),
    '41970': dict(
        unit='src/rendering/models/source_model_render_tu15_generated.cpp',
        header='src/physics/runtime/native_tu15_attachment_schedule.hpp',
        systems='src/physics/runtime/native_tu15_attachment_schedule_systems.hpp',
        test='tests/native_tu15_attachment_schedule_tests.cpp',
        extra=(), unreachable={},
        mutants=[
            ('header', 'frame-size', 'gp[eax]=std::uint32_t(sizeof(logic::Locals));', 'gp[eax]=std::uint32_t(sizeof(logic::Locals))-0x10u;'),
            ('systems', 'mode-width', 'if(memory.load16(skip,local(Local::skip)))', 'if(memory.load8(skip,local(Local::skip)))'),
            ('systems', 'object-slot', 'walk.object=memory.load32(object_argument,local(Local::object));',
             'walk.object=memory.load32(object_argument,local(Local::object)-4u);'),
            ('systems', 'object-stride', 'element(rows,handle_index(walk.object),sizeof(ObjectHeader))', 'element(rows,handle_index(walk.object),16u)'),
            ('systems', 'tag-shift', 'element(instances,tag_row(definition_index),sizeof(TagInstance))', 'element(instances,tag_row(definition_index),8u)'),
            ('systems', 'count-sign', 'if(signed_value(count)<=0)return Exit::no_attachments;', 'if(count==0)return Exit::no_attachments;'),
            ('systems', 'init-order',
             '    memory.store32(lens_flares_clear,local(Local::lens_flares),0);\n    memory.store32(index_clear,local(Local::attachment_index),0);\n',
             '    memory.store32(index_clear,local(Local::attachment_index),0);\n    memory.store32(lens_flares_clear,local(Local::lens_flares),0);\n'),
            ('systems', 'tag-mgs2', 'group==attachment_group::lens_flare || group==attachment_group::light_volume || group==attachment_group::tdtl;',
             'group==attachment_group::lens_flare || group==attachment_group::tdtl;'),
            ('systems', 'marker-name', 'memory.load32(attachment_tag,at(walk.attachment,offsetof(ObjectAttachment,tag)))!=none',
             'memory.load32(attachment_tag,at(walk.attachment,offsetof(ObjectAttachment,marker)))!=none'),
            ('header', 'resolve-limit', 'this->push(0x41a17u,logic::marker_limit);', 'this->push(0x41a17u,logic::marker_limit-1u);'),
            ('header', 'resolve-flags', 'set_low_byte(gp[ecx],0);this->flags.logic(0,1);\n        gp[ebx]=out;', 'set_low_byte(gp[ecx],0);\n        gp[ebx]=out;'),
            ('systems', 'resolve-output', 'frame.find_markers(walk,name,local(Local::markers))', 'frame.find_markers(walk,name,local(Local::markers)+4u)'),
            ('systems', 'count-sign-extend', 'walk.marker_count=widen16(frame.find_markers(walk,name,local(Local::markers)));',
             'walk.marker_count=frame.find_markers(walk,name,local(Local::markers))&0xffffu;'),
            ('systems', 'vector-order',
             '            memory.store32(red_store,local(Local::color),red);\n            const auto green=memory.load32(white_green,white_color+4u);\n',
             '            const auto green=memory.load32(white_green,white_color+4u);\n            memory.store32(red_store,local(Local::color),red);\n'),
            ('header', 'offset-partial', 'case change_color:gp[eax]=(gp[eax]&0xffff0000u)|v;', 'case change_color:gp[eax]=v;'),
            ('header', 'offset-flags', 'this->flags.logic(v,2);break;   // MOV AX; TEST AX,AX', 'this->flags.logic(gp[eax],4);break;'),
            ('systems', 'result-store', 'memory.store32(primary_scale_store,local(Local::primary_scale),primary_scale);',
             'memory.store32(primary_scale_store,local(Local::flare_scale),primary_scale);'),
            ('systems', 'offset-reload', '                walk.object=memory.load32(object_after_color,local(Local::object));\n', ''),
            ('systems', 'marker-stride', 'walk.marker+=std::uint32_t(sizeof(ObjectMarker));', 'walk.marker+=0x6cu;'),
            ('systems', 'marker-base', 'walk.marker=local(Local::markers);', 'walk.marker=local(Local::markers)-4u;'),
            ('systems', 'lens-limit', 'if(signed_value(memory.load32(lens_count,local(Local::lens_flares)))>=lens_flare_limit)',
             'if(signed_value(memory.load32(lens_count,local(Local::lens_flares)))>lens_flare_limit)'),
            ('header', 'lens-byte', 'case limit_seen:set_low_byte(gp[eax],v);', 'case limit_seen:gp[eax]=v;'),
            ('systems', 'lens-set',
             'if(!memory.load8(limit_seen,globals::lens_flare_limit_seen))memory.store8(limit_seen_set,globals::lens_flare_limit_seen,1);',
             'memory.load8(limit_seen,globals::lens_flare_limit_seen);'),
            ('header', 'lens-flag', 'case lens_flags:this->test8(v&low_byte(gp[eax]));', 'case lens_flags:this->test8(v);'),
            ('systems', 'lens-scale', 'scale=memory.load32(lens_marker_scale,at(walk.marker,offsetof(ObjectMarker,world.scale)));',
             'scale=memory.load32(lens_marker_scale,at(walk.marker,offsetof(ObjectMarker,world.forward)));'),
            ('header', 'lens-upper', 'this->s.xmm[0]=load_ss(scale);', 'this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],load_ss(scale));'),
            ('header', 'lens-arguments', 'case lens_index:this->push(0x41aebu,gp[edx]);this->push(0x41aecu,gp[eax]);',
             'case lens_index:this->push(0x41aebu,gp[eax]);this->push(0x41aecu,gp[edx]);'),
            ('header', 'lens-null', 'this->push(0x41b05u,0);', 'this->push(0x41b05u,1);'),
            ('systems', 'lens-count', 'memory.store32(lens_count_store,local(Local::lens_flares),submitted+1u);',
             'memory.store32(lens_count_store,local(Local::lens_flares),submitted);'),
            ('systems', 'mgs2-count',
             'if(signed_value(memory.load32(volume_entries_count,at(definition,offsetof(LightVolumeDefinition,entries.count))))>0) {',
             'if(memory.load32(volume_entries_count,at(definition,offsetof(LightVolumeDefinition,entries.count)))!=0) {'),
            ('systems', 'mgs2-flag', 'if(memory.load8(volume_flags,first)&light_volume_flags::marker_scale)', 'if(memory.load8(volume_flags,first)&0x10u)'),
            ('header', 'mgs2-flags', 'case volume_flags:this->test8(v&logic::light_volume_flags::marker_scale);break;', 'case volume_flags:break;'),
            ('header', 'mgs2-frame', 'gp[edx]=local(Local::color);gp[eax]=gp[ebx];', 'gp[edx]=local(Local::color)-4u;gp[eax]=gp[ebx];'),
            ('header', 'tdtl-partial', 'gp[edi]=gp[ebx];set_low_byte(gp[edx],0);', 'gp[edi]=gp[ebx];gp[edx]=0;'),
            ('header', 'tdtl-scale', 'this->push(0x41b8du,logic::unit_scale);', 'this->push(0x41b8du,0x3f000000u);'),
            ('systems', 'marker-bound', 'if(!(signed_value(next)<signed_value(markers)))break;', 'if(!(signed_value(next)<=signed_value(markers)))break;'),
            ('systems', 'attachment-stride', '+std::uint32_t(sizeof(ObjectAttachment));', '+0x14u;'),
            ('systems', 'attachment-reload', 'definition=memory.load32(definition_again,local(Local::definition));',
             'definition=memory.load32(definition_again,local(Local::object_index));'),
            ('header', 'exit-flags', 'gp[esp]=this->flags.plus(gp[esp],std::uint32_t(sizeof(logic::Locals)),4);',
             'gp[esp]+=std::uint32_t(sizeof(logic::Locals));'),
            ('header', 'exit-ret', 'this->ret(0x41c02u,8);', 'this->ret(0x41c02u,4);'),
            ('header', 'exit-pops', 'gp[ebp]=this->pop(0x41bf8u);gp[ebx]=this->pop(0x41bf9u);', 'gp[ebx]=this->pop(0x41bf8u);gp[ebp]=this->pop(0x41bf9u);'),
        ]),
    '133a70': dict(
        unit='src/campaign/source_campaign_environment_tu15_generated.cpp',
        header='src/physics/runtime/native_tu15_environment_palette.hpp',
        systems='src/physics/runtime/native_tu15_environment_palette_systems.hpp',
        test='tests/native_tu15_environment_palette_tests.cpp',
        extra=(), unreachable={},
        mutants=[
            ('systems', 'palette-global', 'scenario=0x4ecc20u;', 'scenario=0x4ecc24u;'),
            ('systems', 'output-clear', 'memory.store32(output_clear,output,none);', 'memory.store32(output_clear,output,0u);'),
            ('header', 'push-order', 'gp[ecx]=v;this->push(0x133a76u,gp[esi]);gp[esi]=gp[eax];',
             'gp[ecx]=v;gp[esi]=gp[eax];this->push(0x133a76u,gp[esi]);'),
            ('header', 'primary-partial', '            set_low_byte(gp[edx],v);gp[eax]=logic::none;', '            gp[edx]=v;gp[eax]=logic::none;'),
            ('systems', 'primary-sign', 'if(negative_byte(own) ||', 'if((own&0x100u) ||'),
            ('systems', 'primary-bound',
             'signed_value(widen8(own))>=signed_value(memory.load32(own_count,at(scenario,offsetof(Scenario,skies.count))))',
             'widen8(own)>=memory.load32(own_count,at(scenario,offsetof(Scenario,skies.count)))'),
            ('header', 'primary-flags', 'case own_count:gp[edi]=v;gp[edx]=sx8(gp[edx]);this->compare32(gp[edx],v);',
             'case own_count:gp[edi]=v;gp[edx]=sx8(gp[edx]);'),
            ('systems', 'primary-reread', 'const auto index=widen8(memory.load8(own_again,at(cluster,offsetof(Cluster,sky))));',
             'const auto index=widen8(own);'),
            ('systems', 'primary-stride', 'memory.load32(own_list,at(scenario,offsetof(Scenario,skies.address))),index,sizeof(SkyReference));',
             'memory.load32(own_list,at(scenario,offsetof(Scenario,skies.address))),index,4u);'),
            ('header', 'primary-order', '        case own_output:gp[edi]=this->pop(0x133aa3u);return 0x133aa4u;\n        case own_tag:return 0x133aa6u;',
             '        case own_output:return 0x133aa4u;\n        case own_tag:gp[edi]=this->pop(0x133aa3u);return 0x133aa6u;'),
            ('systems', 'secondary-byte', 'at(cluster,offsetof(Cluster,visible_sky))', 'at(cluster,offsetof(Cluster,unused_here_6d))'),
            ('systems', 'secondary-none', 'if(visible==no_sky ||', 'if(visible==0x7fu ||'),
            ('systems', 'secondary-name', 'if(tag==none)return structure_bsp_sky', 'if(tag==0)return structure_bsp_sky'),
            ('systems', 'secondary-tag', 'element(instances,tag_row(tag),sizeof(TagInstance))', 'element(instances,tag_row(tag),8u)'),
            ('systems', 'secondary-flag', '&sky_flags::not_visible_from_other_clusters)', '&0x20u)'),
            ('header', 'secondary-ebp', 'gp[ebp]=this->pop(0x133ae7u);', 'gp[ebp]=this->pop(0x133ae7u)^1u;'),
            ('systems', 'secondary-result', 'memory.load32(visible_tag_again,at(entry,offsetof(SkyReference,sky)))',
             'memory.load32(visible_tag_again,entry)'),
            ('systems', 'default-selector', 'const auto current=widen16(memory.load16(bsp_index,globals::structure_bsp_index));',
             'const auto current=memory.load16(bsp_index,globals::structure_bsp_index);'),
            ('systems', 'default-stride', 'current,\n                           sizeof(StructureBspReference));',
             'current,\n                           0x40u);'),
            ('systems', 'default-enable', '&structure_bsp_flags::default_sky))', '&2u))'),
            ('header', 'default-partial', 'gp[edx]=(gp[edx]&0xffff0000u)|v;', 'gp[edx]=v;'),
            ('header', 'default-none-flags', 'this->flags.minus(gp[edx],0xffffu,2);', 'this->flags.minus(gp[edx],0xffffu,4);'),
            ('systems', 'default-negative', '(index&0x8000u)', '(index&0x4000u)'),
            ('header', 'default-unnamed', 'gp[esi]=v;this->compare32(v,logic::none);gp[ecx]+=gp[edx]*8u;', 'gp[esi]=v;gp[ecx]+=gp[edx]*8u;'),
            ('systems', 'default-store', 'memory.store32(default_output,output,widen16(index));', 'memory.store32(default_output,output,tag);'),
            ('header', 'exit-pops', 'gp[edi]=this->pop(pops);gp[esi]=this->pop(pops+1u);', 'gp[esi]=this->pop(pops);gp[edi]=this->pop(pops+1u);'),
        ]),
    '153ba0': dict(
        unit='src/gameplay/units/source_combat_action_tu15_generated.cpp',
        header='src/physics/runtime/native_tu15_player_action.hpp',
        systems='src/physics/runtime/native_tu15_player_action_systems.hpp',
        test='tests/native_tu15_player_action_tests.cpp',
        extra=(), unreachable={},
        mutants=[
            ('systems', 'player-global', 'players=0x4f54f4u;', 'players=0x4f54f8u;'),
            ('systems', 'player-stride', 'player_size=0x21cu;', 'player_size=0x218u;'),
            ('systems', 'clear-order', '    memory.store32(context_clear_4,at(context,4),0);\n    memory.store32(context_clear_8,at(context,8),0);\n',
             '    memory.store32(context_clear_8,at(context,8),0);\n    memory.store32(context_clear_4,at(context,4),0);\n'),
            ('systems', 'melee-none', 'memory.store32(melee_none,at(context,offsetof(ActionContext,melee_target)),none);',
             'memory.store32(melee_none,at(context,offsetof(ActionContext,melee_target)),0);'),
            ('systems', 'riding', 'offsetof(ObjectData,parent)))!=none)return Exit::riding;', 'offsetof(ObjectData,parent)))==0)return Exit::riding;'),
            ('systems', 'pickup-margin', 'pickup_margin=0x46a240u;', 'pickup_margin=0x46a244u;'),
            ('systems', 'reach-store', 'memory.store32(pass_radius_store,local(Local::radii),pickup_reach);',
             'memory.store32(pass_radius_store,local(Local::radii),radius);'),
            ('systems', 'search-order', 'sse::add(pickup_reach,memory.load32(search_margin,globals::search_margin));',
             'sse::add(memory.load32(search_margin,globals::search_margin),pickup_reach);'),
            ('systems', 'weapon-mask', 'weapon_types=object_type::bit(object_type::weapon);', 'weapon_types=object_type::bit(object_type::equipment);'),
            ('systems', 'ignored-mask', 'object_type::bit(object_type::sound_scenery)|object_type::bit(object_type::creature);\nstatic_assert(ignored_types==0x1430u);',
             'object_type::bit(object_type::sound_scenery);'),
            ('systems', 'reach-types', 'if((bit&reach_types) && !within_reach', 'if((bit&weapon_types) && !within_reach'),
            ('systems', 'distance-axis', 'r.dz=sse::subtract(z,memory.load32(center_z,at(center,8)));', 'r.dz=sse::subtract(z,memory.load32(center_z,at(center,4)));'),
            ('systems', 'distance-sum', 'r.distance_squared=sse::add(xz,sse::multiply(r.dy,r.dy));', 'r.distance_squared=sse::add(xz,sse::multiply(r.dy,r.dz));'),
            ('systems', 'reach-compare', 'const bool out=sse::below(r.reach_squared,r.distance_squared);',
             'const bool out=sse::below(r.reach_squared,r.distance_squared) && !sse::below(r.distance_squared,r.reach_squared);'),
            ('systems', 'reach-margin', 'r.reach=sse::add(radius,memory.load32(reach_reload,frame.local(Local::pickup_reach)));',
             'r.reach=sse::add(radius,memory.load32(reach_reload,frame.local(Local::radii)+4u));'),
            ('systems', 'argument-order', '        const auto context=memory.load32(test_context,frame.argument(Argument::context));\n'
             '        const auto player=memory.load32(test_player,frame.argument(Argument::player));\n',
             '        const auto player=memory.load32(test_player,frame.argument(Argument::player));\n'
             '        const auto context=memory.load32(test_context,frame.argument(Argument::context));\n'),
            ('systems', 'attached-link', 'attached=memory.load32(attached_sibling,at(data,offsetof(ObjectData,next_sibling)));',
             'attached=memory.load32(attached_sibling,at(data,offsetof(ObjectData,first_child)));'),
            ('systems', 'attached-count', '        ++scan.count;\n    }', '        ++scan.count;++scan.count;\n    }'),
            ('systems', 'count-sign', 'Scan scan{widen16(frame.find_objects(', 'Scan scan{(frame.find_objects('),
            ('systems', 'scan-bound', 'while(signed_value(++scan.index)<signed_value(scan.count));', 'while(++scan.index<scan.count);'),
            ('systems', 'pass-reload', '            pass.offset=memory.load32(pass_reload,local(Local::pass));\n', ''),
            ('systems', 'pass-bound', 'while(pass.offset<pass_count_bytes);', 'while(signed_value(pass.offset)<signed_value(pass_count_bytes));'),
            ('systems', 'unit-slot', 'at(record,offsetof(Player,unit))', 'at(record,offsetof(Player,unit)+4u)'),
            ('systems', 'location-slot', 'at(object,offsetof(ObjectBounds,location)));', 'at(object,offsetof(ObjectBounds,location)+4u));'),
            ('systems', 'type-bit', 'const auto bit=object_type::bit(std::int32_t(type));', 'const auto bit=object_type::bit(std::int32_t(type)+1);'),
            ('systems', 'found-limit', 'while(attached!=none && scan.count<found_limit)', 'while(attached!=none && scan.count<found_limit-1u)'),
            ('header', 'test-flags', 'case Case::unit:\n            this->push', 'case Case::unit:this->compare32(1u,8u);\n            this->push'),
            ('header', 'type-register', 'gp[ecx]=sx8(v);gp[edx]=this->flags.shift(1u,low_byte(gp[ecx]),4);', 'gp[ecx]=v;gp[edx]=this->flags.shift(1u,low_byte(gp[ecx]),4);'),
            ('header', 'frame-size', 'gp[esp]=this->flags.minus(gp[esp],std::uint32_t(sizeof(logic::Locals)),4);',
             'gp[esp]=this->flags.minus(gp[esp],std::uint32_t(sizeof(logic::Locals))+4u,4);'),
            ('header', 'saved-order', 'gp[edi]=this->pop(0x153dd2u);gp[ebp]=this->pop(0x153dd3u);',
             'gp[ebp]=this->pop(0x153dd2u);gp[edi]=this->pop(0x153dd3u);'),
            ('header', 'edi-once', 'if(!saved_edi){saved_edi=true;this->push(0x153c55u,gp[edi]);}', 'this->push(0x153c55u,gp[edi]);'),
            ('header', 'list-argument', 'gp[eax]=gp[esp]+0x34u;', 'gp[eax]=gp[esp]+0x30u;'),
            ('header', 'mask-flags', 'gp[ecx]=this->flags.logic(types,4);', 'gp[ecx]=types;'),
            ('header', 'pass-kept', '        pass.offset=gp[esi];\n', ''),
            ('header', 'xmm-parts', 'x[2]=load_ss(r.dy);x[3]=load_ss(r.dz);', 'x[2]=load_ss(r.dz);x[3]=load_ss(r.dy);'),
            ('header', 'case-flags', 'this->compare32(type,8u);\n', ''),
            ('header', 'case-table', 'this->read(0x153d2cu,0x153de0u+type*4u,4)', 'this->read(0x153d2cu,0x153de4u+type*4u,4)'),
            ('header', 'case-default', '        default:throw std::runtime_error("Required original solver dispatch target unavailable");\n        }\n        return current_case;',
             '        default:current_case=Case::attached;break;\n        }\n        return current_case;'),
            ('header', 'test-arguments', 'this->push(0x153d86u,gp[edx]);this->push(0x153d87u,gp[ecx]);',
             'this->push(0x153d86u,gp[ecx]);this->push(0x153d87u,gp[edx]);'),
            ('header', 'test-target', 'this->call(0x153d71u,0x153d76u,0x153f20u,vehicle);', 'this->call(0x153d71u,0x153d76u,0x153f20u,unit);'),
            ('header', 'scan-kept', '        scan.count=gp[ebx];scan.index=gp[ebp];\n', ''),
            ('header', 'xmm0-store', 'case pickup_reach_store:case search_radius_store:this->s.xmm[0]=load_ss(v);break;',
             'case pickup_reach_store:this->s.xmm[0]=load_ss(v);break;'),
            ('header', 'exit-flags', 'gp[esp]=this->flags.plus(gp[esp],std::uint32_t(sizeof(logic::Locals)),4);         // ADD ESP,120',
             'gp[esp]+=std::uint32_t(sizeof(logic::Locals));'),
            ('header', 'exit-ret', 'this->ret(0x153ddcu,8);', 'this->ret(0x153ddcu,4);'),
        ]),
    '131610': dict(
        unit='src/campaign/source_campaign_environment_tu15_generated.cpp',
        header='src/physics/runtime/native_tu15_fog_mixers.hpp',
        systems='src/physics/runtime/native_tu15_fog_mixers_systems.hpp',
        test='tests/native_tu15_fog_mixers_tests.cpp',
        extra=(), unreachable={},
        mutants=[
            ('systems', 'scenario-global', 'scenario=0x4ecc20u;', 'scenario=0x4ecc24u;'),
            ('systems', 'bsp-global', 'structure_bsp=0x4ecc18u;', 'structure_bsp=0x4ecc1cu;'),
            ('systems', 'interpolators-global', 'interpolators=0x4f3010u;', 'interpolators=0x4f3014u;'),
            ('systems', 'early-table-order',
             '    memory.store32(field_offset_early,field(2,false),mixed_fields[2].offset);\n'
             '    memory.store32(field_size_early,field(13,true),mixed_fields[13].size);',
             '    memory.store32(field_size_early,field(13,true),mixed_fields[13].size);\n'
             '    memory.store32(field_offset_early,field(2,false),mixed_fields[2].offset);'),
            ('systems', 'cluster-stride', 'cluster_index*std::uint32_t(sizeof(Cluster))', 'cluster_index*0xacu'),
            ('systems', 'fog-field', 'at(cluster,offsetof(Cluster,atmospheric_fog))', 'at(cluster,offsetof(Cluster,atmospheric_fog)-1u)'),
            ('systems', 'fog-sign-extend', 'const auto index=widen8(fog);', 'const auto index=fog&0xffu;'),
            ('systems', 'fog-count-bound', 'if(signed_value(index)>=signed_value(memory.load32(fog_count,', 'if(signed_value(index)>signed_value(memory.load32(fog_count,'),
            ('systems', 'palette-stride', 'sizeof(FogEntry));\n    memory.store32(fog_entry_store', '0xf0u);\n    memory.store32(fog_entry_store'),
            ('systems', 'mixer-count-sign', 'if(signed_value(count)<=0)return;', 'if(count==0)return;'),
            ('systems', 'source-sign-check', 'if(!(source&0x8000u)) {', 'if(!(source&0x4000u)) {'),
            ('systems', 'interpolator-stride', 'sizeof(InterpolatorState))', '0x1cu)'),
            ('systems', 'clamp-zero', 'if(sse::above(sse::zero,weight))clamp=Clamp::zero,weight=sse::zero;',
             'if(sse::above(weight,sse::zero))clamp=Clamp::zero,weight=sse::zero;'),
            ('systems', 'clamp-one', 'else if(sse::above(weight,one))clamp=Clamp::one,weight=one;',
             'else if(sse::above(one,weight))clamp=Clamp::one,weight=one;'),
            ('systems', 'remaining-mul', 'remaining=sse::multiply(sse::subtract(one,weight),remaining);',
             'remaining=sse::multiply(remaining,sse::subtract(one,weight));'),
            ('systems', 'weight-total-test', 'if(!sse::above(total,sse::zero))return;', 'if(sse::above(sse::zero,total))return;'),
            ('systems', 'blend-scale-div', 'const auto scale=sse::divide(one,total);', 'const auto scale=sse::divide(total,one);'),
            ('systems', 'blend-source-sign', 'if(signed_value(index)>=0) {', 'if(signed_value(index)>0) {'),
            ('systems', 'blend-scale-op', 'value=sse::multiply(value,scale);', 'value=sse::add(value,scale);'),
            ('systems', 'mix-complement', 'MixStep step_values{sse::subtract(one,remaining),0,0};',
             'MixStep step_values{sse::subtract(remaining,one),0,0};'),
            ('systems', 'mix-add-order', 'step_values.result=sse::add(mixed_value,step_values.own_share);',
             'step_values.result=sse::add(step_values.own_share,mixed_value);'),
            ('systems', 'link-store-field', 'at(entry,offsetof(ClusterDistance,link)),slot_index);',
             'at(entry,offsetof(ClusterDistance,link)-4u),slot_index);'),
            ('systems', 'output-slot-step', 'local(Local::output_slot))+std::uint32_t(sizeof(FogEntry));',
             'local(Local::output_slot))+0xf0u;'),
            ('header', 'frame-align', 'gp[esp]=this->flags.logic(gp[esp]&0xfffffff8u,4);', 'gp[esp]=this->flags.logic(gp[esp]&0xfffffffcu,4);'),
            ('header', 'saved-order', 'this->push(0x131621u,gp[ebx]);this->push(0x131622u,gp[esi]);',
             'this->push(0x131621u,gp[esi]);this->push(0x131622u,gp[ebx]);'),
            ('header', 'string-direction', 'return (this->s.other_flags&0x400u)?0xfffffffcu:4u;', 'return 4u;'),
            ('header', 'child-args-order', 'this->push(0x131881u,gp[edx]);this->push(0x131882u,gp[eax]);',
             'this->push(0x131881u,gp[eax]);this->push(0x131882u,gp[edx]);'),
            ('header', 'child-walk-kept', 'walk.scenario=gp[edi];walk.entry=gp[ebx];', 'walk.scenario=gp[edi];'),
            ('header', 'exit-ret', 'this->ret(0x131aaau,8);this->publish(0x131aaau);', 'this->ret(0x131aaau,4);this->publish(0x131aaau);'),
            ('header', 'early-exit-ret', 'this->ret(0x131ab5u,8);this->publish(0x131ab5u);', 'this->ret(0x131ab5u,4);this->publish(0x131ab5u);'),
        ]),
}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper():
    spec = importlib.util.spec_from_file_location('native_frame_misc_replacements', HELPER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compile_harness(scratch, test, name, includes, sanitizers=False):
    binary = scratch / name
    command = ['c++', '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
               *[f'-I{path}' for path in includes], f'-I{ROOT / "include"}', f'-I{ROOT / "src"}',
               f'-I{scratch}', str(test), '-o', str(binary)]
    if sanitizers:
        command[1:1] = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True, cwd=scratch)
    return binary


def run(binary, cases, mode=''):
    command = [str(binary), str(cases)] + ([mode] if mode else [])
    return json.loads(subprocess.run(command, check=True, capture_output=True, text=True).stdout)


def check_binding(module, name, text):
    found = module.body(text, name)
    output, installed = module.install(text, text, [name])
    if name not in installed or module.prefix(name) not in output:
        raise ValueError(f'{name} binding did not install')
    if name in module.install(text, output, [name])[1]:
        raise ValueError(f'{name} binding is not idempotent')
    changed = text.replace(found[0], found[0].replace('{\n', '{\n// changed\n', 1), 1)
    if changed == text or name in module.install(changed, changed, [name])[1]:
        raise ValueError(f'{name} binding accepted a changed body')
    owner = module.OWNERS[name]
    for child in owner['direct']:
        child_body = module.body(text, child)
        missing = text.replace(child_body[0], '', 1)
        if name in module.install(missing, missing, [name])[1]:
            raise ValueError(f'{name} binding accepted a missing {child} body')
    for child in owner['dispatched']:
        direct = text.replace(found[0], found[0] + f'\nstatic void run_{child}(WorldState &s) {{\n}}', 1)
        if name in module.install(direct, direct, [name])[1]:
            raise ValueError(f'{name} binding accepted a direct {child} route')
    other = text.replace(owner['unit'], '', 1)
    if name in module.install(other, other, [name])[1]:
        raise ValueError(f'{name} binding accepted another unit')
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--owner', required=True, choices=sorted(OWNERS))
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=8000)
    parser.add_argument('--mutant-cases', type=int, default=2000)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    proof = OWNERS[args.owner]
    unit, header, test = ROOT / proof['unit'], ROOT / proof['header'], ROOT / proof['test']
    mutant_files = sorted({ROOT / proof.get(where, where) for where, *_ in proof['mutants']})
    sources = [unit, header, SHARED, HARNESS, test, HELPER, Path(__file__).resolve()]
    sources += [path for path in mutant_files if path not in sources]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = unit.read_text()
    module = helper()
    if not module.admitted(text, args.owner):
        raise ValueError(f'Canonical {args.owner} body drifted from its pin')
    found = module.body(text, args.owner)
    (scratch / 'body.inc').write_text(found[0] + '\n')
    (scratch / 'reference.inc').write_text(module.reference(args.owner, found))
    extra = ''.join(module.body(text, name)[0] + '\n' for name in proof['extra'])
    (scratch / 'extra.inc').write_text(extra)
    check_binding(module, args.owner, text)
    report = dict(passed=False, owner=args.owner, unit=proof['unit'], pin=module.OWNERS[args.owner]['pin'])
    release = compile_harness(scratch, test, 'release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    block = re.search(r'enum Branch : unsigned \{(.*?)\};', header.read_text(), re.S)[1]
    names = [name for name in re.sub(r'\s', '', block).split(',') if name and name != 'branch_count']
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(names) if coverage >> index & 1}
    unreachable = proof['unreachable']
    missing = sorted(set(names) - hit - set(unreachable))
    reached_unreachable = sorted(hit & set(unreachable))
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit),
                  unreachable=unreachable, missing=missing)
    if missing:
        raise ValueError(f'Uncovered native {args.owner} branches: {missing}')
    if reached_unreachable:
        raise ValueError(f'Branches declared unreachable were reached: {reached_unreachable}')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, test, 'sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 5, 1000))
        report['sanitized_verify'] = run(sanitized, 400, 'verify')
        sanitized.unlink()
    texts = {path: path.read_text() for path in mutant_files}

    def screen(mutant):
        where, name, old, new = mutant
        path = ROOT / proof.get(where, where)
        if texts[path].count(old) != 1:
            return dict(name=name, file=where, rejected=False, error='mutation anchor missing or ambiguous')
        directory = scratch / 'mutants' / name / path.parent.relative_to(ROOT / 'src')
        directory.mkdir(parents=True)
        (directory / path.name).write_text(texts[path].replace(old, new, 1))
        try:
            binary = compile_harness(scratch, test, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        # The default mode compares stores and results; verify mode also
        # checks the order, address and width of every read.
        runs = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        if not runs.returncode:
            runs = subprocess.run([str(binary), str(max(args.mutant_cases // 2, 500)), 'verify'],
                                  capture_output=True, text=True)
        binary.unlink()
        return dict(name=name, file=where, rejected=runs.returncode != 0, diagnostic=runs.stderr.strip()[-200:])
    mutants = []
    if not args.skip_mutants:
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            mutants = list(pool.map(screen, proof['mutants']))
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
    print(f"PASS {args.owner}: {result['compared']} comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['children']} genuine children, {result['faults']} faults), census {census['compared']}, "
          f"verify {verify['compared']}, {len(hit)}/{len(names)} branches ({len(unreachable)} unreachable), "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
