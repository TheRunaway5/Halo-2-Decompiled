#!/usr/bin/env python3
"""Prove the native TU1.5 live-support systems C9550, 1EC970, 1F7F60, C1E90,
110350, 243F20, 32CC0C, 16E600, 171010, 1211C0, 1F7E90, 32A4E0, 16E4B0,
16E450, 22A790 and 1F8200 (group L): the logic in native_tu15_live_support_systems.hpp
through the boundary adapters in native_tu15_live_support_leaves.hpp. 32CC0C
is proven in both caller x87 models (the live-support form, and the C1-keeping
form of the other units). 22A790 (4057F0) and 1F8200 (144090, 1E6980) call
genuine children through the leaf framework's child boundary
(native_tu15_leaf_children.hpp): each child is a deterministic stand-in in
both runs, and verify mode replays the binder's reference copy with the
child calls routed to replay_child.

Run through tools/run_test.py. Extracts the unmodified translated bodies from
the checked-in live-support unit (pinned by
tools/native_live_support_leaves_replacements.py), in their production cursor
form, compiles tests/native_tu15_live_support_leaves_tests.cpp against them
and compares every seeded case (ordered store events with preimages, bank,
registers, flags, XMM, x87 status, MXCSR, control word, cursor; faults by
exception, cursor and store prefix) in Release, leaf verify mode, census mode
and ASan/UBSan. Every conditional branch of every body must be observed in
both directions, except the declared unreachable ones. Then screens
deliberate defects in the logic and the adapters: each mutant is an
out-of-tree copy of one header placed first on the include path and must be
rejected by the release comparison or, failing that, by leaf verify mode
(which replays the exact access order of the leaves with children). A fault compares the exception, cursor and the stores outside the
leaf's own dead frame. The production
binding must install through the real specializer, be idempotent and refuse
a changed body or a nonidentity selected operand. Source hashes are rechecked
after the screen.
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
UNIT = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
COPIES = [ROOT / 'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
          ROOT / 'src/gameplay/units/source_combat_action_tu15_generated.cpp',
          ROOT / 'src/rendering/effects/source_lens_flare_tu15_generated.cpp',
          ROOT / 'src/rendering/effects/source_lens_query_tu15_generated.cpp',
          ROOT / 'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',
          ROOT / 'src/rendering/models/source_model_render_tu15_generated.cpp',
          ROOT / 'src/campaign/scripting/source_recorded_animation_tu15_generated.cpp',
          ROOT / 'src/campaign/source_level_handles_new_map_tu15_generated.cpp',
          ROOT / 'src/rendering/renderer/source_render_object_startup_tu15_generated.cpp',
          ROOT / 'src/ui/source_ui_script_startup_tu15_generated.cpp',
          ROOT / 'src/rendering/source_geometry_cache_frame_tu15_generated.cpp',
          ROOT / 'src/runtime/source_map_world_support_tu15_generated.cpp',
          ROOT / 'src/platform/source_final_startup_storage_tu15_generated.cpp',
          ROOT / 'src/rendering/renderer/source_renderer_debug_storage_tu15_generated.cpp',
          ROOT / 'src/rendering/renderer/source_renderer_transparent_storage_tu15_generated.cpp',
          ROOT / 'src/audio/source_startup_sound_storage_tu15_generated.cpp',
          ROOT / 'src/physics/physics_pool_startup_runtime_generated.cpp']
# The canonical 16E600 copy (run_16d220, observation-PC form).
WORLD = ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp'
# The C1-keeping 32CC0C form (the x87 store helpers report C1).
TRACKED = ROOT / 'src/gameplay/units/source_combat_action_tu15_generated.cpp'
# The 138D90 form whose FISTP spills leave C1 as the FLD cleared it.
UNTRACKED = ROOT / 'src/runtime/source_map_world_support_tu15_generated.cpp'
# The unit whose copies report canonical store PCs (store-site form).
POOL = ROOT / 'src/physics/physics_pool_startup_runtime_generated.cpp'
ADAPTER = ROOT / 'src/physics/runtime/native_tu15_live_support_leaves.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_live_support_systems.hpp'
TEST = ROOT / 'tests/native_tu15_live_support_leaves_tests.cpp'
MATRIX = ROOT / 'src/physics/runtime/native_tu15_matrix_leaves.hpp'
CHILDREN_HEADER = ROOT / 'src/physics/runtime/native_tu15_leaf_children.hpp'
HELPER = ROOT / 'tools/native_live_support_leaves_replacements.py'
SPECIALIZER = ROOT / 'tools/specialize_native_operands.py'
INSTRUCTION = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
# A string instruction's per-element delivery (REP STOS) has no braces.
REPEAT = re.compile(r'if\(s\.services\.instruction\)s\.services\.instruction\((0x[0-9a-f]+u)\);')


def cursor_form(body):
    # The production cursor form: each delivery is a literal cursor store.
    return REPEAT.sub(r's.instruction_literal(\1);', INSTRUCTION.sub(r's.instruction_literal(\1);', body))
LINE = re.compile(r'^(?:L[0-9a-f]+: )?if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+)u\);\} (.*)$')
BRANCH = re.compile(r'^if\((.*)\)goto L([0-9a-f]+);$')
NAMES = ['c9550', '1ec970', '1f7f60', 'c1e90', '110350', '243f20', '32cc0c', '16e600', '171010', '1211c0', '1f7e90', '32a4e0', '16e4b0', '16e450', '22a790',
         '1f8200', '138d90', '179cd0']
# Harness leaves: the bound names plus the tracked 32CC0C form.
# The physics pool startup unit's copies, in the store-site form.
POOL_COPIES = {'16d070': '16e450', '16d0d0': '16e4b0', '16d220': '16e600', '326970': '32a4e0'}
HARNESS = NAMES + ['32cc0c_tracked', '138d90_untracked'] + [f'{leaf}_pool' for leaf in POOL_COPIES.values()]
# Children whose translated bodies the references call (16E600 -> 32A4E0,
# 1211C0 -> 123210).
CHILDREN = ['123210']
# Conditional branches whose other direction no input can take.
UNREACHABLE = {
    # 1EC970: a hit index is non-negative (JS before the scan), never -1.
    (0x1ec9f1, 0x1ec9be): 'hit index is never -1',
    # 1F7F60: an advanced index is a byte or a non-negative loop index.
    (0x1f7fda, 0x1f7fdc): 'advanced index is never -1',
    # 179CD0: the walk's handle is never -1 (EBX is callee-saved across the
    # children), and the next index, (handle & FFFF) + 1, is positive.
    (0x179d7d, 0x179d7f): 'walk handle is never -1',
    (0x179d91, 0x179dbf): 'next index is positive',
}
MUTANTS = [
    (SYSTEMS, 'c9550-mask', 'rows+(handle&0xffffu)*object_header::size', 'rows+(handle&0xffffffu)*object_header::size'),
    (SYSTEMS, 'c9550-bound', 'const auto end=listed_modes+listed_mode_count*4u;', 'const auto end=listed_modes+(listed_mode_count-1u)*4u;'),
    (SYSTEMS, 'c9550-first-match', 'while(entry<end && !found) {', 'while(entry<end) {'),
    (SYSTEMS, 'c9550-offset', 'widen16(memory.load16(Access::state_offset', '(memory.load16(Access::state_offset'),
    (SYSTEMS, 'iterator-sign', 'if(signed_value(start)<0)return {};', 'if(signed_value(start)<-1)return {};'),
    (SYSTEMS, 'iterator-shift', 'bits+std::uint32_t(signed_value(index)>>5)*4u', 'bits+std::uint32_t(signed_value(index)>>4)*4u'),
    (SYSTEMS, 'iterator-salt', '(salt<<16)|*index', '(salt<<15)|*index'),
    (SYSTEMS, 'iterator-reset', 'memory.load32(Access::maximum_count,array+data_array::maximum_count)', 'memory.load32(Access::maximum_count,array+data_array::datum_size)'),
    (SYSTEMS, 'iterator-filter', 'if(!tested)continue;', 'if(tested)continue;'),
    (SYSTEMS, 'iterator-publish', 'it+iterator::published_handle,handle);', 'it+iterator::published_handle,element);'),
    (SYSTEMS, 'channel-advance', 'if(signed_value(entry_index+1u)<signed_value(count)) {', 'if(signed_value(entry_index+1u)<=signed_value(count)) {'),
    (SYSTEMS, 'channel-start', 'for(auto g=current_group+1u;', 'for(auto g=current_group+2u;'),
    (SYSTEMS, 'channel-empty', 'if(signed_value(count)<=0)continue;', 'if(signed_value(count)<0)continue;'),
    (SYSTEMS, 'channel-pack', '((found_entry&0xffu)<<8)', '((found_entry&0x7fu)<<8)'),
    (SYSTEMS, 'channel-stride', '+found_entry*entry::size;', '+found_entry*0x40u;'),
    (SYSTEMS, 'channel-reset', 'Access::exhausted_position_store,it+iterator::position,0xffffffffu);', 'Access::exhausted_position_store,it+iterator::position,0u);'),
    (SYSTEMS, 'channel-residue', '            residue.last_examined=count;\n', '\n'),
    (SYSTEMS, 'bit-shift', 'vector+std::uint32_t(signed_value(index)>>5)*4u', 'vector+(index>>5)*4u'),
    (SYSTEMS, 'bit-mask', 'return (word&(1u<<(index&31u)))!=0;', 'return (word&(1u<<(index&15u)))!=0;'),
    (SYSTEMS, 'level-threshold', 'ordered_greater(threshold,load_float(Access::level_compare,block+block::level))', 'ordered_greater(load_float(Access::level_compare,block+block::level),threshold)'),
    (SYSTEMS, 'level-ramp', 'level=add(level,load_float(Access::ramp_down', 'level=subtract(level,load_float(Access::ramp_down'),
    (SYSTEMS, 'level-ceiling', 'if(ordered_greater(level,residue.ceiling))level=residue.ceiling;', 'if(!ordered_greater(residue.ceiling,level))level=residue.ceiling;'),
    (SYSTEMS, 'level-decay', 'if(!ordered_greater(level,0.0f))level=0.0f;\n    residue.byte_scale', 'if(ordered_greater(0.0f,level))level=0.0f;\n    residue.byte_scale'),
    (SYSTEMS, 'level-order', 'multiply(multiply(multiply(from_int(value),residue.byte_scale),factor),residue.output_scale)', 'multiply(multiply(multiply(from_int(value),factor),residue.byte_scale),residue.output_scale)'),
    (SYSTEMS, 'level-clamp', 'else if(clamped>127)clamped=127;', 'else if(clamped>127)clamped=126;'),
    (SYSTEMS, 'level-floor', 'if(clamped<-128)clamped=-128;', 'if(clamped<-128)clamped=-127;'),
    (SYSTEMS, 'level-any', 'if(clamped&0xff)result.any_byte_nonzero=true;', 'if(clamped&0x7f)result.any_byte_nonzero=true;'),
    (SYSTEMS, 'level-equal', 'if(!quiet_ordered_equal(residue.level,0.0f)) {', 'if(quiet_ordered_equal(residue.level,0.0f)) {'),
    (SYSTEMS, 'level-lcg', '*0x19660du+0x3c6ef35fu', '*0x19660du+0x3c6ef35eu'),
    (SYSTEMS, 'level-reload', '(residue.reload_product+0x1eu)&0xffu', '(residue.reload_product+0x1fu)&0xffu'),
    (SYSTEMS, 'level-step', '(countdown-1u)&0xffu', '(countdown+1u)&0xffu'),
    (SYSTEMS, 'level-default', '?scale:residue.default_scale;', '?scale:residue.output_scale;'),
    (ADAPTER, 'adapter-c9550-flags', 'if(result.listed)m.flags.logic(1u,1);', 'if(result.listed)m.flags.logic(0u,1);'),
    (ADAPTER, 'adapter-c1e90-flags', 'm.gp[b::eax]=m.flags.minus(0u,set?0xffffffffu:0u,4);', 'm.gp[b::eax]=m.flags.logic(set?1u:0u,4);'),
    (ADAPTER, 'adapter-iterator-slot', 'io.st(0x1ec97eu,slot,4,0u);', 'io.st(0x1ec97eu,slot,4,1u);'),
    (ADAPTER, 'adapter-iterator-flags', 'm.flags.logic(step.last_flag_tested,1);', 'm.flags.logic(step.element,4);'),
    (ADAPTER, 'adapter-channel-edx', 'if(step.residue.last_examined)gp[b::edx]=*step.residue.last_examined;', ';'),
    (ADAPTER, 'adapter-level-lahf', '(((m.flags.bits()|2u)&0xffu)<<8)', '((m.flags.bits()&0xffu)<<8)'),
    (ADAPTER, 'adapter-level-exitflags', 'gp[b::esp]=m.flags.plus(gp[b::esp],8u,4);', 'gp[b::esp]+=8u;'),
    (ADAPTER, 'adapter-level-xmm2', 'x[2]=lane(r.byte_scale);', 'x[2]=lane(r.default_scale);'),
    (ADAPTER, 'adapter-level-frame', 'io.st(0x110431u,esp+0x10u,4,count);', 'io.st(0x110431u,esp+0x10u,4,count+1u);'),
    (SYSTEMS, 'vertex-layout', 'case 2:return Layout::compressed;', 'case 2:return Layout::compressed_with_node;'),
    (SYSTEMS, 'vertex-stride', 'case Layout::compressed:return 6u;', 'case Layout::compressed:return 8u;'),
    (SYSTEMS, 'vertex-node-offset', 'l==Layout::uncompressed_with_node?0xcu:6u', 'l==Layout::uncompressed_with_node?0xcu:4u'),
    (SYSTEMS, 'vertex-scale-order', 'multiply(add(multiply(add(multiply(from_int(q),k.two),k.one),k.step),k.half),range)',
     'multiply(add(multiply(add(multiply(from_int(q),k.one),k.two),k.step),k.half),range)'),
    (SYSTEMS, 'vertex-range', 'return subtract(maximum,load_float(', 'return add(maximum,load_float('),
    (SYSTEMS, 'vertex-widen', 'q[axis]=signed_value(widen16(memory.load16(', 'q[axis]=signed_value((memory.load16('),
    (SYSTEMS, 'vertex-offset', 'load_float(at(Role::offset,axis),bounds_address+bounds::minimum[axis])',
     'load_float(at(Role::offset,axis),bounds_address+bounds::maximum[axis])'),
    (SYSTEMS, 'vertex-store-order', '        memory.store32(at(Role::store,1),point+4u,result.point[1]);\n        memory.store32(at(Role::store,2),point+8u,result.point[2]);',
     '        memory.store32(at(Role::store,2),point+8u,result.point[2]);\n        memory.store32(at(Role::store,1),point+4u,result.point[1]);'),
    (SYSTEMS, 'vertex-spill-order', '        result.point[1]=frame.spill(layout,1u,y);\n        const auto z=coordinate(2);\n        result.point[0]=frame.spill(layout,0u,x);',
     '        result.point[0]=frame.spill(layout,0u,x);\n        const auto z=coordinate(2);\n        result.point[1]=frame.spill(layout,1u,y);'),
    (SYSTEMS, 'vertex-node', 'has_node?memory.load8(at(Role::node),result.residue.element+node_offset(layout)):0u;',
     'has_node?memory.load8(at(Role::node),result.residue.element+node_offset(layout)):1u;'),
    (SYSTEMS, 'vertex-default', 'memory.load32(at(Role::coordinate,axis),source+axis*4u)', 'memory.load32(at(Role::coordinate,axis),source+axis*2u)'),
    (SYSTEMS, 'ftol-zero-mask', 'if((high&0x7fffffffu)==0) {', 'if((high&0xffffffffu)==0) {'),
    (SYSTEMS, 'ftol-negative-zero', 'const bool step=tested>0x80000000u;', 'const bool step=tested>=0x80000000u;'),
    (SYSTEMS, 'ftol-sign', 'const auto tested=negative?bits^0x80000000u:bits;', 'const auto tested=negative?bits:bits;'),
    (SYSTEMS, 'ftol-direction', 'negative?integer+step:integer-step', 'negative?integer-step:integer+step'),
    (SYSTEMS, 'ftol-subtract', 'x87::subtract(value,rounded)', 'x87::subtract(rounded,value)'),
    (SYSTEMS, 'ftol-high', 'negative?Access::negative_high_load:Access::positive_high_load,slot.integer+4u',
     'negative?Access::negative_high_load:Access::positive_high_load,slot.integer'),
    (SYSTEMS, 'ftol-discard', 'memory.store32(Access::discard_value_store,slot.sign,std::uint32_t(last.bits));',
     'memory.store32(Access::discard_value_store,slot.difference,std::uint32_t(last.bits));'),
    (SYSTEMS, 'reset-order', '    memory.store32(Access::first_unallocated_store,array+data_array::first_unallocated,0u);\n    memory.store32(Access::actual_count_store,array+data_array::actual_count,0u);',
     '    memory.store32(Access::actual_count_store,array+data_array::actual_count,0u);\n    memory.store32(Access::first_unallocated_store,array+data_array::first_unallocated,0u);'),
    (SYSTEMS, 'reset-salt-count', 'inline constexpr std::uint32_t salt_characters=2u;', 'inline constexpr std::uint32_t salt_characters=3u;'),
    (SYSTEMS, 'strncpy-zero-test', 'return (((word+zero_test_bias)^~word)&zero_test_mask)!=0;', 'return (((word+zero_test_bias)^word)&zero_test_mask)!=0;'),
    (SYSTEMS, 'strncpy-test-mask', 'zero_test_mask=0x81010100u', 'zero_test_mask=0x01010100u'),
    (SYSTEMS, 'strncpy-reread', 'auto word=memory.load32(Access::word,source);', 'auto word=memory.load32(Access::word,source+1u);'),
    (SYSTEMS, 'strncpy-terminator-mask', 'word&=terminator?0xffffffffu>>(8*(4-terminator)):0u;', 'word&=terminator?0xffffffffu>>((8*(2-terminator))&31u):0u;'),
    (SYSTEMS, 'strncpy-fill-words', 'zero_words(words-1u);', 'zero_words(words);'),
    (SYSTEMS, 'strncpy-align-fill', '                while(destination&3u) {', '                while(destination&1u) {'),
    (SYSTEMS, 'strncpy-leading-exit', 'if(--remaining==0){result.exit=Exit::copied;residue.counted_down=true;return result;}\n            if(!character) {',
     'if(--remaining==0){result.exit=Exit::filled;residue.counted_down=true;return result;}\n            if(!character) {'),
    (SYSTEMS, 'strncpy-trailing-fill', 'zero_bytes(Access::byte_fill,tail-1u);', 'zero_bytes(Access::byte_fill,tail);'),
    (SYSTEMS, 'strncpy-tail', 'auto tail=remaining&3u;', 'auto tail=remaining&7u;'),
    (SYSTEMS, 'setup-align', 'const auto mask=(1u<<(alignment_bits&31u))-1u;', 'const auto mask=(1u<<(alignment_bits&15u))-1u;'),
    (SYSTEMS, 'setup-bits-shift', 'std::uint32_t(signed_value(maximum_count+0x1fu)>>5)<<2', 'std::uint32_t(signed_value(maximum_count+0x1fu)>>5)<<1'),
    (SYSTEMS, 'setup-flags', 'inline constexpr std::uint32_t setup_flags=0x3u;', 'inline constexpr std::uint32_t setup_flags=0x1u;'),
    (SYSTEMS, 'setup-name-end', 'array+data_array::name+data_array::name_length-1u,0u);', 'array+data_array::name+data_array::name_length,0u);'),
    (SYSTEMS, 'setup-signature', 'signature_value=0x64407440u', 'signature_value=0x64407441u'),
    (SYSTEMS, 'setup-order', '    memory.store32(Access::in_use_bits_store,array+data_array::in_use_bits,header.in_use_bits);\n    memory.store32(Access::allocation_store,array+data_array::allocation,header.allocation);',
     '    memory.store32(Access::allocation_store,array+data_array::allocation,header.allocation);\n    memory.store32(Access::in_use_bits_store,array+data_array::in_use_bits,header.in_use_bits);'),
    (SYSTEMS, 'reset-high-bit', 'salt_high_bit=0x8000u', 'salt_high_bit=0x4000u'),
    (SYSTEMS, 'reset-fill-flag', 'debug_fill_flag=0x8u', 'debug_fill_flag=0x4u'),
    (SYSTEMS, 'reset-fill-byte', 'debug_fill_byte=0xbau', 'debug_fill_byte=0xabu'),
    (SYSTEMS, 'reset-reread', '            memory.store16(Access::datum_salt_store,size*index+data,0u);\n            const auto count=memory.load32(Access::datum_recount,array+data_array::maximum_count);',
     '            const auto count=memory.load32(Access::datum_recount,array+data_array::maximum_count);\n            memory.store16(Access::datum_salt_store,size*index+data,0u);'),
    (SYSTEMS, 'reset-bits', 'signed_value(count+0x1fu)>>5', 'signed_value(count+0x20u)>>5'),
    (ADAPTER, 'adapter-vertex-xmm7', 'x[7]=_mm_move_ss(x[7],lane(point[0]));', 'x[7]=lane(point[0]);'),
    (ADAPTER, 'adapter-vertex-eax', 'gp[b::eax]=result.node_stored?result.node:result.residue.element;', 'gp[b::eax]=result.residue.element;'),
    (ADAPTER, 'adapter-vertex-flags', 'gp[b::esp]=m.flags.plus(gp[b::esp],0xcu,4);\n    pop(io,m,exit[3]);', 'gp[b::esp]+=0xcu;\n    pop(io,m,exit[3]);'),
    (ADAPTER, 'adapter-vertex-slot', 'constexpr std::uint32_t slot[3]{0xcu,0x10u,0x14u};', 'constexpr std::uint32_t slot[3]{0x10u,0xcu,0x14u};'),
    (ADAPTER, 'adapter-ftol-c1', 'StoresSetC1 && result.last_store_rounded_up?0x200u:0u', 'result.last_store_rounded_up?0x200u:0u'),
    (ADAPTER, 'adapter-ftol-top', 'stack.top=(stack.top+1u)&7u;', 'stack.top=(stack.top+7u)&7u;'),
    (ADAPTER, 'adapter-ftol-carry', 'm.flags.plus(result.high,result.step && low==0u,4)', 'm.flags.plus(result.high,result.step,4)'),
    (ADAPTER, 'adapter-ftol-align', '(gp[b::esp]-0x20u)&0xfffffff0u', '(gp[b::esp]-0x20u)&0xfffffff8u'),
    (ADAPTER, 'adapter-ftol-ecx', 'case Path::positive:gp[b::ecx]=result.biased_difference;', 'case Path::positive:'),
    (ADAPTER, 'adapter-reset-direction', '&b::reset_tail_site,\n        (m.other_flags&0x400u)!=0};', '&b::reset_tail_site,\n        false};'),
    (ADAPTER, 'adapter-reset-call-frame', 'm.gp[b::edi]=destination;push(io,m,0x16e61du,0x16e622u);', 'push(io,m,0x16e61du,0x16e622u);'),
    (ADAPTER, 'adapter-strncpy-edx', 'if(result.residue.last_word)gp[b::edx]=*result.residue.last_word;', ';'),
    (ADAPTER, 'adapter-strncpy-flags', 'if(result.residue.counted_down)m.flags.minus(1u,1u,4);else m.flags.logic(0u,4);', 'if(result.residue.counted_down)m.flags.minus(0u,1u,4);else m.flags.logic(0u,4);'),
    (ADAPTER, 'adapter-strncpy-fill-site', 'case byte_fill:return 0x32a573u;', 'case byte_fill:return 0x32a556u;'),
    (ADAPTER, 'adapter-header-ecx', 'gp[b::ecx]=(header.maximum_count&~0xffu)|header.alignment_bits;', 'gp[b::ecx]=header.maximum_count;'),
    (ADAPTER, 'adapter-header-edi', 'm.gp[b::edi]=destination+(descending?', 'm.gp[b::edi]=destination+0u*(descending?'),
    (ADAPTER, 'adapter-setup-push', 'push(io,m,0x16e473u,gp[b::ecx]);', 'push(io,m,0x16e473u,gp[b::ecx]&0xffu);'),
    (ADAPTER, 'adapter-setup-direction', '(m.other_flags&0x400u)!=0};\n    struct Frame {\n        Io &io;Machine<Flags> &m;std::uint32_t mask;', 'false};\n    struct Frame {\n        Io &io;Machine<Flags> &m;std::uint32_t mask;'),
    (ADAPTER, 'adapter-reset-edx', 'gp[b::edx]=result.bit_vector_bytes;', 'gp[b::edx]=0;'),
    (SYSTEMS, 'interval-start', 'if(!(signed_value(first)>signed_value(key))) {', 'if(!(signed_value(first)>=signed_value(key))) {'),
    (SYSTEMS, 'interval-end', 'if(signed_value(end)>signed_value(key)) {', 'if(signed_value(end)>=signed_value(key)) {'),
    (SYSTEMS, 'interval-offset', 'key-found_first);', 'found_first-key);'),
    (SYSTEMS, 'interval-stride', 'record {inline constexpr std::uint32_t size=0x48u,', 'record {inline constexpr std::uint32_t size=0x44u,'),
    (SYSTEMS, 'interval-recheck', 'if(result.index==result.count_recheck)', 'if(result.index==count)'),
    (SYSTEMS, 'interval-clear', 'memory.store32(Access::clear_store,cleared_offset,0u);', 'memory.store32(Access::clear_store,cleared_offset,1u);'),
    (SYSTEMS, 'interval-bound', '} while(signed_value(result.index)<signed_value(count));', '} while(signed_value(result.index)<=signed_value(count));'),
    (ADAPTER, 'adapter-interval-edx', 'else {gp[b::edx]=frame.index_pointer;gp[b::ecx]=result.index;}', 'else {gp[b::ecx]=result.index;}'),
    (ADAPTER, 'adapter-interval-ebp', 'void save_loop_register(){push(io,m,0x171025u,m.gp[b::ebp]);saved=true;}', 'void save_loop_register(){push(io,m,0x171025u,m.gp[b::ebx]);saved=true;}'),
    (ADAPTER, 'adapter-interval-flags', 'm.flags.minus(result.index,result.count_recheck,4);', 'm.flags.minus(result.count_recheck,result.index,4);'),
    (SYSTEMS, 'triangle-cross', 'n[2]=subtract(multiply(e1y,e2x),e2y_e1x);', 'n[2]=subtract(e2y_e1x,multiply(e1y,e2x));'),
    (SYSTEMS, 'triangle-plane', 'const bool near_plane=ordered_greater(r.plane_limit,r.dot_squared);', 'const bool near_plane=ordered_greater(r.plane_limit,0.0f);'),
    (SYSTEMS, 'triangle-sign', '(r.axis*2u+(positive?1u:0u))*6u', '(r.axis*2u+(positive?0u:1u))*6u'),
    (SYSTEMS, 'triangle-row', '(r.axis*2u+(positive?1u:0u))*6u', '(r.axis*2u+(positive?1u:0u))*4u'),
    (SYSTEMS, 'triangle-b2', 'if(!ordered_greater(r.b2,r.eps))', 'if(!ordered_greater(r.b2,0.0f))'),
    (SYSTEMS, 'triangle-reuse', 'store(Access::edge2_u_store,block+locals::normal,e2u);', 'store(Access::edge2_u_store,block+locals::normal+4u,e2u);'),
    (SYSTEMS, 'triangle-det', 'if(!ordered_greater(r.eps,r.det) && ', 'if(ordered_greater(r.eps,r.det) && '),
    (SYSTEMS, 'triangle-sum', 'ordered_not_below(r.det_limit,r.sum)', 'ordered_not_below(r.det,r.sum)'),
    (SYSTEMS, 'triangle-inverse', 'divide(load(Access::one,constant::one),r.det)', 'divide(r.det,load(Access::one,constant::one))'),
    (SYSTEMS, 'triangle-outputs', 'r.u_coordinate=multiply(inverse,r.b1);', 'r.u_coordinate=multiply(inverse,r.b2);'),
    (ADAPTER, 'adapter-triangle-xmm1', 'case Path::outside_edge2:  // XMM1 as the dominant-axis call left it\n            x[0]=',
     'case Path::outside_edge2:  // XMM1 as the dominant-axis call left it\n            x[1]=_mm_setzero_ps();x[0]='),
    (ADAPTER, 'adapter-triangle-ecx', 'gp[b::ecx]=std::uint32_t(r.v)*4u;', 'gp[b::ecx]=std::uint32_t(r.v);'),
    (ADAPTER, 'adapter-triangle-call', 'push(io,m,0x1212e3u,0x1212e8u);', 'push(io,m,0x1212e3u,0x1212e9u);'),
    (ADAPTER, 'adapter-triangle-x7', 'x[7]=lane(r.edge1_y_edge2_z);', 'x[7]=lane(r.eps);'),
    (SYSTEMS, 'begin-zero', 'memory.store32(Access::zero_store,selection+word*4u,0u);', 'memory.store32(Access::zero_store,selection+word*4u,1u);'),
    (SYSTEMS, 'begin-none', 'memory.store32(Access::none_store,selection+word*4u,0xffffffffu);', 'memory.store32(Access::none_store,selection+word*4u,0xfefefefeu);'),
    (SYSTEMS, 'begin-group-none', 'if(step.group!=0xffu && step.choice>=0 &&', 'if(step.choice>=0 &&'),
    (SYSTEMS, 'begin-choice-sign', 'step.choice>=0 &&\n', 'step.choice>=-1 &&\n'),
    (SYSTEMS, 'begin-choice-bound', 'step.choice<signed_value(memory.load32(Access::choice_count', 'step.choice<=signed_value(memory.load32(Access::choice_count'),
    (SYSTEMS, 'begin-subtable-none', 'if(subtable!=0xffu) {', 'if(subtable!=0xfeu) {'),
    (SYSTEMS, 'begin-group-sign', '*residue.iterator+std::uint32_t(widen8(step.group))+iterator::selected_subtable', '*residue.iterator+step.group+iterator::selected_subtable'),
    (SYSTEMS, 'begin-stride', 'remap_record {inline constexpr std::uint32_t size=0x10u,', 'remap_record {inline constexpr std::uint32_t size=0x14u,'),
    (SYSTEMS, 'begin-choice-stride', 'remap_choice {inline constexpr std::uint32_t size=8u,', 'remap_choice {inline constexpr std::uint32_t size=4u,'),
    (SYSTEMS, 'begin-reread', 'residue.record_limit=signed_value(frame.reread_limit());', 'frame.reread_limit();'),
    (SYSTEMS, 'begin-limit', 'if(residue.record_limit<=0)return result;', 'if(residue.record_limit<0)return result;'),
    (ADAPTER, 'adapter-begin-clamp', 'if(std::int32_t(count)<=std::int32_t(remap_table::maximum_records))io.st', 'if(std::int32_t(count)<std::int32_t(remap_table::maximum_records))io.st'),
    (ADAPTER, 'adapter-begin-iterator', 'std::uint32_t iterator(){return io.rd(0x1f7f3bu,slot-4u,4);}', 'std::uint32_t iterator(){return io.rd(0x1f7f3bu,slot,4);}'),
    (ADAPTER, 'adapter-begin-edx', 'case Selection::zeroed:gp[b::edx]=0;', 'case Selection::zeroed:'),
    (ADAPTER, 'adapter-begin-ecx', 'if(r.last->selected){gp[b::ecx]=std::uint32_t(native_live_support::widen8(r.last->group));', 'if(r.last->selected){gp[b::ecx]=r.last->group;'),
    (ADAPTER, 'adapter-begin-flags', 'm.flags.minus(r.records_examined,gp[b::eax],4);', 'm.flags.minus(gp[b::eax],r.records_examined,4);'),
    (SYSTEMS, 'busy-kind', 'hardware_kind=2u,', 'hardware_kind=3u,'),
    (SYSTEMS, 'busy-whole', 'whole_structure=0xffffu,', 'whole_structure=0xfffeu,'),
    (SYSTEMS, 'busy-skip', 'block_skip=8u;', 'block_skip=4u;'),
    (SYSTEMS, 'busy-stride', 'holder_address=(element<<5)+memory.load32', 'holder_address=(element<<4)+memory.load32'),
    (SYSTEMS, 'busy-positive', 'if(signed_value(frame.busy(resource))>0)return Exit::busy;', 'if(signed_value(frame.busy(resource))>=0)return Exit::busy;'),
    (SYSTEMS, 'busy-id', 'memory.load32(Access::id,entry+reference::id)!=0xffffffffu', 'memory.load32(Access::id,entry+reference::id)!=0u'),
    (SYSTEMS, 'busy-recount', '        count=memory.load32(Access::recount,owner_address+owner::count);\n', '        memory.load32(Access::recount,owner_address+owner::count);\n'),
    (ADAPTER, 'adapter-busy-ebp', 'gp[b::ebp]=block+block_skip;', 'gp[b::ebp]=block;'),
    (ADAPTER, 'adapter-busy-edx', 'case Access::element_offset:gp[edx]=value;break;', 'case Access::element_offset:break;'),
    (ADAPTER, 'adapter-busy-index', '++gp[ebx];gp[edi]+=', 'gp[edi]+='),
    (ADAPTER, 'adapter-busy-call-flags', 'gp[b::eax]=gp[b::ecx]=resource;m.flags.logic(resource,4);', 'gp[b::eax]=gp[b::ecx]=resource;'),
    (ADAPTER, 'adapter-busy-return', 'push(io,m,0x22a7e6u,0x22a7ebu);io.call', 'push(io,m,0x22a7e6u,0x22a7ecu);io.call'),
    (ADAPTER, 'adapter-busy-al', 'b::set_low_byte(gp[b::eax],1u);m.flags.logic(1u,1);', 'b::set_low_byte(gp[b::eax],2u);m.flags.logic(1u,1);'),
    (CHILDREN_HEADER, 'children-publish', '        publish_machine(s,m);\n        if(trace)trace->calls.push_back', '        if(trace)trace->calls.push_back'),
    (CHILDREN_HEADER, 'children-reload', '        m.gp=s.gp;m.flags=s.flags;\n', '        m.flags=s.flags;\n'),
    (CHILDREN_HEADER, 'children-xmm', '        for(unsigned i=0;i<8;++i)m.x[i]=s.xmm[i];\n        if constexpr(requires{s.fp.lanes;s.fp.count;s.fp.top;s.fp.status;})m.x87={s.fp.lanes,s.fp.count,s.fp.top,s.fp.status};\n    }',
     '        if constexpr(requires{s.fp.lanes;s.fp.count;s.fp.top;s.fp.status;})m.x87={s.fp.lanes,s.fp.count,s.fp.top,s.fp.status};\n    }'),
    (SYSTEMS, 'ray-scale-test', 'const bool origin_unscaled=quiet_ordered_equal(scale,', 'const bool origin_unscaled=ordered_greater(scale,'),
    (SYSTEMS, 'ray-origin-x', 'multiply(pz,load(Access::up_x,local+matrix::up))', 'multiply(py,load(Access::up_x,local+matrix::up))'),
    (SYSTEMS, 'ray-translation', 'y=add(y,load(Access::position_y,', 'y=subtract(y,load(Access::position_y,'),
    (SYSTEMS, 'ray-vector-scale', 'vy=multiply(vy,scale);vz=multiply(vz,scale);', 'vy=multiply(vy,scale);'),
    (SYSTEMS, 'ray-vector-z', 'dz=add(dz,multiply(load(Access::forward_z_again,local+matrix::forward+8u),vx));',
     'dz=add(dz,multiply(load(Access::forward_z_again,local+matrix::forward+8u),vy));'),
    (SYSTEMS, 'ray-floor', 'const auto floor=below_zero?0.0f:fraction;', 'const auto floor=fraction;'),
    (SYSTEMS, 'ray-limit', 'store(Access::limit_store,limit_slot,fraction);', 'store(Access::limit_store,limit_slot,bound);'),
    (SYSTEMS, 'ray-plane', 'memory.store32(Access::plane_store,block+query::plane,none);', 'memory.store32(Access::plane_store,block+query::plane,0u);'),
    (SYSTEMS, 'ray-record-count', 'output+result::record_count,0u);', 'output+result::record_count+4u,0u);'),
    (SYSTEMS, 'ray-hit-node', 'output+result::node,node);', 'output+result::node,position);'),
    (ADAPTER, 'adapter-ray-xmm6', 'x[6]=lane(r.vector_z_up_x);', 'x[6]=lane(r.local_origin_z);'),
    (ADAPTER, 'adapter-ray-test-flags', 'else m.flags.scalar_compare(x[0],x[1],false);', 'else m.flags.scalar_compare(x[1],x[0],false);'),
    (ADAPTER, 'adapter-ray-invert-flags', 'gp[b::ecx]=m.flags.plus(node*matrix::size,matrices,4);', 'gp[b::ecx]=m.flags.logic(node*matrix::size+matrices,4);'),
    (ADAPTER, 'adapter-ray-found', 'b::set_low_byte(gp[b::eax],io.rd(0x1f84dau,gp[b::esp]+0x13u,1));', 'b::set_low_byte(gp[b::eax],io.rd(0x1f84dau,gp[b::esp]+0x14u,1));'),
    (ADAPTER, 'adapter-ray-edi', 'gp[b::edi]=gp[b::ebp]+result::bsp_output;', 'gp[b::edi]=gp[b::ebp];'),
    (ADAPTER, 'adapter-ray-xmm0', 'case Access::initial_fraction:m.x[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(value)));break;', 'case Access::initial_fraction:break;'),
    (ADAPTER, 'adapter-ray-minimum', 'push(io,m,0x1f849du,0u);', 'push(io,m,0x1f849du,1u);'),
    # Access-order mutants: registers and stores unchanged; only the verify
    # replay (exact read and store order) rejects them.
    (ADAPTER, 'adapter-ray-argument-order', '            gp[b::ebp]=io.rd(0x1f8217u,gp[b::esp]+0xb4u,4);\n            push(io,m,0x1f821eu,gp[b::esi]);push(io,m,0x1f821fu,gp[b::edi]);',
     '            push(io,m,0x1f821eu,gp[b::esi]);push(io,m,0x1f821fu,gp[b::edi]);\n            gp[b::ebp]=io.rd(0x1f8217u,gp[b::esp]+0xbcu,4);'),
    (ADAPTER, 'adapter-channel-restore-order', 'case Access::exhausted_record:gp[b::edi]=pop(', 'case Access::exhausted_position_store:gp[b::edi]=pop('),
    (SYSTEMS, 'texel-level-test', 'if(ordered_greater(one,detail)) {', 'if(!ordered_greater(detail,one)) {'),
    (SYSTEMS, 'texel-level-order', 'multiply(subtract(one,detail),', 'multiply(subtract(detail,one),'),
    (SYSTEMS, 'texel-shift-mask', '>>(level&31u));', '>>(level&15u));'),
    (SYSTEMS, 'texel-minimum', 'bitmap_address+bitmap::width));w>1) {', 'bitmap_address+bitmap::width));w>=1) {'),
    (SYSTEMS, 'texel-round', 'return extent+((0u-extent)&3u);', 'return extent+((0u-extent)&7u);'),
    (SYSTEMS, 'texel-compressed-flag', 'inline constexpr std::uint32_t compressed=0x2u,', 'inline constexpr std::uint32_t compressed=0x4u,'),
    (SYSTEMS, 'texel-power-test', 'const bool power_of_two=(divisor&(divisor-1u))==0;', 'const bool power_of_two=(divisor&(divisor+1u))==0;'),
    (SYSTEMS, 'texel-mask', '(extent-1u)&memory.load32(role.masked', 'extent&memory.load32(role.masked'),
    (SYSTEMS, 'texel-modulo', 'std::int64_t(signed_value(first+divisor))%e', 'std::int64_t(signed_value(first))%e'),
    (SYSTEMS, 'texel-half-order', 'subtract(multiply(from_int(signed_value(divisor)),load_float(role.coordinate,c_address)),\n            bits_to_float(half))',
     'subtract(bits_to_float(half),\n            multiply(from_int(signed_value(divisor)),load_float(role.coordinate,c_address)))'),
    (SYSTEMS, 'texel-type', 'enum class Type : std::int32_t {texture_2d=0,texture_3d=1};', 'enum class Type : std::int32_t {texture_2d=0,texture_3d=2};'),
    # Access order only: the verify replay rejects it.
    (SYSTEMS, 'texel-height-reload', 'if(!x.power_of_two)height=memory.load32(A::height_reload,slots.height);',
     'if(x.power_of_two)height=memory.load32(A::height_reload,slots.height);'),
    (ADAPTER, 'adapter-texel-xmm1', 'm.x[1]=_mm_move_ss(m.x[1],_mm_castsi128_ps(_mm_cvtsi32_si128(int(r.v_spill))));',
     'm.x[1]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(r.v_spill)));'),
    (ADAPTER, 'adapter-texel-c1', 'if constexpr(StoresSetC1)m.x87.status=', 'if constexpr(true)m.x87.status='),
    (ADAPTER, 'adapter-texel-c1-clear', 'm.x87.status=std::uint16_t(m.x87.status&~0x200u);m.x87_stack_owned=true;', 'm.x87_stack_owned=true;'),
    (ADAPTER, 'adapter-texel-full-stack', 'if(m.x87.count==8)throw', 'if(m.x87.count==9)throw'),
    (ADAPTER, 'adapter-texel-edx', 'gp[b::edx]=r.y_wrap;', 'gp[b::edx]=r.y_divisor;'),
    (ADAPTER, 'adapter-texel-3d-ecx', 'if(type==std::int32_t(Type::texture_3d)){gp[b::ecx]=gp[b::edi];', 'if(type==std::int32_t(Type::texture_3d)){'),
    (ADAPTER, 'adapter-texel-height-slot', 'gp[b::ecx]=io.rd(0x138f36u,entry-0xcu,4);', 'gp[b::ecx]=io.rd(0x138f36u,entry-8u,4);'),
    (ADAPTER, 'adapter-texel-address-flags', '            m.flags.logic(0u,4);gp[b::ecx]=io.rd(0x138f2bu,bitmap_address+bitmap::flags,2);\n            m.flags.logic(0u,4);gp[b::edx]=',
     '            gp[b::ecx]=io.rd(0x138f2bu,bitmap_address+bitmap::flags,2);\n            gp[b::edx]='),
    (SYSTEMS, 'effect-retiring', 'inline constexpr std::uint32_t retiring=0x40u,', 'inline constexpr std::uint32_t retiring=0x80u,'),
    (SYSTEMS, 'effect-particles', 'checked.datum+effect::particle_systems)==none) {', 'checked.datum+effect::particle_systems)!=none) {'),
    (SYSTEMS, 'effect-null', '} else if(!checked.datum)reread=false;', '} else if(false)reread=false;'),
    (SYSTEMS, 'effect-reread', 'if(reread)array=memory.load32(Access::array_reread,globals::effects);', 'array=memory.load32(Access::array_reread,globals::effects);'),
    (SYSTEMS, 'effect-next-index', 'Found next{none,true,(handle&0xffffu)+1u,', 'Found next{none,true,(handle&0xffffu)+2u,'),
    (SYSTEMS, 'effect-salt', 'f.handle=(widen16(memory.load16(salt,offset+*f.data))<<16)|index;', 'f.handle=(widen16(memory.load16(salt,offset+*f.data))<<15)|index;'),
    (SYSTEMS, 'effect-size', 'namespace effect {inline constexpr std::uint32_t size=0x190u,', 'namespace effect {inline constexpr std::uint32_t size=0x18cu,'),
    (SYSTEMS, 'effect-order', 'if(!frame.attachments(found) && !frame.visibility(handle)) {', 'if(!frame.visibility(handle) && !frame.attachments(found)) {'),
    # Access order only: the verify replay rejects it.
    (SYSTEMS, 'effect-slot-order', '    const auto effects=memory.load32(Access::effects,globals::effects);\n    frame.keep_time_step(time_step);',
     '    frame.keep_time_step(time_step);\n    const auto effects=memory.load32(Access::effects,globals::effects);'),
    (ADAPTER, 'adapter-effect-start', 'gp[b::edx]=m.flags.logic(0u,4);gp[b::ecx]=effects;', 'gp[b::edx]=m.flags.logic(1u,4);gp[b::ecx]=effects;'),
    (ADAPTER, 'adapter-effect-slot', 'void keep_time_step(std::uint32_t step){io.st(0x179ce7u,entry-4u,4,step);}', 'void keep_time_step(std::uint32_t step){io.st(0x179ce7u,entry-8u,4,step);}'),
    (ADAPTER, 'adapter-effect-ecx', 'if(f.scan.index)gp[b::ecx]=std::uint32_t(std::int32_t(*f.scan.index)>>5);', ';'),
    (ADAPTER, 'adapter-effect-esi', 'gp[b::esi]=f.scan.index.value_or(none);', 'gp[b::esi]=none;'),
    (ADAPTER, 'adapter-effect-cl', '|(c.flags>>6);', '|(c.flags>>5);'),
    (ADAPTER, 'adapter-effect-al', 'gp[b::eax]=c.datum&~0xffu;', 'gp[b::eax]=c.datum;'),
    (ADAPTER, 'adapter-effect-restore', 'if(walked){gp[b::edi]=pop(io,m,0x179de2u);gp[b::ebp]=pop(io,m,0x179de3u);}',
     'if(walked){gp[b::ebp]=pop(io,m,0x179de2u);gp[b::edi]=pop(io,m,0x179de3u);}'),
    (ADAPTER, 'adapter-effect-xmm0', '            m.x[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(value)));\n            push(io,m,0x179cdbu', '            push(io,m,0x179cdbu'),
    (ADAPTER, 'adapter-effect-eax', 'case Access::game_time:gp[b::eax]=value;break;', 'case Access::game_time:break;'),
    (ADAPTER, 'adapter-store-sites', 'physics_step_detail::w(this->s.memory,this->s.observe,site_of(p),a,n,x);',
     'physics_step_detail::w(this->s.memory,this->s.observe,p,a,n,x);'),
    (MATRIX, 'framework-x87-publish', 's.fp.lanes=m.x87.lanes;s.fp.count=m.x87.count;', 's.fp.lanes=m.x87.lanes;'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def branches(body):
    lines = body.split('\n')
    parsed = []
    for line in lines:
        match = LINE.match(line)
        if match:
            parsed.append((int(match[1], 16), match[2]))
    result = []
    for index, (pc, statement) in enumerate(parsed):
        branch = BRANCH.match(statement)
        if branch and index + 1 < len(parsed):
            result.append((pc, int(branch[2], 16), parsed[index + 1][0]))
    return result


def unit_helper(text, signature):
    found = re.search(r'^' + re.escape(signature) + r' \{.*?\n\}\n', text, re.S | re.M)
    if not found:
        raise ValueError(f'Unit helper missing: {signature}')
    return found[0]


# 243F20's jump table resolves through the unit's canonical_branch; the
# harness authors the original table, so the four targets resolve to
# themselves and any other entry refuses as the unit does.
CANONICAL_BRANCH = '''std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage){
    switch(p){case 0x243f41u:case 0x243f62u:case 0x24403au:case 0x244070u:return p;
    default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");}
}
'''


# The genuine children of the child-calling leaves are deterministic
# stand-ins, shared by the translated bodies (through dispatch or their local
# call) and the natives' routes. Each reads memory (so it can fault), changes
# scratch registers and flags, and leaves a digest of its entry registers in
# XMM2, so a wrong register at the call reaches the comparison.
CHILD_STANDINS = '''
static void child_standin(WorldState &s,std::uint32_t target,std::uint32_t site) {
    const auto word=r(s.memory,s.gp[1],4);
    s.xmm[2]=_mm_castsi128_ps(_mm_set_epi32(int(s.gp[0]),int(s.gp[1]^target),int(s.gp[2]^s.gp[3]),
        int(s.flags.bits()^s.gp[5]^s.gp[6]^s.gp[7])));
    // Some calls also change the caller's owner count (ESI + 10), which the
    // caller re-reads.
    if(word&0x10u)w(s.memory,s.observe,target,s.gp[6]+0x10u,4,(word>>8)&7u);
    s.gp[0]=word;s.gp[1]=word*3u+site;s.gp[2]=~word;
    s.flags.minus(word,7u,4);
    s.pop();
}
// The stand-ins below first fold a digest of their entry registers (GP,
// flags, every XMM lane, the x87 status and MXCSR) into the word at
// 11F00000, so a wrong register at any child call reaches the bank.
static void fold_entry(WorldState &s,std::uint32_t target) {
    std::uint32_t digest=target^s.flags.bits();
    for(const auto v:s.gp)digest=digest*0x9e3779b1u+v;
    for(const auto &x:s.xmm){std::uint32_t lanes[4];std::memcpy(lanes,&x,16);for(const auto v:lanes)digest=digest*0x9e3779b1u+v;}
    digest=(digest*0x9e3779b1u+s.fp.status+s.fp.count*0x10000u)*31u+_mm_getcsr();
    w(s.memory,s.observe,target,0x11f00000u,4,(r(s.memory,0x11f00000u,4)*0x01000193u)^digest);
}
// 144090 (EAX = destination, ECX = source real_matrix4x3): copies the
// source's thirteen floats, flipping the sign of some; scratch registers,
// flags and every XMM register change.
static void invert_standin(WorldState &s,std::uint32_t target) {
    fold_entry(s,target);
    const auto source=s.gp[1],destination=s.gp[0];
    const auto scale=r(s.memory,source,4);
    for(unsigned i=0;i<13;++i) {
        auto value=r(s.memory,source+i*4u,4);
        if(i && (value&0x100u))value^=0x80000000u;
        w(s.memory,s.observe,target,destination+i*4u,4,value);
    }
    s.gp[0]=scale;s.gp[1]=~scale;s.gp[2]=scale*5u+1u;s.flags.logic(scale,4);
    for(unsigned i=0;i<8;++i)s.xmm[i]=_mm_castsi128_ps(_mm_set1_epi32(int(scale+i)));
    s.pop();
}
// 1E6980 (stdcall: query, node, minimum, limit; RET 10): reads the query's
// flags, BSP and ray, decides a hit from them and its arguments, and on a
// hit lowers the output fraction ([query+18]) and fills an output word.
// AL = hit; XMM2, the flags and x87 C1 change.
static void run_1e6980(WorldState &s) {
    fold_entry(s,0x1e6980u);
    const auto esp=s.gp[4];
    const auto query=r(s.memory,esp+4u,4),node=r(s.memory,esp+8u,4),minimum=r(s.memory,esp+12u,4),limit=r(s.memory,esp+16u,4);
    const auto flags=r(s.memory,query,4),bsp=r(s.memory,query+4u,4);
    const auto origin=r(s.memory,query+16u,4),vector=r(s.memory,query+20u,4),output=r(s.memory,query+24u,4);
    const auto ox=r(s.memory,origin,4),vz=r(s.memory,vector+8u,4);
    const bool hit=((bsp^limit^ox^vz^flags^node^minimum)&3u)==0;
    if(hit) {
        const auto fraction=r(s.memory,output,4);
        w(s.memory,s.observe,0x1e6980u,output,4,(fraction&0x80000000u)|((fraction&0x7fffffffu)>>1));
        w(s.memory,s.observe,0x1e6980u,output+12u,4,bsp);
    }
    s.gp[0]=(bsp&0xffffff00u)|(hit?1u+(ox&0xfeu):0u);s.gp[1]=bsp*7u;s.gp[2]=ox^vz;
    s.flags.minus(bsp,limit,4);
    s.xmm[2]=_mm_castsi128_ps(_mm_set_epi32(int(ox),int(vz),int(limit),int(bsp)));
    s.fp.status^=0x200u;
    s.pop();s.gp[4]+=16u;
}
// 138510 (EDI = bitmap, EAX = level; two arguments, RET 8), 1386E0 (EDI =
// bitmap; four arguments, RET 10) and 1385D0 (ECX = bitmap; four arguments,
// RET 10): a level offset from the bitmap's words, EAX and the arguments.
// XMM3 and x87 C1 change, so the address call and the exit see them.
static void level_offset_standin(WorldState &s,std::uint32_t target,std::uint32_t bitmap,unsigned arguments) {
    fold_entry(s,target);
    std::uint32_t offset=(r(s.memory,bitmap+4u,2)*0x10001u)^r(s.memory,bitmap+0xcu,2)^s.gp[0];
    for(unsigned i=0;i<arguments;++i)offset=offset*33u+r(s.memory,s.gp[4]+4u+i*4u,4);
    s.gp[0]=offset;s.gp[1]=offset>>3;s.gp[2]=~offset;s.flags.plus(offset,target,4);
    s.xmm[3]=_mm_castsi128_ps(_mm_set1_epi32(int(offset)));s.fp.status^=0x200u;
    s.pop();s.gp[4]+=arguments*4u;
}
static void run_138510(WorldState &s){level_offset_standin(s,0x138510u,s.gp[7],2);}
static void run_1386e0(WorldState &s){level_offset_standin(s,0x1386e0u,s.gp[7],4);}
static void run_1385d0(WorldState &s){level_offset_standin(s,0x1385d0u,s.gp[1],4);}
// 138C20 (seven arguments; ECX = y, EDX = width; RET 1C): an address from
// its arguments and registers.
static void run_138c20(WorldState &s) {
    fold_entry(s,0x138c20u);
    std::uint32_t address=s.gp[1]*s.gp[2];
    for(unsigned i=0;i<7;++i)address=address*31u+r(s.memory,s.gp[4]+4u+i*4u,4);
    s.gp[0]=address;s.gp[1]=address^0x5a5a5a5au;s.gp[2]=address+s.gp[3];s.flags.minus(address,s.gp[6],4);
    s.pop();s.gp[4]+=0x1cu;
}
// 179CD0's children. Each folds its entry registers, leaves EBX/ESI/EDI/EBP,
// changes EAX/ECX/EDX, the flags, XMM5 and x87 C1, and sometimes moves the
// effects global (4F720C) to the second fixture array, so the walk's
// re-reads are observed.
static std::uint32_t effect_datum(WorldState &s,std::uint32_t handle) {
    return r(s.memory,r(s.memory,0x4f720cu,4)+0x44u,4)+(handle&0xffffu)*0x190u;
}
static void effect_scratch(WorldState &s,std::uint32_t target,std::uint32_t value) {
    s.gp[0]=value;s.gp[1]=value*13u+target;s.gp[2]=value^target;s.flags.plus(value,target,4);
    s.xmm[5]=_mm_castsi128_ps(_mm_set1_epi32(int(value^target)));s.fp.status^=0x200u;
    if((value&0x70u)==0x70u)w(s.memory,s.observe,target,0x4f720cu,4,r(s.memory,0x11f00004u,4));
}
// 16EA60 (ECX = array, EDX = start): the first in-use index at or after the
// start, below the array's first unallocated index, or -1.
static void slot_scan_standin(WorldState &s,std::uint32_t target) {
    fold_entry(s,target);
    const auto array=s.gp[1],limit=r(s.memory,array+0x38u,4),bits=r(s.memory,array+0x48u,4);
    std::uint32_t found=0xffffffffu;
    for(auto i=s.gp[2];std::int32_t(i)<std::int32_t(limit);++i)
        if(r(s.memory,bits+(i>>5)*4u,4)&(1u<<(i&31u))){found=i;break;}
    effect_scratch(s,target,bits);s.gp[0]=found;
    s.pop();
}
static void run_16ea60(WorldState &s){slot_scan_standin(s,0x16ea60u);}
// 17C370 ([ESP+4] = handle; RET 4) and 17C5C0 (EBX = handle): true when a
// datum word's bit says the effect is gone; 17C370 sometimes marks the
// effect retiring or empties its particle systems.
static void run_17c370(WorldState &s) {
    fold_entry(s,0x17c370u);
    const auto datum=effect_datum(s,r(s.memory,s.gp[4]+4u,4)),word=r(s.memory,datum+0x10u,4);
    if(word&0x100u)w(s.memory,s.observe,0x17c370u,datum+2u,1,r(s.memory,datum+2u,1)^0x40u);
    if(word&0x200u)w(s.memory,s.observe,0x17c370u,datum+0xfcu,4,0xffffffffu);
    effect_scratch(s,0x17c370u,word);s.gp[0]=(word&0xffffff00u)|((word&3u)==3u?1u+(word>>24):0u);
    s.pop();s.gp[4]+=4u;
}
static void run_17c5c0(WorldState &s) {
    fold_entry(s,0x17c5c0u);
    const auto word=r(s.memory,effect_datum(s,s.gp[3])+0x14u,4);
    effect_scratch(s,0x17c5c0u,word);s.gp[0]=(word&0xffffff00u)|((word&6u)==6u?1u:0u);
    s.pop();
}
// 17A380 (EAX = handle): clears the effect's in-use bit (a delete).
static void delete_standin(WorldState &s,std::uint32_t target) {
    fold_entry(s,target);
    const auto handle=s.gp[0],array=r(s.memory,0x4f720cu,4),bits=r(s.memory,array+0x48u,4),index=handle&0xffffu;
    const auto word=r(s.memory,bits+(index>>5)*4u,4);
    w(s.memory,s.observe,target,bits+(index>>5)*4u,4,word&~(1u<<(index&31u)));
    effect_scratch(s,target,word);
    s.pop();
}
// 17C140 ([ESP+4] = handle, [ESP+8] = time step; RET 8): folds the step into
// the datum.
static void run_17c140(WorldState &s) {
    fold_entry(s,0x17c140u);
    const auto handle=r(s.memory,s.gp[4]+4u,4),step=r(s.memory,s.gp[4]+8u,4),datum=effect_datum(s,handle);
    const auto word=r(s.memory,datum+0x18u,4)+step;
    w(s.memory,s.observe,0x17c140u,datum+0x18u,4,word);
    effect_scratch(s,0x17c140u,word^handle);
    s.pop();s.gp[4]+=8u;
}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    switch(target) {
    case 0x16ea60u:slot_scan_standin(s,target);return;
    case 0x17a380u:delete_standin(s,target);return;
    case 0x4057f0u:child_standin(s,target,site);return;
    case 0x144090u:invert_standin(s,target);return;
    case 0x1386e0u:run_1386e0(s);return;
    case 0x1385d0u:run_1385d0(s);return;
    default:throw std::runtime_error("Unbound harness dispatch target");
    }
}
'''


def reference_include(text, tracked_text, bodies):
    # The units' own helpers (sign extension, the x87 spill forms), then the
    # pinned bodies and the children they call.
    helpers = [unit_helper(text, 'std::uint32_t sign_extend(std::uint32_t x,unsigned n)'),
               unit_helper(text, 'std::uint64_t native_read64(const M &memory,std::uint32_t address)'),
               unit_helper(text, 'std::uint64_t native_integer_spill64(long double x)'),
               unit_helper(tracked_text, 'std::uint64_t native_integer_spill64(long double x,std::uint16_t &status)'),
               unit_helper(tracked_text, 'std::uint32_t native_single_spill(long double x,std::uint16_t &status)'),
               CANONICAL_BRANCH]
    declarations = ''.join(f'static void run_{name}(WorldState &);\n' for name in HARNESS + CHILDREN)
    return ''.join(helpers) + declarations + CHILD_STANDINS + '\n'.join(bodies) + '\n'


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


def check_binding(helper, specializer, text):
    output, receipt = specializer.specialize(text)
    installed = set(receipt.get('native_spatial_owners', []))
    if not set(NAMES) <= installed:
        raise ValueError(f'Live-support leaves did not install: {sorted(set(NAMES) - installed)}')
    for name in NAMES:
        if output.count(helper.marker(name)) != 1:
            raise ValueError(f'{name} leaf binding missing or duplicated')
    # The binding is idempotent on the complete specialized unit (the whole
    # specializer's other owners are checked by their own validators).
    if helper.install(output, output) != (output, []):
        raise ValueError('Live-support leaf binding is not idempotent')
    for name in NAMES:
        found = helper.body(text, name)
        # Change the body's first operand literal (inside an address).
        literal = re.search(r'\(0x([0-9a-f]+)u\+', found[0])
        changed_body = found[0][:literal.start(1)] + format(int(literal[1], 16) + 4, 'x') + found[0][literal.end(1):]
        changed = text.replace(found[0], changed_body, 1)
        if changed == text or name in helper.install(changed, changed)[1]:
            raise ValueError(f'{name} binding accepted a changed body')
    copies = {}
    for path in COPIES:
        unit = path.read_text()
        accepted = helper.install(unit, unit)[1]
        copies[str(path.relative_to(ROOT))] = accepted
        for name in accepted:
            if '@' in name:
                continue  # canonical copies: checked below
            found = helper.body(unit, name)
            operand = re.search(r'selected\((0x[0-9a-f]+)u,s\.image\)', found[0])
            if not operand:
                continue
            case = f'case {operand[1]}u:return {operand[1]}u;'
            remapped = unit.replace(case, f'case {operand[1]}u:return {hex(int(operand[1], 16) + 4)}u;', 1)
            if remapped == unit or name in helper.install(remapped, remapped)[1]:
                raise ValueError(f'{name} binding accepted a nonidentity selected operand in {path.name}')
    # The canonical copy binds, idempotently, and refuses a changed body or
    # a remapped return-address operand.
    world = WORLD.read_text()
    output, installed = helper.install(world, world)
    if installed != ['16e600@16d220', '32a4e0@326970'] or helper.install(output, output)[1]:
        raise ValueError(f'Canonical 16E600/32A4E0 copy binding: {installed}')
    copies[str(WORLD.relative_to(ROOT))] = installed
    # The pool startup unit's copies bind in the store-site form, idempotently;
    # a changed store PC or a remapped instruction refuses.
    pool = POOL.read_text()
    output, installed = helper.install(pool, pool)
    if installed != ['16e600@16d220', '16e450@16d070', '16e4b0@16d0d0', '32a4e0@326970'] or helper.install(output, output)[1]:
        raise ValueError(f'Pool startup store-site binding: {installed}')
    for old, new in (('w(s.memory,s.observe,0x16d0afu,', 'w(s.memory,s.observe,0x16d0b0u,'),
                     ('case 0x16d0afu:return 0x16e48fu;', 'case 0x16d0afu:return 0x16e493u;')):
        changed = pool.replace(old, new, 1)
        if changed == pool or '16e450@16d070' in helper.install(changed, changed)[1]:
            raise ValueError(f'Pool startup 16E450 binding accepted {new}')
    found = helper.body(world, '16d220')
    changed = world.replace(found[0], found[0].replace('(0x38u+s.gp[6])', '(0x3cu+s.gp[6])', 1), 1)
    if changed == world or '16e600@16d220' in helper.install(changed, changed)[1]:
        raise ValueError('Canonical 16E600 binding accepted a changed body')
    remapped = world.replace('case 0x16d242u:return 0x16e622u;', 'case 0x16d242u:return 0x16e626u;', 1)
    if remapped == world or '16e600@16d220' in helper.install(remapped, remapped)[1]:
        raise ValueError('Canonical 16E600 binding accepted a remapped operand')
    return copies


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=7200 * len(HARNESS))
    parser.add_argument('--mutant-cases', type=int, default=24000)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-binding', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true', help='iteration only: no mutation screen (not a proof)')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sources = [UNIT, *COPIES, UNTRACKED, POOL, WORLD, SYSTEMS, ADAPTER, MATRIX, CHILDREN_HEADER, TEST, HELPER, SPECIALIZER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    helper = load(HELPER, 'native_live_support_leaves_binding')
    text = UNIT.read_text()
    bodies = []
    cfg = {}
    for name in NAMES:
        found = helper.body(text, name)
        if not found or helper.digest(found) != helper.LEAVES[name][0][0]:
            raise ValueError(f'Canonical {name} body drifted from its pin')
        cfg[name] = branches(found[0])
        bodies.append(cursor_form(found[0]))
        if name in helper.CHILD_CALLS:
            # The verification reference: child calls routed to the replay.
            bodies.append(cursor_form(helper.reference(found, name)))
    for child in CHILDREN:
        found = helper.body(text, child)
        pins = [pin for children in helper.CHILDREN.values() for pin in children.get(child, ())]
        if not found or helper.digest(found) != pins[0]:
            raise ValueError(f'Canonical child {child} drifted from its pin')
        cfg[child] = branches(found[0])
        bodies.append(cursor_form(found[0]))
    # The tracked 32CC0C form: identity selected() operands, so the literal
    # body is the unit's body.
    tracked_text = TRACKED.read_text()
    found = helper.body(tracked_text, '32cc0c')
    if not found or helper.digest(found) not in helper.LEAVES['32cc0c'][0][1:] or not helper.identity_operands(tracked_text, found):
        raise ValueError('Tracked 32CC0C body drifted from its pin')
    literal = re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)', r'\1', found[0]).replace('run_32cc0c(', 'run_32cc0c_tracked(', 1)
    bodies.append(cursor_form(literal))
    # The map world support unit's 138D90 (identity selected() operands; its
    # FISTP spills leave C1 as the FLD cleared it; local children), with its
    # own verification reference.
    untracked_text = UNTRACKED.read_text()
    found = helper.body(untracked_text, '138d90')
    if (not found or helper.digest(found) != helper.LEAVES['138d90'][0][3] or
            not helper.identity_operands(untracked_text, found)):
        raise ValueError('Untracked 138D90 body drifted from its pin')
    literal = re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)', r'\1', found[0])
    bodies.append(cursor_form(literal.replace('run_138d90(', 'run_138d90_untracked(', 1)))
    bodies.append(cursor_form(helper.reference((literal, found[1]), '138d90').replace(
        helper.REFERENCE + '138d90(', helper.REFERENCE + '138d90_untracked(', 1)))
    # The pool startup copies: the TU1.5 cursor form with the unit's
    # canonical store PCs kept, and the binder's site map of each.
    pool_text = POOL.read_text()
    for local, leaf in POOL_COPIES.items():
        if helper.canonical_form(pool_text, local) != 'stores':
            raise ValueError(f'Pool startup {local} left the store-site form')
        children = helper.CANONICAL[local][1]
        form = helper.tu15_form(pool_text, local, leaf, children).replace('STATE &s', 'WorldState &s')
        form = form.replace(f'run_{leaf}(', f'run_{leaf}_pool(', 1)
        for child_leaf in children.values():
            form = form.replace(f'run_{child_leaf}(s)', f'run_{child_leaf}_pool(s)')
        bodies.append(cursor_form(form))
        sites = helper.site_map(helper.store_sites(pool_text, local))
        bodies.append(f'static std::uint32_t pool_sites_{leaf}(std::uint32_t p){{return ({sites})(p);}}\n')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins={n: helper.LEAVES[n][0] for n in NAMES})
    if not args.skip_binding:
        report['copies'] = check_binding(helper, load(SPECIALIZER, 'native_operands'), text)
    (scratch / 'live_support_leaves_body.inc').write_text(reference_include(text, tracked_text, bodies))
    release = compile_harness(scratch, 'leaves-release', [])
    result = run(release, args.cases)
    verify = run(release, max(args.cases // 6, 1200), 'verify')
    census = run(release, max(args.cases // 6, 1200), 'census')
    report.update(release=dict(result, edges=None), verify=dict(verify, edges=None), census=dict(census, edges=None))
    edges = {int(pc, 16): {int(n, 16) for n in nexts} for pc, nexts in result['edges'].items()}
    missing = []
    coverage = {}
    for name, items in cfg.items():
        taken = 0
        for pc, target, fallthrough in items:
            for successor in (target, fallthrough):
                if successor in edges.get(pc, set()):
                    taken += 1
                elif (pc, successor) not in UNREACHABLE:
                    missing.append(f'{name}:{pc:x}->{successor:x}')
        declared = sum(1 for pc, _, _ in items for key in UNREACHABLE if key[0] == pc)
        coverage[name] = f'{taken}/{2 * len(items)} directions ({declared} unreachable)'
    report['coverage'] = coverage
    if missing:
        raise ValueError(f'Unexercised branch directions: {missing}')
    for name in HARNESS:
        leaf = result['leaves'][name]
        if not leaf['compared'] or not leaf['faults']:
            raise ValueError(f'Incomplete {name} coverage: {leaf}')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'leaves-sanitized', [], sanitizers=True)
        out = run(sanitized, max(args.cases // 4, 3000))
        verify_out = run(sanitized, 1200, 'verify')
        report['sanitized'] = dict(out, edges=None)
        report['sanitized_verify'] = dict(verify_out, edges=None)
        sanitized.unlink()
    release.unlink()
    texts = {SYSTEMS: SYSTEMS.read_text(), ADAPTER: ADAPTER.read_text(), MATRIX: MATRIX.read_text(),
             CHILDREN_HEADER: CHILDREN_HEADER.read_text()}

    def screen(mutant):
        header, name, old, new = mutant
        text = texts[header]
        if text.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'leaves-mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / header.name).write_text(text.replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'leaves-mutant-{name}', [scratch / 'leaves-mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        outcome = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        if outcome.returncode == 0:
            # Leaf verify mode replays the exact access order of the leaves
            # with children (reads included).
            outcome = subprocess.run([str(binary), str(max(args.mutant_cases // 6, 1200)), 'verify'], capture_output=True, text=True)
        binary.unlink()
        return dict(name=name, rejected=outcome.returncode != 0, diagnostic=outcome.stderr.strip()[-200:])
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        mutants = [] if args.skip_mutants else list(pool.map(screen, MUTANTS))
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
    compared = sum(leaf['compared'] for leaf in result['leaves'].values())
    stores = sum(leaf['ordered_stores'] for leaf in result['leaves'].values())
    faults = sum(leaf['faults'] for leaf in result['leaves'].values())
    print(f'PASS {compared} native leaf comparisons ({stores} ordered stores, {faults} faults), '
          f'verify {sum(l["compared"] for l in verify["leaves"].values())}, census {sum(l["compared"] for l in census["leaves"].values())}, '
          f'coverage {coverage}, {len(mutants)} mutants rejected')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
