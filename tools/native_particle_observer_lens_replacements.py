"""Bind the complete TU1.5 particle observer lens owner 174BF0."""

import hashlib
import re

import native_world_leaves_replacements as world


OWNER = '174bf0'
PIN = '0ec6442033580436dd2f97e785196031c4482923fb52b9f3eefce28cc8c06b63'
SOURCE_OWNER = 'void source_run_effect_particle_draw_tu15('
INCLUDE = '#include "rendering/effects/native_tu15_particle_observer_lens.hpp"\n'
NAMESPACE = 'halo2::native_particle_observer_lens_detail'
REFERENCE = 'native_particle_observer_lens_reference_' + OWNER
TANGENT = re.compile(r'^long double native_fptan\(.*?^}',re.S | re.M)
TANGENT_SHA = 'ef6f51634151286d60ac4eab8d287ea0b710561e759bbf379a42abe22c160c35'
TANGENT_STATUS = ':"=a"(status),"=m"(result):"m"(x)'
TRIG = (f'[]({NAMESPACE}::Trig op,long double value,std::uint16_t &status){{'
        f'if(op=={NAMESPACE}::Trig::sine)return native_fsin(value,status);'
        f'if(op=={NAMESPACE}::Trig::cosine)return native_fcos(value,status);'
        'return native_fptan(value,status);}')
HOOK = (
    '\n    // BEGIN native particle observer lens 174bf0\n'
    f'    if ({NAMESPACE}::bridge(s,{REFERENCE},\n'
    f'        {TRIG},native_fpatan,native_single_spill))return;\n'
    '    // END native particle observer lens 174bf0'
)


def admitted(canonical):
    body = world.body(canonical, OWNER)
    tangent = TANGENT.search(canonical)
    return (body and body[1] == 'WorldState' and SOURCE_OWNER in canonical and
            canonical.count('case 0x174bf0u:run_174bf0(s);return;') == 1 and
            canonical.count('case 0x174bf0u:run_174bf0(s);break;') == 1 and
            tangent and hashlib.sha256(tangent[0].encode()).hexdigest() == TANGENT_SHA and
            canonical.count('native_fptan(') == 3 and body[0].count('native_fptan(') == 2 and
            world.digest(body) == PIN)


def fixed_tangent(helper):
    # FNSTSW writes AX before the final memory store. That store's address
    # must not use RAX, even when the compiler inlines the helper.
    if helper.count(TANGENT_STATUS) != 1:
        raise ValueError('Particle observer lens FPTAN status operand drifted')
    return helper.replace(TANGENT_STATUS,TANGENT_STATUS.replace('"=a"','"=&a"'),1)


def normalized(body):
    for operation, count, namespace in (('mul',75,'native_tu15_leaves'), ('add',38,'native_world_leaves_detail')):
        generic = f'_mm_{operation}_ss('
        ordered = f'halo2::{namespace}::ordered_{operation}_ss('
        if body.count(generic) + body.count(ordered) != count:
            raise ValueError(f'Particle observer lens {operation}ss destinations drifted')
        body = body.replace(generic, ordered)
    return body


def reference(body):
    return normalized(body).replace('run_' + OWNER + '(', REFERENCE + '(', 1)


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    found = world.body(output, OWNER)
    if not found or found[1] != 'WorldState' or HOOK in found[0] or 'selected(' in found[0]:
        return output, []
    body = found[0]
    sites = re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)',body)
    if len(sites) != 359 or len(set(sites)) != 359:
        return output, []
    tangent = TANGENT.search(output)
    if not tangent or tangent[0] != TANGENT.search(canonical)[0]:
        return output, []
    boundary = body.index('{') + 1
    replacement = body[:boundary] + HOOK + normalized(body[boundary:])
    bound = output[:found.start()] + reference(body) + '\n\n' + replacement + output[found.end():]
    return INCLUDE + bound.replace(tangent[0],fixed_tangent(tangent[0]),1), [OWNER]
