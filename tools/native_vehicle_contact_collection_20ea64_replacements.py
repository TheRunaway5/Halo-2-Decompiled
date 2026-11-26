"""Bind the three vehicle contact collection phases around the two Original calls."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_contact_collection_20ea64.hpp"\n'
SPANS = (
    ('20a164', '20a178', 7, '6e6089459757026de74afdc2a05ab06e69b226873c9b473d0830830ca4a57577',
     '7284ed7bf73de84fa6ca3c57912cb3e36ce5032048b49e0171d8541074cd0893'),
    ('20a17d', '20a2d2', 76, 'be4fe9d768677b9b7971841420fcefe6dc6bcfb0d895fcf3df5a087d89d757e7',
     '9f2b539282d8973354ab0259754c03643c5da78ae157de1bdb36a18f2bac68d9'),
    ('20a2d7', '20a3f7', 57, 'ca61babc4bb9c5ca234500fca3024bb870024dd06d4f6bfe05cb7a72a747c416',
     '0cdb6c2e918853ce2c82d9c711c0f163ae6e5c56943a6974916fddef614752f8'),
)
ENTRIES = ('20a164', '20a17d', '20a190', '20a2d7')
EXITS = (('20a178',), ('20a2d2', '20aff4', '20b018'), ('20a3f7', '20adc1', '20a592'))
DATA = (('46fc44', '474d04'), ('46504c', '46a0ec'), ('46fc50', '474d10'), ('526254', '52be68'))
BRIDGE = 'halo2::native_vehicle_contact_collection_20ea64_detail::bridge'


def region(source, index):
    begin, end, *_ = SPANS[index]
    first = rf'^L{begin}:' if index == 1 else rf'^[^\n]*instruction(?:_literal)?\(0x{begin}u\)'
    owner = OWNER.search(source)
    span = re.search(rf'{first}[\s\S]*?(?=^[^\n]*instruction(?:_literal)?\(0x{end}u\))', owner[0], re.M) if owner else None
    if not span:
        raise ValueError(f'Vehicle 20EA64 phase {index} not found')
    return span, owner.start()


def check_calls(canonical):
    routes = (
        'case 0x209a50u:return 0x20e350u;', 'case 0x209a50u:run_209a50(s);break;',
        'case 0x183c50u:return 0x185070u;', 'case 0x20b470u:return 0x20fd70u;',
        'case 0x20a17du:return 0x20ea7du;', 'case 0x20a2d7u:return 0x20ebd7u;',
        'const auto target=selected(0x183c50u,s.image);s.push(0x20a178u,selected(0x20a17du,s.image));dispatch(s,target,0x20a178u,false);',
        'const auto target=selected(0x20b470u,s.image);s.push(0x20a2d2u,selected(0x20a2d7u,s.image));dispatch(s,target,0x20a2d2u,false);',
        'instruction(0x20a162u);} if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L20a17d;',
        'instruction(0x20b012u);} if(s.flags.sf!=s.flags.of)goto L20a190;',
        *(f'case 0x{old}u:return 0x{new}u;' for old, new in DATA),
        *(f'L{pc}: if(s.services.instruction){{s.services.instruction(0x{pc}u);}}'
          for pc in ('20aff4', '20b018', '20adc1', '20a592')),
    )
    if any(canonical.count(route) != 1 for route in routes):
        raise ValueError('Vehicle 20EA64 original child, loop or relocation changed')
    if any(canonical.count(f'goto L{pc};') != 1 for pc in ('20a17d', '20a190')):
        raise ValueError('Vehicle 20EA64 wheel collection incoming routes changed')


def prepared_literal(canonical, index):
    _, _, expected, raw_pin, prepared_pin = SPANS[index]
    raw = region(canonical, index)[0][0]
    literal, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                            r's.instruction_literal(\1);', raw)
    for old, new in DATA:
        literal = literal.replace(f'selected(0x{old}u,s.image)', f'(s.image==SourcePhysicsContactImage::MapPack11?0x{old}u:0x{new}u)')
    if digest(raw) != raw_pin or count != expected or digest(literal) != prepared_pin:
        raise ValueError(f'Vehicle 20EA64 pinned phase {index} changed')
    return literal


def reference(canonical, index):
    stage = prepared_literal(canonical, index)
    for operation, helper in (
        ('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss'),
        ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss'),
        ('_mm_sub_ss', 'native_tu15_leaves::ordered_difference_ss'),
        ('_mm_div_ss', 'native_vehicle_wheel_response_20e6c3_detail::ordered_divide_ss'),
    ):
        stage = stage.replace(f'{operation}(', f'halo2::{helper}(')
    return stage


def bridge_stage(canonical, index):
    entry, end, *_ = SPANS[index]
    stage = reference(canonical, index)
    sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    assert sites[0] == entry
    resume = lambda pc: f'L_contact_collection_resume_{pc}'
    follow = f'L_contact_collection_continue_{end}'
    for pc in (sites if index == 1 else sites[1:]):
        line = re.compile(rf'^([^\n]*instruction_literal\(0x{pc}u\);)', re.M)
        stage, count = line.subn(rf'{resume(pc)}:\n\1', stage)
        if count != 1:
            raise ValueError(f'Unexpected vehicle contact collection continuation {pc}')

    cases = ''.join(f'case 0x{pc}u: goto {resume(pc)}; ' for pc in (sites if index == 1 else sites[1:]))
    cases += f'case 0x{end}u: goto {follow}; '
    cases += ''.join(f'case 0x{pc}u: goto L{pc}; ' for pc in EXITS[index][1:])
    unexpected = 'default: throw std::logic_error("Unknown vehicle contact collection continuation");'
    if index != 1:
        # The earlier reset span ends at the line with the 20A164 literal.
        # Keep this switch on that line so both binders remain idempotent.
        return (f'switch ({BRIDGE}(s,0x{entry}u)) {{case 0: break; {cases}{unexpected}}} '
                f'{stage}{follow}:\n')

    dispatch = 'L_contact_collection_dispatch'
    for pc in ('20a17d', '20a190'):
        label = f'{resume(pc)}:\nL{pc}: '
        if stage.count(label) != 1:
            raise ValueError(f'Unexpected vehicle contact collection entry {pc}')
        declare = '    std::uint32_t contact_collection_entry;\n' if pc == entry else ''
        stage = stage.replace(label, f'L{pc}:\n{declare}    contact_collection_entry=0x{pc}u;\n'
                              f'    goto {dispatch};\n{resume(pc)}:\n', 1)
    stage += (f'    goto {follow};\n{dispatch}:\n'
              f'    switch ({BRIDGE}(s,contact_collection_entry)) {{\n'
              f'    case 0: if (contact_collection_entry==0x{entry}u) goto {resume(entry)}; '
              f'goto {resume("20a190")};\n'
              f'    {cases}{unexpected}\n    }}\n{follow}:\n')
    return stage


def install(canonical, emitted):
    check_calls(canonical)
    for index in reversed(range(len(SPANS))):
        literal = prepared_literal(canonical, index)
        bound = bridge_stage(canonical, index)
        span, offset = region(emitted, index)
        if span[0] == bound:
            continue
        if span[0] != literal or INCLUDE in emitted:
            raise ValueError(f'Vehicle 20EA64 emitted phase {index} boundaries changed')
        emitted = emitted[:offset+span.start()] + bound + emitted[offset+span.end():]
    if not emitted.startswith(INCLUDE) and INCLUDE not in emitted:
        emitted = INCLUDE + emitted
    if emitted.count(INCLUDE) != 1:
        raise ValueError('Vehicle 20EA64 duplicate adapter include')
    return emitted, ['20ea64']
