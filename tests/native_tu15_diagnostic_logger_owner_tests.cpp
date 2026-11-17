#include "runtime/native_tu15_diagnostic_logger_owner.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
using J=nlohmann::json;
namespace owner=halo2::native_diagnostic_logger_owner_detail;
struct ReadFailure{};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>unhex(const std::string&s){std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
J packet(const SourcePhysicsWorldRuntimeAbi&a){
    J live=J::array();for(unsigned n=0;n<a.x87_depth&&n<8;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    J mmx=nullptr;if(a.active_mmx){J physical=J::array();for(const auto&value:a.active_mmx->physical)physical.push_back(hex(value));mmx=J{{"physical",physical},{"known_mask",a.active_mmx->known_mask}};}
    return J{{"gp",a.gp},{"eflags",a.eflags},{"fs_base",a.fs_base},{"xmm",a.xmm},
        {"fp",a.floating_point?J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr}):J(nullptr)},
        {"live_x87",live},{"x87_depth",a.x87_depth},{"active_mmx",mmx}};
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    const SourcePhysicsWorldRuntimeAbi*abi{};mutable std::uint32_t pc{};
    bool fail{},hostile{};mutable unsigned attempts{},writes{};mutable J reads=J::array(),packets=J::array();
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        ++attempts;packets.push_back(packet(*abi));
        if(hostile){asm volatile("fninit; fld1; fldz":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x3f80);}
        if(fail)throw ReadFailure{};
        auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unbacked getter input");--it;
        check(std::uint64_t(address-it->first)+out.size()<=it->second.size(),"Getter input extent differs");
        std::memcpy(out.data(),it->second.data()+address-it->first,out.size());
        reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});
    }
    void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Childless getter wrote memory");}
};
void reset_host(){asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);}
unsigned guards(){
    unsigned tested{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f591u,0x33b595u};
    for(unsigned policy=0;policy<21;++policy){
        reset_host();Memory memory;unsigned callbacks{},observers{},required{};bool first{},second{};
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,false};
        services.required_owner=[&](auto&,const auto&,auto&,const auto&){++required;};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1,2,3,4,0x62001000,6,7,8};abi.eflags=0x246;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};
        memory.abi=&abi;memory.banks.emplace(0x62000000u,std::vector<std::byte>(8192));
        SourcePhysicsContactRecordObserver observe=[&](const auto&){++observers;};
        SourcePhysicsContactRuntimeCall call{0x30ae24u,owner::entry,false};
        if(policy==0)call.target=0x2e16d1u;
        if(policy==1)abi.floating_point.reset();if(policy==2)abi.active_mmx=SourcePhysicsMmxAlias{};
        if(policy==3)abi.x87_depth=9;if(policy==4)abi.x87_status=0x80;if(policy==5)abi.x87_status=0x8000;
        if(policy==6)abi.floating_point->x87_control_word=0x37e;if(policy==7)abi.floating_point->mxcsr=0x1f00;if(policy==8)abi.floating_point->mxcsr|=0x10000;
        if(policy==9)services.instruction={};if(policy==10){auto old=services.instruction;services.instruction=[old](auto pc){old(pc);};}
        if(policy==11)services.instruction_cursor.executor={};if(policy==12)services.instruction_cursor.callback_type=nullptr;if(policy==13)services.instruction_cursor.callback_type=&typeid(int);
        if(policy==14)services.instruction_cursor.position=nullptr;if(policy==15)services.instruction_cursor.literal_service_sites=false;if(policy==16)services.instruction_cursor.service_sites=wrong;
        if(policy==17)first=true;if(policy==18)second=true;
        if(policy==19){const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw));}
        if(policy==20)_mm_setcsr(0x1f00);
        const auto before=packet(abi);const auto host=native_raw_x87_detail::capture();owner::activity()={};memory.pc=0xdeadbeefu;
        check(!owner::run(memory,call,abi,services,observe),"Unqualified getter input was handled policy="+std::to_string(policy));
        check(packet(abi)==before&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Getter refusal changed source/host state");
        check(!memory.attempts&&!memory.writes&&!callbacks&&!observers&&!required&&memory.pc==0xdeadbeefu&&!owner::activity().native_entries&&!owner::activity().reference_entries,"Getter refusal performed IO or invoked callbacks");++tested;
    }
    reset_host();return tested;
}
int main(int argc,char**argv)try{
    if(argc==2&&std::string_view(argv[1])=="--guards"){std::cout<<J{{"passed",true},{"refusals",guards()}}.dump()<<'\n';return 0;}
    check(argc==2,"Supply unchanged-original getter fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
    unsigned comparisons{},packets{},reads{},failures{},callbacks_total{},observer_calls{},required_calls{};
    for(const auto&row:fixture.at("cases"))for(bool empty:{false,true})for(bool hostile:{false,true}){
        reset_host();Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
        memory.fail=row.value("read_failure",false);memory.hostile=hostile;
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");abi.fs_base=row.value("fs_base",0u);
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}memory.abi=&abi;
        unsigned callbacks{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        SourcePhysicsWorldRuntimeServices services;
        if(!empty){services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,false};}
        services.required_owner=[&](auto&,const auto&,auto&,const auto&){++required_calls;throw std::runtime_error("Getter invoked required owner");};
        SourcePhysicsContactRecordObserver observe=[&](const auto&){++observer_calls;throw std::runtime_error("Getter invoked unused store observer");};
        bool failed{};owner::activity()={};
        try{check(owner::run(memory,{0x30ae24u,owner::entry,false},abi,services,observe),"Genuine getter public owner refused");}catch(const ReadFailure&){failed=true;++failures;}
        check(failed==memory.fail,"Getter actual read-failure ownership differs");
        auto expected=J{{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"fs_base",row.value("fs_base",0u)},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")},{"x87_depth",row.at("live_x87").size()},{"active_mmx",nullptr}};
        check(packet(abi)==expected,"Getter final complete ABI differs case="+row.at("name").get<std::string>());
        auto expected_packet=row.at("read_packet");expected_packet.erase("entry");expected_packet["fs_base"]=row.value("fs_base",0u);expected_packet["x87_depth"]=row.at("live_x87").size();expected_packet["active_mmx"]=nullptr;
        check(memory.packets==J::array({expected_packet}),"Getter referenced ABI was not published before its real read");
        auto actual_reads=memory.reads,expected_reads=row.at("reads");if(empty){for(auto*events:{&actual_reads,&expected_reads})for(auto&event:*events)event.erase("instruction");}
        check(actual_reads==expected_reads&&!memory.writes,"Getter ordered read/store differs");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),"Getter bank changed");
        const auto activity=owner::activity();check(owner::reference()?activity.reference_entries==1&&activity.retained_statements==2&&!activity.native_entries:activity.native_entries==1&&!activity.reference_entries&&!activity.retained_statements,"Getter native/reference ownership changed");
        check(callbacks==(owner::reference()&&!empty?2u:0u),"Getter callback ownership changed");callbacks_total+=callbacks;++comparisons;packets+=memory.packets.size();reads+=memory.reads.size();
    }
    check(!observer_calls&&!required_calls,"Childless getter called a provider/observer");
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"referenced_read_packets",packets},{"committed_reads",reads},{"actual_provider_read_failures",failures},{"callbacks",callbacks_total},{"reference",owner::reference()},{"retained_statements_per_handled_reference",owner::reference()?2:0},{"store_observer_invocations",observer_calls},{"required_owner_invocations",required_calls}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
