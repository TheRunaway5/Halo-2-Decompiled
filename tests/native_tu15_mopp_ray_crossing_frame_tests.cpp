#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_mopp_ray_plane_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_branch_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_crossing_frame.hpp"
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
    asm volatile("fninit; fld1; fldz; fdivp; fld1":::"memory","st","st(1)");
    const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065);
}
J packet(const WorldState&s,std::uint32_t pc){
    const auto a=s.save();J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t pc{};mutable J operations=J::array();J writes=J::array();
    int read_fault{-1},write_fault{-1};mutable unsigned read_count{};unsigned write_count{};bool observer_preimages{},hostile_fp{};
    auto at(std::uint32_t address,std::size_t size)const{
        auto i=banks.upper_bound(address);check(i!=banks.begin(),"Missing ray caller bank");--i;
        check(std::uint64_t(address-i->first)+size<=i->second.size(),"Ray caller bank extent");return i;
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
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
    check(argc==2,"Supply physical plane fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong plane original");
    const auto sites=fixture.at("io_sites").get<std::set<std::uint32_t>>();unsigned comparisons{},packets{};
    for(const auto&row:fixture.at("cases"))for(bool hostile:{false,true})for(bool observe:{false,true}){
        const auto phase=row.at("phase").get<std::string>();
        if(phase!="crossing_boundary"&&observe!=(phase=="after_write"))continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.observer_preimages=observe;memory.hostile_fp=hostile;
        if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
        if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned observed_writes{};
        if(observe)observer=[&](const auto&event){
            const auto&expected=row.at("writes").at(observed_writes);
            check(event.instruction==expected.at("instruction")&&event.address==expected.at("address")&&
                  hex(event.before)==expected.at("before").template get<std::string>()&&
                  hex(event.written)==expected.at("written").template get<std::string>(),"Plane observer differs");
            const auto ordinal=observed_writes++;if(hostile)clobber_fp();
            if(phase=="after_write"&&ordinal==row.at("native_fault_ordinal").get<unsigned>())throw Boundary{};
        };
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
        incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");
        incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
        incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<incoming.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],v.data(),10);}state.load(incoming);
        J observed=J::array();auto before=[&](auto&s,std::uint32_t pc){
            memory.pc=pc;if(pc==0x30ae02u||pc==0x30af16u||pc==0x30b06fu)throw Boundary{};
            if(sites.contains(pc))observed.push_back(packet(s,pc));if(hostile)clobber_fp();
        };
        bool interrupted{},alignment_fault{};std::uint32_t fault_flags{};
        try{
            native_raw_x87_detail::Scope scope{state};native_mopp_ray_caller_detail::PlaneFrame frame{state,before,scope};
            const auto entry=frame.begin();check(entry.valid_program_class,"Invalid plane opcode");
            frame.publish_program_class(native_mopp_aabb_abi_detail::source_class(entry.opcode));
            native_mopp_ray_plane_detail::Progress progress;auto program=entry.program;
            check(native_mopp_ray_plane_detail::prepare(frame,program,entry.opcode,progress),"Wrong plane program");
            native_mopp_ray_caller_detail::BranchFrame branch{state,before,scope};
            const auto result=native_mopp_ray_branch_detail::choose(branch,program);
            if(result==native_mopp_ray_branch_detail::Decision::continue_program)branch.before_continuation(result);
            else{
                native_mopp_ray_caller_detail::CrossingFrame crossing{state,before,scope};
                native_mopp_ray_crossing_detail::Progress cross;
                native_mopp_ray_crossing_detail::prepare_child(crossing,cross);crossing.before_recursion(cross);
            }
        }catch(const Boundary&){interrupted=true;}
        catch(const native_mopp_ray_caller_detail::AlignedLoadFault&fault){
            check(phase=="alignment_fault"&&fault.instruction==row.at("expected_pc")&&fault.address==row.at("alignment_fault_address"),"Unexpected crossing aligned-load boundary");
            check(fault.execution_eflags==(state.other_flags|state.flags.bits())&&fault.execution_eflags==row.at("execution_eflags_before_fault"),"Crossing aligned-load changed execution flags");
            alignment_fault=true;fault_flags=fault.saved_eflags;interrupted=true;
        }
        check(interrupted,"Plane did not reach declared boundary");
        const auto name=row.at("name").get<std::string>();
        check(observed==row.at("calls"),name+": plane complete packets differ actual="+observed.dump()+" expected="+row.at("calls").dump());
        const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("boundary_live_x87")}};
        auto actual=packet(state,memory.pc);if(alignment_fault)actual["eflags"]=fault_flags;
        check(actual==expected,name+": plane boundary ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.architectural_reads()==row.at("reads")&&memory.writes==row.at("writes"),name+": plane ordered IO differs");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": plane bank differs");
        ++comparisons;packets+=observed.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"production_binding",false},{"scope","Native crossing preparation; before next opcode or real recursive child setup"}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
