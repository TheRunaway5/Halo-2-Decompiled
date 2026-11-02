#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void write(PhysicsLiveWorldTu15 &world,unsigned address,unsigned value,unsigned size=4) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<size;++n)bytes[n]=std::byte(value>>(8*n));
    world.write_source_call_input(address,std::span(bytes).first(size));
}
unsigned read(const SourceRuntimeMemory &memory,unsigned address) {
    std::array<std::byte,4> bytes;memory.read(address,bytes);return Bytes(bytes).u32(0);
}
}
int main(int argc,char **argv)try {
    if(argc!=4 || !std::filesystem::is_regular_file(argv[1]))return 77;
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
    require(scratch && *scratch,"World HS test needs its owned storage");
    const Cache cache(argv[1]);require(cache.is_mcc(),"World HS composition requires actual prepared MCC content");
    std::ifstream file(argv[2]);nlohmann::json reference;file>>reference;
    require(reference.at("schema")=="H2_INSTALLED_MCC_BLUR_CALLS_V1","Installed command reference differs");
    const auto saves=std::filesystem::path(scratch)/"world";std::filesystem::create_directories(saves);
    PhysicsLiveWorldTu15 world(cache,saves);
    world.initialize_map();world.configure_session(source_game_variant_default(0),1,30,0x1234);
    auto &memory=world.memory();
    std::ifstream scalar_file(argv[3]);nlohmann::json scalar;scalar_file>>scalar;
    require(scalar.at("schema")=="H2_TU15_HS_SCALAR_CASTS_V1" &&
        scalar.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        scalar.at("table_address")==0x502248 && scalar.at("table_bytes")==0x3c10,
        "World scalar-cast reference is not authenticated TU1.5 evidence");
    const auto expected=scalar.at("cases").at(0).at("table_after").get<std::string>();
    require(expected.size()==0x3c10*2,"World scalar-cast reference extent differs");
    const auto actual=memory.view(0x502248,0x3c10);
    for(unsigned n=0;n<actual.size();++n)require(std::to_integer<unsigned>(actual[n])==
        std::stoul(expected.substr(n*2,2),nullptr,16),"World startup scalar-cast table differs from the complete original");
    constexpr unsigned fixture=0xb0000000,extent=4u*1024u*1024u,scenario=fixture+0x1000,
        pool=fixture+0x2000,syntax=fixture+0x2100,thread=fixture+0x3000,frame=thread+0x40,
        output=fixture+0x7000,nodes=fixture+0x10000,strings=fixture+0x200000;
    // These are explicit HS caller inputs in a separate owned bank. The map,
    // its authored assets and the World's real parser/completion services are
    // unchanged. This is a World composition test, not mission acceptance.
    memory.mount(fixture,std::vector<std::byte>(extent),SourceRuntimeMemory::Access::ReadWrite,
        "isolated authored-call HS input fixture");
    std::vector<std::pair<unsigned,std::vector<std::byte>>> saved;
    for(const auto [address,size]:{std::pair(0x4ecc20u,4u),std::pair(0x505e5cu,4u),
        std::pair(0x505e60u,1u),std::pair(0x505e64u,4u),std::pair(0x505e6cu,4u)}) {
        std::vector<std::byte> bytes(size);memory.read(address,bytes);saved.emplace_back(address,std::move(bytes));
    }
    const auto restore=[&] {
        for(const auto &[address,bytes]:saved)world.write_source_call_input(address,bytes);
        memory.release(fixture);
    };
    unsigned cases=0;
    try {
        for(const auto &map:reference.at("maps"))for(const auto &call:map.at("calls")) {
            const auto count=map.at("node_count").get<unsigned>(),size=map.at("strings_bytes").get<unsigned>();
            require(count && count<=65536 && size && size<=1024u*1024u,"Authored fixture extent differs");
            std::vector<std::byte> raw(count*20),names(size);
            for(const auto &row:call.at("rows")) {
                const auto index=row.at("index").get<unsigned>();const auto hex=row.at("raw").get<std::string>();
                require(index<count && hex.size()==40,"Authored fixture row extent differs");
                for(unsigned n=0;n<20;++n)raw[index*20+n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
            }
            constexpr std::string_view name="rasterizer_blur";
            const auto offset=call.at("function_text_offset").get<unsigned>();
            require(offset<size && name.size()+1<=size-offset,"Authored fixture name escapes its bank");
            std::copy(name.begin(),name.end(),reinterpret_cast<char*>(names.data()+offset));
            world.write_source_call_input(nodes,raw);world.write_source_call_input(strings,names);
            write(world,0x4ecc20,scenario);write(world,0x505e5c,pool);write(world,0x505e60,1,1);
            write(world,0x505e6c,syntax);write(world,pool+0x44,thread);
            write(world,scenario+0x238,count);write(world,scenario+0x23c,nodes);
            write(world,scenario+0x1b0,size);write(world,scenario+0x1b4,strings);
            write(world,syntax+0x20,count);write(world,syntax+0x24,20);write(world,syntax+0x44,nodes);
            write(world,thread+2,1,1);write(world,thread+3,1,1);write(world,thread+8,0);
            write(world,thread+0x10,frame);write(world,frame,thread+0x18);
            write(world,frame+4,call.at("datum").get<unsigned>());write(world,thread+0x20,output);
            write(world,output,0xdeadbeef);write(world,thread+0xc,0x100,2);write(world,frame+0xc,0,2);
            const auto layout=project_mcc_script_layout(raw,names,{});
            const NativeMccScriptBuiltinProfile profile(layout,names,true);
            require(profile.calls()==1 && profile.owns(layout.requirements.at(0)),"Authored World command receipt not owned");
            {
                NativeMccScriptBuiltinBinding binding(memory,profile);
                auto abi=world.prepare_source_call();abi.gp[7]=0xa0010000;const auto before=abi;
                std::vector<unsigned> stores;
                world.run_actual(0x211b10,abi,[&](const auto &store){stores.push_back(store.instruction);});
                for(unsigned reg:{3u,5u,6u,7u})require(abi.gp[reg]==before.gp[reg],"World HS command changed a nonvolatile register");
                require(abi.gp[4]==before.gp[4]+4 && abi.xmm==before.xmm &&
                    abi.x87_depth==before.x87_depth && abi.x87_status==before.x87_status,
                    "World HS command changed its retained stack/FP ABI");
                require(std::ranges::find(stores,0x212096u)!=stores.end() &&
                    std::ranges::find(stores,0x212117u)!=stores.end() &&
                    std::ranges::find(stores,0x211e5eu)!=stores.end(),
                    "World did not use its actual argument parser and completion owners");
            }
            require(read(memory,output)==0 && read(memory,thread+0x10)==thread+0x18,
                "World command did not complete its actual HS frame");
            require(Bytes(memory.view(frame+14,2)).u16(0)==(call.at("argument").at(6).get<unsigned>()&65535u),
                "World parser changed the authored SHORT word");
            require(std::ranges::equal(memory.view(nodes,raw.size()),raw),"World HS execution changed authored syntax");
            ++cases;
        }
    }catch(...){restore();throw;}
    restore();require(cases==7,"World command fixture lost installed coverage");
    std::cout<<"PASS seven authored blur commands through real native World HS/parser/completion services; no supplied dependency results or mission claim\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
