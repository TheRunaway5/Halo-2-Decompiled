"""Bind the authenticated collision pair owner and retain its original routes."""
import hashlib
import re


INCLUDE = '#include "physics/runtime/native_tu15_collision_pair_2f88d0.hpp"\n'
NAMESPACE = 'halo2::native_collision_pair_2f88d0_detail'
REFERENCE = 'native_collision_pair_reference_2f88d0'
PIN = '83c7c39c2af69ddc9d56481a6658b13eb1f140bee99321715b716eb8b99085b9'
UNIT = 'void source_run_physics_live_support_tu15('
BODY = re.compile(r'^static void run_2f88d0\(WorldState &s\) \{.*?^\}(?=\n\nstatic void run_2f8b60)', re.M | re.S)
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)
CURSOR = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
TAIL = ('s.instruction_literal(0x2f8b59u); s.set(6,s.pop(),4,0);\n'
        's.instruction_literal(0x2f8b5au); s.set(3,s.pop(),4,0);')
POPS = ('if(!s.try_pop_registers<6,3>(0x2f8b5au)){\n'
        '    ' + TAIL.replace('\n','\n    ') + '\n}')
DIRECT = 's.push(0x2f8949u,0x2f894eu);run_309150(s);'
ROUTES = {
    '2f8929': ('0x2e4550u', '2f892e'),
    '2f8932': ('r(s.memory,(0x14u+s.gp[2]),4)', '2f8935'),
    '2f895f': ('0x309060u', '2f8964'),
    '2f898f': ('0x30e930u', '2f8994'),
    '2f89b5': ('0x30d350u', '2f89ba'),
    '2f89cd': ('0x2e4760u', '2f89d2'),
    '2f8b27': ('r(s.memory,(0x4u+s.gp[0]),4)', '2f8b2a'),
}
CLOCK = ('if(!s.services.timestamp_counter)throw std::runtime_error("Required original contact RDTSC owner unavailable");'
         '{const auto x=s.services.timestamp_counter();')


def original(source):
    found = BODY.findall(source)
    if len(found) != 1 or hashlib.sha256(found[0].encode()).hexdigest() != PIN:
        raise ValueError('Original 2F88D0 collision pair changed')
    literals = CURSOR.findall(found[0])
    if len(literals) != 187 or len(set(literals)) != 187:
        raise ValueError('Original 2F88D0 collision pair observation sites changed')
    return found[0]


def check_route(source):
    if (UNIT not in source or
            source.count('case 0x2f88d0u:run_2f88d0(s);return;') != 1 or
            source.count('case 0x2f88d0u:run_2f88d0(s);break;') != 1 or
            source.count('case 0x309150u:run_309150(s);return;') != 1):
        raise ValueError('Missing authenticated 2F88D0 entry or direct collision child')


def ordered(source):
    for original, ordered, expected in (
            ('_mm_mul_ps', 'ordered_mul_ps', 8), ('_mm_add_ps', 'ordered_add_ps', 7)):
        source, count = re.subn(rf'\b{original}\(', f'halo2::native_tu15_leaves::{ordered}(', source)
        if count != expected:
            raise ValueError(f'2F88D0 collision pair changed {original} operand sites')
    return source


def reference(canonical, replay=True):
    source = original(canonical)
    signature = 'static void run_2f88d0(WorldState &s)'
    if replay:
        for pc,(target,resume) in ROUTES.items():
            actual = f'const auto target={target};s.push(0x{pc}u,0x{resume}u);dispatch(s,target,0x{pc}u,false);'
            if source.count(actual) != 1:
                raise ValueError(f'2F88D0 collision pair route {pc} changed')
            checked = actual.replace(f'dispatch(s,target,0x{pc}u,false);', 'halo2::native_object_tick_detail::replay(s,target);')
            source = source.replace(actual, checked, 1)
        if source.count(DIRECT) != 1 or source.count(CLOCK) != 2:
            raise ValueError('2F88D0 collision pair direct child or clock protocol changed')
        source = source.replace(DIRECT, 's.push(0x2f8949u,0x2f894eu);halo2::native_object_tick_detail::replay(s,0x309150u);', 1)
        for pc in ('2f88f3', '2f8b42'):
            start = f'if(s.services.instruction){{s.services.instruction(0x{pc}u);}} '
            source = source.replace(start + CLOCK, start + f'{{const auto x=clocks.read(0x{pc}u);', 1)
        if CLOCK in source:
            raise ValueError('2F88D0 collision pair reference still invokes a real clock')
        declaration = f'void {REFERENCE}(WorldState &s,{NAMESPACE}::ClockSamples &clocks)'
    else:
        declaration = f'static void {REFERENCE}_live(WorldState &s)'
    return ordered(source.replace(signature,declaration,1)) + '\n'


def install(canonical, emitted):
    check_route(canonical)
    check_route(emitted)
    raw = original(canonical)
    current = BODY.search(emitted)
    if not current:
        raise ValueError('Missing 2F88D0 collision pair from prepared source')
    expected = CURSOR.sub(r's.instruction_literal(\1);',raw)
    if expected.count(TAIL) != 1:
        raise ValueError('2F88D0 collision pair return frame changed')
    # The spatial pass precedes the global POP pass. Validation also checks
    # the final prepared body with that pass's exact scalar fallback.
    if POPS in current[0]:
        expected = expected.replace(TAIL,POPS,1)
    head = expected.index('{') + 1
    oracle = reference(canonical)
    hook = (f'\n    if ({NAMESPACE}::bridge(s,{REFERENCE},'
            '\n        [](auto &v,std::uint32_t target,std::uint32_t pc,bool tail){dispatch(v,target,pc,tail);},'
            '\n        [](auto &v){run_309150(v);})) return;\n')
    native = ordered(expected[:head] + hook + expected[head:])
    if REFERENCE in emitted:
        if emitted.count(INCLUDE) != 1 or emitted.count(oracle) != 1 or current[0] != native:
            raise ValueError('Prepared 2F88D0 collision pair changed')
        return emitted, ['2f88d0']
    if current[0] != expected or INCLUDE in emitted:
        raise ValueError('Specialized 2F88D0 collision pair differs from the pinned original')
    first = FIRST_BODY.search(emitted)
    if not first:
        raise ValueError('Missing shared prefix for the collision pair replay')
    prefix = emitted[:first.start()] + oracle + '\n'
    return INCLUDE + prefix + emitted[first.start():current.start()] + native + emitted[current.end():], ['2f88d0']
