"""Install the semantic pending countdown in both preserved D8330 parents."""
import hashlib

INCLUDE='''// BEGIN native pending response include
#include "gameplay/objects/source_pending_response_native_tu15_internal.hpp"
// END native pending response include
'''
BEGIN='// BEGIN native pending response step\n'
END='// END native pending response step\n'
LABEL='// BEGIN native pending response continuation\nLd74dc: ;\n// END native pending response continuation\n'


def strip_native_pending_response(source):
    # Strip only our marked additions; the literal block remains byte-exact.
    source=source.replace(INCLUDE,'').replace(LABEL,'')
    if BEGIN in source:
        if source.count(BEGIN)!=1 or source.count(END)!=1:raise ValueError('Ambiguous pending native hook')
        start=source.index(BEGIN);end=source.index(END,start)+len(END);source=source[:start]+source[end:]
    source=source.replace('Ld74a0: ;\n','Ld74a0: ')
    if '// BEGIN native pending response' in source or '// END native pending response' in source:
        raise ValueError('Changed native pending-response hook; retain and re-audit')
    return source


def install_native_pending_response(source,observed):
    source=strip_native_pending_response(source)
    start=source.index('Ld74a0: ');end=source.index('\n',source.index('0xd74d6u',start))+1
    literal=source[start:end]
    expected='bc2a0a779d4fafb4206fd8d45569db3ef15f0d68a448b51fa7775329eaa28f3f' if observed else 'e93a0e5fd953bdb2dcbaa9500b7dead0bb3b1d2ba362bcf976cd6e8387c96ce5'
    if hashlib.sha256(literal.encode()).hexdigest()!=expected:raise ValueError('Preserved pending-response block changed')
    clock='''const auto pc=damage_observation_pc(canonical,actual,s);
        if(s.services.instruction)s.services.instruction(pc);
        return pc;''' if observed else '''(void)actual;
        if(s.services.instruction)s.services.instruction(canonical);
        return canonical;'''
    route=BEGIN+'''if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    const auto status=pending_response_step_native_tu15(s,[&](std::uint32_t canonical,std::uint32_t actual){
        '''+clock+'''
    });
    if(status!=NativeTu15PendingResponseStatus::Due)goto Ld758c;
    goto Ld74dc;
}
'''+END
    source=source[:start]+'Ld74a0: ;\n'+route+literal.removeprefix('Ld74a0: ')+LABEL+source[end:]
    anchor='#include "halo2/source_damage_delivery_runtime.hpp"\n' if observed else '#include "halo2/source_vitality_runtime.hpp"\n'
    if source.count(anchor)!=1:raise ValueError('Pending parent include owner changed')
    return source.replace(anchor,anchor+INCLUDE)
