#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "native_tu15_resource_composition_backend.hpp"
#include "halo2/native_tu15_rng_startup.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace {
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string&text){std::vector<std::byte> result;for(std::size_t n=0;n<text.size();n+=2)result.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return result;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string result;for(auto byte:data){auto n=std::to_integer<unsigned>(byte);result+=digits[n>>4];result+=digits[n&15];}return result;}
J abi_value(const SourcePhysicsWorldRuntimeAbi&a){
    J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex({reinterpret_cast<const std::byte*>(&a.x87[n]),10}));
    return J{{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"live_x87",live},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})}};
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable J reads=J::array();J writes=J::array();
    auto at(std::uint32_t p,std::size_t count)const{
        auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped composition read at"+std::to_string(p));--i;
        check(std::uint64_t(p-i->first)+count<=i->second.size(),"Composition access escaped bank at"+std::to_string(p));return i;
    }
    std::span<std::byte> view(std::uint32_t p,std::size_t count){const auto i=at(p,count);return std::span(banks.at(i->first)).subspan(p-i->first,count);}
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        const auto i=at(p,out.size());reads.push_back(J::array({p,out.size()}));std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override{
        auto out=view(p,in.size());writes.push_back(J::array({p,hex(out),hex(in)}));std::ranges::copy(in,out.begin());
    }
    void put(std::uint32_t at,std::uint32_t value){auto out=view(at,4);for(unsigned n=0;n<4;++n)out[n]=std::byte(value>>(n*8u));}
};
struct Run {
    Memory memory;resource_composition_fixture::FileBackend backend;
    SourceExecutionInputs&inputs;J owners=J::array();unsigned io{},clock{},callbacks{},literal_callbacks{};
    Run(const J&row,const char*file,SourceExecutionInputs&journal):backend(file,127u+row.at("id").get<unsigned>()),inputs(journal){
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    }
    void service(WorldState&state,std::uint32_t target,std::uint32_t site,bool tail){
        if(target!=0x1a3b70u)throw std::runtime_error("Unresolved actual resource owner target="+std::to_string(target)+" site="+std::to_string(site));
        auto a=state.save();
        check(source_try_animation_cache_io_tu15(memory,memory,{site,target,tail},a,std::this_thread::get_id(),
            [&](std::uint32_t descriptor){SourceRuntimeCacheAnimationImage resource;auto actual=memory.view(descriptor,20);std::ranges::copy(actual,resource.descriptor.begin());resource.bytes.resize(Bytes(actual).u32(4));return resource;},
            [&](std::uint32_t descriptor){return memory.view(descriptor,12);},
            [&](const auto&request){return backend.read(request);},[&](const auto&request){return backend.retain(request);}),"Actual synchronous World policy refused composition");
        ++io;state.load(a);
    }
};
thread_local Run*current{};
}
void before_owner(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){current->owners.push_back(J{{"target",target},{"site",site},{"tail",tail},{"abi",abi_value(s.save())}});}
void platform_owner(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){current->service(s,target,site,tail);}
#include "resource_composition_bodies.inc"
namespace {
SourcePhysicsWorldRuntimeAbi incoming(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");const auto fp=row.at("before_fp");
    a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};a.x87_status=fp[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}return a;
}
struct Outcome {J abi,banks,reads,writes,owners,jobs;unsigned io,clock,literal;std::uint64_t file_calls,file_bytes;};
Outcome execute(const J&row,const char*file,SourceExecutionInputs&journal,bool native){
    Run run(row,file,journal);current=&run;std::uint32_t pc{};SourcePhysicsWorldRuntimeServices services;
    auto clock=native_tu15_rng_clock();
    services.instruction=[&](auto p){pc=p;++run.callbacks;if(p!=0x33b590u&&p!=0x33b595u&&p!=0x33b597u)++run.literal_callbacks;
        if(p==0x33b595u){++run.clock;run.memory.put(0x6100e000u,std::uint32_t(journal.sample(SourceExecutionInputKind::Milliseconds,0,[&]{return clock.monotonic_milliseconds();})));}
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    const SourcePhysicsContactRecordObserver observe;WorldState state{run.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(incoming(row));
    auto call=[&](auto entry){if(native)resource_composition::native::dispatch(state,entry,0u,false);else resource_composition::reference::dispatch(state,entry,0u,false);};
    try{call(row.at("entry").get<std::uint32_t>());}catch(const std::exception&e){throw std::runtime_error(row.at("name").get<std::string>()+": "+e.what()+" lastpc="+std::to_string(pc));}
    Outcome result;result.abi=abi_value(state.save());result.reads=run.memory.reads;result.writes=run.memory.writes;result.owners=run.owners;result.banks=J::array();result.jobs=J::array();
    for(const auto&[address,bank]:run.memory.banks)result.banks.push_back(J::array({address,hex(bank)}));
    for(const auto&[job,request]:run.backend.retained.resource_reads)result.jobs.push_back(J::array({job,request.descriptor,request.destination,request.bytes}));
    result.io=run.io;result.clock=run.clock;result.literal=run.literal_callbacks;result.file_calls=run.backend.actual_file_calls;result.file_bytes=run.backend.actual_file_bytes;
    check(run.backend.retained.next_resource_read==0xf8000001ull+result.jobs.size(),"Registry counter lost actual map ownership");current=nullptr;return result;
}
}
int main(int argc,char**argv)try{
    const bool export_native=argc==4&&std::string(argv[1])=="--export",service_only=argc==4&&std::string(argv[1])=="--service";
    check(argc==3||export_native||service_only,"Supply backed resource fixture and actual cache file");const auto offset=(export_native||service_only)?1:0;
    std::ifstream input(argv[1+offset]);const auto fixture=J::parse(input);const auto file=argv[2+offset];
    if(service_only){
        J outputs=J::array();
        for(const auto&row:fixture.at("cases")){
            SourceExecutionInputs inputs;Run run(row,file,inputs);const SourcePhysicsWorldRuntimeServices services;const SourcePhysicsContactRecordObserver observe;
            WorldState state{run.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(incoming(row));run.service(state,0x1a3b70u,0x126139u,false);
            J banks=J::array();for(const auto&[address,bank]:run.memory.banks)banks.push_back(J{{"address",address},{"before",hex(bank)}});
            outputs.push_back(J{{"abi",abi_value(state.save())},{"banks",banks},{"reads",run.memory.reads},{"writes",run.memory.writes},
                {"actual_file_calls",run.backend.actual_file_calls},{"actual_file_bytes",run.backend.actual_file_bytes},{"retained_jobs",run.backend.retained.resource_reads.size()}});
        }
        std::cout<<outputs.dump()<<'\n';return 0;
    }
    if(export_native){
        J outputs=J::array();for(const auto&row:fixture.at("cases")){
            SourceExecutionInputs inputs;auto value=execute(row,file,inputs,true);check(inputs.finish().empty(),"Physical composition must split any real clock-service input separately");
            outputs.push_back(J{{"abi",value.abi},{"banks",value.banks},{"reads",value.reads},{"writes",value.writes},{"jobs",value.jobs},{"actual_io_jobs",value.io}});
        }
        std::cout<<outputs.dump()<<'\n';return 0;
    }
    unsigned cases{},io{},clock{};std::uint64_t file_calls{},file_bytes{},reads{},writes{};
    for(const auto&row:fixture.at("cases")){
        SourceExecutionInputs recording;const auto reference=execute(row,file,recording,false);const auto observed=recording.finish();std::vector<SourceExecutionInput>captured(observed.begin(),observed.end());
        SourceExecutionInputs replay{std::span<const SourceExecutionInput>(captured)};const auto native=execute(row,file,replay,true);(void)replay.finish();const auto name=row.at("name").get<std::string>();
        check(native.abi==reference.abi,name+": complete ABI differs "+native.abi.dump()+" expected "+reference.abi.dump());
        check(native.banks==reference.banks,name+": complete backed banks differ");
        if(native.reads!=reference.reads){std::size_t n{};while(n<native.reads.size()&&n<reference.reads.size()&&native.reads[n]==reference.reads[n])++n;throw std::runtime_error(name+": ordered reads differ at"+std::to_string(n)+" native="+(n<native.reads.size()?native.reads[n].dump():"end")+" reference="+(n<reference.reads.size()?reference.reads[n].dump():"end"));}
        check(native.writes==reference.writes,name+": ordered writes differ");
        check(native.jobs==reference.jobs&&native.io==row.at("expected_io_jobs").get<unsigned>()&&native.io==native.jobs.size(),name+": genuine file/job owner not reached");
        check(native.literal==0,name+": native route entered literal CPU body");check(reference.literal>0,name+": retained controller control was not executed");
        cases++;io+=native.io;clock+=native.clock;file_calls+=native.file_calls;file_bytes+=native.file_bytes;reads+=native.reads.size();writes+=native.writes.size();
    }
    std::cout<<J{{"passed",true},{"complete_canonical_comparisons",cases},{"actual_synchronous_io_jobs",io},{"native_service_clock_refreshes",clock},{"actual_file_calls",file_calls},{"actual_file_bytes",file_bytes},{"ordered_reads",reads},{"ordered_stores",writes},{"native_literal_callbacks",0}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
