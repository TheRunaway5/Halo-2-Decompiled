#include "halo2/source_final_startup_storage_tu15.hpp"
#include "halo2/source_ai_actor_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::array<std::byte,4> encode(unsigned value) {
    std::array<std::byte,4> out{};
    for(unsigned i=0;i<4;++i)out[i]=std::byte(value>>(i*8));
    return out;
}
unsigned word(SourceRuntimeMemory &memory,unsigned address) {
    std::array<std::byte,4> data{};memory.read(address,data);unsigned out{};
    for(unsigned i=0;i<4;++i)out|=std::to_integer<unsigned>(data[i])<<(i*8);
    return out;
}
unsigned hex_word(const std::string &text) {
    check(text.size()==8,"Original word has an invalid extent");unsigned value{};
    for(unsigned i=0;i<4;++i)value|=std::stoul(text.substr(i*2,2),nullptr,16)<<(i*8);
    return value;
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply original descriptor fixture and small report path");
    Json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "AI descriptor reference is not Xbox TU1.5");
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    std::set<unsigned> mutable_bytes;unsigned cache_bytes{},immutable_bytes{};
    for(const auto &row:fixture["mutable_descriptor_caches"]) {
        const unsigned address=row["address"];
        check(word(memory,address)==0xfffffffeu && word(memory,address+4)==0,
            "AI descriptor cache did not retain its original initial inputs");
        check(!memory.is_read_only(address,8),"AI descriptor source-owned cache became immutable");
        memory.write(address,encode(0x12345678u));memory.write(address+4,encode(0x87654321u));
        check(word(memory,address)==0x12345678u && word(memory,address+4)==0x87654321u,
            "AI descriptor cache cannot accept its original producers");
        memory.write(address,encode(0xfffffffeu));memory.write(address+4,encode(0));
        for(unsigned i=0;i<8;++i)mutable_bytes.insert(address+i);
        cache_bytes+=8;
    }
    for(const auto &row:fixture["mask_constructors"]) {
        const unsigned destination=row["destination"],source=row["source"];
        const auto value=hex_word(row["operand_hex"]);
        bool present=true;
        try{(void)memory.view(source,4);}catch(const std::out_of_range&){present=false;}
        if(present)check(word(memory,source)==value && memory.is_read_only(source,4),
            "Original mask operand conflicts with another owner");
        else memory.mount(source,encode(value),SourceRuntimeMemory::Access::ReadOnly,
            "declared original AI mask operand");
        memory.mount(destination,encode(0),SourceRuntimeMemory::Access::ReadWrite,
            "cold original CRT mask destination");
        for(unsigned i=0;i<4;++i)mutable_bytes.insert(destination+i);
    }
    for(const auto &row:fixture["records"]) {
        const unsigned address=row["address"];const auto hex=row["original_hex"].get<std::string>();
        for(unsigned i=0;i<hex.size()/2;++i)if(!mutable_bytes.contains(address+i)) {
            std::array<std::byte,1> value{};memory.read(address+i,value);
            check(std::to_integer<unsigned>(value[0])==std::stoul(hex.substr(i*2,2),nullptr,16) &&
                memory.is_read_only(address+i,1),"AI descriptor literal byte/access differs from original");
            ++immutable_bytes;
        }
    }
    memory.mount(0x71000000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,
        "declared original constructor caller stack");
    unsigned constructors{},instructions{};
    for(const auto &row:fixture["mask_constructors"]) {
        const unsigned destination=row["destination"],source=row["source"];
        check(word(memory,destination)==0,"AI constructor result was prefilled");
        SourcePhysicsWorldRuntimeAbi abi{};
        for(unsigned i=0;i<8;++i)abi.gp[i]=0x11110000u+i*0x100u;
        abi.gp[4]=0x71000800u;abi.eflags=0x202;abi.floating_point={0x27f,0x1f80};
        memory.write(abi.gp[4],encode(0x60300000u));auto expected=abi.gp;
        expected[0]=word(memory,source);expected[4]+=4;
        unsigned count{};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto){++count;};
        source_run_ai_actor_tu15(memory,row["entry"],abi,{0x27f,0x1f80},services);
        check(abi.gp==expected && (abi.eflags&0xcc5)==(0x202&0xcc5) &&
            word(memory,destination)==expected[0] && count==3,
            "Complete original AI mask constructor ABI/store differs");
        ++constructors;instructions+=count;
    }
    bool rejected=false;
    try{memory.write(0x48ad78u,encode(1));}catch(const std::exception&){rejected=true;}
    check(rejected && word(memory,0x48ad78u)==0,"AI callback literal lost its original value/access");
    check(constructors==71 && cache_bytes==920 && immutable_bytes==4632,
        "Original AI descriptor or constructor scope is incomplete");
    Json report{{"passed",true},{"actual_native_source_constructors",constructors},
        {"original_instructions",instructions},{"immutable_descriptor_bytes_compared",immutable_bytes},
        {"source_owned_mutable_cache_bytes",cache_bytes},{"callback_48ad78_original_zero",true},
        {"mutable_masks_prefilled",false},{"mcc_campaign_runtime_verified",false}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
