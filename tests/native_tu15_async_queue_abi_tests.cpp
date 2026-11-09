#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "platform/native_tu15_async_queue_abi.hpp"
#if defined(HALO2_NATIVE_ASYNC_BOUND_ROUTE)
#include "platform/native_tu15_async_order.hpp"
#endif
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
namespace {
struct Boundary{};
template<class State,class Before>bool order(State&s,std::uint32_t site,Before before){
#if defined(HALO2_NATIVE_ASYNC_BOUND_ROUTE)
    (void)before;return native_async_order_detail::compare(s,site);
#else
    return native_async_queue_abi_detail::compare(s,site,before);
#endif
}
template<class State,class Compare,class Before>bool insertion(State&s,std::uint32_t site,Compare&&compare,Before before){
#if defined(HALO2_NATIVE_ASYNC_BOUND_ROUTE)
    (void)before;return native_async_order_detail::insert(s,site,std::forward<Compare>(compare));
#else
    return native_async_queue_abi_detail::insert(s,site,std::forward<Compare>(compare),before);
#endif
}
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>bytes(const std::string&text){std::vector<std::byte>result;for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return result;}
std::string hex(std::span<const std::byte>bytes){constexpr char digits[]="0123456789abcdef";std::string result;for(auto byte:bytes){auto n=std::to_integer<unsigned>(byte);result+=digits[n>>4];result+=digits[n&15];}return result;}
J value(const SourcePhysicsWorldRuntimeAbi&a){
    J live=J::array();for(unsigned i=0;i<a.x87_depth;++i)live.push_back(hex({reinterpret_cast<const std::byte*>(&a.x87[i]),10}));
    return J{{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
}
SourcePhysicsWorldRuntimeAbi input(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");auto fp=row.at("before_fp");
    a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};a.x87_status=fp[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned i=0;i<a.x87_depth;++i){auto raw=bytes(row.at("live_x87")[i]);std::memcpy(&a.x87[i],raw.data(),10);}return a;
}
void hostile_fp(){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const std::uint32_t mxcsr=0x9fc0;const std::uint16_t cw=0xa7f;
    asm volatile("fninit;fldcw %0;ldmxcsr %1;fld1;fldpi"::"m"(cw),"m"(mxcsr):"memory","st","st(1)");
#endif
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array(),accesses=J::array();J writes=J::array();const std::uint32_t*cursor{};
    mutable unsigned read_count{};unsigned write_count{},observer_count{};std::string fault_phase;unsigned fault_ordinal{},hostile{};
    std::span<std::byte>view(std::uint32_t at,unsigned count){
        auto it=banks.upper_bound(at);check(it!=banks.begin(),"Async ABI access before backing");--it;check(std::uint64_t(at-it->first)+count<=it->second.size(),"Async ABI access escaped backing");
        return std::span(it->second).subspan(at-it->first,count);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        if(hostile&1)hostile_fp();if(fault_phase=="before_read"&&read_count==fault_ordinal)throw Boundary{};
        ++read_count;reads.push_back(J::array({at,out.size()}));accesses.push_back(J::array({"read",at,out.size(),cursor?*cursor:0u}));std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(hostile&2)hostile_fp();if(fault_phase=="before_write"&&write_count==fault_ordinal)throw Boundary{};
        ++write_count;auto out=view(at,in.size());writes.push_back(J::array({at,hex(out),hex(in)}));accesses.push_back(J::array({"write",at,in.size(),cursor?*cursor:0u}));std::ranges::copy(in,out.begin());
    }
};
unsigned refusals(){
    for(unsigned variant=0;variant<14;++variant){
        Memory memory;std::uint32_t pc{};bool first=variant==4,second=variant==5;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto at){pc=at;};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        auto&policy=services.instruction_cursor;policy={&pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(variant==1)policy.position=nullptr;if(variant==2)policy.executor={};if(variant==3)policy.callback_type=&typeid(int);
        if(variant==6)policy.observation_only_stores=false;if(variant==7)policy.service_sites={};
        SourcePhysicsContactRecordObserver observe=[](const auto&){};if(variant==13)observe={};
        WorldState state{memory,observe,services,variant==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(abi);
        if(variant==8||variant==13)services.instruction={};if(variant==9)services.instruction=[&](auto at){pc=at+1u;};
        if(variant==10)policy.executor={};if(variant==11)policy.literal_service_sites=false;
        const auto incoming=value(state.save());
        const auto noop=[](auto&,auto){};
        check(!order(state,variant==12?0x1ab331u:0x1ab330u,noop),"Unsafe async comparator admitted");
        check(!insertion(state,variant==12?0x1232c1u:0x1232c0u,[](auto&){throw std::runtime_error("Unsafe child entered");},noop),"Unsafe async insertion admitted");
        check(incoming==value(state.save())&&memory.reads.empty()&&memory.writes.empty()&&pc==0,"Async admission refusal mutated source");
    }
    return 28;
}
}
int main(int argc,char**argv)try{
    if(argc==2&&std::string(argv[1])=="--guards"){std::cout<<J{{"passed",true},{"refusals",refusals()}}.dump()<<'\n';return 0;}
    check(argc==2,"Supply full async ABI cases");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);J outputs=J::array();
    for(const auto&row:fixture.at("cases")){
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto fault=row.value("fault",J::object());memory.fault_phase=fault.value("phase",std::string{});memory.fault_ordinal=fault.value("ordinal",0u);memory.hostile=row.value("hostile_fp",0u);
        unsigned callbacks{};std::uint32_t pc{};memory.cursor=&pc;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto at){pc=at;++callbacks;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observe;
        if(memory.fault_phase=="after_write")observe=[&](const auto&){if(memory.hostile&4)hostile_fp();if(memory.observer_count++==memory.fault_ordinal)throw Boundary{};};
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input(row));J calls=J::array();bool stopped{};
        const auto before=[&](auto&,auto){if(memory.hostile&8)hostile_fp();};
        try{
            const auto entry=row.at("entry").get<std::uint32_t>();bool accepted{};
            if(entry==0x1ab330u)accepted=order(state,0x1ab330u,before);
            else if(entry==0x1232c0u)accepted=insertion(state,0x1232c0u,[&](auto&child){
                auto observed=value(child.save());observed["arguments"]=J::array();
                for(unsigned n=0;n<3;++n){const auto raw=memory.view(child.gp[4]+4u+n*4u,4);std::uint32_t word{};std::memcpy(&word,raw.data(),4);observed["arguments"].push_back(word);}
                calls.push_back(std::move(observed));check(order(child,0x1ab330u,before),"Actual comparator refused");},before);
            else throw std::runtime_error("Unknown original async owner");
            check(accepted,"Typed async ABI refused declared source input");
        }catch(const Boundary&){stopped=true;}
        J banks=J::array();for(const auto&[address,bank]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(bank)}});
        outputs.push_back(J{{"id",row.at("id")},{"stopped",stopped},{"pc",pc},{"abi",value(state.save())},{"banks",banks},
            {"reads",memory.reads},{"writes",memory.writes},{"accesses",memory.accesses},{"calls",calls},{"literal_callbacks",callbacks}});
    }
    std::cout<<outputs.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
