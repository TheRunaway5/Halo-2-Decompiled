#define main unused_arena_physical_main
#include "native_tu15_arena_initialize_frame_tests.cpp"
#undef main
#include <openssl/evp.h>
namespace {
std::function<void(State&,std::uint32_t,std::uint32_t,bool)>source_child;
std::string bank_digest(std::span<const std::byte>data){std::array<unsigned char,EVP_MAX_MD_SIZE>out{};unsigned count{};check(EVP_Digest(data.data(),data.size(),out.data(),&count,EVP_sha256(),nullptr)==1,"Source transport SHA failed");return hex(std::span(reinterpret_cast<const std::byte*>(out.data()),count));}
struct SourceMemory:Memory {
    mutable unsigned preimage_count{},read_count{};mutable J preimages=J::array();J observers=J::array();
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(write_sites.contains(pc)){
            preimages.push_back(capture());
            if(fault&&fault->at("phase")=="before_preimage"&&preimage_count==fault->at("ordinal"))throw Fault{};
            ++preimage_count;
        }else{
            if(fault&&fault->at("phase")=="before_read"&&read_count==fault->at("ordinal")){packets.push_back(capture());throw Fault{};}
            ++read_count;
        }
        Memory::read(address,out);
    }
};
}
namespace source_reference {
using namespace halo2::physics_world_runtime_detail;using WorldState=::State;
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){source_child(s,target,site,tail);}
void run_217090(WorldState&s){source_child(s,0x21b990u,0x1252bau,false);}
void run_1656a0(WorldState&s){source_child(s,0x166a80u,0,false);}
#include "arena_source_transport.inc"
}
namespace {
J run_source(const J&row,const J&write_sites,bool native,bool observed,const std::optional<J>&fault){
    SourceMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
    for(auto pc:write_sites)memory.write_sites.insert(pc.get<std::uint32_t>());memory.fault=fault;
    unsigned callbacks{};std::uint32_t child_entry{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    State*current{};SourcePhysicsContactRecordObserver observer;
    if(observed)observer=[&](const auto&){memory.observers.push_back(packet(*current,memory.pc));if(fault&&fault->at("phase")=="after_write"&&memory.write_count-1==fault->at("ordinal"))throw Memory::Fault{};};
    State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;SourcePhysicsWorldRuntimeAbi input{};
    input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}state.load(input);
    memory.capture=[&]{auto captured=packet(state,memory.pc);captured["arguments"]=J::array();return captured;};
    auto before=[&](auto&,auto pc){memory.pc=pc;};auto child=[&](auto&,std::uint32_t target,std::uint32_t site,bool tail){check(target==0x21b920u&&site==0x1252a4u&&!tail,"Source provider prelude reached unexpected child");child_entry=target;memory.pc=target;throw ChildEntryStop{};};source_child=child;
    bool faulted{};try{
        const bool rep=row.value("phase",std::string{})=="rep";
        if(native){
            if(rep){native_raw_x87_detail::Scope scope{state};native_arena_initialize_detail::Frame frame{state,before,child,scope,native_arena_initialize_detail::Operands{true,true}};native_arena_initialize_detail::clear_words(frame,input.gp[1]);}
            else (void)native_arena_initialize_detail::run_private(state,before,child,{true,true});
        }else if(rep)source_reference::rep_statement(state);else source_reference::run_125280(state);
    }catch(const ChildEntryStop&){}catch(const Memory::Fault&){faulted=true;}
    check(faulted==bool(fault),"Source provider failure was not actually reached");check(native?callbacks==0:callbacks>0,"Source native/reference statement ownership changed");
    J banks=J::array();for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"bytes",data.size()},{"sha256",bank_digest(data)}});
    return J{{"abi",packet(state,memory.pc)},{"reads",memory.reads},{"writes",memory.writes},{"packets",memory.packets},{"preimages",memory.preimages},{"observers",memory.observers},{"banks",banks},{"faulted",faulted},{"genuine_child_entry",child_entry}};
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply source arena transport inputs");std::ifstream file(argv[1]);const auto fixture=J::parse(file);unsigned comparisons{},packets{},preimages{},observer_calls{},prefixes{};
    for(const auto&row:fixture.at("cases"))for(bool observed:{false,true}){
        auto compare=[&](const std::optional<J>&fault){
            const auto reference=run_source(row,fixture.at("write_sites"),false,observed,fault);const auto native=run_source(row,fixture.at("write_sites"),true,observed,fault);
            for(const auto*key:{"abi","reads","writes","packets","preimages","observers","banks","faulted","genuine_child_entry"})check(native.at(key)==reference.at(key),row.at("name").get<std::string>()+" source transport key="+key+" fault="+(fault?fault->dump():"none")+" actual="+native.at(key).dump()+" expected="+reference.at(key).dump());
            ++comparisons;packets+=native.at("packets").size();preimages+=native.at("preimages").size();observer_calls+=native.at("observers").size();if(fault)++prefixes;return reference;
        };
        const auto baseline=compare(std::nullopt);
        for(const auto&[phase,key]:std::array<std::pair<const char*,const char*>,4>{{{"before_read","reads"},{"before_write","writes"},{"before_preimage","preimages"},{"after_write","writes"}}}){
            if(!observed&&(std::string_view(phase)=="before_preimage"||std::string_view(phase)=="after_write"))continue;
            const auto count=baseline.at(key).size();if(!count)continue;
            for(auto ordinal:std::set<std::size_t>{0,count/2,count-1})compare(J{{"phase",phase},{"ordinal",ordinal}});
        }
    }
    std::cout<<J{{"passed",true},{"source_provider_comparisons",comparisons},{"source_provider_prefixes",prefixes},{"full_pre_io_packets",packets},{"full_preimage_packets",preimages},{"full_observer_packets",observer_calls},{"native_instruction_callbacks",0},{"scope","Actual pinned compiled source provider/observer partial ABI; separate from unchanged-i386 instruction-prefix publication."}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
