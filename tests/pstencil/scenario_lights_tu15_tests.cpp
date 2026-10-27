#include "halo2/pstencil/native_lights.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/path_tracing/source_lights.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>
#include <unistd.h>
using namespace halo2;

static void snapshot(const SourceRuntimeMemory& memory,const std::filesystem::path& path) {
    std::ofstream out(path,std::ios::binary);
    memory.visit_committed([&](std::uint32_t address,std::span<const std::byte> bytes) {
        const std::array header{address,std::uint32_t(bytes.size())};
        out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));
        out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    });
    if(!out)throw FormatError("Scenario light snapshot write failed");
}

int main(int argc,char**argv)try {
    if(argc!=4)throw FormatError("Supply TU15_XBE OPENING_MAP OUTPUT_DIRECTORY");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const std::filesystem::path output=argv[3];std::filesystem::create_directories(output);
    auto storage=(std::filesystem::temp_directory_path()/"halo2-scenario-lights-XXXXXX").string();
    if(!::mkdtemp(storage.data()))throw FormatError("Scenario light storage unavailable");
    struct Cleanup {std::string path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}} cleanup{storage};
    MultiplayerObjectRuntime objects(argv[1],cache,0,storage);
    auto& world=objects.tu15_world();world.initialize_map();
    world.configure_campaign_session(1,1,30,0x12345678u);
    const auto& memory=world.memory();
    const auto word=[&](auto at){return Bytes(memory.view(at,4)).u32(0);};
    const auto clusters=cache.load_object_light_spatial_bsp(0).clusters.size();
    const auto before=pstencil::native_lights_tu15(memory,clusters);
    if(!before.lights.identities().empty())throw FormatError("Cold scenario fixture already has lights");
    auto call=world.prepare_source_call();call.gp[0]=word(word(0x4f3218u)+8u);
    const auto abi_json=[](const SourcePhysicsWorldRuntimeAbi& value) {
        if(value.x87_depth || !value.floating_point || value.active_mmx)
            throw FormatError("Scenario fixture needs its actual empty initial/final FP stack");
        return nlohmann::json{{"gp",value.gp},{"xmm",value.xmm},{"eflags",value.eflags},
            {"fs_base",value.fs_base},{"x87_depth",value.x87_depth},{"x87_status",value.x87_status},
            {"x87_control",value.floating_point->x87_control_word},{"mxcsr",value.floating_point->mxcsr}};
    };
    std::ofstream(output/"before-abi.json")<<abi_json(call).dump(2)<<'\n';
    std::ofstream abi(output/"abi.tsv");
    for(auto value:call.gp)abi<<value<<'\t';abi<<call.eflags<<'\t'<<call.fs_base<<'\n';
    snapshot(memory,output/"before.bin");
    std::ofstream stores(output/"stores.tsv");unsigned writes{};
    world.run_actual(0xc29f0u,call,[&](const auto& store){
        ++writes;stores<<store.instruction<<'\t'<<store.address<<'\t';
        static constexpr char hex[]="0123456789abcdef";
        for(auto byte:store.written) {const auto v=std::to_integer<unsigned>(byte);stores<<hex[v>>4]<<hex[v&15u];}
        stores<<'\n';
    });
    std::ofstream(output/"after-abi.json")<<abi_json(call).dump(2)<<'\n';
    snapshot(memory,output/"after.bin");
    const auto after=pstencil::native_lights_tu15(memory,clusters);
    if(after.lights.identities().size()!=2)throw FormatError("Original scenario loop did not create both actual lights");
    std::set<std::uint32_t> definitions;
    for(const auto id:after.lights.identities()) {
        const auto row=after.lights.record(id);definitions.insert(row[1]);
        const auto shape=pstencil::native_light_shape_tu15(world,id);
        if(shape.type()!=2)throw FormatError("Actual scenario frustum became a sphere");
        const auto metadata=cache.load_source_tag_metadata(row[1]);
        const auto bytes=metadata.bytes_at(metadata.source_address(),228);
        EffectLightDefinition definition;std::copy(bytes.begin(),bytes.end(),definition.bytes.begin());
        SourceEffectLightViewRecord rt_record{};rt_record.definition=&definition;
        rt_record.runtime.identity=id;rt_record.runtime.flags=std::uint8_t(row[0]>>16);
        const auto scalar=[&](unsigned offset){return std::bit_cast<float>(row.at(offset/4));};
        rt_record.runtime.state.diffuse={scalar(0xd4),scalar(0xd8),scalar(0xdc)};
        rt_record.runtime.state.brightness=scalar(0xec);
        const auto rt=path_tracing::source_lights(world,std::span{&rt_record,1});
        if(rt.size()!=1)throw FormatError("Path Tracing dropped a native scenario projector");
        const auto cameras=pstencil::native_light_cameras_tu15(world,id);
        if(cameras.size()!=1 || rt.front().planes!=cameras.front().full_frustum.planes)
            throw FormatError("Path Tracing changed an actual native projector boundary");
        const auto value=[&](unsigned offset){return std::bit_cast<float>(shape.words.at(offset));};
        if(rt.front().position.x!=value(11) || rt.front().position.y!=value(12) || rt.front().position.z!=value(13))
            throw FormatError("Path Tracing changed the source projector's virtual origin");
        for(const auto corner:cameras.front().full_frustum.corners) {
            const auto x=corner.x-rt.front().position.x,y=corner.y-rt.front().position.y,z=corner.z-rt.front().position.z;
            if(std::sqrt((x*x+y*y)+z*z)>rt.front().radius)
                throw FormatError("Path Tracing radial bound clips a native projector corner");
        }
        std::cout<<"LIGHT "<<std::hex<<id<<" definition="<<row[1]<<std::dec
                 <<" placement="<<row[2]<<" flags="<<(row[0]>>16)<<'\n';
    }
    if(definitions!=std::set<std::uint32_t>{0xe9d4085fu,0xe9d60861u})
        throw FormatError("Actual scenario palettes changed Arbiter/Prophet ownership");
    std::cout<<"PASS two actual authored scenario lights; "<<writes<<" original stores\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
