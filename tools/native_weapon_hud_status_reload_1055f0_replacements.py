"""Bind the authored reload state within the qualified weapon HUD frame."""

import re

from native_model_marker_lookup_replacements import function, digest
from native_weapon_hud_status_1055f0_replacements import IMAGE, NAMESPACE, bind_phase


INPUT_PIN = '2f53765c2b4bdaabf724fd76b6fcd73bff1e743410f99aec09b0f431982f5c17'
INCLUDE = '#include "gameplay/weapons/native_tu15_weapon_hud_status_reload_1055f0.hpp"\n'
CHILDREN = (('1057bc', '10b330'), ('1057f1', 'bcdf0'), ('105856', 'bad60'))
STACK_WRITES = ('1057b6', '1057bb', '1057bc', '1057ed', '1057f1', '10584c', '10584e', '10584f', '105856')
CONTINUATIONS = (
    '10577a', '105788', '10578d', '10579b', '1057a7', '1057b2', '1057b7', '1057bc', '1057c1', '1057cd',
    '1057e4', '1057ef', '1057f6', '105804', '10580f', '105815', '105821', '105824', '105837', '10583c',
    '105846', '10584c', '10584e', '10584f', '105850', '10585b', '105863', '105867', '105870', '105877',
    '10587c', '105883', '105886', '105889', '105890', '105898', '10589c', '1058a8', '1058ae', '1058b2',
    '1058b6', '1058bc', '1058c0', '1058c7', '1058d7', '1058e0',
)
SPILL = ('w(s.memory,s.observe,0x1058b2u,(0x20u+s.gp[4]),4,'
         'integer_spill(s.fp.st(0),s.fp.status));s.fp.pop();')


def install(prepared):
    owner = function(prepared, 'run_1055f0')
    if digest(owner) != INPUT_PIN or INCLUDE in prepared or prepared.count(owner) != 1:
        return prepared, False
    body = owner
    if body.count(SPILL) != 1:
        raise ValueError('Original weapon HUD integer spill changed')
    body = body.replace(SPILL,
                        f'if({IMAGE}){{{NAMESPACE}::spill_reload_duration(s);}}else{{{SPILL}}}', 1)
    for pc in STACK_WRITES:
        pattern = rf's\.push\((0x{pc}u,[^;]+)\);'
        def push(m):
            return f'if({IMAGE}){{halo2::native_two_bone_solve::push_word(s,{m[1]});}}else{{{m[0]}}}'
        body, count = re.subn(pattern, push, body)
        if count != 1:
            raise ValueError(f'Original weapon HUD reload stack write {pc} changed')
    children = ','.join(f'[](WorldState &v) {{ dispatch(v,0x{target}u,0x{pc}u,false); }}'
                        for pc,target in CHILDREN)
    body = bind_phase(body, '105774', f'frame.reload({children})', CONTINUATIONS, ('1058e6',),
                      bridge_name=f'bridge<{NAMESPACE}::ReloadFrame>')
    return INCLUDE + prepared.replace(owner, body, 1), True
