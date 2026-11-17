// Reuse the qualified ABI packet/backing helpers, never the source algorithm.
#define main integer_scope_unused_parent_mode_test_main
#include "native_tu15_object_parent_mode_tests.cpp"
#undef main
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_required_abi_fp.hpp"
#include "gameplay/weapons/native_tu15_marker_owner_admission.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"

namespace {
struct ChildContext{
    const SourcePhysicsWorldRuntimeServices*services{};
    const SourcePhysicsContactRecordObserver*observe{};
    SourcePhysicsWorldRuntimeAbi**live{},*effect{};
    unsigned*entries{},*effect_aliases{};bool referenced{},hostile_return{};
};
struct ReadonlyChild{
    const ChildContext*context{};
    void operator()(SourcePhysicsContactMemory&memory,const SourcePhysicsContactRuntimeCall&call,
        SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsContactRecordObserver&observe)const{
        const auto&c=*context;
        check(call.target==0xfc510u&&call.instruction==0xbad39u&&!call.tail,"Integer child changed required descriptor");
        check(&observe==c.observe,"Integer child wrapped the original observer");
        if(c.referenced){check(&abi==c.effect,"Integer child copied the referenced Effect ABI");++*c.effect_aliases;}
        ++*c.entries;auto*previous=*c.live;*c.live=&abi;
        try{native_weapon_effect_dispatch_tu15_run(memory,call.target,abi,*c.services,observe);}
        catch(...){*c.live=previous;if(c.hostile_return)hostile();throw;}
        *c.live=previous;if(c.hostile_return)hostile();
    }
    static bool matches(const SourcePhysicsContactRuntimeServices::RequiredOwner&callback,const void*owner)noexcept{
        const auto*actual=callback.target<ReadonlyChild>();return actual&&actual->context==owner;
    }
};
void seed_host(unsigned index) {
    auto image=native_raw_x87_detail::capture();const auto top=index&7u;
    image.word(4,std::uint16_t(top<<11u));image.word(8,0xffffu);
    constexpr std::array<unsigned,7>counts{0,1,4,8,8,8,8};
    constexpr std::array<std::array<unsigned char,10>,6>values{{
        {{0,0,0,0,0,0,0,0x80,0xff,0x3f}},
        {{0,0,0,0,0,0,0,0,0,0x80}},
        {{0x35,0xc2,0x68,0x21,0xa2,0xda,0x0f,0xc9,0,0x40}},
        {{1,0,0,0,0,0,0,0,0,0}},
        {{0x45,0x23,1,0,0,0,0,0xc0,0xff,0x7f}},
        {{0x56,0x34,2,0,0,0,0,0x80,0xff,0x7f}}
    }};
    const auto shape=(index>>3u)%counts.size();
    for(unsigned n=0;n<8;++n) {
        std::array<std::byte,10>payload{};std::memcpy(payload.data(),values[(index+n)%values.size()].data(),10);
        std::memcpy(image.x87.data()+28u+n*10u,payload.data(),10);
        const bool live=shape==4u?n%2u==0:shape==5u?n%3u!=0:n<counts[shape];
        if(live){const auto shift=((top+n)&7u)*2u;const auto tag=shape==6u?0u:native_raw_x87_detail::value_tag(payload.data());image.word(8,std::uint16_t((image.word(8)&~(3u<<shift))|(tag<<shift)));}
    }
    native_raw_x87_detail::restore(image);
}
}
int main(int argc,char**argv)try {
    bool hostile_required{},fenced_required{};
    for(int n=2;n<argc;++n){
        const std::string_view option=argv[n];
        if(option=="--hostile-required")hostile_required=true;
        else if(option=="--fenced-required")fenced_required=true;
        else throw std::runtime_error("Unknown integer scope option");
    }
    check(argc>=2,"Supply genuine FC510 physical fixture");
    std::ifstream input_file(argv[1]);const auto fixture=J::parse(input_file);
    const auto original_host=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{original_host};
    unsigned comparisons{},packets{},io_checks{},world_mmx_refusals{},children{},required_entries{},effect_aliases{};
    for(const auto&row:fixture.at("cases"))for(unsigned mode=0;mode<2;++mode)for(unsigned transport=0;transport<3;++transport) {
        seed_host(comparisons);
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        if(row.contains("fault"))memory.fault=row.at("fault");memory.noisy=mode!=0;
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
        unsigned callbacks{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[&](auto pc){++callbacks;memory.pc=pc;};
        services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp=row.at("gp");input.xmm=row.at("xmm");input.eflags=row.at("eflags");
        input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];
        input.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<input.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}
        if(row.contains("active_mmx")) {
            SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");
            for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}input.active_mmx=opaque;
        }
        state.load(input);const auto host=native_raw_x87_detail::capture();const auto before=packet(state,0);
        auto effect_abi=input;SourcePhysicsWorldRuntimeAbi*live_child{};
        memory.capture=[&]{return live_child?packet(*live_child,memory.pc):packet(state,memory.pc);};bool stopped{};
        {
            native_integer_fp_detail::Scope owner{state};
            check(native_raw_x87_detail::capture().word(8)==0xffffu,"Integer scope did not park host registers");
            check(packet(state,0)==before,"Integer scope interpreted source payloads");
            for(unsigned phase=0;phase<2;++phase) {
                const auto parked=native_raw_x87_detail::capture();bool failed{};
                try{owner.io([&]{hostile();if(phase)throw Fault{};});}catch(const Fault&){failed=true;}
                check(failed==bool(phase)&&native_raw_x87_detail::equivalent(parked,native_raw_x87_detail::capture()),"Integer IO lost host FP or exception identity");
                check(packet(state,0)==before,"Integer IO modified opaque source state");++io_checks;
            }
            if(input.active_mmx) {
                for(bool present:{false,true}){
                    unsigned invoked{};
                    if(present)services.required_owner=[&](auto&,const auto&,auto&,const auto&){++invoked;};
                    bool refused{};try{state.required(0xbad39u,0xfc510u,false);}catch(const std::runtime_error&){refused=true;}
                    check(refused&&!invoked&&memory.reads.empty()&&packet(state,0)==before,"World active-MMX required refusal changed");++world_mmx_refusals;
                    services.required_owner={};
                }
            }
            const ChildContext child_context{&services,&observe,&live_child,&effect_abi,&required_entries,&effect_aliases,transport==2,hostile_required};
            if(transport){services.required_owner=ReadonlyChild{&child_context};services.required_owner_policy={&typeid(ReadonlyChild),std::this_thread::get_id(),&child_context,&ReadonlyChild::matches};}
            try{owner.child([&]{
                ++children;
                if(!transport)native_object_parent_mode_detail::run_private(state,[&](auto&,auto pc){memory.pc=pc;});
                else if(transport==1){
                    if(!fenced_required)native_marker_owner_detail::actual_child(state,0xfc510u,0xbad39u);
                    else{
                        auto actual=state.save();
                        native_required_abi_fp_detail::invoke(state,actual,[&]{services.required_owner(memory,{0xbad39u,0xfc510u,false},actual,state.observe);});
                    }
                }
                else{
                    effect_abi=state.save();
                    if(fenced_required)native_required_abi_fp_detail::invoke(state,effect_abi,[&]{services.required_owner(memory,{0xbad39u,0xfc510u,false},effect_abi,observe);});
                    else{
                        try{services.required_owner(memory,{0xbad39u,0xfc510u,false},effect_abi,observe);}
                        catch(...){state.load(effect_abi);throw;}
                        state.load(effect_abi);
                    }
                }
            });}
            catch(const Fault&){stopped=true;}
        }
        check(stopped==row.contains("fault"),"Integer child changed original completion");
        check(!callbacks,"Integer child executed retained instruction callbacks");
        const auto name=row.at("name").get<std::string>();
        check(packet(state,memory.pc)==row.at("expected"),name+": integer child full ABI differs");
        check(memory.reads==row.at("reads")&&memory.packets==row.at("calls"),name+": integer child read/packet protocol differs");
        auto expected_host=host;const auto returned=state.save();expected_host.word(0,returned.floating_point->x87_control_word);
        expected_host.word(4,std::uint16_t((returned.x87_status&~0x3800u)|(host.word(4)&0x3800u)));expected_host.mxcsr=returned.floating_point->mxcsr;
        check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),"Integer scope lost host payload/tags/TOP or source status/control");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),"Integer child changed banks");
        packets+=memory.packets.size();++comparisons;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"hostile_io_return_throw_checks",io_checks},{"genuine_native_fc510_children",children},{"genuine_required_provider_entries",required_entries},{"declared_reference_abi_handoffs",effect_aliases},{"world_active_mmx_required_refusals",world_mmx_refusals},{"fenced_required_prototype",fenced_required},{"whole_parent_default_qualified",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
