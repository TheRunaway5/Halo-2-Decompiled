#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_format_four_source_owner.hpp"
#include "physics/runtime/native_tu15_codec_copy_source_owner.hpp"
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"

namespace {
namespace owner=halo2::native_codec_format_four_owner_detail;
std::vector<std::uint32_t> retained;
struct Refusal{};
bool stop_at_retained{};
bool foreign_exception_issued{};
void format_four_retained(std::uint32_t pc) {
    if(foreign_exception_issued)throw std::runtime_error("Source decoder resumed after a foreign exception");
    retained.push_back(pc);
    if(stop_at_retained)throw Refusal{};
}
}

#include "codec_format_four_sources.inc"

namespace {
using Banks=std::map<std::uint32_t,std::vector<std::byte>>;
using External=void(*)(SourcePhysicsContactMemory&,std::uint32_t,SourcePhysicsContactRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRuntimeServices&,const SourcePhysicsContactRecordObserver&);

enum class Mode {
    quiet,empty,trace_after_load,remove_after_load,replace_after_child,write_push_fault,
    observer_push_fault,observer_install,observer_clear,foreign_exception,read_fault,vector_half_fault
};

struct CodecMemory:Memory {
    mutable J events=J::array();
    mutable std::function<void(std::uint32_t,std::uint32_t,std::size_t,bool)> feedback;
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        events.push_back(J::array({false,pc,address,out.size(),capture()}));
        std::memcpy(out.data(),at(address,out.size()),out.size());
        if(feedback)feedback(pc,address,out.size(),false);
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        events.push_back(J::array({true,pc,address,hex(in),capture()}));
        if(feedback)feedback(pc,address,in.size(),true);
        std::memcpy(const_cast<std::byte*>(at(address,in.size())),in.data(),in.size());
    }
};

SourcePhysicsWorldRuntimeAbi input(const J& row) {
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    return abi;
}

struct Result {
    Banks banks;
    J io,abi,callbacks,stores,external;
    bool fault{},foreign{},changed{};
    std::vector<std::uint32_t> suffix;
    owner::Activity activity{};
};

