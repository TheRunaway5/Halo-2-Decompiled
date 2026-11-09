#include "gameplay/weapons/native_tu15_effect_datum_allocate.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <xmmintrin.h>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace copies {
std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unqualified datum sign extension");
}
#include "effect_datum_allocate_bodies.inc"
}
namespace {
struct Prefix {};
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>input){constexpr char digits[]="0123456789abcdef";std::string s;for(auto b:input){const auto v=std::to_integer<unsigned>(b);s+=digits[v>>4];s+=digits[v&15];}return s;}
void check(bool condition,const std::string&s){if(!condition)throw std::runtime_error(s);}
bool original_read(std::uint32_t pc) {
    switch(pc){
    case 0x16e6a3:case 0x16e6a7:case 0x16e6b1:case 0x16e6c5:case 0x16e6ca:case 0x16e6db:
    case 0x16e6e7:case 0x16e6ea:case 0x16e6f0:case 0x16e6fa:case 0x16e710:case 0x16e713:
    case 0x16e727:case 0x16e73c:case 0x16e743:case 0x16e747:case 0x16e754:
    case 0x16e75c:case 0x16e75d:case 0x16e75e:case 0x16e75f:case 0x16e760:return true;
    default:return false;
    }
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t*pc{};std::string fault;unsigned ordinal{};mutable unsigned read_count{};unsigned write_count{},observer_count{};
    mutable J reads=J::array();J writes=J::array();
    const J*io{};const std::array<std::uint32_t,8>*live_gp{};const std::uint32_t*live_flags{};
    mutable unsigned io_checks{};
    void state(const char*phase,unsigned index)const {
        if(!io||!live_gp)return;const auto found=io->find(std::string(phase)+"-"+std::to_string(index));if(found==io->end())return;
        check(*live_gp==found->at("gp").get<std::array<std::uint32_t,8>>()&&*live_flags==found->at("eflags"),
            "Complete live datum GP/flags differ at "+std::string(phase)+"-"+std::to_string(index)+" PC="+std::to_string(*pc));
        ++io_checks;
    }
    auto at(std::uint32_t p,std::size_t n)const {
        auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped datum bank");--i;
        check(std::uint64_t(p-i->first)+n<=i->second.size(),"Datum access escaped bank");return i;
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override {
        if(original_read(*pc)) {
            state("before_read",read_count);
            if(fault=="before_read"&&read_count==ordinal)throw Prefix{};
            ++read_count;reads.push_back(J::array({*pc,p,out.size()}));
        }
        const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>input)override {
        state("before_write",write_count);
        if(fault=="before_write"&&write_count==ordinal)throw Prefix{};
        const auto i=at(p,input.size());auto&bank=banks.at(i->first);const auto before=std::span(bank).subspan(p-i->first,input.size());
        writes.push_back(J{{"instruction",*pc},{"address",p},{"before",hex(before)},{"written",hex(input)}});
        std::copy(input.begin(),input.end(),bank.begin()+p-i->first);++write_count;
    }
};
SourcePhysicsWorldRuntimeAbi abi_for(const J&row) {
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");
    const auto fp=row.at("before_fp");a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};
    a.x87_status=fp[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}
    return a;
}
void same_floating(const SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsWorldRuntimeAbi&b,const std::string&name) {
    check(a.xmm==b.xmm&&a.x87_depth==b.x87_depth&&a.x87_status==b.x87_status&&
        a.floating_point->mxcsr==b.floating_point->mxcsr&&a.floating_point->x87_control_word==b.floating_point->x87_control_word,name+": live FP transport changed");
    for(unsigned n=0;n<a.x87_depth;++n)check(!std::memcmp(&a.x87[n],&b.x87[n],10),name+": live80 payload changed");
}
J fallback(const J&row,bool forced) {
    Memory seed;std::uint32_t pc{};seed.pc=&pc;
    for(const auto&bank:row.at("banks"))seed.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    SourcePhysicsWorldRuntimeServices services;unsigned instructions{};bool trace{};
    services.instruction=[&](auto at){++instructions;pc=at;};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    const auto refused=[&](const SourcePhysicsContactRecordObserver&observe={}) {
        Memory memory=seed;auto abi=abi_for(row);const auto gp=abi.gp;const auto flags=abi.eflags;
        check(!native_effect_datum_allocate_detail::try_native(memory,abi.gp,abi.eflags,services,observe),"Unqualified datum protocol admitted");
        check(abi.gp==gp&&abi.eflags==flags&&memory.banks==seed.banks,"Datum refusal changed caller input");
        instructions=0;native_weapon_effect_integer_tu15_run(memory,0x16e6a0u,abi.gp,abi.eflags,services,observe);
        check(instructions&&pc==0x16e760u&&abi.gp==row.at("boundary_gp").get<std::array<std::uint32_t,8>>(),"Refused datum lost retained callbacks/normal result");
        return J{{"reference_admission_refused",true},{"literal_callbacks",instructions},{"last_literal_pc",pc},{"returned_eax",abi.gp[0]}};
    };
    if(forced)return refused();
    auto saved=cursor;cursor={};refused();cursor=saved;
    trace=true;refused();trace=false;
    const auto original=services.instruction;services.instruction=[original](auto at){original(at);};refused();services.instruction=original;
    cursor.executor={};refused();cursor.executor=std::this_thread::get_id();
    constexpr std::array changed{0x38846eu,0x21f590u,0x16e6a0u};cursor.service_sites=changed;refused();cursor.service_sites=sites;
    cursor.literal_service_sites=false;refused();cursor.literal_service_sites=true;
    cursor.observation_only_stores=false;unsigned observed{};SourcePhysicsContactRecordObserver unknown=[&](const auto&){++observed;};
    refused(unknown);check(observed>0,"Unknown observer lost retained stores");
    return {};
}
}
int main(int argc,char**argv)try {
    check(argc==2,"Supply unchanged physical datum fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto&first=fixture.at("cases")[0];
    if(std::getenv("HALO2_EFFECT_DATUM_REFUSAL_TEST")){std::cout<<fallback(first,true).dump()<<'\n';return 0;}
    fallback(first,false);unsigned checked_io{},expected_io{};
    for(const auto&row:fixture.at("cases"))if(row.contains("io_states"))expected_io+=unsigned(row.at("io_states").size())*4u;
    for(const auto&row:fixture.at("cases")) {
      for(unsigned route=0;route<11;++route) {
        if(!row.at("completed").get<bool>()&&route>=9)continue;
        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);Memory memory;std::uint32_t pc{};memory.pc=&pc;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto fault=row.value("fault",J::object());memory.fault=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        const auto incoming=abi_for(row);auto abi=incoming;auto&gp=abi.gp;auto&flags=abi.eflags;
        if(row.contains("io_states")&&(route<=3||route>=9)){memory.io=&row.at("io_states");memory.live_gp=&gp;memory.live_flags=&flags;}
        SourcePhysicsWorldRuntimeServices services;unsigned instructions{};services.instruction=[&](auto at){++instructions;pc=at;};
        constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction_cursor={&pc,nullptr,nullptr,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        J events=J::array();SourcePhysicsContactRecordObserver observe=[&](const auto&e){
            memory.state("after_write",memory.observer_count);
            events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
            if(memory.fault=="after_write"&&memory.observer_count==memory.ordinal)throw Prefix{};
            ++memory.observer_count;
        };
        bool stopped{};WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
        try{
            if(route==0)check(native_effect_datum_allocate_detail::try_native(memory,gp,flags,services,observe),"Native datum unexpectedly refused");
            else if(route==1)native_weapon_effect_datum_allocate_tu15_run(memory,gp,flags,services,observe);
            else if(route==2)native_weapon_effect_integer_tu15_run(memory,0x16e6a0u,gp,flags,services,observe);
            else if(route==3)native_weapon_effect_dispatch_tu15_run(memory,0x16e6a0u,abi,services,observe);
            else if(route==9){
                services.instruction_cursor.observation_only_stores=false;SourcePhysicsContactRecordObserver empty;
                SourceWeaponEffectPresentation presentation;NativeWeaponEffectBirthColorCollector collector(presentation,empty);auto borrow=collector.borrow();
                check(native_effect_slot_scan_detail::admitted_protocol(services,collector.observer()),"Actual birth collector failed datum qualification");
                native_weapon_effect_datum_allocate_tu15_run(memory,gp,flags,services,collector.observer());
            }else if(route==10)native_weapon_effect_integer_tu15_run(memory,0x16e6a0u,gp,flags,services,{});
            else {
                switch(route){case 4:copies::frame::run_16e6a0(state);break;case 5:copies::level::run_16e6a0(state);break;
                case 6:copies::shadow::run_16e6a0(state);break;case 7:copies::scene::run_16e6a0(state);break;case 8:copies::primary::run_16e6a0(state);break;}
                abi=state.save();
            }
        }
        catch(const Prefix&){stopped=true;}
        if(route>=4&&route<=8)abi=state.save();same_floating(abi,incoming,name);
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0; fnstsw %1":"=m"(after_cw),"=m"(after_sw));
        check(mxcsr==_mm_getcsr()&&cw==after_cw&&sw==after_sw,name+": host FP changed");
        check(stopped==!row.at("completed").get<bool>(),name+": prefix completion differs");check(!instructions,name+": native datum entered instruction body");
        const auto expected_gp=row.at("boundary_gp").get<std::array<std::uint32_t,8>>();
        if(gp!=expected_gp||flags!=row.at("boundary_eflags")) {
            J actual{{"gp",gp},{"eflags",flags},{"last_pc",pc}};
            throw std::runtime_error(name+": datum ABI differs actual="+actual.dump()+" expected_gp="+row.at("boundary_gp").dump()+" expected_flags="+row.at("boundary_eflags").dump());
        }
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": complete bank differs");
        check(memory.writes==row.at("writes"),name+": committed ordered stores differ");
        if(memory.fault!="before_write") {
            J reads=J::array();for(const auto&e:row.at("reads"))reads.push_back(J::array({e.at("instruction"),e.at("address"),e.at("bytes")}));
            check(memory.reads==reads,name+": original operand read prefix differs actual="+memory.reads.dump()+" expected="+reads.dump());
        }
        if(route<9&&memory.fault!="before_write"&&memory.fault!="before_read")check(events==row.at("writes"),name+": completed observer stream differs");
        if(route==3&&row.at("completed").get<bool>()){
            Memory opaque_memory;opaque_memory.pc=&pc;for(const auto&bank:row.at("banks"))opaque_memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            auto opaque=incoming;opaque.active_mmx=SourcePhysicsMmxAlias{};opaque.active_mmx->known_mask=0x5a;
            for(unsigned n=0;n<8;++n)for(unsigned k=0;k<10;++k)opaque.active_mmx->physical[n][k]=std::byte(n*10+k);
            const auto before=opaque.active_mmx;native_weapon_effect_dispatch_tu15_run(opaque_memory,0x16e6a0u,opaque,services,{});
            check(opaque.active_mmx&&opaque.active_mmx->known_mask==before->known_mask&&opaque.active_mmx->physical==before->physical,name+": opaque MMX transport changed");
        }
        checked_io+=memory.io_checks;
      }
    }
    check(checked_io>=expected_io,"Missing actual live I/O state checks");std::cerr<<"live_io "<<checked_io<<'\n';
    std::cout<<"PASS "<<fixture.at("cases").size()<<" native datum cases over9 full-prefix routes +2 normal production observers\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
