#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <optional>
#include <nlohmann/json.hpp>
#include <ranges>
#include <stdexcept>

namespace {
using namespace halo2;
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
void put(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte((value>>(i*8))&0xffu);
    memory.write(address,bytes);
}
}

int main(int argc,char** argv)try {
    check(argc<=3,"Expected optional original TU1.5 XBE and unchanged-original fixture paths");
    for(int i=1;i<argc;++i) {
        if(!*argv[i] || !std::filesystem::exists(argv[i])) {
            std::cerr<<"SKIP missing required local original input: "<<argv[i]<<'\n';return 77;
        }
    }
    std::optional<nlohmann::json> oracle;
    if(argc==3) {
        std::ifstream stream(argv[2]);check(bool(stream),"Cannot read present original fixture");
        oracle=nlohmann::json::parse(stream);
        check(oracle->at("format")=="native_lens_query_scalar_tu15_original_v1" &&
            oracle->at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
            oracle->at("entry")==0x3803b0u && oracle->at("original_instruction_hex")==
                "f30f1005c4a04600f30f110598205300c3" &&
            oracle->at("instruction_patches")==nlohmann::json::array() &&
            oracle->at("replaced_services")==nlohmann::json::array(),
            "Present scalar fixture has the wrong original owner or execution contract");
    }
    auto memory=native_tu15_world_memory({0x60212000u,{},"lens normalization scalar retained test clock"});
    const auto cold=memory.checkpoint();
    const auto scalar=std::ranges::find(cold.regions,0x532098u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(scalar!=cold.regions.end() && scalar->bytes.size()==4 &&
        scalar->access==SourceRuntimeMemory::Access::ReadWrite &&
        scalar->owner=="native TU1.5 loader-zeroed 3803B0 lens normalization scalar" &&
        !scalar->page_bytes && scalar->committed.empty(),
        "Native lens normalization scalar is not the complete loader-zeroed 4-byte owner");
    check(word(memory,0x532098u)==0,"Cold scalar fabricated the source initializer result");
    check(memory.native_address(0x532098u,4).value>UINT32_MAX && !memory.is_read_only(0x532098u,4),
        "Lens normalization scalar lacks writable native backing");
    const auto query=std::ranges::find(cold.regions,0x52dfd8u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(query!=cold.regions.end() && query->bytes.size()==0x40c0 &&
        query->address+query->bytes.size()==scalar->address &&
        query->owner=="source12560 query records and three mask banks",
        "Lens scalar broadened or changed the independently owned query bank");
    check(memory.is_read_only(0x46a0c4u,4) && word(memory,0x46a0c4u)==0x3b808081u,
        "Lens initializer authored operand is absent or changed");
    bool rejected=false;
    try{memory.validate_mutable_view(0x532098u,5);}
    catch(const std::out_of_range& error) {
        rejected=std::string(error.what())=="Original runtime view crosses its region owner";
    }
    check(rejected && memory.checkpoint()==cold,"Lens scalar accepted a borrowed span beyond its exact owner");
    if(argc>=2) {
        auto original=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15);
        check(std::ranges::equal(memory.view(0x532098u,4),original.view(0x532098u,4)) &&
            std::ranges::equal(memory.view(0x46a0c4u,4),original.view(0x46a0c4u,4)),
            "Lens scalar/operand bytes differ from authenticated original loader");
    }
    constexpr std::uint32_t stack=0x62000000u;
    memory.mount(stack,std::array<std::byte,0x100>{},SourceRuntimeMemory::Access::ReadWrite,
        "declared original3803B0 caller stack");
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x11u,0x22u,0x33u,0x44u,stack+0x80u,0x55u,0x66u,0x77u};
    abi.eflags=0x202u;
    for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)abi.xmm[i][j]=0x7fa12000u+i*16+j;
    put(memory,abi.gp[4],0x63000000u);const auto before=abi;
    if(oracle) {
        check(oracle->at("entry_gp")==before.gp && oracle->at("entry_xmm")==before.xmm &&
            oracle->at("entry_eflags")==before.eflags && oracle->at("entry_x87_control")==0x27fu &&
            oracle->at("entry_mxcsr")==0x1f80u && oracle->at("scalar_before")==
                std::array<unsigned,4>{0,0,0,0},
            "Original fixture does not use the declared native caller/loader inputs");
    }
    const auto query_before=std::vector<std::byte>(memory.view(0x52dfd8u,0x40c0).begin(),memory.view(0x52dfd8u,0x40c0).end());
    std::vector<std::uint32_t> trace;std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){trace.push_back(pc);};
    services.required_owner=[](auto&,const auto&,auto&,const auto&) {
        throw std::runtime_error("Complete3803B0 unexpectedly requested a supplied owner");
    };
    check(source_lens_query_tu15_owns_entry(0x3803b0u),"Complete original3803B0 is unavailable");
    source_run_lens_query_tu15(memory,0x3803b0u,abi,{0x27f,0x1f80},services,
        [&](const auto& store){stores.push_back(store);});
    constexpr std::array expected_trace{0x3803b0u,0x3803b8u,0x3803c0u};
    check(std::ranges::equal(trace,expected_trace),"Lens initializer did not execute its complete three original PCs");
    check(stores.size()==1 && stores[0].instruction==0x3803b8u && stores[0].address==0x532098u &&
        stores[0].before==std::vector<std::byte>(4) && stores[0].written.size()==4 &&
        std::ranges::equal(stores[0].written,memory.view(0x46a0c4u,4)) && word(memory,0x532098u)==0x3b808081u,
        "Lens scalar was not written by exact original initializer store");
    check(std::ranges::equal(memory.view(0x52dfd8u,0x40c0),query_before),
        "Lens scalar initializer mutated the adjacent query bank");
    auto expected_gp=before.gp;expected_gp[4]+=4;
    auto expected_xmm=before.xmm;expected_xmm[0]={0x3b808081u,0u,0u,0u};
    check(abi.gp==expected_gp && abi.xmm==expected_xmm && abi.eflags==before.eflags,
        "Lens initializer changed original GP/SSE/flag/return ABI");
    if(oracle) {
        check(oracle->at("instructions")==trace && oracle->at("exit_gp")==abi.gp &&
            oracle->at("exit_xmm")==abi.xmm && oracle->at("exit_eflags")==abi.eflags &&
            oracle->at("exit_x87_control")==0x27fu && oracle->at("exit_mxcsr")==0x1f80u &&
            oracle->at("exit_pc")==0x63000000u && oracle->at("query_bank_unchanged")==true &&
            oracle->at("query_bank_address")==0x52dfd8u && oracle->at("query_bank_bytes")==0x40c0u &&
            oracle->at("scalar_after")==std::array<unsigned,4>{0x81,0x80,0x80,0x3b},
            "Native scalar owner differs from unchanged original execution/ABI");
        nlohmann::json native_stores=nlohmann::json::array();
        for(const auto& store:stores) {
            std::vector<unsigned> prior,written;
            for(auto value:store.before)prior.push_back(std::to_integer<unsigned>(value));
            for(auto value:store.written)written.push_back(std::to_integer<unsigned>(value));
            native_stores.push_back({{"instruction",store.instruction},{"address",store.address},
                {"before",prior},{"written",written}});
        }
        check(oracle->at("stores")==native_stores,"Native scalar stores differ from unchanged original execution");
    }
    memory.release(stack);memory.restore(cold);
    check(memory.checkpoint()==cold && word(memory,0x532098u)==0,
        "Lens scalar checkpoint retained a completed initializer result in the cold bank");
    std::cout<<"PASS exact4-byte cold lens scalar, separate query bound, authored operand, native backing; "
        <<"complete original3803B0 three-PC/one-store GP/SSE/flag/return and checkpoint controls\n"
        <<"LIMIT data/source-store admission only; constructor scheduling, native query results and GPU remain open\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