template<class S>Result execute(const Banks& banks,const SourcePhysicsWorldRuntimeAbi& input_abi,
        Mode mode,void(*invoke)(S&),External primary,External model) {
    CodecMemory memory;memory.banks=banks;
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observe;
    Result result;result.callbacks=J::array();result.stores=J::array();result.external=J::array();
    bool trace{};std::uint32_t expected_after_child{};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    const auto record=[&](std::uint32_t pc){memory.pc=pc;result.callbacks.push_back(memory.capture());};
    services.instruction=[&](std::uint32_t pc){memory.pc=pc;if(trace)record(pc);};
    auto& cursor=services.instruction_cursor;
    cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    const auto store_observer=[&](const SourcePhysicsContactRecordStore& event) {
        result.stores.push_back(J::array({event.instruction,event.address,hex(event.before),hex(event.written),memory.capture()}));
        if(mode==Mode::observer_push_fault&&event.instruction==0x293290u)throw Fault{};
    };
    observe=store_observer;
    if(mode==Mode::empty){services.instruction={};cursor={};observe={};}
    if(mode==Mode::observer_install)observe={};
    S state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input_abi);
    memory.capture=[&]{return packet(state,memory.pc);};
    services.required_owner=[&](auto& actual_memory,const auto& call,auto& abi,const auto& actual_observer) {
        check(&actual_memory==&memory&&&actual_observer==&observe,"Required decoder owner references changed");
        check(!call.tail&&((call.instruction==0x293329u&&call.target==0x2963d0u)||
                         (call.instruction==0x293467u&&call.target==0x296410u)||
                         (call.instruction==0x29354au&&call.target==0x2cec60u)),"Unexpected required row-eight owner");
        const auto before=packet(abi,call.instruction);
        const auto child=call.target==0x2cec60u?model:primary;
        child(actual_memory,call.target,abi,*abi.floating_point,services,actual_observer);
        result.external.push_back(J::array({call.instruction,call.target,before,packet(abi,call.instruction+6u)}));
        if(mode==Mode::replace_after_child&&!result.changed) {
            result.changed=true;expected_after_child=call.instruction+6u;
            services.instruction=[&,tag=3u](std::uint32_t pc){(void)tag;record(pc);};
        }
    };
    unsigned vector_writes{};
    memory.feedback=[&](std::uint32_t pc,std::uint32_t,std::size_t length,bool write) {
        if(mode==Mode::trace_after_load&&!write&&pc==0x293299u&&!result.changed){result.changed=true;trace=true;}
        if(mode==Mode::remove_after_load&&!write&&pc==0x293299u&&!result.changed){result.changed=true;services.instruction={};cursor={};}
        if(mode==Mode::foreign_exception&&!write&&pc==0x293299u) {
            foreign_exception_issued=true;
            throw owner::Continuation{0x29329fu,nullptr};
        }
        if(mode==Mode::read_fault&&!write&&pc==0x293299u)throw Fault{};
        if(mode==Mode::write_push_fault&&write&&pc==0x293290u)throw Fault{};
        if(mode==Mode::observer_clear&&!write&&pc==0x293290u&&!result.changed){result.changed=true;observe={};}
        if(write&&pc==0x2933c7u&&length==8u) {
            ++vector_writes;
            if(mode==Mode::observer_install&&vector_writes==1){result.changed=true;observe=store_observer;}
            if(mode==Mode::vector_half_fault&&vector_writes==2)throw Fault{};
        }
    };
    retained.clear();foreign_exception_issued=false;const auto before=owner::activity();
    try {invoke(state);}
    catch(const Fault&){result.fault=true;}
    catch(const owner::Continuation& resume){check(!resume.owner&&resume.pc==0x29329fu,"Foreign decoder exception changed");result.foreign=true;}
    const auto after=owner::activity();
    result.activity={after.calls-before.calls,after.completed-before.completed,after.continuations-before.continuations,after.children-before.children};
    result.suffix=retained;
    result.io=std::move(memory.events);result.banks=std::move(memory.banks);result.abi=packet(state,memory.pc);
    if(result.activity.continuations) {
        check(!result.suffix.empty(),"Source decoder did not enter the retained suffix");
        if(mode==Mode::replace_after_child)check(result.suffix.front()==expected_after_child,"Wrong source decoder postchild successor");
        if(mode==Mode::trace_after_load||mode==Mode::remove_after_load)check(result.suffix.front()==0x29329fu,"Wrong source decoder postload successor");
    }
    return result;
}

template<class S>unsigned refusals(void(*invoke)(S&),bool literal) {
    unsigned checked{};
    for(unsigned mode=0;mode<7;++mode) {
        CodecMemory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
        bool trace=false;std::array sites{0x38846eu,0x21f590u,0x33b595u};unsigned callbacks{};
        services.instruction=[&](std::uint32_t){++callbacks;};
        auto& cursor=services.instruction_cursor;
        cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        S state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        switch(mode) {
        case 0:state.image=SourcePhysicsContactImage::MapPack11;break;
        case 1:trace=true;break;
        case 2:services.instruction={};cursor={};break;
        case 3:cursor.callback_type=&typeid(int);break;
        case 4:cursor.position=nullptr;break;
        case 5:sites[2]^=1u;break;
        case 6:observe=[](const auto&){};cursor.observation_only_stores=false;break;
        }
        const auto before=owner::activity();memory.pc=0xdeadbeefu;retained.clear();foreign_exception_issued=false;stop_at_retained=true;
        const auto abi=packet(state,memory.pc);bool stopped{};
        try{invoke(state);}catch(const Refusal&){stopped=true;}
        stop_at_retained=false;
        check(stopped&&owner::activity().calls==before.calls&&retained==std::vector{0x293290u}&&
              memory.events.empty()&&!callbacks&&packet(state,memory.pc)==abi,
              literal?"Prepared decoder refusal touched entry ABI":"Raw decoder refusal touched entry ABI");
        ++checked;
    }
    return checked;
}

struct Counts {unsigned comparisons{},native{},children{},handoffs{},retained{},required{},faults{},foreign{},refusals{};};

