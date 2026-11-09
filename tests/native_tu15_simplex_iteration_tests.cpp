#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_iteration_abi.hpp"
#include "physics/runtime/native_tu15_simplex_projection_abi.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using Kind=NativeTu15SimplexReductionKind;
namespace reference {
#include "simplex_iteration_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,work=base+0x100u,stack=base+0x3000u,table_base=0x30f6e0u;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x4000u);
    std::array<std::byte,69>tables=[] {
        std::array<std::byte,69>b{};const std::array targets{0x30f253u,0x30f066u,0x30eb76u,0x30eb31u,0x30f175u,0x30ea8eu,0x30ea3eu,0x30ecd3u,0x30ea5cu,0x30eca2u,0x30edd0u};
        const std::array<std::uint8_t,25>classes{0,1,2,3,10,10,10,10,4,5,6,10,10,10,10,10,7,8,10,10,10,10,10,10,9};
        std::memcpy(b.data(),targets.data(),44u);std::memcpy(b.data()+44u,classes.data(),25u);return b;
    }();
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at>=table_base&&std::uint64_t(at-table_base)+size<=tables.size())return std::span(tables).subspan(at-table_base,size);
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Iteration DATA escaped bank");return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared iteration read stop");std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{if(++write_count==fail_write)throw std::runtime_error("Declared iteration write stop");std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4u);}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{};Kind kind{};
    std::vector<SourcePhysicsContactRecordStore>stores;std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>packets;std::set<std::uint32_t>sites;std::uint64_t callbacks{};
};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned feedback=0,unsigned stop_observer=0){
    Result out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){out.stores.push_back(e);
        if(feedback&&e.instruction==0x30ea13u){m.put(abi.gp[4]+0x1cu,0xdeadbeefu);m.put(work,0x12345678u);}
        if(out.stores.size()==stop_observer)throw std::runtime_error("Declared iteration observer stop");};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;cursor.position=&out.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try{if(native)out.kind=native_simplex_iteration_detail::run_private_stage(s,[&](auto&state,std::uint32_t pc){out.packets.emplace_back(pc,state.save());});else out.kind=reference::run_stage(s);}catch(const std::runtime_error&){out.stopped=true;}
    out.abi=s.save();return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||x.kind!=y.kind||a.bank!=b.bank||a.tables!=b.tables||a.reads!=b.reads||x.stores!=y.stores){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<" fp "<<x.abi.x87_status<<"/"<<y.abi.x87_status<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";for(unsigned n=0;n<8u;++n)if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs\n";
        throw std::runtime_error("Native simplex iteration mismatch "+name);
    }
    if(y.callbacks)throw std::runtime_error("Native iteration used literal callbacks");
}
void supplemental(){
    std::mt19937 rng(0x30ea00u);unsigned cases{},prefixes{};std::set<std::uint32_t>sites;std::set<Kind>kinds;
    const std::array<std::uint32_t,16>eps{0,0x80000000u,1,0x80000001u,0x007fffffu,0x00800000u,0x7f7fffffu,0xff7fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0xffc23456u,0x7f812345u,0xff823456u,0x2edbe6ffu,0xbf800000u};
    for(unsigned k=0;k<128u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();abi.gp[4]=k%8u==1u?work:stack;abi.gp[6]=work;abi.gp[7]=work+4u;
        seed.put(work,k%5u);seed.put(work+4u,(k/5u)%5u);if(k>=80u){seed.put(work,rng());seed.put(work+4u,rng());}
        if(k%16u==7u)abi.gp[7]=work;seed.put(abi.gp[4]+0x14u,eps[k%16u]);seed.put(abi.gp[4]+0x1cu,k>=64u?0xffffffffu:k);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(std::array{0x7fu,0x27fu,0x37fu}[k%3u]|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,7u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-(1.234567890123456789L+n):(9.876543210987654321L+n);for(auto&v:abi.xmm)for(auto&word:v)word=rng();
        Memory a=seed,b=seed;const auto x=run(a,abi,false,k%2u),y=run(b,abi,true,k%2u);compare(a,b,x,y,"normal-"+std::to_string(k));if(x.stopped)throw std::runtime_error("Normal iteration stopped");sites.insert(x.sites.begin(),x.sites.end());kinds.insert(x.kind);++cases;
        if(k<12u){Memory baseline=seed;const auto unchanged=run(baseline,abi,false);
            for(unsigned category=0;category<3u;++category){const auto limit=category==0?baseline.read_count:category==1?baseline.write_count:unsigned(unchanged.stores.size());for(unsigned n=1;n<=limit;++n){Memory c=seed,d=seed;if(category==0)c.fail_read=d.fail_read=n;if(category==1)c.fail_write=d.fail_write=n;const auto r=run(c,abi,false,0,category==2?n:0),s=run(d,abi,true,0,category==2?n:0);compare(c,d,r,s,"prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));if(!r.stopped)throw std::runtime_error("Iteration prefix did not stop");++prefixes;}}
        }
    }
    if(sites.size()!=17u||kinds.size()!=11u)throw std::runtime_error("Iteration site/kind coverage incomplete");std::cerr<<"{\"canonical_cases\":"<<cases<<",\"retired_fp_prefixes\":"<<prefixes<<",\"source_sites\":"<<sites.size()<<",\"reduction_kinds\":"<<kinds.size()<<",\"native_literal_callbacks\":0}\n";
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4u);if(!std::cin)throw std::runtime_error("Truncated iteration fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4u);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){for(auto value:abi.gp)emit(value);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10u);for(const auto&v:abi.xmm)for(auto value:v)emit(value);}
void fixture(){const auto count=input_word();for(unsigned k=0;k<count;++k){SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=input_word();abi.eflags=input_word();const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>b{};std::cin.read(reinterpret_cast<char*>(b.data()),10u);std::memcpy(&abi.x87[n],b.data(),10u);}for(auto&v:abi.xmm)for(auto&word:v)word=input_word();Memory m;std::cin.read(reinterpret_cast<char*>(m.bank.data()),m.bank.size());std::cin.read(reinterpret_cast<char*>(m.tables.data()),m.tables.size());const auto got=run(m,abi,true);if(got.stopped)throw std::runtime_error("Native hardware iteration stopped");emit_abi(got.abi);std::cout.write(reinterpret_cast<const char*>(m.bank.data()),m.bank.size());emit(got.stores.size());for(const auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}emit(got.pc);emit(std::uint32_t(got.kind));emit(got.packets.size());for(const auto&[pc,state]:got.packets){emit(pc);emit_abi(state);}}}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else supplemental();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
