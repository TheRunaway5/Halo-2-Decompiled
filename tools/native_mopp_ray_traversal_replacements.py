"""Authenticate the TU1.5 MOPP entry tranche and the private full-controller overlay."""
import hashlib
import importlib.util
from pathlib import Path
import re

_spec=importlib.util.spec_from_file_location('_mopp_ray_owner_functions',Path(__file__).with_name('native_model_marker_lookup_replacements.py'))
_shared=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_shared)
function=_shared.function
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
BODY=re.compile(r'static void run_30aa70\(WorldState &s\) \{.*?\n\}',re.S)
PINS={
    'ai':{'0965ba2dfbc6d947bea64e48f8224b3f85d36efbe9c39739f46d7ec012d0064d',
          '33e3dbf70856230662c38036abc235fc57cba33a544e9d4eb746e1853e4ec8f7'},
    # 7705b4f4...: the Death body after its four FSTP ST(0) sites (30AE00,
    # 30B2C7, 30B6AB, 30B6AD) gained the C1 clear the AI-frame copy and the
    # hardware already have (docs/native_replacement/16C620_collision_ray.md).
    'death':{'f5d6ae21129685622ea289febf4dae15317283b7cfb2c14c299d3853fc25070e',
             '0073fcd26126caf3b2005b45d23e2b213819144739a1da60a9acc0ff8036570d',
             '7705b4f435c057f959617f5ef867fce610b934cec04ce7ec1a1f2d7c1c10e333'},
}
# Full resolver, dispatcher and complete LiveState. The two original bodies
# differ at94 explicit legacy C1 clears: never normalize or conflate them.
CONTEXTS={
    ('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
     '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
     'c3e351629369a688a5ca40be8dfcaf725482d008a3050759060cacd2a2bed275'):'ai',
    ('cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61',
     '2ddcb4ca932da7abcdbbbf8050bead73a3a18e4a4153021913f0555281b8aa20',
     '68accf0054ca64fec14dc244a1c1d00e6015e79ef2e0182717e8b2fe4cf99fb3'):'death',
}
INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_traversal.hpp"\n'
PREFIX='\n    if(halo2::native_mopp_ray_traversal_detail::bridge(s,[](auto&child,auto target,auto site){dispatch(child,target,site,false);}))return;'
def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body
def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete MOPP ray owner')
    match=found[0];body=canonical_body(match[0]);owner=re.search(r'^struct LiveState :.*?^\};',source,re.M|re.S)
    if not owner or source.count('using WorldState=LiveState;')!=1:raise ValueError('Complete MOPP ray State ownership drift')
    resolver=function(source,'selected');dispatcher=function(source,'dispatch')
    context=(digest(resolver),digest(dispatcher),digest(owner[0]))
    if context not in CONTEXTS:raise ValueError('Complete MOPP ray resolver/dispatcher/State context drift')
    kind=CONTEXTS[context]
    if digest(body)not in PINS[kind]or function(source,'run_30aa70')!=match[0]:raise ValueError('Complete MOPP ray body drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{context[0]})
    return match,body,kind
def replace_mopp_ray_traversal(source):
    if not BODY.search(source):
        if INCLUDE in source or PREFIX in source or re.search(r'\brun_30aa70\s*\(',source):raise ValueError('MOPP ray reference/overlay has no complete owner')
        return source
    match,body,_=authenticate(source)
    if body==match[0]:
        start=body.index('{')+1;source=source[:match.start()]+body[:start]+PREFIX+body[start:]+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate MOPP ray native include')
    return source if INCLUDE in source else INCLUDE+source


# Prepared AI contains a nested closing brace introduced by grouped POPs.
# The balanced function reader authenticates the full owner in both units.
PREPARED_ENTRY_PINS={
    'death':'33b98bcaa96fc76dd2ba68a2ed1c9b8a100be0c20d43d578111c1f4fa29de110',
    'ai':'c4d82d4eec1bd2487661454fe1779202a1a64592b04685269276d02a47f40ed3',
}
ENTRY_ROUTES = """
    {
        using namespace halo2::native_mopp_ray_traversal_detail;
        switch(entry_prefix<EntryOwner::OWNER>(s)) {
        case EntryContinuation::classify: goto L30aaa0;
        case EntryContinuation::diagnostic: goto L30ae16;
        case EntryContinuation::retained: break;
        }
    }"""


DISPATCH_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_dispatch.hpp"\n'
DISPATCH_ARMS={
    '30b328':'if(halo2::native_mopp_ray_dispatch_detail::jump_case<1>(s))goto L30ae02;',
    '30b335':'if(halo2::native_mopp_ray_dispatch_detail::jump_case<2>(s))goto L30ae02;',
    '30b34b':'if(halo2::native_mopp_ray_dispatch_detail::jump_case<3>(s))goto L30ae02;',
    '30b6af':'if(halo2::native_mopp_ray_dispatch_detail::empty_case(s))return;',
}


DISPATCH_TABLES_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_dispatch_tables.hpp"\n'
DISPATCH_CANONICAL_PINS={
    'death':'d08a432fe31561c0557f7f3b512be667a2787165b6f36e165194b9f05e3caa24',
    'ai':'2175819fb3f46bc16a00b7416a04d5f1464ac5ab060c1791190e9db9a8192470',
}


def install_dispatch_tables(body, canonical, owner):
    index='s.instruction_literal(0x30aaa0u); s.set(1,r(s.memory,(0x30b738u+s.gp[0]),1),4,0);'
    target='s.instruction_literal(0x30aaa7u); '
    fetch='canonical_branch(r(s.memory,(0x30b6b8u+s.gp[1]*4u),4),s.image)'
    entry=f'L30aaa0: {index}\n{target}switch({fetch}){{'
    error='default:throw std::runtime_error("Required original solver dispatch target unavailable");}'
    if body.count(entry)!=1:
        raise ValueError('Prepared MOPP ray table access drift')
    start=body.index(entry)
    stop=body.index('\n',start+len(entry))
    arms=body[start+len(entry):stop]
    targets=re.findall(r'case (0x[0-9a-f]+u):',arms)
    # TU1.5's authentic common dispatcher maps all these literals to
    # themselves. Other labels preserve its original two error cases.
    if (digest(canonical)!=DISPATCH_CANONICAL_PINS[owner] or len(targets)!=32 or
        not arms.endswith(error) or
        any(canonical.count(f'case {label}:return {label};')!=1 for label in targets)):
        raise ValueError('Prepared MOPP ray canonical target ownership drift')
    arms=arms[:-len(error)]+'default:if(native_tables)(void)canonical_branch(table_target,s.image);'+error[8:]
    bridge=(
        'L30aaa0: { std::uint32_t table_target; '
        'const auto native_tables=halo2::native_mopp_ray_dispatch_detail::read_dispatch_tables(s,table_target); '
        f'if(!native_tables) {{ {index}\n{target}table_target={fetch}; }}\n'
        'switch(table_target){'+arms+' }')
    return body[:start]+bridge+body[stop:]


SCALE_INPUTS_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_scale_inputs.hpp"\n'
SCALE_INPUTS_CONTINUATIONS={
    'load_endpoint':'30b372', 'read_final_component':'30b379', 'spill_components':'30b37d',
}
SCALE_INPUTS_REFERENCE=(
    'L30b36a: s.instruction_literal(0x30b36au); s.set(1,r(s.memory,(0x1u+s.gp[6]),1),4,0);\n'
    's.instruction_literal(0x30b36eu); s.set(2,r(s.memory,(0x2u+s.gp[6]),1),4,0);\n'
    's.instruction_literal(0x30b372u); s.xmm[0]=v(s.memory,(0x0u+s.gp[7]));\n'
    's.instruction_literal(0x30b375u); w(s.memory,s.observe,0x30b375u,(0x14u+s.gp[4]),4,s.reg(1,4,0));\n'
    's.instruction_literal(0x30b379u); s.set(1,r(s.memory,(0x3u+s.gp[6]),1),4,0);\n'
    's.instruction_literal(0x30b37du);')


def install_scale_inputs(body):
    if body.count(SCALE_INPUTS_REFERENCE)!=1:
        raise ValueError('Prepared MOPP ray scale input sequence drift')
    entry='L30b36a: '
    routes=' '.join(f'case Continuation::{name}: goto L_native_scale_input_{pc};'
                    for name,pc in SCALE_INPUTS_CONTINUATIONS.items())
    hook=('{ using namespace halo2::native_mopp_ray_scale_inputs_detail; '
          'switch(read_scale_inputs(s)) { case Continuation::original: break; '
          +routes+' } } ')
    body=body.replace(entry,entry+hook,1)
    for pc in SCALE_INPUTS_CONTINUATIONS.values():
        literal=f's.instruction_literal(0x{pc}u);'
        if body.count(literal)!=1:
            raise ValueError('Prepared MOPP ray scale input continuation drift: '+pc)
        body=body.replace(literal,f'L_native_scale_input_{pc}: '+literal,1)
    return body


SCALE_TRANSLATION_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_scale_translation.hpp"\n'
SCALE_TRANSLATION_CONTINUATIONS={
    'original':'30b37d', 'constant_one':'30b385', 'zero_lane':'30b38e',
    'read_y':'30b396', 'scale_shift':'30b39e', 'read_z':'30b3a6', 'first_vector':'30b3ae',
}


def install_scale_translation(body):
    for route,pc in SCALE_TRANSLATION_CONTINUATIONS.items():
        before=f's.instruction_literal(0x{pc}u);'
        if body.count(before)!=1:
            raise ValueError('Prepared MOPP ray scale continuation drift: '+pc)
        cases=f'L_native_scale_{pc}: '
        if route=='original':
            arms=' '.join(f'case Continuation::{name}: goto L_native_scale_{pc};'
                          for name,pc in SCALE_TRANSLATION_CONTINUATIONS.items())
            cases=('{ using namespace halo2::native_mopp_ray_scale_translation_detail; '
                   'switch(prepare(s,native_single_spill)) { '+arms+' } }\n'+cases)
        body=body.replace(before,cases+before,1)
    return body


OPCODE_RESUME_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_opcode_resume.hpp"\n'
OPCODE_RESUME_ROUTES=(
    '{ using namespace halo2::native_mopp_ray_opcode_resume_detail; switch(resume(s)) { '
    'case Continuation::classify: goto L30aaa0; '
    'case Continuation::diagnostic: goto L30ae16; '
    'case Continuation::store_default: goto L_native_opcode_default; '
    'case Continuation::branch_default: goto L_native_opcode_branch; '
    'case Continuation::read_opcode: goto L_native_opcode_read; '
    'case Continuation::original: break; } } ')


def install_opcode_resume(body):
    anchor='L30ae02: '
    store='s.instruction_literal(0x30ae08u);'
    branch='s.instruction_literal(0x30ae10u);'
    if body.count(anchor)!=1 or body.count(store)!=1 or body.count(branch)!=1:
        raise ValueError('Prepared MOPP opcode resume boundary drift')
    pop='L30ae00: '
    opcode='s.instruction_literal(0x30ae02u);'
    if body.count(pop)!=1 or body.count(opcode)!=1:
        raise ValueError('Prepared MOPP resume x87 pop boundary drift')
    return (body.replace(anchor,anchor+OPCODE_RESUME_ROUTES,1)
            .replace(pop,pop+OPCODE_RESUME_ROUTES.replace('resume(s)','resume<true>(s)'),1)
            .replace(opcode,'L_native_opcode_read: '+opcode,1)
            .replace(store,'L_native_opcode_default: '+store,1)
            .replace(branch,'L_native_opcode_branch: '+branch,1))


SCALE_VECTORS_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_scale_vectors.hpp"\n'
SCALE_VECTOR_CONTINUATIONS={
    'original':'30b3ae','read_end':'30b3b9','read_factor':'30b3c1','read_start':'30b3cc',
    'reload_factor':'30b3d3','read_end_scaled':'30b3e6','read_origin':'30b3f8','scale_tail':'30b40d',
}


def install_scale_vectors(body):
    # Preserve the original destructive SSE operand order for NaN payloads
    # in both the native route and every retained continuation.
    for pc,reg,op in (('30b3e0',3,'mul'),('30b3f1',3,'mul'),('30b3fb',2,'add'),('30b405',1,'mul')):
        site=f's.instruction_literal(0x{pc}u); s.xmm[{reg}]=_mm_{op}_ps('
        if body.count(site)!=1:
            raise ValueError('Prepared MOPP packed scale arithmetic drift: '+pc)
        body=body.replace(site,site.replace(f'_mm_{op}_ps(',f'halo2::native_tu15_leaves::ordered_{op}_ps('),1)
    for name,pc in SCALE_VECTOR_CONTINUATIONS.items():
        before=f's.instruction_literal(0x{pc}u);'
        if body.count(before)!=1:
            raise ValueError('Prepared MOPP packed scale continuation drift: '+pc)
        label=f'L_native_scale_vectors_{pc}: '
        if name=='original':
            cases=' '.join(f'case Continuation::{route}: goto L_native_scale_vectors_{target};'
                           for route,target in SCALE_VECTOR_CONTINUATIONS.items())
            label=('{ using namespace halo2::native_mopp_ray_scale_vectors_detail; '
                   'switch(scale_vectors(s,native_single_spill)) { '+cases+' } }\n'+label)
        body=body.replace(before,label+before,1)
    return body


SCALE_TAIL_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"\n'
SCALE_TAIL_CONTINUATIONS={
    'original':'30b40d', 'write_shift':'30b412', 'read_scale':'30b416',
    'read_property':'30b420', 'write_property':'30b423', 'read_key':'30b427',
    'write_key':'30b42a', 'set_variables':'30b42e',
}


def install_scale_tail(body):
    for route,pc in SCALE_TAIL_CONTINUATIONS.items():
        before=f's.instruction_literal(0x{pc}u);'
        if body.count(before)!=1:
            raise ValueError('Prepared MOPP ray scalar scale-tail continuation drift: '+pc)
        label=f'L_native_scale_tail_{pc}: '
        if route=='original':
            arms=' '.join(f'case Continuation::{name}: goto L_native_scale_tail_{next_pc};'
                          for name,next_pc in SCALE_TAIL_CONTINUATIONS.items())
            label=('{ using namespace halo2::native_mopp_ray_scale_tail_detail; '
                   'switch(scale_tail(s,native_single_spill)) { '+arms+
                   ' case Continuation::resume: goto L30ae02; } }\n'+label)
        body=body.replace(before,label+before,1)
    return body


DECREASING_CLIP_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_decreasing_clip.hpp"\n'
DECREASING_CLIP_BLOCKS=(
    ('false','30b022','30b06f',{'ratio':'30b037','reload_fraction':'30b047','read_one':'30b057'}),
    ('true','30b082','30b0d1',{'ratio':'30b097','reload_fraction':'30b0b2','read_start':'30b0c2'}),
)


def decreasing_clip_arithmetic(body, after):
    ops=(('30b086','mul','34'),('30b09f','sub','30'),('30b0aa','divide','34')) if after else (
         ('30b026','mul','2c'),('30b03b','sub','18'),('30b03f','divide','2c'))
    for pc,op,offset in ops:
        literal=f's.instruction_literal(0x{pc}u); '
        operand=f'r(s.memory,(0x{offset}u+s.gp[4]),4)'
        if op=='divide':
            old=literal+f's.fp.st(0)=divide(ext(f({operand})),s.fp.st(0));'
            replacement='halo2::native_mopp_ray_decreasing_clip_detail::reverse_divide(s,'+operand+');'
        else:
            old=literal+f's.fp.st(0)={op}(s.fp.st(0),ext(f({operand})));'
            mode='true' if op=='mul' else 'false'
            replacement=f'halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<{mode}>(s,{operand});'
        if body.count(old)!=1:
            raise ValueError('Prepared MOPP decreasing clip x87 arithmetic drift: '+pc)
        body=body.replace(old,literal+replacement,1)
    sse=(('30b0bf',2,'mul'),('30b0c8',3,'mul'),('30b0cb',3,'add')) if after else (
        ('30b054',3,'mul'),('30b061',2,'mul'),('30b064',2,'add'))
    for pc,reg,op in sse:
        literal=f's.instruction_literal(0x{pc}u); s.xmm[{reg}]=_mm_{op}_ps('
        if body.count(literal)!=1:
            raise ValueError('Prepared MOPP decreasing clip packed arithmetic drift: '+pc)
        body=body.replace(literal,literal.replace(f'_mm_{op}_ps(',f'halo2::native_tu15_leaves::ordered_{op}_ps('),1)
    return body


def install_decreasing_clip(body):
    for side,entry,exit,handoffs in DECREASING_CLIP_BLOCKS:
        body=decreasing_clip_arithmetic(body,side=='true')
        literal=f's.instruction_literal(0x{entry}u);'
        anchor=literal if side=='true' else f'L{entry}: '
        if body.count(anchor)!=1 or body.count(f'L{exit}: ')!=1:
            raise ValueError('Prepared MOPP decreasing clip boundary drift: '+entry)
        jumps=' '.join(f'case Continuation::{name}: goto L_native_clip_{pc};' for name,pc in handoffs.items())
        bridge=('{ using namespace halo2::native_mopp_ray_decreasing_clip_detail; '
                f'switch(decreasing_clip<{side}>(s)) {{ case Continuation::original: break; '
                +jumps+f' case Continuation::done: goto L{exit}; '
                'default: throw std::logic_error("Invalid MOPP decreasing clip continuation"); } } ')
        body=body.replace(anchor,bridge+anchor if side=='true' else anchor+bridge,1)
        for pc in handoffs.values():
            literal=f's.instruction_literal(0x{pc}u);'
            if body.count(literal)!=1:
                raise ValueError('Prepared MOPP decreasing clip handoff drift: '+pc)
            body=body.replace(literal,f'L_native_clip_{pc}: '+literal,1)
    return body


DECREASING_RESUME_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_decreasing_resume.hpp"\n'
DECREASING_RESUME_PHASES = {'fraction': {'entry': '30b0d1',
              'routes': {'read_fraction': '30b0d5', 'shorten': '30b0e9', 'advance': '30b16e'}},
 'shorten': {'entry': '30b0e9',
             'routes': {'read_end': '30b0ec',
                        'read_one': '30b0f0',
                        'read_code_pointer': '30b0f7',
                        'read_code': '30b0fa',
                        'save_fraction': '30b0fd',
                        'refill_fraction': '30b101',
                        'read_start': '30b111',
                        'read_scale': '30b125',
                        'read_short_end': '30b128',
                        'save_scale': '30b12c',
                        'refill_scale': '30b130',
                        'read_axis': '30b136',
                        'read_translation': '30b14b',
                        'read_scaled_end': '30b14e',
                        'branch_axis': '30b159',
                        'read_endpoint': '30b15b',
                        'advance': '30b16e',
                        'empty': '30b6af'}},
 'advance': {'entry': '30b16e', 'routes': {'read_delta': '30b172', 'opcode': '30ae02'}}}
DECREASING_RESUME_ORDERED = {'30b10e': (3, 'mul'), '30b118': (4, 'mul'), '30b11b': (4, 'add'), '30b144': (2, 'mul')}

def install_decreasing_resume(body):
    for pc, (reg, op) in DECREASING_RESUME_ORDERED.items():
        literal = f's.instruction_literal(0x{pc}u); s.xmm[{reg}]=_mm_{op}_ps('
        if body.count(literal) != 1:
            raise ValueError('Prepared MOPP decreasing resume arithmetic drift: ' + pc)
        ordered = literal.replace(f'_mm_{op}_ps(', f'halo2::native_tu15_leaves::ordered_{op}_ps(')
        body = body.replace(literal, ordered, 1)
    for phase, stage in DECREASING_RESUME_PHASES.items():
        entry, routes = stage['entry'], stage['routes']
        retained = {role: pc for role, pc in routes.items()
                    if role not in {'shorten', 'advance', 'empty', 'opcode'}}
        cases = ' '.join(
            f'case Continuation::{role}: goto L_native_decreasing_{pc};' if role in retained
            else f'case Continuation::{role}: goto L{pc};'
            for role, pc in routes.items())
        prefix = ('{ using namespace halo2::native_mopp_ray_decreasing_resume_detail; '
                  f'switch(decreasing_resume<Phase::{phase}>(s)) {{ case Continuation::original: break; '
                  + cases + ' default: throw std::logic_error("Invalid MOPP decreasing resume continuation"); } } ')
        if phase == 'shorten':
            anchor = f's.instruction_literal(0x{entry}u);'
            replacement = f'L{entry}: ' + prefix + anchor
        else:
            anchor = f'L{entry}: '
            replacement = anchor + prefix
        if body.count(anchor) != 1:
            raise ValueError('Prepared MOPP decreasing resume entry drift: ' + entry)
        body = body.replace(anchor, replacement, 1)
        for pc in retained.values():
            literal = f's.instruction_literal(0x{pc}u);'
            if body.count(literal) != 1:
                raise ValueError('Prepared MOPP decreasing resume handoff drift: ' + pc)
            body = body.replace(literal, f'L_native_decreasing_{pc}: ' + literal, 1)
    return body


WIDE_PLANE_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_wide_plane.hpp"\n'


def install_wide_plane(body):
    entry='L30ad4a: '
    resumes={'save_low':'30ad52','read_left_code':'30ad56','read_lower':'30ad5a','save_high':'30ad61',
             'read_right_code':'30ad65','read_upper':'30ad6d','save_axis':'30ad71',
             'advance_program':'30ad75','read_start':'30ad7c','read_end':'30ad7f',
             'read_left_high':'30ad83','save_left':'30ad8c','read_right_high':'30ad90'}
    if body.count(entry)!=1:
        raise ValueError('Prepared MOPP wide plane entry drift')
    jumps=' '.join(f'case Continuation::{route}: goto L_native_wide_plane_{pc};' for route,pc in resumes.items())
    switch=('{ using namespace halo2::native_mopp_ray_wide_plane_detail; '
            'switch(wide_plane(s,native_single_spill)) { case Continuation::original: break; '+jumps+
            ' case Continuation::offsets: goto L30add8; } } ')
    body=body.replace(entry,entry+switch,1)
    for pc in resumes.values():
        before=f's.instruction_literal(0x{pc}u);'
        if body.count(before)!=1:
            raise ValueError('Prepared MOPP wide plane continuation drift: '+pc)
        body=body.replace(before,f'L_native_wide_plane_{pc}: '+before,1)
    return body


AXIS_PLANE_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_axis_plane.hpp"\n'


def install_axis_plane(body):
    stages=(('30ad9b','axis_plane(s,native_single_spill)',
             {'store_lower':'30ada3','adjust_axis':'30ada7','store_axis':'30adaa','read_lower':'30adae',
              'spill_lower':'30adb6','read_upper':'30adba','read_start':'30adc2','read_end':'30adc5'},
             'offsets','30adc9'),
            ('30adc9','plane_offsets(s)',{'clear_left':'30add0','read_left':'30add8','store_right':'30addc'},
             'compare','30ade0'))
    for entry,function,resumes,exit,target in stages:
        anchor=f'L{entry}: '
        if body.count(anchor)!=1:
            raise ValueError('Prepared MOPP axis plane entry drift: '+entry)
        jumps=' '.join(f'case Continuation::{route}: goto L_native_axis_plane_{pc};'
                       for route,pc in resumes.items())
        switch=('{ using namespace halo2::native_mopp_ray_axis_plane_detail; '
                f'switch({function}) {{ case Continuation::original: break; '+jumps+
                f' case Continuation::{exit}: goto L{target}; default: throw std::logic_error("Invalid native axis plane continuation"); }} }} ')
        body=body.replace(anchor,anchor+switch,1)
        for pc in resumes.values():
            before=f's.instruction_literal(0x{pc}u);'
            if body.count(before)!=1:
                raise ValueError('Prepared MOPP axis plane handoff drift: '+pc)
            body=body.replace(before,f'L_native_axis_plane_{pc}: '+before,1)
    shared_tail='L30add8: L_native_axis_plane_30add8: '
    if body.count(shared_tail)!=1:
        raise ValueError('Prepared MOPP shared plane-offset tail drift')
    hook=('{ using namespace halo2::native_mopp_ray_axis_plane_detail; '
          'switch(plane_offsets(s,true)) { '
          'case Continuation::compare: goto L30ade0; '
          'case Continuation::store_right: goto L_native_axis_plane_30addc; '
          'default: break; } } ')
    return body.replace(shared_tail,'L30add8: '+hook+'L_native_axis_plane_30add8: ',1)


COLLECTOR_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_collector.hpp"\n'
COLLECTOR_STAGES=(
    ('select','30b5f5','30b603','L30b624','30b605',{'read_key':'30b5f9','read_filter':'30b5fc'}),
    ('shape_call','30b624','30b632','30b634',None,{'read_collection_table':'30b627',
        'push_shape_output':'30b630','push_shape_key':'30b631','set_shape_receiver':'30b632'}),
    ('shape_result','30b637','30b642','L30b671','30b644',{'read_output':'30b63b','read_shape_table':'30b640'}),
    ('collector_call','30b671','30b692','30b696',None,{'save_parent':'30b674','read_parent_value':'30b678',
        'save_parent_value':'30b67b','read_collector':'30b67f','push_collector':'30b682',
        'resolve_query':'30b683','push_query':'30b687','resolve_ray':'30b688','push_ray':'30b68b',
        'set_collector_receiver':'30b68c','save_shape':'30b68e','save_key':'30b692'}),
    ('collector_result','30b699','30b69f','30b6a2',None,{'read_fraction':'30b69c'}),
)


def install_collector(body):
    def place(text,pc,addition):
        token=f's.instruction_literal(0x{pc}u);'
        if text.count(token)!=1:
            raise ValueError('Prepared MOPP collector boundary drift: '+pc)
        return text.replace(token,addition+token,1)
    for part,entry,_,done,alternate,prefix in COLLECTOR_STAGES:
        exits=dict(prefix)
        if not done.startswith('L'):
            exits['done']=done
        if alternate:
            exits['alternate']=alternate
        cases=' '.join(f'case Continuation::{name}: goto L_native_collector_{pc};' for name,pc in exits.items())
        if done.startswith('L'):
            cases+=f' case Continuation::done: goto {done};'
        hook=('{ using namespace halo2::native_mopp_ray_collector_detail; '
              f'switch(collector<Part::{part}>(s)) {{ case Continuation::original: break; '
              +cases+' default: throw std::logic_error("Invalid MOPP collector continuation"); } } ')
        body=place(body,entry,hook)
        for pc in exits.values():
            body=place(body,pc,f'L_native_collector_{pc}: ')
    for shape,pc,returned,offset in (('true','30b634','30b637','34'),('false','30b696','30b699','24')):
        literal=(f's.instruction_literal(0x{pc}u); {{const auto target=r(s.memory,'
                 f'(0x{offset}u+s.gp[2]),4);s.push(0x{pc}u,0x{returned}u);dispatch(s,target,0x{pc}u,false);}}')
        if body.count(literal)!=1:
            raise ValueError('Prepared MOPP collector call drift: '+pc)
        hook=f'if(!halo2::native_mopp_ray_collector_detail::dispatch_call<{shape}>(s,dispatch)) '
        body=body.replace(literal,hook+'{ '+literal+' }',1)
    return body


PLANE_SPLIT_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_plane_split.hpp"\n'


def install_plane_split(body):
    entry='L30ae55: '
    start='s.instruction_literal(0x30ae66u);'
    if body.count(entry)!=1 or body.count(start)!=1:
        raise ValueError('Prepared MOPP ray plane split boundary drift')
    routes=('{ using namespace halo2::native_mopp_ray_plane_split_detail; '
            'switch(plane_split(s)) { '
            'case Continuation::original: break; '
            'case Continuation::read_start: goto L_native_plane_split_start; '
            'case Continuation::geometry: goto L30ae75; '
            'case Continuation::resume: goto L30ae00; } } ')
    return body.replace(entry,entry+routes,1).replace(start,'L_native_plane_split_start: '+start,1)


PLANE_COMPARE_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_plane_compare.hpp"\n'


def install_plane_compare(body):
    entry='L30ade0: '
    reload='s.instruction_literal(0x30ade4u);'
    if body.count(entry)!=1 or body.count(reload)!=1:
        raise ValueError('Prepared MOPP ray plane comparison boundary drift')
    routes=('{ using namespace halo2::native_mopp_ray_plane_compare_detail; '
            'switch(plane_compare(s,native_single_spill)) { '
            'case Continuation::original: break; '
            'case Continuation::read_end: goto L_native_plane_compare_reload; '
            'case Continuation::split: goto L30ae55; '
            'case Continuation::resume: goto L30ae02; } } ')
    return body.replace(entry,entry+routes,1).replace(reload,'L_native_plane_compare_reload: '+reload,1)


RECURSIVE_ARGUMENTS_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_recursive_arguments.hpp"\n'
RECURSIVE_ARGUMENT_BLOCKS=(
    ('true','30af16','30af28','30af2d',{'first':'30af1f','advance_code':'30af20',
        'second':'30af22','read_cache':'30af23','third':'30af27'}),
    ('false','30b06f','30b07d','30b082',{'first':'30b07a','second':'30b07b','third':'30b07c'}),
)


def install_recursive_arguments(body):
    for increasing,entry,site,returned,fallback in RECURSIVE_ARGUMENT_BLOCKS:
        anchor=f'L{entry}: '
        literal=f's.instruction_literal(0x{site}u); s.push(0x{site}u,0x{returned}u);run_30aa70(s);'
        if body.count(anchor)!=1 or body.count(literal)!=1:
            raise ValueError('Prepared MOPP recursive argument boundary drift: '+site)
        jumps=' '.join(f'case Continuation::{route}: goto L_native_arguments_{pc};'
                       for route,pc in fallback.items())
        bridge=('{ using namespace halo2::native_mopp_ray_recursive_arguments_detail; '
                f'switch(publish_arguments<{increasing}>(s)) {{ case Continuation::original: break; '
                +jumps+f' case Continuation::return_address: goto L_native_arguments_{site}; '
                f'case Continuation::invoke: goto L_native_arguments_invoke_{site}; '
                'default: throw std::logic_error("Invalid MOPP recursive argument continuation"); } } ')
        body=body.replace(anchor,anchor+bridge,1)
        for pc in fallback.values():
            token=f's.instruction_literal(0x{pc}u);'
            if body.count(token)!=1:
                raise ValueError('Prepared MOPP recursive argument fallback drift: '+pc)
            body=body.replace(token,f'L_native_arguments_{pc}: '+token,1)
        split=literal.replace('run_30aa70(s);',f'\nL_native_arguments_invoke_{site}: run_30aa70(s);')
        body=body.replace(literal,f'L_native_arguments_{site}: '+split,1)
    return body


KEY_COPY_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_key_copy.hpp"\n'
KEY_COPY_HANDOFFS={
    'read_vector':'30b4e7','read_shift':'30b4ef','write_shift':'30b4f2',
    'read_scale':'30b4f6','write_scale':'30b4f9','read_old_key':'30b4fd',
    'write_old_key':'30b500','read_property':'30b504','write_property':'30b507',
    'set_cache':'30b50b','advance_key':'30b513',
}
OFFSET_KEY_COPY_HANDOFFS={
    'read_vector':'30b48c','read_shift':'30b494','write_shift':'30b497',
    'read_scale':'30b49b','write_scale':'30b49e','read_old_key':'30b4a2',
    'write_old_key':'30b4a5','read_property':'30b4a9','write_property':'30b4ac',
    'set_cache':'30b4b0','advance_key':'30b4b8',
}


def install_key_copy(body):
    entry='L30b4c0: '
    tail='L30b50f: '
    offset='L30b477: '
    if body.count(entry)!=1 or body.count(tail)!=1 or body.count(offset)!=1 or body.count('L30b4b4: ')!=1:
        raise ValueError('Prepared MOPP key-copy boundary drift')
    jumps=' '.join(f'case Continuation::{name}: goto L_native_key_copy_{pc};'
                   for name,pc in KEY_COPY_HANDOFFS.items())
    prefix=('{ using namespace halo2::native_mopp_ray_key_copy_detail; '
            'switch(key_copy(s)) { case Continuation::original: break; '
            +jumps+' case Continuation::write_key: goto L30b50f; '
            'case Continuation::resume: goto L30ae02; } } ')
    body=body.replace(entry,entry+prefix,1)
    tail_hook=('{ using namespace halo2::native_mopp_ray_key_copy_detail; '
               'switch(key_copy<true>(s)) { '
               'case Continuation::advance_key: goto L_native_key_copy_30b513; '
               'case Continuation::resume: goto L30ae02; default: break; } } ')
    body=body.replace(tail,tail+tail_hook,1)
    jumps=' '.join(f'case Continuation::{name}: goto L_native_key_copy_{pc};'
                   for name,pc in OFFSET_KEY_COPY_HANDOFFS.items())
    offset_hook=('{ using namespace halo2::native_mopp_ray_key_copy_detail; '
                 'switch(key_copy<false,true>(s)) { case Continuation::original: break; '
                 +jumps+' case Continuation::write_key: goto L30b4b4; '
                 'case Continuation::resume: goto L30ae02; } } ')
    body=body.replace(offset,offset+offset_hook,1)
    for pc in (*KEY_COPY_HANDOFFS.values(), *OFFSET_KEY_COPY_HANDOFFS.values()):
        literal=f's.instruction_literal(0x{pc}u);'
        if body.count(literal)!=1:
            raise ValueError('Prepared MOPP key-copy provider handoff drift: '+pc)
        body=body.replace(literal,f'L_native_key_copy_{pc}: '+literal,1)
    return body


def install_shared_return(body):
    reject='L30b6ab: '
    if body.count(reject)!=1:
        raise ValueError('Prepared MOPP x87 rejection return drift')
    routes=('{ using namespace halo2::native_mopp_ray_shared_return_detail; '
            'switch(rejection(s)) { case Rejection::original: break; '
            'case Rejection::frame: goto L_native_mopp_return; case Rejection::complete: return; } } ')
    body=body.replace(reject,reject+routes,1)
    for entry,exit in (('30b668','30b66e'),('30b6a2','30b6a8'),('30b6af','30b6b5')):
        literal=f's.instruction_literal(0x{entry}u);'
        if body.count(literal)!=1:
            raise ValueError('Prepared MOPP ordinary return drift: '+entry)
        bridge=f'if(halo2::native_mopp_ray_shared_return_detail::frame<0x{exit}u>(s))return; '
        if entry=='30b6af':bridge+='L_native_mopp_return: '
        body=body.replace(literal,bridge+literal,1)
    return body


SLAB_BOUNDS_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_slab_bounds.hpp"\n'


def install_slab_bounds(body):
    # These are the six original x87 m32 operations. Keep C1 from the
    # actual instruction in the retained fallback as in the native stage.
    for pc,operation in (('30b1a6','mul'),('30b1bb','mul'),('30b1c6','sub'),
                        ('30b1d0','mul'),('30b1d3','mul'),('30b1d6','sub')):
        site=f's.instruction_literal(0x{pc}u); s.fp.st(0)={operation}(s.fp.st(0),ext(f('
        pattern=re.compile(re.escape(site)+r'(r\([^\n]*\))\)\)\);')
        matches=list(pattern.finditer(body))
        if len(matches)!=1:
            raise ValueError('Prepared MOPP slab m32 operation drift: '+pc)
        match=matches[0]
        mode='true' if operation=='mul' else 'false'
        replacement=(f's.instruction_literal(0x{pc}u); '
                     f'halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<{mode}>(s,{match[1]});')
        body=body[:match.start()]+replacement+body[match.end():]
    stages=(
        ('30b1db','false',{'store_low':'30b1e3','short_adjust_axis':'30b1e7','short_read_lower':'30b1ed',
                         'store_high':'30b1f1','read_upper':'30b1f5'}),
        ('30b17d','true',{'store_low':'30b197','wide_cache_low':'30b19b','wide_read_lower':'30b1a2',
                        'wide_scale_cache_low':'30b1a6','wide_read_left_code':'30b1a9',
                        'wide_read_right_code':'30b1b2','wide_scale_vars_low':'30b1bb',
                        'store_high':'30b1be','wide_cache_high':'30b1c2','wide_translate_low':'30b1c6',
                        'read_upper':'30b1cc','wide_scale_cache_high':'30b1d0',
                        'wide_scale_vars_high':'30b1d3','wide_translate_high':'30b1d6'}))
    for entry,wide,resumes in stages:
        anchor=f'L{entry}: '
        if body.count(anchor)!=1:
            raise ValueError('Prepared MOPP slab bound entry drift: '+entry)
        jumps=' '.join(f'case Continuation::{route}: goto L_native_slab_bounds_{pc};'
                       for route,pc in resumes.items())
        switch=('{ using namespace halo2::native_mopp_ray_slab_bounds_detail; '
                f'switch(slab_bounds<{wide}>(s)) {{ case Continuation::original: break; '+jumps+
                ' case Continuation::endpoint: goto L30b1f9; default: throw std::logic_error("Invalid native slab bounds continuation"); } } ')
        body=body.replace(anchor,anchor+switch,1)
        for pc in resumes.values():
            before=f's.instruction_literal(0x{pc}u);'
            if body.count(before)!=1:
                raise ValueError('Prepared MOPP slab bound handoff drift: '+pc)
            body=body.replace(before,f'L_native_slab_bounds_{pc}: '+before,1)
    return body


ENDPOINT_ORDER_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_endpoint_order.hpp"\n'


def install_endpoint_order(body):
    entry='L30b1f9: '
    result=(
        '{ using namespace halo2::native_mopp_ray_endpoint_order_detail; '
        'switch(endpoint_order(s,native_single_spill)) { '
        'case Continuation::original: break; '
        'case Continuation::read_end: goto L_native_endpoint_read_end; '
        'case Continuation::read_scratch: goto L_native_endpoint_read_scratch; '
        'case Continuation::ascending: goto L30b23d; '
        'case Continuation::descending: goto L_native_endpoint_descending; } } ')
    if body.count(entry)!=1:
        raise ValueError('Prepared MOPP endpoint-order entry drift')
    body=body.replace(entry,entry+result,1)
    for route,pc in (('read_end','30b200'),('read_scratch','30b208'),('descending','30b217')):
        before=f's.instruction_literal(0x{pc}u);'
        if body.count(before)!=1:
            raise ValueError('Prepared MOPP endpoint-order continuation drift: '+pc)
        handoff=f'L_native_endpoint_{route}: '
        if route=='read_scratch':
            handoff+=('{ using namespace halo2::native_mopp_ray_endpoint_order_detail; '
                      'const auto route=compare_endpoints(s); '
                      'if(route==Continuation::ascending)goto L30b23d; '
                      'if(route==Continuation::descending)goto L_native_endpoint_descending; } ')
        body=body.replace(before,handoff+before,1)
    return body


SLAB_DELTA_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_slab_delta.hpp"\n'


def install_slab_delta(body):
    for pc,reg,op in (('30b2ba',3,'mul'),('30b2bd',4,'mul'),('30b2c0',4,'add'),
                      ('30b30c',3,'mul'),('30b319',2,'mul'),('30b31c',2,'add')):
        before=f's.instruction_literal(0x{pc}u); s.xmm[{reg}]=_mm_{op}_ps('
        if body.count(before)!=1:
            raise ValueError('Prepared MOPP axial ratio packed arithmetic drift: '+pc)
        body=body.replace(before,before.replace(f'_mm_{op}_ps(',f'halo2::native_tu15_leaves::ordered_{op}_ps('),1)
    stages=(('30b264','false','30b275','30b28c','30b2aa','30b2c7'),
            ('30b2c7','true','30b2d3','30b2ee','30b2ff','30ae00'))
    for pc,second,read_end,ratio,reload,skip in stages:
        entry=f'L{pc}: '
        if body.count(entry)!=1:
            raise ValueError('Prepared MOPP axial delta entry drift: '+pc)
        routes=('{ using namespace halo2::native_mopp_ray_slab_delta_detail; '
                f'switch(slab_delta<{second}>(s,native_single_spill)) {{ '
                'case Continuation::original: break; '
                f'case Continuation::read_end: goto L_native_slab_delta_{read_end}; '
                f'case Continuation::ratio: goto L_native_slab_delta_{ratio}; '
                f'case Continuation::skip_ratio: goto L{skip}; }} }} ')
        body=body.replace(entry,entry+routes,1)
        ratio_hook=('{ using namespace halo2::native_mopp_ray_slab_delta_detail; '
                    f'const auto route=slab_ratio<{second}>(s,native_single_spill); '
                    f'if(route==RatioExit::reload)goto L_native_slab_ratio_{reload}; '
                    f'if(route==RatioExit::complete)goto L{skip}; ')
        if second=='true':
            ratio_hook+='if(route==RatioExit::branch)goto L_native_slab_ratio_30b323; '
        ratio_hook+='} '
        for next_pc,hook in ((read_end,''),(ratio,ratio_hook)):
            before=f's.instruction_literal(0x{next_pc}u);'
            if body.count(before)!=1:
                raise ValueError('Prepared MOPP axial delta continuation drift: '+next_pc)
            body=body.replace(before,f'L_native_slab_delta_{next_pc}: '+hook+before,1)
        for next_pc in ((reload,'30b323') if second=='true' else (reload,)):
            literal=f's.instruction_literal(0x{next_pc}u);'
            if body.count(literal)!=1:
                raise ValueError('Prepared MOPP axial ratio handoff drift: '+next_pc)
            body=body.replace(literal,f'L_native_slab_ratio_{next_pc}: '+literal,1)
    return body


GEOMETRY_PREP_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_geometry_prep.hpp"\n'
GEOMETRY_PREP_HANDOFFS={
    'read_first_vector':'30ae79','read_second_vector':'30ae7e','store_second_vector':'30ae8a',
    'spill_first':'30ae92','subtract_first':'30ae96','load_second':'30ae9e',
    'load_third':'30aeaa','load_comparison':'30aeb6',
}


def geometry_prep_arithmetic(body):
    # Both copies must expose the original arithmetic C1 to read/write
    # callbacks and use its real m32 argument form for NaNs and denormals.
    reverse='s.instruction_literal(0x30ae7cu); s.fp.st(0)=sub(s.fp.st(1),s.fp.st(0));'
    if body.count(reverse)!=1:
        raise ValueError('Prepared MOPP geometry reverse subtraction drift')
    body=body.replace(reverse,'s.instruction_literal(0x30ae7cu); '
        'halo2::native_mopp_ray_geometry_prep_detail::reverse_subtract(s);',1)
    for pc,offset in (('30ae96','1c'),('30aea2','20'),('30aeae','1c')):
        literal=f's.instruction_literal(0x{pc}u); '
        operand=f'r(s.memory,(0x{offset}u+s.gp[4]),4)'
        old=literal+f's.fp.st(0)=sub(s.fp.st(0),ext(f({operand})));'
        if body.count(old)!=1:
            raise ValueError('Prepared MOPP geometry subtraction drift: '+pc)
        body=body.replace(old,literal+
            'halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<false>(s,'+operand+');',1)
    return body


def install_geometry_prep(body):
    entry='L30ae75: '
    fallthrough='s.instruction_literal(0x30aec9u);'
    if body.count(entry)!=1 or body.count(fallthrough)!=1:
        raise ValueError('Prepared MOPP geometry boundary drift')
    body=geometry_prep_arithmetic(body)
    jumps=' '.join(f'case Continuation::{name}: goto L_native_geometry_{pc};'
                   for name,pc in GEOMETRY_PREP_HANDOFFS.items())
    prefix=('{ using namespace halo2::native_mopp_ray_geometry_prep_detail; '
            'switch(geometry_prep(s,native_single_spill)) { case Continuation::original: break; '
            +jumps+' case Continuation::decreasing: goto L30b022; '
            'case Continuation::numeric: goto L_native_geometry_ratio; } } ')
    body=body.replace(entry,entry+prefix,1)
    for pc in GEOMETRY_PREP_HANDOFFS.values():
        literal=f's.instruction_literal(0x{pc}u);'
        if body.count(literal)!=1:
            raise ValueError('Prepared MOPP geometry provider handoff drift: '+pc)
        body=body.replace(literal,f'L_native_geometry_{pc}: '+literal,1)
    return body.replace(fallthrough,'L_native_geometry_ratio: '+fallthrough,1)


def install_increasing_resume(body):
    for phase,anchor,next_pc,skip in (
        ('before_child','L_native_geometry_ratio: ','30aede','30af16'),
        ('after_child','s.instruction_literal(0x30af2du);','30af42','30af7c'),
        ('fraction','L30af7c: ','30af94','30ae02'),
    ):
        literal=f's.instruction_literal(0x{next_pc}u);'
        if body.count(anchor)!=1 or body.count(literal)!=1:
            raise ValueError('Prepared MOPP increasing resume drift: '+phase)
        hook=('{ using namespace halo2::native_mopp_ray_decreasing_resume_detail; '
              f'const auto route=increasing_resume<IncreasingPhase::{phase}>(s); '
              f'if(route==Continuation::advance)goto L{skip}; '
              f'if(route==Continuation::shorten)goto L_native_increasing_{next_pc}; ')
        if phase=='fraction':
            hook+='if(route==Continuation::read_fraction)goto L_native_increasing_30af80; '
        hook+='} '
        body=body.replace(anchor,anchor+hook if anchor.startswith('L') else hook+anchor,1)
        body=body.replace(literal,f'L_native_increasing_{next_pc}: '+literal,1)
    literal='s.instruction_literal(0x30af80u);'
    if body.count(literal)!=1:
        raise ValueError('Prepared MOPP increasing fraction handoff drift')
    return body.replace(literal,'L_native_increasing_30af80: '+literal,1)

FINAL_ROUTE_INCLUDES=''.join(
    f'#include "physics/runtime/native_tu15_mopp_ray_{kernel}.hpp"\n'
    for kernel in ('terminal_key', 'split_bytes', 'increasing_clip'))


def install_final_routes(body):
    # Each hook returns either the exact unretired instruction or an existing
    # dispatch label. Zero is the untouched entry. The prepared owner pin
    # above authenticates the complete source carrying those frontiers.
    runs = (
        ('30acc6', 'split_bytes_detail::split_bytes<true>(s,native_single_spill)',
         ('30acd2','30acda','30ace2','30acea','30acee','30acf6','30acf8','30acfb','30acfe','30ad01','30ad04'),
         ('30adc9',)),
        ('30ad0c', 'split_bytes_detail::split_bytes(s,native_single_spill)',
         ('30ad10','30ad18','30ad1e','30ad28','30ad2c','30ad30','30ad34','30ad3e','30ad41'), ('30ade0',)),
        ('30b5a3', 'terminal_key_detail::terminal_key<1>(s)', (), ('30b5f5',)),
        ('30b5a9', 'terminal_key_detail::terminal_key<2>(s)', ('30b5ad',), ('30b5f5',)),
        ('30b5b8', 'terminal_key_detail::terminal_key<3>(s)', ('30b5bc','30b5c0'), ('30b5f5',)),
        ('30b5d0', 'terminal_key_detail::terminal_key<4>(s)', ('30b5d4','30b5d8','30b5e1'), ('30b5f5',)),
        ('30b5f2', 'terminal_key_detail::terminal_key<0>(s)', (), ('30b5f5',)),
        ('30aede', 'increasing_clip_detail::increasing_clip<halo2::native_mopp_ray_increasing_clip_detail::Phase::before_child>(s,native_single_spill)',
         ('30aee2','30aee6','30aeee','30aefe'), ('30af16',)),
        ('30af42', 'increasing_clip_detail::increasing_clip<halo2::native_mopp_ray_increasing_clip_detail::Phase::after_child>(s,native_single_spill)',
         ('30af46','30af4a','30af4e','30af55','30af5d','30af6d'), ('30af7c',)),
        ('30af94', 'increasing_clip_detail::increasing_clip<halo2::native_mopp_ray_increasing_clip_detail::Phase::shortened>(s,native_single_spill)',
         ('30af97','30af9b','30af9e','30afa5','30afa8','30afb3','30afc3','30afd7','30afda',
          '30afe2','30aff6','30aff9','30b004','30b00a'), ('30ae02','30b6af')),
    )
    for entry, call, frontiers, targets in runs:
        anchor=f's.instruction_literal(0x{entry}u);'
        if body.count(anchor)!=1 or any(body.count(f'L{pc}:')!=1 for pc in targets):
            raise ValueError('Prepared MOPP final route drift: '+entry)
        jumps=' '.join([f'case 0x{pc}u: goto L_native_final_{pc};' for pc in frontiers]
                      +[f'case 0x{pc}u: goto L{pc};' for pc in targets])
        bridge=('{ switch(halo2::native_mopp_ray_'+call+') { case 0: break; '+jumps+
                ' default: throw std::logic_error("Invalid MOPP final route continuation"); } } ')
        body=body.replace(anchor,bridge+anchor,1)
        for pc in frontiers:
            literal=f's.instruction_literal(0x{pc}u);'
            if body.count(literal)!=1:
                raise ValueError('Prepared MOPP final route handoff drift: '+pc)
            body=body.replace(literal,f'L_native_final_{pc}: '+literal,1)
    return body


SETUP_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_setup_provider.hpp"\n'
SETUP_PINS={
    '78112579074b880885db5bb77045684209ff4ebaab527687290907ada3bdc14c':
        ('death','ab36c9c644b1b493ef65732833b190d99baf33154e671d09b0ec2e26edf99ca6'),
    'bc8f518eb54afb1c710835bd2a158d7f9894be65ea576c8830db03ac646a7444':
        ('live_support','cb815ab1093484a3c309604c93acfe6dc5b551627aeb07aeb484baccf53f2bdd'),
    'f11fe0845612899e45b7ac1300db89389680ca509fe12ed7c2217316c8c520fb':
        ('ai','f9a6bb6a6115809d7ce23dce93340f6082aad1773aa2f7210ba2db071e9601a1'),
}
SETUP_FRONTIERS=(
    '30b891','30b899','30b89c','30b8a5','30b8ab','30b8b1','30b8b6','30b8bc','30b8c2','30b8ca',
    '30b8ce','30b8d2','30b8d5','30b8d8','30b8db','30b8e1','30b8e4','30b8e7','30b8f2','30b8fc',
    '30b910','30b917','30b91f','30b925','30b92b','30b92e','30b932','30b936','30b93a','30b93f',
    '30b940','30b948','30b94d','30b952','30b957','30b958','30b95b','30b95c',
)


def install_setup_call(canonical, prepared):
    original=function(canonical,'run_30b890')
    retained=function(prepared,'run_30b890')
    owner,expected=SETUP_PINS.get(digest(original), (None,None))
    if not owner or digest(retained)!=expected or prepared.count(retained)!=1 or SETUP_INCLUDE in prepared:
        raise ValueError('Complete prepared MOPP call-site setup drift')
    child,spill=('dispatch(s,0x30aa70u,0x30b952u,false);','halo2::native_bsp_segment_detail::spill_reference') \
        if owner=='live_support' else ('run_30aa70(s);','native_single_spill')
    jumps=' '.join(f'case 0x{pc}u: goto L_native_setup_{pc};' for pc in SETUP_FRONTIERS)
    bridge=('\n    { using namespace halo2::native_mopp_ray_setup_detail; '
            f'switch(setup_provider(s,[&]{{{child}}},{spill})) {{ '
            'case 0: break; case setup_complete: return; '+jumps+
            ' default: throw std::logic_error("Invalid MOPP setup continuation"); } }')
    opening=retained.index('{')+1
    bound=retained[:opening]+bridge+retained[opening:]
    # The untouched TU1.5 bytes encode MULPS xmm4,xmm1 and xmm1,xmm2.
    # Keep those destinations in the retained path for dual-NaN operands.
    for pc,register in (('30b909',4),('30b944',1)):
        literal=f's.instruction_literal(0x{pc}u); s.xmm[{register}]=_mm_mul_ps('
        if bound.count(literal)!=1:
            raise ValueError('Prepared MOPP setup SSE destination drift: '+pc)
        bound=bound.replace(literal,literal.replace('_mm_mul_ps','halo2::native_tu15_leaves::ordered_mul_ps'),1)
    for pc in SETUP_FRONTIERS:
        literal=f's.instruction_literal(0x{pc}u);'
        if bound.count(literal)!=1:
            raise ValueError('Prepared MOPP setup continuation drift: '+pc)
        bound=bound.replace(literal,f'L_native_setup_{pc}: '+literal,1)
    return SETUP_INCLUDE+prepared.replace(retained,bound,1),owner


SLAB_INTERVAL_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_slab_interval.hpp"\n'


def install_slab_interval(body):
    for pc,direction in (('30b217','false'),('30b23d','true')):
        before=f's.instruction_literal(0x{pc}u);'
        if body.count(before)!=1:
            raise ValueError('Prepared MOPP ray slab interval boundary drift: '+pc)
        routes=('{ using namespace halo2::native_mopp_ray_slab_interval_detail; '
                f'switch(slab_interval<{direction}>(s)) {{ '
                'case Continuation::original: break; '
                'case Continuation::reject: goto L30b6ab; '
                'case Continuation::accept: goto L30b264; } }\n')
        body=body.replace(before,routes+before,1)
    return body


AXIS_BYTE_NODES_INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_axis_byte_nodes.hpp"\n'
AXIS_BYTE_NODE_STARTS=('30aaae','30aae3','30ab22','30ab56','30ab94','30abc8','30ac06')
AXIS_BYTE_NODE_PINS={
    'death':('11b229c76e852a63f35a1af171518b696ad45cb79a16ba7dceea00ee53fc318f',
             '238b4d13372190b9a7b1b24ff39648ece06ce55a4bbf95ad4edc80c8f74cfab8'),
    'ai':('b9bb71bbfd48653c25c6bda479e0150154dca9f819148e5c37a2d1364807672f',
           'f0a36ef210680571cbd13a88f89a705abbff2adbe3588964762ce59fe8982891'),
}
AXIS_BYTE_NODE_ROUTES={
    3:'write_first',4:'adjust_second',5:'load_first',6:'write_second',7:'spill_first',8:'load_second',
    9:'spill_second',10:'load_low',11:'combine_low',12:'load_high',13:'combine_high',
}


def select_axis_byte_blocks(body):
    anchors=[f'L{site}: ' for site in AXIS_BYTE_NODE_STARTS]
    if any(body.count(anchor)!=1 for anchor in anchors):
        raise ValueError('MOPP axis byte-node label drift')
    positions=[body.index(anchor) for anchor in anchors]
    if positions!=sorted(positions):
        raise ValueError('MOPP axis byte-node sequence drift')
    return [body[start:end] for start,end in zip(positions,positions[1:])]


def install_axis_byte_nodes(body,canonical,owner):
    raw=select_axis_byte_blocks(canonical)
    prepared=select_axis_byte_blocks(body)
    if (digest(''.join(raw)),digest(''.join(prepared)))!=AXIS_BYTE_NODE_PINS[owner]:
        raise ValueError('MOPP axis byte-node canonical or prepared body drift: '+owner)
    shared='goto L30adc9;'
    for axis,block in enumerate(prepared):
        pc=AXIS_BYTE_NODE_STARTS[axis]
        sites=re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\);',block)
        if len(sites)!=15 or len(set(sites))!=15 or sites[0]!=pc or not block.endswith(shared+'\n'):
            raise ValueError('MOPP axis byte-node callback or continuation drift: '+pc)
        replacement=block
        arms=[]
        for index,route in AXIS_BYTE_NODE_ROUTES.items():
            literal=f's.instruction_literal(0x{sites[index]}u);'
            label=f'L_native_axis_byte_{sites[index]}'
            replacement=replacement.replace(literal,label+': '+literal,1)
            arms.append(f'case Continuation::{route}: goto {label};')
        hook=('{ using namespace halo2::native_mopp_ray_axis_byte_nodes_detail; '
              f'switch(prepare_axis_byte_node<{axis}>(s,native_single_spill)) {{ case Continuation::retained: break; '
              +' '.join(arms)+f' case Continuation::offsets: {shared} }} }} ')
        anchor=f'L{pc}: '
        replacement=replacement.replace(anchor,anchor+hook,1)
        if body.count(block)!=1:
            raise ValueError('MOPP axis byte-node prepared interval is not unique: '+pc)
        body=body.replace(block,replacement,1)
    return body


