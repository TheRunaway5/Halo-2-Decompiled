"""Bind the particle definition prefix of TU1.5 particle draw (1772C0)."""

import importlib.util
from pathlib import Path
import re


ENTRY = '1772c0'
PIN = 'd80fbeef10c96b5885cd67688b86b9fc98310f4e29b09513f7ab587a38af3ea6'
REFERENCE = 'native_particle_draw_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_entry.hpp"\n'
POSITION_INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_position.hpp"\n'
VISIBILITY_INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_visibility.hpp"\n'
VISIBILITY_REFERENCE = 'native_particle_draw_visibility_reference_' + ENTRY
POSITION_REFERENCE = 'native_particle_draw_position_reference_' + ENTRY
DRAW_SITE = re.compile(r'^((?:if\(s\.services\.instruction\)\{s\.services\.instruction\(0x177376u\);\}'
                       r'|s\.instruction_literal\(0x177376u\);))', re.M)
CALL = re.compile(r'dispatch\(s,target,0x([0-9a-f]+)u,false\);')
ROUTES = ('1772df', '1772e7', '1772f6', '17732d', '177336', '17735c')
PREFIX = (
    '\n    // BEGIN native particle draw entry 1772c0\n'
    '    switch(halo2::native_particle_draw_entry_detail::bridge(s,' + REFERENCE + ',\n'
    '        [](auto &state,std::uint32_t target,std::uint32_t site){dispatch(state,target,site,false);})) {\n'
    '    case 0x177376u:goto L177376;\n'
    '    case 0x1777c4u:goto L1777c4;\n'
    '    case 0xffffffffu:break;\n'
    '    default:throw std::logic_error("Invalid native particle draw continuation");\n'
    '    }\n'
    '    // END native particle draw entry 1772c0'
)


def world_helper():
    path = Path(__file__).with_name('native_world_leaves_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_draw_entry_metadata', path)
    world = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(world)
    return world


def admitted(canonical, world):
    found = world.body(canonical, ENTRY)
    if not found or found[1] != 'WorldState' or world.digest(found) != PIN:
        return False
    return '#include "halo2/source_effect_particle_draw_tu15.hpp"' in canonical


def reference(body, name=REFERENCE, replay=True):
    end, = DRAW_SITE.finditer(body)
    segment = body[body.index('\n')+1:end.start()]
    if tuple(CALL.findall(segment)) != ROUTES or segment.count('goto L1777c4;') != 2:
        raise ValueError('Particle draw entry branch or child routing drifted')
    segment = segment.replace('goto L1777c4;', 'return 0x1777c4u;')
    if replay:
        segment = CALL.sub('halo2::native_object_tick_detail::replay(s,target);', segment)
    return f'std::uint32_t {name}(WorldState &s) {{\n{segment}return 0x177376u;\n}}\n'


def position_parts(body):
    copy = '\nL1776da: '
    adjust = '\nL17770b: '
    transform = re.compile(r'^((?:if\(s\.services\.instruction\)\{s\.services\.instruction\(0x177759u\);\}'
                           r'|s\.instruction_literal\(0x177759u\);))', re.M)
    stop, = transform.finditer(body)
    if body.count(copy) != 1 or body.count(adjust) != 1:
        raise ValueError('Particle position continuation labels drifted')
    start_copy, start_adjust = body.index(copy), body.index(adjust)
    return (start_copy, start_adjust, stop.start(), copy, adjust)


def ordered_adjustment(section):
    for intrinsic, ordered in (
        ('_mm_mul_ss', 'halo2::native_tu15_leaves::ordered_mul_ss'),
        ('_mm_add_ss', 'halo2::native_world_leaves_detail::ordered_add_ss'),
    ):
        if section.count(f'{intrinsic}(') != 3:
            raise ValueError(f'Particle adjustment {intrinsic} sites drifted')
        section = section.replace(f'{intrinsic}(', f'{ordered}(')
    return section


def position_references(body):
    start, adjusted, stop, copy, adjust = position_parts(body)
    phases = (('copy', body[start + len(copy):adjusted], '1777b7', '17770b'),
              ('adjust', ordered_adjustment(body[adjusted + len(adjust):stop]), '177775', '177759'))
    output = []
    for kind, section, branch, fallthrough in phases:
        edge = f'goto L{branch};'
        if section.count(edge) != 1:
            raise ValueError(f'Particle position {kind} branch drifted')
        section = section.replace(edge, f'return 0x{branch}u;')
        output.append(f'std::uint32_t {POSITION_REFERENCE}_{kind}(WorldState &s) {{\n'
                      f'{section}\nreturn 0x{fallthrough}u;\n}}\n')
    return ''.join(output)


