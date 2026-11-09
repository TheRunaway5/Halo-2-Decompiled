#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_direction_abi.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
#include "simplex_direction_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,work=base+0x100u;
using Side=NativeTu15SimplexDirectionSide;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x4000u);
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Direction DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());
        if(++read_count==fail_read)throw std::runtime_error("Declared direction read stop");
        std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(++write_count==fail_write)throw std::runtime_error("Declared direction write stop");
        std::ranges::copy(in,view(at,in.size()).begin());
    }
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4);}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{};
    std::vector<SourcePhysicsContactRecordStore>stores;
    std::set<std::uint32_t>sites;std::uint64_t callbacks{};
};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,Side side,bool native,unsigned feedback=0,unsigned fail_observer=0){
    Result out;
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        out.stores.push_back(e);
        if(feedback&&out.stores.size()==1){
            m.put(abi.gp[7],0x40000000u);
            m.put(work+0x20u,0x3f800000u);
            m.put(work+0xa0u,0x40800000u);
        }
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared direction observer stop");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;cursor.position=&out.pc;cursor.service_sites=service_sites;
    cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try{
        if(native)native_simplex_direction_detail::run_private_stage(s,side);
        else reference::run_stage(s,unsigned(side));
    }catch(const std::runtime_error&){out.stopped=true;}
    out.abi=s.save();return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||
       a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;
    return true;
}
void compare(Memory&expected,Memory&actual,const Result&a,const Result&b,const std::string&name){
    if(!same_abi(a.abi,b.abi)||a.pc!=b.pc||a.stopped!=b.stopped||expected.bank!=actual.bank||a.stores!=b.stores||expected.reads!=actual.reads){
        std::cerr<<name<<" pc "<<std::hex<<a.pc<<"/"<<b.pc<<" flags "<<a.abi.eflags<<"/"<<b.abi.eflags<<std::dec
            <<" stores "<<a.stores.size()<<"/"<<b.stores.size()<<" reads "<<expected.reads.size()<<"/"<<actual.reads.size()
            <<" mxcsr "<<a.abi.floating_point->mxcsr<<"/"<<b.abi.floating_point->mxcsr<<"\n";
        for(unsigned n=0;n<8;++n)if(a.abi.xmm[n]!=b.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";
        throw std::runtime_error("Native simplex direction mismatch "+name);
    }
    if(b.callbacks)throw std::runtime_error("Native direction used literal instruction callbacks");
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated direction fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
void fixture(){
    const unsigned count=input_word();
    for(unsigned k=0;k<count;++k){
        const auto side=Side(input_word());SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&v:abi.gp)v=input_word();abi.eflags=input_word();
        const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(auto&v:abi.xmm)for(auto&w:v)w=input_word();
        Memory memory;std::cin.read(reinterpret_cast<char*>(memory.bank.data()),memory.bank.size());
        const auto got=run(memory,abi,side,true);
        if(got.stopped)throw std::runtime_error("Native direction physical case stopped");
        for(auto v:got.abi.gp)emit(v);emit(got.abi.eflags);emit(got.abi.floating_point->mxcsr);emit(got.abi.x87_status);emit(got.abi.floating_point->x87_control_word);
        emit(got.abi.x87_depth);for(unsigned n=0;n<got.abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&got.abi.x87[n]),10);
        for(auto&v:got.abi.xmm)for(auto w:v)emit(w);
        std::cout.write(reinterpret_cast<const char*>(memory.bank.data()),memory.bank.size());emit(got.stores.size());
        for(auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}
        emit(got.pc);
    }
}
void supplemental(){
    std::mt19937 random(0x30e930u);std::uint64_t cases{},prefixes{},reads{},stores{};std::set<std::uint32_t>sites;
    std::array<std::uint32_t,6>count_offsets{4u,0x120u,0x20u,0xa0u,8u,0x70u};
    for(unsigned k=0;k<240;++k)for(unsigned side=0;side<3;++side){
        Memory seed;for(auto&v:seed.bank)v=std::byte(random());
        for(auto offset:{0x20u,0x30u,0xa0u,0xb0u})for(unsigned n=0;n<4;++n){
            const auto value=float(int(random()%65u)-32)/4.0f;seed.put(work+offset+n*4u,std::bit_cast<std::uint32_t>(value));
        }
        seed.put(work,2u);seed.put(work+4u,2u);
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=random();abi.gp[6]=work;abi.gp[7]=work+count_offsets[k%6];
        abi.eflags=0x202u|(random()&0xcd5u);
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(std::array{0x7fu,0x27fu,0x37fu}[k%3u]|((k%4u)<<10u)),
            0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,8u}[k%4];abi.x87_status=0x4100u|((k%8u)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2?-(1.234567890123456789L+n):(9.876543210987654321L+n);
        for(auto&v:abi.xmm)for(auto&w:v)w=random();
        Memory expected=seed,actual=seed;const auto a=run(expected,abi,Side(side),false,k%7u==0u);const auto b=run(actual,abi,Side(side),true,k%7u==0u);
        compare(expected,actual,a,b,"normal-"+std::to_string(k)+"-"+std::to_string(side));
        if(a.stopped)throw std::runtime_error("Normal direction fixture unexpectedly stopped");
        sites.insert(a.sites.begin(),a.sites.end());++cases;reads+=expected.reads.size();stores+=a.stores.size();
        if(k<12u){
            for(unsigned category=0;category<3;++category){
                const unsigned limit=category==0?expected.read_count:category==1?expected.write_count:unsigned(a.stores.size());
                for(unsigned n=1;n<=limit;++n){
                    Memory er=seed,ar=seed;if(category==0)er.fail_read=ar.fail_read=n;if(category==1)er.fail_write=ar.fail_write=n;
                    const auto e=run(er,abi,Side(side),false,0,category==2?n:0);
                    const auto r=run(ar,abi,Side(side),true,0,category==2?n:0);
                    compare(er,ar,e,r,"prefix-"+std::to_string(k)+"-"+std::to_string(side)+"-"+std::to_string(category)+"-"+std::to_string(n));
                    if(!e.stopped)throw std::runtime_error("Declared direction prefix did not stop");++prefixes;
                }
            }
        }
    }
    if(sites.size()!=141u)throw std::runtime_error("Direction canonical site coverage "+std::to_string(sites.size())+" !=141");
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"retained_prefixes\":"<<prefixes<<",\"canonical_sites\":"<<sites.size()
        <<",\"ordered_reads\":"<<reads<<",\"ordered_stores\":"<<stores<<",\"native_literal_callbacks\":0}\n";
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else supplemental();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
