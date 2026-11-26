"""Bind TU1.5 physical-result setup with the original child dispatch policy."""

from hashlib import sha256
import re


CLOSE = 'throw std::runtime_error("Original solver fell outside authenticated CFG");\n}'
BODY = re.compile(r'^static void run_e4a40\(WorldState &s\) \{.*?' + re.escape(CLOSE), re.M | re.S)
PIN = '0a7bcd306aa3b34e678f7ef5c32a5019360dc721b57e5d5d1b17c037c7a072c7'
PREPARED = '175c35b1a05823abaae6732a7085808b52ead62084b3aa02acc9ae321605b35f'
HEADER = 'physics/runtime/native_tu15_physical_result_activity.hpp'
INCLUDE = f'#include "{HEADER}"\n'
DETAIL = 'halo2::native_physical_result_activity'
ENTER = re.compile(r'^.*?instruction(?:_literal)?\(0xe4b81u\)', re.M)
RMW = 'w(s.memory,s.observe,0xe4a92u,(0x348u+s.gp[5]),1,s.flags.logic((r(s.memory,(0x348u+s.gp[5]),1))&(0xefu),1));'
TITLE_UPDATE = 'if(s.image==SourcePhysicsContactImage::TitleUpdate15)'
PUSH = re.compile(r'\bs\.push\((0x([0-9a-f]+)u,[^;\n]+)\);')
PUSH_SITES = ('e4a55', 'e4a56', 'e4a57', 'e4a7e', 'e4aaa', 'e4ac1', 'e4ac4', 'e4afe',
              'e4b05', 'e4b0a', 'e4b53', 'e4b58', 'e4b6f', 'e4b77', 'e4b7b', 'e4b7c')
CHILDREN = (('e4aaa', 'e0500', 'e4aaf', False), ('e4ac4', 'bbbc0', 'e4ac9', True),
            ('e4b0a', '1ed880', 'e4b0f', False), ('e4b58', '1edda0', 'e4b5d', False),
            ('e4b7c', 'e53e0', 'e4b81', False))
PROJECT_CHILDREN = (('e4d2f', '144210', 'e4d34', False), ('e4d3f', '1452f0', 'e4d44', False),
                    ('e4d52', '1d5b50', 'e4d57', False), ('e4d68', '1d5db0', 'e4d6d', True))
ACTIVITY_CHILDREN = (('e5046', 'e0500', 'e504b', False),)
PROJECT_PUSH_SITES = ('e4d2f', 'e4d3f', 'e4d4a', 'e4d52', 'e4d66', 'e4d67', 'e4d68')
ACTIVITY_PUSH_SITES = ('e5046', 'e508a')
PROJECT = ('e4c58', 'e4d6d')
ACTIVITY = ('e4f90', 'e5094')


def sha(text):
    return sha256(text.encode()).hexdigest()


def child_call(site, target, resume, dispatch):
    push = f's.push(0x{site}u,0x{resume}u);'
    if dispatch:
        return f'{{const auto target=0x{target}u;{push}dispatch(s,target,0x{site}u,false);}}'
    return f'{push}run_{target}(s);'


def retained(stage):
    if tuple(match[2] for match in PUSH.finditer(stage)) != PUSH_SITES or stage.count(RMW) != 1:
        raise ValueError('Physical-result activity provider sites changed')
    stage = stage.replace(RMW, f'{TITLE_UPDATE}{DETAIL}::clear_activity_flag(s);else {RMW}')
    return PUSH.sub(lambda match: f'{TITLE_UPDATE}halo2::native_two_bone_solve::push_word(s,{match[1]});else {match[0]}', stage)


def route_hook(api, children):
    route = ''.join(f'        case 0x{site}u: ' + (f'dispatch(v,target,0x{site}u,false);' if dispatch else f'run_{target}(v);')
                    + ' return;\n' for site, target, _, dispatch in children)
    return (f'{DETAIL}::{api}(s,[](auto &v,std::uint32_t site,std::uint32_t target) {{\n'
            '        switch(site) {\n' + route + '        default: throw std::logic_error("Unknown physical-result activity child");\n'
            '        }\n    })')


def source_span(body, bounds):
    def marker(pc):
        return re.search(rf'^.*?instruction(?:_literal)?\(0x{pc}u\)', body, re.M).start()
    return slice(marker(bounds[0]), marker(bounds[1]))