def install_position(body):
    start, adjusted, stop, copy, adjust = position_parts(body)
    scope = 'halo2::native_particle_draw_position_detail'
    copy_hook = (f'\n    switch({scope}::bridge(s,{scope}::Stage::copy,{POSITION_REFERENCE}_copy)) {{\n'
                 '    case 0x17770bu:goto L17770b;\n    case 0x1777b7u:goto L1777b7;\n    default:break;\n    }\n')
    adjust_hook = (f'\n    switch({scope}::bridge(s,{scope}::Stage::adjust,{POSITION_REFERENCE}_adjust)) {{\n'
                   '    case 0x177759u:goto L177759;\n    case 0x177775u:goto L177775;\n    default:break;\n    }\n')
    return (body[:start] + copy + copy_hook + body[start + len(copy):adjusted]
            + adjust + adjust_hook + ordered_adjustment(body[adjusted + len(adjust):stop])
            + 'L177759: ' + body[stop:])



def visibility_parts(body):
    start = '\nL177594: '
    end = '\nL17764e: '
    if body.count(start) != 1 or body.count(end) != 1:
        raise ValueError('Particle visibility boundaries drifted')
    return body.index(start) + len(start), body.index(end)


def visibility_reference(body, name=VISIBILITY_REFERENCE, replay=True):
    begin, end = visibility_parts(body)
    section = body[begin:end]
    if section.count('goto L1776da;') != 2 or section.count('goto L17764e;') != 1:
        raise ValueError('Particle visibility continuations drifted')
    section = section.replace('goto L1776da;', 'return Resume::copy;')
    section = section.replace('goto L17764e;', 'return Resume::draw;')
    for target in ('167ef0', '184690'):
        child = f'run_{target}(s);'
        if section.count(child) != 1:
            raise ValueError('Particle visibility child route drifted')
        if replay:
            section = section.replace(child, f'halo2::native_object_tick_detail::replay(s,0x{target}u);')
    scope = 'halo2::native_particle_draw_visibility_detail'
    return (f'{scope}::Resume {name}(WorldState &s) {{\n'
            f'using {scope}::Resume;\n{section}\nreturn Resume::draw;\n}}\n')


def install_visibility(body):
    start, _ = visibility_parts(body)
    scope = 'halo2::native_particle_draw_visibility_detail'
    hook = (f'\n    switch({scope}::bridge(s,{VISIBILITY_REFERENCE},\n'
            '        [](auto &state,std::uint32_t target){\n'
            '            if(target==0x167ef0u)run_167ef0(state);\n'
            '            else run_184690(state);\n'
            '        })) {\n'
            f'    case {scope}::Resume::copy:goto L1776da;\n'
            f'    case {scope}::Resume::draw:goto L17764e;\n'
            f'    case {scope}::Resume::retained:break;\n'
            '    }\n')
    return body[:start] + hook + body[start:]



VIEW_INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_view.hpp"\n'
VIEW_REFERENCE = 'native_particle_draw_view_reference_' + ENTRY

def view_site(body, pc):
    pattern = re.compile(r'(?m)(?:if\(s\.services\.instruction\)\{s\.services\.instruction\(0x'
                         + pc + r'u\);\}|s\.instruction_literal\(0x' + pc + r'u\);)')
    match, = pattern.finditer(body)
    return match.start()


def ordered_view_setup(section):
    if section.count('_mm_mul_ss(') != 1:
        raise ValueError('Particle view size multiplication drifted')
    return section.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')


