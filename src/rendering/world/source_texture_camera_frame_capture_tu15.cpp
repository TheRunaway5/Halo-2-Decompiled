#include "halo2/source_texture_camera_frame_tu15.hpp"
#include "halo2/cache.hpp"
#include <map>

namespace halo2 {
void SourceTextureCameraNativeTargetsTu15::require_current()const {
    if(!current_)throw FormatError("Texture camera has no native target owner");
    current_();
}
void SourceTextureCameraNativeTargetsTu15::require_inputs(const SourceTextureCameraInputsTu15 &input)const {
    require_current();
    if(input.camera_words!=input_.camera_words || input.cinematic_auxiliary!=input_.cinematic_auxiliary ||
       input.cinematic!=input_.cinematic || input.texture_camera_view!=input_.texture_camera_view ||
       input.copy_complete!=input_.copy_complete || input.target18.allocation!=input_.target18.allocation ||
       input.target18.retiring!=input_.target18.retiring || input.target20.allocation!=input_.target20.allocation ||
       input.target20.retiring!=input_.target20.retiring)
        throw FormatError("Texture camera activation differs from its prepared native target generation");
    const bool needed=(input.cinematic_auxiliary && input.cinematic) ||
        (input.camera_words[0] && input.camera_words[0]!=0xffffffffu);
    if(needed!=target18_)
        throw FormatError("Texture camera18 allocation/retirement did not complete for its actual activation");
}
bool SourceTextureCameraNativeTargetsTu15::available(unsigned target)const {
    require_current();
    if(target!=18 && target!=20)throw FormatError("Texture camera target is outside original18/20");
    return target==18?target18_:target20_;
}
std::optional<unsigned> SourceTextureCameraNativeTargetsTu15::selected()const {
    require_current();if(target20_)return 20;if(target18_)return 18;return {};
}
namespace {
thread_local std::map<SourcePhysicsContactMemory *,SourceTextureCameraFrameTu15 *> captures;
unsigned word(const SourcePhysicsContactMemory &m,unsigned p) {
    std::array<std::byte,4> b;m.read(p,b);unsigned out=0;
    for(unsigned i=0;i<4;++i)out|=std::to_integer<unsigned>(b[i])<<(i*8);return out;
}
}
class SourceTextureCameraFrameCaptureTu15 {
public:
    static bool call(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRuntimeCall &call,
                     SourcePhysicsWorldRuntimeAbi &abi) {
        const auto found=captures.find(&memory);if(found==captures.end())return false;
        if(call.instruction!=0x258e9u && call.instruction!=0x258f7u && call.instruction!=0x25903u)return false;
        auto &frame=*found->second;frame.targets_.require_current();
        if(call.tail || word(memory,abi.gp[4])!=call.instruction+5u)
            throw FormatError("Texture camera backend lost the original25800 call/return frame");
        if(call.instruction==0x258e9u) {
            if(call.target!=0x2b770u || frame.view_ || word(memory,abi.gp[4]+8u)!=1u)
                throw FormatError("Texture camera scene request differs from original258E9");
            std::array<std::byte,0x118> view;memory.read(word(memory,abi.gp[4]+4u),view);
            if(word(memory,word(memory,abi.gp[4]+4u))!=0u)
                throw FormatError("Texture camera scene request is not an original world view");
            frame.view_=view;abi.gp[4]+=12u;return true;
        }
        if(call.instruction==0x258f7u) {
            if(call.target!=0x25940u || !frame.view_ || frame.selected_)
                throw FormatError("Texture camera target selection is not after its actual scene request");
            frame.selected_=true;abi.gp[0]=frame.targets_.selected().value_or(0xffffffffu);
            abi.gp[4]+=4u;return true;
        }
        const auto target=word(memory,abi.gp[4]+4u);
        if(call.target!=0x352d0u || !frame.view_ || !frame.selected_ || frame.copy_target_ ||
           word(memory,abi.gp[4]+8u)!=1u || frame.targets_.selected()!=target)
            throw FormatError("Texture camera copy request differs from original25903");
        frame.copy_target_=target;abi.gp[4]+=12u;return true;
    }
    static void begin(SourcePhysicsContactMemory &m,SourceTextureCameraFrameTu15 &frame) {
        if(!captures.emplace(&m,&frame).second)throw FormatError("Texture camera source frame is already being captured");
    }
    static void end(SourcePhysicsContactMemory &m)noexcept {captures.erase(&m);}
};
bool source_texture_camera_frame_call_tu15(SourcePhysicsContactMemory &m,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi) {
    return SourceTextureCameraFrameCaptureTu15::call(m,call,abi);
}
void begin_source_texture_camera_capture_tu15(SourcePhysicsContactMemory &m,SourceTextureCameraFrameTu15 &f) {
    SourceTextureCameraFrameCaptureTu15::begin(m,f);
}
void end_source_texture_camera_capture_tu15(SourcePhysicsContactMemory &m)noexcept {
    SourceTextureCameraFrameCaptureTu15::end(m);
}
}
