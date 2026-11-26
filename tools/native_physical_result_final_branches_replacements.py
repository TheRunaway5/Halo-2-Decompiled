"""Bind the two remaining TU1.5 physical-result notification and publication ranges."""

import re

import native_physical_result_activity_replacements as activity


OWNER_PIN = 'f6e7acc994f89c99e4ffaca8d50389941ce4a7578a28fa6594d60cdbd299b8fa'
NOTIFY = ('e4d6d', 'e4dc0')
IMPULSE = ('e5094', 'e51d6')
COMMON_PUBLISH = 'e51cd'
HEADERS = ('physics/runtime/native_tu15_physical_result_notify_e4d6d.hpp',
           'physics/runtime/native_tu15_physical_result_impulse_e5094.hpp')
INCLUDES = ''.join(f'#include "{header}"\n' for header in HEADERS)
NOTIFY_DETAIL = 'halo2::native_physical_result_notify'
IMPULSE_DETAIL = 'halo2::native_physical_result_impulse'
NOTIFY_CHILDREN = (('e4da0', 'e3310', 'e4da5', False), ('e4dab', 'e31c0', 'e4db0', True),
                   ('e4db4', 'e94e0', 'e4db9', False), ('e4dbb', 'e7e80', 'e4dc0', False))
IMPULSE_CHILDREN = (('e50f5', 'ba7e0', 'e50fa', False), ('e5176', 'b7e50', 'e517b', False),
                    ('e5181', '1cd620', 'e5186', False), ('e518d', 'ba630', 'e5192', False),
                    ('e5194', 'bda50', 'e5199', True), ('e51d1', '1cd6a0', 'e51d6', True))
PUSHES = {
    NOTIFY: ('e4da0', 'e4dab', 'e4db4', 'e4dbb'),
    IMPULSE: ('e50f4', 'e50f5', 'e5173', 'e5175', 'e5176', 'e517b', 'e517d', 'e5181', 'e518a', 'e518c',
              'e518d', 'e5194', 'e51cb', 'e51cf', 'e51d1'),
}
PINS = {
    NOTIFY: ('bebecc7d333e79e56df0839080763ff1c21aa14a0041aa62b8e597dd4450e843',
             'c313776ace114b3bd28a5fd7ac720797625419339022a81207ead63d865de6e5'),
    IMPULSE: ('b6781c05301b1f63c6a1e28714f2f175f514d98c5cbd9c99dd70f4840b9e4d74',
              'aab0270b473d9c6db75ecb31b97d67e90e454cd776f4c028c822c26104ac2e49'),
}


def canonical_stages(canonical):
    owner = activity.BODY.search(canonical)
    if not owner or activity.sha(owner[0]) != activity.PIN:
        raise ValueError('Canonical physical-result owner changed')
    stages = {}
    for bounds, children, sites in ((NOTIFY, NOTIFY_CHILDREN, 26), (IMPULSE, IMPULSE_CHILDREN, 89)):
        span = activity.source_span(owner[0], bounds)
        stage = owner[0][span]
        if activity.sha(stage) != PINS[bounds][0] or stage.count('s.services.instruction(') != sites:
            raise ValueError(f'Canonical physical-result stage {bounds[0]} changed')
        for site, target, resume, dispatch in children:
            if stage.count(activity.child_call(site, target, resume, dispatch)) != 1 or (
                    not dispatch and canonical.count(f'static void run_{target}(WorldState &s) {{') != 1):
                raise ValueError(f'Canonical physical-result child at {site} changed')
        stages[bounds] = stage
    return stages


def retained(stage, bounds):
    if tuple(match[2] for match in activity.PUSH.finditer(stage)) != PUSHES[bounds]:
        raise ValueError(f'Physical-result stage {bounds[0]} stack sites changed')
    stage = activity.PUSH.sub(lambda m: f'{activity.TITLE_UPDATE}halo2::native_two_bone_solve::push_word(s,{m[1]});else {m[0]}', stage)
    if bounds == IMPULSE:
        for name, exact in (('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss'),
                            ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss')):
            if stage.count(name+'(') != 3:
                raise ValueError(f'Physical-result impulse SSE operands of {name} changed')
            stage = stage.replace(name+'(', 'halo2::'+exact+'(')
    return stage


def hook(pc, exit_pc, detail, api, children, entry=None):
    route = activity.route_hook(api, children).replace(activity.DETAIL, detail, 1)
    if entry:
        route = route.removesuffix('})') + f'}}, {detail}::Entry::{entry})'
    return f'L{pc}:\n    if({route})goto L{exit_pc};\n'


def new_body(owner):
    if activity.sha(owner) != OWNER_PIN:
        raise ValueError('Combined physical-result owner changed')
    for bounds, end in ((NOTIFY, 'Le4dc0:\n    if('), (IMPULSE, 'Le51d6:\n')):
        first, exit_pc = bounds
        ingress = f'L{first}: '
        start = activity.source_span(owner,bounds).start
        stop = owner.index(end, start)
        stage = owner[start:stop]
        if activity.sha(stage) != PINS[bounds][1] or not stage.startswith(ingress):
            raise ValueError(f'Prepared physical-result stage {first} changed')
        stage = retained(stage, bounds)
        if bounds == NOTIFY:
            stage = hook(first, exit_pc, NOTIFY_DETAIL, 'notify', NOTIFY_CHILDREN) + stage.removeprefix(ingress)
        else:
            stage = hook(first, exit_pc, IMPULSE_DETAIL, 'publish', IMPULSE_CHILDREN, 'apply') + stage.removeprefix(ingress)
            shared = f'L{COMMON_PUBLISH}: '
            if stage.count(shared) != 1:
                raise ValueError('Physical-result common publication entry changed')
            stage = stage.replace(shared, hook(COMMON_PUBLISH, exit_pc, IMPULSE_DETAIL, 'publish', IMPULSE_CHILDREN, 'publish'), 1)
        owner = owner[:start] + stage + owner[stop:]
    return owner


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    canonical_stages(canonical)
    owner = activity.BODY.search(output)
    if not owner or output.count(activity.INCLUDE) != 1 or INCLUDES in output:
        raise ValueError('Expected preceding E4A40 binding is absent or already extended')
    return INCLUDES + output[:owner.start()] + new_body(owner[0]) + output[owner.end():], [NOTIFY[0], IMPULSE[0]]
