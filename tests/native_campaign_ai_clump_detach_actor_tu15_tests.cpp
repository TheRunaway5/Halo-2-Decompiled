#include "halo2/native_campaign_ai_clump_retirement_tu15_binding.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;using U=std::uint32_t;
constexpr U stack_base=0x2000000u,stack_size=0x40000u,stop=0x3000000u;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> result;for(std::size_t n=0;n<text.size();n+=2)result.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return result;}
std::string hex(std::span<const std::byte> bytes){constexpr char digits[]="0123456789abcdef";std::string result;for(auto b:bytes){auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}return result;}
std::vector<std::byte> bank_data(const Json &bank,bool after=false){std::vector<std::byte> data(bank.at("size").get<std::size_t>());for(const auto &r:bank.at("before_runs")){auto b=bytes(r[1]);std::ranges::copy(b,data.begin()+r[0].get<std::size_t>());}if(after)for(const auto &r:bank.at("after_runs")){auto b=bytes(r[1]);std::ranges::copy(b,data.begin()+r[0].get<std::size_t>());}return data;}
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<U,std::vector<std::byte>> banks;
    mutable std::vector<std::byte> stack=std::vector<std::byte>(stack_size);
    std::vector<SourcePhysicsContactRecordStore> stores;bool recording{};
    std::pair<std::vector<std::byte>*,U> locate(U at,std::size_t size)const {
        auto it=banks.upper_bound(at);if(it!=banks.begin()){--it;if(std::uint64_t(at-it->first)+size<=it->second.size())return {&it->second,at-it->first};}
        if(at>=stack_base && std::uint64_t(at)+size<=stack_base+stack_size)return {&stack,at-stack_base};
        std::ostringstream why;why<<"Missing clump-detach bank "<<std::hex<<at<<" size "<<size;throw std::runtime_error(why.str());
    }
    void read(U at,std::span<std::byte> out)const override{auto [data,offset]=locate(at,out.size());std::copy_n(data->begin()+offset,out.size(),out.begin());}
    void write(U at,std::span<const std::byte> value)override{auto [data,offset]=locate(at,value.size());if(recording && data!=&stack)stores.push_back({0,at,{data->begin()+offset,data->begin()+offset+value.size()},{value.begin(),value.end()}});std::ranges::copy(value,data->begin()+offset);}
    void word(U at,U value){std::array<std::byte,4> data{};for(unsigned n=0;n<4;++n)data[n]=std::byte(value>>(n*8));write(at,data);}
    explicit Memory(const Json &row){for(const auto &b:row.at("banks"))banks.emplace(b.at("address"),bank_data(b));}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply unchanged original fixture");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Original authority differs");
    check(fixture.at("fixture_kind")=="native-clump-detach-actor-v1" && fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Fixture contract differs");
    std::size_t count{},stores{},checks{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified") && row.at("completed"),name+": original incomplete");Memory memory(row);SourcePhysicsWorldRuntimeAbi abi{};
        for(unsigned i=0;i<8;++i){abi.gp[i]=row.at("gp")[i];for(unsigned k=0;k<4;++k)abi.xmm[i][k]=row.at("xmm")[i][k];}
        abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
        memory.word(abi.gp[4],stop);memory.word(abi.gp[4]+4u,row.at("matrix"));memory.word(abi.gp[4]+8u,row.at("source"));memory.recording=true;
        auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        try{native_campaign_ai_clump_retirement_tu15_run(memory,row.at("entry"),abi);}catch(const std::exception &e){throw std::runtime_error(name+": "+e.what());}
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
        // This void semantic boundary promises caller-consumed retained DATA,
        // saved GP/ESP and unchanged FP/XMM. Dead volatile GP are not results.
        for(unsigned i:{3u,4u,5u,6u,7u}){check(abi.gp[i]==row.at("boundary_gp")[i],name+": saved GP/ESP differs");++checks;}
        for(unsigned i=0;i<8;++i)for(unsigned k=0;k<4;++k){check(abi.xmm[i][k]==row.at("boundary_xmm")[i][k],name+": XMM differs");++checks;}
        check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] && abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP differs");++checks;
        for(const auto &b:row.at("banks"))check(memory.banks.at(b.at("address"))==bank_data(b,true),name+": retained DATA differs");
        const auto &writes=row.at("writes");check(memory.stores.size()==writes.size(),name+": store count differs");
        for(std::size_t n=0;n<writes.size();++n){const auto &a=memory.stores[n];const auto &b=writes[n];check(a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": ordered store differs");}
        stores+=writes.size();++count;
    }
    std::cout<<Json{{"passed",true},{"cases",count},{"ordered_stores",stores},{"gp_xmm_fp_assertions",checks},{"private_frame_bytes_claimed",false},{"volatile_gp_claimed",false},{"store_pc_claimed",false}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
