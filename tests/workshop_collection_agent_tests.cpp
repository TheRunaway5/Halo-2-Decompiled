#include "halo2/native_tu15_collection_contact_agent.hpp"
#include "halo2/native_tu15_collection_filter.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
namespace {
void check(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
struct Fault:std::runtime_error{Fault():std::runtime_error("declared collection input/service fault") {}};
class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::byte> bytes;
    mutable std::uint32_t fail_read{};std::uint32_t fail_write{};
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        if(p==fail_read){fail_read=0;throw Fault();}
        for(unsigned n=0;n<out.size();++n)out[n]=bytes.at(p+n);
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(p==fail_write){fail_write=0;throw Fault();}
        for(unsigned n=0;n<in.size();++n)bytes.at(p+n)=in[n];
    }
    void put(std::uint32_t p,std::uint32_t v,unsigned width=4){for(unsigned n=0;n<width;++n)bytes[p+n]=std::byte(v>>(n*8u));}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(bytes.at(p+n))<<(n*8u);return v;}
};
void run(std::uint32_t count,const std::string& fault,bool rejected=false) {
    Memory m;for(std::uint32_t p=0x1000;p<0x20000;++p)m.bytes[p]=std::byte{0xa5};
    m.put(0x48c938,0x7000);m.put(0x7000,0x7100);m.put(0x7110,0x149da0);m.put(0x7114,0x149e00);m.put(0x48b860,0xdeadbeef);
    m.put(0x1000,0x3000);m.put(0x100c,0);m.put(0x101c,rejected?0x12340001:2);m.put(0x1020,0);
    m.put(0x1100,0x4000);m.put(0x1108,0x7500);m.put(0x110c,0);m.put(0x111c,rejected?0x12340001:1);
    m.put(0x3000,0x6000);m.put(0x6014,0xab4b0);m.put(0x3100,0x6100);m.put(0x6114,0xa9520);
    m.put(0x4000,0x6200);m.put(0x400c,0x5000);m.put(0x4010,count);m.put(0x6228,0x3061f0);m.put(0x6234,0x306220);
    for(unsigned n=0;n<9;++n){m.put(0x5000+n*8u,0x3100);m.put(0x5004+n*8u,0);}
    m.put(0x2000,0x8000);m.put(0x2004,0x680c);m.put(0x680c,0x6900);m.put(0x6900,0x1cbe70);
    m.put(0x8000+((23u<<5u)+24u)*4u+0x18cu,0x303c10);
    for(unsigned n=0;n<32;++n)m.put(0x6800+n*4u+28u,0xffffffff);
    if(fault=="header_data")m.fail_write=0xe00c;
    if(fault=="header_count")m.fail_write=0xe010;
    if(fault=="header_flags")m.fail_write=0xe014;
    if(fault=="count_owner")m.fail_read=0x6228;
    std::uint32_t next=0xe000;std::map<std::uint32_t,std::pair<std::uint32_t,std::uint32_t>> allocations;
    struct Released {std::uint32_t address,bytes,kind,vtable;};std::vector<Released> released;
    std::vector<std::string> phases;unsigned predicates{},classes{},children{};
    NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1fa0};NativeTu15CollectionContactServices services;
    services.allocation.allocate=[&](const auto& request,auto&){
        check(request.receiver==0x7000&&request.owner==0x149da0,"Wrong collection allocation owner");
        phases.push_back("allocate"+std::to_string(request.memory_class));
        if(fault=="reserve"&&request.memory_class==0x12)throw Fault();
        const auto p=next;next=(next+request.bytes+15u)&~15u;check(next<0x20000,"Owned allocator overflow");allocations.emplace(p,std::pair{request.bytes,request.memory_class});return p;
    };
    services.allocation.release=[&](const auto& request,auto&){
        check(request.receiver==0x7000&&request.owner==0x149e00,"Wrong collection release owner");
        check(allocations.at(request.data)==std::pair{request.bytes,request.memory_class},"Collection release size/class/lifetime differs");
        released.push_back({request.data,request.bytes,request.memory_class,m.word(0xe000)});allocations.erase(request.data);
    };
    services.count_children=[&](auto shape,auto owner,auto&){
        phases.push_back("count");check(shape.identity==0x4000&&owner.identity==0x3061f0,"Collection factory lost argument reversal/count owner");
        if(fault=="count")throw Fault();
        const auto current=native_tu15_collection_child_count(m,shape);
        if(fault=="publish_count")m.fail_write=0xe010;
        return current;
    };
    services.filter_child=[&](auto receiver,auto owner,auto output,auto context,auto second,auto first,auto shape,auto index,auto& current){
        ++predicates;check(receiver==0x680c&&owner.identity==0x1cbe70&&output==0xb020&&context.identity==0x2000&&first.identity==0x1100&&second.identity==0x1000&&shape.identity==0x4000,"Collection filter inputs differ");
        check(m.word(0xb000)==0x3100&&m.word(0xb004)==index&&m.word(0xb008)==0x7500&&m.word(0xb00c)==0x1100,"Collection child DATA differs");
        if(fault=="filter")throw Fault();
        // The genuine native matrix algorithm computes the result. No fixed
        // predicate or captured result is returned by a callback.
        const auto result=native_tu15_filter_collection_child(m,receiver,output,second,first,shape,index,{0xb100},current,{});
        if(fault=="shrink_after_filter")m.put(0xe010,1);
        return result;
    };
    services.dispatch.classify_shape=[&](auto shape,auto owner,auto&){
        const bool first=classes++%2==0;
        if(fault==(first?"class_first":"class_second"))throw Fault();
        check(shape.identity==(first?0x3100u:0x3000u),"Collection classifier shape differs");
        if(owner.identity==0xa9520)return native_tu15_contact_child_class_23();
        if(owner.identity==0xab4b0)return native_tu15_contact_child_class_24();
        throw std::runtime_error("Unqualified collection class owner");
    };
    services.dispatch.create_agent=[&](auto owner,auto first,auto second,auto context,std::uint32_t user,auto& current){
        ++children;check(owner.identity==0x303c10&&first.identity==0xb000&&second.identity==0x1000&&context.identity==0x2000&&user==0xfeedabcd,"Collection selected child inputs differ");
        if(fault=="nested")throw Fault();
        const auto child=native_tu15_create_contact_agent(m,NativeTu15ContactAgentFactory::owner_303c10,user,current,services.allocation);
        if(fault=="child_store")m.fail_write=m.word(0xe00c);
        if(fault=="derived_store")m.fail_write=0xe000;
        return child;
    };
    auto input=[&](const char* name,std::uint32_t value){phases.emplace_back(name);if(fault==name)throw Fault();return value;};
    const NativeTu15CollectionContactArguments arguments{[&]{return input("first",0x1000);},[&]{return input("second",0x1100);},[&]{return input("context",0x2000);},[&]{return input("user",0xfeedabcd);}};
    bool threw=false;
    try{const auto agent=native_tu15_create_collection_contact_agent(m,arguments,{0xb000,0xb020},fp,services);check(agent.identity==0xe000,"Collection return allocation differs");}catch(const Fault&){threw=true;}
    const bool input_change=fault=="shrink_after_filter";
    check(threw==(!fault.empty()&&!input_change),"Collection declared fault did not propagate");
    check(m.word(0xe004,2)==40&&m.word(0xe006,2)==0xa5a5,"Collection allocation header differs");
    const bool before_arm=fault=="user"||fault=="context"||fault=="first"||fault=="second";
    if(threw&&!before_arm) {
        check(!released.empty()&&released.back().address==0xe000&&released.back().kind==0x1a,"Collection factory cleanup missing");
        check(released.back().vtable==(fault=="derived_store"?0x41ccc8u:0x45edf0u),"Collection base/factory cleanup phase differs");
        if(released.size()==2)check(released.front().kind==0x12&&released.front().vtable==0x41ccc8,"Collection array must release before base cleanup");
    }else check(released.empty(),"Collection released storage before source arm or after success");
    if(!threw) {
        const auto expected_count=input_change?1u:count;
        check(m.word(0xe000)==0x41cce8&&m.word(0xe008)==0xfeedabcd&&m.word(0xe010)==expected_count,"Completed collection fields differ");
        const bool positive=static_cast<std::int32_t>(expected_count)>0;
        check(predicates==(positive?expected_count:0)&&children==(rejected?0:predicates),"Collection filter/child loop differs");
        const auto capacity=count>4u&&count<0x80000000u?std::max(8u,count):4u;
        check(m.word(0xe014)==(capacity==4?0x80000004u:capacity),"Collection inline/reserved capacity differs");
        for(unsigned n=0;n<predicates;++n){const auto child=m.word(m.word(0xe00c)+n*4u);if(rejected)check(child==0x48cdc4,"Collection rejection sentinel differs");else check(m.word(child)==0x41d01c&&m.word(child+8u)==0xfeedabcd,"Collection genuine child differs");}
    }
    check(fp.control_word==0x27f&&fp.status==0x20&&fp.mxcsr==0x1fa0,"Collection integer/filter environment differs");
}
}
int main(){try{
    unsigned cases{};for(auto count:{0u,1u,4u,5u,9u,0xffffffffu,0x80000000u})for(bool reject:{false,true}){run(count,"",reject);++cases;}
    for(const auto& fault:{"user","context","first","second","header_data","header_count","header_flags","count_owner","count","reserve","publish_count","filter","class_first","class_second","nested","child_store","derived_store","shrink_after_filter"}){run(5,fault);++cases;}
    std::cout<<"Passed "<<cases<<" collection construction/ownership cases with real native child construction and group filtering\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
