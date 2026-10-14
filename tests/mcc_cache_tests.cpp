#include "halo2/cache.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/model.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/unit_animation.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include "halo2/tags/system.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <zlib.h>

namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void word(Buffer& b,std::size_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)b.at(at+i)=std::byte(value>>(8*i));}
void text(Buffer& b,std::size_t at,const std::string& value){for(std::size_t i=0;i<value.size();++i)b.at(at+i)=std::byte(value[i]);}
void save(const std::filesystem::path& path,const Buffer& b){std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));check(bool(out),"Fixture write failed");}
template<class F>void rejects(F action){try{action();}catch(const FormatError&){return;}throw std::runtime_error("Malformed MCC data was admitted");}
Buffer fixture(bool shared){
    Buffer b(8192);const auto magic=(shared?0x100000u:0x200000u)-4096;
    word(b,0,0x68656164);word(b,4,13);word(b,8,b.size());word(b,16,1024);word(b,32,3);
    word(b,36,2048);word(b,40,128);word(b,44,2304);word(b,720,1024+magic);
    word(b,1024,0x400000+1056);word(b,1028,2);word(b,1032,0x400000+1080);
    word(b,1036,shared?0xffffffffu:0xe0000001u);word(b,1040,0xffffffffu);word(b,1048,3);word(b,1052,0x74616773);
    word(b,1056,0x6269746d);word(b,1060,0xffffffffu);word(b,1064,0xffffffffu);
    word(b,1068,0x73636e72);word(b,1072,0xffffffffu);word(b,1076,0xffffffffu);
    word(b,1084,0xffffffffu);
    word(b,1096,0x73636e72);word(b,1100,shared?0xffffffffu:0xe0000001u);
    word(b,1112,0x6269746d);word(b,1116,0xe1000002u);
    text(b,2048,"scenarios\\fixture");text(b,2112,"textures\\original");word(b,2308,0);word(b,2312,64);
    if(shared){
        word(b,1120,0x100000);word(b,1124,256);
        word(b,4096+68,1);word(b,4096+72,0x100000+80);
        const auto p=4096+80;word(b,p+4,2|(2u<<16));word(b,p+8,0x801);word(b,p+12,11|(513u<<16));
        word(b,p+56,0x80000000u);word(b,p+80,24);
    }else{
        word(b,1104,0x200000);word(b,1108,0x24c);
        word(b,4096+0x100,1);word(b,4096+0x104,0x200218);
        word(b,4096+0x218,0x3fa00000);word(b,4096+0x21c,0xbf000000);word(b,4096+0x220,0x41100000);
    }
    return b;
}
Buffer compressed(const Buffer& input,bool raw){
    Buffer out(input.begin(),input.begin()+896);word(out,28,1);word(out,776,8192);word(out,780,904);word(out,784,896);word(out,788,1);
    Buffer block;
    if(raw){block.assign(input.begin()+896,input.end());}
    else{
        uLongf size=compressBound(input.size()-896);block.resize(size);
        check(compress2(reinterpret_cast<Bytef*>(block.data()),&size,reinterpret_cast<const Bytef*>(input.data()+896),input.size()-896,Z_BEST_SPEED)==Z_OK,"Fixture compression failed");block.resize(size);
    }
    out.resize(904);word(out,896,raw?std::uint32_t(-std::int32_t(input.size()-896)):std::uint32_t(block.size()));word(out,900,904);
    out.insert(out.end(),block.begin(),block.end());return out;
}
Buffer mixed_chunks(const Buffer& input){
    Buffer out(input.begin(),input.begin()+896);out.resize(920);
    word(out,28,1);word(out,776,8192);word(out,780,920);word(out,784,896);word(out,788,3);
    const std::array<std::size_t,4> offsets{896,1025,4097,input.size()};
    for(unsigned i=0;i<3;++i){
        const auto size=offsets[i+1]-offsets[i];Buffer block;
        if(i==1){
            uLongf extent=compressBound(size);block.resize(extent);
            check(compress2(reinterpret_cast<Bytef*>(block.data()),&extent,
                reinterpret_cast<const Bytef*>(input.data()+offsets[i]),size,Z_BEST_SPEED)==Z_OK,
                "Mixed fixture compression failed");block.resize(extent);
        }else block.assign(input.begin()+offsets[i],input.begin()+offsets[i+1]);
        word(out,896+i*8,i==1?std::uint32_t(block.size()):std::uint32_t(-std::int32_t(size)));
        word(out,900+i*8,std::uint32_t(out.size()));out.insert(out.end(),block.begin(),block.end());
    }
    return out;
}
}
int main(){
    const auto root=std::filesystem::temp_directory_path()/("halo2-mcc-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try{
        std::filesystem::create_directories(root);
        const auto local=fixture(false),shared=fixture(true);
        for(bool raw:{false,true}){
            const auto encoded=compressed(local,raw);const auto decoded=decode_mcc_cache(encoded);
            check(decoded.size()==local.size() && std::equal(decoded.begin()+896,decoded.end(),local.begin()+896),"MCC decoded metadata changed");
            auto bad=encoded;word(bad,776,16);rejects([&]{(void)decode_mcc_cache(bad);});
            bad=encoded;bad.pop_back();rejects([&]{(void)decode_mcc_cache(bad);});
        }
        {
            auto source=local;
            // A raw entry may begin with zlib's magic. Its signed table size
            // still selects literal bytes, including both ends of the chunk.
            source[896]=std::byte{0x78};source[897]=std::byte{0x9c};
            const auto encoded=mixed_chunks(source),decoded=decode_mcc_cache(encoded);
            check(std::ranges::equal(std::span(decoded).subspan(896),std::span(source).subspan(896)),
                  "Adjacent raw/zlib/raw MCC chunks lost or borrowed boundary bytes");
            auto bad=encoded;bad.pop_back();rejects([&]{(void)decode_mcc_cache(bad);});
        }
        save(root/"fixture.map",compressed(local,false));save(root/"shared.map",compressed(shared,true));
        Buffer texture(24);word(texture,0,1);word(texture,4,std::uint32_t(-16));
        for(unsigned i=0;i<4;++i){texture[8+i*4]=std::byte{1};texture[9+i*4]=std::byte{2};texture[10+i*4]=std::byte{3};texture[11+i*4]=std::byte{255};}
        save(root/"textures.dat",texture);
        const Cache cache(root/"fixture.map");
        check(cache.is_mcc() && !cache.is_vista(),"MCC cache identity changed");
        check(cache.original_data()==OriginalData::native_tu15() &&
              cache.original_data()->image==OriginalData::Image::TitleUpdate15,
              "MCC cache did not select its compiled native TU1.5 engine operands");
        check(cache.tags().size()==3 && cache.tags()[0].datum==0xffffffffu,"Sparse MCC slots collapsed");
        check(cache.spawns().size()==1 && cache.spawns()[0].position.x==1.25f,"MCC player-start owner changed");
        const auto metadata=cache.load_source_tag_metadata(0xe1000002u);
        check(std::ranges::equal(metadata.bytes_at(0x100000,256),std::span(shared).subspan(4096,256)),"MCC shared tag bytes changed");
        const auto image=cache.load_bitmap(0xe1000002u,0);const auto resource=cache.load_bitmap_resource(0xe1000002u,0);
        check(image.width==2 && image.height==2 && image.pixels[0]==std::byte{3} && image.pixels[1]==std::byte{2} && image.pixels[2]==std::byte{1},"MCC BGRA pixels were not preserved");
        check(resource.levels[0].size()==16 && resource.levels[0][0]==std::byte{1},"MCC residency changed encoded pixels");
        {
            TagSystem tags(std::make_shared<const Cache>(root/"fixture.map"));
            const auto bitmap=tags.get({0xe1000002u},tag_groups::bitmap);
            check(bitmap.info().datum==0xe1000002u && bitmap.source_address()==0x100000,"Native tag system changed MCC shared identity/address");
            check(bitmap.address().value>0xffffffffull && tags.source_address(bitmap.address(),256)==0x100000,
                  "Native MCC tag owner did not use a checked 64-bit address");
            const auto frames=bitmap.block(0x44,168);
            check(frames.size()==1 && frames.element(0).u16(4)==2,"Native tag system did not retain MCC resident bitmap metadata");
            rejects([&]{(void)tags.read_source(0x100100,1);});
        }
        rejects([&]{(void)cache.load_bitmap(0xe2000002u,0);});
        {
            auto physics_local=local,physics_shared=shared;
            for(auto* owner:{&physics_local,&physics_shared}) {
                word(*owner,1056,0x70686d6f);word(*owner,1112,0x70686d6f);
            }
            word(physics_shared,1124,0x200);
            std::fill(physics_shared.begin()+4096,physics_shared.begin()+4608,std::byte{});
            constexpr unsigned root_offset=4096,sphere=root_offset+0x110;
            word(physics_shared,root_offset+0x48,1);word(physics_shared,root_offset+0x4c,0x100110);
            word(physics_shared,sphere+0x20,0x12345678u);word(physics_shared,sphere+0x24,0xabcdef01u);
            word(physics_shared,sphere+0x28,0x80000000u);word(physics_shared,sphere+0x30,0x33333333u);
            word(physics_shared,sphere+0x34,0x99999999u);word(physics_shared,sphere+0x38,0x3f000000u);
            word(physics_shared,sphere+0x60,0x3fa00000u);
            word(physics_shared,root_offset+0xa8,6);word(physics_shared,root_offset+0xac,0x1001c0);
            for(unsigned i=0;i<6;++i)physics_shared[root_offset+0x1c0+i]=std::byte(i*13+7);
            save(root/"physics.map",physics_local);save(root/"shared.map",physics_shared);
            TagSystem tags(std::make_shared<const Cache>(root/"physics.map"));
            const auto model=tags.get({0xe1000002u},TagGroup{0x70686d6fu});
            check(model.info().name=="textures\\original" && model.info().datum==0xe1000002u &&
                  model.source_address()!=0x100000,"Canonical physics changed authored tag identity or retained donor addresses");
            const auto shape=model.block(0x48,0x80).element(0);
            check(shape.address().value>0xffffffffull && shape.u32(0x20)==0 && shape.u32(0x28)==0 &&
                  shape.u32(0x2c)==0x3f000000u && shape.u32(0x40)==0x3fa00000u,
                  "Native physics did not own the converted shape or changed authored values");
            const auto code=model.block(0xa8,1);
            check(code.size()==6 && code.element(5).bytes()[0]==std::byte{72},"Canonical physics changed MOPP bytecode");
            rejects([&]{(void)tags.read_source(0x100000,1);});
            save(root/"shared.map",shared);
        }
        {
            auto animation_local=local,animation_shared=shared;
            for(auto* owner:{&animation_local,&animation_shared}) {
                word(*owner,1056,0x6a6d6164);word(*owner,1112,0x6a6d6164);
            }
            word(animation_shared,1124,1024);
            std::fill(animation_shared.begin()+4096,animation_shared.begin()+5120,std::byte{});
            constexpr unsigned graph_offset=4096,address=0x100000,clip=graph_offset+0xd0,resource=graph_offset+0x130;
            word(animation_shared,graph_offset+0xc,1);word(animation_shared,graph_offset+0x10,address+0xb0);
            word(animation_shared,graph_offset+0x14,1);word(animation_shared,graph_offset+0x18,address+0x16c);
            word(animation_shared,graph_offset+0x2c,1);word(animation_shared,graph_offset+0x30,address+0xd0);
            word(animation_shared,graph_offset+0xb0,0x01000007);word(animation_shared,graph_offset+0xb8,0x0001ffff);
            word(animation_shared,clip,0x01000008);word(animation_shared,clip+0x10,0x01000000);
            word(animation_shared,clip+0x14,2);word(animation_shared,clip+0x18,4);
            word(animation_shared,clip+0x20,0xffffffffu);word(animation_shared,clip+0x24,0x0000ffff);
            word(animation_shared,clip+0x28,56);word(animation_shared,clip+0x2c,address+0x130);
            word(animation_shared,clip+0x30,12);word(animation_shared,clip+0x34,44u<<16);
            word(animation_shared,clip+0x40,1);word(animation_shared,clip+0x44,address+0x168);
            word(animation_shared,clip+0x48,1);word(animation_shared,clip+0x4c,address+0x178);
            word(animation_shared,resource,0x00010001);word(animation_shared,resource+12,32);
            word(animation_shared,resource+32,0x3fa00000);word(animation_shared,resource+36,0xc0000000);
            word(animation_shared,resource+40,0x3f000000);word(animation_shared,resource+48,1);
            word(animation_shared,graph_offset+0x168,0x00010002);
            word(animation_shared,graph_offset+0x16c,0x736e6421);word(animation_shared,graph_offset+0x170,0xffffffffu);
            word(animation_shared,graph_offset+0x174,0x35);word(animation_shared,graph_offset+0x178,0x00010000);
            word(animation_shared,graph_offset+0x17c,0x0b001234);
            save(root/"animation.map",animation_local);save(root/"shared.map",animation_shared);
            const Cache animation_cache(root/"animation.map");
            const auto graph=animation_cache.load_source_unit_animation_graph_metadata(0xe1000002u);
            check(graph.source_address()!=address && graph.graph()==0xe1000002u,"Canonical animation lost its handle or retained donor addresses");
            const auto group=Bytes(graph.header()).u32(0xb0);
            const auto io=animation_cache.load_source_runtime_animation_image(group);
            const auto file=Bytes(io.descriptor).u32(8);
            check(io.bytes.size()==56 && file%512==0,"Canonical animation I/O lost its owned sector resource");
            const auto sector=animation_cache.load_source_runtime_resource(file,512);
            check(sector.size()==512 && std::ranges::equal(io.bytes,std::span(sector).first(56)) &&
                  std::ranges::all_of(std::span(sector).subspan(56),[](std::byte b){return b==std::byte{};}),
                  "Canonical animation sector read escaped its codec bytes and zero padding");
            rejects([&]{(void)animation_cache.load_source_runtime_resource(file,513);});
            rejects([&]{(void)animation_cache.load_source_runtime_resource(file+1,512);});
            rejects([&]{(void)animation_cache.load_source_runtime_animation_image(group+4);});
            const auto decoded=animation_cache.load_animation(0xe1000002u,0);
            check(decoded.frame_count==2 && decoded.nodes.size()==1 && decoded.components[0]==2,
                  "Converted animation was not admitted by the existing Xbox codec consumer");
            for(const auto& pose:decoded.frames)check(pose.translation.x==1.25f && pose.translation.y==-2.f && pose.translation.z==.5f,
                  "Canonical resource group changed the decoded pose");
            const auto definition=animation_cache.load_source_unit_animation_definition(0xe1000002u,0xffffu);
            check(definition.loop_frame==-1 && definition.authored_flags==4 && definition.timing.size()==1 &&
                  definition.timing[0].type==2 && definition.timing[0].frame==1 && definition.sounds.size()==1 &&
                  definition.sounds[0].marker==0x0b001234 && definition.sound_references.size()==1 &&
                  definition.sound_references[0].flags==0x35,"Canonical animation event/reference ordering changed");
            auto bad=animation_shared;word(bad,clip+0x2c,address-1);save(root/"shared.map",bad);
            rejects([&]{(void)Cache(root/"animation.map");});
            save(root/"shared.map",shared);
        }
        {
            auto cube_source=shared;const auto p=4096+80;
            word(cube_source,p+8,1|(2u<<16));word(cube_source,p+20,1);word(cube_source,p+80,128);
            save(root/"shared.map",cube_source);
            Buffer cube_data(128);word(cube_data,0,1);word(cube_data,4,std::uint32_t(-120));
            for(unsigned face=0;face<6;++face) {
                for(unsigned pixel=0;pixel<4;++pixel) {
                    cube_data[8+(face*4+pixel)*4+2]=std::byte(face+1);
                    cube_data[8+(face*4+pixel)*4+3]=std::byte{255};
                }
                cube_data[104+face*4+2]=std::byte(face+10);cube_data[104+face*4+3]=std::byte{255};
            }
            save(root/"textures.dat",cube_data);
            const auto image=Cache(root/"fixture.map").load_bitmap(0xe1000002u,0);
            check(image.layers==6 && image.mips.size()==12 && image.pixels.size()==120,"MCC cube face/mip extents changed");
            constexpr unsigned mapping[]{0,2,1,3,4,5};
            for(unsigned face=0;face<6;++face) {
                check(image.pixels[face*20]==std::byte(mapping[face]+1),"MCC PC-to-Xbox cube face mapping changed");
                check(image.pixels[face*20+16]==std::byte(mapping[face]+10),"MCC cube lower mip lost its owner");
            }
        }
        {
            auto volume_source=shared;const auto p=4096+80;
            word(volume_source,p+8,2|(1u<<16));word(volume_source,p+20,1);word(volume_source,p+80,44);
            save(root/"shared.map",volume_source);
            Buffer volume(44);word(volume,0,1);word(volume,4,std::uint32_t(-36));
            for(unsigned pixel=0;pixel<9;++pixel){volume[8+pixel*4+2]=std::byte(pixel+1);volume[8+pixel*4+3]=std::byte{255};}
            save(root/"textures.dat",volume);
            const auto image=Cache(root/"fixture.map").load_bitmap(0xe1000002u,0);
            check(image.depth==2 && image.layers==1 && image.mips.size()==2 && image.mips[1].depth==1,"MCC volume/mip dimensions changed");
            for(unsigned pixel=0;pixel<9;++pixel)check(image.pixels[pixel*4]==std::byte(pixel+1),"MCC volume texels were reordered");
        }
        save(root/"shared.map",shared);save(root/"textures.dat",texture);
        auto bad=shared;text(bad,2112,"textures\\different");save(root/"shared.map",bad);
        rejects([&]{Cache rejected(root/"fixture.map");});
        save(root/"shared.map",shared);bad=texture;word(bad,4,std::uint32_t(-17));save(root/"textures.dat",bad);
        rejects([&]{(void)cache.load_bitmap_resource(0xe1000002u,0);});
        std::filesystem::remove_all(root);std::cout<<"MCC chunks, exact shared owners, bounds, spawns, bitmap pixels and residency passed\n";return 0;
    }catch(const std::exception& e){std::filesystem::remove_all(root);std::cerr<<e.what()<<'\n';return 1;}
}
