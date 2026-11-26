"""Bind the 219330 projection around its real 11E600 call and shared return."""

from hashlib import sha256

from native_contrail_effect_launch_replacements import CANONICAL, INCLUDE, owner


CANONICAL_STATUS = '0932f526ac150818b18ef1310f11a4ceffd96a733f6f74356a058c142b0e49aa'
PREPARED_STATUS = '5f41faa7c09906f836710f33105a7e8869493a106a208dd22c3b7bbe3be4b942'
CANONICAL_PROJECTION = 'efdea834b16ca63ceb87393883ea4ceb20c5fc2c56af07379022dd72721b4cbf'
PREPARED_PROJECTION = '2721dc84556c5c18b785bd537474cf8823b3115abaa1c0eaf9d3905520af4b36'
STATUS = 'L2197f8: '
OTHER_EXIT = 'L219809: '
PROJECTION = 'L219847: '
EARLY_EXIT = 'L2198b7: '
CHILD = ('s.instruction_literal(0x219884u); '
         '{const auto target=0x11e600u;s.push(0x219884u,0x219889u);dispatch(s,target,0x219884u,false);}')
CALL_LABEL = 'L_native_contrail_effect_projection_call: '
STATUS_HOOK = '\n    if (halo2::native_contrail_effect_launch_detail::return_launch_status(s)) return;\n'
PROJECTION_HOOK = ('\n    if (halo2::native_contrail_effect_launch_detail::project_to_child(s))\n'
                   '        goto L_native_contrail_effect_projection_call;\n')
RESUME_HOOK = ('\n    {\n'
               '        using namespace halo2::native_contrail_effect_launch_detail;\n'
               '        switch (projection_result(s)) {\n'
               '        case Projection::retained: break;\n'
               '        case Projection::miss: goto L2197f8;\n'
               '        case Projection::finished: return;\n'
               '        }\n'
               '    }')


def spans(body):
    labels = (STATUS, OTHER_EXIT, PROJECTION, EARLY_EXIT)
    if any(body.count(label) != 1 for label in labels):
        raise ValueError('Expected one of each 219330 projection boundary')
    a, b, c, d = (body.index(label) for label in labels)
    if not a < b < c < d:
        raise ValueError('Contrail projection boundary order changed')
    return body[a:b], body[c:d]


def ordered(projection):
    for op, helper in (('mul', 'mul'), ('add', 'sum')):
        token = f'_mm_{op}_ss('
        if projection.count(token) != 3:
            raise ValueError(f'Unexpected 219330 projection {op} operand sites')
        projection = projection.replace(token, f'halo2::native_tu15_leaves::ordered_{helper}_ss(')
    return projection


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    original_status, original_projection = spans(original[0])
    status, projection = spans(current[0])
    check = ((original[0], CANONICAL), (original_status, CANONICAL_STATUS), (status, PREPARED_STATUS),
             (original_projection, CANONICAL_PROJECTION), (projection, PREPARED_PROJECTION))
    if (any(sha256(block.encode()).hexdigest() != pin for block, pin in check) or prepared.count(INCLUDE) != 1
            or projection.count(CHILD) != 1 or projection.count('s.instruction_literal(0x219889u);') != 1):
        raise ValueError('Contrail effect projection differs from the pinned source')
    bound_status = status.replace(STATUS, STATUS + STATUS_HOOK, 1)
    bound_projection = ordered(projection).replace(PROJECTION, PROJECTION + PROJECTION_HOOK, 1)
    bound_projection = bound_projection.replace(CHILD, CALL_LABEL + CHILD + RESUME_HOOK, 1)
    native = current[0].replace(status, bound_status, 1).replace(projection, bound_projection, 1)
    return prepared[:current.start()] + native + prepared[current.end():]
