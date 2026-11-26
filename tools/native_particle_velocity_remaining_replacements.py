"""Bind the remaining 252480 motion models and the original table lookup."""

import re
from typing import NamedTuple

import native_particle_velocity_angles_replacements as angles
import native_particle_velocity_direction_replacements as direction
import native_particle_velocity_properties_replacements as properties


OWNER = properties.OWNER
INCLUDE = '#include "rendering/effects/native_tu15_particle_velocity_special.hpp"\n'
NAMESPACE = 'halo2::native_particle_velocity_'
DISPATCH = '[](auto &value,std::uint32_t target,std::uint32_t at){dispatch(value,target,at,false);}'
CANONICAL = '[](std::uint32_t target,auto image){return canonical_branch(target,image);}'
LOOKUP = 'canonical_branch(r(s.memory,(0x252d84u+s.gp[0]*4u),4),s.image)'
DESTINATIONS = (0x2524d4,0x252597,0x252628,0x252672,0x2526ee,0x2527ac,0x2528b0,0x252a12,0x252b19,0x252bd8)


class Mode(NamedTuple):
    name: str
    header: str
    which: bool
    entry: int
    limit: int
    resume: str
    sites: int
    exits: int
    mul: int
    curves: tuple
    required: tuple

    @property
    def start(self):
        return f'L{self.entry:x}: '

    @property
    def end(self):
        return f'L{self.limit:x}: '


MODES = (
    Mode('mode4','cone',False,0x2526ee,0x2527ac,'L252cce',46,1,3,
         (0x252730,0x252746),()),
    Mode('mode5','cone',True,0x2527ac,0x2528b0,'L252cce',63,1,4,
         (0x2527f4,0x252846),((0x252879,0x148de0),)),
    Mode('mode6','bounds',False,0x2528b0,0x252a12,'L252cce',94,2,3,
         (0x2528e9,),((0x252996,0x259b0),(0x2529e0,0x259b0))),
    Mode('mode7','bounds',True,0x252a12,0x252b19,'L252cce',70,1,1,
         (0x252a8e,),((0x252aca,0x259b0),(0x252ae6,0x259b0))),
    Mode('mode8','special',False,0x252b19,0x252bd8,'L252cce',47,1,0,
         (0x252b58,),()),
    Mode('alternate','special',True,0x252bd8,0x252cc9,'L252cc9',62,0,1,
         (0x252c20,0x252c72),((0x252c98,0x259b0),)),
)


def alias(name):
    return f'native_particle_velocity_remaining_{name}_reference_{OWNER}'


def hook(mode):
    return (f'\n    // BEGIN native particle velocity {mode.name}\n'
            f'    if ({NAMESPACE}{mode.header}_detail::bridge<{str(mode.which).lower()}>(\n'
            f'        s,{alias(mode.name)},{properties.DIRECT},{DISPATCH},\n'
            f'        {angles.TRIG},native_single_spill))goto {mode.resume};\n'
            f'    // END native particle velocity {mode.name}\n')


def reference(mode, span, replay=True):
    continuation = f'goto {mode.resume};'
    if span.count(properties.CHILD) != len(mode.curves) or span.count(continuation) != mode.exits:
        raise ValueError(f'Particle velocity {mode.name} curve calls or exits drifted')
    for site in mode.curves:
        if not re.search(rf's\.push\(0x{site:x}u,0x{site+5:x}u\);{re.escape(properties.CHILD)}',span):
            raise ValueError(f'Particle velocity {mode.name} curve return drifted')
    if replay:
        span = span.replace(properties.CHILD,properties.REPLAY)
    if span.count('dispatch(s,target,') != len(mode.required):
        raise ValueError(f'Particle velocity {mode.name} required dispatch count drifted')
    for site,target in mode.required:
        call = (f'const auto target=0x{target:x}u;s.push(0x{site:x}u,0x{site+5:x}u);'
                f'dispatch(s,target,0x{site:x}u,false);')
        if span.count(call) != 1:
            raise ValueError(f'Particle velocity {mode.name} required dispatch drifted')
        if replay:
            span = span.replace(f'dispatch(s,target,0x{site:x}u,false);',
                                'halo2::native_world_leaves_detail::replay_child(s,target);')
    name = alias(mode.name) if replay else alias(mode.name) + '_live'
    return (f'static void {name}(WorldState &s) {{\n' +
            angles.normalized(span,mode.mul).replace(continuation,'return;') + '}\n')


def selection_reference():
    return (f'static std::uint32_t {alias("selection")}(WorldState &s) {{\n'
            f'    {properties.SWITCH}return {LOOKUP};\n'
            '}\n')


def selection_hook():
    return (
        '{std::uint32_t target;\n'
        f'    if (!{NAMESPACE}special_detail::select(s,target,{alias("selection")},{CANONICAL}))'
        f'{{{properties.SWITCH}target={LOOKUP};}}\n'
        '    switch(target)'
    )


def install(canonical, output):
    if not properties.admitted(canonical):
        return output,[]
    resolved = re.findall(r'case 0x([a-f0-9]+)u:return 0x([a-f0-9]+)u;',
                          re.search(r'^std::uint32_t canonical_branch\([^\n]+',canonical,re.M)[0])
    if {tuple(int(value,16) for value in pair) for pair in resolved if int(pair[1],16) in DESTINATIONS} != {
            (value,value) for value in DESTINATIONS}:
        return output,[]
    found = properties.world.body(output,OWNER)
    if not found or any(alias_name + ',' not in found[0] for alias_name in
                        [angles.alias(mode[0]) for mode in angles.MODES] +
                        [direction.alias(mode[0]) for mode in direction.MODES]):
        return output,[]
    body = found[0]
    if hook(MODES[0]) in body:
        return output,[]
    refs = [selection_reference()]
    for mode in reversed(MODES):
        start, end = angles.extract(body,mode.start,mode.end)
        span = body[start:end]
        if 'selected(' in span or span.count('s.instruction_literal(') != mode.sites:
            return output,[]
        refs.append(reference(mode,span))
        body = body[:start] + hook(mode) + angles.normalized(span,mode.mul) + body[end:]
    dispatch = properties.SWITCH + ' switch(' + LOOKUP + ')'
    endings = ''.join(f'case 0x{at:x}u:goto L{at:x};' for at in DESTINATIONS)
    trap = 'default:throw std::runtime_error("Required original solver dispatch target unavailable");}'
    if body.count(dispatch + '{' + endings + trap) != 1:
        return output,[]
    body = body.replace(dispatch + '{' + endings + trap,selection_hook() + '{' + endings + trap + '}',1)
    return INCLUDE + output[:found.start()] + '\n'.join(refs) + '\n' + body + output[found.end():],[OWNER]
