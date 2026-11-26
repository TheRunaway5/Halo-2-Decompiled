"""Bind the call-free physical-result decay and both original RET12 paths."""

from hashlib import sha256
import re

import native_physical_result_activity_replacements as activity


ENTRY = 'e51d6'
BLOCKED = 'e52bd'
RAW_PIN = '6ae8c7c0f94c8905e56efca0e414f4d5e2bedf9952f205b847794f617e156e59'
PREPARED_PIN = '93d41fbf767c5604d204f264fcd0ee5c2d582f6e951c58489ec2f10e31f2044a'
OWNER_PIN = '6fc62ebc94d4a16061cd8bea624523bcb0aafc47d993b509419ebd4a7b6d2afa'
HEADER = 'physics/runtime/native_tu15_physical_result_decay_e51d6.hpp'
INCLUDE = f'#include "{HEADER}"\n'
DETAIL = 'halo2::native_physical_result_decay'


def digest(text):
    return sha256(text.encode()).hexdigest()


def fixed_stage(owner):
    first = owner.index(f'L{ENTRY}: ')
    stop = owner.index(activity.CLOSE, first)
    return slice(first, stop)


def canonical_stage(canonical):
    owner = activity.BODY.search(canonical)
    if not owner or digest(owner[0]) != activity.PIN:
        raise ValueError('Canonical physical-result owner changed')
    span = fixed_stage(owner[0])
    stage = owner[0][span]
    if digest(stage) != RAW_PIN or stage.count('s.pop();s.gp[4]+=12u;return;') != 2:
        raise ValueError('Canonical physical-result tail changed')
    outer = owner[0][:span.start]
    labels = set(re.findall(r'^L([0-9a-f]+):', stage, re.M)) - {ENTRY, BLOCKED}
    if (outer.count(f'goto L{ENTRY};') != 1 or outer.count(f'goto L{BLOCKED};') != 2 or
            any(f'goto L{label};' in outer for label in labels)):
        raise ValueError('Original physical-result tail entries changed')
    edge = '{const auto target=0x1cd6a0u;s.push(0xe51d1u,0xe51d6u);dispatch(s,target,0xe51d1u,false);}'
    if outer.count(edge) != 1 or re.search(r's\.push\(|\brun_[0-9a-f]+\(|\bdispatch\(', stage):
        raise ValueError('Physical-result tail child topology changed')
    return stage


def retained(stage):
    operations = (('_mm_sub_ss', 'native_physical_result_activity::projection_difference', 2),
                  ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss', 4),
                  ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss', 1))
    for source, native, count in operations:
        if stage.count(source + '(') != count:
            raise ValueError(f'Physical-result decay operation {source} changed')
        stage = stage.replace(source + '(', 'halo2::' + native + '(')
    return stage


def hook(pc, path):
    return f'L{pc}:\n    if({DETAIL}::finish(s,{DETAIL}::Entry::{path}))return;\n'


def new_body(owner):
    if digest(owner) != OWNER_PIN:
        raise ValueError('Qualified E4A40 owner input changed')
    span = fixed_stage(owner)
    stage = owner[span]
    if digest(stage) != PREPARED_PIN:
        raise ValueError('Prepared physical-result decay tail changed')
    if (stage.count('if(!s.try_pop_registers<6,5>(0xe52b0u)){') != 1 or
            stage.count('if(!s.try_pop_registers<7,6,5,3>(0xe52c4u)){') != 1):
        raise ValueError('Prepared physical-result return register groups changed')
    stage = retained(stage)
    for pc, path in ((ENTRY, 'decay'), (BLOCKED, 'blocked')):
        stage = stage.replace(f'L{pc}: ', hook(pc, path), 1)
    return owner[:span.start] + stage + owner[span.stop:]


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    canonical_stage(canonical)
    owner = activity.BODY.search(output)
    if not owner or output.count(activity.INCLUDE) != 1 or INCLUDE in output:
        raise ValueError('Expected preceding E4A40 binding is absent or already extended')
    return INCLUDE + output[:owner.start()] + new_body(owner[0]) + output[owner.end():], [ENTRY]
