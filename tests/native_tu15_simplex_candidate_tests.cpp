#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_candidate_abi.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
#include "simplex_candidate_reference.inc"
}
namespace ordered_reference {
#include "simplex_candidate_ordered_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,work=base+0x100u;
using Side=NativeTu15SimplexCandidateSide;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x4000u);
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Candidate DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared candidate read stop");
        std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(++write_count==fail_write)throw std::runtime_error("Declared candidate write stop");
        std::ranges::copy(in,view(at,in.size()).begin());
    }
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4);}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{},accepted{};
    std::vector<SourcePhysicsContactRecordStore>stores;std::set<std::uint32_t>sites;std::uint64_t callbacks{};
    std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>packets;
};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,Side side,bool native,bool feedback=false,unsigned fail_observer=0,bool ordered=false){
    Result out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        out.stores.push_back(e);
        if(feedback&&out.stores.size()==1){m.put(e.address,0xc0000000u);m.put(abi.gp[4]+0x10u,0x3f800000u);}
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared candidate observer stop");
    };
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;cursor.position=&out.pc;cursor.service_sites=service_sites;
    cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try{
        if(native)out.accepted=native_simplex_candidate_detail::run_private_stage(s,side,[&](auto&state,std::uint32_t pc){out.packets.emplace_back(pc,state.save());});
        else out.accepted=ordered?ordered_reference::run_stage(s,unsigned(side)):reference::run_stage(s,unsigned(side));
    }catch(const std::runtime_error&){out.stopped=true;}
    out.abi=s.save();return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||
       a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;
    return true;
}
void compare(const Memory&expected,const Memory&actual,const Result&a,const Result&b,const std::string&name){
    if(!same_abi(a.abi,b.abi)||a.pc!=b.pc||a.stopped!=b.stopped||a.accepted!=b.accepted||expected.bank!=actual.bank||a.stores!=b.stores||expected.reads!=actual.reads){
        std::cerr<<name<<" pc "<<std::hex<<a.pc<<"/"<<b.pc<<" flags "<<a.abi.eflags<<"/"<<b.abi.eflags
            <<" sw "<<a.abi.x87_status<<"/"<<b.abi.x87_status<<std::dec<<" depth "<<a.abi.x87_depth<<"/"<<b.abi.x87_depth
            <<" stores "<<a.stores.size()<<"/"<<b.stores.size()<<" reads "<<expected.reads.size()<<"/"<<actual.reads.size()<<"\n";
        for(unsigned n=0;n<8;++n){if(a.abi.xmm[n]!=b.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";if(a.abi.gp[n]!=b.abi.gp[n])std::cerr<<"gp "<<n<<" differs "<<std::hex<<a.abi.gp[n]<<"/"<<b.abi.gp[n]<<std::dec<<"\n";}
        throw std::runtime_error("Native simplex candidate mismatch "+name);
    }
    if(b.callbacks)throw std::runtime_error("Native candidate used literal callbacks");
}
void supplemental(){
    std::mt19937 rng(0x30eeb1u);unsigned cases{},prefixes{},accepted{};std::set<std::uint32_t>sites;
    const std::array counts{0u,1u,2u,3u,4u,0xffffffffu,0x80000000u,0x10000000u};
    for(unsigned k=0;k<96;++k)for(unsigned side=0;side<3;++side){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());
        for(unsigned at=0x10;at<0x400;at+=4)seed.put(work+at,std::bit_cast<std::uint32_t>(float(int(rng()%33)-16)/4.0f));
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[6]=work;abi.gp[7]=work+4u;abi.gp[3]=work+0x300u;abi.gp[4]=base+0x3000u;
        seed.put(side==1u?abi.gp[7]:work,counts[k%counts.size()]);
        for(unsigned n=0;n<4;++n){const auto v=std::bit_cast<std::uint32_t>(n==0?1.0f:0.0f);seed.put(work+0x300u+n*4,v);seed.put(abi.gp[4]+0x70u+n*4,v);}
        seed.put(abi.gp[4]+0x10u,std::bit_cast<std::uint32_t>(std::array{0.0f,0.125f,1.0f,8.0f,-1.0f}[k%5]));
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{
            std::uint16_t(std::array{0x7fu,0x27fu,0x37fu}[k%3u]|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,7u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2?-(1.234567890123456789L+n):(9.876543210987654321L+n);
        for(auto&v:abi.xmm)for(auto&w:v)w=rng();
        Memory expected=seed,actual=seed;const auto a=run(expected,abi,Side(side),false,k%7==0),b=run(actual,abi,Side(side),true,k%7==0);
        compare(expected,actual,a,b,"normal-"+std::to_string(k)+"-"+std::to_string(side));
        if(a.stopped)throw std::runtime_error("Normal candidate fixture stopped");sites.insert(a.sites.begin(),a.sites.end());++cases;accepted+=a.accepted;
        if(k<12)for(unsigned category=0;category<3;++category){
            const auto limit=category==0?expected.read_count:category==1?expected.write_count:unsigned(a.stores.size());
            for(unsigned n=1;n<=limit;++n){
                Memory er=seed,ar=seed;if(category==0)er.fail_read=ar.fail_read=n;if(category==1)er.fail_write=ar.fail_write=n;
                const auto e=run(er,abi,Side(side),false,false,category==2?n:0,true),r=run(ar,abi,Side(side),true,false,category==2?n:0);
                compare(er,ar,e,r,"prefix-"+std::to_string(k)+"-"+std::to_string(side)+"-"+std::to_string(category)+"-"+std::to_string(n));
                if(!e.stopped)throw std::runtime_error("Declared candidate prefix did not stop");++prefixes;
            }
        }
    }
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"retained_prefixes\":"<<prefixes<<",\"source_sites\":"<<sites.size()<<",\"accepted\":"<<accepted<<",\"native_literal_callbacks\":0}\n";
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated candidate fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){
    for(auto value:abi.gp)emit(value);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);
    for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);
    for(auto&vector:abi.xmm)for(auto value:vector)emit(value);
}
void fixture(){
    const unsigned count=input_word();
    for(unsigned k=0;k<count;++k){
        const auto side=Side(input_word());SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&value:abi.gp)value=input_word();abi.eflags=input_word();
        const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(auto&vector:abi.xmm)for(auto&value:vector)value=input_word();
        Memory memory;std::cin.read(reinterpret_cast<char*>(memory.bank.data()),memory.bank.size());
        const auto got=run(memory,abi,side,true);if(got.stopped)throw std::runtime_error("Native physical candidate unexpectedly stopped");
        emit_abi(got.abi);std::cout.write(reinterpret_cast<const char*>(memory.bank.data()),memory.bank.size());emit(got.stores.size());
        for(auto&event:got.stores){emit(event.instruction);emit(event.address);emit(event.written.size());std::cout.write(reinterpret_cast<const char*>(event.before.data()),event.before.size());std::cout.write(reinterpret_cast<const char*>(event.written.data()),event.written.size());}
        emit(got.pc);emit(got.accepted);emit(got.packets.size());
        for(auto&[pc,state]:got.packets){emit(pc);emit_abi(state);}
    }
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else supplemental();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
