#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/mcc_cpu_bitmap_projection.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_bitmap_pending_header.hpp"
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

using namespace halo2;
namespace {
#include "fixtures/mcc_vehicle_cube.original.inc"
void check(bool ok,const char* message){if(!ok)throw FormatError(message);}
void put(SourceRuntimeMemory& memory,unsigned at,unsigned value){
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8));
    memory.write(at,bytes);
}
}
int main(int argc,char** argv)try{
    if(argc!=2||!std::filesystem::is_regular_file(argv[1]))return 77;
    const std::filesystem::path path=argv[1];
    Cache cache(path,path.parent_path(),OriginalData::native_tu15());
    check(cache.is_mcc(),"Dimensions regression requires the actual imported MCC cache");
    auto memory=native_tu15_world_memory({0x60212000u,{},"MCC dimensions regression clock"});
    const auto canonical=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u);
    const auto image=canonical.tag_image();
    for(const auto& region:image.regions)
        memory.mount(region.address,region.bytes,SourceRuntimeMemory::Access::ReadOnly,"actual MCC dimension metadata");
    put(memory,0x4f0414u,image.table);
    memory.mount(0x70000000u,std::array<std::byte,4096>{},SourceRuntimeMemory::Access::ReadWrite,"dimensions caller stack");
    private_cpu_bitmap::WorldProjection projection(cache,memory,canonical);
    unsigned completed{};
    for(const auto& [name,frame]:std::array{
        std::pair{"ui\\global_bitmaps\\emblems\\foreground",32u},
        std::pair{"objects\\vehicles\\warthog\\bitmaps\\antenna",0u}}){
        const auto* tag=cache.query_tag({.group=tag_groups::bitmap,.name=name,.require_address=true});
        check(tag,"Dimensions regression lost its actual authored bitmap");
        const Bytes source(cache.read_tag_bytes(tag->address,tag->size));
        const auto count=source.u32(0x44),source_first=source.u32(0x48);
        check(frame<count,"Declared dimension frame is outside its actual bitmap");
        const auto row=cache.read_tag_bytes(source_first+frame*168u,168);
        const auto expected=canonical.runtime_address(source_first+frame*168u,168);
        const auto root=canonical.runtime_address(tag->address,tag->size);
        const auto before=memory.view(root,tag->size);
        const std::vector<std::byte> snapshot(before.begin(),before.end());
        if(frame==32)check(count==64&&Bytes(row).u16(12)==11,"Original failing emblem input differs");
        else check(Bytes(row).u16(12)==15,"Metadata-only DXT3 regression input differs");
        const auto lookup=[&](unsigned index){
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp[0]=tag->datum;abi.gp[1]=index;abi.gp[4]=0x70000800u;abi.gp[6]=0x12345678u;abi.eflags=0x202;
            put(memory,abi.gp[4],0x65000000u);
            projection.lookup_dimensions(abi,{0x27f,0x1f80},{},{});
            check(abi.gp[4]==0x70000804u&&abi.gp[6]==0x12345678u,"Original lookup return or preserved ESI differs");
            check(!projection.has_resources(),"Dimension lookup published unrequested pixel resources");
            check(std::ranges::equal(memory.view(root,tag->size),snapshot),"Dimension lookup modified canonical metadata");
            ++completed;return abi.gp[0];
        };
        check(lookup(frame)==expected,"Original116-byte lookup did not return the actual selected MCC168-byte row");
        check(Bytes(memory.view(expected,24)).u16(4)==Bytes(row).u16(4),"Original width operand differs from the actual authored frame");
        check(lookup(0xffffu)==0&&lookup(count)==0,"Original signed/range rejection returned metadata");
    }
    const auto* emblem=cache.query_tag({.group=tag_groups::bitmap,
        .name="ui\\global_bitmaps\\emblems\\foreground",.require_address=true});
    check(emblem,"Actual emblem resource missing");
    const auto root=canonical.runtime_address(emblem->address,emblem->size);
    const Bytes source(cache.read_tag_bytes(emblem->address,emblem->size));
    const auto raw=cache.read_tag_bytes(source.u32(0x48)+32u*168u,168);
    const auto descriptor=Bytes(memory.view(root+0x48u,4)).u32(0);
    const auto selected=projection.row_for_canonical_frame(emblem->handle(),root,32);
    check(projection.has_resources()&&std::ranges::equal(memory.view(selected,24),raw.first(24)),
        "Inline frame32 did not retain its real encoded-image metadata");
    const auto header=Bytes(memory.view(selected+0x50u,4)).u32(0);
    check(header&&selected!=descriptor+32u*116u,"Inline frame32 borrowed the raw MCC stride result");
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=selected;abi.gp[4]=0x70000800u;abi.gp[6]=0x12345678u;abi.eflags=0x202;
    put(memory,abi.gp[4],0x1714b1u);
    source_run_physics_live_support_tu15(memory,0x3bca0u,abi,{0x27f,0x1f80},{},{});
    check(abi.gp[0]==header&&abi.gp[4]==0x70000804u&&abi.gp[6]==0x12345678u,
        "Complete original inline resource caller did not return the retained actual frame32 header");
    check(Bytes(memory.view(root+0x48u,4)).u32(0)==descriptor,"Inline resource changed canonical MCC descriptor");
    const auto* cube=cache.query_tag({.group=tag_groups::bitmap,
        .name="objects\\bitmaps\\reflection_maps\\masterchief_armor",.require_address=true});
    check(cube,"Actual armor reflection cube missing");
    const Bytes cube_source(cache.read_tag_bytes(cube->address,cube->size));
    const auto cube_raw=cache.read_tag_bytes(cube_source.u32(0x48),168);const Bytes cube_row(cube_raw);
    check(cube_source.u32(0x44)==1&&cube_row.u16(10)==2&&cube_row.u16(12)==14,
        "Declared actual six-face cube input differs");
    const auto cube_root=canonical.runtime_address(cube->address,cube->size);
    const auto cube_descriptor=Bytes(memory.view(cube_root+0x48u,4)).u32(0);
    const auto cube_selected=projection.row_for_canonical_frame(cube->handle(),cube_root,0);
    const auto cube_header=Bytes(memory.view(cube_selected+0x50u,4)).u32(0);
    const auto encoded=Bytes(memory.view(cube_selected+0x54u,4)).u32(0);
    const auto encoded_bytes=Bytes(memory.view(cube_selected+0x34u,4)).u32(0);
    const auto cube_texture=cache.load_bitmap(cube->datum,0);
    check(cube_texture.layers==6&&cube_texture.mips.size()==30&&encoded_bytes==cube_texture.pixels.size()&&
        std::ranges::equal(memory.view(encoded,encoded_bytes),cube_texture.pixels),
        "Retained cube lost an actual converted face/mip");
    std::array<unsigned char,32> digest{};unsigned digest_bytes{};
    const auto payload=memory.view(encoded,encoded_bytes);
    check(EVP_Digest(payload.data(),payload.size(),digest.data(),&digest_bytes,EVP_sha256(),nullptr)==1&&digest_bytes==32,
        "Cube proof digest unavailable");
    std::string hex;for(const auto value:digest){hex+="0123456789abcdef"[value>>4];hex+="0123456789abcdef"[value&15];}
    check(hex==actual_mcc_cube_native_sha256,"Native cube face/mip bytes differ from independent raw MCC conversion");
    const SourceBitmapPendingHeaderMetadata cube_metadata{64,64,14,4,1,2,515};
    const auto cube_expected=source_bitmap_pending_header(cube_metadata,0,0x81200000u);
    check(std::equal(cube_expected.begin(),cube_expected.end(),original_cube_header),
        "Cube header differs from complete original TU1.5 1301E0");
    const auto retained_expected=source_bitmap_pending_header(cube_metadata,0,encoded);
    const Bytes actual_header(memory.view(cube_header,20));for(unsigned n=0;n<5;++n)
        check(actual_header.u32(n*4)==retained_expected[n],"Retained cube header lost its actual physical binding");
    abi={};abi.gp[0]=cube_selected;abi.gp[4]=0x70000800u;abi.gp[6]=0x12345678u;abi.eflags=0x202;
    put(memory,abi.gp[4],0x1714b1u);
    source_run_physics_live_support_tu15(memory,0x3bca0u,abi,{0x27f,0x1f80},{},{});
    check(abi.gp[0]==cube_header&&abi.gp[4]==0x70000804u&&abi.gp[6]==0x12345678u,
        "Complete original cube resource lookup differs");
    bool cpu_rejected=false;try{(void)projection.first_row_for_canonical_root(cube->handle(),cube_root);}
    catch(const FormatError&){cpu_rejected=true;}
    check(cpu_rejected&&Bytes(memory.view(cube_root+0x48u,4)).u32(0)==cube_descriptor,
        "Cube resource escaped into the closed 2D CPU sampler or changed canonical metadata");
    std::cout<<"PASS actual MCC bitmap dimensions: "<<completed<<" original lookups; DXT3 metadata and complete original emblem32/cube resource access; cube excluded from 2D samples\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
