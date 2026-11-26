"""Bind the exact selected World contact-cache replacement and insertion owner."""

import hashlib
import re


INCLUDE = '#include "physics/runtime/native_tu15_contact_cache_update_30cd00.hpp"\n'
DETAIL = 'halo2::native_contact_cache_update_30cd00_detail'
REFERENCE = 'native_contact_cache_update_reference_309210'
MARKER = 'static void run_309210(ObservedWorldState &s)'
BODY = re.compile(r'^' + re.escape(MARKER) + r' \{.*?^\}', re.M | re.S)
PIN = 'ca0b0510d188ddc9b1ab0ec64d9c53e561815088400cfc290cab330265ad2cd8'
PREPARED = '832df289225bd877b909cf1d8343f002ac05c52fd1075d9d62aedcaf103f77fe'
SELECTED = {
    '309210': '30cd00', '4654c0': '46a560', '309363': '30ce53', '309371': '30ce61',
    '418a60': '41c500', '3093d2': '30cec2', '308fb0': '30caa0', '309428': '30cf18', '2e0ba0': '2e4690',
}
CALLS = ('3093cf', '309423')
OPERANDS = {
    '_mm_sub_ps': ('halo2::native_world_leaves_detail::ordered_sub_ps', 3),
    '_mm_mul_ps': ('halo2::native_tu15_leaves::ordered_mul_ps', 4),
    '_mm_add_ps': ('halo2::native_tu15_leaves::ordered_add_ps', 1),
    '_mm_add_ss': ('halo2::native_world_leaves_detail::ordered_add_ss', 4),
}
HOOK = (f'\n    if ({DETAIL}::bridge(s,{REFERENCE},'
        '\n        [](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);},'
        '\n        [](auto &v){run_2e0ba0(v);})) return;\n')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


REDUCTION_INCLUDE = '#include "physics/runtime/native_tu15_contact_reduction_30caa0.hpp"\n'
REDUCTION_PINS = {
    '66522d079de2c3a93a8efeac46ca650f9fd85e732b8dc59fd17cdc4e88da4f20':
        '29ad8d4a9be18b67b6ed17000f59a8ac8be4dbceb223445e6ccaf448851aa9be',
    '47fa03a1d1a20104ae1305967df589f9d2e056c40675021e014551b5c4296dac':
        'ac0100f8b79a402a472eb8235a6f3ec47865d79a70ba363641401a7779a21764',
}
REDUCTION_SCANS = {
    'first': ('30cad0','30cadc','30cadf','30cae2','30cb09','30cb1a','30cb24'),
    'second': ('30cb40','30cb48','30cb4b','30cb4e','30cb75','30cb86','30cb90'),
    'cross': ('30cbb0','30cbb8','30cbbb','30cbbe','30cc0d','30cc1e','30cc28'),
    'fourth': ('30cc40','30cc48','30cc4b','30cc4e','30cc75','30cc86','30cc90'),
}
REDUCTION_REMOVAL = (
    '30cca0','30cca8','30ccab','30ccaf','30ccb2','30ccb5','30ccb8','30ccba','30ccbd','30ccc3',
    '30cccb','30cccf','30ccd7','30cce0','30cce4','30ccea','30ccf2','30ccf3','30ccf4','30ccf7','30ccf8',
)
REDUCTION_FRAMES = {
    'initial': ('30caa0','30cad0',('30caa1','30caa9','30cab0','30cab3','30cab6','30cabc','30cabd','30cac7','30cacb')),
    'first_anchor': ('30cb24','30cb40',('30cb2d','30cb33','30cb3b')),
    'second_anchor': ('30cb90','30cbb0',('30cb95','30cb98','30cb9e','30cba1','30cba9')),
    'cross_anchor': ('30cc28','30cc40',('30cc2d','30cc38')),
    'finish_selection': ('30cc90','30cca0',('30cc97',)),
}
# Preserve the actual untouched XBE destination when both inputs carry NaNs.
REDUCTION_ADDSS = {'30caf9':1,'30cb01':2,'30cb65':1,'30cb6d':2,
                   '30cbf9':2,'30cc05':3,'30cc65':1,'30cc6d':2}
REDUCTION_MULPS = {'30cbd5':3,'30cbe6':0}


def reduction_hook(body, entry, call, frontiers, finished=False, *, labeled=True, next_label=None):
    anchor=f'L{entry}: ' if labeled else f's.instruction_literal(0x{entry}u);'
    if body.count(anchor)!=1:
        raise ValueError('Prepared 30CAA0 native entry drift: '+entry)
    jumps=' '.join(f'case 0x{pc}u: goto L_native_reduction_{pc};' for pc in frontiers)
    finish='case complete: return; ' if finished else ''
    if next_label:
        if body.count(f'L{next_label}: ')!=1:
            raise ValueError('Prepared 30CAA0 phase transition drift: '+next_label)
        finish+=f'case 0x{next_label}u: goto L{next_label}; '
    bridge=('{ using namespace halo2::native_contact_reduction_30caa0_detail; '
            f'switch({call}) {{ case 0: break; '+finish+jumps+
            ' default: throw std::logic_error("Invalid contact-reduction continuation"); } } ')
    body=body.replace(anchor,anchor+bridge if labeled else bridge+anchor,1)
    for pc in frontiers:
        literal=f's.instruction_literal(0x{pc}u);'
        if body.count(literal)!=1:
            raise ValueError('Prepared 30CAA0 handoff drift: '+pc)
        body=body.replace(literal,f'L_native_reduction_{pc}: '+literal,1)
    return body


