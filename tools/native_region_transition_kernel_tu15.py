"""Install the qualified native BC720 parent, retaining its literal fallback."""
import hashlib
import re

INCLUDE='''// BEGIN native region transition observation include
#include "halo2/source_physics_entry_observation.hpp"
// END native region transition observation include
'''
DIAGNOSTIC='''// BEGIN native region transition entry observation
SourcePhysicsEntryObservation region_transition_entry(s.memory,0xbc720u,[&]{return s.save();});
// END native region transition entry observation
'''
NATIVE_INCLUDE='''// BEGIN native region transition kernel include
#include "gameplay/objects/source_region_transition_native_tu15_internal.hpp"
// END native region transition kernel include
'''
NATIVE='''// BEGIN native region transition kernel
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    region_transition_native_tu15(s,[&](std::uint32_t pc) {
        if(s.services.instruction)s.services.instruction(pc);return pc;
    },[&](std::uint32_t pc,std::uint32_t target) {
        if(target==0x1704c0u)run_1704c0(s);
        else dispatch(s,target,pc,false);
    });
    return;
}
// END native region transition kernel
'''


def install_region_transition_observation(source):
    source=source.replace(INCLUDE,'').replace(DIAGNOSTIC,'').replace(NATIVE_INCLUDE,'').replace(NATIVE,'')
    anchor='static void run_bc720(WorldState &s) {\n';assert source.count(anchor)==1
    start=source.index(anchor);end=source.index('\n}',start)+2
    expected='e556b5bb561ab50746bd69083b9a54815b46037ed96af0070116a4319c8e8297'
    specialized='6cccdd27bf32508b8f317dc25489e6bc698c6617d3a7f67cb8054115f72dca6b'
    body=source[start:end]
    # The newer fixed-image lowerer folds selected(identity,TU15). Permit
    # exactly that authenticated textual specialization, without accepting
    # another instruction, operand, branch, callee or return change.
    normalized=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
    assert hashlib.sha256(body.encode()).hexdigest()in (expected,specialized),'Changed complete BC720 body; retain and re-audit'
    assert hashlib.sha256(normalized.encode()).hexdigest()==specialized
    include='#include "physics/runtime/physics_world_runtime_internal.hpp"\n';assert source.count(include)==1
    return source.replace(include,include+INCLUDE).replace(anchor,anchor+DIAGNOSTIC)


def install_region_transition_kernel(source):
    source=install_region_transition_observation(source)
    return source.replace(INCLUDE,INCLUDE+NATIVE_INCLUDE).replace(DIAGNOSTIC,DIAGNOSTIC+NATIVE)
