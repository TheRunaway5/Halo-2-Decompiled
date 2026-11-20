"""Bind the native TU1.5 frame/gameplay owners of group F
(docs/native_replacement/FRAME_MISC_OWNERS.md).

Each owner is installed only into the build-local specialized copy of its
own unit, and only when the complete canonical body matches its pin (state
type normalized), every direct child body the closure enters is present and
every dispatched child keeps its dispatch route. The verification reference
is the unmodified canonical body with only its child calls routed to the
replay (native_object_tick_detail::replay). A changed body keeps its
translated route.
"""
import hashlib
import re

DETAIL = 'halo2::native_object_tick_detail'
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)
DIRECT = re.compile(r'\brun_([0-9a-f]+)\(s\)')
DISPATCH = re.compile(r'\bdispatch\(s,(target|0x[0-9a-f]+u),0x[0-9a-f]+u,(?:false|true)\)')
# name -> owner record. 'unit' is a line only the owning unit carries;
# 'direct' children must have bodies in the unit, 'dispatched' must not.
OWNERS = {
    '1306b0': dict(
        pin='09a0ee065558df4dbc6c7faea11296532b0b495142bc2544fff9e0cd884323ce',
        unit='#include "halo2/source_geometry_cache_frame_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_geometry_cache_count.hpp"\n',
        bridge='halo2::native_geometry_cache_count_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    'd7230': dict(
        pin='54490f7355105549c798f207a9c169806c9c020f585b65155bfaefa3f0f635b0',
        unit='#include "halo2/source_model_render_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_attachment_update.hpp"\n',
        bridge=('halo2::native_attachment_update_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '1eee80': dict(
        pin='7d42f55d4aee93243d593357dbd7c9634f6d5db016c73ce668f37020f655cc52',
        unit='#include "halo2/source_multiplayer_engines_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_engine_player_update.hpp"\n',
        bridge=('halo2::native_engine_player_update_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}},\n'
                '        [](auto &v){{run_1efd60(v);}},[](auto &v){{run_1f0670(v);}},[](auto &v){{run_1f1030(v);}})'),
        direct=('1efd60', '1f0670', '1f1030'), dispatched=()),
    '41970': dict(
        pin='f1195d21e27893a26a86fe8b2312296de78385feffe5d2ca467ccef727bf6ef8',
        unit='#include "halo2/source_model_render_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_attachment_schedule.hpp"\n',
        bridge=('halo2::native_attachment_schedule_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}})'),
        direct=(), dispatched=()),
    '133a70': dict(
        pin='666f7adec7a376e969303b6258e8f063d91ea9a979bb58d32161d82f0c375b88',
        unit='#include "halo2/source_campaign_environment_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_environment_palette.hpp"\n',
        bridge='halo2::native_environment_palette_detail::bridge(s,[](auto &v){{{reference}(v);}})',
        direct=(), dispatched=()),
    '153ba0': dict(
        pin='42aefdb2ede564565e3d5e7fd3b54a227a98530f6d93f00efea62f6ef2c41a0c',
        unit='#include "halo2/source_combat_action_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_player_action.hpp"\n',
        bridge=('halo2::native_player_action_detail::bridge(s,[](auto &v){{{reference}(v);}},\n'
                '        [](std::uint32_t p,SourcePhysicsContactImage image){{return canonical_branch(p,image);}},\n'
                '        [](auto &v){{run_bd080(v);}},[](auto &v){{run_1540d0(v);}},[](auto &v){{run_153f20(v);}},\n'
                '        [](auto &v){{run_154270(v);}},[](auto &v){{run_1541b0(v);}})'),
        direct=('bd080', '1540d0', '153f20', '154270', '1541b0'), dispatched=()),
    '131610': dict(
        pin='70daa4bb9b4d19f1985efeef65b2f22d32a213b9cd7eaf4388e9374571b0449c',
        unit='#include "halo2/source_campaign_environment_tu15.hpp"',
        include='#include "physics/runtime/native_tu15_fog_mixers.hpp"\n',
        bridge='halo2::native_fog_mixers_detail::bridge(s,[](auto &v){{{reference}(v);}},[](auto &v){{run_137ac0(v);}})',
        direct=('137ac0',), dispatched=()),
}


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def reference_name(name):
    return f'native_frame_misc_reference_{name}'


def markers(name):
    return (f'// BEGIN native frame misc reference {name}\n', f'// END native frame misc reference {name}\n',
            f'\n    // BEGIN native frame misc {name}\n', f'\n    // END native frame misc {name}')


def prefix(name):
    owner = OWNERS[name]
    begin, end = markers(name)[2:]
    return begin + f'    if({owner["bridge"].format(reference=reference_name(name))})return;' + end


def reference(name, found):
    text = found[0].replace(f'static void run_{name}({found[1]} &s) {{', f'void {reference_name(name)}(WorldState &s) {{', 1)
    text = DIRECT.sub(lambda m: f'{DETAIL}::replay(s,0x{m[1]}u)', text)
    text = DISPATCH.sub(lambda m: f'{DETAIL}::replay(s,{m[1]})', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('void '):
        raise ValueError(f'Frame misc reference {name} kept an unrouted child')
    return text + '\n'


def admitted(canonical, name):
    owner = OWNERS[name]
    found = body(canonical, name)
    if not found or owner['unit'] not in canonical or digest(found) != owner['pin']:
        return False
    if any(not body(canonical, child) for child in owner['direct']):
        return False
    return not any(body(canonical, child) for child in owner['dispatched'])


# Owners installed by default. Each closure is now written to the code standard
# of docs/native_replacement/REMAINING_TRANSLATED_20261115.md (game logic in
# its *_systems.hpp, a thin boundary adapter in the bridge's header) and
# re-proven by tools/validate_native_frame_misc_tu15.py (2026-11-16). Bound
# after the live checks on the restored MCC cache (2026-11-16 evening): Armory
# parity in every role, and the MP and campaign verify sweep with 0
# differences; see docs/native_replacement/REMAINING_TRANSLATED_20261115.md.
BOUND = ('1306b0', 'd7230', '1eee80', '41970', '133a70', '153ba0', '131610')


def install(canonical, output, names=None):
    """Return (output, installed entries)."""
    installed = []
    for name in (BOUND if names is None else names):
        if not admitted(canonical, name):
            continue
        current = body(output, name)
        begin, end, prefix_begin, _ = markers(name)
        if not current or prefix_begin in current[0]:
            continue
        start = current[0].index('{') + 1
        output = output[:current.start()] + current[0][:start] + prefix(name) + current[0][start:] + output[current.end():]
        first = FIRST_BODY.search(output)
        if not first or begin in output:
            raise ValueError(f'Frame misc reference {name} needs one unbound unit')
        block = begin + reference(name, body(canonical, name)) + end
        output = output[:first.start()] + block + output[first.start():]
        include = OWNERS[name]['include']
        if include not in output:
            output = include + output
        installed.append(name)
    return output, installed
