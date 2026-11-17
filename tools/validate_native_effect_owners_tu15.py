#!/usr/bin/env python3
"""Prove the native TU1.5 effect owners (20680 lens query reset; C35D0 /
C3300 / C2A80 per-view light fades; 176A40 / 252100 particle inputs).

Run through tools/run_test.py. Extracts the unmodified translated bodies from
the checked-in lens flare and effect light view units (pinned) and the
verification references the production binder generates, compiles
tests/native_tu15_effect_owners_tests.cpp against them and compares every
seeded case (ordered stores with preimages, 16EA60 child log, bank,
registers, flags, XMM, x87, MXCSR, cursor; faults by exception, cursor and
store prefix) in Release, census, verify and reference-switch modes and an
ASan/UBSan build. Requires complete reachable branch coverage, checks the
production binding (install, idempotence, refusal of a changed body, a
missing child body or a non-identity operand) and screens deliberate
defects: each mutant is an out-of-tree copy of the logic (SYSTEMS) or the
adapter (HEADER) placed first on the include path. Source hashes are
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
LENS_UNIT = ROOT / 'src/rendering/effects/source_lens_flare_tu15_generated.cpp'
LIGHT_UNIT = ROOT / 'src/rendering/effects/source_effect_light_view_tu15_generated.cpp'
STATE_UNIT = ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
COMBAT_UNIT = ROOT / 'src/gameplay/units/source_combat_action_tu15_generated.cpp'
DEATH_UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_effect_owners.hpp'
SYSTEMS = ROOT / 'src/rendering/effects/native_tu15_effect_owners_systems.hpp'
SHARED = ROOT / 'src/physics/runtime/native_tu15_object_tick.hpp'
SHARED_SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_object_tick_systems.hpp'
TEST = ROOT / 'tests/native_tu15_effect_owners_tests.cpp'
HELPER = ROOT / 'tools/native_effect_owner_replacements.py'
# Branches no input can reach (see the owner doc): C3300 restores EDI from
# its own push, so the loop handle is never -1 at C3631, and the scan start
# (index + 1, or 0) is never negative.
# 176A40's BSF reads the mask slot it rewrote with the remaining mask just
# before the call, so its zero-source path needs a 4EFA0 that returns a
# nonzero EBX after the last input; the seeded cases have none.
UNREACHABLE = {'lv_null_handle', 'lv_scan_negative', 'sa_zero_scan'}
# (name, file, anchor, replacement[, harness mode]). The logic lives in
# SYSTEMS and the ABI in the HEADER adapter; each mutant is an out-of-tree
# copy of its file.
MUTANTS = [
    # 20680 logic
    ('lens-negative', SYSTEMS, 'if(result.stamp>=0) {', 'if(result.stamp>0) {'),
    ('lens-mode-low', SYSTEMS, 'return mode>=4 &&', 'return mode>=3 &&'),
    ('lens-mode-high', SYSTEMS, 'mode<=7;}', 'mode<=8;}'),
    ('lens-newer-signed', SYSTEMS, 'result.stamp>*result.last ||', 'std::uint32_t(result.stamp)>std::uint32_t(*result.last) ||'),
    ('lens-unset', SYSTEMS, '*result.last==0;', '*result.last==1;'),
    ('lens-same-mode', SYSTEMS, 'queries_off(*result.mode) && result.stamp==*result.last',
     'queries_off(*result.mode) && result.stamp!=*result.last'),
    ('lens-slot-mask', SYSTEMS, 'inline constexpr std::uint32_t frame=0x2u;', 'inline constexpr std::uint32_t frame=0x1u;'),
    ('lens-slot-stride', SYSTEMS, 'element(globals::records,index,sizeof(LensQueryRecord))', 'element(globals::records,index,16)'),
    ('lens-slot-end', SYSTEMS, 'record_count=511;', 'record_count=510;'),
    ('lens-slot-field', SYSTEMS, 'sizeof(LensQueryRecord)),offsetof(LensQueryRecord,flags));',
     'sizeof(LensQueryRecord)),offsetof(LensQueryRecord,area));'),
    ('lens-store-order', SYSTEMS,
     '    memory.store32(last_stamp_store,globals::reset_stamp,std::uint32_t(result.stamp));\n'
     '    memory.store8(reset_set,globals::frame_reset,1u);',
     '    memory.store8(reset_set,globals::frame_reset,1u);\n'
     '    memory.store32(last_stamp_store,globals::reset_stamp,std::uint32_t(result.stamp));'),
    ('lens-flag-value', SYSTEMS, 'queries_off(*result.mode)?0u:1u);', 'queries_off(*result.mode)?1u:0u);'),
    ('lens-off-width', SYSTEMS, 'memory.store8(reset_clear,globals::frame_reset,0u);', 'memory.store32(reset_clear,globals::frame_reset,0u);'),
    ('lens-read-order', SYSTEMS,
     '        result.mode=signed_value(memory.load32(mode,globals::view_mode));\n'
     '        result.last=signed_value(memory.load32(last_stamp,globals::reset_stamp));',
     '        result.last=signed_value(memory.load32(last_stamp,globals::reset_stamp));\n'
     '        result.mode=signed_value(memory.load32(mode,globals::view_mode));', 'verify'),
    # 20680 adapter
    ('lens-ret', HEADER, 'this->ret(0x206edu,0);', 'this->ret(0x206edu,4);'),
    ('lens-exit-flags', HEADER, ':std::uint32_t(*result.last));', ':std::uint32_t(result.stamp));'),
    ('lens-flag-test', HEADER, 'this->test8(off?1u:0u);', 'this->test8(off?0u:1u);'),
    ('lens-result-register', HEADER, 'this->gp[eax]=off?0u:1u;', 'this->gp[eax]=off?1u:0u;'),
    ('lens-mode-register', HEADER, 'else if(a==mode)gp[ecx]=v;', 'else if(a==mode)gp[ecx]=v+1u;'),
    ('lens-stamp-register', HEADER, 'if(a==stamp)gp[edx]=v;', 'if(a==stamp)gp[edx]=v^1u;'),
    ('lens-store-pc', HEADER, 'case reset_set:return 0x206cbu;', 'case reset_set:return 0x206ceu;'),
    # C35D0 logic
    ('light-first-width', SYSTEMS, 'walk.handle=make_handle(memory.load16(first_identifier,datum),first);',
     'walk.handle=make_handle(memory.load8(first_identifier,datum),first);'),
    ('light-flag-bit', SYSTEMS, 'inline constexpr std::uint32_t active=0x2u;', 'inline constexpr std::uint32_t active=0x4u;'),
    ('light-scan-start', SYSTEMS, 'walk.handle==none?none:walk.index,scan_limit', 'walk.handle==none?none:walk.index+1u,scan_limit'),
    ('light-datum-stride', SYSTEMS, 'const auto light=element(data,walk.index,sizeof(LightDatum));',
     'const auto light=element(data,walk.index,0x100u);'),
    ('light-next-index', SYSTEMS, 'datum_of(memory,walk.lights,search.index,next_size,next_data)',
     'datum_of(memory,walk.lights,search.index+1u,next_size,next_data)'),
    ('light-next-handle', SYSTEMS, 'walk.handle=make_handle(memory.load16(next_identifier,datum),search.index);',
     'walk.handle=make_handle(memory.load16(next_identifier,datum),search.index+1u);'),
    # C3300 logic
    ('light-tag-stride', SYSTEMS, 'element(tags,handle_index(tag),sizeof(TagInstance))', 'element(tags,handle_index(tag),8u)'),
    ('light-class-sign', SYSTEMS, 'return signed_value(widen16(memory.load16(access,at(light_definition,field))));',
     'return signed_value(memory.load16(access,at(light_definition,field)));'),
    ('light-specular-class', SYSTEMS, 'distance_class(specular_class,offsetof(LightDefinition,diffuse_class));',
     'distance_class(specular_class,offsetof(LightDefinition,secondary_class));'),
    ('light-table', SYSTEMS, 'volume_distances=0x44a728u;', 'volume_distances=0x44a72cu;'),
    ('light-overall-table', SYSTEMS, 'radius(overall_radius),globals::overall_distances};',
     'radius(overall_radius),globals::diffuse_distances};'),
    ('light-specular-range', SYSTEMS, 'call.range=radius(specular_range);', 'call.range=memory.load_float(specular_range,globals::one);'),
    ('light-volume-range', SYSTEMS,
     'call.range=sse::multiply(volume_radius_value,memory.load_float(volume_range_scale,globals::volume_range_scale));',
     'call.range=memory.load_float(volume_range_scale,globals::volume_range_scale);'),
    ('light-overall-field', SYSTEMS, 'memory.store_float(overall_store,fade_field(offsetof(LightDatum,overall_fade)),overall);',
     'memory.store_float(overall_store,fade_field(offsetof(LightDatum,overall_fade)+4u),overall);'),
    ('light-store-order', SYSTEMS,
     '    frame.keep_secondary(secondary);\n'
     '    memory.store_float(secondary_store,fade_field(offsetof(LightDatum,secondary_fade)),secondary);',
     '    memory.store_float(secondary_store,fade_field(offsetof(LightDatum,secondary_fade)),secondary);\n'
     '    frame.keep_secondary(secondary);'),
    ('light-arguments-order', SYSTEMS, '    frame.pass_fade_arguments(call);\n    frame.keep_secondary(secondary);',
     '    frame.keep_secondary(secondary);\n    frame.pass_fade_arguments(call);'),
    ('light-hotter', SYSTEMS, 'residue.diffuse_limited=sse::greater(diffuse,overall);', 'residue.diffuse_limited=sse::greater(overall,diffuse);'),
    ('light-raise', SYSTEMS, 'residue.secondary_limited=sse::greater(residue.secondary,stored_diffuse);',
     'residue.secondary_limited=!sse::greater(stored_diffuse,residue.secondary);'),
    ('light-limit-value', SYSTEMS, 'fade_field(offsetof(LightDatum,diffuse_fade)),overall);', 'fade_field(offsetof(LightDatum,diffuse_fade)),diffuse);'),
    ('light-copy-source', SYSTEMS, 'memory.load32(diffuse_copy,fade_field(offsetof(LightDatum,diffuse_fade)));',
     'memory.load32(diffuse_copy,fade_field(offsetof(LightDatum,overall_fade)));'),
    ('light-reload-field', SYSTEMS, 'memory.load_float(diffuse_reload,fade_field(offsetof(LightDatum,diffuse_fade)));',
     'memory.load_float(diffuse_reload,fade_field(offsetof(LightDatum,specular_fade)));'),
    # C2A80 logic
    ('fade-zero', SYSTEMS, 'const float zero=0.0f;', 'const float zero=-0.0f;'),
    ('fade-ratio', SYSTEMS, 'const auto ratio=sse::divide(distance,divisor);', 'const auto ratio=sse::divide(divisor,distance);'),
    ('fade-over', SYSTEMS, 'else if(sse::greater(ratio,one_value))', 'else if(sse::greater(one_value,ratio))'),
    ('fade-divisor', SYSTEMS, 'const auto divisor=range_divides?call.range:minimum_range;',
     'const auto divisor=range_divides?minimum_range:call.range;'),
    ('fade-range-compare', SYSTEMS, 'const bool range_divides=sse::greater(call.range,minimum_range);',
     'const bool range_divides=!sse::greater(minimum_range,call.range);'),
    ('fade-offset-index', SYSTEMS, 'element(distances,std::uint32_t(call.distance_class),4)',
     'element(distances,std::uint32_t(call.distance_class),2)'),
    ('fade-within-range', SYSTEMS, 'else result={sse::subtract(one_value,ratio),ratio,divisor,divisor,',
     'else result={sse::subtract(one_value,ratio),ratio,call.range,divisor,'),
    ('fade-inside-divisor', SYSTEMS, 'if(inside)result={sse::subtract(one_value,zero),zero,call.range,zero,',
     'if(inside)result={sse::subtract(one_value,zero),zero,call.range,divisor,'),
    ('fade-beyond-value', SYSTEMS, '{sse::subtract(one_value,one_value),one_value,', '{sse::subtract(one_value,ratio),one_value,'),
    ('fade-length-order', SYSTEMS, r'fld %%st(3)\n\tfmul %%st(4),%%st', r'fld %%st(2)\n\tfmul %%st(3),%%st'),
    ('fade-spill-c1', SYSTEMS, 'return {result,(status&0x200u)!=0};', 'return {result,false};'),
    ('fade-radius-sign', SYSTEMS, r'flds %[radius]\n\tfsubr %%st(1),%%st', r'flds %[radius]\n\tfsub %%st(1),%%st'),
    ('fade-operand-form', SYSTEMS, r'flds %[offset]\n\tfsubr %%st(1),%%st\n\tfstp %%st(1)\n\t', r'fsubs %[offset]\n\t'),
    ('fade-camera-order', SYSTEMS,
     '    const auto y=memory.load_float(position_y,at(call.position,4));\n'
     '    const auto camera_y_value=memory.load_float(camera_y,globals::camera_position+4u);',
     '    const auto camera_y_value=memory.load_float(camera_y,globals::camera_position+4u);\n'
     '    const auto y=memory.load_float(position_y,at(call.position,4));', 'verify'),
    ('fade-table-order', SYSTEMS,
     '    const auto distances=frame.distance_table();\n'
     '    const auto camera_z_value=memory.load_float(camera_z,globals::camera_position+8u);',
     '    const auto camera_z_value=memory.load_float(camera_z,globals::camera_position+8u);\n'
     '    const auto distances=frame.distance_table();', 'verify'),
    # C35D0 / C3300 / C2A80 adapter
    ('light-depth', HEADER, 's.fp.count>3u', 's.fp.count>4u'),
    ('light-child-flags', HEADER, 'gp[edx]=this->flags.logic(0,4);', 'gp[edx]=0;'),
    ('light-child-array', HEADER, ';gp[ecx]=gp[ebp];', ';gp[ecx]=gp[ebp]+4u;'),
    ('light-kept-array', HEADER, 'walk.lights=gp[ebp];', 'walk.lights=gp[ecx];'),
    ('light-exit-flags', HEADER, 'this->compare32(logic::none,logic::none);', 'this->compare32(logic::none,0xfffffffeu);'),
    ('light-pop-order', HEADER, 'gp[esi]=this->pop(0xc3690u);gp[ebx]=this->pop(0xc3691u);', 'gp[ebx]=this->pop(0xc3690u);gp[esi]=this->pop(0xc3691u);'),
    ('light-walk-saves', HEADER, 'this->push(0xc3604u,gp[ebx]);this->push(0xc3605u,gp[esi]);',
     'this->push(0xc3604u,gp[esi]);this->push(0xc3605u,gp[ebx]);'),
    ('light-handle-register', HEADER, 'else {gp[edi]=gp[eax];this->reached(lv_more);}', 'else {this->reached(lv_more);}'),
    ('light-index-register', HEADER, 'case datums:gp[ecx]=v;gp[esi]=gp[edi]&0xffffu;break;', 'case datums:gp[ecx]=v;gp[esi]=gp[edi];break;'),
    ('light-data-register', HEADER, 'case datums:gp[ecx]=v;', 'case datums:gp[ecx]=v+1u;'),
    ('light-first-register', HEADER, 'case first_data:gp[edx]=v;', 'case first_data:gp[edx]=v+4u;'),
    ('light-identifier-register', HEADER, 'gp[edi]=(sx16(v)<<16)|gp[eax];', 'gp[edi]=(sx16(v)<<16)|gp[ecx];'),
    ('light-next-register', HEADER, 'case next_size:gp[edx]=v;break;', 'case next_size:gp[edx]=v+4u;break;'),
    ('light-scan-register', HEADER, 'first_probe=false;gp[ecx]=sar(gp[eax],5);', 'first_probe=false;gp[ecx]=gp[eax]>>4;'),
    ('light-probe-step', HEADER, 'if(!first_probe)++gp[eax];', '++gp[eax];'),
    ('light-start-register', HEADER, 'gp[eax]=gp[edi]==logic::none?0u:gp[esi]+1u;', 'gp[eax]=gp[edi]==logic::none?0u:gp[esi]+2u;'),
    ('light-light-register', HEADER, 'gp[edx]=address-std::uint32_t(offsetof(logic::light_fades::LightDatum,definition));',
     'gp[edx]=address;'),
    ('light-volume-pop', HEADER, 'case volume_store:gp[edi]=this->pop(0xc33f8u);return 0xc33f9u;', 'case volume_store:return 0xc33f9u;'),
    ('light-return-address', HEADER, '{0xc33d8u,0xc33ddu,0xc33f0u,0xc33f5u}', '{0xc33d8u,0xc33ddu,0xc33f0u,0xc33f4u}'),
    ('light-slot', HEADER, 'this->gp[esp]+0x10u,4,std::bit_cast<std::uint32_t>(value));}', 'this->gp[esp]+0x0cu,4,std::bit_cast<std::uint32_t>(value));}'),
    ('light-saved-ecx', HEADER, 'gp[ecx]=this->pop(0xc3426u);', 'gp[ecx]=this->pop(0xc3426u)^1u;'),
    ('light-xmm-fades', HEADER, 'x[6]=scalar(residue.overall);x[7]=scalar(residue.diffuse);',
     'x[6]=scalar(residue.diffuse);x[7]=scalar(residue.overall);'),
    ('light-xmm-secondary', HEADER, 'x[0]=scalar(residue.secondary);', 'x[0]=scalar(v.clamped);'),
    ('light-radius-argument', HEADER, 'this->push(site.radius_push,std::bit_cast<std::uint32_t>(call.radius));',
     'this->push(site.radius_push,std::bit_cast<std::uint32_t>(call.range));'),
    ('fade-spill-status', HEADER, 'status=std::uint16_t((status&~0x200u)|(distance.rounded_up?0x200u:0u));',
     'status=std::uint16_t(status&~0x200u);'),
    ('fade-ret', HEADER, 'this->ret(0xc2b2cu,8);break;', 'this->ret(0xc2b2cu,4);break;'),
    ('fade-table-slot', HEADER, 'return this->read(0xc2aa2u,this->gp[esp]+8u,4);', 'return this->read(0xc2aa2u,this->gp[esp]+4u,4);'),
    ('fade-radius-slot', HEADER, 'this->read(0xc2ac4u,this->gp[esp]+4u,4)', 'this->read(0xc2ac4u,this->gp[esp]+8u,4)'),
    ('fade-spill-slot', HEADER, 'this->store(0xc2acbu,this->gp[esp]+4u,4,', 'this->store(0xc2acbu,this->gp[esp]+8u,4,'),
    # 176A40 / 252100 logic
    ('state-bit-clear', SYSTEMS, 'kept.remaining&=~(1u<<(kept.input&31u));', 'kept.remaining&=~(2u<<(kept.input&31u));'),
    ('state-output', SYSTEMS, 'element(kept.outputs,kept.input,4),result.value);', 'element(kept.outputs,kept.input+1u,4),result.value);'),
    ('state-kept-particle', SYSTEMS, 'kept.particle=records.particle;', 'kept.particle=records.location;'),
    ('state-record-order', SYSTEMS,
     '        records.particle=frame.particle();\n        records.location=frame.location();',
     '        records.location=frame.location();\n        records.particle=frame.particle();', 'verify'),
    ('value-range', SYSTEMS, 'if(input>=input_count)return finish(result);', 'if(input>input_count)return finish(result);'),
    ('value-word', SYSTEMS, 'memory.load16(random_word,element(records.particle+base,input,2));',
     'memory.load8(random_word,element(records.particle+base,input,2));'),
    ('value-word-sign', SYSTEMS, 'sse::from_integer(std::int32_t(word))', 'sse::from_integer(std::int32_t(std::int16_t(word)))'),
    ('value-word-low', SYSTEMS, 'particle_random_3_4_base=0xfffffffcu;', 'particle_random_3_4_base=0xfffffffeu;'),
    ('value-system-input', SYSTEMS, 'emitter_random_base=0x14u;', 'emitter_random_base=0x10u;'),
    ('value-time', SYSTEMS,
     'const auto ticks=sse::from_integer(signed_value(memory.load32(elapsed_ticks,at(time,offsetof(GameTime,elapsed_ticks)))));',
     'const auto ticks=std::bit_cast<float>(memory.load32(elapsed_ticks,at(time,offsetof(GameTime,elapsed_ticks))));'),
    ('value-owner', SYSTEMS, 'element(data,handle_index(handle),sizeof(EffectDatum));', 'element(data,handle_index(handle),0x18cu);'),
    ('value-no-owner', SYSTEMS, 'if(handle==none) {', 'if(handle==0xfffffffeu) {'),
    ('value-owner-field', SYSTEMS, 'reader==Reader::effect_a_scale?offsetof(EffectDatum,a_scale):offsetof(EffectDatum,b_scale);',
     'reader==Reader::effect_a_scale?offsetof(EffectDatum,b_scale):offsetof(EffectDatum,a_scale);'),
    ('value-fraction-spill', SYSTEMS, 'memory.store_float(spill,frame.local_scratch(),value);',
     'memory.store_float(spill,frame.local_scratch()+4u,value);'),
    ('value-truncate', SYSTEMS, 'asm volatile("cvttss2si %1,%0"', 'asm volatile("cvtss2si %1,%0"'),
    ('value-rotation-reread', SYSTEMS,
     'const auto again=memory.load_float(rotation_again,at(records.particle,offsetof(Particle,rotation)));',
     'const auto again=rotation_value;', 'verify'),
    ('value-projection-order', SYSTEMS, 'auto sum=sse::add(z_term,y_term);\n        const auto fx=',
     'auto sum=sse::add(y_term,z_term);\n        const auto fx='),
    ('value-cross', SYSTEMS, 'const auto left_y_value=sse::subtract(up_z_forward_x,forward_z_up_x);',
     'const auto left_y_value=sse::subtract(forward_z_up_x,up_z_forward_x);'),
    ('value-left-order', SYSTEMS, 'sse::subtract(sse::multiply(up_y_value,forward_z_value),up_z_forward_y);',
     'sse::subtract(up_z_forward_y,sse::multiply(up_y_value,forward_z_value));'),
    ('value-cross-store', SYSTEMS,
     '        memory.store_float(Access::left_y,at(left,offsetof(Vector,y)),left_y_value);\n'
     '        memory.store_float(Access::left_z,at(left,offsetof(Vector,z)),left_z_value);',
     '        memory.store_float(Access::left_z,at(left,offsetof(Vector,z)),left_z_value);\n'
     '        memory.store_float(Access::left_y,at(left,offsetof(Vector,y)),left_y_value);'),
    ('value-wrap', SYSTEMS, 'if(sse::greater(0.0f,result.value)) {', 'if(sse::greater(result.value,0.0f)) {'),
    ('value-byte', SYSTEMS, 'at(records.location,offsetof(Location,random))', 'at(records.location,offsetof(Location,random)-1u)'),
    ('value-read-order', SYSTEMS,
     '        const auto up_y_value=memory.load_float(up_y,globals::camera+offsetof(Camera,up)+4u);\n'
     '        const auto forward_z_value=memory.load_float(forward_z,globals::camera+offsetof(Camera,forward)+8u);',
     '        const auto forward_z_value=memory.load_float(forward_z,globals::camera+offsetof(Camera,forward)+8u);\n'
     '        const auto up_y_value=memory.load_float(up_y,globals::camera+offsetof(Camera,up)+4u);', 'verify'),
    # 176A40 / 252100 adapter
    ('state-mask-slot', HEADER, 'void clear_input(){this->store(0x176a4cu,this->gp[esp]+0x1cu,4,0);}',
     'void clear_input(){this->store(0x176a4cu,this->gp[esp]+0x18u,4,0);}'),
    ('state-ret', HEADER, 'this->ret(0x176a9fu,0x14u);', 'this->ret(0x176a9fu,0x10u);'),
    ('state-args', HEADER, 'return this->read(0x176a6du,this->gp[esp]+0x20u,4);', 'return this->read(0x176a6du,this->gp[esp]+0x1cu,4);'),
    ('state-exit-flags', HEADER, 'this->test32(gp[ebx]);\n        if(looped)', 'this->test32(gp[ebx]|1u);\n        if(looped)'),
    ('state-saves', HEADER, 'this->push(0x176a56u,gp[esi]);this->push(0x176a57u,gp[edi]);',
     'this->push(0x176a56u,gp[edi]);this->push(0x176a57u,gp[esi]);'),
    ('state-input-register', HEADER, 'gp[ebx]=kept.remaining;gp[eax]=kept.input;', 'gp[ebx]=kept.remaining;'),
    ('value-keep-particle', HEADER, 'gp[edi]=kept.input;gp[esi]=kept.particle;', 'gp[edi]=kept.input;'),
    ('value-table', HEADER, 'switch(this->read(0x252112u,0x252430u+input*4u,4)) {', 'switch(this->read(0x252112u,0x252434u+input*4u,4)) {'),
    ('value-frame', HEADER, 'x[1]=_mm_setzero_ps();gp[esp]-=0x1cu;x[0]=x[1];', 'x[1]=_mm_setzero_ps();gp[esp]-=0x18u;x[0]=x[1];'),
    ('value-child-flags', HEADER, 'gp[ecx]=vector;this->test32(gp[edx]);', 'gp[ecx]=vector;'),
    ('value-child-vector', HEADER, 'std::uint32_t local_vector(){return this->gp[esp]+4u;}', 'std::uint32_t local_vector(){return this->gp[esp]+8u;}'),
    ('value-scratch', HEADER, 'x[0]=scalar(scratch.left_y);x[1]=scalar(scratch.left_z);', 'x[0]=scalar(scratch.left_z);x[1]=scalar(scratch.left_y);'),
    ('value-animation-scratch', HEADER, 'this->s.xmm[0]=scalar(scratch.offset_z);', 'this->s.xmm[0]=_mm_setzero_ps();'),
    ('value-kept-input', HEADER, 'kept.outputs=gp[ebp];kept.input=gp[edi];', 'kept.outputs=gp[ebp];'),
    ('value-truncated-register', HEADER, 'if(result.truncated)gp[eax]=std::uint32_t(*result.truncated);', ''),
    ('value-converted-lanes', HEADER, 'x[2]=replace_low(x[2],*result.converted);', 'x[2]=scalar(*result.converted);'),
    ('value-word-register', HEADER, 'case random_word:gp[current==Reader::particle_random_1_2?eax:ecx]=v;break;',
     'case random_word:gp[eax]=v;break;'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper():
    spec = importlib.util.spec_from_file_location('native_effect_owner_replacements', HELPER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


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
    done = subprocess.run(command, capture_output=True, text=True)
    if done.returncode:
        raise ValueError(f'{binary.name} {mode or "release"} failed: {done.stderr.strip()[-600:]}')
    return json.loads(done.stdout)


def check_binding(module):
    lens, light = LENS_UNIT.read_text(), LIGHT_UNIT.read_text()
    units = [(lens, '20680'), (light, 'c35d0')] + [(path.read_text(), '176a40') for path in (STATE_UNIT, COMBAT_UNIT, DEATH_UNIT)]
    for text, owner in units:
        output, installed = module.install(text, text)
        if installed != [owner] or module.prefix(owner, text) not in output or module.REFERENCES_BEGIN not in output:
            raise ValueError(f'Effect owner {owner} binding did not install')
        if module.install(text, output)[1]:
            raise ValueError(f'Effect owner {owner} binding is not idempotent')
    # A changed body anywhere in the closure keeps the translated route.
    for name, anchor, changed in (('20680', '0x531fc0u', '0x531fa0u'), ('c35d0', '0x110u', '0x10cu'),
                                  ('c3300', '0x46a0ccu', '0x46a0c8u'), ('c2a80', '0x46a178u', '0x46a17cu')):
        text = lens if name == '20680' else light
        found = module.body(text, name)
        edited = text.replace(found[0], found[0].replace(anchor, changed, 1), 1)
        if edited == text or module.install(edited, edited)[1]:
            raise ValueError(f'Effect owner binding accepted a changed {name} body')
    state = STATE_UNIT.read_text()
    found = module.body(state, '252100')
    edited = state.replace(found[0], found[0].replace('0x46a160u', '0x46a164u', 1), 1)
    if module.install(edited, edited)[1]:
        raise ValueError('Effect owner binding accepted a changed 252100 body')
    # A direct-route 252100 without the 4EFA0 body it calls keeps the translated route.
    unrouted = state.replace(module.body(state, '4efa0')[0], '', 1)
    if module.install(unrouted, unrouted)[1]:
        raise ValueError('Effect owner binding accepted a missing 4EFA0 body')
    missing = light.replace(module.body(light, '16ea60')[0], '', 1)
    if module.install(missing, missing)[1]:
        raise ValueError('Effect owner binding accepted a missing 16EA60 body')
    mapped = light.replace('case 0x46a110u:return 0x46a110u;', 'case 0x46a110u:return 0x46a114u;', 1)
    if mapped == light or module.install(mapped, mapped)[1]:
        raise ValueError('Effect owner binding accepted a non-identity operand')
    return dict(install=True, idempotent=True, units=len(units), changed_bodies_refused=5, missing_child_refused=2,
                non_identity_refused=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
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
    sources = [LENS_UNIT, LIGHT_UNIT, STATE_UNIT, COMBAT_UNIT, DEATH_UNIT, HEADER, SYSTEMS, SHARED, SHARED_SYSTEMS, TEST, HELPER,
               Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    module = helper()
    lens, light = LENS_UNIT.read_text(), LIGHT_UNIT.read_text()
    state, death = STATE_UNIT.read_text(), DEATH_UNIT.read_text()
    if module.admitted(lens) != ['20680'] or module.admitted(light) != ['c35d0'] or \
            module.admitted(state) != ['176a40'] or module.admitted(death) != ['176a40'] or \
            module.admitted(COMBAT_UNIT.read_text()) != ['176a40']:
        raise ValueError('Canonical effect owner bodies drifted from their pins')
    (scratch / 'state.inc').write_text(''.join(module.body(state, name)[0] + '\n' for name in ('252100', '176a40')))
    (scratch / 'state_dispatch.inc').write_text(''.join(module.body(death, name)[0] + '\n' for name in ('252100', '176a40')))
    (scratch / 'state_dispatch_references.inc').write_text(
        ''.join(module.reference(module.body(death, name), name, '176a40') for name in ('252100', '176a40')))
    (scratch / 'lens.inc').write_text(module.body(lens, '20680')[0] + '\n')
    (scratch / 'light.inc').write_text(''.join(module.body(light, name)[0] + '\n' for name in ('c2a80', 'c3300', 'c35d0')))
    references = module.reference(module.body(lens, '20680'), '20680', '20680')
    for name in ('c2a80', 'c3300', 'c35d0'):
        references += module.reference(module.body(light, name), name, 'c35d0')
    for name in ('252100', '176a40'):
        references += module.reference(module.body(state, name), name, '176a40')
    (scratch / 'references.inc').write_text(references)
    report = dict(passed=False, pins=module.PINS, binding=check_binding(module))
    release = compile_harness(scratch, 'effect-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    switched = run(release, max(args.cases // 8, 500), 'reference')
    block = re.search(r'enum Branch : unsigned \{(.*?)\};', HEADER.read_text(), re.S)[1]
    block = re.sub(r'//[^\n]*', '', block)
    names = [name for name in re.sub(r'\s', '', block).split(',') if name and name != 'branch_count']
    coverage = result['coverage'] | census['coverage'] | verify['coverage'] | switched['coverage']
    hit = {name for index, name in enumerate(names) if coverage >> index & 1}
    missing = sorted(set(names) - hit - UNREACHABLE)
    reached_unreachable = sorted(hit & UNREACHABLE)
    report.update(release=result, census=census, verify=verify, reference_switch=switched, branches=names,
                  covered=sorted(hit), missing=missing, unreachable=sorted(UNREACHABLE))
    if reached_unreachable:
        raise ValueError(f'Branches documented as unreachable were reached: {reached_unreachable}')
    if missing:
        raise ValueError(f'Uncovered native effect owner branches: {missing}')
    if switched['light'] or not switched['lens']:
        raise ValueError('The C35D0 reference switch did not keep only C35D0 translated')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'effect-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 5, 1000))
        report['sanitized_verify'] = run(sanitized, 400, 'verify')
        sanitized.unlink()
    release.unlink()
    texts = {HEADER: HEADER.read_text(), SYSTEMS: SYSTEMS.read_text()}

    def screen(mutant):
        name, target, old, new = mutant[:4]
        mode = mutant[4:] and [mutant[4]] or []
        text = texts[target]
        if text.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'mutants' / name / 'rendering/effects'
        directory.mkdir(parents=True)
        (directory / target.name).write_text(text.replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        try:
            runs = subprocess.run([str(binary), str(args.mutant_cases), *mode], capture_output=True, text=True, timeout=600)
            rejected, diagnostic = runs.returncode != 0, runs.stderr.strip()[-200:]
        except subprocess.TimeoutExpired:
            rejected, diagnostic = True, 'timed out'
        binary.unlink()
        return dict(name=name, rejected=rejected, diagnostic=diagnostic)
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
    print(f"PASS {result['compared']} native effect owner comparisons ({result['lens']} 20680, {result['light']} C35D0, "
          f"{result['state']} 176A40; "
          f"{result['ordered_stores']} ordered stores, {result['children']} genuine children, {result['faults']} faults), "
          f"census {census['compared']}, verify {verify['compared']}, reference switch {switched['compared']}, "
          f"{len(names) - len(UNREACHABLE)}/{len(names) - len(UNREACHABLE)} reachable branches, {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
