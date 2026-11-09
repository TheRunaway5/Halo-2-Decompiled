#include "gameplay/weapons/native_tu15_effect_transform_product.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
using J=nlohmann::json;
namespace {
struct Prefix {};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
SourcePhysicsWorldRuntimeAbi abi_for(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");
    const auto fp=row.at("before_fp");a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};
    a.x87_status=fp[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}return a;
}
void same_abi(const SourcePhysicsWorldRuntimeAbi&a,const J&state,const J&row){
    check(a.gp==state.at("gp").get<std::array<std::uint32_t,8>>()&&a.eflags==state.at("eflags").get<std::uint32_t>(),"Effect product GP/full flags differ");
    const auto expected=state.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    for(unsigned n=0;n<8;++n)for(unsigned lane=0;lane<4;++lane)
        check(a.xmm[n][lane]==expected[n][lane],"Effect product XMM differs "+row.at("name").get<std::string>()+" xmm"+std::to_string(n)+" lane"+std::to_string(lane)+" actual="+std::to_string(a.xmm[n][lane])+" expected="+std::to_string(expected[n][lane]));
    const auto fp=state.at("fp");check(a.floating_point->x87_control_word==fp[0].get<unsigned>()&&a.x87_status==fp[1].get<unsigned>()&&a.floating_point->mxcsr==fp[2].get<unsigned>(),"Effect product FP controls/status differ");
    check(a.x87_depth==row.at("live_x87").size(),"Effect product live x87 depth changed");
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);check(!std::memcmp(&a.x87[n],value.data(),10),"Effect product live80 changed");}
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t*pc{};SourcePhysicsWorldRuntimeAbi*abi{};const J*row{};
    unsigned writes{},observed{};mutable unsigned reads{},packets{};std::string phase;unsigned ordinal{};
    mutable J read_events=J::array();J write_events=J::array();
    auto bank(std::uint32_t at,std::size_t count)const{auto i=banks.upper_bound(at);check(i!=banks.begin(),"Unmapped product bank");--i;check(std::uint64_t(at-i->first)+count<=i->second.size(),"Product access escaped bank");return i;}
    bool is_read()const{return *pc==0x145560u||*pc==0x14566eu||row->at("read_sites").contains(std::to_string(*pc));}
    void packet()const{
        if(!abi||!row->contains("io_states"))return;
        const auto key=std::to_string(*pc);const auto i=row->at("io_states").find(key);
        if(i!=row->at("io_states").end()){same_abi(*abi,*i,*row);++packets;}
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        packet();if(is_read()){
            if(phase=="before_read"&&reads==ordinal)throw Prefix{};
            ++reads;read_events.push_back(J{{"instruction",*pc},{"address",at},{"bytes",out.size()}});
        }
        const auto i=bank(at,out.size());std::copy_n(i->second.begin()+at-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>data)override{
        packet();if(phase=="before_write"&&writes==ordinal)throw Prefix{};
        const auto i=bank(at,data.size());auto&storage=banks.at(i->first);
        write_events.push_back(J{{"instruction",*pc},{"address",at},{"before",hex(std::span(storage).subspan(at-i->first,data.size()))},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),storage.begin()+at-i->first);++writes;
    }
};
Memory memory_for(const J&row,std::uint32_t&pc){
    Memory memory;memory.pc=&pc;memory.row=&row;
    for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    const auto fault=row.value("fault",J::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);return memory;
}
J refusal(const J&row,bool forced){
    std::uint32_t pc{};auto a=abi_for(row);auto memory=memory_for(row,pc);unsigned callbacks{};bool trace{};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){pc=p;++callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    const auto refused=[&](bool execute=true,const SourcePhysicsContactRecordObserver&observe={}){
        const auto original=abi_for(row);a=original;memory=memory_for(row,pc);callbacks=0;
        check(!native_effect_transform_product_detail::try_native(memory,a,services,observe),"Unsupported product protocol admitted");
        check(a.gp==original.gp&&a.xmm==original.xmm&&a.eflags==original.eflags&&memory.banks==memory_for(row,pc).banks,"Product refusal mutated input");
        if(execute){native_weapon_effect_dispatch_tu15_run(memory,0x145560u,a,services,observe);check(callbacks==69&&pc==0x14566eu,"Product fallback lost measured callbacks");same_abi(a,row.at("final"),row);}
    };
    if(forced){refused();return J{{"native_refused",true},{"callbacks",callbacks},{"last_pc",pc},{"returned_eax",a.gp[0]}};}
    const auto cursor=services.instruction_cursor;services.instruction_cursor={};refused();services.instruction_cursor=cursor;
    trace=true;refused();trace=false;
    const auto instruction=services.instruction;services.instruction=[instruction](auto p){instruction(p);};refused();services.instruction=instruction;
    services.instruction_cursor.executor={};refused();services.instruction_cursor.executor=std::this_thread::get_id();
    services.instruction_cursor.literal_service_sites=false;refused();services.instruction_cursor.literal_service_sites=true;
    services.instruction_cursor.observation_only_stores=false;unsigned stores{};refused(true,[&](const auto&){++stores;});check(stores==9,"Product fallback observer lost stores");
    services.instruction_cursor=cursor;
    a=abi_for(row);a.floating_point.reset();check(!native_effect_transform_product_detail::try_native(memory,a,services,{}),"Product admitted missing FP");
    a=abi_for(row);a.floating_point->mxcsr&=~0x80u;check(!native_effect_transform_product_detail::try_native(memory,a,services,{}),"Product admitted unmasked SSE");
    a=abi_for(row);a.active_mmx.emplace();check(!native_effect_transform_product_detail::try_native(memory,a,services,{}),"Product admitted active MMX");
    return {};
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply original product fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto&first=fixture.at("cases")[0];if(std::getenv("HALO2_EFFECT_PRODUCT_REFUSAL_TEST")){std::cout<<refusal(first,true).dump()<<'\n';return 0;}
    refusal(first,false);unsigned packets{};unsigned comparisons{};
    for(const auto&row:fixture.at("cases"))for(unsigned route=0;route<5;++route){
        if(!row.at("completed").get<bool>()&&route>=2)continue;
        std::uint32_t pc{};auto memory=memory_for(row,pc);auto a=abi_for(row);memory.abi=&a;
        SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};services.instruction=[&](auto p){++callbacks;pc=p;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observe=[&](const auto&){memory.packet();if(memory.phase=="after_write"&&memory.observed==memory.ordinal)throw Prefix{};++memory.observed;};
        const auto host=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));bool stopped{};
        try{
            if(route==0)check(native_effect_transform_product_detail::try_native(memory,a,services,observe),"Direct product native refused");
            else if(route==1)native_weapon_effect_dispatch_tu15_run(memory,0x145560u,a,services,observe);
            else if(route==2)native_weapon_effect_dispatch_tu15_run(memory,0x145560u,a,services,{});
            else if(route==3){
                SourceWeaponEffectPresentation presentation;SourcePhysicsContactRecordObserver empty;NativeWeaponEffectBirthColorCollector collector(presentation,empty);auto borrow=collector.borrow();
                services.instruction_cursor.observation_only_stores=false;
                check(NativeWeaponEffectBirthColorCollector::qualified(collector.observer()),"Actual product birth collector unqualified");
                native_weapon_effect_dispatch_tu15_run(memory,0x145560u,a,services,collector.observer());
            }
            else{services.instruction_cursor={};native_weapon_effect_dispatch_tu15_run(memory,0x145560u,a,services,observe);}
        }catch(const Prefix&){stopped=true;}
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0;fnstsw %1":"=m"(after_cw),"=m"(after_sw));
        check(host==_mm_getcsr()&&cw==after_cw&&sw==after_sw,"Product changed host FP");
        check(stopped==!row.at("completed").get<bool>(),"Product completion differs");
        check(callbacks==(route==4?69u:0u),"Product native/retained callback count differs");
        same_abi(a,row.at("final"),row);check(pc==row.at("last_pc").get<std::uint32_t>(),"Product cursor differs");
        check(memory.read_events==row.at("reads"),"Product physical read sequence differs");
        check(memory.write_events==row.at("writes"),"Product physical write sequence differs "+row.at("name").get<std::string>()+" route="+std::to_string(route)+" actual="+memory.write_events.dump()+" expected="+row.at("writes").dump());
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address").get<std::uint32_t>()))==bank.at("after").get<std::string>(),"Product final bank differs");
        packets+=memory.packets;++comparisons;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"live_io_full_abi_packets",packets},{"native_literal_callbacks",0}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
