"""Bind physical-result contact classification through the projection entry."""

import re

import native_physical_result_activity_replacements as activity


ENTRY = 'e4b81'
CONTINUATION = 'e4c58'
RAW_PIN = '6fc7234b9093563e384620bc1dff431331ae879af6103caa926451411444c932'
PREPARED_PIN = 'aec0df41e84615983e2c370754d093c0a5a76af12092016c7d2d083c59c8d51a'
OWNER_PIN = '2980027f846a6d0999853ae5f4fa5df9d05817d1ae4e2fc72f3c586a4dea1b46'
HEADER = 'physics/runtime/native_tu15_physical_result_classification_e4b81.hpp'
INCLUDE = f'#include "{HEADER}"\n'
DETAIL = 'halo2::native_physical_result_classification'
PROJECTION = '    if(' + activity.route_hook('project_result', activity.PROJECT_CHILDREN) + ')goto Le4d6d;\n'
CHILDREN = (('e4bb0', 'b95d0', 'e4bb5', False), ('e4c29', '1d3c20', 'e4c2e', False),
            ('e4c3f', '1d6490', 'e4c44', True), ('e4c4d', 'df820', 'e4c52', False))
PUSH_SITES = ('e4ba0', 'e4ba2', 'e4ba4', 'e4bb0', 'e4c14', 'e4c1b', 'e4c29', 'e4c3a', 'e4c3f', 'e4c4d')
RMW = ('w(s.memory,s.observe,0xe4be4u,(0x3a8u+s.gp[5]),1,'
       's.flags.increment(r(s.memory,(0x3a8u+s.gp[5]),1),1,false));')


def canonical_stage(canonical):
    owner = activity.BODY.search(canonical)
    if not owner or activity.sha(owner[0]) != activity.PIN:
        raise ValueError('Canonical physical-result owner changed')
    span = activity.source_span(owner[0], (ENTRY, CONTINUATION))
    stage = owner[0][span]
    outer = owner[0][:span.start] + owner[0][span.stop:]
    if activity.sha(stage) != RAW_PIN or stage.count('s.services.instruction(') != 54:
        raise ValueError('Canonical physical-result classification span changed')
    labels = re.findall(r'^L([0-9a-f]+):', stage, re.M)
    if (any(f'goto L{label};' in outer for label in labels) or
            outer.count(activity.child_call('e4b7c', 'e53e0', ENTRY, False)) != 1):
        raise ValueError('Physical-result classification ingress changed')
    for site, target, resume, dispatch in CHILDREN:
        if stage.count(activity.child_call(site, target, resume, dispatch)) != 1 or (
                not dispatch and canonical.count(f'static void run_{target}(WorldState &s) {{') != 1):
            raise ValueError(f'Physical-result classification child at {site} changed')
    return stage


def retained(stage):
    if (tuple(match[2] for match in activity.PUSH.finditer(stage)) != PUSH_SITES or
            stage.count(RMW) != 1 or stage.count('_mm_mul_ss(') != 1):
        raise ValueError('Physical-result classification fault/arithmetic sites changed')
    stage = stage.replace(RMW, f'{activity.TITLE_UPDATE}{DETAIL}::increment_duration(s);else {RMW}')
    stage = stage.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
    return activity.PUSH.sub(lambda m: f'{activity.TITLE_UPDATE}halo2::native_two_bone_solve::push_word(s,{m[1]});else {m[0]}', stage)


def hook():
    children = activity.route_hook('classify', CHILDREN).replace(activity.DETAIL, DETAIL, 1)
    return f'L{ENTRY}:\n    if({children})goto L{CONTINUATION};\n'


def new_body(owner):
    if activity.sha(owner) != OWNER_PIN or owner.count(PROJECTION) != 1:
        raise ValueError('Qualified E4A40 owner or projection route changed')
    start = activity.ENTER.search(owner).start()
    end = owner.index(PROJECTION, start)
    stage = owner[start:end]
    label = f'L{ENTRY}: '
    if activity.sha(stage) != PREPARED_PIN or not stage.startswith(label):
        raise ValueError('Prepared physical-result classification span changed')
    return owner[:start] + hook() + retained(stage.removeprefix(label)) + f'L{CONTINUATION}:\n' + owner[end:]


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    canonical_stage(canonical)
    owner = activity.BODY.search(output)
    if not owner or output.count(activity.INCLUDE) != 1 or INCLUDE in output:
        raise ValueError('Expected preceding E4A40 binding is absent or already extended')
    return INCLUDE + output[:owner.start()] + new_body(owner[0]) + output[owner.end():], [ENTRY]
