#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_particle_world_execution_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_particle_program_header_tu15.hpp"
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

namespace halo2 {
namespace {
constexpr std::uint32_t device=0x40f0c0,flags=device+8,mode=device+0x1928,alias=0x411568;
constexpr std::uint32_t depth_cell=device+0x1a08,depth_token=0xf9000381,depth_mode=0x4117b4,control_cache=0x4116c0;
constexpr std::uint32_t program_cell=device+0x794,program_token=0xf90003c0;
constexpr std::uint32_t program_shadow=device+0x10a8;
constexpr std::size_t program_shadow_bytes=136*16;
constexpr std::uint32_t declaration_input=0xf7810000,program_header=0x4125f8,dirty=0x411570;
constexpr std::size_t declaration_bytes=256;
constexpr std::size_t device_bytes=0x928*4;
bool within(std::uint32_t p,std::size_t n,std::uint32_t base,std::size_t size) {
    return p>=base && n<=size && std::size_t(p-base)<=size-n;
}
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {return Bytes(m.view(p,4)).u32(0);}
void verify_program_packets(std::span<const std::byte> packets,std::span<const std::byte> code) {
    const Bytes bytes(packets);
    if(packets.size()<8 || bytes.u32(0)!=0x41e9c || bytes.u32(4)!=0)
        throw FormatError("Particle program requires the original slot0 load method");
    std::size_t offset=4;
    for(std::size_t at=8;at<packets.size();) {
        if(packets.size()-at<4)throw FormatError("Truncated particle program method");
        const auto header=bytes.u32(at),count=header>>18;
        if(!count || count>32 || count%4 || header!=((count<<18)|0xb00u) ||
           count*4>packets.size()-at-4 || offset>code.size() || count*4>code.size()-offset)
            throw FormatError("Particle program method exceeds its owned instruction extent");
        if(!std::equal(packets.begin()+at+4,packets.begin()+at+4+count*4,code.begin()+offset))
            throw FormatError("Original program packet differs from its actual native shader code");
        offset+=count*4;at+=4+count*4;
    }
    if(offset!=code.size())throw FormatError("Particle program did not publish its complete instruction bank");
}
}
SourceParticleDepthLayoutTu15 source_particle_d24s8_layout_tu15(std::uint32_t width,std::uint32_t height) {
    if(!width || !height || width>4096 || height>4096)
        throw FormatError("Native D24S8 dimensions exceed the original linear surface layout");
    SourceRuntimeMemory memory;mount_source_particle_graphics_layout_data_tu15(memory);
    constexpr std::uint32_t scratch=0xf7600000,sp=scratch+0x800,stop=scratch+0xffc;
    memory.mount(scratch,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,
        "native original D24S8 layout arithmetic workspace");
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=sp;abi.eflags=0x202;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    memory.write(sp,std::as_bytes(std::span(&stop,1)));
    //42 is the authenticated cold native SDK D24S8 presentation field,
    // admitted by NativeTu15GraphicsDevice. Whole407860 owns normalization.
    abi.gp[0]=42;source_run_effect_particle_draw_tu15(memory,0x407860,abi,*abi.floating_point,{});
    SourceParticleDepthLayoutTu15 result;result.width=width;result.height=height;result.normalized_format=abi.gp[0];
    // Match the actual depth constructor's406E11 pitch producer and406E3A
    // layout call.406D87 belongs to the separate auxiliary color allocation.
    // An automatic64-byte row alignment alone can understate the original
    // pitch (1920 D24S8 pixels require8192 bytes, not7680).
    const std::array<std::uint32_t,2> pitch_args{stop,width};
    memory.write(sp,std::as_bytes(std::span(pitch_args)));abi.gp[4]=sp;abi.gp[0]=result.normalized_format;
    source_run_effect_particle_draw_tu15(memory,0x407900,abi,*abi.floating_point,{});result.row_pitch=abi.gp[0];
    // Declared native metadata caller: one depth slice/mip, no cube/volume,
    // plus actual dimensions and the whole original pitch producer. This
    // supplies metadata only and never allocates Xbox physical storage.
    const std::array<std::uint32_t,9> args{stop,1,1,result.normalized_format,result.row_pitch,0,0,scratch+0x100,scratch+0x104};
    memory.write(sp,std::as_bytes(std::span(args)));abi.gp[4]=sp;abi.gp[0]=width;abi.gp[2]=height;
    source_run_effect_particle_draw_tu15(memory,0x407ee0,abi,*abi.floating_point,{});
    result.storage_bytes=abi.gp[0];result.format_word=word(memory,scratch+0x100);result.size_word=word(memory,scratch+0x104);
    if(((result.format_word>>8)&255)<0x2a || ((result.format_word>>8)&255)>0x31)
        throw FormatError("Original D24S8 normalization escaped the depth-scale dispatch domain");
    //407A50 reads only the format byte of its declared surface-layout input.
    // No fake Data/Common/refcount/physical address is provided.
    memory.write(scratch+0x20c,std::as_bytes(std::span(&result.format_word,1)));
    memory.write(sp,std::as_bytes(std::span(&stop,1)));abi.gp[4]=sp;abi.gp[0]=scratch+0x108;abi.gp[1]=scratch+0x200;
    source_run_effect_particle_draw_tu15(memory,0x407a50,abi,*abi.floating_point,{});
    result.depth_scale_bits=word(memory,scratch+0x108);return result;
}
SourceParticleGraphicsBindingTu15::SourceParticleGraphicsBindingTu15(
    RendererDeviceLease renderer,RendererConstantMode control,RendererTargets::Use targets)
    :renderer_(std::move(renderer)),use_(renderer_.acquire()),control_(std::move(control)),targets_(std::move(targets)) {
    if(!control_.valid() || !targets_.valid() || control_.device_address()!=use_.native_address() ||
       targets_.device_address()!=use_.native_address())
        throw FormatError("Particle graphics constants belong to another native renderer");
    configuration_=control_.select(0x11);
    if(configuration_.mode!=0x11 || configuration_.registers!=192 || !configuration_.application_owned ||
       !configuration_.material_backing.value || !configuration_.extended_backing.value ||
       !configuration_.material_bytes || !configuration_.extended_bytes)
        throw FormatError("Particle graphics binding lacks actual native constant storage");
    // Native ABI construction has no enabled SDK device flags or configured
    // source mode yet. Access stays closed until whole400EF0 realizes the
    // real renderer's selected mode. No other405CF0 device state is invented.
    for(auto p:{flags,mode,alias})fields_.mount(p,std::array<std::byte,4>{},
        SourceRuntimeMemory::Access::ReadWrite,"live native particle graphics ABI field");
    fields_.write(alias,std::as_bytes(std::span(&device,1)));
    fields_.mount(program_shadow,std::vector<std::byte>(program_shadow_bytes),
        SourceRuntimeMemory::Access::ReadWrite,"native original particle program CPU cache workspace");
    fields_.mount(dirty,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native pending original declaration CPU dirty-mask workspace");
    fields_.mount(program_header+4,std::as_bytes(std::span(&source_particle_program_flags_tu15,1)),
        SourceRuntimeMemory::Access::ReadOnly,"original programmable-kind fragment in native declaration header");
    fields_.mount(program_header+20,std::vector<std::byte>(declaration_bytes),SourceRuntimeMemory::Access::ReadWrite,
        "native original declaration CPU output workspace");
    fields_.mount(declaration_input,std::vector<std::byte>(declaration_bytes),SourceRuntimeMemory::Access::ReadWrite,
        "native produced original particle declaration caller input");
}
void SourceParticleGraphicsBindingTu15::check(bool require_configuration) const {
    if(std::this_thread::get_id()!=thread_ || !renderer_.valid() || !control_.valid() || !targets_.valid() ||
       (depth_configured_ && !depth_control_.valid()) || (view_ && !view_->valid()) ||
       (program_staged_ && (!program_control_.valid() || program_configuration_.target_generation!=targets_.bank().generation)) ||
       control_.device_address()!=use_.native_address() || !begun_ ||
       (require_configuration && !configured_ && !configuring_))
        throw FormatError("Particle graphics ABI has no active renderer/thread/configuration owner");
}
void SourceParticleGraphicsBindingTu15::configure_depth(RendererDepthMode control) {
    check();
    if(depth_configured_ || !control.valid() || control.device_address()!=use_.native_address())
        throw FormatError("Particle depth binding has a duplicate or foreign native owner");
    const auto configured=control.select(2);const auto &attachment=targets_.bank().depth_stencil;
    if(configured.mode!=2 || configured.depth_bits!=24 || configured.stencil_bits!=8 ||
       !configured.state_backing.value || !configured.attachment_handle ||
       configured.attachment_handle!=attachment.image || configured.format!=attachment.format)
        throw FormatError("Particle depth binding does not name its actual D24S8 attachment");
    // This is a native opaque resource identity backed by the real target
    // use.3FF5F5 tests it for presence. The original format fragment below
    // is the only admitted surface metadata; no SDK allocation is invented.
    const auto layout=source_particle_d24s8_layout_tu15(attachment.width,attachment.height);
    fields_.mount(depth_cell,std::as_bytes(std::span(&depth_token,1)),SourceRuntimeMemory::Access::ReadOnly,
        "native particle actual depth attachment identity");
    fields_.mount(depth_mode,std::as_bytes(std::span(&configured.mode,1)),SourceRuntimeMemory::Access::ReadWrite,
        "native particle realized W-depth mode");
    // Only the original-format fragment has a layout owner. All other
    // fields of this native opaque identity remain unavailable.
    fields_.mount(depth_token+0xc,std::as_bytes(std::span(&layout.format_word,1)),SourceRuntimeMemory::Access::ReadOnly,
        "native actual D24S8 original format fragment");
    depth_control_=std::move(control);depth_configuration_=configured;depth_layout_=layout;depth_configured_=true;
}
const RendererDepthConfiguration &SourceParticleGraphicsBindingTu15::depth_configuration() const {
    check();if(!depth_configured_)throw FormatError("Particle graphics depth is not configured");
    return depth_configuration_;
}
const SourceParticleDepthLayoutTu15 &SourceParticleGraphicsBindingTu15::depth_layout() const {
    check();if(!depth_configured_)throw FormatError("Particle graphics depth layout is not configured");
    return depth_layout_;
}
void SourceParticleGraphicsBindingTu15::stage_program(RendererParticleProgram control,
    std::shared_ptr<const ParticleMaterial> material) {
    check();if(program_staged_ || !control.valid() || control.device_address()!=use_.native_address() || !material)
        throw FormatError("Particle program selection has a duplicate, foreign or unissued owner");
    const auto configuration=control.select(material);std::uint64_t hash=14695981039346656037ull;
    for(auto byte:material->vertex.program)hash=(hash^std::to_integer<unsigned>(byte))*1099511628211ull;
    if(!configuration.pipeline || !configuration.vertex_module || !configuration.layout || !configuration.state_backing.value ||
       configuration.target_generation!=targets_.bank().generation || configuration.program_hash!=hash ||
       configuration.classification!=material->vertex.classification || !configuration.instructions || configuration.instructions>136)
        throw FormatError("Particle program lacks its actual source-validated native pipeline");
    fields_.mount(program_cell,std::as_bytes(std::span(&program_token,1)),SourceRuntimeMemory::Access::ReadWrite,
        "native staged particle program identity");
    fields_.mount(program_cell+4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native source declaration handle publication workspace");
    const std::uint32_t start=0;
    fields_.mount(program_cell+8,std::as_bytes(std::span(&start,1)),SourceRuntimeMemory::Access::ReadWrite,
        "native original program instruction0 entry");
    fields_.mount(program_token+4,std::as_bytes(std::span(&source_particle_program_flags_tu15,1)),SourceRuntimeMemory::Access::ReadOnly,
        "original programmable-kind fragment backed by actual particle program");
    selected_vertex_=material->vertex;
    program_control_=std::move(control);program_configuration_=configuration;program_staged_=true;
}
const RendererParticleProgramConfiguration &SourceParticleGraphicsBindingTu15::program_configuration() const {
    check();if(!program_staged_)throw FormatError("Particle graphics program has not been staged");return program_configuration_;
}
void SourceParticleGraphicsBindingTu15::configure_program_declaration(
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsWorldRuntimeServices &services) {
    check();
    if(!program_staged_ || declaration_attempted_)
        throw FormatError("Particle declaration has no selected program or was already attempted");
    const auto produced=source_particle_declaration_tu15(*this,selected_vertex_);
    fields_.write(declaration_input,std::as_bytes(std::span(produced.elements)));
    // A declared native same-kind caller. The GPU program is already resident
    // at its original instruction0; no pretransformed kind change is admitted.
    abi.gp[0]=0;abi.gp[1]=declaration_input;
    declaration_packet_begin_=constants_.commands().size();declaration_attempted_=true;
    declaration_configuring_=true;
    try {
        run(0x400dd0,abi,services);declaration_configuring_=false;
        declaration_packet_end_=constants_.commands().size();
        const auto packet=constants_.commands().subspan(declaration_packet_begin_,
            declaration_packet_end_-declaration_packet_begin_);
        if(packet.size()!=8 || Bytes(packet).u32(0)!=0x41ea0 || Bytes(packet).u32(4)!=0 ||
           word(fields_,program_cell)!=program_header || word(fields_,program_cell+4)!=program_header+1 ||
           word(fields_,program_cell+8)!=0 || word(fields_,dirty)!=0x70 ||
           !std::equal(fields_.view(program_header+20,declaration_bytes).begin(),
               fields_.view(program_header+20,declaration_bytes).end(),
               std::as_bytes(std::span(produced.elements)).begin()))
            throw FormatError("Original particle declaration/start publication differs from its native program");
        declaration_=produced;declaration_configured_=true;
    }catch(...) {declaration_configuring_=false;throw;}
}
const SourceParticleDeclarationTu15 &SourceParticleGraphicsBindingTu15::program_declaration() const {
    check();if(!declaration_configured_)throw FormatError("Particle original declaration is not configured");
    return declaration_;
}
void SourceParticleGraphicsBindingTu15::load_program(RendererParticleProgram control,
    std::shared_ptr<const ParticleMaterial> material,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services) {
    check();
    if(program_staged_ || program_load_attempted_ || !control.valid() ||
       control.device_address()!=use_.native_address() || !material)
        throw FormatError("Particle program load has a repeated, foreign or unissued owner");
    const auto decoded=decode_vertex_program(material->vertex.program);
    std::array<std::uint32_t,2> args;
    constants_.read(abi.gp[4]+4,std::as_writable_bytes(std::span(args)));
    if(args[1]!=0)throw FormatError("Native particle program only owns original slot0");
    std::vector<std::byte> code(material->vertex.program.size());constants_.read(args[0],code);
    if(code!=material->vertex.program)
        throw FormatError("Original particle program source differs from the native material");
    // This is native CPU cache workspace. Only the original REP producer's
    // before-image observer may read its initial allocation bytes. No source
    // CPU reader is allowed to mistake unwritten tail words for SDK state.
    program_code_=std::move(code);program_load_attempted_=true;
    program_packet_begin_=constants_.commands().size();program_loading_=true;
    try {
        run(0x400d40,abi,services);program_loading_=false;
        program_packet_end_=constants_.commands().size();
        verify_program_packets(constants_.commands().subspan(program_packet_begin_,
            program_packet_end_-program_packet_begin_),program_code_);
        if(!std::equal(fields_.view(program_shadow,decoded.count*16).begin(),
            fields_.view(program_shadow,decoded.count*16).end(),program_code_.begin()+4))
            throw FormatError("Original particle CPU program cache differs from emitted instructions");
        stage_program(std::move(control),std::move(material));
        if(program_configuration_.instructions!=decoded.count)
            throw FormatError("Native particle program instruction count differs from its original loader");
    }catch(...) {program_loading_=false;throw;}
}
bool SourceParticleGraphicsBindingTu15::configured() const {
    return configured_ && renderer_.valid() && control_.valid() && targets_.valid() &&
        (!view_ || view_->valid()) && std::this_thread::get_id()==thread_;
}
NativeRuntimeAddress SourceParticleGraphicsBindingTu15::device_address() const {
    check();return use_.native_address();
}
void SourceParticleGraphicsBindingTu15::begin(SourcePhysicsContactMemory &source,
    std::span<const ShaderVector,192> inherited) {
    if(view_ || &source==this || std::this_thread::get_id()!=thread_ || !renderer_.valid() || !control_.valid() || !targets_.valid())
        throw FormatError("Particle graphics begin has no live renderer/thread");
    constants_.begin(source,inherited);program_packet_begin_=program_packet_end_=0;
    declaration_packet_begin_=declaration_packet_end_=0;begun_=true;
}
void SourceParticleGraphicsBindingTu15::stage_view(RendererParticleView::Use view) {
    check();
    if(view_ || !depth_configured_ || !program_staged_ || !view.valid() ||
       view.device_address()!=use_.native_address())
        throw FormatError("Particle view requires one actual same-renderer depth/program/view owner");
    const auto &v=view.state();const auto &c=v.viewport_constants;
    if(v.prepared && view.current_state().published)
        throw FormatError("Prepared particle view already has its original camera publication");
    const bool compatible=v.prepared? v.layout==program_configuration_.layout &&
        program_configuration_.classification>=7 && program_configuration_.classification<=9:
        v.pipeline==program_configuration_.pipeline;
    if(v.generation!=targets_.bank().generation || !compatible ||
       v.viewport!=std::array<float,6>{0,0,float(v.surface_width),float(v.surface_height),0,1} ||
       v.depth_parameters[2]!=1 || v.depth_parameters[3]!=1)
        throw FormatError("Particle view is outside the admitted full-surface programmable W path");
    const float width=c[0][0]*2,height=-c[1][1]*2;
    const float left=c[3][0]-width*.5f-.5f,top=c[3][1]-height*.5f-.5f;
    if(!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(left) || !std::isfinite(top) ||
       width<=0 || height<=0 || left<0 || top<0 || left+width>v.surface_width || top+height>v.surface_height ||
       std::trunc(width)!=width || std::trunc(height)!=height || std::trunc(left)!=left || std::trunc(top)!=top ||
       v.scissor!=std::array<std::int32_t,4>{std::int32_t(left),std::int32_t(top),std::int32_t(width),std::int32_t(height)} ||
       c!=std::array<ShaderVector,4>{{{width*.5f,0,0,left+(width+1)*.5f+.03125f},
           {0,-height*.5f,0,top+(height+1)*.5f+.03125f},{width*.5f,-height*.5f,16777215,0},
           {left+width*.5f+.5f,top+height*.5f+.5f,0,0}}})
        throw FormatError("Particle recorded scissor and source viewport constants disagree");
    std::array<float,2> planes;constants_.read(0x4922f4,std::as_writable_bytes(std::span(planes)));
    if(v.vertex_clip[0]!=0 || v.vertex_clip[1]!=16777215.f ||
       v.depth_clip[0]!=0 || v.depth_clip[1]!=16777215.f || !std::isfinite(planes[0]) ||
       !std::isfinite(planes[1]) || planes[0]<=0 || planes[1]<=planes[0])
        throw FormatError("Particle native view does not match its original169D0 near/far inputs");
    // A reset after collection would erase already authored work. Do not
    // restore those counters later or call the initializer on a captured queue.
    require_empty_view_queues();
    const auto mount=[&](unsigned p,auto value){fields_.mount(p,std::as_bytes(std::span(&value,1)),
        SourceRuntimeMemory::Access::ReadWrite,"recorded native programmable particle view ABI fragment");};
    const std::array<unsigned,6> viewport{unsigned(left),unsigned(top),unsigned(width),unsigned(height),0,0x3f800000};
    fields_.mount(device+0xee0,std::as_bytes(std::span(viewport)),SourceRuntimeMemory::Access::ReadWrite,
        "recorded native particle view rectangle and viewport depth interval");
    mount(device+0x95c,1.f);mount(device+0x960,1.f);
    // Original405B10's literal raster offsets agree with actual c8/c9 above.
    mount(device+0xef8,.53125f);mount(device+0xefc,.53125f);
    mount(device+0x948,depth_layout_.depth_scale_bits);
    mount(device+0x1c10,v.depth_clip[2]);mount(device+0x1c14,v.depth_clip[3]);
    // Native shader inputs always supply explicit programmable clip bounds.
    // This narrow ABI flag represents that fact; no Xbox SDK prior flags or
    // fixed-function bit0 history is inferred or exposed to other readers.
    mount(device+0x1c18,2u);mount(control_cache,0u);
    view_=std::move(view);
}
void SourceParticleGraphicsBindingTu15::require_empty_view_queues() const {
    std::array<std::byte,4> count;
    for(auto p:{0x4c2aa0u,0x4c2aa4u}){constants_.read(p,count);if(Bytes(count).u32(0))
        throw FormatError("Original particle view must initialize before queue construction");}
}
void SourceParticleGraphicsBindingTu15::check_view_initialization() const {
    check();
    if(!view_ || view_attempted_ || view_initialized_ || view_initializing_ || (camera_attempted_ && !camera_configured_))
        throw FormatError("Particle view requires one complete pending initializer");
    if(view_->state().prepared && view_->current_state().published)
        throw FormatError("Prepared particle view already has its original camera publication");
    require_empty_view_queues();
}
void SourceParticleGraphicsBindingTu15::prepare_view(RendererParticleView::Use view) {
    stage_view(std::move(view));
}
void SourceParticleGraphicsBindingTu15::initialize_staged_view(SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services) {
    check_view_initialization();
    view_attempted_=true;view_initializing_=true;view_packet_begin_=constants_.commands().size();
    try {
        run(0x13430,abi,services);view_packet_end_=constants_.commands().size();view_initializing_=false;
        const auto packet=constants_.commands().subspan(view_packet_begin_,view_packet_end_-view_packet_begin_);
        if(packet.size()!=20 || Bytes(packet).u32(0)!=0x80394 || Bytes(packet).u32(12)!=0x41d78 ||
           Bytes(packet).u32(16)!=0 || !control_cache_owned_)
            throw FormatError("Original particle view did not publish its complete bounds/control methods");
        view_initialized_=true;publish_view_resource();
    }catch(...) {view_initializing_=false;throw;}
}
void SourceParticleGraphicsBindingTu15::initialize_view(RendererParticleView::Use view,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsWorldRuntimeServices &services) {
    stage_view(std::move(view));initialize_staged_view(abi,services);
}
SourceParticleViewPassTu15 SourceParticleGraphicsBindingTu15::initialize_view_pass(RendererParticleView::Use view,
    std::uint32_t mode,std::uint32_t distance_bits,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services) {
    stage_view(std::move(view));
    return initialize_prepared_view_pass(mode,distance_bits,abi,services);
}
SourceParticleViewPassTu15 SourceParticleGraphicsBindingTu15::initialize_prepared_view_pass(
    std::uint32_t mode,std::uint32_t distance_bits,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services) {
    check_view_initialization();
    try {
        std::array<std::byte,12> caller_packet;read(abi.gp[1],caller_packet);
        const auto packet=source_particle_view_pass_tu15(*this,mode,distance_bits);
        write(abi.gp[1],std::as_bytes(std::span(packet.words)));
        initialize_staged_view(abi,services);return packet;
    }catch(...) {view_attempted_=true;throw;}
}
void SourceParticleGraphicsBindingTu15::configure(SourcePhysicsWorldRuntimeAbi &abi,
    const std::function<void(std::uint32_t)> &instruction) {
    check(false);
    if(configured_ || configuring_ || abi.gp[0]!=configuration_.mode)
        throw FormatError("Particle graphics constant-mode caller differs or was repeated");
    configuring_=true;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=instruction;
    services.required_owner=[](auto &,const auto &,auto &,const auto &) {
        throw FormatError("Native particle constant mode reached an unowned SDK dependency");
    };
    try {
        run(0x400ef0,abi,services);
        if(word(fields_,flags)!=0x200 || word(fields_,mode)!=1)
            throw FormatError("Original source constant mode differs from its real native binding");
        configuring_=false;configured_=true;
    }catch(...) {configuring_=false;throw;}
}
void SourceParticleGraphicsBindingTu15::publish_camera(SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services) {
    check();if(!view_ || (view_attempted_ && !view_initialized_) || camera_attempted_ || !abi.gp[0] || !abi.gp[1])
        throw FormatError("Particle camera requires its actual staged view and one complete native caller");
    camera_attempted_=true;camera_initializing_=true;camera_bounds_begin_=constants_.commands().size();
    try {
        run(0x12f80,abi,services);camera_bounds_end_=camera_bounds_begin_+12;
        const auto packets=constants_.commands();
        if(packets.size()<camera_bounds_end_ || Bytes(packets).u32(camera_bounds_begin_)!=0x80394)
            throw FormatError("Original camera omitted its complete depth-bounds method");
        std::array<ShaderVector,192> produced;decode_constants(produced,true);
        if(std::memcmp(produced.data(),view_->state().camera_constants.data(),12*sizeof(ShaderVector)))
            throw FormatError("Original camera publisher differs from the actual native descriptor camera bank");
        camera_configured_=true;camera_initializing_=false;publish_view_resource();
    }catch(...) {camera_initializing_=false;throw;}
}
void SourceParticleGraphicsBindingTu15::run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services) {
    check();
    if(!source_effect_particle_draw_tu15_owns_entry(entry))
        throw FormatError("Particle graphics root requires its owned particle entry");
    const auto previous=instruction_;auto wrapped=services;
    wrapped.instruction=[&](auto p) {instruction_=p;if(services.instruction)services.instruction(p);};
    try {
        source_run_particle_world_execution_tu15(*this,entry,abi,
            abi.floating_point.value_or(SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80}),wrapped);
    }catch(...) {instruction_=previous;throw;}
    instruction_=previous;
}
bool SourceParticleGraphicsBindingTu15::owns_field(std::uint32_t p,std::size_t n) const {
    return within(p,n,flags,4) || within(p,n,mode,4) || within(p,n,alias,4) ||
        (depth_configured_ && (within(p,n,depth_cell,4) || within(p,n,depth_mode,4) || within(p,n,depth_token+0xc,4))) ||
        (program_staged_ && (within(p,n,program_cell,12) || within(p,n,program_token+4,4) ||
            within(p,n,program_header+4,4) || within(p,n,program_header+20,declaration_bytes))) ||
        (declaration_configuring_ && (within(p,n,dirty,4) || within(p,n,declaration_input,declaration_bytes))) ||
        (program_load_attempted_ && within(p,n,program_shadow,program_shadow_bytes)) ||
        (view_ && (within(p,n,device+0xee0,24) || within(p,n,device+0x95c,8) ||
            within(p,n,device+0xef8,8) || within(p,n,device+0x948,4) ||
            within(p,n,device+0x1c10,12) || within(p,n,control_cache,4)));
}
void SourceParticleGraphicsBindingTu15::reject_unknown_device_field(std::uint32_t p,std::size_t n) const {
    if(n>0x100000000ull-p)throw FormatError("Particle graphics ABI access wraps source storage");
    if(p<std::uint64_t(device)+device_bytes && std::uint64_t(p)+n>device &&
       !within(p,n,device,8)) {
        std::ostringstream error;error<<"Particle graphics requires unowned original device field0x"<<std::hex<<p;
        throw FormatError(error.str());
    }
    if(p<std::uint64_t(alias)+4 && std::uint64_t(p)+n>alias)
        throw FormatError("Particle graphics access crosses its device alias owner");
}
void SourceParticleGraphicsBindingTu15::read(std::uint32_t p,std::span<std::byte> out) const {
    check();
    if(p<std::uint64_t(dirty)+4 && std::uint64_t(p)+out.size()>dirty &&
       !(declaration_configuring_ && (instruction_==0x400e3b || instruction_==0x400e51) && p==dirty && out.size()==4))
        throw FormatError("Particle declaration dirty mask requires its original same-kind owner");
    if(p<std::uint64_t(declaration_input)+declaration_bytes && std::uint64_t(p)+out.size()>declaration_input &&
       !(declaration_configuring_ && instruction_==0x400ded && within(p,out.size(),declaration_input,declaration_bytes)))
        throw FormatError("Particle declaration caller input requires its original copy owner");
    if(p<std::uint64_t(program_header)+280 && std::uint64_t(p)+out.size()>program_header &&
       !(program_staged_ && (
         (p==program_header+4 && ((out.size()==1 && (instruction_==0x40027c || instruction_==0x401026)) ||
          (declaration_configuring_ && out.size()==4 && instruction_==0x400e63))) ||
         (declaration_configuring_ && instruction_==0x400ded && within(p,out.size(),program_header+20,declaration_bytes)))))
        throw FormatError("Particle declaration header exposes only its produced kind/input fragments");
    if(p<std::uint64_t(program_shadow)+program_shadow_bytes && std::uint64_t(p)+out.size()>program_shadow &&
       !(program_loading_ && instruction_==0x400d6f && within(p,out.size(),program_shadow,program_code_.size()-4)))
        throw FormatError("Particle program CPU cache requires its original producer observer");
    if(within(p,out.size(),program_cell,4) &&
       !(program_staged_ && (instruction_==0x400276 || instruction_==0x401020 ||
            (declaration_configuring_ && (instruction_==0x400e41 || instruction_==0x400e57))) && p==program_cell && out.size()==4))
        throw FormatError("Particle program pointer requires an unowned source reader");
    if(p<std::uint64_t(program_cell)+12 && std::uint64_t(p)+out.size()>program_cell+4 &&
       !(declaration_configuring_ && out.size()==4 &&
         ((p==program_cell+4 && instruction_==0x400e5d) || (p==program_cell+8 && instruction_==0x400edc))))
        throw FormatError("Particle declaration handle/start requires its original store observer");
    if(p<std::uint64_t(program_token)+0x118 && std::uint64_t(p)+out.size()>program_token &&
       !(program_staged_ && p==program_token+4 &&
         ((out.size()==1 && (instruction_==0x40027c || instruction_==0x401026)) ||
          (declaration_configuring_ && instruction_==0x400e47 && out.size()==4))))
        throw FormatError("Particle program identity exposes only its owned programmable-kind reader");
    if(instruction_==0x13438 || instruction_==0x13463) {
        std::array<float,2> planes;constants_.read(0x4922f4,std::as_writable_bytes(std::span(planes)));
        if(!std::isfinite(planes[0]) || !std::isfinite(planes[1]) || planes[1]<=0)
            throw FormatError("Original particle view lacks configured near/far producer169D0");
    }
    if(view_ && (within(p,out.size(),device+0xee0,24) || within(p,out.size(),device+0x95c,8) ||
        within(p,out.size(),device+0xef8,8) || within(p,out.size(),device+0x948,4) ||
        within(p,out.size(),device+0x1c10,12)) && !view_initializing_ && !camera_initializing_)
        throw FormatError("Particle view cache has no admitted original reader outside its initializer");
    if(p<std::uint64_t(control_cache)+4 && std::uint64_t(p)+out.size()>control_cache &&
       !(view_ && p==control_cache && out.size()==4 &&
         ((view_initializing_ && instruction_==0x135de) || (control_cache_owned_ && instruction_==0x3ff611))))
        throw FormatError("Original particle depth control cache requires its13430 per-view owner");
    if(p<std::uint64_t(depth_token)+0x20 && std::uint64_t(p)+out.size()>depth_token &&
       !(depth_configured_ && instruction_==0x4004a0 && p==depth_token+0xd && out.size()==1))
        throw FormatError("Particle depth identity has no owned reader beyond its original format fragment");
    if(within(p,out.size(),depth_cell,4) &&
       !(depth_configured_ && (instruction_==0x3ff5f5 || instruction_==0x400496) && p==depth_cell && out.size()==4))
        throw FormatError("Particle depth resource requires an unowned source reader");
    // This prefix owns only the constant-mode flag and the non-pure CPU
    // shadow behavior. Do not let a later SDK reader interpret all remaining
    //405CF0 flags as zero. Only these authenticated instructions consume the
    // admitted bits; another flag reader requires its real device owner.
    if(within(p,out.size(),flags,4) &&
       !((configuring_ && (instruction_==0x400ef9 || instruction_==0x400f0f) && p==flags && out.size()==4) ||
         ((instruction_==0x247e32 || (program_loading_ && instruction_==0x400d50)) && p==flags && out.size()==1) ||
         ((view_initializing_ || camera_initializing_) && (instruction_==0x400188 || instruction_==0x400293) && p==flags && out.size()==4) ||
         (camera_initializing_ && instruction_==0x13256 && p==flags && out.size()==1)))
    {
        //400F0F's read is the native store observer's before-image, not an
        // additional original CPU load.
        std::ostringstream error;error<<"Particle graphics requires unowned device flag reader0x"<<std::hex<<instruction_;
        throw FormatError(error.str());
    }
    if(owns_field(p,out.size()))fields_.read(p,out);
    else {reject_unknown_device_field(p,out.size());constants_.read(p,out);}
}
void SourceParticleGraphicsBindingTu15::write(std::uint32_t p,std::span<const std::byte> in) {
    check();
    if(view_ && (within(p,in.size(),device+0xee0,24) || within(p,in.size(),device+0x95c,8) ||
        within(p,in.size(),device+0xef8,8) || within(p,in.size(),device+0x948,4) ||
        within(p,in.size(),device+0x1c10,12) || within(p,in.size(),control_cache,4))) {
        if(in.size()!=4 || !((view_initializing_ &&
           ((p==device+0x1c10 && instruction_==0x1359a) || (p==device+0x1c14 && instruction_==0x135a2) ||
             (p==device+0x1c18 && instruction_==0x135aa && Bytes(in).u32(0)==2) ||
             (p==control_cache && instruction_==0x135de && Bytes(in).u32(0)==0))) ||
           (camera_initializing_ && ((p==device+0x1c10 && instruction_==0x12d79) ||
             (p==device+0x1c14 && instruction_==0x12d81) ||
             (p==device+0x1c18 && instruction_==0x12d89 && Bytes(in).u32(0)==2)))))
            throw FormatError("Particle view field lacks its whole original initializer producer");
        fields_.write(p,in);if(p==control_cache)control_cache_owned_=true;return;
    }
    if((p<std::uint64_t(dirty)+4 && std::uint64_t(p)+in.size()>dirty) ||
       (p<std::uint64_t(program_header)+280 && std::uint64_t(p)+in.size()>program_header) ||
       (p<std::uint64_t(program_cell)+12 && std::uint64_t(p)+in.size()>program_cell)) {
        const bool allowed=declaration_configuring_ && (
            (p==dirty && in.size()==4 && instruction_==0x400e51 && Bytes(in).u32(0)==0x70) ||
            (p==program_cell && in.size()==4 && instruction_==0x400e57 && Bytes(in).u32(0)==program_header) ||
            (p==program_cell+4 && in.size()==4 && instruction_==0x400e5d && Bytes(in).u32(0)==program_header+1) ||
            (p==program_cell+8 && in.size()==4 && instruction_==0x400edc && Bytes(in).u32(0)==0) ||
            (instruction_==0x400ded && within(p,in.size(),program_header+20,declaration_bytes)));
        if(!allowed)throw FormatError("Particle declaration write lacks its original same-kind publication owner");
        fields_.write(p,in);return;
    }
    if(p<std::uint64_t(program_shadow)+program_shadow_bytes && std::uint64_t(p)+in.size()>program_shadow) {
        if(!program_loading_ || instruction_!=0x400d6f || !within(p,in.size(),program_shadow,program_code_.size()-4))
            throw FormatError("Particle program CPU cache write lacks its original REP producer");
        fields_.write(p,in);return;
    }
    if((p<std::uint64_t(program_cell)+4 && std::uint64_t(p)+in.size()>program_cell) ||
       (p<std::uint64_t(program_token)+0x118 && std::uint64_t(p)+in.size()>program_token))
        throw FormatError("Particle staged program has no original mutable declaration owner");
    if(p<std::uint64_t(control_cache)+4 && std::uint64_t(p)+in.size()>control_cache)
        throw FormatError("Original particle control cache has no per-view writer owner");
    if(p<std::uint64_t(depth_token)+0x20 && std::uint64_t(p)+in.size()>depth_token)
        throw FormatError("Opaque particle depth identity has no writable Xbox surface header");
    if(depth_configured_ && p==depth_mode && in.size()==4) {
        if(instruction_!=0x3ff62e || Bytes(in).u32(0)!=depth_configuration_.mode)
            throw FormatError("Particle depth mode requires its original/native state owner");
        fields_.write(p,in);return;
    }
    if(owns_field(p,in.size())) {
        if(!configuring_ || in.size()!=4 ||
           !((p==flags && Bytes(in).u32(0)==0x200) || (p==mode && Bytes(in).u32(0)==1)))
            throw FormatError("Particle graphics field write has no original constant-mode owner");
        fields_.write(p,in);
    }else {reject_unknown_device_field(p,in.size());constants_.write(p,in);}
}
std::span<const std::byte> SourceParticleGraphicsBindingTu15::commands() const {
    check();return constants_.commands();
}
void SourceParticleGraphicsBindingTu15::consume_constants(std::span<ShaderVector,192> destination) const {
    decode_constants(destination,false);
}
void SourceParticleGraphicsBindingTu15::decode_constants(std::span<ShaderVector,192> destination,
    bool pending_camera) const {
    check();std::array<ShaderVector,192> staged;
    if((program_load_attempted_ && !program_staged_) || (declaration_attempted_ && !declaration_configured_) ||
       (view_ && !view_initialized_ && !(pending_camera && camera_initializing_)) ||
       (camera_attempted_ && !camera_configured_ && !camera_initializing_))
        throw FormatError("Particle program packets have no realized native pipeline owner");
    if(program_packet_end_ || declaration_packet_end_ || view_packet_end_ || camera_bounds_end_) {
        const auto packets=constants_.commands();
        if(program_packet_begin_>program_packet_end_ || program_packet_end_>packets.size())
            throw FormatError("Particle program packet range has no complete emission owner");
        if(program_packet_end_)verify_program_packets(packets.subspan(program_packet_begin_,program_packet_end_-program_packet_begin_),program_code_);
        if(declaration_packet_end_) {
            if(declaration_packet_begin_<program_packet_end_ || declaration_packet_end_-declaration_packet_begin_!=8 ||
               declaration_packet_end_>packets.size() || Bytes(packets).u32(declaration_packet_begin_)!=0x41ea0 ||
               Bytes(packets).u32(declaration_packet_begin_+4)!=0)
                throw FormatError("Particle declaration packet has no matching instruction0 owner");
        }
        if(view_packet_end_ && (view_packet_begin_<std::max(program_packet_end_,declaration_packet_end_) ||
           view_packet_end_>packets.size() || view_packet_end_-view_packet_begin_!=20 ||
           Bytes(packets).u32(view_packet_begin_)!=0x80394 || Bytes(packets).u32(view_packet_begin_+12)!=0x41d78 ||
           Bytes(packets).u32(view_packet_begin_+16)!=0))
            throw FormatError("Particle view packet has no complete original bounds/control owner");
        if(camera_bounds_end_ && (camera_bounds_begin_<std::max(program_packet_end_,declaration_packet_end_) || camera_bounds_end_>packets.size() ||
           camera_bounds_end_-camera_bounds_begin_!=12 || Bytes(packets).u32(camera_bounds_begin_)!=0x80394))
            throw FormatError("Particle camera bounds method has no complete original publication owner");
        std::array<std::pair<std::size_t,std::size_t>,4> ranges{{
            {program_packet_begin_,program_packet_end_},{declaration_packet_begin_,declaration_packet_end_},
            {view_packet_begin_,view_packet_end_},{camera_bounds_begin_,camera_bounds_end_}}};
        std::sort(ranges.begin(),ranges.end());
        std::vector<std::byte> constants;std::size_t at=0;
        for(const auto [first,last]:ranges) {
            if(!last)continue;
            if(first<at || first>last || last>packets.size())
                throw FormatError("Particle method owners overlap or escape their emitted stream");
            constants.insert(constants.end(),packets.begin()+at,packets.begin()+first);at=last;
        }
        constants.insert(constants.end(),packets.begin()+at,packets.end());
        constants_.consume_packet(constants,staged);
    }else constants_.consume(staged);
    check();
    std::copy(staged.begin(),staged.end(),destination.begin());
}
std::size_t SourceParticleGraphicsBindingTu15::last_bounds_packet() const {
    return camera_bounds_end_>view_packet_end_?camera_bounds_begin_:view_packet_begin_;
}
void SourceParticleGraphicsBindingTu15::publish_view_resource() {
    if(!view_initialized_ || !camera_configured_ || !view_->state().prepared)return;
    std::array<ShaderVector,192> produced;consume_constants(produced);
    auto clip=view_->state().vertex_clip;const Bytes packets(constants_.commands());
    const auto bounds=last_bounds_packet();clip[2]=packets.f32(bounds+4);clip[3]=packets.f32(bounds+8);
    if(!std::isfinite(clip[2]) || !std::isfinite(clip[3]) || clip[2]<0 || clip[3]<clip[2] || clip[3]>16777215.f)
        throw FormatError("Original particle clip packet exceeds its native W-depth domain");
    view_->publish(std::span<const ShaderVector,12>(produced.data(),12),clip);
}
void SourceParticleGraphicsBindingTu15::consume_draw(ParticleDraw &draw) const {
    check();if(!view_initialized_ || !view_ || !draw.material ||
       draw.material->vertex.classification!=selected_vertex_.classification ||
       draw.material->vertex.program!=selected_vertex_.program)
        throw FormatError("Particle draw lacks its native view and selected source program");
    const auto &v=view_->state();
    if(v.prepared && !camera_configured_)throw FormatError("Prepared particle view lacks its complete original camera publisher");
    if(!std::equal(v.viewport_constants.begin(),v.viewport_constants.end(),draw.vertex.constants.begin()+8) ||
       draw.vertex.clip_range!=v.vertex_clip || draw.depth.clip_range!=v.depth_clip ||
       draw.depth.parameters!=v.depth_parameters)
        throw FormatError("Particle draw differs from the recorded native view inputs");
    auto produced=draw;consume_constants(produced.vertex.constants);
    const auto packet=Bytes(constants_.commands());
    const auto bounds=last_bounds_packet();
    const auto minimum=packet.f32(bounds+4),maximum=packet.f32(bounds+8);
    if(!std::isfinite(minimum) || !std::isfinite(maximum) || minimum<0 || maximum<minimum || maximum>16777215.f)
        throw FormatError("Original particle clip packet exceeds its native W-depth domain");
    produced.vertex.clip_range[2]=produced.depth.clip_range[2]=minimum;
    produced.vertex.clip_range[3]=produced.depth.clip_range[3]=maximum;
    check();draw=std::move(produced);
}
void SourceParticleGraphicsBindingTu15::visit_committed(const std::function<void(
    std::uint32_t,std::span<const std::byte>)> &visitor) const {
    // Read-only construction diagnostics can inspect pending fields without
    // opening source execution before configure() completes.
    check(false);constants_.visit_committed(visitor);fields_.visit_committed(visitor);
}
}
