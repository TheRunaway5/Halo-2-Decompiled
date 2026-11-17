#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/runtime/native_tu15_arena_initialize_frame.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
using namespace halo2;using J=nlohmann::json;using State=physics_world_runtime_detail::WorldState;
namespace {
void check(bool condition,const std::string&message){if(!condition)throw std::runtime_error(message);}
std::vector<std::byte>unhex(const std::string&text){std::vector<std::byte>bytes;for(unsigned n=0;n<text.size();n+=2)bytes.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return bytes;}
std::string hex(std::span<const std::byte>bytes){constexpr char digits[]="0123456789abcdef";std::string text;for(auto b:bytes){const auto value=std::to_integer<unsigned>(b);text+=digits[value>>4];text+=digits[value&15];}return text;}
J packet(State&state,std::uint32_t pc){
    const auto abi=state.save();J live=J::array();for(unsigned n=0;n<abi.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10)));
    return J{{"entry",pc},{"gp",abi.gp},{"eflags",abi.eflags},{"xmm",abi.xmm},{"fp",J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})},{"live_x87",live}};
}
void hostile(){const std::uint16_t cw=0x37eu;asm volatile("fninit;fldz;fldz;fdivp;fldcw %0"::"m"(cw):"memory","st","st(1)");_mm_setcsr(0x8065u);}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;std::uint32_t pc{};std::set<std::uint32_t>write_sites;
    std::function<J()>capture;mutable J reads=J::array(),writes=J::array(),packets=J::array();bool noisy{},bulk{};std::optional<J>fault;unsigned write_count{};
    struct Fault{};
    std::byte*at(std::uint32_t address,std::size_t bytes)const{
        auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unbacked private arena data");--it;
        check(std::uint64_t(address-it->first)+bytes<=it->second.size(),"Arena data exceeds owned bank");return const_cast<std::byte*>(it->second.data())+(address-it->first);
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(!write_sites.contains(pc)){if(!bulk)packets.push_back(capture());reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});}
        std::memcpy(out.data(),at(address,out.size()),out.size());if(noisy)hostile();
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        if(!bulk)packets.push_back(capture());if(fault&&fault->at("phase")=="before_write"&&write_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
        if(bulk&&pc==0x126678u){std::memcpy(at(address,in.size()),in.data(),in.size());++write_count;return;}
        const auto before=hex(std::span(at(address,in.size()),in.size()));std::memcpy(at(address,in.size()),in.data(),in.size());
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",before},{"written",hex(in)}});++write_count;if(noisy)hostile();
    }
};
struct ChildEntryStop{};
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply original-backed private arena fixture");std::ifstream input_file(argv[1]);const auto fixture=J::parse(input_file);unsigned comparisons{},packets{},returns{},entries{},savegames{},complete_repeats{},checksums{},repeats{},faults{};
    for(const auto&row:fixture.at("cases"))for(unsigned mode=0;mode<3;++mode){
        const bool bulk=row.value("phase",std::string{})=="bulk_rep";if(bulk&&mode)continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
        for(auto value:fixture.at("write_sites"))memory.write_sites.insert(value.get<std::uint32_t>());memory.noisy=mode==1;memory.bulk=bulk;
        if(row.contains("fault"))memory.fault=row.at("fault");
        unsigned callbacks{};bool trace{};constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observer;if(mode==2||(memory.fault&&memory.fault->at("phase")=="after_write"))observer=[&](const auto&){if(memory.fault&&memory.fault->at("phase")=="after_write"&&memory.write_count-1==memory.fault->at("ordinal"))throw Memory::Fault{};hostile();};
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};
        input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}state.load(input);
        memory.capture=[&]{auto result=packet(state,memory.pc);result["arguments"]=J::array();return result;};auto before=[&](auto&,auto pc){memory.pc=pc;if(mode==2)hostile();};
        auto child=[&](auto&,std::uint32_t target,std::uint32_t site,bool tail){
            const auto phase=row.value("phase",std::string{});const bool crc=phase=="tail"||phase=="metadata";
            const bool savegame=bulk||phase=="savegame";
            check(target==(crc?0x166a80u:savegame?0x21b990u:0x21b920u)&&site==(phase=="metadata"?0x1266adu:crc?0x1266e1u:savegame?0x12667au:0x126664u)&&!tail,"Unexpected private arena child transport");memory.pc=target;throw ChildEntryStop{};
        };
        bool stopped{},faulted{};try{
            if(row.value("phase",std::string{})=="savegame"){
                native_raw_x87_detail::Scope scope{state};native_arena_initialize_detail::Frame frame{state,before,child,scope};
                frame.initialize_savegames();
            }else if(row.value("phase",std::string{})=="reservation"){
                native_raw_x87_detail::Scope scope{state};native_arena_initialize_detail::Frame frame{state,before,child,scope};
                frame.backing_reserved();memory.pc=0x126678u;
            }else if(row.value("phase",std::string{})=="metadata"){
                native_raw_x87_detail::Scope scope{state};native_arena_initialize_detail::Frame frame{state,before,child,scope};
                native_arena_initialize_detail::initialize_metadata(frame);
            }else if(row.value("phase",std::string{})=="final"){
                native_raw_x87_detail::Scope scope{state};native_arena_initialize_detail::Frame frame{state,before,child,scope};
                (void)native_arena_initialize_detail::finish_root(frame);
            }else if(row.value("phase",std::string{})=="tail"){
                native_raw_x87_detail::Scope scope{state};native_arena_initialize_detail::Frame frame{state,before,child,scope};
                (void)native_arena_initialize_detail::finish_initialization(frame);
            }else if(row.value("phase",std::string{})=="rep"||bulk){
                native_raw_x87_detail::Scope scope{state};native_arena_initialize_detail::Frame frame{state,before,child,scope};
                native_arena_initialize_detail::clear_words(frame,input.gp[1]);
                if(bulk)frame.initialize_savegames();
            }else{const auto result=native_arena_initialize_detail::run_private(state,before,child);check(!result.initialized_now,"Cold arena completed without its actual owner");}
        }catch(const ChildEntryStop&){stopped=true;}catch(const Memory::Fault&){faulted=true;}
        const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);
        check(stopped==row.at("child_stop").get<bool>(),name+": genuine child boundary changed");
        check(faulted==row.contains("fault"),name+": original memory-prefix boundary changed");
        const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")}};
        check(packet(state,memory.pc)==expected,name+": final ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.packets.size()==row.at("calls").size(),name+": pre-I/O packet count differs");
        for(unsigned n=0;n<memory.packets.size();++n)check(memory.packets[n]==row.at("calls")[n],name+": packet"+std::to_string(n)+" differs actual="+memory.packets[n].dump()+" expected="+row.at("calls")[n].dump());
        check(memory.reads==row.at("reads")&&memory.writes==row.at("writes"),name+": actual read/store sequence differs");
        if(bulk)check(memory.write_count==0xff800u+1u,name+": complete authored REP count changed");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": bank bytes differ");
        check(callbacks==0,name+": private native path invoked instruction callback");++comparisons;packets+=memory.packets.size();if(faulted)++faults;else if(stopped){if(bulk||row.value("phase",std::string{})=="savegame"){++savegames;if(bulk)++complete_repeats;}else if(row.value("phase",std::string{})=="tail"||row.value("phase",std::string{})=="metadata")++checksums;else++entries;}else if(row.value("phase",std::string{})=="rep"||row.value("phase",std::string{})=="reservation")++repeats;else++returns;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"pre_io_packets",packets},{"completed_returns",returns},{"real_allocation_child_entries",entries},{"real_savegame_child_entries",savegames},{"real_crc_child_entries",checksums},{"complete_rep_words",std::uint64_t(complete_repeats)*0xff800u},{"declared_rep_continuations",repeats},{"memory_prefixes",faults},{"instruction_callbacks",0}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
