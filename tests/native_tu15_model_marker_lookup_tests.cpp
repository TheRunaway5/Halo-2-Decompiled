#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
using J=nlohmann::json;
namespace {
struct Fault{};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>bytes(const std::string&s){
    std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;
}
std::string hex(std::span<const std::byte>v){
    constexpr char digits[]="0123456789abcdef";std::string s;
    for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=digits[n>>4u];s+=digits[n&15u];}return s;
}
void hostile(){
    const std::uint16_t cw=0x37e;
    asm volatile("fninit;fldz;fldz;fdivp;fldcw %0"::"m"(cw):"memory","st","st(1)");_mm_setcsr(0x8065);
}
J packet(const SourcePhysicsWorldRuntimeAbi&a,std::uint32_t pc){
    J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    J result{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},
        {"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
    if(a.active_mmx){
        J physical=J::array();unsigned tags{};
        for(unsigned n=0;n<8;++n){const auto&raw=a.active_mmx->physical[n];physical.push_back(hex(raw));tags|=native_raw_x87_detail::value_tag(raw.data())<<(n*2u);}
        result["active_mmx"]=J{{"tag_word",tags},{"physical",physical},{"known_mask",a.active_mmx->known_mask}};
    }
    return result;
}
J packet(const State&s,std::uint32_t pc){return packet(s.save(),pc);}
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::set<std::uint32_t>write_sites;std::function<J()>capture;
    mutable std::uint32_t pc{};mutable unsigned read_count{},write_count{};
    mutable J reads=J::array(),writes=J::array(),packets=J::array();std::optional<J>fault;bool noisy{};
    std::byte*at(std::uint32_t address,std::size_t size)const{
        auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unbacked marker lookup address");--it;
        check(std::uint64_t(address-it->first)+size<=it->second.size(),"Marker lookup crossed bank extent");
        return const_cast<std::byte*>(it->second.data())+(address-it->first);
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(!write_sites.contains(pc)){
            packets.push_back(capture());
            if(fault&&fault->at("phase")=="before_read"&&read_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
            ++read_count;reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});
        }
        std::memcpy(out.data(),at(address,out.size()),out.size());if(noisy)hostile();
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        packets.push_back(capture());
        if(fault&&fault->at("phase")=="before_write"&&write_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
        const auto before=hex(std::span(at(address,in.size()),in.size()));std::memcpy(at(address,in.size()),in.data(),in.size());
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",before},{"written",hex(in)}});++write_count;if(noisy)hostile();
    }
};
unsigned guards(bool forced=false){
    unsigned checks{};constexpr std::array good{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f590u,0x1e11c1u};
    for(unsigned route=0;route<2;++route)for(unsigned policy=0;policy<(forced?1u:18u);++policy){
        Memory memory;unsigned callbacks{},stores{};bool trace{};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto){++callbacks;};
        services.instruction_cursor={&memory.pc,&trace,nullptr,good,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observe;
        if(policy==10)observe=[&](const auto&){++stores;};
        State state{memory,observe,services,!forced&&policy==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1,2,3,4,0x62000e00,6,7,8};abi.eflags=0x246;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(abi);
        switch(policy){
        case 0:if(route&&!forced)abi.floating_point.reset();break;
        case 1:services.instruction={};break;
        case 2:services.instruction=[](std::uint32_t){};break;
        case 3:services.instruction_cursor.position=nullptr;break;
        case 4:services.instruction_cursor.callback_type=&typeid(int);break;
        case 5:services.instruction_cursor.executor={};break;
        case 6:services.instruction_cursor.literal_service_sites=false;break;
        case 7:services.instruction_cursor.service_sites=wrong;break;
        case 8:trace=true;break;
        case 9:services.instruction={};services.instruction_cursor={};if(!route)state.instruction_cursor=&services.instruction_cursor;else abi.floating_point.reset();break;
        case 10:services.instruction_cursor.observation_only_stores=false;break;
        case 11:state.fp.count=9;abi.x87_depth=9;break;
        case 12:state.fp.status|=0x80;abi.x87_status|=0x80;break;
        case 13:state.fp.status|=0x8000;abi.x87_status|=0x8000;break;
        case 14:state.active_mmx=SourcePhysicsMmxAlias{};state.fp.count=1;abi.active_mmx=SourcePhysicsMmxAlias{};abi.x87_depth=1;break;
        case 15:if(route)abi.floating_point->x87_control_word=0x37e;else{const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw):"memory");}break;
        case 16:if(route)abi.floating_point->mxcsr=0x1f00;else _mm_setcsr(0x1f00);break;
        case 17:services.instruction_cursor.callback_type=nullptr;break;
        }
        memory.pc=0xdeadbeefu;const auto gp=state.gp;const auto flags=state.other_flags|state.flags.bits();const auto input=abi;
        const auto depth=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
        std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);
        const auto host=native_raw_x87_detail::capture();
        const bool accepted=route?native_model_marker_lookup_detail::effect(memory,abi,services,observe):native_model_marker_lookup_detail::bridge(state,native_model_marker_lookup_detail::Observation::actual);
        check(!accepted,"Unqualified marker owner admitted");
        check(state.gp==gp&&(state.other_flags|state.flags.bits())==flags&&abi.gp==input.gp&&abi.eflags==input.eflags,"Marker refusal changed GP/flags");
        check(state.fp.count==depth&&state.fp.top==top&&state.fp.status==status&&!std::memcmp(xmm.data(),state.xmm,128)&&abi.xmm==input.xmm&&abi.x87_depth==input.x87_depth&&abi.x87_status==input.x87_status&&abi.floating_point.has_value()==input.floating_point.has_value(),"Marker refusal changed FP/SIMD metadata");
        check(memory.pc==0xdeadbeefu&&memory.reads.empty()&&memory.writes.empty()&&!callbacks&&!stores&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Marker refusal performed I/O or changed native FP");
        ++checks;asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return checks;
}
}
int main(int argc,char**argv)try{
    if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guard")){
        std::cout<<J{{"passed",true},{"untouched_refusals",guards(std::string_view(argv[1])=="--reference-guard")}}.dump()<<'\n';return 0;
    }
    check(argc==2,"Supply original marker lookup fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
    unsigned comparisons{},packets{};
    for(const auto&row:fixture.at("cases"))for(unsigned mode=0;mode<4;++mode){
        if(mode==3&&row.at("image")!="TitleUpdate15")continue;
        const bool observer_fault=row.contains("fault")&&row.at("fault").at("phase")=="after_write";
        if(observer_fault&&mode<2)continue;
        Memory memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        for(auto pc:row.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
        memory.noisy=mode==1||mode==3;if(row.contains("fault"))memory.fault=row.at("fault");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
        unsigned callbacks{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[&](auto pc){++callbacks;memory.pc=pc;};
        services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(mode>=2)observe=[&](const auto&){
            if(observer_fault&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};hostile();
        };
        const auto image=row.at("image")=="TitleUpdate15"?SourcePhysicsContactImage::TitleUpdate15:SourcePhysicsContactImage::MapPack11;
        State state{memory,observe,services,image};SourcePhysicsWorldRuntimeAbi input{};
        input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");
        input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];
        input.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<input.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}
        if(row.contains("active_mmx")){
            SourcePhysicsMmxAlias alias{};alias.known_mask=row.at("active_mmx").at("known_mask");
            for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(alias.physical[n].data(),raw.data(),10);}input.active_mmx=alias;
        }
        state.load(input);auto effect_abi=input;
        memory.capture=[&]{return mode==3?packet(effect_abi,memory.pc):packet(state,memory.pc);};
        const auto observation=row.at("observation")=="actual"?native_model_marker_lookup_detail::Observation::actual:native_model_marker_lookup_detail::Observation::canonical;
        bool stopped{};try{
            if(mode==3)check(native_model_marker_lookup_detail::effect(memory,effect_abi,services,observe),"Real Effect ABI candidate refused");
            else native_model_marker_lookup_detail::run_private(state,[&](auto&,auto pc){memory.pc=pc;if(mode==1)hostile();},observation);
        }catch(const Fault&){stopped=true;}
        if(mode==3)state.load(effect_abi);
        const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);
        check(stopped==row.contains("fault"),name+": fault completion differs");
        check(!callbacks,name+": native delivered an instruction callback");
        const auto actual=packet(state,memory.pc);const auto&expected=row.at("expected");
        check(actual==expected,name+": final ABI differs actual="+actual.dump()+" expected="+expected.dump());
        check(memory.packets.size()==row.at("calls").size(),name+": pre-I/O packet count differs");
        for(unsigned n=0;n<memory.packets.size();++n)check(memory.packets[n]==row.at("calls")[n],name+": pre-I/O packet "+std::to_string(n)+" differs actual="+memory.packets[n].dump()+" expected="+row.at("calls")[n].dump());
        check(memory.reads==row.at("reads"),name+": ordered reads differ");check(memory.writes==row.at("writes"),name+": ordered stores differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backed bytes differ");
        ++comparisons;packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
