#include "halo2/native_campaign_ai_area_count_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
struct Prefix{};
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    Json timeline=Json::array();mutable Json applied=Json::array(),reads=Json::array();
    mutable std::map<std::uint32_t,unsigned> occurrences;
    std::optional<unsigned> fault;mutable unsigned attempts{},returns{};
    std::uint32_t return_address{};unsigned writes{};
    auto locate(std::uint32_t at,std::size_t width)const{
        auto found=banks.upper_bound(at);check(found!=banks.begin(),"Missing area input storage");--found;
        check(std::uint64_t(at-found->first)+width<=found->second.size(),"Area input extent differs");return found;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override{
        if(at==return_address){++returns;const auto found=locate(at,out.size());std::copy_n(found->second.begin()+at-found->first,out.size(),out.begin());return;}
        if(fault && attempts++==*fault)throw Prefix{};
        const auto found=locate(at,out.size());std::copy_n(found->second.begin()+at-found->first,out.size(),out.begin());
        reads.push_back(Json::array({at,out.size()}));const auto occurrence=++occurrences[at];
        for(const auto &change:timeline)if(change.at("on_read").at("address")==at && change.at("on_read").at("occurrence")==occurrence){
            for(const auto &edit:change.at("writes")){const auto data=bytes(edit.at("bytes"));const auto target=edit.at("address").get<std::uint32_t>();const auto region=locate(target,data.size());std::copy(data.begin(),data.end(),region->second.begin()+target-region->first);}
            applied.push_back(change);
        }
    }
    void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Area query attempted persistent write");}
    explicit Memory(const Json &row){for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));timeline=row.at("input_timeline");}
};
SourcePhysicsWorldRuntimeAbi abi_from(const Json &row){
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.eflags=0x602;abi.fs_base=0x87650000;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
    for(unsigned i=0;i<8;++i)for(unsigned lane=0;lane<4;++lane)abi.xmm[i][lane]=0x3f000000u+i*4u+lane;
    if(row.at("active_mmx").get<bool>()){
        abi.active_mmx=SourcePhysicsMmxAlias{};abi.active_mmx->known_mask=0xffu;
        for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("live_x87")[n]);std::copy_n(raw.begin(),10,abi.active_mmx->physical[n].begin());}
    }else for(const auto &value:row.at("live_x87")){const auto raw=bytes(value);std::memcpy(&abi.x87[abi.x87_depth++],raw.data(),10);}
    return abi;
}
}
int main(int argc,char **argv)try{
    check(argc==2,"Supply independent area-query original fixture");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Different original area image");
    check(fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Original area outcomes/instructions were supplied");
    std::size_t cases{},calls{},reads{},timelines{};
    for(const auto &row:fixture.at("cases")){
        const auto entry=row.at("entry").get<std::uint32_t>();const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>()&&row.at("writes").empty(),name+": nonphysical or impure query reference");
        for(bool binding:{false,true}){
            check(entry==0x1bbb50u,"Unowned reference entry");
            const auto fault=row.value("fault",Json::object());const auto ordinal=fault.value("ordinal",0u);
            if(!binding&&!fault.empty()&&ordinal==0)continue; // typed input starts after argument transport
            Memory memory(row);auto abi=abi_from(row);const auto initial=abi;memory.return_address=abi.gp[4];
            if(binding)memory.banks.emplace(abi.gp[4],bytes("97011b00"));
            if(!fault.empty())memory.fault=ordinal-unsigned(!binding);
            const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(control),"=m"(status));
            bool stopped{};std::optional<std::uint16_t> result;
            try{
                if(binding)native_campaign_ai_area_count_tu15_run(memory,{0x1b0195u,0x1bbb50u,false},abi);
                else result=native_campaign_ai_has_multiple_areas_tu15(memory,row.at("matrix"));
            }catch(const Prefix&){stopped=true;}
            std::uint16_t after_control{},after_status{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_control),"=m"(after_status));
            check(control==after_control&&status==after_status&&mxcsr==_mm_getcsr(),name+": host FP changed");
            check(stopped==!row.at("completed").get<bool>(),name+": original query completion differs");
            auto expected=initial.gp;
            if(!stopped){
                const auto candidate=row.at("boundary_gp")[0].get<std::uint32_t>()&0xffffu;
                check(row.at("boundary_gp")[4]==initial.gp[4]+8u,name+": physical original stack contract differs");
                for(const auto reg:{3u,5u,6u,7u})check(row.at("boundary_gp")[reg]==initial.gp[reg],name+": original callee preservation differs");
                if(binding){expected[0]=(expected[0]&0xffff0000u)|candidate;expected[4]+=8u;}
                else check(result==candidate,name+": original area result differs");
            }
            check(abi.gp==expected,name+": candidate AX16/RET4/native preserved GPR contract differs");
            check(memory.returns==unsigned(binding&&!stopped),name+": actual RET word consumption differs");
            check(abi.eflags==initial.eflags&&abi.fs_base==initial.fs_base&&abi.xmm==initial.xmm&&
                !std::memcmp(abi.x87.data(),initial.x87.data(),sizeof(abi.x87))&&abi.x87_depth==initial.x87_depth&&
                abi.x87_status==initial.x87_status&&abi.floating_point&&abi.floating_point->x87_control_word==initial.floating_point->x87_control_word&&
                abi.floating_point->mxcsr==initial.floating_point->mxcsr&&abi.active_mmx.has_value()==initial.active_mmx.has_value()&&
                (!abi.active_mmx||(abi.active_mmx->known_mask==initial.active_mmx->known_mask&&abi.active_mmx->physical==initial.active_mmx->physical)),
                name+": native transport changed flags/SIMD/liveFP/MMX/policy");
            Json expected_reads=Json::array();unsigned index{};
            for(const auto &event:row.at("reads")){if(!binding&&index++==0)continue;expected_reads.push_back(Json::array({event.at("address"),event.at("bytes")}));}
            if(memory.reads!=expected_reads)std::cerr<<name<<" actual "<<memory.reads.dump()<<" expected "<<expected_reads.dump()<<'\n';
            check(memory.reads==expected_reads,name+": original ordered reads differ");
            check(memory.applied==row.at("applied_input_timeline"),name+": declared input publication timeline differs");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained query/input bytes differ");
            check(memory.writes==0,name+": query emitted persistent memory writes");++calls;reads+=memory.reads.size();timelines+=memory.applied.size();
        }
        ++cases;
    }
    const auto &complete=*std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),[](const auto &row){return row.at("completed").template get<bool>()&&row.at("input_timeline").empty();});
    const SourcePhysicsContactRuntimeCall call{0x1b0195u,0x1bbb50u,false};
    for(const auto wrong:std::array{SourcePhysicsContactRuntimeCall{0x1b0194u,0x1bbb50u,false},
            SourcePhysicsContactRuntimeCall{0x1b0195u,0x1bbb51u,false},
            SourcePhysicsContactRuntimeCall{0x1b0195u,0x1bbb50u,true}}){
        Memory memory(complete);auto abi=abi_from(complete);bool rejected{};
        const auto activity=native_campaign_ai_area_count_calls_tu15();
        try{native_campaign_ai_area_count_tu15_run(memory,wrong,abi);}catch(const std::exception&){rejected=true;}
        check(rejected&&memory.reads.empty()&&!memory.writes,"Unowned callback performed source work");
        check(native_campaign_ai_area_count_calls_tu15()==activity,"Unowned callback published native activity");
    }
    check(native_campaign_ai_area_count_calls_tu15()>0,"Native area-count binding did not execute");
    for(const bool backed:std::array{false,true}){
        Memory memory(complete);auto abi=abi_from(complete);const auto sp=abi.gp[4];
        memory.return_address=sp;
        if(backed)memory.banks.emplace(sp,bytes("98011b00"));
        bool rejected{};
        try{native_campaign_ai_area_count_tu15_run(memory,call,abi);}catch(const std::exception&){rejected=true;}
        check(rejected&&memory.returns==1&&abi.gp[4]==sp,"Invalid actual return fabricated RET4");
        check((abi.gp[0]&0xffffu)==(complete.at("boundary_gp")[0].get<unsigned>()&0xffffu),"Failed RET lost completed algorithm result");
    }
    std::cout<<cases<<" physical original cases/prefixes; "<<calls<<" native typed/binding calls; "<<reads<<" ordered reads; "<<timelines<<" declared input changes; AX16/MMX/FP/read-only/missingRET passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
