#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
struct ObservedWorldState:WorldState {using WorldState::WorldState;bool selected_observation_pcs{true};};
#include "mopp_aabb_bodies.inc"
namespace {
struct ChildBoundary {};
struct ReturnBoundary {};
struct ReadBoundary {};
struct WriteBoundary {};
struct ObserverBoundary {};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>bytes(const std::string&text){std::vector<std::byte>out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
template<class State>J packet(const State&s,std::uint32_t pc){
    const auto a=s.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
}
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;std::uint32_t*pc{};mutable J operations=J::array();
    int fault{-1},write_fault{-1};mutable unsigned read_ordinal{};unsigned write_ordinal{};bool store_preimages{true},stop_return{};
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped MOPP caller input "+std::to_string(p));--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"MOPP caller access escaped bank");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(stop_return&&(*pc==0x309e9eu||*pc==0x30639eu))throw ReturnBoundary{};
        const bool code=*pc==0x309960u||*pc==0x309967u||*pc==0x305e60u||*pc==0x305e67u;
        if(!code){if(int(read_ordinal++)==fault)throw ReadBoundary{};
            operations.push_back(J{{"kind","read"},{"instruction",*pc},{"address",p},{"bytes",out.size()}});}
        const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>data)override{
        if(int(write_ordinal++)==write_fault)throw WriteBoundary{};
        const auto i=at(p,data.size());auto&bank=banks.at(i->first);operations.push_back(J{{"kind","write"},{"instruction",*pc},{"address",p},{"before",hex(std::span(bank).subspan(p-i->first,data.size()))},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),bank.begin()+p-i->first);
    }
    std::pair<J,J>architectural_io()const{
        J reads=J::array(),writes=J::array();
        for(std::size_t n=0;n<operations.size();++n){auto op=operations[n];
            if(op.at("kind")=="read"){
                // The retained store-observer API reads one preimage. It is
                // a distinct SourceMemory contract, not an x86 load. Keep an
                // earlier RMW/source read even when its address is identical.
                if(store_preimages&&n+1<operations.size()){
                    const auto&next=operations[n+1];
                    if(next.at("kind")=="write"&&op.at("instruction")==next.at("instruction")&&op.at("address")==next.at("address")&&op.at("bytes").get<unsigned>()==next.at("written").get_ref<const std::string&>().size()/2)continue;
                }
                op.erase("kind");reads.push_back(op);
            }else{op.erase("kind");writes.push_back(op);}
        }
        return{reads,writes};
    }
};
SourcePhysicsWorldRuntimeAbi incoming(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");const auto fp=row.at("before_fp");
    a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};a.x87_status=fp[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],v.data(),10);}return a;
}
template<class State>struct Routes {
    static constexpr bool world=std::is_same_v<State,ObservedWorldState>;
    static unsigned&fallbacks(){if constexpr(world)return copies::fallback_calls;else return canonical_copies::fallback_calls;}
    static auto&owner(){if constexpr(world)return copies::owner;else return canonical_copies::owner;}
    template<class Call,class Before>static bool production_direct(State&s,Call call,Before before){
        if constexpr(world)return native_mopp_aabb_abi_detail::bridge(s,call,before);
        else return native_mopp_aabb_abi_detail::bridge_canonical(s,call,before);
    }
    template<class Call,class Before>static bool direct(State&s,Call call,Before before){
        return production_direct(s,call,before);
    }
    static void reference(State&s){if constexpr(world)copies::reference::run_305e40(s);else canonical_copies::reference::run_305e40(s);}
    static void bound(State&s,unsigned route){
        if constexpr(world){if(route==1)copies::bound::run_305e40(s);else copies::specialized::run_305e40(s);}
        else{if(route==1)canonical_copies::bound::run_305e40(s);else canonical_copies::specialized::run_305e40(s);}
    }
};
template<class State>unsigned refusals(const J&row){
    for(unsigned variant=0;variant<9;++variant){
        std::uint32_t pc=0x11223344u;Memory memory;memory.pc=&pc;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto at){pc=at;};bool trace=variant==0,second_trace=variant==6;constexpr std::array ordinary{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f590u,0x309940u};
        services.instruction_cursor={&pc,&trace,&second_trace,ordinary,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(variant==1)services.instruction_cursor.executor={};
        if(variant==2){auto original=services.instruction;services.instruction=[original](auto at){original(at);};}
        if(variant==3)services.instruction_cursor.position=nullptr;
        if(variant==4)services.instruction_cursor.service_sites=wrong;
        if(variant==7)services.instruction_cursor.observation_only_stores=false;
        if(variant==8)services.instruction={};
        SourcePhysicsContactRecordObserver observer=[](const auto&){};
        State state{memory,observer,services,variant==5?static_cast<SourcePhysicsContactImage>(0xffu):SourcePhysicsContactImage::TitleUpdate15};state.load(incoming(row));
        const auto before=packet(state,pc);const auto old_calls=native_mopp_aabb_abi_detail::activity().calls;
        check(!Routes<State>::production_direct(state,[](auto&,auto,auto,bool){throw std::runtime_error("Refused MOPP called child");},native_mopp_aabb_abi_detail::Noop{}),"Unsafe MOPP observer/image admitted");
        check(packet(state,pc)==before&&memory.operations.empty()&&native_mopp_aabb_abi_detail::activity().calls==old_calls,"MOPP refusal changed caller state");
    }
    return 9;
}
}
template<class CurrentState>J compare(const J&fixture){
    using Route=Routes<CurrentState>;unsigned comparisons{},io_packets{},child_entries{};
    const bool legacy=fixture.value("legacy_image",false);
    const bool forced=native_mopp_aabb_abi_detail::reference_mode();
    const auto admission_refusals=refusals<CurrentState>(fixture.at("cases")[0]);
    const auto io_sites=fixture.at("io_sites").get<std::set<std::uint32_t>>();
    for(bool selected:{true,false})for(unsigned route=0;route<3;++route)for(const auto&original:fixture.at("cases")){
        if constexpr(!Route::world)if(selected)continue;
        auto row=original;
        if(!selected&&!legacy){
            for(auto&event:row["calls"])event["entry"]=event["entry"].get<std::uint32_t>()-0x3b00u;
            for(const auto*kind:{"reads","writes"})for(auto&event:row[kind])event["instruction"]=event["instruction"].template get<std::uint32_t>()-0x3b00u;
            const auto last=row["expected_pc"].get<std::uint32_t>();if(last>=0x309940u&&last<=0x309f2cu)row["expected_pc"]=last-0x3b00u;
        }
        // Physical write/observer exception semantics are qualified only for
        // native admission; the retained oracle keeps its historical protocol.
        if(forced&&row.contains("fault")&&row.at("fault").at("phase")!="before_read")continue;
        std::uint32_t pc{};Memory memory;memory.pc=&pc;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.banks.emplace(fixture.at("reference_tables").at("address"),bytes(fixture.at("reference_tables").at("bytes")));
        memory.stop_return=row.value("stop_kind",std::string{})=="before_return";
        if(row.contains("fault")){
            const auto phase=row.at("fault").at("phase").get<std::string>();
            if(phase=="before_read")memory.fault=row.at("fault").at("ordinal");
            if(phase=="before_write")memory.write_fault=row.at("fault").at("ordinal");
            memory.store_preimages=phase=="after_write";
        }
        SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};CurrentState*active{};std::function<void(CurrentState&,std::uint32_t)>pre;
        services.instruction=[&](auto at){pc=at;++callbacks;const auto actual=selected&&!legacy?at:at+0x3b00u;if(pre&&io_sites.contains(actual)&&!((actual==0x309d6cu||actual==0x309d98u||actual==0x309dd6u||actual==0x309e4au)&&active->gp[1]==0))pre(*active,at);};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        bool trace=forced;services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        unsigned observed{};SourcePhysicsContactRecordObserver observer=[&](const auto&){if(row.contains("fault")&&row.at("fault").at("phase")=="after_write"&&observed++==row.at("fault").at("ordinal").get<unsigned>())throw ObserverBoundary{};};
        if(!memory.store_preimages)observer={};
        CurrentState state{memory,observer,services,legacy?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};if constexpr(Route::world)state.selected_observation_pcs=selected;state.load(incoming(row));J packets=J::array();
        const auto before=[&](auto&current,std::uint32_t at){
            if(row.value("stop_kind",std::string{})=="before_return"&&at==(selected&&!legacy?0x309e9eu:0x30639eu))throw ReturnBoundary{};
            packets.push_back(packet(current,at));
        };
        active=&state;pre=before;
        std::function<void(CurrentState&,std::uint32_t,std::uint32_t,bool)>call;
        call=[&](auto&current,std::uint32_t target,std::uint32_t,bool){
            if(target==(legacy?0x305e40u:0x309940u)){if(!Route::direct(current,call,before)){++Route::fallbacks();Route::reference(current);}return;}
            pc=target;throw ChildBoundary{};
        };
        Route::owner()=call;const auto old_fallbacks=Route::fallbacks();
        std::string stopped="return";try{
            if(route==0){if(!Route::direct(state,call,before)){++Route::fallbacks();Route::reference(state);}}
            else Route::bound(state,route);
        }catch(const ChildBoundary&){stopped="child";++child_entries;}catch(const ReturnBoundary&){stopped="before_return";}catch(const ReadBoundary&){stopped="before_read";}catch(const WriteBoundary&){stopped="before_write";}catch(const ObserverBoundary&){stopped="after_write";}
        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route)+" selected="+std::to_string(selected);const auto&expected_packets=row.at("calls");
        if(route==0||forced){for(std::size_t n=0;n<std::min(packets.size(),expected_packets.size());++n)if(packets[n]!=expected_packets[n])
            throw std::runtime_error(name+": pre-I/O packet "+std::to_string(n)+" differs actual="+packets[n].dump()+" expected="+expected_packets[n].dump());
        check(packets.size()==expected_packets.size(),name+": I/O packet count differs actual="+std::to_string(packets.size())+" expected="+std::to_string(expected_packets.size()));}
        check(stopped==row.at("stop_kind").get<std::string>(),name+": completion differs");
        const auto actual=packet(state,pc);const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")}};
        check(actual==expected,name+": returned/child-entry packet differs actual="+actual.dump()+" expected="+expected.dump());
        const auto[reads,writes]=memory.architectural_io();
        check(reads==row.at("reads"),name+": architectural reads differ actual="+reads.dump()+" expected="+row.at("reads").dump());
        check(writes==row.at("writes"),name+": architectural stores differ actual="+writes.dump()+" expected="+row.at("writes").dump());
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address").get<std::uint32_t>()))==bank.at("after").get<std::string>(),name+": complete caller/data bank differs");
        check(forced?callbacks>0:callbacks==0,name+": parent native/reference routing differs");
        check(forced?Route::fallbacks()>old_fallbacks:Route::fallbacks()==old_fallbacks,name+": retained continuation differs");++comparisons;io_packets+=packets.size();
    }
    return J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",io_packets},{"genuine_child_entries",child_entries},{"admission_refusals",admission_refusals}};
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply physical MOPP ABI fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto world=compare<ObservedWorldState>(fixture),canonical=compare<halo2::physics_contact_runtime_detail::State>(fixture);
    J result{{"passed",true},{"world",world},{"canonical_state",canonical},{"native_calls",native_mopp_aabb_abi_detail::activity().calls}};
    for(const auto*key:{"comparisons","complete_pre_io_packets","genuine_child_entries","admission_refusals"})result[key]=world.at(key).get<unsigned>()+canonical.at(key).get<unsigned>();
    std::cout<<result.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
