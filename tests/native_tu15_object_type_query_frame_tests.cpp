#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "gameplay/objects/native_tu15_object_type_query_frame.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#ifdef HALO2_TYPE_QUERY_CHILD_FIXTURE
#include "rendering/effects/source_effect_compiled_symbols_tu15.cpp"
#endif
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
using J=nlohmann::json;
namespace {
#ifdef HALO2_TYPE_QUERY_CHILD_FIXTURE
#include HALO2_TYPE_QUERY_CHILD_FIXTURE
#endif
struct Fault{};
void require(bool ok,const std::string&why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte>bytes(const std::string&text){std::vector<std::byte>out;for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>raw){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:raw){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4u];out+=digits[n&15u];}return out;}
J packet(const State&s,std::uint32_t pc){const auto a=s.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};}
void hostile(){const std::uint16_t cw=0x37eu;asm volatile("fninit;fldz;fldz;fdivp;fldcw %0"::"m"(cw):"memory","st","st(1)");_mm_setcsr(0x8065u);}
struct Memory final:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::function<J()>capture;mutable J reads=J::array(),packets=J::array();J writes=J::array();
    std::uint32_t pc{};bool noisy{},capture_packets{true};std::optional<J>fault;mutable unsigned read_ordinal{};unsigned write_ordinal{};
    std::byte*at(std::uint32_t p,std::size_t width)const{auto it=banks.upper_bound(p);require(it!=banks.begin(),"Unbacked object-type-query address");--it;require(std::uint64_t(p-it->first)+width<=it->second.size(),"Object-type-query bank extent");return const_cast<std::byte*>(it->second.data())+(p-it->first);}
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(capture_packets)packets.push_back(capture());if(fault&&fault->at("phase").get<std::string>()=="before_read"&&read_ordinal==fault->at("ordinal").get<unsigned>())throw Fault{};
        ++read_ordinal;reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",out.size()}});
        std::memcpy(out.data(),at(p,out.size()),out.size());if(noisy)hostile();
    }
    void write(std::uint32_t p,std::span<const std::byte>raw)override{
        if(capture_packets)packets.push_back(capture());if(fault&&fault->at("phase").get<std::string>()=="before_write"&&write_ordinal==fault->at("ordinal").get<unsigned>())throw Fault{};
        ++write_ordinal;auto*target=at(p,raw.size());writes.push_back(J{{"instruction",pc},{"address",p},{"before",hex(std::span(target,raw.size()))},{"written",hex(raw)}});
        std::memcpy(target,raw.data(),raw.size());if(noisy)hostile();
    }
};
}
int main(int argc,char**argv)try{
    require(argc==2,"Supply original object-type-query fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    unsigned comparisons{},packets{};
    for(const auto&row:fixture.at("cases"))for(bool noisy:{false,true}){
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){memory.pc=pc;};SourcePhysicsContactRecordObserver observer;State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}state.load(abi);
        memory.noisy=noisy;if(row.contains("fault"))memory.fault=row.at("fault");memory.capture=[&]{return packet(state,memory.pc);};
        bool stopped{};try{native_object_type_query_detail::run_private(state,[&](auto&v,auto target,auto site,bool tail){
#ifdef HALO2_TYPE_QUERY_CHILD_FIXTURE
            require(target==0xe0900u&&site==0x10de95u&&!tail,"Undeclared inherited callback owner");
            memory.packets.push_back(packet(v,target));
            const bool old_noisy=memory.noisy;memory.noisy=false;memory.capture_packets=false;
            try{genuine_unit::run_e0900(v);}catch(...){memory.noisy=old_noisy;memory.capture_packets=true;throw;}
            memory.noisy=old_noisy;memory.capture_packets=true;
#else
            throw std::runtime_error("Undeclared type-query callback");
#endif
        },[&](auto&,auto pc){memory.pc=pc;if(noisy)hostile();});}catch(const Fault&){stopped=true;}
        require(stopped==row.contains("fault"),"Object-type-query completion boundary differs");
        const auto name=row.at("name").get<std::string>()+(noisy?" hostile":" ordinary");const auto actual=packet(state,memory.pc);
        require(actual==row.at("expected"),name+" final ABI differs actual="+actual.dump()+" expected="+row.at("expected").dump());
        if(memory.packets!=row.at("calls")){
            const auto&expected=row.at("calls");std::size_t first{};
            while(first<std::min(memory.packets.size(),expected.size())&&memory.packets[first]==expected[first])++first;
            throw std::runtime_error(name+" pre-I/O packets differ at "+std::to_string(first)+" counts="+std::to_string(memory.packets.size())+"/"+std::to_string(expected.size())+" actual="+(first<memory.packets.size()?memory.packets[first].dump():"missing")+" expected="+(first<expected.size()?expected[first].dump():"missing"));
        }
        require(memory.reads==row.at("reads"),name+" read sequence differs actual="+memory.reads.dump()+" expected="+row.at("reads").dump());require(memory.writes==row.at("writes"),name+" write sequence differs");
        for(const auto&bank:row.at("banks"))require(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+" final bank differs");
        ++comparisons;packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"full_pre_io_packets",packets},{"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
