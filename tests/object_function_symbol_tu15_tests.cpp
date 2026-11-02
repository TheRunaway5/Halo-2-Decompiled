#include "halo2/cache.hpp"
#include "halo2/death_cycle_runtime_tu15.hpp"
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
// Execute the actual1704C0 region-choice owner through both real request-key
// stores. The observation boundary stops before178E30's unrelated allocation;
// it supplies no callee return, chosen state or success. BCB70 then consumes
// the produced words through its complete native body and ordinary ABI.
Json model_loop_one_seam(const Cache &cache,bool mcc) {
    auto m=native_tu15_world_memory({0x60212000u,{},"isolated model-loop producer namespace input"});
    constexpr unsigned body=0x1000000u,model=0x1006000u,variants=0x1006200u,
        regions=0x1006400u,permutations=0x1006600u,states=0x1006800u,
        pool=0x100a000u,headers=0x100c000u,tags=0x100e000u,
        choices=0x1012000u,live=0x1012100u,out=0x1012200u,
        stack=0x2010000u,stop=0x3000000u,object=0x12340000u,model_tag=0x22340007u,
        effect=0x33440008u,marker=0x11223344u;
    m.mount(body,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,
        "declared singleton model-state and actual object-function operands");
    m.mount(stack-0x10000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,
        "declared original1704C0 caller and source scratch stack");
    input_pointer(m,0x4f0414u,tags);input_pointer(m,0x4ecbd0u,pool);
    put(m,tags+7u*16u+8u,model);put(m,pool+0x44u,headers);put(m,headers+8u,body);put(m,body,model_tag);
    put(m,model+0x70u,1u);put(m,model+0x54u,variants);put(m,variants+0x18u,regions);
    put(m,regions+8u,1u);put(m,regions+0xcu,permutations);
    put(m,permutations+8u,0x3f800000u);put(m,permutations+0xcu,1u);put(m,permutations+0x10u,states);
    put(m,states+4u,3u);put(m,states+0xcu,effect);put(m,states+0x10u,marker);
    m.write(states+6u,std::array{std::byte{2},std::byte{0}});
    m.write(live,std::array{std::byte{0xff},std::byte{0},std::byte{0},std::byte{0}});put(m,live+4u,0xffffffffu);
    const auto metadata_before=std::vector<std::byte>(m.view(model,0xa00).begin(),m.view(model,0xa00).end());
    std::unique_ptr<SourceEffectCompiledSymbolsTu15> binding;
    if(mcc)binding=std::make_unique<SourceEffectCompiledSymbolsTu15>(m,true,[&](auto id){return cache.string_id(id);});
    const unsigned expected=mcc?0x030005b2u:0x030005a6u;
    check(!mcc||cache.string_id(expected)=="one","Actual MCC producer ONE name differs");
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](auto &memory,const auto &call,auto &abi,const auto &observe) {
        if(source_death_cycle_runtime_tu15_owns_entry(call.target))
            source_run_death_cycle_runtime_tu15(memory,call.target,abi,{0x27f,0x1f80},services,observe);
        else if(source_physics_live_support_tu15_owns_entry(call.target))
            source_run_physics_live_support_tu15(memory,call.target,abi,{0x27f,0x1f80},services,observe);
        else throw FormatError("Model-loop seam reached an undeclared real callee");
    };
    unsigned instructions{},producer_visits{},primary_stores{},secondary_stores{},request{};
    services.instruction=[&](unsigned pc) {
        if(++instructions>10000u)throw FormatError("Model-loop seam exceeded its bounded original prefix");
        producer_visits+=pc==0x170676u;
    };
    struct RequestBoundary {};
    const SourcePhysicsContactRecordObserver observer=[&](const auto &store) {
        primary_stores+=store.instruction==0x170688u;secondary_stores+=store.instruction==0x17068cu;
        if(store.instruction==0x170690u){request=store.address;throw RequestBoundary{};}
    };
    constexpr std::array arguments{object,model_tag,0u,0u,2u,0u,0xffffffffu,0u,choices,live};
    put(m,stack,stop);for(unsigned i=0;i<arguments.size();++i)put(m,stack+4u+i*4u,arguments[i]);
    SourcePhysicsWorldRuntimeAbi a{};a.gp={0x11223344u,0x55667788u,0x99aabbccu,0x13572468u,
        stack,0x24681357u,0x12344321u,0x56788765u};a.eflags=2u;
    bool reached{};
    try{source_run_death_cycle_runtime_tu15(m,0x1704c0u,a,{0x27f,0x1f80},services,observer);}
    catch(const RequestBoundary&){reached=true;}
    check(reached&&producer_visits==1u&&primary_stores==1u&&secondary_stores==1u,
        "Actual model-loop producer did not execute both exact original stores");
    const Bytes packet(m.view(request,96));
    check(packet.u32(0)==3u&&packet.u32(4)==effect&&packet.u32(8)==object&&packet.u32(0x30)==marker&&
        packet.u32(0x34)==expected&&packet.u32(0x38)==expected,
        "Actual generated170676 changed model-loop request keys or unrelated declared inputs");
    check(std::ranges::equal(metadata_before,m.view(model,0xa00)),"Producer changed authored model-state metadata");
    unsigned consumers{};
    for(unsigned offset:{0x34u,0x38u}) {
        put(m,out,0xdeadbeefu);
        for(const auto &[at,value]:std::array<std::pair<unsigned,unsigned>,4>{{
                {0,stop},{4,object},{8,packet.u32(offset)},{12,out}}})put(m,stack+at,value);
        SourcePhysicsWorldRuntimeAbi consumer{};consumer.gp[4]=stack;consumer.eflags=2u;
        source_run_physics_live_support_tu15(m,0xbcb70u,consumer,{0x27f,0x1f80},services);
        check((consumer.gp[0]&255u)==1u&&consumer.gp[4]==stack+16u&&Bytes(m.view(out,4)).u32(0)==0x3f800000u,
            "RealBCB70 rejected the actual compiled model-loop ONE producer");
        ++consumers;
    }
    for(unsigned wrong:{0u,0xffffffffu,0x130005a6u,0x030005b2u})
        check(source_object_function_compiled_symbol_tu15(m,0x170676u,wrong)==wrong,
            "Producer namespace join accepted a different full word");
    for(unsigned pc:{0x170675u,0x170677u,0x170688u,0x17068cu})
        check(source_object_function_compiled_symbol_tu15(m,pc,0x030005a6u)==0x030005a6u,
            "Producer namespace join spread to an adjacent instruction or store");
    binding.reset();
    check(source_object_function_compiled_symbol_tu15(m,0x170676u,0x030005a6u)==0x030005a6u,
        "Producer namespace join outlived its actual Cache binding");
    return {{"MCC_namespace",mcc},{"producer_entry",0x1704c0u},{"producer_instruction",0x170676u},
        {"producer_visits",producer_visits},{"primary_stores",primary_stores},{"secondary_stores",secondary_stores},
        {"produced_function",expected},{"actual_consumer_calls",consumers},{"native_owner",0xbcb70u},
        {"scope","Actual generated model-region prefix through original request stores and complete nativeBCB70; no effect allocation or full-world result supplied"}};
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
        report["model_loop_one_original"]=model_loop_one_seam(cache,false);
        report["model_loop_one_MCC"]=model_loop_one_seam(cache,true);
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
