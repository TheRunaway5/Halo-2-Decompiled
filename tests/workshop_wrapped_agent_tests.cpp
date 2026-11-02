#include "halo2/native_tu15_wrapped_contact_agent.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
struct Fault:std::runtime_error {Fault():std::runtime_error("declared input/service fault") {}};
class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::byte> bytes;
    std::uint32_t fail_read{},fail_write{};
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(address==fail_read)throw Fault();
        for(unsigned n=0;n<out.size();++n)out[n]=bytes.at(address+n);
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        if(address==fail_write){fail_write=0;throw Fault();}
        for(unsigned n=0;n<in.size();++n)bytes.at(address+n)=in[n];
    }
    void put(std::uint32_t address,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)bytes[address+n]=std::byte(value>>(n*8u));
    }
    std::uint32_t word(std::uint32_t address,unsigned width=4)const {
        std::uint32_t result{};for(unsigned n=0;n<width;++n)result|=std::to_integer<unsigned>(bytes.at(address+n))<<(n*8u);return result;
    }
};
// Genuine owned allocation and the independently qualified actual303C10
// constructor provide child lifetime. No recorded class or child result is
// returned by a callback. These declared synthetic input graphs complement
// the protected physical-original comparison in the offline validator.
struct Case {
    Memory memory;
    NativeTu15WrappedContactServices services;
    NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1fa0};
    std::vector<std::string> phases;
    std::map<std::uint32_t,std::uint32_t> allocations;
    std::vector<std::uint32_t> released,release_vtables;
    std::uint32_t next=0x9000;unsigned classifications{};std::string fault;
    explicit Case(std::string requested):fault(std::move(requested)) {
        for(std::uint32_t p=0x1000;p<0xa000;++p)memory.bytes[p]=std::byte{0xa5};
        memory.put(0x48c938,0x7000);memory.put(0x7000,0x7100);memory.put(0x7110,0x149da0);memory.put(0x7114,0x149e00);
        memory.put(0x1000,0x3000);memory.put(0x1100,0x4000);memory.put(0x1104,0xcafebabe);memory.put(0x1108,7);
        memory.put(0x400c,0x3100);memory.put(0x3000,0x5000);memory.put(0x3100,0x5100);
        memory.put(0x5014,0xa9520);memory.put(0x5114,0xab4b0);memory.put(0x2000,0x6000);
        memory.put(0x6000+((24u<<5u)+23u)*4u+0x18cu,0x303c10);
        if(fault=="extra")memory.fail_read=0x1108;
        if(fault=="child")memory.fail_read=0x400c;
        if(fault=="child_store")memory.fail_write=0x900c;
        if(fault=="counter_store")memory.fail_write=0x9010;
        services.allocation.allocate=[&](const auto& request,auto&){
            check(request.receiver==0x7000&&request.owner==0x149da0&&request.memory_class==0x1a,"Wrong actual allocation owner");
            const auto result=next;next=(next+request.bytes+15u)&~15u;check(next<=0xa000,"Owned allocator overflow");allocations.emplace(result,request.bytes);phases.push_back("allocate"+std::to_string(request.bytes));return result;
        };
        services.allocation.release=[&](const auto& request,auto&){
            check(request.receiver==0x7000&&request.owner==0x149e00&&request.memory_class==0x1a,"Wrong actual release owner");
            check(allocations.at(request.data)==request.bytes,"Release must reread genuine size word");
            released.push_back(request.data);release_vtables.push_back(memory.word(request.data));allocations.erase(request.data);phases.push_back("release");
        };
        services.dispatch.classify_shape=[&](auto shape,auto owner,auto&){
            const bool first=classifications++==0;phases.push_back(first?"class_first":"class_second");
            if(fault==(first?"class_first":"class_second"))throw Fault();
            check(shape.identity==(first?0x3100u:0x3000u),"Factory lost original argument reversal");
            if(owner.identity==0xa9520)return native_tu15_contact_child_class_23();
            if(owner.identity==0xab4b0)return native_tu15_contact_child_class_24();
            throw std::runtime_error("Unqualified classifier");
        };
        services.dispatch.create_agent=[&](auto owner,auto first,auto second,auto context,std::uint32_t data,auto& current){
            phases.push_back("nested");check(owner.identity==0x303c10&&first.identity==0x8000&&second.identity==0x1000&&context.identity==0x2000&&data==0xfeedabcd,"Wrong nested factory input");
            check(memory.word(first.identity)==0x3100&&memory.word(first.identity+4)==0xcafebabe&&memory.word(first.identity+8)==7&&memory.word(first.identity+12)==0x1100,"Incomplete child DATA payload");
            if(fault=="nested")throw Fault();
            const auto child=native_tu15_create_contact_agent(memory,NativeTu15ContactAgentFactory::owner_303c10,data,current,services.allocation);
            if(fault=="derived_store")memory.fail_write=0x9000;
            return child;
        };
    }
    std::uint32_t input(const char* name,std::uint32_t value) {
        phases.emplace_back(name);if(fault==name)throw Fault();return value;
    }
    void run() {
        const NativeTu15WrappedContactArguments args{[&]{return input("first",0x1000);},[&]{return input("second",0x1100);},[&]{return input("context",0x2000);},[&]{return input("user",0xfeedabcd);}};
        bool threw=false;
        try{const auto agent=native_tu15_create_wrapped_contact_agent(memory,args,{0x8000},fp,services);check(agent.identity==0x9000,"Wrong allocated wrapper identity");}catch(const Fault&){threw=true;}
        check(threw==!fault.empty(),"Declared fault must propagate");
        const std::vector<std::string> inputs{"allocate20","user","context","first","second"};
        auto count=inputs.size();const auto found=std::find(inputs.begin(),inputs.end(),fault);if(found!=inputs.end())count=found-inputs.begin()+1;
        check(phases.size()>=count&&std::equal(inputs.begin(),inputs.begin()+count,phases.begin()),"Factory input phases differ from actual cdecl reads");
        check(memory.word(0x9004,2)==20&&memory.word(0x9006,2)==0xa5a5,"Factory size store overwrote adjacent header bytes");
        const bool input_fault=found!=inputs.end();
        check(released.size()==(threw&&!input_fault?1u:0u),"Factory unwind ownership phase differs");
        if(!released.empty()) {
            check(released.front()==0x9000,"Factory released wrong resource");
            const bool base_armed=fault!="extra"&&fault!="child"&&fault!="derived_store";
            check(release_vtables.front()==(base_armed?0x45edf0u:0x41ca9cu),"Actual37E400/37E4C0 cleanup order differs");
        }
        if(!threw) {
            check(memory.word(0x9000)==0x41caf0&&memory.word(0x9008)==0xfeedabcd&&memory.word(0x900c)==0x9020&&memory.word(0x9010)==0,"Wrapper construction differs");
            check(memory.word(0x9020)==0x41d01c&&memory.word(0x9024,2)==64&&memory.word(0x9028)==0xfeedabcd,"Actual nested constructor differs");
        }
        check(fp.control_word==0x27f&&fp.status==0x20&&fp.mxcsr==0x1fa0,"Integer wrapper changed FP environment");
    }
};
void transformed_case(const std::string& requested,bool reversed=false) {
    const bool late=requested=="late_values";
    Case c(late?"":requested);auto& m=c.memory;
    // Two copied metadata vectors followed by an authored affine basis.
    for(unsigned n=0;n<24;++n)m.put(0x7200+n*4u,0);
    m.put(0x1108,0x7200);
    for(unsigned n=0;n<3;++n)m.put(0x7220+n*20u,0x3f800000);
    m.put(0x7250,0x40a00000);m.put(0x7254,0x40c00000);m.put(0x7258,0x40e00000);
    for(unsigned n=0;n<16;++n)m.put(0x4010+n*4u,0);
    for(unsigned n=0;n<3;++n)m.put(0x4010+n*20u,0x3f800000);
    m.put(0x4040,0x40000000);m.put(0x4044,0x40400000);m.put(0x4048,0x40800000);
    if(requested=="user_store")m.fail_write=0x9008;
    if(requested=="vector_read")m.fail_read=0x7250;
    if(requested=="vector_store")m.fail_write=0x8250;
    if(requested=="geometry_read")m.fail_read=0x4010;
    const auto child_user=late?0xfeed1234u:0xfeedabcdu,child_context=late?0x2100u:0x2000u;
    c.services.dispatch.create_agent=[&](auto owner,auto first,auto second,auto context,std::uint32_t data,auto&fp){
        c.phases.push_back("nested");
        check(owner.identity==0x303c10&&first.identity==0x8000&&second.identity==0x1000&&context.identity==child_context&&data==child_user,"Transformed factory lost early dispatch/late ordinary inputs");
        check(m.word(0x8000)==0x3100&&m.word(0x8004)==0xcafebabe&&m.word(0x8008)==0x8200&&m.word(0x800c)==0x1100,"Transformed child payload differs");
        for(unsigned n=0;n<32;++n)check(m.bytes.at(0x8200+n)==m.bytes.at(0x7200+n),"Copied metadata vectors differ");
        check(m.word(0x8250)==0x40e00000&&m.word(0x8254)==0x41100000&&m.word(0x8258)==0x41300000&&m.word(0x825c)==0,"Genuine affine translation composition differs");
        if(c.fault=="nested")throw Fault();
        const auto child=native_tu15_create_contact_agent(m,NativeTu15ContactAgentFactory::owner_303c10,data,fp,c.services.allocation);
        if(requested=="derived_store")m.fail_write=0x9000;
        return child;
    };
    unsigned users{},contexts{};
    const NativeTu15WrappedContactArguments args{
        [&]{return c.input("first",reversed?0x1000:0x1100);},[&]{return c.input("second",reversed?0x1100:0x1000);},
        [&]{const bool initial=contexts++==0;return c.input(initial?"context_initial":"context_late",initial?0x2000u:child_context);},
        [&]{const bool initial=users++==0;return c.input(initial?"user_initial":"user_late",initial?0xfeedabcdu:child_user);}};
    bool threw=false;
    const SourcePhysicsContactRecordObserver observer=[&](const auto& event){
        if(requested=="derived_observer"&&event.instruction==0x2f7703u)throw Fault();
    };
    try{const auto agent=reversed?
        native_tu15_create_reversed_transformed_contact_agent(m,args,{0x8000,0x8200},c.fp,c.services,observer):
        native_tu15_create_transformed_contact_agent(m,args,{0x8000,0x8200},c.fp,c.services,observer);
        check(agent.identity==0x9000,"Transformed allocation identity differs");}catch(const Fault&){threw=true;}
    check(threw==(!requested.empty()&&!late),"Transformed declared fault did not propagate");
    check(m.word(0x9004,2)==16&&m.word(0x9006,2)==0xa5a5,"Transformed size word/header differs");
    const bool before_arm=requested=="user_initial";
    check(c.released.size()==(threw&&!before_arm?1u:0u),"Transformed allocation unwind arm differs");
    if(!c.released.empty()) {
        const bool before_vtable=requested=="user_store"||requested==(reversed?"second":"first");
        const bool before_base=before_vtable||requested=="extra"||requested=="child"||requested=="vector_read";
        check(c.released.front()==0x9000&&c.release_vtables.front()==(before_vtable?0xa5a5a5a5u:before_base?0x41cc64u:0x45edf0u),"Transformed state0/state1 cleanup order differs");
    }
    if(!threw) {
        const std::vector<std::string> phases{"allocate16","user_initial",reversed?"second":"first","context_initial","class_first",reversed?"first":"second","class_second","user_late","context_late","nested","allocate64"};
        check(c.phases==phases,"Transformed source input/service phases differ");
        check(m.word(0x9000)==(reversed?0x41cc84u:0x41cc64u)&&m.word(0x9008)==0xfeedabcd&&m.word(0x900c)==0x9010,"Transformed parent fields differ");
        check(m.word(0x9010)==0x41d01c&&m.word(0x9018)==child_user,"Transformed real child construction differs");
    }
    check(c.fp.control_word==0x27f&&c.fp.status==0x20&&c.fp.mxcsr==0x1fa0,"Transformed exact affine environment differs");
}
}
int main(){try{
    const std::vector<std::string> cases{"","user","context","first","second","extra","child","class_first","class_second","nested","child_store","counter_store","derived_store"};
    for(const auto& fault:cases)Case(fault).run();
    const std::vector<std::string> transformed{"","late_values","user_initial","user_store","first","second","extra","child","vector_read","vector_store","geometry_read","context_initial","class_first","class_second","user_late","context_late","nested","child_store"};
    for(const auto& fault:transformed)transformed_case(fault);
    for(const auto& fault:transformed)transformed_case(fault,true);
    for(const auto& fault:{"derived_store","derived_observer"})transformed_case(fault,true);
    std::cout<<"Passed "<<cases.size()<<" wrapper, "<<transformed.size()<<" transformed and "<<transformed.size()+2<<" reversed transformed construction/ownership cases\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
