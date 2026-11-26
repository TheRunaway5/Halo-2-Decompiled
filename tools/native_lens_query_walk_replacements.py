"""Bind the TU1.5 lens query walk and preserve its six real call sites."""

import hashlib
import re

import native_world_leaves_replacements as world


OWNER = '20b90'
PIN = 'afadfebc4158ae795c3970145d69f1366d562b370591e4fb0cc2b6e66809fa60'
PREPARED = 'd1067c928356a57fba800ad163393fea41a2d950fc2e657964c999eb61cfbd8d'
CHILDREN = {
    'c1e90': '46c83b17377dccecd8a078cf050eb0807d7007188907022f8bfc858b1cf9d852',
    '402710': 'b5aa6af01b6df74fc724ddccc2fb31a49125ddca07617a28999f21ab098f3ebc',
    '4026b0': '26ae5ddcaaa0465022eb9ad7d5f6e8325b7eec36d3a8f43c0576b83896b7e35a',
}
NAMESPACE = 'halo2::native_lens_query_walk_detail'
INCLUDE = '#include "rendering/effects/native_tu15_lens_query_walk.hpp"\n'
RESUMES = (
    '20b96', '20b9b', '20bac', '20bb8', '20bc5', '20bd7', '20bdf', '20be7',
    '20beb', '20bef', '20bf0', '20bf8', '20bff', '20c00', '20c01', '20c07',
    '20c0b', '20c17', '20c1b', '20c21', '20c32', '20c3a', '20c3e', '20c5e',
    '20c6a', '20c7f', '20c87', '20c8c', '20c95', '20ca0', '20cb6', '20cb8',
    '20cbb', '20cc2', '20cc7', '20ccc', '20cd2', '20cda', '20ce0', '20ce1',
    '20ce5', '20ce7', '20ceb', '20cf4', '20d11', '20d15', '20d1b', '20d1f',
    '20d33', '20d39', '20d3a', '20d3e', '20d42', '20d49', '20d50', '20d51',
    '20d52', '20d53',
)
HOOK = ('\n    // BEGIN native lens query walk 20b90\n'
        f'    switch ({NAMESPACE}::bridge(s,dispatch,run_c1e90,run_402710,run_4026b0)) {{\n'
        f'    case {NAMESPACE}::done: return;\n'
        + ''.join(f'    case 0x{site}u: goto L{site};\n' for site in RESUMES)
        + '    default: break;\n    }\n'
        '    // END native lens query walk 20b90\n')


def admitted(canonical):
    owner = world.body(canonical, OWNER)
    if (not owner or owner[1] != 'WorldState' or world.digest(owner) != PIN or
            'void source_run_lens_query_tu15(' not in canonical or
            canonical.count('case 0x20b90u:run_20b90(s);break;') != 1 or
            canonical.count('if(target==selected(0x20b90u,s.image)){run_20b90(s);return;}') != 1):
        return False
    mapping = world.mapping(canonical)
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', owner[0])}
    return all(mapping.get(value, value) == value for value in used)


def prepared_by_pin(output, name, pin):
    found = world.body(output, name)
    return found if found and hashlib.sha256(found[0].encode()).hexdigest() == pin else None


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    owner = prepared_by_pin(output, OWNER, PREPARED)
    if not owner or not all(prepared_by_pin(output, name, pin) for name, pin in CHILDREN.items()):
        return output, []

    body, saved = re.subn(r'\bs\.push\((0x[0-9a-f]+u),',
                          rf'{NAMESPACE}::save_word(s,\1,', owner[0])
    if saved != 17:
        raise ValueError('Lens query original push and call set drifted')
    old_bitmap = ('w(s.memory,s.observe,0x20d31u,(0x0u+s.gp[0]),4,'
                  's.flags.logic((r(s.memory,(0x0u+s.gp[0]),4))|(s.reg(2,4,0)),4));')
    if body.count(old_bitmap) != 1:
        raise ValueError('Lens query original pending bitmap store drifted')
    body = body.replace(old_bitmap, f'{NAMESPACE}::mark_pending(s);', 1)
    for site in RESUMES:
        marker = f'\nL{site}: '
        if marker in body:
            continue
        anchor = f'\ns.instruction_literal(0x{site}u);'
        if body.count(anchor) != 1:
            raise ValueError(f'Lens query continuation {site} drifted')
        body = body.replace(anchor, marker + anchor[1:], 1)
    offset = body.index('{') + 1
    return INCLUDE + output[:owner.start()] + body[:offset] + HOOK + body[offset:] + output[owner.end():], [OWNER]
