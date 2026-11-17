#include "halo2/native_campaign_ai_movement_jump_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;using u32=std::uint32_t;
constexpr u32 stack_low=0x2000000u,stack_size=0x40000u,stop=0x3000000u;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> result;for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return result;}
std::string hex(std::span<const std::byte> data){const char digits[]="0123456789abcdef";std::string out;for(auto byte:data){auto n=std::to_integer<unsigned>(byte);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> bank_bytes(const Json &bank,bool after=false){std::vector<std::byte> out(bank.at("size").get<std::size_t>());for(const auto &run:bank.at("before_runs")){const auto data=bytes(run[1]);std::ranges::copy(data,out.begin()+run[0].get<std::size_t>());}if(after)for(const auto &run:bank.at("after_runs")){const auto data=bytes(run[1]);std::ranges::copy(data,out.begin()+run[0].get<std::size_t>());}return out;}
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<u32,std::vector<std::byte>> banks;
    mutable std::vector<std::byte> stack=std::vector<std::byte>(stack_size);
    std::vector<SourcePhysicsContactRecordStore> stores;
    bool recording{};
    std::pair<std::vector<std::byte>*,u32> locate(u32 at,std::size_t size)const {
        auto it=banks.upper_bound(at);if(it!=banks.begin()){--it;if(std::uint64_t(at-it->first)+size<=it->second.size())return {&it->second,at-it->first};}
        if(at>=stack_low && std::uint64_t(at)+size<=stack_low+stack_size)return {&stack,at-stack_low};
        std::ostringstream why;why<<"missing native command storage "<<std::hex<<at<<" width "<<size;throw std::runtime_error(why.str());
    }
    void read(u32 at,std::span<std::byte> out)const override{const auto [data,offset]=locate(at,out.size());std::copy_n(data->begin()+offset,out.size(),out.begin());}
    void write(u32 at,std::span<const std::byte> value)override{const auto [data,offset]=locate(at,value.size());if(recording && data!=&stack)stores.push_back({0,at,{data->begin()+offset,data->begin()+offset+value.size()},{value.begin(),value.end()}});std::copy(value.begin(),value.end(),data->begin()+offset);}
    void word(u32 at,u32 value){std::array<std::byte,4> data{};for(unsigned i=0;i<4;++i)data[i]=std::byte(value>>(i*8));write(at,data);}
    explicit Memory(const Json &row){for(const auto &b:row.at("banks"))banks.emplace(b.at("address"),bank_bytes(b));}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply physical movement-jump fixture");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Original result supplied");
    check(fixture.at("fixture_kind")=="movement_jump", "Wrong original cohort");
    std::size_t count{},stores{},tagged{},refused{},producer{},checks{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified"),name+": original incomplete");Memory memory(row);SourcePhysicsWorldRuntimeAbi abi{};
        for(unsigned i=0;i<8;++i){abi.gp[i]=row.at("gp")[i];for(unsigned k=0;k<4;++k)abi.xmm[i][k]=row.at("xmm")[i][k];}
        abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
        std::vector<u32> words{stop,row.at("matrix"),row.at("source")};for(const auto &a:row.at("stack_arguments"))words.push_back(a);
        for(std::size_t i=0;i<words.size();++i)memory.word(abi.gp[4]+u32(i)*4u,words[i]);memory.recording=true;
        SourcePhysicsContactRecordObserver observe=[&](const auto &event){const auto [data,offset]=memory.locate(event.address,event.written.size());(void)offset;if(data==&memory.stack)return;check(!memory.stores.empty() && memory.stores.back().address==event.address && memory.stores.back().written==event.written,name+": store observer mismatch");memory.stores.back().instruction=event.instruction;};
        const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        const auto entry=row.at("entry").get<u32>();const bool is_producer=row.value("producer_region",false);u32 refusal{};
        try{check(entry==0xff1a0u && !is_producer,name+": unowned entry");native_campaign_ai_movement_jump_call_tu15(memory,entry,abi,{},observe);}
        catch(const NativeCampaignAiPerformRequiredOwnerTu15 &error){refusal=error.entry();}
        catch(const std::exception &error){throw std::runtime_error(name+": "+error.what());}
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
        const auto end=row.value("stopped_at",stop);
        if(is_producer){check(end==0x2b0c4au && !refusal,name+": producer boundary differs");++producer;}
        else if(end!=stop){check(refusal==end,name+": native refusal differs");++refused;}
        else {
            check(!refusal,name+": unexpected native refusal "+std::to_string(refusal));const auto &gp=row.at("boundary_gp");const u32 mask=(entry==0x262a00u || entry==0x263aa0u)?0xffffu:entry==0x263140u?0xffffffffu:0xffu;
            check((abi.gp[0]&mask)==(gp[0].get<u32>()&mask),name+": result differs");
            for(unsigned i:{3u,4u,5u,6u,7u})check(abi.gp[i]==gp[i].get<u32>(),name+": saved GP/ESP differs");checks+=6;
        }
        check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] && abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP differs");++checks;
        for(const auto &b:row.at("banks")) {
            const auto expected=bank_bytes(b,true);const auto &actual=memory.banks.at(b.at("address"));
            if(expected!=actual){std::size_t n{};while(n<expected.size() && actual[n]==expected[n])++n;std::ostringstream why;why<<name<<": DATA mismatch "<<std::hex<<b.at("address").get<u32>()+n<<" native="<<std::to_integer<unsigned>(actual[n])<<" original="<<std::to_integer<unsigned>(expected[n]);throw std::runtime_error(why.str());}
        }
        const auto &writes=row.at("writes");check(memory.stores.size()==writes.size(),name+": store count differs native="+std::to_string(memory.stores.size())+" original="+std::to_string(writes.size()));
        for(std::size_t n=0;n<writes.size();++n){const auto &a=memory.stores[n];const auto &b=writes[n];const bool pc=a.instruction==b.at("instruction");check(pc && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": ordered store differs "+std::to_string(n)+" original "+b.dump());tagged+=a.instruction!=0;}
        stores+=writes.size();++count;
    }
    std::cout<<Json{{"passed",true},{"cases",count},{"producer_regions",producer},{"named_refusals",refused},{"ordered_stores",stores},{"tagged_store_pcs",tagged},{"gp_fp_assertions",checks},{"ordered_reads_claimed",false},{"private_child_frames_claimed",false}}.dump()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
