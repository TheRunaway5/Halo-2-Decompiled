"""Bind the complete TU1.5 particle cleanup and its original child edges."""

import hashlib
import re

import native_world_leaves_replacements as world


ENTRY = '251650'
CANONICAL_PIN = 'd75ce9b476a953667c66c3bcab1f1e8a1ff74425a5d824154061ab726e6b8ba3'
PREPARED_PIN = 'd3d09348f6ff7915206e5268146d3496872fdef7094822efce6050011c972c2a'
INCLUDE = '#include "rendering/effects/native_tu15_particle_cleanup_251650.hpp"\n'
DETAIL = 'halo2::native_particle_cleanup_251650_detail'
HOOK = (
    f'\n    if ({DETAIL}::bridge(s,'
    '\n        [](auto &value) { run_178070(value); },'
    '\n        [](auto &value) { run_13a6b0(value); },'
    '\n        [](auto &value, std::uint32_t target, std::uint32_t site) { dispatch(value,target,site,false); })) return;'
)
TITLE_UPDATE = 's.image==SourcePhysicsContactImage::TitleUpdate15'
PUSH = re.compile(r'\bs\.push\((0x[0-9a-f]+u),(0x[0-9a-f]+u|s\.reg\(\d,4,0\))\);')
DEC = 'w(s.memory,s.observe,0x2516a9u,(0x2u+s.gp[5]),2,s.flags.increment(r(s.memory,(0x2u+s.gp[5]),2),2,true));'


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def retained(body):
    # On TU1.5, precise guest faults and post-store observers see the committed
    # stack pointer and DEC flags. The other image keeps its existing policy.
    body, n = PUSH.subn(
        lambda m: f'if({TITLE_UPDATE}){{{DETAIL}::push(s,{m[1]},{m[2]});}}else{{{m[0]}}}', body)
    if n != 12 or body.count(DEC) != 1:
        raise ValueError('Prepared 251650 PUSH or DEC operands changed')
    return body.replace(DEC, f'if({TITLE_UPDATE}){{{DETAIL}::decrement_particle_count(s);}}else{{{DEC}}}', 1)


def install(canonical, output):
    if 'void source_run_effect_particle_frame_tu15(' not in canonical:
        return output, []
    original = world.body(canonical, ENTRY)
    prepared = world.body(output, ENTRY)
    dispatch = f'if(target==selected(0x{ENTRY}u,s.image)){{run_{ENTRY}(s);return;}}'
    if (not original or not prepared or original[1] != 'WorldState' or prepared[1] != 'WorldState' or
            sha(original[0]) != CANONICAL_PIN or sha(prepared[0]) != PREPARED_PIN or
            canonical.count(f'case 0x{ENTRY}u:run_{ENTRY}(s);break;') != 1 or
            canonical.count(dispatch) != 1 or INCLUDE in output):
        raise ValueError('Authenticated particle cleanup owner or route changed')
    operands = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', original[0])}
    mapping = world.mapping(canonical)
    if mapping.get(int(ENTRY, 16)) != int(ENTRY, 16) or any(mapping.get(value) != value for value in operands):
        raise ValueError('Particle cleanup TU1.5 resolver changed')
    body = retained(prepared[0])
    opening = body.index('{') + 1
    return INCLUDE + output[:prepared.start()] + body[:opening] + HOOK + body[opening:] + output[prepared.end():], [ENTRY]