def view_references(body):
    prepare = body[view_site(body, '177376'):view_site(body, '17739f')]
    first = body[view_site(body, '177447'):view_site(body, '17749e')]
    following = first[first.index('L177453: ') + len('L177453: '):]
    scope = 'halo2::native_particle_draw_view_detail'
    options = (
        ('prepare', ordered_view_setup(prepare), {'177447': 'first_view'}, 'matrix'),
        ('first', first, {'1777c4': 'descendants', '1777af': 'skip_view', '17756a': 'visibility'}, 'transform'),
        ('next', following, {'1777af': 'skip_view', '17756a': 'visibility'}, 'transform'),
    )
    result = []
    for kind, section, edges, fallthrough in options:
        for pc, edge in edges.items():
            goto = f'goto L{pc};'
            if section.count(goto) != 1:
                raise ValueError(f'Particle view {kind} edge {pc} drifted')
            section = section.replace(goto, f'return Resume::{edge};')
        result.append(f'{scope}::Resume {VIEW_REFERENCE}_{kind}(WorldState &s) {{\n'
                      f'using {scope}::Resume;\n{section}return Resume::{fallthrough};\n}}\n')
    return ''.join(result)


def install_view(body):
    scope = 'halo2::native_particle_draw_view_detail'
    setup = view_site(body, '177376')
    matrix = view_site(body, '17739f')
    if body[setup - len('L177376: '):setup] != 'L177376: ':
        raise ValueError('Native view entry label missing')
    options = (
        ('prepare', setup, (('matrix', 'L17739f_view_matrix'), ('first_view', 'L177447'))),
        ('first', view_site(body, '177447'), (('descendants', 'L1777c4'), ('skip_view', 'L1777af'),
                    ('visibility', 'L17756a'), ('transform', 'L17749e_view_transform'))),
        ('next', view_site(body, '177453'), (('skip_view', 'L1777af'),
                    ('visibility', 'L17756a'), ('transform', 'L17749e_view_transform'))),
    )
    patches = [(setup, matrix, ordered_view_setup(body[setup:matrix]) + 'L17739f_view_matrix: '),
               (view_site(body, '17749e'), view_site(body, '17749e'), 'L17749e_view_transform: ')]
    for stage, site, arms in options:
        cases = ''.join(f'    case {scope}::Resume::{edge}:goto {label};\n' for edge, label in arms)
        hook = (f'\n    switch({scope}::bridge(s,{scope}::Stage::{stage},{VIEW_REFERENCE}_{stage})) {{\n'
                f'{cases}    default:break;\n    }}\n')
        patches.append((site, site, hook))
    for start, end, insert in sorted(patches, reverse=True):
        body = body[:start] + insert + body[end:]
    return body



DISTANCE_INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_distance.hpp"\n'
DISTANCE_REFERENCE = 'native_particle_draw_distance_reference_' + ENTRY

def distance_part(body):
    start, stop = '\nL17764e: ', '\nL1776da: '
    if body.count(start) != 1 or body.count(stop) != 1:
        raise ValueError('Particle draw distance boundaries drifted')
    return body.index(start) + len(start), body.index(stop)

def distance_reference(body, name=DISTANCE_REFERENCE, replay=True):
    begin, end = distance_part(body)
    section = body[begin:end]
    if section.count('run_252ec0(s);') != 1 or section.count('native_single_spill(') != 1:
        raise ValueError('Particle draw distance call and spill sites drifted')
    if replay:
        section = section.replace('run_252ec0(s);', 'halo2::native_object_tick_detail::replay(s,0x252ec0u);')
    return f'void {name}(WorldState &s) {{\n{section}\n}}\n'

def install_distance(body):
    start, _ = distance_part(body)
    hook = ('\n    if(halo2::native_particle_draw_distance_detail::bridge(s,'
            + DISTANCE_REFERENCE + ',\n'
            '        [](auto &state){run_252ec0(state);},native_single_spill))goto L1776da;\n')
    return body[:start] + hook + body[start:]



ROUTING_INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_routing.hpp"\n'
ROUTING_REFERENCE = 'native_particle_draw_routing_reference_' + ENTRY
ROUTING_SEGMENTS = (
    ('input', 'visibility_input', '17756a', '177594', (('visibility', '177594'),)),
    ('skip', 'skip_view', '1777af', '1777c4', (('next_view', '177453'), ('descendants', '1777c4'))),
    ('advance', 'advance_view', '1777b7', '1777c4', (('next_view', '177453'), ('descendants', '1777c4'))),
)


def routing_sites(body, start, end):
    label, stop = f'\nL{start}: ', f'\nL{end}: '
    if body.count(label) != 1 or body.count(stop) != 1:
        raise ValueError('Particle routing continuation labels drifted')
    return body.index(label) + len(label), body.index(stop)