template<class B,class R>void compare(const J& rows,unsigned index,bool literal,bool local,void(*bound)(B&),void(*reference)(R&),
        External primary,External model,Counts& count) {
    for(const auto& row:rows) {
        Banks seed;for(const auto& bank:row.at("banks"))seed.emplace(bank.at("address"),bytes(bank.at("before")));
        const std::array selector{0x510f34u,0x510f5cu,0x510f80u};
        for(auto address:selector)for(unsigned n=0;n<(address==0x510f80u?1u:4u);++n)seed.at(0x510000u)[address-0x510000u+n]=std::byte{};
        const auto abi=input(row);
        constexpr std::array modes{Mode::quiet,Mode::empty,Mode::trace_after_load,Mode::remove_after_load,Mode::replace_after_child,
            Mode::write_push_fault,Mode::observer_push_fault,Mode::observer_install,Mode::observer_clear,
            Mode::foreign_exception,Mode::read_fault,Mode::vector_half_fault};
        for(auto mode:modes) {
            const auto want=execute(seed,abi,mode,reference,primary,model);
            const auto got=execute(seed,abi,mode,bound,primary,model);
            if(want.banks!=got.banks||want.abi!=got.abi||want.io!=got.io||want.callbacks!=got.callbacks||
               want.stores!=got.stores||want.external!=got.external||want.fault!=got.fault||want.foreign!=got.foreign||want.changed!=got.changed) {
                std::cerr<<"expected "<<want.abi.dump()<<" actual "<<got.abi.dump()<<'\n';
                for(unsigned n=0;n<std::min(want.io.size(),got.io.size());++n)if(want.io[n]!=got.io[n]) {
                    std::cerr<<"I/O "<<n<<" expected "<<want.io[n].dump()<<" actual "<<got.io[n].dump()<<'\n';break;
                }
                throw std::runtime_error("Format-four source differs, variant "+std::to_string(index)+" mode "+
                    std::to_string(unsigned(mode))+" row "+row.at("name").get<std::string>());
            }
            const bool expected_native=!owner::reference()&&(literal||mode==Mode::empty);
            check(got.activity.calls==unsigned(expected_native),"Wrong real 293290 source entry policy");
            if(expected_native&&mode==Mode::quiet)check(got.activity.completed==1&&got.suffix.empty(),"Normal 293290 source kept retained statements");
            if(!expected_native)check(!got.suffix.empty(),"Reference 293290 source did not run its authentic body");
            if(!local&&expected_native&&mode==Mode::replace_after_child&&!got.external.empty())
                check(got.activity.continuations==1,"CombatAction decoder missed returned-provider policy change");
            count.comparisons++;count.native+=got.activity.calls;count.children+=got.activity.children;
            count.handoffs+=got.activity.continuations;count.retained+=got.suffix.size();count.required+=got.external.size();
            count.faults+=got.fault;count.foreign+=got.foreign;
        }
    }
}
}

int main(int argc,char** argv)try {
    check(argc==2,"Supply exact format-four source fixture");std::ifstream file(argv[1]);const auto rows=J::parse(file);
    const auto host=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{host};
    Counts count;
#define FORMAT_RUN(N,L,P) compare(rows,N,L,P,&format_bound_##N::execute,&format_reference_##N::execute, \
        &format_primary_child_##L::execute,&format_model_child_##L::execute,count); \
        count.refusals+=refusals(&format_bound_##N::run_293290,L);
    FORMAT_FOUR_VARIANTS(FORMAT_RUN)
#undef FORMAT_RUN
    check(count.comparisons&&count.required&&count.faults&&count.foreign&&count.refusals,"Missing real format-four source witness");
    check(owner::reference()?(!count.native&&!count.handoffs):(count.native&&count.children&&count.handoffs),"Missing format-four native/control witness");
    std::cout<<J{{"comparisons",count.comparisons},{"native_entries",count.native},{"native_child_calls",count.children},
        {"continuations",count.handoffs},{"retained_statements",count.retained},{"genuine_external_children",count.required},
        {"provider_exceptions",count.faults},{"foreign_exceptions",count.foreign},{"untouched_refusals",count.refusals}}.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
