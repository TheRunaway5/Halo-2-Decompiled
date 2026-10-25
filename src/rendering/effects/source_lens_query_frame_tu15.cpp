#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_lens_frame_camera_tu15.hpp"
#include "halo2/source_lens_query_geometry_tu15.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_execution_lens.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <thread>

namespace halo2 {
namespace {
constexpr std::uint32_t driver=0xf7100000u,fifo=0xf7110000u,pages=0xf7140000u;
constexpr unsigned fifo_size=0x20000,query_pages=(source_lens_query_capacity+255)/256;
std::uint32_t word(const SourcePhysicsContactMemory &m,std::uint32_t p) {
    std::array<std::byte,4> b{};m.read(p,b);return Bytes(b).u32(0);
}
void put(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8));m.write(p,b);
}
}
struct SourceLensQueryFrameTu15::Impl final:SourcePhysicsContactMemory {
    Renderer &renderer;
    RendererDeviceLease lease;
    SourceRuntimeMemory *source{};
    const SourceRuntimeMemory *expected_source{};
    SourceRuntimeMemory graphics;
    const std::thread::id thread=std::this_thread::get_id();
    const std::shared_ptr<const int> identity=std::make_shared<const int>(0);
    std::uint64_t revision{};
    std::map<unsigned,std::uint64_t> pending,staged;
    std::array<std::uint64_t,source_lens_query_capacity> generations{};
    std::vector<SourceLensQueryResult> last_completed;
    std::array<std::byte,0x74> camera{};
    SourceViewProjectionBank projection{};
    struct Rollback {
        unsigned id,area;
        bool bit;
        std::array<std::byte,16> metadata;
    };
    std::vector<Rollback> rollback;
    bool active{},initialized{};
    std::uint32_t prepared_views{},views{},fence{};
    unsigned active_view=~0u;
    explicit Impl(Renderer &r):renderer(r),lease(r.device_lease()) {
        graphics.mount(driver,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"native lens query device ABI");
        graphics.mount(fifo,std::vector<std::byte>(fifo_size),SourceRuntimeMemory::Access::ReadWrite,"native lens query emission storage");
        graphics.mount(pages,std::vector<std::byte>(query_pages*4096),SourceRuntimeMemory::Access::ReadWrite,"actual native GPU query metadata");
        put(graphics,driver,fifo);put(graphics,driver+4,fifo+fifo_size);
        for(unsigned n=0;n<query_pages;++n)put(graphics,driver+0x7d4+n*4,pages+n*4096);
        for(unsigned n=0;n<source_lens_query_capacity;++n)put(graphics,pages+n*16+12,0xffffffffu);
    }
    void check(SourceLensFrameWorldTu15 world) {
        if(std::this_thread::get_id()!=thread || !lease.valid())
            throw FormatError("Original lens queries lack their live renderer/thread owner");
        if(expected_source&&expected_source!=&world.memory())
            throw FormatError("Original lens query owner received a different prepared world");
        if(source && source!=&world.memory())throw FormatError("Original lens query owner cannot cross source worlds");
        if(!source) {
            if(revision==std::numeric_limits<std::uint64_t>::max())throw FormatError("Original lens query owner revision exhausted");
            ++revision;source=&world.memory();
        }
    }
    static bool contains(std::uint32_t p,std::size_t size,std::uint32_t base,std::size_t count) {
        return p>=base && std::uint64_t(p)+size<=std::uint64_t(base)+count;
    }
    void read(std::uint32_t p,std::span<std::byte> b)const override {
        if(contains(p,b.size(),0x411568,4)) {
            const std::array<std::byte,4> pointer{std::byte(driver),std::byte(driver>>8),std::byte(driver>>16),std::byte(driver>>24)};
            std::copy_n(pointer.begin()+p-0x411568,b.size(),b.begin());
        } else if(contains(p,b.size(),0x411570,4))std::fill(b.begin(),b.end(),std::byte{});
        else if(contains(p,b.size(),0x491e38,camera.size()))std::memcpy(b.data(),camera.data()+p-0x491e38,b.size());
        else if(contains(p,b.size(),0x491eac,sizeof(projection)))std::memcpy(b.data(),reinterpret_cast<const std::byte*>(&projection)+p-0x491eac,b.size());
        else if(contains(p,b.size(),driver,4096) || contains(p,b.size(),fifo,fifo_size) || contains(p,b.size(),pages,query_pages*4096))graphics.read(p,b);
        else source->read(p,b);
    }
    void write(std::uint32_t p,std::span<const std::byte> b)override {
        if(contains(p,b.size(),0x411570,4)) {
            for(auto value:b)if(value!=std::byte{})throw FormatError("Lens query immutable native pipeline acquired unsupported SDK state");
        } else if(contains(p,b.size(),driver,4096) || contains(p,b.size(),fifo,fifo_size) || contains(p,b.size(),pages,query_pages*4096))graphics.write(p,b);
        else source->write(p,b);
    }
    void run(SourceLensFrameWorldTu15 world,unsigned entry,std::span<const std::uint32_t> arguments={}) {
        auto abi=world.prepare_source_call(arguments);
        const auto use=lease.acquire();
        source_execute_lens_query(world.memory(),entry,abi,this);
    }
    void harvest() {
        last_completed=renderer.take_lens_query_results();
        for(const auto &result:last_completed) {
            const auto found=pending.find(result.id);
            if(found==pending.end() || found->second!=result.generation)
                throw FormatError("Native lens query completion lost its original ring generation");
            if(result.samples>std::numeric_limits<std::uint32_t>::max())
                throw FormatError("Native lens query samples exceed original hardware result storage");
            const auto p=pages+result.id*16;
            put(graphics,p,std::uint32_t(result.timestamp));put(graphics,p+4,std::uint32_t(result.timestamp>>32));
            put(graphics,p+8,std::uint32_t(result.samples));put(graphics,p+12,0);
            pending.erase(found);
        }
    }
};
struct SourceLensQueryFrameTu15::PreparedData {
    std::shared_ptr<const int> owner;
    std::uint64_t revision{};
    std::optional<Renderer::PreparedLensQueryRetirement> retirement;
    std::unique_ptr<Impl> next;
};
SourceLensQueryFrameTu15::PreparedRebind::PreparedRebind()=default;
SourceLensQueryFrameTu15::PreparedRebind::~PreparedRebind()=default;
SourceLensQueryFrameTu15::PreparedRebind::PreparedRebind(PreparedRebind &&) noexcept=default;
SourceLensQueryFrameTu15::PreparedRebind &SourceLensQueryFrameTu15::PreparedRebind::operator=(PreparedRebind &&) noexcept=default;
SourceLensQueryFrameTu15::SourceLensQueryFrameTu15(Renderer &r):impl_(std::make_unique<Impl>(r)){
    r.retire_lens_queries();
}
SourceLensQueryFrameTu15::~SourceLensQueryFrameTu15(){
    // A renderer handoff can explicitly retire the device before this scene
    // owner leaves scope. Its lease is safe to inspect without the Renderer.
    if(impl_->lease.valid())impl_->renderer.retire_lens_queries();
}
std::span<const SourceLensQueryResult> SourceLensQueryFrameTu15::completed()const{return impl_->last_completed;}
void SourceLensQueryFrameTu15::rebind(const SourceRuntimeMemory &memory) {
    auto prepared=prepare_rebind(memory);
    if(!commit_rebind(std::move(prepared),memory))throw FormatError("Original lens query handoff became stale");
}
SourceLensQueryFrameTu15::PreparedRebind SourceLensQueryFrameTu15::prepare_rebind(const SourceRuntimeMemory &memory) const {
    const auto &s=*impl_;
    if(std::this_thread::get_id()!=s.thread||!s.lease.valid()||s.active||s.source==&memory||s.expected_source==&memory)
        throw FormatError("Original lens query handoff lacks a new world and completed renderer frame");
    const auto use=s.lease.acquire();
    source_require_retired_lens_queries(memory);
    PreparedRebind prepared;prepared.data_=std::make_unique<PreparedData>();
    auto &p=*prepared.data_;p.owner=s.identity;p.revision=s.revision;
    p.next=std::make_unique<Impl>(s.renderer);p.next->expected_source=&memory;
    p.retirement.emplace(s.renderer.prepare_lens_query_retirement());
    return prepared;
}
bool SourceLensQueryFrameTu15::rebind_ready(const PreparedRebind &prepared,const SourceRuntimeMemory &memory) const noexcept {
    try {
        const auto &s=*impl_;const auto *p=prepared.data_.get();
        if(std::this_thread::get_id()!=s.thread||!p||p->owner!=s.identity||p->revision!=s.revision||
           s.active||!s.lease.valid()||!p->next||p->next->expected_source!=&memory||
           s.source==&memory||s.expected_source==&memory)return false;
        source_require_retired_lens_queries(memory);
        return p->retirement&&s.renderer.lens_query_retirement_ready(*p->retirement);
    } catch(...) {return false;}
}
bool SourceLensQueryFrameTu15::rebind_ready_held(const PreparedRebind &prepared,const SourceRuntimeMemory &memory) const noexcept {
    try {
        const auto &s=*impl_;const auto *p=prepared.data_.get();
        if(std::this_thread::get_id()!=s.thread||!p||p->owner!=s.identity||p->revision!=s.revision||
           s.active||!s.lease.valid()||!p->next||p->next->expected_source!=&memory||
           s.source==&memory||s.expected_source==&memory)return false;
        return p->retirement&&s.renderer.lens_query_retirement_ready(*p->retirement);
    } catch(...) {return false;}
}
Renderer *SourceLensQueryFrameTu15::renderer_owner() const noexcept {return &impl_->renderer;}
void SourceLensQueryFrameTu15::publish_rebind(PreparedRebind &prepared) noexcept {
    auto &p=*prepared.data_;
    if(!impl_->renderer.commit_lens_query_retirement(std::move(*p.retirement)))std::terminate();
    impl_.swap(p.next);p.owner.reset();
}
bool SourceLensQueryFrameTu15::commit_rebind(PreparedRebind &&prepared,const SourceRuntimeMemory &memory) noexcept {
    if(!rebind_ready(prepared,memory))return false;
    publish_rebind(prepared);return true;
}
void SourceLensQueryFrameTu15::begin(PhysicsLiveWorldTu15 &world) {begin(SourceLensFrameWorldTu15{world});}
void SourceLensQueryFrameTu15::prepare_view(PhysicsLiveWorldTu15 &world,unsigned v) {prepare_view(SourceLensFrameWorldTu15{world},v);}
std::vector<SourceLensQueryDraw> SourceLensQueryFrameTu15::view(PhysicsLiveWorldTu15 &world,unsigned v,
    Vec3 p,Vec3 f,Vec3 u,float a,const SourceViewProjectionBank &b,std::array<std::int16_t,4> r,unsigned w,unsigned h) {
    return view(SourceLensFrameWorldTu15{world},v,p,f,u,a,b,r,w,h);
}
std::vector<SourceLensQueryDraw> SourceLensQueryFrameTu15::view(PhysicsLiveWorldTu15 &world,unsigned v,
    Vec3 p,Vec3 f,Vec3 u,float a,const SourceViewProjectionBank &b,std::array<std::int16_t,4> r,unsigned w,unsigned h,Vec2 c) {
    return view(SourceLensFrameWorldTu15{world},v,p,f,u,a,b,r,w,h,c);
}
void SourceLensQueryFrameTu15::finish_view(PhysicsLiveWorldTu15 &world,unsigned v) {finish_view(SourceLensFrameWorldTu15{world},v);}
void SourceLensQueryFrameTu15::finish(PhysicsLiveWorldTu15 &world,bool submitted) {finish(SourceLensFrameWorldTu15{world},submitted);}
void SourceLensQueryFrameTu15::begin(SourceLensFrameWorldTu15 world) {
    auto &s=*impl_;
    if(std::this_thread::get_id()!=s.thread||!s.lease.valid())
        throw FormatError("Original lens queries lack their live renderer/thread owner");
    if(s.active)throw FormatError("Original lens query frame already active");
    if(s.revision==std::numeric_limits<std::uint64_t>::max())throw FormatError("Original lens query owner revision exhausted");
    ++s.revision;s.check(world);
    s.harvest();
    if(!s.initialized){world.call_actual(0x3803b0u);s.initialized=true;}
    world.call_actual(0x20680u);s.prepared_views=0;s.views=0;s.active_view=~0u;s.active=true;
}
void SourceLensQueryFrameTu15::prepare_view(SourceLensFrameWorldTu15 world,unsigned view) {
    auto &s=*impl_;s.check(world);
    if(!s.active || s.active_view!=~0u || view>=4 || (s.prepared_views&(1u<<view)))
        throw FormatError("Original lens view admission lacks a unique active frame");
    auto abi=world.prepare_source_call();abi.gp[0]=view;
    world.run_actual(0x20700u,abi);
    s.prepared_views|=1u<<view;s.active_view=view;
}
std::vector<SourceLensQueryDraw> SourceLensQueryFrameTu15::view(SourceLensFrameWorldTu15 world,unsigned view,
    Vec3 position,Vec3 forward,Vec3 up,float angle,const SourceViewProjectionBank &projection,
    std::array<std::int16_t,4> rectangle,unsigned logical_width,unsigned logical_height) {
    return this->view(world,view,position,forward,up,angle,projection,rectangle,
        logical_width,logical_height,Vec2{.015625f,1024.f});
}
std::vector<SourceLensQueryDraw> SourceLensQueryFrameTu15::view(SourceLensFrameWorldTu15 world,unsigned view,
    Vec3 position,Vec3 forward,Vec3 up,float angle,const SourceViewProjectionBank &projection,
    std::array<std::int16_t,4> rectangle,unsigned logical_width,unsigned logical_height,Vec2 clip_planes) {
    auto &s=*impl_;s.check(world);
    if(!s.active || view>=4 || s.active_view!=view || !(s.prepared_views&(1u<<view)) ||
       (s.views&(1u<<view)) || !logical_width || !logical_height)
        throw FormatError("Original lens query view lacks a unique active frame/surface");
    s.views|=1u<<view;s.projection=projection;
    const auto targets=s.renderer.target_resources().acquire();const auto &depth=targets.bank().depth_stencil;
    s.camera=source_lens_frame_camera_tu15(position,forward,up,angle,rectangle,
        logical_width,logical_height,depth.width,depth.height,clip_planes);
    // Retain only query-issue fields for a declined submission. Subsequent
    // views' actual requests and completed prior queries must survive rollback.
    const auto records=s.source->view(0x52dfdc,511*32);
    const std::vector<std::byte> before_records(records.begin(),records.end());
    std::array<std::byte,192> before_bitmap{};s.source->read(0x531fc0,before_bitmap);
    const auto metadata=s.graphics.view(pages,query_pages*4096);
    const std::vector<std::byte> before_metadata(metadata.begin(),metadata.end());
    put(s.graphics,driver,fifo);
    const auto mode=word(*s.source,0x4920b8);
    if(mode<4 || mode>7) {
        // Authentic2DEC8..2DED4: actual view, classes0..4, implicit retained
        // list, hardware query enabled, whole2E520 callback.
        const std::array<std::uint32_t,6> args{view,0x1f,0,0,1,0x2e520};
        s.run(world,0x20b90,args);
    }
    std::vector<SourceLensQueryDraw> draws;
    const auto end=word(s.graphics,driver);
    if(end<fifo || end>fifo+fifo_size)throw FormatError("Original lens query FIFO exceeds its native capacity");
    const auto packet=s.graphics.view(fifo,end-fifo);const Bytes data(packet);
    for(unsigned offset=0;offset<packet.size();) {
        if(data.u32(offset)!=0x817c8u || data.u32(offset+4)!=1 || data.u32(offset+8)!=1)
            throw FormatError("Original lens query issue packet differs");
        offset+=12;const auto first=offset;
        if(data.u32(offset)==0x417fcu)offset+=96;
        if(offset+12>packet.size() || data.u32(offset)!=0x817ccu || data.u32(offset+4)!=0)
            throw FormatError("Original lens query completion packet differs");
        const auto alias=data.u32(offset+8);unsigned id=source_lens_query_capacity;
        for(unsigned n=0;n<source_lens_query_capacity;++n)
            if((((pages+n*16)&0x0effffffu)|0x01000000u)==alias){id=n;break;}
        if(id==source_lens_query_capacity || s.pending.contains(id) || s.staged.contains(id))
            throw FormatError("Original lens query issue lacks an available actual native identity");
        const auto generation=++s.generations[id];s.staged.emplace(id,generation);
        const auto slot=id/3,ring=id%3,bit=1u<<(slot%32);
        Impl::Rollback saved{id,Bytes(before_records).u16(slot*32+6),
            bool(Bytes(before_bitmap).u32(ring*64+(slot/32)*4)&bit),{}};
        std::copy_n(before_metadata.begin()+id*16,16,saved.metadata.begin());s.rollback.push_back(saved);
        const auto area=Bytes(s.source->view(0x52dfdc+(id/3)*32+6,2)).u16(0);
        draws.push_back(source_lens_query_geometry_tu15(packet.subspan(first,offset-first),area,id,generation));
        offset+=12;
    }
    // Original2C2F7 completion follows query issue for this submitting view.
    // Only harvested actual GPU metadata can make402710 report ready.
    s.run(world,0x20750);
    return draws;
}
void SourceLensQueryFrameTu15::finish_view(SourceLensFrameWorldTu15 world,unsigned view) {
    auto &s=*impl_;s.check(world);
    if(!s.active || view>=4 || s.active_view!=view || !(s.views&(1u<<view)))
        throw FormatError("Original lens view retirement lacks its completed world pass");
    // Whole2B5B0: EDI=-1 at2B65F, retained across2B770. Literal2B6AA
    // disables request admission after that view returns, before the next view.
    const std::array<std::byte,4> none{std::byte{255},std::byte{255},std::byte{255},std::byte{255}};
    world.write_source_call_input(0x532094u,none);s.active_view=~0u;
}
void SourceLensQueryFrameTu15::finish(SourceLensFrameWorldTu15 world,bool submitted) {
    auto &s=*impl_;s.check(world);
    if(!s.active || s.active_view!=~0u)throw FormatError("Original lens query finish has an incomplete view/frame");
    std::vector<SourceLensQueryRollback> source_rollback;source_rollback.reserve(s.rollback.size());
    for(const auto &saved:s.rollback)source_rollback.push_back({saved.id,saved.area,saved.bit});
    source_finish_lens_query(world.memory(),submitted,source_rollback);
    if(submitted) {
        s.pending.merge(s.staged);put(s.graphics,driver+0x2c,++s.fence);
    } else {
        for(const auto &saved:s.rollback)s.graphics.write(pages+saved.id*16,saved.metadata);
        s.staged.clear();
    }
    s.rollback.clear();s.active=false;
}
}