def routing_references(body, replay=True):
    result = []
    scope = 'halo2::native_particle_draw_routing_detail'
    for name, _, first, last, branches in ROUTING_SEGMENTS:
        begin, end = routing_sites(body, first, last)
        section = body[begin:end]
        goto = f'goto L{branches[0][1]};'
        if section.count(goto) != 1:
            raise ValueError('Particle routing edge drifted')
        section = section.replace(goto, f'return Resume::{branches[0][0]};')
        if name == 'input':
            call = 'dispatch(s,target,0x17756eu,false);'
            if section.count(call) != 1:
                raise ValueError('Particle routing visibility query drifted')
            if replay:
                section = section.replace(call, 'halo2::native_object_tick_detail::replay(s,target);')
        reference = ROUTING_REFERENCE + '_' + name + ('' if replay else '_live')
        result.append(f'{scope}::Resume {reference}(WorldState &s) {{\n'
                      f'using {scope}::Resume;\n{section}\nreturn Resume::{branches[-1][0]};\n}}\n')
    return ''.join(result)


def install_routing(body):
    scope = 'halo2::native_particle_draw_routing_detail'
    for name, stage, first, last, branches in ROUTING_SEGMENTS:
        begin, _ = routing_sites(body, first, last)
        arms = ''.join(f'    case {scope}::Resume::{resume}:goto L{label};\n' for resume, label in branches)
        dispatch = ('[](auto &state){dispatch(state,0x3eb10u,0x17756eu,false);}' if name == 'input'
                    else '[](auto &){throw std::logic_error("Particle view advance has no query");}')
        hook = (f'\n    switch({scope}::bridge(s,{scope}::Stage::{stage},{ROUTING_REFERENCE}_{name},\n'
                f'        {dispatch})) {{\n{arms}    default:break;\n    }}\n')
        body = body[:begin] + hook + body[begin:]
    return body


SUBTREE_INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_subtree.hpp"\n'
SUBTREE_REFERENCE = 'native_particle_draw_subtree_reference_' + ENTRY
SUBTREE_ANCHOR = '\nL1777c4: '

def subtree_part(body):
    if body.count(SUBTREE_ANCHOR) != 1 or not body.endswith('\n}'):
        raise ValueError('Particle draw descendant boundary drifted')
    return body.index(SUBTREE_ANCHOR) + len(SUBTREE_ANCHOR)

def subtree_reference(body, name=SUBTREE_REFERENCE, replay=True):
    section = body[subtree_part(body):-2]
    call = 'run_1772c0(s);'
    if section.count(call) != 1 or section.count('goto L1777d0;') != 1:
        raise ValueError('Particle draw child loop drifted')
    if replay:
        section = section.replace(call, 'halo2::native_object_tick_detail::replay(s,0x1772c0u);')
    return f'void {name}(WorldState &s) {{\n{section}\n}}\n'

def install_subtree(body):
    start = subtree_part(body)
    hook = ('\n    if(halo2::native_particle_draw_subtree_detail::bridge(s,'
            + SUBTREE_REFERENCE + ',[](auto &state){run_1772c0(state);}))return;\n')
    return body[:start] + hook + body[start:]



MATRIX_INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_matrix.hpp"\n'
MATRIX_REFERENCE = 'native_particle_draw_matrix_reference_' + ENTRY
MATRIX_SEGMENTS = (
    ('basis', '17739f', 'L17739f_view_matrix', '177447', '177447', 6, 0),
    ('transform', '17749e', 'L17749e_view_transform', '17756a', '17770b', 9, 9),
)

def matrix_order(section, multiplications, additions):
    operations = (
        ('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(', multiplications),
        ('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(', additions),
    )
    for old, new, count in operations:
        if section.count(old) != count:
            raise ValueError('Particle draw matrix operation drifted')
        section = section.replace(old, new)
    return section

def matrix_references(body):
    result = []
    for kind, start, _, end, next_pc, mul, add in MATRIX_SEGMENTS:
        section = matrix_order(body[view_site(body, start):body.index('\nL' + end + ': ')], mul, add)
        if kind == 'transform':
            edge = 'goto L' + next_pc + ';'
            if section.count(edge) != 1:
                raise ValueError('Particle view transform continuation drifted')
            section = section.replace(edge, 'return;')
        result.append(f'void {MATRIX_REFERENCE}_{kind}(WorldState &s) {{\n{section}\n}}\n')
    return ''.join(result)

