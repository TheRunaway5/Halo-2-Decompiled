#include "halo2/bsp_lighting.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>

namespace {
using namespace halo2;
void require(bool valid,const char* message) {
    if(!valid)throw std::runtime_error(message);
}
std::vector<std::byte> read(const std::filesystem::path& path,std::size_t at,std::size_t size) {
    std::ifstream stream(path,std::ios::binary);
    require(bool(stream),"fixture resource missing");
    stream.seekg(static_cast<std::streamoff>(at));
    std::vector<std::byte> result(size);
    stream.read(reinterpret_cast<char*>(result.data()),static_cast<std::streamsize>(size));
    require(bool(stream),"fixture resource truncated");
    return result;
}
struct Block { std::size_t size{},at{}; };
Block block(const Bytes& b,std::size_t p,std::int64_t magic) {
    const auto count=b.u32(p);
    if(!count)return {};
    const auto at=std::int64_t(b.u32(p+4))-magic;
    require(at>=0,"fixture block pointer out of bounds");
    return {count,std::size_t(at)};
}
std::vector<std::byte> resource(const std::filesystem::path& map,std::uint32_t pointer,std::size_t size) {
    constexpr const char* names[]{"","mainmenu.map","shared.map","single_player_shared.map"};
    return read(pointer>>30 ? map.parent_path()/names[pointer>>30] : map,pointer&0x3fffffff,size);
}
struct Position {
    std::array<std::uint32_t,3> words;
    explicit Position(Vec3 p):words{std::bit_cast<std::uint32_t>(p.x),std::bit_cast<std::uint32_t>(p.y),std::bit_cast<std::uint32_t>(p.z)}{}
    bool operator==(const Position&)const=default;
};
struct Hash {
    std::size_t operator()(const Position& p)const {
        std::size_t h=0;
        for(const auto v:p.words)h^=v+0x9e3779b9u+(h<<6)+(h>>2);
        return h;
    }
};
bool close(Vec3 a,Vec3 b) {
    return std::abs(a.x-b.x)<2e-6f && std::abs(a.y-b.y)<2e-6f && std::abs(a.z-b.z)<2e-6f;
}
Vec3 point(const Bytes& bytes,std::size_t at) {
    return {bytes.f32(at),bytes.f32(at+4),bytes.f32(at+8)};
}
std::size_t verify_scenery(const Cache& cache,const std::filesystem::path& path,
                         const Bytes& meta,std::int64_t magic,std::size_t group) {
    const auto infos=block(meta,group+88,magic),refs=block(meta,group+96,magic);
    const auto buckets=block(meta,group+64,magic);
    const auto lighting=cache.load_bsp_lighting(0);
    require(lighting.structure_matches,"multiplayer authored lightmap checksum must match BSP geometry");
    const auto content=cache.load_content(0,true);
    require(infos.size==refs.size && infos.size==lighting.scenery.size(),"scenery identity/reference fixture count mismatch");
    std::size_t vertices=0;
    for(std::size_t i=0;i<infos.size;++i) {
        const auto info=infos.at+i*12,ref=refs.at+i*12;
        const auto type=static_cast<std::int8_t>(std::to_integer<unsigned>(meta.slice(info+6,1)[0]));
        const auto source=static_cast<std::int8_t>(std::to_integer<unsigned>(meta.slice(info+7,1)[0]));
        const auto origin=static_cast<std::int16_t>(meta.u16(info+4));
        const auto object=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto& o) {
            return o.unique_id==meta.u32(info) && o.type==type && o.source==source &&
                (source!=0 || o.origin_bsp==origin);
        });
        require(object!=content.objects.end(),"raw scenery lightmap identity did not match original placement");
        auto model=cache.load_model(object->definition,true,object->variant);
        require(model.checksum==meta.u32(info+8),"raw scenery render-model checksum mismatch");
        require(cache.apply_scenery_lighting(model,lighting,*object),"authored scenery lighting was not admitted");
        const auto bucket_index=meta.u16(ref+2);
        require(bucket_index<buckets.size,"scenery raw bucket invalid");
        const auto bucket=buckets.at+bucket_index*56;
        const auto raw=resource(path,meta.u32(bucket+12),meta.u32(bucket+16));
        const Bytes bytes(raw);
        const auto base=std::size_t(meta.u32(bucket+20))+8;
        const auto resources=block(meta,bucket+28,magic),offsets=block(meta,ref+4,magic);
        std::size_t incident=0,radiance=0;
        for(std::size_t j=0;j<resources.size;++j) {
            const auto r=resources.at+j*16;
            if(meta.slice(r,1)[0]!=std::byte{2} || meta.u16(r+4)!=0)continue;
            if(meta.u16(r+6)==0)incident=base+meta.u32(r+12);
            if(meta.u16(r+6)==1)radiance=base+meta.u32(r+12);
        }
        require(incident && radiance,"scenery fixture must have both authored lighting streams");
        const auto posed=pose_model(model);
        require(posed.size()==model.vertex_origins.size(),"scenery pose changed vertex ordering");
        for(std::size_t v=0;v<posed.size();++v) {
            const auto index=model.vertex_origins[v];
            require(index.section<offsets.size,"scenery source section outside offset fixture");
            const auto source_vertex=std::size_t(meta.u16(offsets.at+index.section*2))+index.vertex;
            const auto packed=bytes.u32(incident+source_vertex*4);
            const auto sign=[](unsigned n,unsigned bits) { return int(n^(1u<<(bits-1)))-int(1u<<(bits-1)); };
            const Vec3 expected_incident{float(sign(packed&2047,11))/1023.f,
                float(sign((packed>>11)&2047,11))/1023.f,float(sign(packed>>22,10))/511.f};
            const Vec3 expected_radiance{
                float(std::to_integer<unsigned>(bytes.slice(radiance+source_vertex*3,1)[0]))*0.003921568859368563f,
                float(std::to_integer<unsigned>(bytes.slice(radiance+source_vertex*3+1,1)[0]))*0.003921568859368563f,
                float(std::to_integer<unsigned>(bytes.slice(radiance+source_vertex*3+2,1)[0]))*0.003921568859368563f};
            require(close(posed[v].incident,expected_incident) && close(posed[v].color,expected_radiance),"scenery authored incident/radiance changed through native pose");
            const auto& material=model.mesh.materials.at(posed[v].material);
            if(!material.world_pass)
                throw std::runtime_error(path.stem().string()+" scenery "+std::to_string(object->placement)+" unsupported authored material "+material.name);
            if(material.environment_pass || material.forward_vertex==6)
                require(material.environment_pass ? material.environment_pass->per_vertex_lighting : material.per_vertex_lighting,
                        "scenery pass did not select authored vertex-light implementation");
            ++vertices;
        }
        auto mismatch=*object;mismatch.unique_id^=0x10000u;
        require(!cache.apply_scenery_lighting(model,lighting,mismatch),"scenery identity mismatch was admitted");
        mismatch=*object;mismatch.origin_bsp+=1;
        require(cache.apply_scenery_lighting(model,lighting,mismatch)==(source!=0),"scenery origin BSP predicate differs from original 10A8B0");
    }
    std::cout<<path.stem().string()<<": "<<infos.size<<" authored scenery objects, "<<vertices<<" source vertex lighting samples\n";
    return vertices;
}
// An independent raw-cache fixture reader samples the first triangle of every
// authored visible part, including every repeated instance and every bucket
// offset. It checks the final expanded mesh at the actual transformed position.
std::size_t verify(const std::filesystem::path& path) {
    const Cache cache(path);
    require(cache.bsps().size()==1,"multiplayer fixture BSP count changed");
    const auto& bsp=cache.bsps()[0];
    const auto storage=read(path,bsp.file_offset,bsp.size);
    const Bytes meta(storage);
    const auto magic=std::int64_t(bsp.memory_address);
    const auto mesh=cache.load_bsp(0);
    std::unordered_multimap<Position,std::size_t,Hash> positions;
    positions.reserve(mesh.vertices.size());
    for(std::size_t v=0;v<mesh.vertices.size();++v)positions.emplace(Position(mesh.vertices[v].position),v);
    const auto ltmp=std::size_t(std::int64_t(meta.u32(8))-magic);
    const auto groups=block(meta,ltmp+128,magic);
    require(groups.size==1,"multiplayer fixture must retain exactly one authored lighting group");
    const auto group=groups.at;
    const auto refs=block(meta,group+80,magic),buckets=block(meta,group+64,magic);
    const auto bindings=block(meta,group+72,magic),clusters=block(meta,16+156,magic);
    const auto instances=block(meta,16+320,magic),defs=block(meta,16+312,magic);
    const auto materials=block(meta,16+164,magic);
    require(refs.size==instances.size && bindings.size==instances.size,"instance lighting count changed");
    std::size_t verified=0,vertex_instances=0,pixel_instances=0;
    for(std::size_t i=0;i<instances.size;++i) {
        const auto instance=instances.at+i*88;
        const auto definition=meta.u16(instance+52);
        require(definition<defs.size,"fixture definition invalid");
        const auto section=defs.at+definition*200;
        if(!meta.u16(section))continue;
        const bool per_vertex=meta.u16(instance+86)==1;
        const bool bitmap_bound=static_cast<std::int16_t>(meta.u16(bindings.at+i*4))>=0;
        const auto ref=refs.at+i*12;
        const auto bucket_index=meta.u16(ref+2);
        require(bucket_index<buckets.size,"fixture bucket invalid");
        const auto offsets=block(meta,ref+4,magic);
        require(offsets.size==1,"fixture instance must have one section");
        const auto vertex_offset=meta.u16(offsets.at);
        const auto bucket=buckets.at+bucket_index*56;
        const auto bucket_storage=resource(path,meta.u32(bucket+12),meta.u32(bucket+16));
        const Bytes raw_bucket(bucket_storage);
        const auto bucket_base=std::size_t(meta.u32(bucket+20))+8;
        const auto bucket_resources=block(meta,bucket+28,magic);
        std::size_t directions=0,colors=0;
        for(std::size_t j=0;j<bucket_resources.size;++j) {
            const auto r=bucket_resources.at+j*16;
            if(meta.u16(r+4)!=0)continue;
            if(meta.u16(r+6)==0)directions=bucket_base+meta.u32(r+12);
            if(meta.u16(r+6)==1)colors=bucket_base+meta.u32(r+12);
        }
        const auto geometry_storage=resource(path,meta.u32(section+40),meta.u32(section+44));
        const Bytes geometry(geometry_storage);
        const auto geometry_base=std::size_t(meta.u32(section+48))+8;
        const auto resources=block(meta,section+56,magic);
        const auto part_resource=resources.at;
        const auto parts=geometry_base+meta.u32(part_resource+12);
        const auto part_count=meta.u32(part_resource+8)/72;
        std::size_t index_buffer=0,position_buffer=0,position_stride=0;
        for(std::size_t j=0;j<resources.size;++j) {
            const auto r=resources.at+j*16;
            if(meta.u16(r+4)==32)index_buffer=geometry_base+meta.u32(r+12);
            if(meta.u16(r+4)==56 && meta.u16(r+6)==0) {
                position_buffer=geometry_base+meta.u32(r+12);
                position_stride=meta.u32(r+8)/meta.u16(section);
            }
        }
        require(index_buffer && position_buffer && directions,"fixture stream missing");
        bool sampled=false;
        for(std::size_t j=0;j<part_count;++j) {
            const auto p=parts+j*72;
            const auto type=geometry.u16(p),material=geometry.u16(p+4),first=geometry.u16(p+6),count=geometry.u16(p+8);
            if(type==0 || type==1 || type==5 || count<3)continue;
            require(material<materials.size,"visible fixture material invalid");
            const auto shader=meta.u32(materials.at+material*32+12);
            const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto& t){return t.datum==shader;});
            require(tag!=cache.tags().end(),"visible fixture shader missing");
            std::vector<unsigned> corners{0,1,2};
            std::vector<std::uint16_t> minimum_vertices;
            for(unsigned corner=3;corner<count;++corner) {
                const auto vertex=geometry.u16(index_buffer+(std::size_t(first)+corner)*2);
                const auto packed=raw_bucket.u32(directions+(std::size_t(vertex_offset)+vertex)*4);
                if((packed&2047)!=1024 && ((packed>>11)&2047)!=1024 && ((packed>>22)&1023)!=512)continue;
                if(std::find(minimum_vertices.begin(),minimum_vertices.end(),vertex)!=minimum_vertices.end())continue;
                minimum_vertices.push_back(vertex);
                corners.push_back(corner);
            }
            for(const auto corner:corners) {
                const auto source_vertex=geometry.u16(index_buffer+(std::size_t(first)+corner)*2);
                const auto raw_position=point(geometry,position_buffer+std::size_t(source_vertex)*position_stride);
                const auto m=instance;
                const float scale=meta.f32(m);
                // Preserve retail 4x3 multiplication order for exact lookup.
                const Vec3 transformed{
                    scale*(raw_position.x*meta.f32(m+4)+raw_position.y*meta.f32(m+16)+raw_position.z*meta.f32(m+28))+meta.f32(m+40),
                    scale*(raw_position.x*meta.f32(m+8)+raw_position.y*meta.f32(m+20)+raw_position.z*meta.f32(m+32))+meta.f32(m+44),
                    scale*(raw_position.x*meta.f32(m+12)+raw_position.y*meta.f32(m+24)+raw_position.z*meta.f32(m+36))+meta.f32(m+48)};
                const auto expected_incident=decode_bsp_incident(raw_bucket.u32(directions+(std::size_t(vertex_offset)+source_vertex)*4));
                Vec3 expected_radiance{1,1,1};
                if(per_vertex) {
                    require(colors!=0,"per-vertex fixture color stream missing");
                    const auto bytes=raw_bucket.slice(colors+(std::size_t(vertex_offset)+source_vertex)*3,3);
                    expected_radiance=decode_bsp_radiance({std::to_integer<std::uint8_t>(bytes[0]),std::to_integer<std::uint8_t>(bytes[1]),std::to_integer<std::uint8_t>(bytes[2])});
                }
                const auto [begin,end]=positions.equal_range(Position(transformed));
                bool found=false;
                for(auto it=begin;it!=end;++it) {
                    const auto& actual=mesh.vertices[it->second];
                    const auto& authored=mesh.materials.at(actual.material);
                    if(authored.name!=tag->name || actual.bsp_cluster!=-2-static_cast<std::int32_t>(i) ||
                       !close(actual.incident,expected_incident) || (per_vertex && !close(actual.color,expected_radiance)))continue;
                    const auto& lighting=authored.environment_pass ? *authored.environment_pass : authored;
                    if(lighting.world_pass && lighting.forward_vertex==6)
                        require(lighting.per_vertex_lighting==per_vertex,"world material lighting category drifted");
                    if((actual.lightmap!=0)!=bitmap_bound)continue;
                    found=true;
                    break;
                }
                if(!found) {
                    for(const auto &warning:mesh.material_warnings)std::cerr<<"material warning: "<<warning<<'\n';
                    std::cerr<<"expected "<<tag->name<<" incident "<<expected_incident.x<<','<<expected_incident.y<<','<<expected_incident.z<<" radiance "<<expected_radiance.x<<','<<expected_radiance.y<<','<<expected_radiance.z<<" bitmap "<<bitmap_bound<<'\n';
                    for(auto it=begin;it!=end;++it) {
                        const auto &actual=mesh.vertices[it->second];
                        std::cerr<<"actual "<<mesh.materials.at(actual.material).name<<" incident "<<actual.incident.x<<','<<actual.incident.y<<','<<actual.incident.z<<" radiance "<<actual.color.x<<','<<actual.color.y<<','<<actual.color.z<<" bitmap "<<actual.lightmap<<" cluster "<<actual.bsp_cluster<<'\n';
                    }
                    throw std::runtime_error(path.stem().string()+" instance "+std::to_string(i)+" part "+std::to_string(j)+" corner "+std::to_string(corner)+" authored lighting not found at transformed position "+std::to_string(transformed.x)+","+std::to_string(transformed.y)+","+std::to_string(transformed.z));
                }
                ++verified;
            }
            sampled=true;
        }
        if(sampled) { if(per_vertex)++vertex_instances; else ++pixel_instances; }
    }
    require(verified>0 && clusters.size>0,"multiplayer lighting fixture has no samples");
    std::cout<<path.stem().string()<<": "<<verified<<" authored triangle vertices, "<<pixel_instances<<" pixel and "<<vertex_instances<<" vertex-lit instances\n";
    return verified+verify_scenery(cache,path,meta,magic,group);
}
}
int main(int argc,char** argv) {
    try {
        if(argc==2 && std::string_view(argv[1])=="--radiance") {
            unsigned r,g,b;
            while(std::cin>>r>>g>>b) {
                const auto v=decode_bsp_radiance({std::uint8_t(r),std::uint8_t(g),std::uint8_t(b)});
                std::cout<<std::bit_cast<std::uint32_t>(v.x)<<' '<<std::bit_cast<std::uint32_t>(v.y)<<' '<<std::bit_cast<std::uint32_t>(v.z)<<'\n';
            }
            return 0;
        }
        if(argc==5 && std::string_view(argv[1])=="--radiance") {
            const auto v=decode_bsp_radiance({std::uint8_t(std::stoul(argv[2])),std::uint8_t(std::stoul(argv[3])),std::uint8_t(std::stoul(argv[4]))});
            std::cout<<std::bit_cast<std::uint32_t>(v.x)<<' '<<std::bit_cast<std::uint32_t>(v.y)<<' '<<std::bit_cast<std::uint32_t>(v.z)<<'\n';
            return 0;
        }
        if(argc==2 && std::filesystem::path(argv[1]).extension()==".map") {
            verify(argv[1]);return 0;
        }
        require(argc==2 || argc==3,"expected original maps directory and optional map-pack maps directory");
        constexpr const char* names[]{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar"};
        std::size_t count=0;
        for(const auto name:names)count+=verify(std::filesystem::path(argv[1])/(std::string(name)+".map"));
        const auto pack=argc==3?std::filesystem::path(argv[2]):std::filesystem::path(argv[1]).parent_path().parent_path()/"map-pack/maps";
        unsigned maps=12;
        if(argc==3 || std::filesystem::exists(pack)) {
            constexpr const char* extra[]{"backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
            for(const auto name:extra)count+=verify(pack/(std::string(name)+".map"));
            maps+=std::size(extra);
        }
        // The authored cyclotron bucket contains signed X=-1024 at a drawn
        // instance vertex. NV2A's packed GPU reference preserves its quotient.
        const auto minimum=decode_bsp_incident(0x400);
        require(minimum.x < -1.f,"minimum incident encoding was incorrectly clamped");
        std::cout<<count<<" original-cache bucket samples verified across all "<<maps<<" multiplayer maps\n";
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';return 1;
    }
}
