#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_finalize_abi.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
#include "simplex_finalize_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,work=base+0x100u,stack=base+0x3000u;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x4000u);
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Finalization DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared finalization read stop");
        std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(++write_count==fail_write)throw std::runtime_error("Declared finalization write stop");
        std::ranges::copy(in,view(at,in.size()).begin());
    }
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4);}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{},kernel_result{};bool stopped{};
    std::vector<SourcePhysicsContactRecordStore>stores;std::vector<std::uint32_t>estimates;
    std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>packets;
    std::set<std::uint32_t>sites;std::uint64_t callbacks{};
};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned feedback=0,unsigned fail_observer=0,bool fail_estimate=false){
    Result out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        out.stores.push_back(e);
        if(feedback==6u&&out.stores.size()==1){m.put(abi.gp[5]+0x14u,base+0x1800u);m.put(abi.gp[5]+0x18u,base+0x1900u);}
        if(feedback==7u&&e.instruction==0x30f616u){m.put(abi.gp[4]+0x70u,0x3f800000u);m.put(abi.gp[4]+0x74u,0x3f400000u);}
        if(feedback==7u&&e.instruction==0x30f67fu)m.put(e.address,0x41200000u);
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared finalization observer stop");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};
    services.reciprocal_sqrt=[&](std::uint32_t input){out.estimates.push_back(input);if(fail_estimate)throw std::runtime_error("Declared actual estimate owner stop");return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(input)))));};
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&out.pc;cursor.service_sites=service_sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try{
        if(native)out.kernel_result=native_simplex_finalize_detail::run_private_stage(s,[&](auto&state,std::uint32_t pc){out.packets.emplace_back(pc,state.save());});
        else reference::run_stage(s);
    }catch(const std::runtime_error&){out.stopped=true;}
    out.abi=s.save();if(!native&&!out.stopped)out.kernel_result=out.abi.gp[0];return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||
        a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||x.kernel_result!=y.kernel_result||a.bank!=b.bank||a.reads!=b.reads||x.stores!=y.stores||x.estimates!=y.estimates){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";
        for(unsigned n=0;n<8;++n){if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs\n";if(x.abi.xmm[n]!=y.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";}
        throw std::runtime_error("Native simplex finalization mismatch "+name);
    }
    if(y.callbacks)throw std::runtime_error("Native finalization used literal callbacks");
}
void supplemental(){
    std::mt19937 rng(0x30f5d7u);unsigned cases{},prefixes{};std::set<std::uint32_t>sites;
    const std::array counts{0u,1u,2u,3u,4u,0xffffffffu,0x80000000u};
    for(unsigned k=0;k<112;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[4]=stack;abi.gp[5]=stack+0x80u+k%16u;abi.gp[6]=work;abi.gp[7]=work+4u;abi.gp[3]=work+0x300u;
        for(auto offset:{0x20u,0xa0u,0x300u})for(unsigned lane=0;lane<4;++lane)seed.put(work+offset+lane*4,std::bit_cast<std::uint32_t>(float(1+int(rng()%31))/8.0f));
        seed.put(work,counts[k%7]);seed.put(work+4u,counts[(k/7)%7]);
        std::uint32_t plane=base+0x1000u,cache=base+0x1200u;
        switch(k%8){case 1:plane=stack;break;case 2:cache=stack;break;case 3:cache=stack+0x20u;break;case 4:cache=work+0x3cu;break;case 5:plane=work+0x20u;break;default:break;}
        seed.put(abi.gp[5]+0x18u,plane);seed.put(abi.gp[5]+0x14u,cache);seed.put(abi.gp[5],0x3456789au);seed.put(abi.gp[5]+4u,0x63000000u);
        seed.put(stack,0x63000000u);seed.put(stack+4u,0x12345678u);seed.put(stack+8u,0x23456789u);seed.put(stack+0x20u,0xaabbccddu^k);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(std::array{0x7fu,0x27fu,0x37fu}[k%3]|((k%4)<<10u)),0x1f80u|((k%4)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,8u}[k%4];abi.x87_status=0x4100u|((k%8)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2?-(1.234567890123456789L+n):(9.876543210987654321L+n);
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();
        Memory expected=seed,actual=seed;const auto x=run(expected,abi,false,k%8),y=run(actual,abi,true,k%8);compare(expected,actual,x,y,"normal-"+std::to_string(k));
        if(x.stopped)throw std::runtime_error("Normal finalization case stopped");sites.insert(x.sites.begin(),x.sites.end());++cases;
        if(k<16){for(unsigned category=0;category<3;++category){const auto limit=category==0?expected.read_count:category==1?expected.write_count:unsigned(x.stores.size());
            for(unsigned n=1;n<=limit;++n){Memory a=seed,b=seed;if(category==0)a.fail_read=b.fail_read=n;if(category==1)a.fail_write=b.fail_write=n;
                const auto r=run(a,abi,false,0,category==2?n:0),s=run(b,abi,true,0,category==2?n:0);compare(a,b,r,s,"prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));
                if(!r.stopped)throw std::runtime_error("Finalization prefix did not stop");++prefixes;
            }
        }
        Memory a=seed,b=seed;const auto r=run(a,abi,false,0,0,true),s=run(b,abi,true,0,0,true);compare(a,b,r,s,"estimate-owner-stop");if(!r.stopped)throw std::runtime_error("Estimate owner did not stop");++prefixes;}
    }
    if(sites.size()!=78)throw std::runtime_error("Finalization source coverage "+std::to_string(sites.size()));
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"retained_prefixes\":"<<prefixes<<",\"source_sites\":"<<sites.size()<<",\"native_literal_callbacks\":0}\n";
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated finalization fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){
    for(auto value:abi.gp)emit(value);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);
    for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);
    for(auto&vector:abi.xmm)for(auto value:vector)emit(value);
}
void fixture(){
    const unsigned count=input_word();for(unsigned k=0;k<count;++k){
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&value:abi.gp)value=input_word();abi.eflags=input_word();
        const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(auto&vector:abi.xmm)for(auto&value:vector)value=input_word();Memory memory;std::cin.read(reinterpret_cast<char*>(memory.bank.data()),memory.bank.size());
        const auto got=run(memory,abi,true);if(got.stopped)throw std::runtime_error("Native physical finalization stopped");emit_abi(got.abi);
        std::cout.write(reinterpret_cast<const char*>(memory.bank.data()),memory.bank.size());emit(got.stores.size());
        for(auto&event:got.stores){emit(event.instruction);emit(event.address);emit(event.written.size());std::cout.write(reinterpret_cast<const char*>(event.before.data()),event.before.size());std::cout.write(reinterpret_cast<const char*>(event.written.data()),event.written.size());}
        emit(got.pc);emit(got.estimates.size());for(auto input:got.estimates)emit(input);emit(got.packets.size());
        for(auto&[pc,state]:got.packets){emit(pc);emit_abi(state);}
    }
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else supplemental();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