def install_matrix(body):
    scope = 'halo2::native_particle_draw_matrix_detail'
    for kind, start, label, end, next_pc, mul, add in MATRIX_SEGMENTS:
        begin = view_site(body, start)
        stop = body.index('\nL' + end + ': ', begin)
        if body[begin-len(label)-2:begin] != label + ': ':
            raise ValueError('Native particle matrix continuation label missing')
        hook = f'\n    if({scope}::bridge(s,{scope}::Stage::{kind},{MATRIX_REFERENCE}_{kind}))goto L{next_pc};\n'
        body = body[:begin] + hook + matrix_order(body[begin:stop], mul, add) + body[stop:]
    return body


SUBMISSION_INCLUDE = '#include "rendering/effects/native_tu15_particle_draw_submission.hpp"\n'
SUBMISSION_REFERENCE = 'native_particle_draw_submission_reference_' + ENTRY
SUBMISSION_SEGMENTS = (('transform', '177759'), ('submit', '177775'))

def submission_references(body, replay=True):
    result = []
    end = body.index('\nL1777af: ')
    calls = {'run_1e4c0(s);': '0x1e4c0u',
             'dispatch(s,target,0x17777fu,false);': '0x178070u',
             'dispatch(s,target,0x1777a2u,false);': '0x40e20u'}
    for kind, pc in SUBMISSION_SEGMENTS:
        section = body[view_site(body, pc):end]
        if section.count('goto L1777b7;') != 2:
            raise ValueError('Particle draw submission exit drifted')
        section = section.replace('goto L1777b7;', 'return;')
        wanted = calls if kind == 'transform' else dict(list(calls.items())[1:])
        for call, target in wanted.items():
            if section.count(call) != 1:
                raise ValueError(f'Particle draw submission call {target} drifted')
            if replay:
                section = section.replace(call, f'halo2::native_object_tick_detail::replay(s,{target});')
        name = SUBMISSION_REFERENCE + '_' + kind + ('' if replay else '_live')
        result.append(f'void {name}(WorldState &s) {{\n{section}\n}}\n')
    return ''.join(result)

def install_submission(body):
    scope = 'halo2::native_particle_draw_submission_detail'
    for kind, pc in SUBMISSION_SEGMENTS:
        label = f'L{pc}: '
        start = view_site(body, pc)
        if body[start-len(label):start] != label:
            raise ValueError('Native draw submission label missing')
        hook = (f'\n    if({scope}::bridge(s,{scope}::Stage::{kind},{SUBMISSION_REFERENCE}_{kind},'
                '\n        [](auto &state,std::uint32_t target,std::uint32_t site){'
                '\n            if(target==0x1e4c0u)run_1e4c0(state);'
                '\n            else dispatch(state,target,site,false);'
                '\n        }))goto L1777b7;\n')
        body = body[:start] + hook + body[start:]
    return body


def install(canonical, output):
    world = world_helper()
    if not admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREFIX in current[0] or current[0].count('\nL1777c4: ') != 1:
        return output, []
    entry = current[0].index('{') + 1
    text, count = DRAW_SITE.subn(r'L177376: \1', install_visibility(install_position(current[0])))
    if count != 1:
        return output, []
    text = install_view(text)
    text = install_distance(text)
    text = install_subtree(install_routing(text))
    text = install_submission(install_matrix(text))
    text = text[:entry] + PREFIX + text[entry:]
    original = world.body(canonical, ENTRY)[0]
    output = output[:current.start()] + (reference(original) + position_references(original) + visibility_reference(original)
              + view_references(original) + distance_reference(original) + routing_references(original) + subtree_reference(original)
              + matrix_references(original) + submission_references(original) + '\n' + text) + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    if POSITION_INCLUDE not in output:
        output = POSITION_INCLUDE + output
    if VISIBILITY_INCLUDE not in output:
        output = VISIBILITY_INCLUDE + output
    if VIEW_INCLUDE not in output:
        output = VIEW_INCLUDE + output
    if DISTANCE_INCLUDE not in output:
        output = DISTANCE_INCLUDE + output
    if ROUTING_INCLUDE not in output:
        output = ROUTING_INCLUDE + output
    if SUBTREE_INCLUDE not in output:
        output = SUBTREE_INCLUDE + output
    if MATRIX_INCLUDE not in output:
        output = MATRIX_INCLUDE + output
    if SUBMISSION_INCLUDE not in output:
        output = SUBMISSION_INCLUDE + output
    return output, [ENTRY]
