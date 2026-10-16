#include "halo2/source_lens_draw_frame_tu15.hpp"
#include "halo2/source_lens_draw_tu15.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/source_lens_program_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_vehicle_motion_math.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_execution_lens.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <map>
#include <set>
#include <thread>
#include <sstream>

namespace halo2 {
namespace {
constexpr unsigned driver=0xf7200000u,fifo=0xf7210000u,headers=0xf7500000u,images=0xf7600000u;
constexpr unsigned fifo_bytes=0x200000u;
unsigned word(const SourcePhysicsContactMemory &m,unsigned p,unsigned size=4) {
    std::array<std::byte,4> b{};m.read(p,std::span(b).first(size));return Bytes(b).u32(0);
}
void put(SourcePhysicsContactMemory &m,unsigned p,unsigned v,unsigned size=4) {
    std::array<std::byte,4> b{};for(unsigned n=0;n<size;++n)b[n]=std::byte(v>>(n*8));m.write(p,std::span(b).first(size));
}
bool within(unsigned p,std::size_t n,unsigned start,std::size_t size) {
    return p>=start && std::uint64_t(p)+n<=std::uint64_t(start)+size;
}
struct PassBoundary {std::array<unsigned,6> arguments;};
}
struct SourceLensDrawFrameTu15::Impl final:SourcePhysicsContactMemory {
    struct Resource {
        unsigned bitmap,index,row,header,alias,material;
        std::array<std::byte,0x74> metadata;
        std::shared_ptr<const Texture> texture;
    };
    const Cache &cache;
    std::vector<Resource> resources;
    std::map<unsigned,std::size_t> domains;
    std::map<unsigned,unsigned> header_material;
    std::map<unsigned,unsigned> alias_header;
    SourceRuntimeMemory graphics;
    SourceRuntimeMemory *source{};
    Renderer *renderer{};
    RendererDeviceLease lease;
    const std::thread::id thread=std::this_thread::get_id();
    std::array<std::byte,0x74> camera{};
    SourceViewProjectionBank projection{};
    std::vector<SourceLensDraw> draws;
    bool active{},planning{};
    unsigned views{},fence{};

