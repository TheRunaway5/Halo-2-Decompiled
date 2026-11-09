#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_camera_collision_publication.hpp"
#include "halo2/native_tu15_camera_collision_publication_value.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace {
struct Boundary {};
void check(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto value=std::to_integer<unsigned>(b);out+=digits[value>>4];out+=digits[value&15];}return out;}
void clobber_fp(){
    asm volatile("fninit; fld1; fldz; fdivp; fld1;pxor %%mm0,%%mm0;pxor %%mm1,%%mm1;pxor %%mm2,%%mm2;pxor %%mm3,%%mm3;pxor %%mm4,%%mm4;pxor %%mm5,%%mm5;pxor %%mm6,%%mm6;pxor %%mm7,%%mm7":::"memory","st","st(1)","mm0","mm1","mm2","mm3","mm4","mm5","mm6","mm7");
    const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065);
}
J packet(const WorldState&s,std::uint32_t pc){
    const auto a=s.save();J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    J result{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};if(a.active_mmx){J physical=J::array();unsigned tag{};for(unsigned n=0;n<8;++n){const auto&raw=a.active_mmx->physical[n];physical.push_back(hex(raw));tag|=native_raw_x87_detail::value_tag(raw.data())<<(n*2u);}result["active_mmx"]=J{{"tag_word",tag},{"physical",physical},{"known_mask",a.active_mmx->known_mask}};}return result;
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t pc{};std::function<void()>before_read;mutable J operations=J::array();J writes=J::array();
    int read_fault{-1},write_fault{-1};mutable unsigned read_count{};unsigned write_count{};bool observer_preimages{},hostile_fp{};
    auto at(std::uint32_t address,std::size_t size)const{
        auto i=banks.upper_bound(address);check(i!=banks.begin(),"Missing ray caller bank");--i;
        check(std::uint64_t(address-i->first)+size<=i->second.size(),"Ray caller bank extent");return i;
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(before_read)before_read();
        if(int(read_count++)==read_fault){if(hostile_fp)clobber_fp();throw Boundary{};}
        operations.push_back(J{{"kind","read"},{"instruction",pc},{"address",address},{"bytes",out.size()}});
        auto i=at(address,out.size());std::copy_n(i->second.begin()+address-i->first,out.size(),out.begin());
        if(hostile_fp)clobber_fp();
    }
    // This fixture owns committed ordinary banks. The independent CPU oracle
    // records one sixteen-byte vector operand; device adapters retain their
    // separate four-scalar policy and need their own provider proof.
    void read_vector(std::uint32_t address,std::span<std::byte,16>out)const override{read(address,out);}
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        if(int(write_count++)==write_fault){if(hostile_fp)clobber_fp();throw Boundary{};}
        auto i=at(address,data.size());auto&bank=banks.at(i->first);
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",hex(std::span(bank).subspan(address-i->first,data.size()))},{"written",hex(data)}});
        operations.push_back(J{{"kind","write"},{"instruction",pc},{"address",address},{"bytes",data.size()}});
        std::copy(data.begin(),data.end(),bank.begin()+address-i->first);
        if(hostile_fp)clobber_fp();
    }
    J architectural_reads()const{
        J out=J::array();
        for(std::size_t n=0;n<operations.size();++n){
            auto event=operations[n];if(event.at("kind")!="read")continue;
            if(observer_preimages&&n+1<operations.size()){
                const auto&next=operations[n+1];
                if(next.at("kind")=="write"&&event.at("instruction")==next.at("instruction")&&
                   event.at("address")==next.at("address")&&event.at("bytes")==next.at("bytes"))continue;
            }
            event.erase("kind");out.push_back(event);
        }
        return out;
    }
};
}
int main(int argc,char**argv)try{
    check(argc==2||argc==3,"Supply actual16B9A0 fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong publication original");
    const auto sites=fixture.at("io_sites").get<std::set<std::uint32_t>>();unsigned comparisons{},packets{},observer_events{};
    for(const auto&row:fixture.at("cases"))for(bool observed:{false,true})for(bool hostile:{false,true}){
        const auto phase=row.at("phase").get<std::string>();if(phase!="normal"&&observed!=(phase=="after_observer"))continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.hostile_fp=hostile;memory.observer_preimages=observed;
        if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
        if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");unsigned event_count{};
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        if(observed)observer=[&](const auto&event){++observer_events;if(phase=="after_observer"&&event_count++==row.at("native_fault_ordinal").get<unsigned>()){if(hostile)clobber_fp();throw Boundary{};}if(hostile)clobber_fp();};
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
        incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<incoming.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],value.data(),10);}
        if(row.contains("active_mmx")){SourcePhysicsMmxAlias alias{};alias.known_mask=row.at("active_mmx").at("known_mask");for(unsigned n=0;n<8;++n){const auto value=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(alias.physical[n].data(),value.data(),10);}incoming.active_mmx=alias;}
        state.load(incoming);J observed_packets=J::array();
        auto before=[&](auto&s,std::uint32_t pc){memory.pc=pc;if(sites.contains(pc))observed_packets.push_back(packet(s,pc));if(hostile)clobber_fp();};
        bool interrupted{};try{native_camera_collision_publication_detail::run_private(state,before);}catch(const Boundary&){interrupted=true;}check(interrupted==!row.at("returned").get<bool>(),"Publication completion/fault differs");
        const auto name=row.at("name").get<std::string>();
        check(observed_packets==row.at("calls"),name+": publication complete packets differ actual="+observed_packets.dump()+" expected="+row.at("calls").dump());
        auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("boundary_live_x87")}};if(row.contains("active_mmx"))expected["active_mmx"]=row.at("boundary_active_mmx");
        check(packet(state,memory.pc)==expected,name+": publication final ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.architectural_reads()==row.at("reads")&&memory.writes==row.at("writes"),name+": publication ordered IO differs");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": publication bank differs");
        ++comparisons;packets+=observed_packets.size();
    }
    const auto report=J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"observer_events",observer_events},{"production_binding",false}};
    std::cout<<report.dump()<<'\n';if(argc==3){std::ofstream out(argv[2]);out<<report.dump()<<'\n';check(bool(out),"Publication report failed");}return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