def install_prepared_entry(canonical, prepared):
    if PLANE_SPLIT_INCLUDE in prepared:
        raise ValueError('Unexpected MOPP plane split include in prepared owner')
    original,body,owner=authenticate(canonical)
    # The old full-controller overlay is a separate private experiment.
    if original[0]!=body:
        return prepared,None
    retained=function(prepared,'run_30aa70')
    if (digest(retained)!=PREPARED_ENTRY_PINS[owner] or
        prepared.count(retained)!=1 or INCLUDE in prepared or DISPATCH_INCLUDE in prepared or
        ENDPOINT_ORDER_INCLUDE in prepared or SCALE_VECTORS_INCLUDE in prepared or SCALE_TRANSLATION_INCLUDE in prepared or OPCODE_RESUME_INCLUDE in prepared or SCALE_TAIL_INCLUDE in prepared or PLANE_COMPARE_INCLUDE in prepared or SLAB_INTERVAL_INCLUDE in prepared or SLAB_DELTA_INCLUDE in prepared or AXIS_PLANE_INCLUDE in prepared or DECREASING_RESUME_INCLUDE in prepared or WIDE_PLANE_INCLUDE in prepared or SLAB_BOUNDS_INCLUDE in prepared or
        retained.count('L30aaa0:')!=1 or retained.count('L30ae16:')!=1):
        raise ValueError('Complete prepared MOPP ray entry/continuation drift: '+owner)
    opening=retained.index('{')+1
    routes=ENTRY_ROUTES.replace('EntryOwner::OWNER','EntryOwner::'+owner)
    patched=retained[:opening]+routes+retained[opening:]
    # The actual table reads select the original case switch. A refused
    # arm hook enters the exact original case label.
    for target,prefix in DISPATCH_ARMS.items():
        arm=f'case 0x{target}u:goto L{target};'
        if patched.count(arm)!=1:
            raise ValueError('Prepared MOPP ray retained dispatch arm drift: '+target)
        patched=patched.replace(arm,f'case 0x{target}u:{prefix}goto L{target};',1)
    if DISPATCH_TABLES_INCLUDE in prepared:
        raise ValueError('Unexpected MOPP dispatcher table include in prepared owner')
    patched=install_dispatch_tables(patched,function(prepared,'canonical_branch'),owner)
    prepared=DISPATCH_TABLES_INCLUDE+prepared
    patched=install_scale_inputs(patched)
    prepared=SCALE_INPUTS_INCLUDE+prepared
    patched=install_scale_translation(patched)
    if RECURSIVE_ARGUMENTS_INCLUDE in prepared:
        raise ValueError('Unexpected MOPP recursive argument include in prepared owner')
    patched=install_recursive_arguments(patched)
    prepared=RECURSIVE_ARGUMENTS_INCLUDE+prepared
    patched=install_opcode_resume(patched)
    patched=install_scale_vectors(patched)
    patched=install_scale_tail(patched)
    patched=install_wide_plane(patched)
    if DECREASING_CLIP_INCLUDE in prepared:
        raise ValueError('Unexpected MOPP decreasing clip include in prepared owner')
    patched=install_decreasing_clip(patched)
    prepared=DECREASING_CLIP_INCLUDE+prepared
    patched=install_decreasing_resume(patched)
    if AXIS_BYTE_NODES_INCLUDE in prepared:
        raise ValueError('Unexpected MOPP axis byte-node include in prepared owner')
    patched=install_axis_byte_nodes(patched,body,owner)
    prepared=AXIS_BYTE_NODES_INCLUDE+prepared
    patched=install_axis_plane(patched)
    patched=install_plane_compare(patched)
    if COLLECTOR_INCLUDE in prepared:
        raise ValueError('Unexpected MOPP collector include in prepared owner')
    patched=install_collector(patched)
    prepared=COLLECTOR_INCLUDE+prepared
    patched=install_plane_split(patched)
    prepared=PLANE_SPLIT_INCLUDE+prepared
    patched=install_slab_bounds(patched)
    if KEY_COPY_INCLUDE in prepared:
        raise ValueError('Unexpected MOPP key-copy include in prepared owner')
    patched=install_key_copy(patched)
    prepared=KEY_COPY_INCLUDE+prepared
    patched=install_shared_return(patched)
    patched=install_endpoint_order(patched)
    patched=install_slab_interval(patched)
    patched=install_slab_delta(patched)
    if GEOMETRY_PREP_INCLUDE in prepared:
        raise ValueError('Unexpected MOPP geometry include in prepared owner')
    patched=install_geometry_prep(patched)
    patched=install_increasing_resume(patched)
    patched=install_final_routes(patched)
    prepared=FINAL_ROUTE_INCLUDES+GEOMETRY_PREP_INCLUDE+prepared
    return SLAB_BOUNDS_INCLUDE+ENDPOINT_ORDER_INCLUDE+SLAB_DELTA_INCLUDE+SLAB_INTERVAL_INCLUDE+DECREASING_RESUME_INCLUDE+WIDE_PLANE_INCLUDE+AXIS_PLANE_INCLUDE+PLANE_COMPARE_INCLUDE+SCALE_VECTORS_INCLUDE+SCALE_TAIL_INCLUDE+OPCODE_RESUME_INCLUDE+SCALE_TRANSLATION_INCLUDE+DISPATCH_INCLUDE+INCLUDE+prepared.replace(retained,patched,1),owner