def install_reduction_scan(canonical, prepared):
    from native_mopp_ray_traversal_replacements import function
    owner = function(canonical,'run_30caa0')
    retained = function(prepared,'run_30caa0')
    if (REDUCTION_PINS.get(digest(owner)) != digest(retained) or
            prepared.count(retained) != 1 or REDUCTION_INCLUDE in prepared):
        raise ValueError('Complete prepared 30CAA0 contact reduction drift')
    body = retained
    for intrinsic, boundaries, detail in (('add_ss',REDUCTION_ADDSS,'native_world_leaves_detail'),
                                           ('mul_ps',REDUCTION_MULPS,'native_tu15_leaves')):
        for pc, destination in boundaries.items():
            literal=f's.instruction_literal(0x{pc}u); s.xmm[{destination}]=_mm_{intrinsic}('
            if body.count(literal)!=1:
                raise ValueError('Prepared 30CAA0 SSE operand drift: '+pc)
            body=body.replace(literal,literal.replace(f'_mm_{intrinsic}',f'halo2::{detail}::ordered_{intrinsic}'),1)
    for scan, frontiers in REDUCTION_SCANS.items():
        body=reduction_hook(body,frontiers[0],f'select<Scan::{scan}>(s)',frontiers)
    child='dispatch(s,target,0x30ccb5u,false);'
    if body.count(child)!=1:
        raise ValueError('Prepared 30CAA0 actual indirect child drift')
    body=reduction_hook(body,'30cca0',f'remove(s,[&](std::uint32_t target){{{child}}})',
                        REDUCTION_REMOVAL,finished=True)
    for stage,(entry,next_label,frontiers) in REDUCTION_FRAMES.items():
        body=reduction_hook(body,entry,f'configure_frame<FrameStage::{stage}>(s)',frontiers,
                            labeled=False,next_label=next_label)
    return REDUCTION_INCLUDE+prepared.replace(retained,body,1)


def original(canonical):
    bodies = BODY.findall(canonical)
    if len(bodies) != 1 or digest(bodies[0]) != PIN:
        raise ValueError('Original World 30CD00 contact-cache update changed')
    sites = re.findall(r'instruction\(world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)\)', bodies[0])
    if len(sites) != 187 or len(set(sites)) != 187 or any(int(b, 16) - int(a, 16) != 0x3af0 for a, b in sites):
        raise ValueError('World 30CD00 observation sites changed')
    return bodies[0]


def routes(text, canonical=False):
    if (text.count('case 0x309210u:run_309210(s);break;') != 1 or
            text.count('static void run_2e0ba0(ObservedWorldState &s) {') != 1 or
            'void source_run_physics_world_runtime_selected_observations(' not in text):
        raise ValueError('Missing World 30CD00 public route or real transform')
    if canonical:
        for disc, tu15 in SELECTED.items():
            if text.count(f'case 0x{disc}u:return 0x{tu15}u;') != 1:
                raise ValueError(f'World 30CD00 operand {disc} changed')
        target = 'if(target==selected(0x309210u,s.image)){run_309210(s);return;}'
    else:
        target = 'if(target==(s.image==SourcePhysicsContactImage::MapPack11?0x309210u:0x30cd00u)){run_309210(s);return;}'
    if text.count(target) != 1:
        raise ValueError('Missing World 30CD00 internal dispatcher route')


def ordered(text):
    for intrinsic, (equivalent, count) in OPERANDS.items():
        source = intrinsic + '('
        if text.count(source) != count:
            raise ValueError(f'World 30CD00 {intrinsic} operand sites changed')
        text = text.replace(source,equivalent + '(')
    return text


def reference(canonical, replay=True):
    text = original(canonical)
    if replay:
        for site in CALLS:
            actual = f'dispatch(s,target,0x{site}u,false);'
            if text.count(actual) != 1:
                raise ValueError(f'World 30CD00 dispatched site {site} changed')
            text = text.replace(actual,'x87.child(s,target);',1)
        if text.count('run_2e0ba0(s);') != 2:
            raise ValueError('World 30CD00 direct transform routes changed')
        text = text.replace('run_2e0ba0(s);','x87.child(s,0x2e4690u);')
        signature = f'static void {REFERENCE}(ObservedWorldState &s,{DETAIL}::X87Replay &x87)'
    else:
        signature = f'static void {REFERENCE}_live(ObservedWorldState &s)'
    return ordered(text.replace(MARKER,signature,1)) + '\n'


def install(canonical, emitted):
    routes(canonical,canonical=True)
    routes(emitted)
    current = BODY.search(emitted)
    if not current:
        raise ValueError('Missing prepared World 30CD00 body')
    retained = reference(canonical)
    if REFERENCE in emitted:
        restored = current[0].replace(HOOK,'',1)
        for intrinsic, (equivalent, _) in OPERANDS.items():
            restored = restored.replace(equivalent + '(',intrinsic + '(')
        if (emitted.count(INCLUDE) != 1 or emitted.count(retained) != 1 or
                current[0].count(HOOK) != 1 or digest(restored) != PREPARED):
            raise ValueError('Prepared World 30CD00 body changed')
        return emitted, ['30cd00']
    if digest(current[0]) != PREPARED or INCLUDE in emitted:
        raise ValueError('Specialized 30CD00 differs from the exact World owner')
    head = current[0].index('{') + 1
    native = ordered(current[0][:head] + HOOK + current[0][head:])
    return INCLUDE + emitted[:current.start()] + retained + native + emitted[current.end():], ['30cd00']