def retained_motion(stage, bounds):
    projection = bounds == PROJECT
    pushes = PROJECT_PUSH_SITES if projection else ACTIVITY_PUSH_SITES
    if tuple(match[2] for match in PUSH.finditer(stage)) != pushes:
        raise ValueError('Physical-result motion push sites changed')
    stage = PUSH.sub(lambda match: f'{TITLE_UPDATE}halo2::native_two_bone_solve::push_word(s,{match[1]});else {match[0]}', stage)
    operations = (('_mm_sub_ss', 'native_physical_result_activity::projection_difference', 3),) if projection else (
        ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss', 3),
        ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss', 2))
    for source, native, count in operations:
        if stage.count(source + '(') != count:
            raise ValueError('Physical-result motion SSE sites changed')
        stage = stage.replace(source + '(', 'halo2::' + native + '(')
    if not projection:
        for site, offset, mask, set_bits in (('e4f94','349','8',True), ('e4f9f','348','2',True), ('e4fa8','348','fd',False)):
            address = f'(0x{offset}u+s.gp[5])'
            op = '|' if set_bits else '&'
            expected = f'w(s.memory,s.observe,0x{site}u,{address},1,s.flags.logic((r(s.memory,{address},1)){op}(0x{mask}u),1));'
            if stage.count(expected) != 1:
                raise ValueError(f'Physical-result activity RMW {site} changed')
            args = f's,0x{site}u,0x{offset}u,0x{mask}u,{str(set_bits).lower()}'
            stage = stage.replace(expected, f'{TITLE_UPDATE}{DETAIL}::change_activity_byte({args});else {expected}')
    return stage


def native_motion(body):
    projection = '    if(' + route_hook('project_result', PROJECT_CHILDREN) + ')goto Le4d6d;\n'
    activity = ('Le4f90:\n    switch(' + route_hook('update_activity', ACTIVITY_CHILDREN) + ') {\n' +
                f'    case {DETAIL}::Alternate::apply_impulse: goto Le5094;\n'
                f'    case {DETAIL}::Alternate::decay: goto Le51d6;\n'
                f'    case {DETAIL}::Alternate::publish: goto Le51cd;\n'
                f'    case {DETAIL}::Alternate::reference: break;\n    }}\n')
    for bounds, hook in ((PROJECT, projection), (ACTIVITY, activity)):
        window = source_span(body, bounds)
        stage = retained_motion(body[window], bounds)
        if bounds == ACTIVITY:
            if not stage.startswith('Le4f90: '):
                raise ValueError('Prepared physical-result activity entry label changed')
            stage = stage.removeprefix('Le4f90: ')
        body = body[:window.start] + hook + stage + body[window.stop:]
    return body


def new_body(prepared):
    if sha(prepared) != PREPARED:
        raise ValueError('Prepared physical-result activity owner changed')
    first, rest = prepared[:ENTER.search(prepared).start()], prepared[ENTER.search(prepared).start():]
    hook = ('\n    switch(' + route_hook('begin', CHILDREN) + ') {\n' +
            f'    case {DETAIL}::Continuation::tick: goto Le4b81;\n'
            f'    case {DETAIL}::Continuation::blocked: goto Le52bd;\n'
            f'    case {DETAIL}::Continuation::reference: break;\n'
            '    }\n')
    first = retained(first)
    opening = first.index('{') + 1
    return native_motion(first[:opening] + hook + first[opening:] + 'Le4b81: ' + rest)


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    raw, prepared = BODY.search(canonical), BODY.search(output)
    if not raw or sha(raw[0]) != PIN or not prepared or INCLUDE in output:
        raise ValueError('Canonical physical-result activity binding changed')
    first = raw[0][:ENTER.search(raw[0]).start()]
    if (canonical.count('case 0xe4a40u:run_e4a40(s);return;') != 1 or
            canonical.count('case 0xe4a40u:run_e4a40(s);break;') != 1):
        raise ValueError('Physical-result activity dispatch changed')
    for stage, children in ((first, CHILDREN), (raw[0][source_span(raw[0], PROJECT)], PROJECT_CHILDREN),
                            (raw[0][source_span(raw[0], ACTIVITY)], ACTIVITY_CHILDREN)):
        for site, target, resume, dispatch in children:
            if stage.count(child_call(site, target, resume, dispatch)) != 1 or (
                    not dispatch and canonical.count(f'static void run_{target}(WorldState &s) {{') != 1):
                raise ValueError(f'Physical-result activity child at {site} changed')
    return INCLUDE + output[:prepared.start()] + new_body(prepared[0]) + output[prepared.end():], ['e4a40']
