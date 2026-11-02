"""Install qualified native 1704C0 control, retaining the exact literal body."""
import hashlib
import re

INCLUDE='''// BEGIN native model region state include
#include "gameplay/objects/source_model_region_state_native_tu15_internal.hpp"
#include "halo2/source_physics_entry_observation.hpp"
// END native model region state include
'''
NATIVE='''// BEGIN native model region state kernel
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    SourcePhysicsEntryObservation model_region_entry(s.memory,0x1704c0u,[&]{return s.save();});
    model_region_state_native_tu15(s,[&](std::uint32_t pc) {
        if(s.services.instruction)s.services.instruction(pc);return pc;
    },[&](std::uint32_t pc,std::uint32_t target) {
        switch(target) {
        case 0x17a0f0u:run_17a0f0(s);break;
        case 0x178d80u:run_178d80(s);break;
        case 0x178e30u:run_178e30(s);break;
        default:dispatch(s,target,pc,false);break;
        }
    });
    return;
}
// END native model region state kernel
'''


def install_model_region_state_kernel(source):
    source=source.replace(INCLUDE,'').replace(NATIVE,'')
    anchor='static void run_1704c0(WorldState &s) {\n';assert source.count(anchor)==1
    start=source.index(anchor);end=source.index('\n}',start)+2;body=source[start:end]
    literal='fdd58c7e8deadf2053ba8b4c3a0de9cfb81f1c0f7d1cd20f38e8a008363510d4'
    specialized='f5613df3415b7168cc9bfb6a50d5196e58015184156f71d3d5926cd5a046ae76'
    normalized=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
    assert hashlib.sha256(body.encode()).hexdigest()in (literal,specialized),'Changed complete1704C0; retain and re-audit'
    assert hashlib.sha256(normalized.encode()).hexdigest()==specialized
    include='#include "physics/runtime/physics_world_runtime_internal.hpp"\n';assert source.count(include)==1
    return source.replace(include,include+INCLUDE).replace(anchor,anchor+NATIVE)
