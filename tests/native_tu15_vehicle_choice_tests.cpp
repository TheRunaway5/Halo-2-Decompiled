#include "gameplay/objects/native_tu15_vehicle_choice.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace copies {
#include "vehicle_choice_bodies.inc"
}
namespace {
std::string hex(std::span<const std::byte>raw){constexpr char h[]="0123456789abcdef";std::string s;for(auto b:raw){auto x=std::to_integer<unsigned>(b);s+=h[x>>4];s+=h[x&15];}return s;}
std::vector<std::byte>bytes(const std::string&text){std::vector<std::byte>v(text.size()/2);for(unsigned n=0;n<v.size();++n)v[n]=std::byte(std::stoul(text.substr(n*2,2),nullptr,16));return v;}
struct Stop{};
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>>banks;
    WorldState*state{};std::uint32_t pc{};mutable J reads=J::array(),writes=J::array(),calls=J::array();
    std::string phase;unsigned ordinal{},read_count{},write_count{},mutable_banks{};
    std::span<std::byte>view(std::uint32_t at,unsigned count){for(auto&[base,raw]:banks)if(at>=base&&std::uint64_t(at-base)+count<=raw.size())return std::span(raw).subspan(at-base,count);throw std::runtime_error("Vehicle choice escaped declared bank "+std::to_string(at));}
    void packet()const {
        if(!state)throw std::runtime_error("Refused vehicle route accessed memory");
        const auto a=state->save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
        calls.push_back(J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}});
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override {
        auto&self=*const_cast<Memory*>(this);packet();
        const bool retained=std::any_of(banks.begin(),banks.begin()+mutable_banks,[&](const auto&b){return at>=b.first&&std::uint64_t(at-b.first)+out.size()<=b.second.size();});
        if(retained&&phase=="before_read"&&self.read_count++==ordinal)throw Stop{};
        std::ranges::copy(self.view(at,out.size()),out.begin());reads.push_back(J{{"instruction",pc},{"address",at},{"bytes",out.size()}});
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override {
        packet();if(phase=="before_write"&&write_count==ordinal)throw Stop{};
        const auto old=hex(view(at,in.size()));std::ranges::copy(in,view(at,in.size()).begin());
        writes.push_back(J{{"instruction",pc},{"address",at},{"before",old},{"written",hex(in)}});++write_count;
    }
};
void admission_controls() {
    if(native_vehicle_choice_detail::reference_mode())return;
    Memory memory;SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};
    const auto refuses=[&](SourcePhysicsContactImage image,const SourcePhysicsContactRecordObserver&observe={}){
        WorldState state{memory,observe,services,image};
        if(native_vehicle_choice_detail::bridge(state,NativeTu15VehicleChoiceFamily::light))throw std::runtime_error("Unsafe vehicle DATA admission");
    };
    refuses(SourcePhysicsContactImage::MapPack11);
    services.instruction=[&](auto){++callbacks;};refuses(SourcePhysicsContactImage::TitleUpdate15);
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};std::uint32_t pc{};auto&c=services.instruction_cursor;
    c.service_sites=sites;c.callback_type=&services.instruction.target_type();c.executor=std::this_thread::get_id();c.literal_service_sites=true;c.observation_only_stores=true;
    refuses(SourcePhysicsContactImage::TitleUpdate15);c.position=&pc;
    c.executor={};refuses(SourcePhysicsContactImage::TitleUpdate15);c.executor=std::this_thread::get_id();
    c.callback_type=&typeid(int);refuses(SourcePhysicsContactImage::TitleUpdate15);c.callback_type=&services.instruction.target_type();
    bool trace=true;c.first_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15);c.first_trace=nullptr;
    c.second_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15);c.second_trace=nullptr;
    c.observation_only_stores=false;refuses(SourcePhysicsContactImage::TitleUpdate15,[](const auto&){});c.observation_only_stores=true;
    constexpr std::array conflict{0x38846eu,0x15cbbau,0x33b595u};c.service_sites=conflict;refuses(SourcePhysicsContactImage::TitleUpdate15);c.service_sites=sites;
    services.instruction={};refuses(SourcePhysicsContactImage::TitleUpdate15,[](const auto&){});
    if(callbacks)throw std::runtime_error("Admission replaced or ran a genuine callback");
}
}
int main(int argc,char**argv)try {
    if(argc!=3)throw std::runtime_error("Supply vehicle inputs/output");admission_controls();std::ifstream input(argv[1]);const auto rows=J::parse(input);J results=J::array();
    for(const auto&row:rows) {
        Memory m;for(const auto&b:row["banks"])m.banks.emplace_back(b["address"],bytes(b["before"]));m.mutable_banks=m.banks.size();
        // Code-section jump tables are real immutable image operands, supplied
        // separately from mutable banks in both native and physical fixtures.
        for(const auto&b:row["tables"])m.banks.emplace_back(b["address"],bytes(b["before"]));
        const auto entry=row["entry"].get<std::uint32_t>();const auto family=entry==0x15cb60?NativeTu15VehicleChoiceFamily::light:
            entry==0x15cc70?NativeTu15VehicleChoiceFamily::heavy:entry==0x15cdf0?NativeTu15VehicleChoiceFamily::turret:NativeTu15VehicleChoiceFamily::flying;
        SourcePhysicsContactRuntimeAbi abi{};abi.gp=row["gp"].get<decltype(abi.gp)>();abi.eflags=row["eflags"];abi.xmm=row["xmm"].get<decltype(abi.xmm)>();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row["before_fp"][0].get<std::uint16_t>(),row["before_fp"][2].get<std::uint32_t>()};abi.x87_status=row["before_fp"][1];
        abi.x87_depth=row["live_x87"].size();for(unsigned n=0;n<abi.x87_depth;++n){const auto b=bytes(row["live_x87"][n]);std::memcpy(&abi.x87[n],b.data(),10);}
        if(row.contains("fault")){m.phase=row["fault"]["phase"];m.ordinal=row["fault"]["ordinal"];}
        unsigned callbacks{};J trace=J::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){++callbacks;m.pc=pc;trace.push_back(pc);};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&c=services.instruction_cursor;
        c.position=&m.pc;c.service_sites=sites;c.callback_type=&services.instruction.target_type();c.executor=std::this_thread::get_id();c.literal_service_sites=true;c.observation_only_stores=true;
        bool generic=row.value("generic",false);if(generic)c.first_trace=&generic;
        SourcePhysicsContactRecordObserver observe;
        const bool observer=row.value("observer",false);
        if(observer)observe=[&](const auto&){if(m.phase=="after_write"&&m.write_count-1u==m.ordinal)throw Stop{};};
        WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);m.state=&state;
        const unsigned route=row.value("route",0u);bool completed=true;
        try{if(route==0){if(!native_vehicle_choice_detail::bridge(state,family))throw std::runtime_error("Qualified vehicle choice refused native route");}
            else if(route==1)copies::installed::run(entry,state);else if(route==2)copies::reference::run(entry,state);else throw std::runtime_error("Unknown vehicle test route");
        }catch(const Stop&){completed=false;}
        const auto a=state.save();if(a.x87_depth!=abi.x87_depth)throw std::runtime_error("Vehicle choice changed live x87 depth");
        for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&abi.x87[n],10))throw std::runtime_error("Vehicle choice changed live80 caller value");
        const bool reference=route==2||generic||native_vehicle_choice_detail::reference_mode();
        if(!reference&&callbacks)throw std::runtime_error("Vehicle kernel entered instruction callback");
        if(reference&&!callbacks)throw std::runtime_error("Vehicle reference path lost forwarded observations");
        J banks=J::array();for(unsigned n=0;n<row["banks"].size();++n)banks.push_back(J{{"address",m.banks[n].first},{"after",hex(m.banks[n].second)}});
        results.push_back(J{{"id",row["id"]},{"completed",completed},{"boundary_gp",a.gp},{"boundary_eflags",a.eflags},{"boundary_xmm",a.xmm},
            {"after_fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"banks",banks},{"reads",m.reads},{"writes",m.writes},{"calls",m.calls},{"pc",m.pc},{"callbacks",callbacks},{"trace",trace}});
    }
    std::ofstream output(argv[2]);output<<results.dump();return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
