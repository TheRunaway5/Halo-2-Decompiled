"""Bind and account for the last 133B40 instruction using its real dispatcher."""
from collections import Counter
import hashlib
import re

import native_fog_plane_classifier_replacements as classifier
import native_fog_plane_output_replacements as output
import native_fog_plane_projection_replacements as projection
import native_fog_plane_transform_replacements as transform


INCLUDE = '#include "campaign/native_tu15_fog_plane_call.hpp"\n'
REFERENCE = 'native_fog_plane_call_reference_133b40'
STAGES = (classifier, projection, output, transform)
DISPATCH_PIN = '1517f03e7cb2193572df9f624025386390ba7927c2cfac3fa1358a2edd8e8911'
ORIGINAL = 'if(s.services.instruction){s.services.instruction(0x133d8au);} ' + classifier.GEOMETRY_CALL
EMITTED = 's.instruction_literal(0x133d8au); ' + classifier.GEOMETRY_CALL
LABEL = '\nL133d8a:'
LITERAL = re.compile(r's\.services\.instruction\(0x([0-9a-f]+)u\);')
DISPATCH = re.compile(r'^static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}', re.M)


def check_dispatch(unit):
    entries = DISPATCH.findall(unit)
    if len(entries) != 1 or hashlib.sha256(entries[0].encode()).hexdigest() != DISPATCH_PIN:
        raise ValueError('133B40 vector-angle required dispatcher changed')
    if 'case 0x11f7e0u:' in entries[0] or 'default:s.required(site,target,tail);return;' not in entries[0]:
        raise ValueError('133B40 vector-angle child no longer uses its required-owner route')


def reference_body(canonical):
    owner = classifier.source(canonical)
    if owner.count(ORIGINAL) != 1:
        raise ValueError('133B40 original vector-angle call changed')
    replay = ORIGINAL.replace('dispatch(s,target,0x133d8au,false);',
                              'halo2::native_object_tick_detail::replay(s,target);')
    return f'static void {REFERENCE}(WorldState &s) {{\n{replay}\n}}\n'


def coverage(canonical):
    original = Counter(LITERAL.findall(classifier.source(canonical)))
    references = [stage.reference_body(canonical) for stage in STAGES]
    references.append(reference_body(canonical))
    seen = Counter(pc for reference in references for pc in LITERAL.findall(reference))
    if sum(original.values()) != 492 or any(count != 1 for count in original.values()) or seen != original:
        raise ValueError('133B40 native references do not partition the 492 original literal sites')
    return references, sorted(original, key=lambda pc: int(pc, 16))


def install(canonical, emitted):
    check_dispatch(canonical)
    check_dispatch(emitted)
    references, _ = coverage(canonical)
    current = classifier.body(emitted)
    if not current or emitted.count(classifier.CALL) != 1 or any(emitted.count(ref) != 1 for ref in references[:-1]):
        raise ValueError('133B40 call requires all four native stage references and the original campaign caller')
    retained = LABEL + EMITTED
    installed = (LABEL + f'\n    if(!halo2::native_fog_plane_call_detail::bridge(s,{REFERENCE},\n'
                 + '        [](auto &state,std::uint32_t target,std::uint32_t pc,bool tail){dispatch(state,target,pc,tail);})) {\n'
                 + '        ' + EMITTED + '\n    }')
    reference = references[-1]
    if REFERENCE in emitted:
        if (emitted.count(INCLUDE) != 1 or emitted.count(reference) != 1 or
                current[0].count(installed) != 1 or current[0].count(classifier.GEOMETRY_CALL) != 1):
            raise ValueError('Prepared 133B40 vector-angle call changed')
        return emitted, ['133b40']
    if (current[0].count(retained) != 1 or current[0].count(classifier.GEOMETRY_CALL) != 1 or INCLUDE in emitted):
        raise ValueError('Specialized 133B40 vector-angle call changed')
    bound = current[0].replace(retained, installed, 1)
    return INCLUDE + emitted[:current.start()] + reference + '\n' + bound + emitted[current.end():], ['133b40']
