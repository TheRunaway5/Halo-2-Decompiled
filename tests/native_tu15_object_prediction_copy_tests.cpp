#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_prediction_copy.hpp"
#include "native_tu15_object_node_admission_checks.hpp"
#ifdef HALO2_OBJECT_PREDICTION_WITH_OWNER_DISPATCH
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#endif
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
#include "native_tu15_object_prediction_copy_reference.inc"
}
namespace {
constexpr std::array bases{0x61000000u,0x62000000u};
constexpr std::array sizes{65536u,16384u};
struct Stop {};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,2> banks;
    mutable std::vector<std::pair<std::uint32_t,unsigned>> reads;
    mutable unsigned read_ordinal{};unsigned write_ordinal{};
    int fail_read=-1,fail_write=-1;
    Memory(){for(unsigned n=0;n<banks.size();++n)banks[n].resize(sizes[n]);}
    std::span<std::byte> view(std::uint32_t at,unsigned size){
        for(unsigned n=0;n<banks.size();++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+size<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],size);
        throw std::runtime_error("Prediction copy escaped declared bank");
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override{
        if(int(read_ordinal++)==fail_read)throw Stop{};
        reads.emplace_back(at,out.size());std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override{
        if(int(write_ordinal++)==fail_write)throw Stop{};
        std::ranges::copy(in,view(at,in.size()).begin());
    }
    void put(std::uint32_t at,std::uint32_t value){auto bytes=view(at,4);for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8));}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{};
    unsigned callbacks{};std::vector<SourcePhysicsContactRecordStore> stores;
};
Result run(Memory &memory,const SourcePhysicsWorldRuntimeAbi &abi,bool native,int fail_observer=-1,bool owner=false){
    Result result;unsigned observed{};
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){result.stores.push_back(e);if(int(observed++)==fail_observer)throw Stop{};};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){result.pc=pc;++result.callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&result.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    std::uint64_t before{};
#ifdef HALO2_OBJECT_PREDICTION_WITH_OWNER_DISPATCH
    if(owner)before=native_allocator_owner_detail::activity().prediction_entries[0xadcd0u];
#endif
    try{if(native){bool accepted{};
#ifdef HALO2_OBJECT_PREDICTION_WITH_OWNER_DISPATCH
        if(owner)accepted=native_allocator_owner_detail::dispatch(state,0xadcd0u,[](auto&,auto,auto,auto){throw std::runtime_error("Prediction copy requested external outcome");});
        else
#endif
        accepted=native_object_prediction_copy_detail::bridge(state);
        if(!accepted)throw std::runtime_error("Qualified prediction copy candidate refused");
    }else reference::run_adcd0(state);}
    catch(const Stop&){result.stopped=true;}
#ifdef HALO2_OBJECT_PREDICTION_WITH_OWNER_DISPATCH
    if(owner&&native_allocator_owner_detail::activity().prediction_entries[0xadcd0u]!=before+1u)throw std::runtime_error("Actual prediction owner activity differs");
#endif
    result.abi=state.save();return result;
}
bool same_abi(const auto&a,const auto&b){
    bool same=a.gp==b.gp&&a.eflags==b.eflags&&a.xmm==b.xmm&&a.x87_depth==b.x87_depth&&a.x87_status==b.x87_status&&
        a.floating_point->mxcsr==b.floating_point->mxcsr&&a.floating_point->x87_control_word==b.floating_point->x87_control_word;
    for(unsigned n=0;n<a.x87_depth;++n)same=same&&!std::memcmp(&a.x87[n],&b.x87[n],10);
    return same;
}
void canonical_prefixes(bool owner=false){
    unsigned cases{},prefixes{};
    constexpr std::uint32_t sp=0x62002000u,source=0x61001000u;
    for(unsigned alias=0;alias<5;++alias)for(bool backwards:{false,true}){
        Memory seed;for(unsigned n=0;n<100;++n)seed.put(source+n*4,n*0x1234567u);
        const auto packet=alias==4?sp-0x78u:source;
        const auto output=alias==0?source+0x200u:alias==1?source+0x6cu:alias==2?sp-4u:alias==3?sp+4u:source+0x200u;
        seed.put(sp,0x63000000u);seed.put(sp+4,0x12345678u);seed.put(sp+8,packet);seed.put(sp+12,output);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678u,2u,3u,4u,sp,6u,7u,8u};abi.eflags=0x2d7u|(backwards?1024u:0u);
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};abi.x87_depth=2;abi.x87_status=0x4100;
        abi.x87[0]=1.234567890123456789L;abi.x87[1]=-9.876543210987654321L;
        for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x7f800001u+n*4u+k;
        auto a=seed,b=seed;const auto complete=run(a,abi,false),candidate=run(b,abi,true,-1,owner);
        const auto compare=[&](const auto&ref,const auto&got,const auto&left,const auto&right){
            if(!same_abi(ref.abi,got.abi)||ref.pc!=got.pc||ref.stopped!=got.stopped||left.banks!=right.banks||left.reads!=right.reads||ref.stores!=got.stores||got.callbacks)
                throw std::runtime_error("Prediction copy canonical prefix mismatch alias="+std::to_string(alias));
        };
        compare(complete,candidate,a,b);++cases;
        for(unsigned kind=0;kind<3;++kind){const auto count=kind==0?a.read_ordinal:kind==1?a.write_ordinal:unsigned(complete.stores.size());
            for(unsigned ordinal=0;ordinal<count;++ordinal){auto left=seed,right=seed;
                if(kind==0)left.fail_read=right.fail_read=int(ordinal);
                if(kind==1)left.fail_write=right.fail_write=int(ordinal);
                const auto ref=run(left,abi,false,kind==2?int(ordinal):-1),got=run(right,abi,true,kind==2?int(ordinal):-1,owner);
                compare(ref,got,left,right);if(!got.stopped)throw std::runtime_error("Missing retained prefix stop");++prefixes;
            }
        }
    }
    std::cerr<<"canonical_cases="<<cases<<" retained_prefixes="<<prefixes<<'\n';
}
std::uint32_t word(){std::uint32_t v{};std::cin.read(reinterpret_cast<char*>(&v),4);if(!std::cin)throw std::runtime_error("Truncated prediction copy fixture");return v;}
void emit(std::uint32_t v){std::cout.write(reinterpret_cast<const char*>(&v),4);}
}
int main(int argc,char**argv){try{
    const std::string_view command=argc==2?argv[1]:"";
    if(command=="--live-admission"||command=="--owner-live-admission"){
        const auto cases=object_node_test_detail::check_live_admission_fallbacks([&](auto&s){
#ifdef HALO2_OBJECT_PREDICTION_WITH_OWNER_DISPATCH
            if(command=="--owner-live-admission")return native_allocator_owner_detail::dispatch(s,0xadcd0u,[](auto&,auto,auto,auto){throw std::runtime_error("Rejected prediction owner requested outcome");});
#endif
            return native_object_prediction_copy_detail::bridge(s);
        },[](auto&s){reference::run_adcd0(s);});
        std::cout<<"live_protocol_fallbacks="<<cases<<'\n';return 0;
    }
    if(command=="--admission"||command=="--reference"||command=="--owner-admission"||command=="--owner-reference"){
        const auto cases=object_node_test_detail::check_admission_refusals<Memory>([&](auto&s){
#ifdef HALO2_OBJECT_PREDICTION_WITH_OWNER_DISPATCH
            if(command.starts_with("--owner-"))return native_allocator_owner_detail::dispatch(s,0xadcd0u,[](auto&,auto,auto,auto){throw std::runtime_error("Rejected prediction owner requested outcome");});
#endif
            return native_object_prediction_copy_detail::bridge(s);
        },0xadcd0u,0xadce8u,command=="--reference"||command=="--owner-reference");
        std::cout<<"admission_refusals="<<cases<<'\n';return 0;
    }
    const bool owner=command=="--owner-dispatch";canonical_prefixes(owner);
    while(std::cin.peek()!=std::char_traits<char>::eof()){
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=word();abi.eflags=word();const auto mxcsr=word(),cw=word(),status=word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=status;abi.x87_depth=word();
        for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);
        for(auto&x:abi.xmm)for(auto&v:x)v=word();Memory memory;for(auto&b:memory.banks)std::cin.read(reinterpret_cast<char*>(b.data()),b.size());
        auto reference_memory=memory;const auto ref=run(reference_memory,abi,false),got=run(memory,abi,true,-1,owner);
        if(!same_abi(ref.abi,got.abi)||ref.pc!=got.pc||reference_memory.banks!=memory.banks||reference_memory.reads!=memory.reads||ref.stores!=got.stores||got.callbacks)
            throw std::runtime_error("Prediction copy canonical hardware-input transaction mismatch");
        for(auto v:got.abi.gp)emit(v);emit(got.abi.eflags);emit(got.abi.floating_point->mxcsr);emit(got.abi.x87_status);emit(got.abi.floating_point->x87_control_word);
        for(auto&x:got.abi.xmm)for(auto v:x)emit(v);for(auto&b:memory.banks)std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());
        emit(got.stores.size());for(const auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}
        emit(got.pc);
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