    Impl(const Cache &c,Mesh &mesh):cache(c) {
        if(c.is_vista() || c.is_mcc())throw FormatError("Original lens draw owner requires the admitted Xbox cache");
        const Bytes globals(c.tag_payload(c.globals_tag()));
        if(globals.u32(0x108)!=1)throw FormatError("Original lens rasterizer globals are unavailable");
        const Bytes raster(c.read_tag_bytes(globals.u32(0x10c),0x68));
        if(raster.u32(0x58)<=10)throw FormatError("Original lens vertex reference slot is absent");
        const Bytes shaders(c.read_tag_bytes(raster.u32(0x5c),raster.u32(0x58)*8u));
        if(shaders.u32(0x50)!=0x76727478u)throw FormatError("Original lens vertex slot is not vrtx");
        const auto program=c.load_vertex_shader_code(shaders.u32(0x54),0);
        if(program.size()!=source_lens_vertex_bytes.size())throw FormatError("Original lens vertex program differs");
        for(unsigned n=0;n<program.size();++n)if(std::to_integer<unsigned char>(program[n])!=source_lens_vertex_bytes[n])
            throw FormatError("Original lens vertex MOV program differs from the compiled owner");
        std::set<unsigned> bitmaps;std::map<unsigned,std::array<std::byte,0x74>> rows;
        bitmaps.insert(raster.u32(0x64)); // Original14540/14460 type0 default.
        for(const auto &lens:c.select_tags({.group=TagGroup::from_string("lens"),.require_address=true})) {
            const Bytes definition(c.tag_payload(lens.handle()));const auto bitmap=definition.u32(0x24);
            if(bitmap!=0xffffffffu)bitmaps.insert(bitmap);
        }
        for(auto bitmap:bitmaps) {
            if(bitmap==0xffffffffu)continue;
            const auto *tag=c.query_tag({.handle=TagHandle{bitmap},.group=tag_groups::bitmap,.require_address=true});
            if(!tag)throw FormatError("Original lens bitmap definition is absent");
            const Bytes root(c.tag_payload(tag->handle()));const auto count=root.u32(0x44),first=root.u32(0x48);
            if(count!=c.bitmap_count(bitmap) || count>65536)throw FormatError("Original lens bitmap row count differs");
            const auto data=c.read_tag_bytes(first,std::size_t(count)*0x74);
            for(unsigned image=0;image<count;++image) {
                Resource resource{bitmap,image,first+image*0x74,headers+unsigned(resources.size())*32,
                    images+unsigned(resources.size())*4096,unsigned(mesh.materials.size()),{}, {}};
                std::copy_n(data.begin()+image*0x74,0x74,resource.metadata.begin());
                if(const auto found=rows.find(resource.row);found!=rows.end()) {
                    if(found->second!=resource.metadata)throw FormatError("Aliased lens bitmap rows disagree");
                    continue;
                }
                if(Bytes(resource.metadata).u16(0xa)!=0)throw FormatError("Original lens bitmap requires its non-2D resource owner");
                resource.texture=std::make_shared<const Texture>(c.load_bitmap(bitmap,image));
                if(resource.texture->layers!=1 || resource.texture->depth || resource.texture->mips.empty())
                    throw FormatError("Original lens bitmap has no complete native 2D mip image");
                Material material;material.name="source lens bitmap "+std::to_string(bitmap)+":"+std::to_string(image);
                material.diffuse=*resource.texture;mesh.materials.push_back(std::move(material));
                rows.emplace(resource.row,resource.metadata);header_material.emplace(resource.header,resource.material);
                alias_header.emplace(resource.alias&0x0fffffffu,resource.header);
                resources.push_back(std::move(resource));
            }
        }
    }
    void mount(unsigned p,std::span<const std::byte> bytes,const char *owner) {
        graphics.mount(p,bytes,SourceRuntimeMemory::Access::ReadWrite,owner);domains.emplace(p,bytes.size());
    }
    bool owned(unsigned p,std::size_t n)const {
        auto i=domains.upper_bound(p);if(i==domains.begin())return false;--i;
        return within(p,n,i->first,i->second);
    }
    void read(unsigned p,std::span<std::byte> out)const override {
        for(auto base:{0x491e38u,0x4c65c0u})if(within(p,out.size(),base,camera.size())) {
            std::memcpy(out.data(),camera.data()+p-base,out.size());return;
        }
        for(auto base:{0x491eacu,0x4c6634u})if(within(p,out.size(),base,sizeof(projection))) {
            std::memcpy(out.data(),reinterpret_cast<const std::byte*>(&projection)+p-base,out.size());return;
        }
        if(owned(p,out.size()))graphics.read(p,out);else source->read(p,out);
    }
    void write(unsigned p,std::span<const std::byte> in)override {
        if(owned(p,in.size()))graphics.write(p,in);else source->write(p,in);
    }
    void check()const {
        if(std::this_thread::get_id()!=thread || !renderer || !lease.valid())
            throw FormatError("Original lens drawing lacks its actual renderer/thread lease");
    }
    void execute(PhysicsLiveWorldTu15 &world,unsigned entry,SourcePhysicsWorldRuntimeAbi &abi) {
        if(auto pass=source_execute_lens_draw(world,entry,abi,planning,domains,*this))throw PassBoundary{*pass};
    }
    void run(PhysicsLiveWorldTu15 &world,unsigned entry,std::span<const unsigned> args={}) {
        auto abi=world.prepare_source_call(args);execute(world,entry,abi);
    }
    void consume() {
        const auto end=word(graphics,driver);
        if(end<fifo || end>fifo+fifo_bytes)throw FormatError("Original final flare FIFO exceeds its native backing");
        const auto packet=graphics.view(fifo,end-fifo);const Bytes b(packet);
        unsigned material=~0u;float secondary=-1;bool depth{};
        for(unsigned at=0;at<packet.size();) {
            const auto header=b.u32(at),method=header&0x1fffu,count=(header>>18)&2047u;
            if(!count || std::uint64_t(at)+4+count*4>packet.size())throw FormatError("Original final flare packet is truncated");
            if(header==0x417fc && b.u32(at+4)==7) {
                if(material==~0u || secondary<0 || at+136>packet.size())
                    throw FormatError("Original flare primitive lacks resident bitmap/attribute8 state");
                draws.push_back(source_lens_draw_geometry_tu15(packet.subspan(at,136),material,secondary,depth));
                at+=136;continue;
            }
            if(method==0x1b00 && count==2) {
                const auto found=alias_header.find(b.u32(at+4));
                if(found==alias_header.end() || b.u32(at+8)!=word(graphics,found->second+0xc))
                    throw FormatError("Original flare bitmap command lost its genuine resident header");
                material=header_material.at(found->second);
            }else if(method==0x18c0 && count==2) {
                if(b.u32(at+8)!=0)throw FormatError("Original flare attribute8 second component differs");
                secondary=b.f32(at+4);
            }else if(method==0x30c && count==1) {
                const auto mode=b.u32(at+4);if(mode!=0 && mode!=2)throw FormatError("Original flare depth mode differs");depth=mode!=0;
            }else if(method==0x1e70 && count==1) {
                if(b.u32(at+4)!=source_lens_pixel_words[54])throw FormatError("Original flare texture shader mode differs");
            }else if(!((method==0x1b08 && count==2) ||
                ((method==0x1b0c || method==0x1b14 || method==0x1d78) && count==1))) {
                std::ostringstream error;error<<"Original flare requires unadmitted GPU method0x"<<std::hex<<method;throw FormatError(error.str());
            }
            at+=4+count*4;
        }
    }
    void initialize(PhysicsLiveWorldTu15 &world) {
        if(source && source!=&world.memory())throw FormatError("Original lens draw owner cannot cross source worlds");
        if(source)return;
        source=&world.memory();
        mount(0x40eeb0,source->view(0x40eeb0,0x411800-0x40eeb0),"native lens SDK state with actual source tables");
        mount(0x491788,std::as_bytes(std::span(source_lens_pixel_words)),"original final lens pixel definition");
        mount(0x4c4b00,source->view(0x4c4b00,0x200),"native lens raster state");
        mount(0x4ce1e0,std::vector<std::byte>(0x48),"original final lens secondary output");
        mount(0x52df98,std::vector<std::byte>(32),"native lens actual bitmap bindings");
        mount(driver,std::vector<std::byte>(8192),"native lens device ABI backed by renderer lease");
        mount(fifo,std::vector<std::byte>(fifo_bytes),"original lens primitive emission storage");
        mount(headers,std::vector<std::byte>(std::max<std::size_t>(32,resources.size()*32)),"genuine resident lens texture headers");
        put(graphics,0x411568,driver);put(graphics,0x411570,0);put(graphics,driver,fifo);put(graphics,driver+4,fifo+fifo_bytes);
        for(unsigned stage=0;stage<4;++stage) {
            const std::array<unsigned,6> sampler{1,1,1,2,2,2};
            graphics.write(0x40eeb0+stage*128,std::as_bytes(std::span(sampler)));
            put(graphics,driver+12+stage*4,0x80000000);
        }
        const auto table=word(*source,0x4f0414);
        for(const auto &r:resources) {
            renderer->require_lens_bitmap(r.material,*r.texture);
            if(word(*source,table+(r.bitmap&65535)*16+4)!=r.bitmap)
                throw FormatError("Lens bitmap source tag identity differs from its native resident owner");
            mount(r.row,source->view(r.row,0x74),"native resident lens bitmap metadata view");
            put(graphics,r.row+0x40,r.alias);
            auto abi=world.prepare_source_call(std::array<unsigned,2>{0,r.header});abi.gp[0]=r.row;
            execute(world,0x1301e0,abi);
            if(word(graphics,r.header+4)!=(r.alias&0x0fffffffu))throw FormatError("Original lens header lost its resident native alias");
        }
    }
};
SourceLensDrawFrameTu15::SourceLensDrawFrameTu15(const Cache &c,Mesh &m):impl_(std::make_unique<Impl>(c,m)){}
SourceLensDrawFrameTu15::~SourceLensDrawFrameTu15()=default;
void SourceLensDrawFrameTu15::bind(Renderer &r) {
    auto &s=*impl_;if(s.renderer || std::this_thread::get_id()!=s.thread)throw FormatError("Original lens renderer binding is duplicate or foreign");
    for(const auto &resource:s.resources)r.require_lens_bitmap(resource.material,*resource.texture);
    s.renderer=&r;s.lease=r.device_lease();s.check();
}
void SourceLensDrawFrameTu15::begin(){auto &s=*impl_;s.check();if(s.active)throw FormatError("Original lens frame already active");s.active=true;s.views=0;}
std::vector<SourceLensDraw> SourceLensDrawFrameTu15::view(PhysicsLiveWorldTu15 &world,unsigned view,
    Vec3 position,Vec3 forward,Vec3 up,float angle,const SourceViewProjectionBank &projection,
    std::array<std::int16_t,4> rectangle,unsigned width,unsigned height) {
    auto &s=*impl_;s.check();const auto use=s.lease.acquire();
    if(!s.active || view>=4 || (s.views&(1u<<view)) || !width || !height)throw FormatError("Original lens draw view is not uniquely active");
    s.views|=1u<<view;s.initialize(world);s.draws.clear();s.camera.fill(std::byte{});s.projection=projection;
    const auto copy=[&](unsigned p,const auto &v){std::memcpy(s.camera.data()+p,&v,sizeof(v));};
    copy(0,position);copy(12,forward);copy(24,up);copy(0x28,angle);
    const auto targets=s.renderer->target_resources().acquire();const auto &surface=targets.bank().depth_stencil;
    for(unsigned n=0;n<4;++n) {
        const auto scaled=std::int64_t(rectangle[n])*(n%2?surface.width:surface.height)/(n%2?width:height);
        if(scaled<0 || scaled>32767)throw FormatError("Original flare rectangle exceeds signed-short hardware coordinates");
        rectangle[n]=std::int16_t(scaled);
    }
    std::memcpy(s.camera.data()+0x30,rectangle.data(),8);std::memcpy(s.camera.data()+0x38,rectangle.data(),8);
    const float near=.015625f,far=1024.f;copy(0x40,near);copy(0x44,far);
    // Whole13660 reads the actual published view type/mode and BL copied by
    //132B0 from the view record. Stop at its actual world-pass call, with all
    // arguments retained; no original2BCB0 result is invented.
    auto gate=world.prepare_source_call();gate.gp[3]=word(*s.source,0x491e27,1);s.planning=true;
    bool enabled{};
    try{s.execute(world,0x13660,gate);throw FormatError("Original lens parent did not reach its world-pass handoff");}
    catch(const PassBoundary &pass){enabled=(pass.arguments[2]&255u)!=0;s.planning=false;}
    catch(...){s.planning=false;throw;}
    if(!enabled)return {};
    for(unsigned stage=0;stage<4;++stage)put(s.graphics,0x52dfa8+stage*4,0);
    s.run(world,0x1cf30);put(s.graphics,driver,fifo);
    const auto counter=word(*s.source,0x4f2d58);
    if(counter>0x7ffffffdu)throw FormatError("Original bitmap residency stamp exhausted its signed frame range");
    for(const auto &r:s.resources) {
        s.renderer->require_lens_bitmap(r.material,*r.texture);
        put(s.graphics,r.row+0x50,r.header);put(s.graphics,r.row+0x70,counter+2);
    }
    put(s.graphics,0x4c4c58,0);put(s.graphics,0x4117b4,0);put(s.graphics,0x4ce1e0,0,1);put(s.graphics,0x4ce224,0);
    auto program=world.prepare_source_call();program.gp[2]=0x491788;s.execute(world,0x1ccd0,program);
    put(s.graphics,driver,fifo);
    const std::array<unsigned,6> args{view,0xf,0,0,0,0x2e9c0};s.run(world,0x20b90,args);
    if(word(s.graphics,0x4ce224))throw FormatError("Original lens drawing requires its authored46E90 secondary consumer");
    s.consume();
    // Retire CPU SDK references after recording. Renderer owns all actual
    // images through GPU completion, independently of the source header refs.
    for(unsigned stage=0;stage<4;++stage)put(s.graphics,0x52dfa8+stage*4,0);
    s.run(world,0x1cf30);
    return std::move(s.draws);
}
void SourceLensDrawFrameTu15::finish(bool submitted) {
    auto &s=*impl_;s.check();if(!s.active)throw FormatError("Original lens finish lacks an active frame");
    if(submitted && s.source)put(s.graphics,driver+0x2c,++s.fence);
    s.active=false;
}
}
