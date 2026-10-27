
namespace {
struct TextureCameraPrefixYield final {};
std::uint32_t prefix_word(M &m,std::uint32_t p) {return r(m,p,4);}
void prefix_frame_publish(M &m,const O &observe,std::span<const std::byte,0x298> packet) {
    // Literal132BC: A6 DWORDs from the actual completed backend packet.
    for(unsigned n=0;n<0xa6;++n) {
        std::uint32_t value;std::memcpy(&value,packet.data()+n*4u,4);
        w(m,observe,0x132c3u,0x491e20u+n*4u,4,value);
    }
    const std::uint16_t mode_bits=std::uint16_t(std::to_integer<unsigned>(packet[2])|
        (std::to_integer<unsigned>(packet[3])<<8));
    const auto mode=std::bit_cast<std::int16_t>(mode_bits);
    // Exact132C5..132E5 native target policy. This publishes an input enum;
    // no native image allocation or original GPU command is supplied.
    w(m,observe,mode==1?0x132cfu:0x132dbu,0x4920d8u,4,mode==1?18u:0u);
}
}
SourceTextureCameraScenePrefixTu15 source_capture_texture_camera_scene_prefix_tu15(
    M &memory,SourcePhysicsWorldRuntimeAbi &abi,FP fp,
    const SourcePhysicsWorldRuntimeServices &services,const O &observe) {
    SourceTextureCameraScenePrefixTu15 result;
    const auto input=prefix_word(memory,abi.gp[4]+4u);
    const auto texture=prefix_word(memory,abi.gp[4]+8u);
    if(texture>1u)throw std::runtime_error("Texture scene prefix requires an actual0/1 caller flag");
    result.texture_camera=texture!=0;result.original_abi=abi;
    result.frame_depth_before=std::bit_cast<std::int32_t>(prefix_word(memory,0x47357cu));
    if(result.frame_depth_before< -1 || result.frame_depth_before>=1)
        throw std::runtime_error("Texture scene prefix exceeds the original two-frame stack");
    auto wrapped=services;
    wrapped.required_owner=[&](M &m,const SourcePhysicsContactRuntimeCall &call,
        SourcePhysicsWorldRuntimeAbi &a,const O &o) {
        if(call.instruction==0x2bbfbu && call.target==0x132b0u && !call.tail) {
            if(result.backend_requested || prefix_word(m,a.gp[4])!=0x2bc00u)
                throw std::runtime_error("Texture scene prefix lost its exact backend call/return");
            const auto packet=prefix_word(m,a.gp[4]+4u);
            result.frame_depth_at_boundary=std::bit_cast<std::int32_t>(prefix_word(m,0x47357cu));
            if(result.frame_depth_at_boundary!=result.frame_depth_before+1 ||
               packet!=0x4918f0u+std::uint32_t(result.frame_depth_at_boundary)*0x298u)
                throw std::runtime_error("Texture scene prefix lost its actual source packet owner");
            m.read(packet,result.packet);m.read(input,result.completed_view);
            m.read(0x4c6710u,result.environment);m.read(0x4c65c0u,result.raw_camera);
            m.read(0x4c6634u,result.projection);
            result.actual_local_user=std::bit_cast<std::int32_t>(prefix_word(m,0x4c66f8u));
            result.continuation=a;result.backend_requested=true;result.memory_=&memory;
            result.serial_=prefix_word(memory,0x4c6858u);result.active_->store(true);
            throw TextureCameraPrefixYield{};
        }
        if(!services.required_owner)throw std::runtime_error("Texture scene CPU prefix reached an unowned engine call");
        services.required_owner(m,call,a,o);
    };
    FpScope scope(fp);LiveState state{memory,observe,wrapped,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try {run_2b770(state);} catch(const TextureCameraPrefixYield &) {
        // Deliberately a completed CPU-prefix result, not a completed2B770.
        // Save the suspended original ABI before returning to its typed owner.
        abi=state.save();result.continuation=abi;return result;
    } catch(...) {abi=state.save();throw;}
    abi=state.save();result.continuation=abi;
    // Original2B8F1..2B8FD can finish without issuing a backend view. Preserve
    // that real absence; the caller cannot call this a rendered frame.
    memory.read(input,result.completed_view);return result;
}
void source_publish_texture_camera_scene_prefix_tu15(M &memory,
    const SourceTextureCameraScenePrefixTu15 &prefix,const O &observe) {
    if(!prefix.backend_requested || prefix.memory_!=&memory || !prefix.active_->load() ||
       prefix.serial_!=prefix_word(memory,0x4c6858u) ||
       prefix.frame_depth_at_boundary!=std::bit_cast<std::int32_t>(prefix_word(memory,0x47357cu)))
        throw std::runtime_error("Texture scene prefix has no current actual backend packet");
    prefix_frame_publish(memory,observe,prefix.packet);
}
void source_finish_texture_camera_scene_prefix_tu15(M &memory,
    SourceTextureCameraScenePrefixTu15 &prefix,SourcePhysicsWorldRuntimeAbi &abi,FP fp,
    const SourcePhysicsWorldRuntimeServices &services,const O &observe) {
    prefix.require_current(memory);
    if(!prefix.backend_requested || prefix.finished ||
       std::bit_cast<std::int32_t>(prefix_word(memory,0x47357cu))!=prefix.frame_depth_at_boundary)
        throw std::runtime_error("Texture scene prefix has no current original cleanup owner");
    if(prefix_word(memory,0x4c686cu)!=1u)
        throw std::runtime_error("Texture scene cleanup requires the actual one-view campaign caller");
    auto wrapped=services;
    wrapped.required_owner=[&](M &m,const SourcePhysicsContactRuntimeCall &call,
        SourcePhysicsWorldRuntimeAbi &a,const O &o) {
        if(call.instruction==0x13c40u && call.target==0x132b0u && !call.tail) {
            if(prefix_word(m,a.gp[4])!=0x13c45u)throw std::runtime_error("Texture scene pop lost its source return");
            std::array<std::byte,0x298> previous;m.read(prefix_word(m,a.gp[4]+4u),previous);
            prefix_frame_publish(m,o,previous);
            // Native132B0 backend ABI is void/RET4. This retains the original
            // CPU packet publication; Vulkan state restoration is separately
            // owned by the actual renderer, never reported as an Xbox draw.
            a.gp[4]+=8u;return;
        }
        if(!services.required_owner)throw std::runtime_error("Texture scene cleanup reached an unowned engine call");
        services.required_owner(m,call,a,o);
    };
    FpScope scope(fp);LiveState state{memory,observe,wrapped,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try {run_13c00(state);} catch(...) {abi=state.save();prefix.active_->store(false);throw;}
    abi=state.save();
    if(std::bit_cast<std::int32_t>(prefix_word(memory,0x47357cu))!=prefix.frame_depth_before)
        throw std::runtime_error("Texture scene source13C00 did not restore frame-stack depth");
    prefix.finished=true;prefix.active_->store(false);
}
void SourceTextureCameraScenePrefixTu15::require_current(const M &memory)const {
    if(memory_!=&memory || !active_->load() || !backend_requested || finished ||
       serial_!=r(memory,0x4c6858u,4) || frame_depth_at_boundary!=
       std::bit_cast<std::int32_t>(r(memory,0x47357cu,4)))
        throw std::runtime_error("Texture camera CPU prefix scope is stale or belongs to another World");
    std::array<std::byte,0x298> current;memory.read(0x491e20u,current);
    if(current!=packet)throw std::runtime_error("Texture camera CPU prefix no longer owns the current source packet");
}
