#include "halo2/cache.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <sstream>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *why){if(!value)throw FormatError(why);}
void put(SourceRuntimeMemory &m,unsigned at,unsigned value){
    const std::array<std::byte,4> bytes{std::byte(value),std::byte(value>>8),std::byte(value>>16),std::byte(value>>24)};
    m.write(at,bytes);
}
void input_pointer(SourceRuntimeMemory &m,unsigned at,unsigned value){
    try{(void)m.view(at,4);}catch(const std::out_of_range &){
        m.mount(at,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared callback global-pointer input");
    }
    put(m,at,value);
}
std::string digest(std::span<const unsigned char> bytes){
    unsigned char hash[SHA256_DIGEST_LENGTH];SHA256(bytes.data(),bytes.size(),hash);
    std::ostringstream text;text<<std::hex<<std::setfill('0');for(auto b:hash)text<<std::setw(2)<<unsigned(b);return text.str();
}
Json domain(const Json &fixture,const Json &folded,const Cache &cache,bool mcc){
    auto m=native_tu15_world_memory({0x60212000u,{},"isolated function namespace clock input"});
    constexpr unsigned weapon=0x1000000u,definition=0x1006000u,pool=0x100a000u,
        headers=0x100c000u,tags=0x100e000u,clock=0x1010000u,out=0x1012000u,
        stack=0x2010000u,stop=0x3000000u,object=0x12340000u;
    m.mount(weapon,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared original callback object/tag inputs");
    m.mount(stack-0x10000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared original callback stack");
    input_pointer(m,0x4ecbd0u,pool);input_pointer(m,0x4f0414u,tags);input_pointer(m,0x51df54u,clock);
    put(m,pool+0x44,headers);put(m,headers+8,weapon);put(m,weapon,7);put(m,tags+7*16+8,definition);
    std::unique_ptr<SourceEffectCompiledSymbolsTu15> binding;
    if(mcc)binding=std::make_unique<SourceEffectCompiledSymbolsTu15>(m,true,[&](auto id){return cache.string_id(id);});
    std::vector<unsigned char> bytes;unsigned cases{};
    for(const auto &count:fixture["barrel_counts"])
    for(const auto &pair:fixture["tick_pairs"])
    for(const auto &primary:fixture["raw_values"])
    for(const auto &secondary:fixture["raw_values"])
    for(const auto &selector:fixture["selectors"]){
        const auto original=selector.get<unsigned>();
        const auto authored=mcc?(original==0x0e000572u?0x0e00057eu:0x1000057fu):original;
        put(m,definition+0x2d0,count.get<unsigned>());put(m,clock+8,pair[0].get<unsigned>());
        put(m,weapon+0x248,pair[1].get<unsigned>());put(m,weapon+0x1b4,primary.get<unsigned>());
        put(m,weapon+0x1e8,secondary.get<unsigned>());
        put(m,out,0xdeadbeefu);put(m,out+4,0xdeadbeefu);
        for(const auto &[offset,value]:std::array<std::pair<unsigned,unsigned>,5>{{{0,stop},{4,object},{8,authored},{12,out},{16,out+4}}})put(m,stack+offset,value);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=2;
        // Complete original callback, not the separate typed material helper.
        source_run_physics_live_support_tu15(m,0x1046c0u,abi,{0x27f,0x1f80},{});
        check(abi.gp[4]==stack+20 && (abi.gp[0]&255u)==1,"Whole firing callback did not preserve its actual return ABI");
        const auto result=Bytes(m.view(out,4)).u32(0),active=std::to_integer<unsigned>(m.view(out+4,1)[0]);
        for(const auto word:{count.get<unsigned>(),pair[0].get<unsigned>(),pair[1].get<unsigned>(),
            primary.get<unsigned>(),secondary.get<unsigned>(),original,result})
            for(unsigned i=0;i<4;++i)bytes.push_back(word>>(i*8));
        bytes.push_back(active);++cases;
    }
    const auto hash=digest(bytes);
    Json folded_rows=Json::array();unsigned folded_failures{};
    if(!folded.is_null()) {
        constexpr unsigned debug=0x1014000u;input_pointer(m,0x4eabc4u,debug);
        for(const auto &row:folded["cases"]) {
            const auto selector=row[mcc?"mcc_selector":"selector"].get<unsigned>();
            if(mcc)check(cache.string_id(selector)==row["name"].get<std::string>(),"Folded input lost its actual named MCC identity");
            const bool device=row.value("device",false);
            put(m,clock+4,0x3d088889u);
            m.write(weapon+0xaa,std::array{std::byte{0}});put(m,weapon+0x12c,0);
            for(const auto &[offset,value]:std::array<std::pair<unsigned,unsigned>,5>{{
                {0x134,0x3e800000u},{0x138,0x3e000000u},{0x140,0x3f000000u},{0x144,0x3e000000u},{0x148,1}}})put(m,weapon+offset,value);
            put(m,definition+0xc0,0x3f000000u);put(m,definition+0xc8,0x3e800000u);put(m,definition+0x10c,0x3f800000u);
            put(m,weapon+0x25c,0x3e000000u);put(m,weapon+0x260,0x3e800000u);
            for(const auto &[offset,value]:std::array<std::pair<unsigned,unsigned>,4>{{
                {0x70,0x3e000000u},{0x74,0x3e800000u},{0x78,0x3f000000u},{0x7c,0x3f400000u}}})put(m,debug+offset,value);
            put(m,(row["unit"].get<bool>()||device?weapon:debug)+row["offset"].get<unsigned>(),row["input_bits"].get<unsigned>());
            put(m,out,0xdeadbeefu);put(m,out+4,0xdeadbeefu);
            for(const auto &[offset,value]:std::array<std::pair<unsigned,unsigned>,5>{{{0,stop},{4,object},{8,selector},{12,out},{16,out+4}}})put(m,stack+offset,value);
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=2;
            if(device)source_run_map_world_support_tu15(m,row["entry"].get<unsigned>(),abi,{0x27f,0x1f80},{});
            else source_run_physics_live_support_tu15(m,row["entry"].get<unsigned>(),abi,{0x27f,0x1f80},{});
            const auto value=Bytes(m.view(out,4)).u32(0),active=std::to_integer<unsigned>(m.view(out+4,1)[0]);
            const bool passed=abi.gp[4]==stack+20 && (abi.gp[0]&255u)==row["recognized"].get<unsigned>() &&
                value==row["value_bits"].get<unsigned>()&&active==row["active"].get<unsigned>();
            folded_failures+=!passed;
            folded_rows.push_back({{"name",row["name"]},{"input_bits",row["input_bits"]},
                {"value_bits",value},{"active",active},{"recognized",abi.gp[0]&255u},{"passed",passed}});
        }
    }
    return {{"MCC_namespace",mcc},{"cases",cases},{"results_sha256",hash},
        {"folded_cases",folded_rows},{"folded_original_mismatches",folded_failures},
        {"matches_unchanged_Xbox_execution",hash==fixture["results_sha256"].get<std::string>()}};
}
}
int main(int argc,char **argv){
    if(argc>=3 && !*argv[2])return 77;
    Json report{{"passed",false},{"scope","Complete1046C0 with declared ABI inputs and independently executed original firing fixture; no gameplay or rendered result supplied"}};
    try{
        check(argc==4||argc==5,"Supply original fixture, prepared MCC map, report and optional folded fixture");
        std::ifstream file(argv[1]);const auto fixture=Json::parse(file);
        check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
            fixture["weapon_callback"]==0x1046c0u&&fixture["cases"]==13520,"Original callback fixture authority differs");
        Cache cache(argv[2]);check(cache.is_mcc(),"Namespace regression requires actual MCC metadata");
        check(cache.string_id(0x0e00057eu)=="primary_firing"&&cache.string_id(0x1000057fu)=="secondary_firing","MCC named firing inputs differ");
        Json folded;
        if(argc==5){std::ifstream file(argv[4]);folded=Json::parse(file);check(folded["original_xbe_sha256"]==fixture["original_xbe_sha256"],"Folded callback reference differs");}
        report["original"]=domain(fixture,folded,cache,false);report["MCC"]=domain(fixture,folded,cache,true);
        check(report["original"]["matches_unchanged_Xbox_execution"].get<bool>(),"Unbound original callback changed");
        check(report["MCC"]["matches_unchanged_Xbox_execution"].get<bool>(),"MCC compiled callback selected different firing results");
        check(report["original"]["folded_original_mismatches"]==0&&report["MCC"]["folded_original_mismatches"]==0,"Folded callback differs from unchanged original execution");
        report["passed"]=true;
    }catch(const std::exception &error){report["error"]=error.what();}
    if(argc>=4){std::ofstream file(argv[3]);file<<report.dump(2)<<'\n';}
    std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
}
