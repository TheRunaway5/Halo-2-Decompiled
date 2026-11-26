"""Bind the TU1.5 E4DC0 physical contact transition phase."""

from hashlib import sha256
import re

import native_physical_result_activity_replacements as activity


ENTRY = 'e4dc0'
EXIT = 'e4f90'
RAW_PIN = 'e5e0d205a75f510252dcaea9c823266e7bdb27fbdafe88a738d7a2dfd0735047'
PREPARED_PIN = 'fcc7afaf23b8094489f68bb47dd83d9fb985ba9c329a3b67a4e38881d0fc3dff'
OWNER_PIN = '9199e452ee7179509e57d44ee7833a3314ef330dfef8d1b2248a1e48b19ea518'
HEADER_PIN = '90eb4a6b1b3494e614b46a3bf8ac528ae57eecbd359d619e6ffa63642ae8ebe2'
ENTRY_LABEL = 'Le4dc0: '
CONTINUATION = '\nLe4f90:\n'
CHILDREN = (('e4dcc', 'e3310', 'e4dd1', False), ('e4e8c', '1cc310', 'e4e91', False),
            ('e4ef8', '1cc310', 'e4efd', False), ('e4f19', 'dfd20', 'e4f1e', False),
            ('e4f45', 'e65a0', 'e4f4a', False))
PUSH_SITES = ('e4dcc', 'e4e83', 'e4e85', 'e4e8c', 'e4ee5', 'e4ef3', 'e4ef8', 'e4f19', 'e4f45')
RMW_SITES = ('e4efd', 'e4f0a')


def digest(text):
    return sha256(text.encode()).hexdigest()


def canonical_stage(canonical):
    owner = activity.BODY.search(canonical)
    if not owner or digest(owner[0]) != activity.PIN:
        raise ValueError('Canonical physical contact owner changed')
    window = activity.source_span(owner[0], (ENTRY, EXIT))
    stage = owner[0][window]
    if digest(stage) != RAW_PIN:
        raise ValueError('Canonical physical contact phase changed')
    for site, target, resume, dispatch in CHILDREN:
        if (stage.count(activity.child_call(site, target, resume, dispatch)) != 1 or
                canonical.count(f'static void run_{target}(WorldState &s) {{') != 1):
            raise ValueError(f'Physical contact child at {site} changed')
    # The only incoming jumps land on the same authenticated entry as the
    # preceding child return. No internal label can bypass the new hook.
    inner_labels = set(re.findall(r'^L([0-9a-f]+):', stage, re.M)) - {ENTRY}
    outer = owner[0][:window.start] + owner[0][window.stop:]
    if any(f'goto L{label};' in outer for label in inner_labels) or outer.count(f'goto L{ENTRY};') != 2:
        raise ValueError('Physical contact phase has different incoming edges')
    return stage


def retained(stage):
    if tuple(match[2] for match in activity.PUSH.finditer(stage)) != PUSH_SITES:
        raise ValueError('Physical contact push sites changed')
    stage = activity.PUSH.sub(lambda match: f'{activity.TITLE_UPDATE}halo2::native_two_bone_solve::push_word(s,{match[1]});else {match[0]}', stage)
    arithmetic = (('_mm_sub_ss', 'native_physical_result_activity::projection_difference', 3),
                  ('_mm_div_ss', 'native_physical_result_activity::contact_division', 1),
                  ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss', 3),
                  ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss', 2))
    for source, native, count in arithmetic:
        if stage.count(source + '(') != count:
            raise ValueError(f'Physical contact operation {source} changed')
        stage = stage.replace(source + '(', 'halo2::' + native + '(')
    for site, mask in zip(RMW_SITES, ('20', '40'), strict=True):
        address = '(0x349u+s.gp[5])'
        expected = f'w(s.memory,s.observe,0x{site}u,{address},1,s.flags.logic((r(s.memory,{address},1))|(0x{mask}u),1));'
        if stage.count(expected) != 1:
            raise ValueError(f'Physical contact RMW {site} changed')
        operation = f'{activity.DETAIL}::change_activity_byte(s,0x{site}u,0x349u,0x{mask}u,true);'
        stage = stage.replace(expected, f'{activity.TITLE_UPDATE}{operation}else {expected}')
    return stage


def new_body(owner):
    if digest(owner) != OWNER_PIN:
        raise ValueError('Qualified E4A40 owner input changed')
    opening = owner.index(ENTRY_LABEL + 's.instruction_literal(0xe4dc0u);')
    ending = owner.index(CONTINUATION, opening) + 1
    before = owner[opening:ending]
    if digest(before) != PREPARED_PIN:
        raise ValueError('Prepared physical contact phase changed')
    hook = f'L{ENTRY}:\n    if(' + activity.route_hook('update_contact_transition', CHILDREN) + f')goto L{EXIT};\n'
    return owner[:opening] + hook + retained(before).removeprefix(ENTRY_LABEL) + owner[ending:]


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    canonical_stage(canonical)
    owner = activity.BODY.search(output)
    if not owner or output.count(activity.INCLUDE) != 1:
        raise ValueError('Expected preceding E4A40 binding is absent')
    return output[:owner.start()] + new_body(owner[0]) + output[owner.end():], [ENTRY]
