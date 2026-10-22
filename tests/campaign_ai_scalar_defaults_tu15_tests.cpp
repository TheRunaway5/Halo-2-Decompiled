#include "halo2/source_campaign_ai_scalar_defaults_tu15.hpp"
#include "halo2/source_final_startup_storage_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using namespace halo2;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string& text) {
    check(text.size()%2==0,"Odd scalar reference bytes");std::vector<std::byte> result;
    for(unsigned i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto b:bytes){auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}
    return result;
}
std::array<std::byte,4> bytes(unsigned word) {
    return {std::byte(word),std::byte(word>>8),std::byte(word>>16),std::byte(word>>24)};
}
unsigned word(const SourceRuntimeMemory& memory,unsigned address) {
    const auto raw=memory.view(address,4);unsigned result{};
    for(unsigned i=0;i<4;++i)result|=std::to_integer<unsigned>(raw[i])<<(8*i);
    return result;
}
void run_case(const nlohmann::json& fixture,const nlohmann::json& input) {
    SourceRuntimeMemory memory;
    for(const auto& operand:input.at("operands"))memory.mount(operand.at(0),unhex(operand.at(1)),
        SourceRuntimeMemory::Access::ReadOnly,"Declared raw MOVSS operand control");
    for(const auto& output:input.at("outputs"))memory.mount(output.at(0),
        std::vector<std::byte>(4,std::byte(input.at("poison").get<unsigned>())),
        SourceRuntimeMemory::Access::ReadWrite,"Original scalar output");
    memory.mount(fixture.at("stack"),bytes(fixture.at("return_address")),
        SourceRuntimeMemory::Access::ReadWrite,"Real scalar caller return word");
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=input.at("gp").get<decltype(abi.gp)>();
    abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=input.at("flags");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
    SourcePhysicsWorldRuntimeServices services{};
    services.instruction=[&](unsigned pc){trace.push_back(pc);};
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Scalar callback delegated an owner");};
    source_run_campaign_ai_scalar_defaults_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,[&](const auto& store){
        stores.push_back(nlohmann::json::array({store.instruction,store.address,hex(store.before),hex(store.written)}));});
    check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Scalar complete GPR state differs");
    check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Scalar complete SIMD state differs");
    check(abi.eflags==input.at("final_flags"),"Scalar changed flags");
    check(abi.floating_point&&abi.floating_point->x87_control_word==0x27f&&abi.floating_point->mxcsr==0x1f80,
        "Scalar changed floating point controls");
    check(trace==input.at("trace").get<std::vector<unsigned>>(),"Scalar original instruction path differs");
    check(stores==input.at("stores"),"Scalar ordered stores or previous values differ");
    for(const auto& output:input.at("outputs"))check(hex(memory.view(output.at(0),4))==output.at(1).get<std::string>(),
        "Scalar original output differs");
}
void mounted_producers(const nlohmann::json& fixture) {
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    check(hex(memory.view(fixture.at("callback_table"),12))==fixture.at("callback_table_hex").get<std::string>(),"Scalar _cinit order differs");
    memory.mount(fixture.at("stack"),bytes(fixture.at("return_address")),SourceRuntimeMemory::Access::ReadWrite,"Scalar startup caller");
    for(const auto& function:fixture.at("functions"))for(const auto& copy:function.at("copies")) {
        check(word(memory,copy.at("destination"))==0,"Scalar was seeded instead of loader-zeroed");
        check(memory.is_read_only(copy.at("source"),4),"Authored scalar operand became writable");
    }
    SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    for(unsigned slot=0x46dd00;slot<0x46dd0c;slot+=4) {
        abi.gp[4]=fixture.at("stack");
        source_run_campaign_ai_scalar_defaults_tu15(memory,word(memory,slot),abi,*abi.floating_point,{});
    }
    const auto saved=memory.checkpoint();
    for(const auto& function:fixture.at("functions"))for(const auto& copy:function.at("copies")) {
        check(hex(memory.view(copy.at("destination"),4))==copy.at("default_hex").get<std::string>(),"Original _cinit failed to produce scalar");
        memory.write(copy.at("destination"),bytes(0xdeadbeef));
    }
    memory.restore(saved);
    for(const auto& function:fixture.at("functions"))for(const auto& copy:function.at("copies"))
        check(hex(memory.view(copy.at("destination"),4))==copy.at("default_hex").get<std::string>(),"Checkpoint lost produced scalar");
    for(unsigned address:{0x56e07fu,0x56e090u}) {
        bool hole=false;try{(void)memory.view(address,1);}catch(const std::out_of_range&){hole=true;}
        check(hole,"Scalar mapping borrowed adjacent unowned bytes");
    }
}
void missing_inputs(const nlohmann::json& fixture) {
    // Missing source, destination and caller stack stop at the exact original
    // instruction, retaining the publications and register effects before it.
    for(unsigned missing=0;missing<3;++missing) {
        SourceRuntimeMemory memory;const auto raw=bytes(0x7fa12345);
        if(missing!=0)memory.mount(0x46a544u,raw,SourceRuntimeMemory::Access::ReadOnly,"Scalar control");
        if(missing!=1)memory.mount(0x56e08cu,bytes(0xa5a5a5a5),SourceRuntimeMemory::Access::ReadWrite,"Scalar destination");
        if(missing!=2)memory.mount(fixture.at("stack"),bytes(fixture.at("return_address")),
            SourceRuntimeMemory::Access::ReadWrite,"Scalar return");
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=fixture.at("stack");abi.xmm[0]={1,2,3,4};abi.eflags=0xed7;
        std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services{};
        services.instruction=[&](unsigned pc){trace.push_back(pc);};
        bool rejected=false;try{source_run_campaign_ai_scalar_defaults_tu15(memory,0x3826f0,abi,{0x27f,0x1f80},services);}
        catch(const std::out_of_range&){rejected=true;}
        check(rejected&&trace.size()==missing+1&&abi.gp[4]==fixture.at("stack")&&abi.eflags==0xed7,
            "Missing scalar input did not preserve original failure boundary");
        check(abi.xmm[0]==(missing==0?std::array<unsigned,4>{1,2,3,4}:std::array<unsigned,4>{0x7fa12345,0,0,0}),
            "Partial scalar MOVSS state differs");
        if(missing==2)check(word(memory,0x56e08c)==0x7fa12345,"Missing RET erased preceding scalar store");
    }
    check(!source_campaign_ai_scalar_defaults_tu15_owns_entry(0x3826d0),"Scalar interior exported as a whole callback");
}
}
int main(int argc,char** argv)try {
    check(argc==2,"Expected independent original scalar callback reference");
    nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        fixture.at("instruction_patches").empty()&&fixture.at("supplied_dependency_returns").empty(),"Scalar original authority differs");
    for(const auto& input:fixture.at("cases"))run_case(fixture,input);
    mounted_producers(fixture);missing_inputs(fixture);
    std::cout<<"PASS "<<fixture.at("cases").size()<<" complete original scalar callback cases, exact GPR/SIMD/flags, "
        "instruction/store paths, authored _cinit producers, checkpoint restore and missing input boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
