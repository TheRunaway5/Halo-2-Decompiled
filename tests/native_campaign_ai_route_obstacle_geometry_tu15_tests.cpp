#include "halo2/native_campaign_ai_route_obstacle_geometry_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <xmmintrin.h>

// Whole physical-i386 obstacle marker/append/orientation replay. Checks every
// retained byte, ordered read, PC-tagged store, GP/XMM value and FP control.
// Callee-private stack storage is outside this whole-call contract.
namespace {
using namespace halo2;using Json=nlohmann::json;
constexpr std::uint32_t stack_low=0x2000000u,stack_end=0x2040000u,stop_pc=0x3000000u;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto value:data){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;}
std::vector<std::byte> before_bytes(const Json &bank) {
    std::vector<std::byte> out(bank.at("size").get<std::size_t>());
    for(const auto &run:bank.at("before_runs")){const auto data=bytes(run[1]);std::ranges::copy(data,out.begin()+run[0].get<std::size_t>());}
    return out;
}
std::vector<std::byte> after_bytes(const Json &bank) {
    auto out=before_bytes(bank);
    for(const auto &run:bank.at("after_runs")){const auto data=bytes(run[1]);std::ranges::copy(data,out.begin()+run[0].get<std::size_t>());}
    return out;
}
bool silent_owner(std::uint32_t){return false;}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    // Undeclared stack: pushes and private frames are neither recorded nor compared.
    mutable std::vector<std::byte> scratch=std::vector<std::byte>(stack_end-stack_low);
    mutable Json reads=Json::array();
    // Capstone's explicit-operand metadata omits PUSH/POP. Saved words
    // remain in full-bank comparisons and restored-register assertions.
    std::vector<std::uint32_t> implicit_saved_words;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{},written{};
    bool recording{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::optional<decltype(banks)::iterator> locate(std::uint32_t address,std::size_t width)const {
        auto found=banks.upper_bound(address);
        if(found!=banks.begin()){--found;if(std::uint64_t(address-found->first)+width<=found->second.size())return found;}
        if(address>=stack_low && std::uint64_t(address)+width<=stack_end)return std::nullopt;
        throw Missing{address,width};
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        const auto found=locate(address,out.size());
        if(!found){std::copy_n(scratch.begin()+(address-stack_low),out.size(),out.begin());return;}
        const bool implicit=out.size()==4 && std::ranges::find(implicit_saved_words,address)!=implicit_saved_words.end();
        if(!implicit && phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        std::copy_n((*found)->second.begin()+(address-(*found)->first),out.size(),out.begin());
        if(recording && !implicit)reads.push_back(Json::array({address,out.size()}));
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        const auto found=locate(address,values.size());
        if(!found){std::copy(values.begin(),values.end(),scratch.begin()+(address-stack_low));return;}
        const bool implicit=values.size()==4 && std::ranges::find(implicit_saved_words,address)!=implicit_saved_words.end();
        if(!implicit && phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        auto &bank=(*found)->second;const auto at=address-(*found)->first;
        if(!implicit)stores.push_back({0,address,{bank.begin()+at,bank.begin()+at+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),bank.begin()+at);
    }
    void install(std::uint32_t address,std::uint32_t value) {
        for(unsigned k=0;k<4;++k) {
            const auto byte=std::byte(value>>(k*8u));const auto at=address+k;
            auto found=banks.upper_bound(at);
            if(found!=banks.begin() && at-std::prev(found)->first<std::prev(found)->second.size())std::prev(found)->second[at-std::prev(found)->first]=byte;
            else scratch.at(at-stack_low)=byte;
        }
    }
    std::uint32_t peek_word(std::uint32_t address)const {
        const auto found=locate(address,4);std::uint32_t value{};
        const auto &data=found?(*found)->second:scratch;
        const auto offset=address-(found?(*found)->first:stack_low);
        for(unsigned k=0;k<4;++k)value|=std::to_integer<unsigned>(data[offset+k])<<(k*8u);
        return value;
    }
    explicit Memory(const Json &row):implicit_saved_words(row.value("implicit_saved_words",std::vector<std::uint32_t>{})) {
        for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),before_bytes(bank));
    }
};
SourcePhysicsWorldRuntimeAbi entry_abi(const Json &row,Memory &memory) {
    SourcePhysicsWorldRuntimeAbi abi{};
    for(unsigned n=0;n<8;++n)abi.gp[n]=row.at("gp")[n];
    for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)abi.xmm[r][k]=row.at("xmm")[r][k];
    abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=0;
    std::vector<std::uint32_t> words{row.value("return_pc",stop_pc),row.at("matrix").get<std::uint32_t>(),row.at("source").get<std::uint32_t>()};
    for(const auto &word:row.at("stack_arguments"))words.push_back(word);
    for(std::size_t n=0;n<words.size();++n)memory.install(abi.gp[4]+std::uint32_t(n)*4u,words[n]);
    return abi;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply the physical obstacle geometry fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","original authority changed");
    check(fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"original outcomes were supplied");
    std::size_t count{},reads{},stores{};
    for(const auto&row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),name+": unqualified reference");
        check(row.value("fault",Json::object()).empty()&&row.value("stopped_at",stop_pc)==stop_pc,name+": whole-entry cohort required");
        for(bool reading:{false,true}) {
            Memory memory(row);memory.recording=reading;auto abi=entry_abi(row,memory);const auto entry=row.at("entry").get<std::uint32_t>();
            SourcePhysicsContactRecordObserver observer;
            if(!reading)observer=[&](const auto &event){if(memory.locate(event.address,event.written.size())) {
                check(!memory.stores.empty()&&memory.stores.back().address==event.address,name+": missing observed store");memory.stores.back().instruction=event.instruction;}};
            const auto mxcsr=_mm_getcsr();std::uint16_t cw,sw;asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            try{native_campaign_ai_route_obstacle_geometry_call_tu15(memory,entry,abi,observer);}
            catch(const Missing &e){std::ostringstream why;why<<name<<": missing "<<std::hex<<e.address<<" width "<<e.width;throw std::runtime_error(why.str());}
            std::uint16_t aftercw,aftersw;asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(aftercw),"=m"(aftersw));
            check(cw==aftercw&&sw==aftersw&&mxcsr==_mm_getcsr(),name+": host FP leaked");
            const auto &gp=row.at("boundary_gp");
            for(unsigned n=0;n<8;++n)if(abi.gp[n]!=gp[n].get<std::uint32_t>()) {
                std::ostringstream why;why<<name<<": GP "<<n<<" differs native="<<std::hex<<abi.gp[n]<<" original="<<gp[n].get<std::uint32_t>();throw std::runtime_error(why.str());
            }
            for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)check(abi.xmm[r][k]==row.at("boundary_xmm")[r][k],name+": XMM differs");
            const auto actualfp=Json::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr});
            check(actualfp==row.at("after_fp"),name+": FP differs native="+actualfp.dump()+" original="+row.at("after_fp").dump());
            for(const auto&bank:row.at("banks")){const auto &a=memory.banks.at(bank.at("address"));const auto b=after_bytes(bank);if(a!=b){
                std::size_t n=0;while(n<a.size()&&a[n]==b[n])++n;std::ostringstream why;why<<name<<": bank differs at "<<std::hex<<bank.at("address").get<std::uint32_t>()+n<<" native="<<std::to_integer<unsigned>(a[n])<<" original="<<std::to_integer<unsigned>(b[n]);throw std::runtime_error(why.str());}}
            if(reading){Json expected=Json::array();for(const auto&e:row.at("reads"))expected.push_back(Json::array({e.at("address"),e.at("bytes")}));
                if(memory.reads!=expected){std::size_t n=0;while(n<memory.reads.size()&&n<expected.size()&&memory.reads[n]==expected[n])++n;
                    throw std::runtime_error(name+": read "+std::to_string(n)+" native="+(n<memory.reads.size()?memory.reads[n].dump():"end")+" original="+(n<expected.size()?row.at("reads")[n].dump():"end"));}reads+=expected.size();
            }else{const auto&writes=row.at("writes");check(memory.stores.size()==writes.size(),name+": store count differs native="+std::to_string(memory.stores.size())+" original="+std::to_string(writes.size()));
                for(std::size_t n=0;n<writes.size();++n){const auto&a=memory.stores[n];const auto&b=writes[n];
                    if(a.instruction!=b.at("instruction")||a.address!=b.at("address")||hex(a.before)!=b.at("before").get<std::string>()||hex(a.written)!=b.at("written").get<std::string>()) {
                        std::ostringstream why;why<<name<<": store "<<n<<" differs native="<<std::hex<<a.instruction<<" at="<<a.address<<" data="<<hex(a.written)<<" original="<<b.dump();throw std::runtime_error(why.str());}}
                stores+=writes.size();}
        }++count;
    }
    std::cout<<Json{{"passed",true},{"physical_cases",count},{"comparisons",count*2},{"reads",reads},{"stores",stores}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
