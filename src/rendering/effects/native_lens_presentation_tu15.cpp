#include "halo2/native_lens_presentation_tu15.hpp"
#include "halo2/source_lens_flare_tu15.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/source_renderer_request_tu15.hpp"
#include "halo2/source_renderer_tail_storage_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_lens_frame_camera_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/renderer.hpp"
#include "halo2/original_data.hpp"
#include "halo2/shader_animation.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <thread>

namespace halo2 {
namespace {
void put(SourceRuntimeMemory &m,std::uint32_t at,std::uint32_t v) {
    std::array<std::byte,4> b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8));m.write(at,b);
}
}
struct NativeLensPresentationTu15::Impl {
    const Cache &cache;
    SourceRuntimeMemory memory;
    SourceLensDrawFrameTu15 &draw;
    std::unique_ptr<SourceLensQueryFrameTu15> query;
    Renderer *renderer{};
    const std::thread::id thread=std::this_thread::get_id();
    unsigned view=~0u,width{},height{},local_views{};
    std::uint32_t texture_stamp{};
    Vec3 position,forward,up;
    float angle{};
    SourceViewProjectionBank projection{};
    std::array<std::int16_t,4> rectangle{};
    bool active{},queried{},drawn{};
    static constexpr std::uint32_t stack=0xf6000000u,arena=0xf6010000u,table=0xf6200000u;
    void own()const {
        if(std::this_thread::get_id()!=thread)throw FormatError("Native lens presentation crossed its executor");
    }
    SourcePhysicsWorldRuntimeAbi prepare(std::span<const std::uint32_t> arguments={}) {
        if(arguments.size()>64)throw FormatError("Native lens caller arguments exceed their retained stack");
        SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=stack+0x8000u;a.eflags=0x202;
        a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        put(memory,a.gp[4],0);for(unsigned n=0;n<arguments.size();++n)put(memory,a.gp[4]+4+n*4,arguments[n]);
        return a;
    }
    void run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &a) {
        SourcePhysicsWorldRuntimeServices services;
        std::function<void(std::uint32_t,SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver &)> execute;
        services.required_owner=[&](auto &,const auto &call,auto &state,const auto &observe){execute(call.target,state,observe);};
        execute=[&](auto target,auto &state,const auto &observe) {
            const auto fp=state.floating_point.value_or(SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80});
            if(source_lens_flare_tu15_owns_entry(target))source_run_lens_flare_tu15(memory,target,state,fp,services,observe);
            else if(source_lens_query_tu15_owns_entry(target))source_run_lens_query_tu15(memory,target,state,fp,services,observe);
            else if(source_renderer_tail_storage_tu15_owns_entry(target))source_run_renderer_tail_storage_tu15(memory,target,state,fp,services,observe);
            else if(target==0x169d0u)source_run_renderer_request_tu15(memory,target,state,fp,services,observe);
            else if(target==0x126850u)source_run_campaign_cinematic_tu15(memory,target,state,fp,services,observe);
            else throw FormatError("Native lens CPU execution reached an unowned entry "+std::to_string(target));
        };
        execute(entry,a,{});
    }
    std::uint32_t call(std::uint32_t entry,std::optional<std::uint32_t> eax={}) {
        auto a=prepare();if(eax)a.gp[0]=*eax;run(entry,a);return a.gp[0];
    }
    void request(unsigned category,std::uint32_t owner,std::uint32_t lens,unsigned ordinal,
        Vec3 position,Vec3 forward,float scale,float opacity,Vec3 color) {
        own();if(!active||view>=local_views||queried||owner==0xffffffffu||ordinal>=64)
            throw FormatError("Native lens request lost its actual owner/view or original request bound");
        const auto request=stack+0x1000u;
        memory.write(request,std::as_bytes(std::span{&position,1}));memory.write(request+12,std::as_bytes(std::span{&forward,1}));
        memory.write(request+24,std::as_bytes(std::span{&color,1}));
        const std::array<std::uint32_t,9> args{0,category,owner&65535u,ordinal,request,request+24,
            std::bit_cast<std::uint32_t>(1.f),std::bit_cast<std::uint32_t>(opacity),std::bit_cast<std::uint32_t>(scale)};
        auto a=prepare(args);a.gp[1]=lens;a.gp[7]=request+12;run(0x2db80u,a);
    }
    explicit Impl(const Cache &c,SourceLensDrawFrameTu15 &resident):cache(c),draw(resident) {initialize();}
    void initialize() {
        const auto &c=cache;
        if(!c.original_data()||c.original_data()->image!=OriginalData::Image::TitleUpdate15)
            throw FormatError("Native lens presentation requires authenticated TU1.5 behavior");
        const auto operand=[&](std::uint32_t at,const auto &bytes,SourceRuntimeMemory::Access access) {
            memory.mount(at,std::as_bytes(std::span(bytes)),access,"native lens authenticated fixed CPU operand");
        };
#include "rendering/effects/native_client_lens_operands_tu15.inc"
        // Whole17F790/17F8A0 index the authenticated periodic/transition
        // banks through a register. Retain the same authored tables used by
        // native shader/effect arithmetic; neither bank is a GPU result.
        const auto tables=c.shader_function_tables();
        if(!tables)throw FormatError("Native lens rotation lacks its original function tables");
        memory.mount(0x4752a8u,std::as_bytes(std::span(tables->periodic)),SourceRuntimeMemory::Access::ReadOnly,"native lens original periodic function bank");
        memory.mount(0x4782a8u,std::as_bytes(std::span(tables->transition)),SourceRuntimeMemory::Access::ReadOnly,"native lens original transition function bank");
        mount_native_tu15_renderer_tail_storage_data(memory);
        memory.mount(0x4c65c0u,std::vector<std::byte>(0x138),SourceRuntimeMemory::Access::ReadWrite,"native lens actual submitting view inputs");
        memory.mount(0x491e20u,std::vector<std::byte>(0x14c),SourceRuntimeMemory::Access::ReadWrite,"native lens actual ordinary world camera inputs");
        memory.mount(stack,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"native lens retained CPU call stack");
        memory.mount(arena,std::vector<std::byte>(0x1000),SourceRuntimeMemory::Access::ReadWrite,"native lens private CPU interface arena");
        put(memory,0x4f2950u,arena);
        call(0x169d0u);call(0x1fb90u);
        source_renderer_cpu_interface_initialize_tu15(memory,[&](auto entry,auto eax){return call(entry,eax);});
        if(c.tags().size()>65536)throw FormatError("Native lens cache directory exceeds original tag indices");
        memory.mount(table,std::vector<std::byte>(std::max<std::size_t>(16,c.tags().size()*16)),SourceRuntimeMemory::Access::ReadWrite,"native lens actual typed cache directory");
        put(memory,0x4f0414u,table);
        for(const auto &tag:c.tags()) {
            const auto &group=tag.group;
            if(!tag.address || (group!="lens"&&group!="bitm"&&group!="matg"))continue;
            const auto metadata=c.load_source_tag_metadata(tag.datum);
            memory.mount(metadata.source_address(),metadata.bytes_at(metadata.source_address(),metadata.size()),SourceRuntimeMemory::Access::ReadOnly,"native lens retained canonical "+group+" cache allocation");
            const auto at=table+(tag.datum&65535u)*16;
            put(memory,at,tag.type().value);put(memory,at+4,tag.datum);put(memory,at+8,metadata.source_address());put(memory,at+12,metadata.size());
        }
        // Actual22589/22590 publishes this authored matg rasterizer row.
        // Its native cache allocation above is retained, not a readiness flag.
        const auto globals=c.load_source_tag_metadata(c.globals_tag().value);
        const Bytes root(globals.bytes_at(globals.source_address(),globals.size()));
        if(root.u32(0x108)!=1)throw FormatError("Native lens rasterizer globals lack their unique authored row");
        put(memory,0x4922a0u,root.u32(0x10c));
    }
};
NativeLensPresentationTu15::NativeLensPresentationTu15(const Cache &c,SourceLensDrawFrameTu15 &resident):impl_(std::make_unique<Impl>(c,resident)){}
NativeLensPresentationTu15::~NativeLensPresentationTu15()=default;
SourceLensFrameWorldTu15 NativeLensPresentationTu15::execution() {
    static constexpr SourceLensFrameWorldTu15::Operations operations{
        [](void *p)->SourceRuntimeMemory &{return static_cast<Impl *>(p)->memory;},
        [](void *p,std::span<const std::uint32_t> a){return static_cast<Impl *>(p)->prepare(a);},
        [](void *p,std::uint32_t e,SourcePhysicsWorldRuntimeAbi &a){static_cast<Impl *>(p)->run(e,a);},
        [](void *p,std::uint32_t e){return static_cast<Impl *>(p)->call(e);},
        [](void *p,std::uint32_t at,std::span<const std::byte> b){static_cast<Impl *>(p)->memory.write(at,b);},
        [](void *p,const Cache &c,std::uint32_t at,std::size_t n) {
            auto &s=*static_cast<Impl *>(p);if(&c!=&s.cache)throw FormatError("Native lens resident cache owner differs");
            return s.memory.native_address(at,n);
        }
    };
    return {impl_.get(),operations};
}
void NativeLensPresentationTu15::bind(Renderer &r) {
    auto &s=*impl_;s.own();if(s.renderer)throw FormatError("Native lens renderer was already bound");
    if(s.draw.renderer_owner()!=&r)throw FormatError("Native lens shared bitmap bank lacks its actual bound renderer");
    s.renderer=&r;
}
void NativeLensPresentationTu15::begin(unsigned local_views) {
    auto &s=*impl_;s.own();if(!s.renderer||s.active)throw FormatError("Native lens frame lacks its unique bound renderer");
    if(local_views<1||local_views>4)throw FormatError("Native lens frame lacks its actual local view count");
    // Literal2B609 copies the actual main caller's first argument here.
    // This is current native layout input, never an original device result.
    s.local_views=local_views;put(s.memory,0x4c686cu,local_views);
    if(!s.query) {
        if(!s.draw.accepts_source(s.memory))s.draw.rebind(s.memory);
        s.query=std::make_unique<SourceLensQueryFrameTu15>(*s.renderer);
    }
    s.query->begin(execution());s.draw.begin();s.active=true;
}
void NativeLensPresentationTu15::prepare_view(unsigned view,Vec3 position,Vec3 forward,Vec3 up,float angle,
    const SourceViewProjectionBank &projection,std::array<std::int16_t,4> rectangle,unsigned width,unsigned height) {
    auto &s=*impl_;s.own();if(!s.active||s.view!=~0u||view>=s.local_views)throw FormatError("Native lens view lacks unique active frame");
    const auto targets=s.renderer->target_resources().acquire();const auto &surface=targets.bank().depth_stencil;
    const auto camera=source_lens_frame_camera_tu15(position,forward,up,angle,rectangle,width,height,surface.width,surface.height,{.015625f,1024.f});
    s.memory.write(0x4c65c0u,camera);s.memory.write(0x4c6634u,std::as_bytes(std::span{&projection,1}));
    put(s.memory,0x4c66f4u,view);
    s.query->prepare_view(execution(),view);
    s.view=view;s.position=position;s.forward=forward;s.up=up;s.angle=angle;s.projection=projection;s.rectangle=rectangle;s.width=width;s.height=height;s.queried=false;s.drawn=false;
}
void NativeLensPresentationTu15::free_projectile(std::uint32_t owner,std::uint32_t lens,unsigned ordinal,
    Vec3 position,Vec3 forward,float scale,float opacity,Vec3 color) {
    impl_->request(0,owner,lens,ordinal,position,forward,scale,opacity,color);
}
void NativeLensPresentationTu15::unattached_effect(const SourceEffectReplicaRenderTu15 &source,
    std::uint32_t lens,unsigned ordinal,Vec3 position,Vec3 forward,float opacity) {
    if(!source_effect_replica_static_world_tu15(source))
        throw FormatError("Replica effect lens requires its original attached/FP/moving owner");
    // Whole3E9B0 returns zero immediately for the observed NONE owner.
    impl_->request(1,source.identity,lens,ordinal,position,forward,1.f,opacity,source.color);
}
void NativeLensPresentationTu15::nonbiped_attached_effect(const SourceEffectReplicaRenderTu15 &source,
    std::uint32_t lens,unsigned ordinal,Vec3 position,Vec3 forward,float opacity) {
    if(!source_effect_replica_attached_world_tu15(source))
        throw FormatError("Attached replica lens lacks its current non-biped WORLD root");
    // Whole3E9B0 rejects a nonzero actual ultimate-root kind in every view.
    impl_->request(1,source.identity,lens,ordinal,position,forward,1.f,opacity,source.color);
}
void NativeLensPresentationTu15::standalone_light(const SourceLightLensRequestTu15 &request) {
    auto &s=*impl_;s.own();
    if(!s.active || s.view>=s.local_views || s.queried)
        throw FormatError("Standalone light lens lacks its active replica view");
    source_request_light_lens_tu15(execution(),request);
}
std::vector<SourceLensQueryDraw> NativeLensPresentationTu15::queries() {
    auto &s=*impl_;s.own();if(!s.active||s.view>=4||s.queried)throw FormatError("Native lens query view was not uniquely prepared");s.queried=true;
    return s.query->view(execution(),s.view,s.position,s.forward,s.up,s.angle,s.projection,s.rectangle,s.width,s.height);
}
std::vector<SourceLensDraw> NativeLensPresentationTu15::draws() {
    auto &s=*impl_;s.own();if(!s.queried||s.drawn)throw FormatError("Native lens drawing lacks its actual query pass");s.drawn=true;
    return s.draw.view(execution(),s.view,s.position,s.forward,s.up,s.angle,s.projection,s.rectangle,s.width,s.height);
}
std::span<const SourceLensQueryResult> NativeLensPresentationTu15::completed_queries()const {
    const auto &s=*impl_;s.own();return s.query?s.query->completed():std::span<const SourceLensQueryResult>{};
}
void NativeLensPresentationTu15::finish_view() {
    auto &s=*impl_;s.own();if(!s.drawn)throw FormatError("Native lens view was not completed");
    s.query->finish_view(execution(),s.view);s.view=~0u;
}
void NativeLensPresentationTu15::finish(bool submitted) {
    auto &s=*impl_;s.own();if(!s.active||s.view!=~0u)throw FormatError("Native lens frame was not completed");
    s.query->finish(execution(),submitted);s.draw.finish(submitted);s.active=false;
    if(submitted) {
        if(s.texture_stamp>=0x7ffffffdu)throw FormatError("Native lens texture stamp exhausted");
        put(s.memory,0x4f2d58u,++s.texture_stamp);
    }
}
void NativeLensPresentationTu15::suspend() {
    auto &s=*impl_;s.own();if(s.active)throw FormatError("Cannot suspend an active native lens frame");
    if(s.query) {
        s.query.reset();
        // Actual renderer retirement discards GPU work, then whole original
        // slot retirement clears its corresponding private CPU request bank.
        for(unsigned n=0;n<511;++n)s.call(0x20e30u,n);
    }
}
}
