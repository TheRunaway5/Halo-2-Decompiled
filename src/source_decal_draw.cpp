#include "halo2/source_decal_draw.hpp"
#include "halo2/cache.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "source_decal_program.hpp"
#include <bit>
#include <map>

namespace halo2 {
struct SourceDecalScene::Impl {
    struct Definition {unsigned layer{},images{};std::vector<Texture> textures;};
    std::map<std::uint32_t,Definition> definitions;
    std::map<std::uint32_t,std::string> unavailable;
    bool enabled{};
    explicit Impl(const Cache &cache) {
        if(cache.is_vista() || cache.is_mcc())return;
        const auto globals=cache.query_tag({.group=tag_groups::globals});
        if(!globals)throw FormatError("Cached decal renderer requires rasterizer globals");
        const Bytes root(cache.tag_payload(globals->handle()));
        const Bytes raster(cache.read_tag_bytes(root.u32(0x10c),0x108));
        const Bytes shaders(cache.read_tag_bytes(raster.u32(0x5c),raster.u32(0x58)*8u));
        const auto datum=shaders.u32(3*8+4);
        const auto code=cache.load_vertex_shader_code(datum,0);
        const auto declaration=cache.load_source_vertex_declaration(datum,0);
        std::uint64_t hash=14695981039346656037ull;
        for(auto b:code)hash=(hash^std::to_integer<unsigned char>(b))*1099511628211ull;
        if(hash!=source_decal_program_hash || declaration.semantics!=std::vector<std::uint16_t>({0,3}))
            throw FormatError("Cached decal vertex program differs from compiled original");
        for(const auto &tag:cache.select_tags({.group=tag_groups::decal,.require_address=true}))try {
            const Bytes d(cache.tag_payload(tag.handle()));
            Definition def;def.layer=d.u16(4);(void)source_decal_layer_state_tu15(def.layer);
            const auto bitmap=cache.query_tag({.handle=TagHandle{d.u32(0x8c)},.group=tag_groups::bitmap});
            if(!bitmap)throw FormatError("Cached decal bitmap fallback remains unbound");
            const Bytes b(cache.tag_payload(bitmap->handle()));def.images=b.u32(0x44);
            if(!def.images || def.images>128)throw FormatError("Cached decal bitmap index range unavailable");
            for(unsigned image=0;image<def.images;++image) {
                auto texture=cache.load_bitmap(bitmap->datum,image);
                if(texture.layers!=1 || texture.depth)
                    throw FormatError("Cached decal requires original two-dimensional bitmap fallback");
                def.textures.push_back(std::move(texture));
            }
            definitions.emplace(tag.datum,std::move(def));
        }catch(const FormatError&e){unavailable.emplace(tag.datum,e.what());}
        enabled=true;
    }
};
SourceDecalScene::SourceDecalScene(const Cache &c):impl_(std::make_unique<Impl>(c)){}
SourceDecalScene::~SourceDecalScene()=default;
bool SourceDecalScene::active()const{return impl_->enabled;}
std::vector<SourceDecalDraw> SourceDecalScene::draw(SourceRuntimeMemory &memory,
    const SourceBspAdmission &view,const std::array<ShaderVector,192>&constants,
    ShaderVector viewport,ShaderVector clip_range)const {
    if(!impl_->enabled)return {};
    const auto read=[&](std::uint32_t p,unsigned n=4){std::uint32_t v{};auto b=memory.view(p,n);
        for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(i*8);return v;};
    const auto records=source_decal_vertex_cache_tu15(memory);
    std::map<std::uint32_t,const SourceDecalCacheGeometry*> by_handle;
    for(const auto &record:records) {
        if(record.layer>5)throw FormatError("Live cached decal layer has no native draw state");
        by_handle.emplace(record.datum,&record);
    }
    const auto context=read(0x4f721cu),pool=read(0x4f7220u),data=read(pool+0x44u);
    // The original view context increments this same serial at 2BA65/2BA73.
    // Preserve its low16 stamp semantics and primary-record side effect.
    const auto serial=read(0x4c6858u)+1u;std::array<std::byte,4> serial_bytes;
    for(unsigned i=0;i<4;++i)serial_bytes[i]=std::byte(serial>>(i*8));
    memory.write(0x4c6858u,serial_bytes);
    std::vector<SourceDecalDraw> result;
    for(unsigned layer=0;layer<=5;++layer)for(const auto &cluster:view.clusters) {
        if(cluster.id>=512)throw FormatError("Cached decal cluster exceeds original head bank");
        auto handle=read(context+(layer*512u+cluster.id)*4u);unsigned traversed{};
        while(handle!=0xffffffffu) {
            if(++traversed>1024)throw FormatError("Cyclic cached decal cluster list");
            const auto found=by_handle.find(handle);
            if(found==by_handle.end())throw FormatError("Cached decal cluster links a retired full handle");
            const auto &record=*found->second;handle=record.next_cluster_datum;
            if(record.layer!=layer)throw FormatError("Cached decal layer differs from its cluster head");
            const auto primary=by_handle.find(record.primary_datum);
            if(primary==by_handle.end())throw FormatError("Cached decal primary handle is unavailable");
            const auto stamp=data+(record.primary_datum&65535u)*64u+8u;
            if(read(stamp,2)==(serial&65535u))continue;
            const auto definition=impl_->definitions.find(record.definition);
            if(definition==impl_->definitions.end()) {
                const auto gap=impl_->unavailable.find(record.definition);
                throw FormatError(gap==impl_->unavailable.end()?"Cached decal definition was not prepared":gap->second);
            }
            const auto &d=definition->second;
            if(d.layer!=record.layer || record.bitmap_index<0)
                throw FormatError("Cached decal retained layer/image does not match its authored definition");
            const std::array stamp_bytes{std::byte(serial),std::byte(serial>>8)};
            memory.write(stamp,stamp_bytes);
            SourceDecalDraw draw;draw.datum=record.datum;draw.primary=record.primary_datum;
            draw.cache_datum=record.cache_datum;draw.layer=layer;
            draw.texture=&d.textures[unsigned(record.bitmap_index)%d.images];
            draw.color={float((record.tint>>16)&255u)/255.f,float((record.tint>>8)&255u)/255.f,
                float(record.tint&255u)/255.f,float(record.tint>>24)/255.f};
            draw.state=source_decal_layer_state_tu15(layer);draw.quads=record.quads;
            draw.constants=constants;draw.viewport=viewport;draw.clip_range=clip_range;
            result.push_back(std::move(draw));
        }
    }
    return result;
}
}
