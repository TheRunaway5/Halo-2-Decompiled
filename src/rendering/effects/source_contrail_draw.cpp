#include "halo2/source_contrail_draw.hpp"
#include "halo2/source_contrail_geometry_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/original_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_transparent_queue.hpp"
#include "source_contrail_program.hpp"
#include <cstring>
#include <map>
#include <set>
namespace halo2 {
namespace {
class ReadViews final:public SourcePhysicsContactMemory {
    const SourcePhysicsContactMemory &source;
    mutable std::map<std::pair<std::uint32_t,std::size_t>,std::vector<std::byte>> buffers;
public:
    explicit ReadViews(const SourcePhysicsContactMemory &s):source(s){}
    std::span<const std::byte> view(std::uint32_t p,std::size_t n)const {
        auto &b=buffers[{p,n}];b.resize(n);source.read(p,b);return b;
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override{source.read(p,out);}
    void write(std::uint32_t,std::span<const std::byte>)override{throw FormatError("Contrail read view cannot mutate source inputs");}
};
class DrawMemory final:public SourcePhysicsContactMemory {
public:
    static constexpr std::uint32_t base=0x6ff00000u;
    const SourcePhysicsContactMemory &source;Vec3 eye;
    std::vector<std::byte> scratch=std::vector<std::byte>(0x100000);
    DrawMemory(const SourcePhysicsContactMemory &m,Vec3 e):source(m),eye(e){}
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        if(p>=base && std::uint64_t(p)+out.size()<=std::uint64_t(base)+scratch.size())
            std::memcpy(out.data(),scratch.data()+p-base,out.size());
        else if(p>=0x4c65c0u && std::uint64_t(p)+out.size()<=0x4c65ccu)
            std::memcpy(out.data(),reinterpret_cast<const std::byte*>(&eye)+p-0x4c65c0u,out.size());
        else source.read(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(p<base || std::uint64_t(p)+in.size()>std::uint64_t(base)+scratch.size())
            throw FormatError("Contrail presentation attempted to mutate source state");
        std::memcpy(scratch.data()+p-base,in.data(),in.size());
    }
    void word(std::uint32_t p,std::uint32_t value){write(p,std::as_bytes(std::span(&value,1)));}
};
}
struct SourceContrailScene::Impl {
    struct Bank {std::uint32_t bitmap{};unsigned blend{};std::uint8_t properties{};ShaderTextureState sampler;std::vector<Texture> images;};
    std::map<std::uint32_t,Bank> banks;bool enabled{};
    const Cache &cache;mutable std::unique_ptr<SourceRuntimeMemory> replica_original_data;
    explicit Impl(const Cache &c):cache(c) {
        if(c.is_vista())return;
        if(c.is_mcc()) {
            const auto original=c.original_data();
            if(!original || original->image!=OriginalData::Image::TitleUpdate15)
                throw FormatError("MCC contrails require the native original TU1.5 owner");
            // The renderer executes the original vertex-global4/class0 program
            // compiled offline by SourceContrailGraphics.cmake. MCC's shader
            // resource contains a different platform program; it is not Xbox
            // vertex microcode and must not be presented as the original hash.
            static_assert(source_contrail_program_hash==0x9eb11be43eaff30aull);
        } else {
        const auto globals=c.query_tag({.group=tag_groups::globals});
        if(!globals)throw FormatError("Contrail renderer requires actual rasterizer globals");
        const Bytes root(c.tag_payload(globals->handle()));
        const Bytes raster(c.read_tag_bytes(root.u32(0x10c),0x108));
        const Bytes shaders(c.read_tag_bytes(raster.u32(0x5c),raster.u32(0x58)*8u));
        const auto code=c.load_vertex_shader_code(shaders.u32(4*8+4),0);
        std::uint64_t hash=14695981039346656037ull;
        for(auto b:code)hash=(hash^std::to_integer<unsigned char>(b))*1099511628211ull;
        if(hash!=source_contrail_program_hash)
            throw FormatError("Contrail effect vertex program differs from compiled original");
        }
        for(const auto &t:c.select_tags({.group=tag_groups::contrail,.require_address=true})) {
            const auto tag=c.load_source_tag_metadata(t.datum);
            const Bytes d(tag.bytes_at(tag.source_address(),tag.size()));
            if(d.u16(0x60)!=1)throw FormatError("Contrail requires its original non-2D bitmap render owner");
            Bank b;b.bitmap=d.u32(0x34);b.blend=d.u16(0x66);b.properties=std::uint8_t(d.u16(0x64));
            if(b.blend>10)throw FormatError("Contrail blend selector exceeds its original table");
            const auto flags=std::uint8_t(d.u16(0x6a));
            b.sampler.address_u=(flags&2u)|1u;b.sampler.address_v=((flags&4u)|2u)>>1u;
            b.sampler.address_w=b.sampler.mag_filter=b.sampler.min_filter=b.sampler.mip_filter=(flags&1u)?1:2;
            for(unsigned n=0;n<c.bitmap_count(b.bitmap);++n)b.images.push_back(c.load_bitmap(b.bitmap,n));
            if(b.images.empty())throw FormatError("Contrail bitmap has no actual images");
            banks.emplace(t.datum,std::move(b));
        }
        enabled=true;
    }
};
SourceContrailScene::SourceContrailScene(const Cache &c):impl_(std::make_unique<Impl>(c)){}
SourceContrailScene::~SourceContrailScene()=default;
std::vector<SourceContrailDraw> SourceContrailScene::draw(const SourcePhysicsContactMemory &source,
    Vec3 eye,Vec3 forward,const std::array<ShaderVector,192> &constants,
    ShaderVector viewport,ShaderVector clip)const {
    if(!impl_->enabled)return {};
    ReadViews memory(source);
    const auto word=[&](std::uint32_t p,unsigned n=4){const Bytes b(memory.view(p,n));return n==2?b.u16(0):b.u32(0);};
    const auto pool=word(0x4f7214u),points=word(0x4f7210u);
    if(word(pool+0x24)!=72 || word(points+0x24)!=56)
        throw FormatError("Contrail presentation lost its original pool strides");
    const auto high=word(pool+0x38),capacity=word(pool+0x20),point_capacity=word(points+0x20);
    if(high>capacity || capacity>4096 || point_capacity>16384)
        throw FormatError("Contrail pool exceeds its actual bounded storage");
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    DrawMemory call(memory,eye);constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](auto &m,const auto &c,auto &a,const auto &o) {
        auto controls=fp;source_run_physics_phantom_world_tu15_runtime(m,c.target,a,controls,services,o);
    };
    std::vector<SourceContrailDraw> result;SourceTransparentQueue queue;
    for(unsigned index=0;index<high;++index) {
        if(!(word(word(pool+0x48)+(index/32)*4)&(1u<<(index%32))))continue;
        const auto row=word(pool+0x44)+index*72;
        const auto handle=(word(row,2)<<16)|index;
        if(source_raw_datum_lookup(pool,handle,storage)!=row)throw FormatError("Contrail lost its full source identity");
        const auto definition=word(row+4),tagrow=word(0x4f0414u)+(definition&65535u)*16;
        if(word(tagrow+4)!=definition)throw FormatError("Contrail lost its actual definition identity");
        const auto tag=word(tagrow+8);const auto bank=impl_->banks.find(definition);
        if(bank==impl_->banks.end())throw FormatError("Live contrail definition was not prepared");
        // Actual2C16B ordinary world mask is FFFFFFF3, excluding modes2/3.
        if(!(0xfffffff3u&(1u<<(word(tag+0x18,2)&31u))))continue;
        SourcePhysicsWorldRuntimeAbi bitmap{};bitmap.gp[1]=bank->second.bitmap;
        bitmap.gp[7]=word(row+0x1a,2);bitmap.gp[4]=DrawMemory::base+0xf0000;bitmap.eflags=0x202;
        call.word(bitmap.gp[4],0);call.word(bitmap.gp[4]+4,word(row+0x18,2));
        source_run_contrail_geometry_tu15(call,0x13a070u,bitmap,fp,services);
        if(bitmap.gp[0]>=bank->second.images.size())throw FormatError("Original contrail image selector exceeds actual bitmap");
        for(unsigned chain=0;chain<4;++chain) {
            const auto count=word(row+0x30+chain*2,2);if(count<2)continue;
            if(count>point_capacity)throw FormatError("Contrail chain exceeds its actual point pool");
            const auto head=word(row+0x38+chain*4);std::set<std::uint32_t> visited;
            for(auto p=head;p!=0xffffffffu;) {
                const auto point=source_raw_datum_lookup(points,p,storage);
                if(!point || !visited.insert(p).second || visited.size()>count)
                    throw FormatError("Contrail point chain has a stale identity or cycle");
                p=word(point+0x34);
            }
            if(visited.size()!=count)throw FormatError("Contrail point count differs from actual chain");
            const auto output=DrawMemory::base+0x10000,context=DrawMemory::base+0x1000;
            call.word(context,row);call.word(context+4,tag);call.word(context+8,chain);
            call.word(context+12,tag+0x3c);call.word(context+16,word(tag+0x68,2)!=0);
            call.word(context+20,count*2);call.word(context+24,bank->second.bitmap);call.word(context+28,bitmap.gp[0]);
            SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=DrawMemory::base+0xf0000;a.eflags=0x202;
            call.word(a.gp[4],0);call.word(a.gp[4]+4,output);call.word(a.gp[4]+8,0);call.word(a.gp[4]+12,context);
            source_run_contrail_geometry_tu15(call,0x42ce0u,a,fp,services);
            if(a.gp[4]!=DrawMemory::base+0xf0010)throw FormatError("Contrail callback lost RET12 ABI");
            if(!(a.gp[0]&255u))continue;
            SourceContrailDraw draw;draw.datum=handle;draw.definition=definition;draw.chain=chain;draw.blend=bank->second.blend;
            draw.texture=&bank->second.images[bitmap.gp[0]];draw.sampler=bank->second.sampler;
            draw.pixel=source_contrail_pixel_program(draw.blend,bank->second.properties);
            if(draw.blend!=10)for(unsigned n=0;n<3;++n)draw.blend_operands[n]=word(std::array{0x45c438u,0x45c408u,0x45c3d8u}[n]+draw.blend*4);
            draw.constants=constants;draw.constants[174]={1,0,0,0};draw.constants[175]={0,1,0,0};draw.constants[176]={0,0,1,0};
            draw.viewport=viewport;draw.clip_range=clip;
            std::vector<SourceContrailVertex> data(count*2);call.read(output,std::as_writable_bytes(std::span(data)));
            for(unsigned n=2;n<data.size();++n)for(auto p:std::array{n-2+(n&1u),n-1-(n&1u),n})draw.vertices.push_back(data[p]);
            const auto point=source_raw_datum_lookup(points,head,storage);Vec3 center;
            memory.read(point+0x1c,std::as_writable_bytes(std::span(&center,1)));
            const std::array payload{handle,chain};
            const auto *command=queue.append_callback(center,eye,forward,0x35af0u,
                std::span(reinterpret_cast<const std::uint8_t*>(payload.data()),sizeof(payload)));
            if(!command)return result;
            draw.command=*command;result.push_back(std::move(draw));
        }
    }
    return result;
}
std::vector<SourceContrailDraw> SourceContrailScene::draw(const SourceContrailSnapshot &snapshot,
    Vec3 eye,Vec3 forward,const std::array<ShaderVector,192> &constants,ShaderVector viewport,ShaderVector clip)const {
    if(!snapshot.authoritative || !impl_->enabled)return {};
    if(!impl_->replica_original_data) {
        const auto original=impl_->cache.original_data();
        if(!original || original->image!=OriginalData::Image::TitleUpdate15)
            throw FormatError("Contrail replica requires authenticated original TU1.5 static data");
        impl_->replica_original_data=std::make_unique<SourceRuntimeMemory>(impl_->cache.is_mcc()
            ? native_tu15_world_memory(SourceRuntimeKernelTickCount{0x6fe00000u,{},
                "contrail replica static-data bank unused tick import"})
            : load_source_runtime_image(original->source,SourcePhysicsContactImage::TitleUpdate15));
    }
    SourceContrailSnapshotMemory memory(snapshot,*impl_->replica_original_data);
    return draw(memory,eye,forward,constants,viewport,clip);
}
}
