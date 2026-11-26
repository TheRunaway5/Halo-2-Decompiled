"""Bind the TU1.5 weapon animation selection and retain its authored children."""
import hashlib
import re

BODY = re.compile(r'^static void run_1e3420\(WorldState &s\) \{.*?^\}', re.M | re.S)
PIN = '65df16521b262f0f88cceccf00b91b8e0425bdc170638323d188b0f39220327b'
PREPARED = 'fe568e6bf1008b5a0736e4bb2f4753cb1e01cf718975307005cef5ceb5a165c9'
HEADER = 'gameplay/weapons/native_tu15_weapon_animation_selection.hpp'
INCLUDE = f'#include "{HEADER}"\n'
ROUTE = 'case 0x1e3420u:run_1e3420(s);break;'
CHILDREN = {'2a0100': 1, '1e5810': 61, '1e5c80': 1, '126200': 20}
HOOK = (
    '\n    if(halo2::native_weapon_animation_selection::bridge(s,'
    '[](auto &v){run_2a0100(v);},[](auto &v){run_1e5810(v);},'
    '[](auto &v){run_1e5c80(v);},[](auto &v){run_126200(v);}))return;\n'
)


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def retained(body):
    # A failed PUSH leaves ESP at its original value. A rejected store
    # observer receives the advanced ESP after a successful write.
    push = r'\bs\.push\((0x[0-9a-f]+u,)([^;\n]+)\);'
    body, changed = re.subn(
        push,
        lambda m: ('if(s.image==SourcePhysicsContactImage::TitleUpdate15)'
                   'halo2::native_two_bone_solve::push_word(s,'+m[1]+m[2]+');else '+m[0]),
        body,
    )
    if changed != 273:
        raise ValueError('Prepared 1E3420 push sites changed')
    return body


def install(canonical, prepared):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return prepared, []
    raw = list(BODY.finditer(canonical))
    known = list(BODY.finditer(prepared))
    if len(raw) != 1 or sha(raw[0][0]) != PIN or len(known) != 1 or sha(known[0][0]) != PREPARED:
        raise ValueError('Authenticated 1E3420 owner changed')
    if canonical.count(ROUTE) != 1 or prepared.count(ROUTE) != 1 or INCLUDE in prepared:
        raise ValueError('Authenticated 1E3420 route changed')
    match = known[0]
    for child, count in CHILDREN.items():
        if (canonical.count(f'static void run_{child}(WorldState &s) {{') != 1 or
                raw[0][0].count(f';run_{child}(s);') != count or
                match[0].count(f';run_{child}(s);') != count):
            raise ValueError(f'Authenticated 1E3420 child {child} changed')
    body = retained(match[0])
    opening = body.index('{')+1
    return INCLUDE+prepared[:match.start()]+body[:opening]+HOOK+body[opening:]+prepared[match.end():], ['1e3420']
