#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_copy_source_owner.hpp"

namespace {
using Entry=halo2::native_codec_copy_owner_detail::Entry;
using Activity=halo2::native_codec_copy_owner_detail::Activity;
struct Statements {unsigned count{};std::uint32_t first{};};
std::map<unsigned,Statements> retained;
bool stop_at_retained{};
struct RetainedStop {};
void codec_statement(unsigned route,std::uint32_t pc) {
    auto& row=retained[route];if(!row.count++)row.first=pc;
    if(stop_at_retained)throw RetainedStop{};
}
}
#include "codec_copy_sources.inc"

namespace {
Activity activity(std::uint32_t entry) {
    namespace owner=halo2::native_codec_copy_owner_detail;
    if(entry==0x2963d0u)return owner::activity<Entry::quaternion>();
    if(entry==0x296410u)return owner::activity<Entry::translation>();
    return owner::activity<Entry::scale>();
}
struct SourceMemory:OwnerMemory {
    std::function<void(bool)> feedback;
    mutable unsigned preimages{};
    mutable J preimage_packets=J::array();
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(write_sites.contains(pc)) {
            preimage_packets.push_back(capture());
            if(fault&&fault->at("phase")=="before_preimage"&&preimages++==fault->at("ordinal"))throw Fault{};
        }
        OwnerMemory::read(address,out);if(feedback)feedback(false);
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        OwnerMemory::write(address,in);if(feedback)feedback(true);
    }
};
using Banks=std::map<std::uint32_t,std::vector<std::byte>>;
Banks decode_banks(const J& row) {
    Banks result;for(const auto& bank:row.at("banks"))result.emplace(bank.at("address"),bytes(bank.at("before")));return result;
}
struct Input {const J* row;Banks banks;};
struct Result {J observations;Banks banks;Activity native;Statements statements;};
template<class S>Result execute(const J& row,unsigned route,unsigned mode,bool original,void(*bound)(S&),void(*reference)(S&),const Banks& seed) {
    SourceMemory memory;memory.banks=seed;
    if(row.contains("fault"))memory.fault=row.at("fault");
    const auto entry=row.at("entry").get<std::uint32_t>();
    const bool quaternion=entry==0x2963d0u,scale=entry==0x2cec60u;
    memory.write_sites=quaternion?std::set<std::uint32_t>{0x2963f7u,0x2963fcu,0x296402u,0x296408u}:
        scale?std::set<std::uint32_t>{0x2cec83u}:std::set<std::uint32_t>{0x29643cu,0x296441u,0x296447u};
    const auto first_store=*memory.write_sites.begin();
    const auto output_pc=quaternion?0x2963e7u:scale?0x2cec7eu:0x296433u;
    const auto product_pc=quaternion?0x2963d9u:scale?0x2cec68u:0x296419u;
    const auto return_pc=quaternion?0x29640bu:scale?0x2cec86u:0x29644au;
    const auto frontier=row.value("frontier",0u);
    const bool frontier_store=memory.write_sites.contains(frontier);
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observe;
    S* live{};
    bool trace{},changed{};
    unsigned completed{};
    J callbacks=J::array(),observations=J::array();
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    const auto record=[&](std::uint32_t pc) {memory.pc=pc;callbacks.push_back(packet(*live,pc));};
    auto& cursor=services.instruction_cursor;
    services.instruction=[&](std::uint32_t pc) {
        memory.pc=pc;
        const bool valid=cursor.position&&cursor.callback_type&&*cursor.callback_type==services.instruction.target_type()&&
            cursor.executor==std::this_thread::get_id()&&cursor.literal_service_sites&&std::ranges::equal(cursor.service_sites,sites);
        if(trace||!valid||std::ranges::find(sites,pc)!=sites.end())record(pc);
    };
    cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    const auto observe_write=[&](const SourcePhysicsContactRecordStore& event) {
        observations.push_back(J{{"packet",memory.capture()},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});
        if(mode==7&&event.instruction==first_store) {live->gp[0]+=4u;live->gp[1]+=8u;changed=true;}
        if(memory.fault&&memory.fault->at("phase")=="after_write"&&completed++==memory.fault->at("ordinal"))throw Fault{};
    };
    if(mode!=8&&mode!=9&&mode!=10)observe=observe_write;
    if(mode==8||mode==9) {services.instruction={};cursor={};}
    S state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};live=&state;
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n) {const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    if(row.contains("active_mmx")) {
        SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");
        for(unsigned n=0;n<8;++n) {const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}
        abi.active_mmx=opaque;
    }
    state.load(abi);memory.capture=[&] {return packet(state,memory.pc);};
    memory.feedback=[&](bool write) {
        if(changed)return;
        if(frontier) {
            if(memory.pc==frontier&&frontier_store==write) {trace=true;changed=true;}
            return;
        }
        if(mode>=1&&mode<=5&&!write&&memory.pc==entry) {
            changed=true;
            switch(mode) {
                case 1:trace=true;break;
                case 2:services.instruction=[&,different=1u](std::uint32_t pc){(void)different;record(pc);};break;
                case 3:services.instruction={};break;
                case 4:cursor.position=nullptr;break;
                case 5:throw halo2::native_codec_copy_source_detail::Continuation{0x12345678u,nullptr};
            }
        } else if(mode==6&&!write&&memory.pc==output_pc) {state.gp[2]+=4u;changed=true;}
        else if(mode==9&&!write) {services.instruction=record;changed=true;}
        else if(mode==10&&write&&memory.pc==first_store) {observe=observe_write;changed=true;}
        else if(mode==11&&!write&&memory.pc==first_store) {observe={};changed=true;}
        else if(mode==12&&!write&&memory.pc==product_pc) {state.gp[scale?1u:2u]+=4u;changed=true;}
        else if(mode==13&&write&&memory.pc==first_store) {cursor.observation_only_stores=false;changed=true;}
        else if(mode==14&&!write&&memory.pc==return_pc) {state.gp[4]+=4u;trace=true;changed=true;}
    };
    const auto before=activity(entry);retained[route]={};
    bool fault{},foreign{};
    try {if(original)reference(state);else bound(state);}
    catch(const Fault&) {fault=true;}
    catch(const halo2::native_codec_copy_source_detail::Continuation& error) {
        check(error.pc==0x12345678u&&!error.owner,"Foreign codec copy continuation changed");foreign=true;
    }
    check(fault==row.contains("fault")&&foreign==(mode==5),"Codec copy source changed provider exception identity");
    if(frontier||(mode&&mode!=8))check(changed,"Declared codec copy feedback did not execute");
    const auto after=activity(entry);
    return {J{{"abi",memory.capture()},{"reads",std::move(memory.reads)},{"writes",std::move(memory.writes)},
             {"packets",std::move(memory.packets)},{"preimages",std::move(memory.preimage_packets)},
             {"observers",std::move(observations)},{"callbacks",std::move(callbacks)},
             {"foreign",foreign},{"fault",fault},{"observer_present",bool(observe)}},std::move(memory.banks),
            {after.calls-before.calls,after.completed-before.completed,after.continuations-before.continuations},retained[route]};
}
struct Totals {unsigned comparisons{},calls{},completed{},handoffs{},statements{},frontiers{};};
template<class S>void run(const std::vector<Input>& inputs,unsigned route,std::uint32_t entry,bool literal,void(*bound)(S&),void(*reference)(S&),Totals& totals) {
    const auto first_next=entry==0x2963d0u?0x2963d6u:entry==0x296410u?0x296416u:0x2cec65u;
    const auto after_first_write=entry==0x2963d0u?0x2963f9u:entry==0x296410u?0x29643eu:0x2cec86u;
    const bool force_reference=halo2::native_codec_copy_owner_detail::reference();
    for(const auto& input:inputs) {
        const auto& row=*input.row;
        if(row.at("entry")!=entry)continue;
        const auto& seed=input.banks;
        const auto frontier_next=row.value("next",0u);
        const auto modes=row.contains("fault")||frontier_next?1u:15u;
        for(unsigned mode=0;mode<modes;++mode) {
            const auto expected=execute(row,route,mode,true,bound,reference,seed);
            Result actual;
            try {actual=execute(row,route,mode,false,bound,reference,seed);}
            catch(const std::exception& error) {
                throw std::runtime_error("Codec copy source at route "+std::to_string(route)+" / mode "+
                    std::to_string(mode)+" / "+row.at("name").get<std::string>()+": "+error.what());
            }
            if(expected.observations!=actual.observations||expected.banks!=actual.banks) {
                std::cerr<<"Codec copy source differs at route "<<route<<" / mode "<<mode<<" / "<<row.at("name")<<'\n';
                for(auto it=expected.observations.begin();it!=expected.observations.end();++it)
                    if(it.value()!=actual.observations.at(it.key()))
                        std::cerr<<it.key()<<" expected "<<it.value().dump().substr(0,2400)<<" actual "<<actual.observations.at(it.key()).dump().substr(0,2400)<<'\n';
                if(expected.banks!=actual.banks)std::cerr<<"Backed bank bytes differ\n";
                throw std::runtime_error("Codec copy source packets differ");
            }
            const bool admitted=!force_reference&&(literal||mode==8||mode==9);
            const auto next=frontier_next?frontier_next:mode==13?after_first_write:((mode>=1&&mode<=4)||mode==9)?first_next:0u;
            const bool resumes=admitted&&next;
            check(actual.native.calls==admitted,"Codec copy source default/refusal entry differs");
            check(actual.native.completed==(admitted&&!next&&mode!=5&&!row.contains("fault")),"Codec copy source completion ownership differs");
            check(actual.native.continuations==resumes,"Codec copy source retirement ownership differs");
            check(actual.statements.first==(admitted?(resumes?next:0u):entry),"Codec copy source exact retained continuation PC differs");
            ++totals.comparisons;totals.calls+=actual.native.calls;totals.completed+=actual.native.completed;
            totals.handoffs+=actual.native.continuations;totals.statements+=actual.statements.count;
            totals.frontiers+=frontier_next&&resumes;
        }
    }
}

template<class S>unsigned guards(unsigned route,std::uint32_t entry,void(*bound)(S&)) {
    for(unsigned kind=0;kind<24;++kind) {
        SourceMemory memory;
        SourcePhysicsWorldRuntimeServices services;
        SourcePhysicsContactRecordObserver observe=[](const auto&){};
        unsigned callbacks{};bool trace=true;
        services.instruction=[&](std::uint32_t){++callbacks;};
        std::array sites{0x38846eu,0x21f590u,0x33b595u};
        auto& cursor=services.instruction_cursor;
        cursor={&memory.pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(kind==11)cursor.literal_service_sites=false;
        if(kind==23) {services.instruction={};cursor={};}
        S state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        auto copied=cursor;
        switch(kind) {
            case 0:state.image=SourcePhysicsContactImage::MapPack11;break;
            case 1:state.image=SourcePhysicsContactImage(99);break;
            case 2:state.fp.count=9;break;
            case 3:state.fp.top=8;break;
            case 4:state.fp.status|=0x8080;break;
            case 5:state.active_mmx.emplace();state.fp.count=1;break;
            case 6:services.instruction={};break;
            case 7:services.instruction=[different=1u](std::uint32_t){(void)different;};break;
            case 8:cursor.position=nullptr;break;
            case 9:cursor.executor={};break;
            case 10:cursor.literal_service_sites=false;break;
            case 11:cursor.literal_service_sites=true;break;
            case 12:state.instruction_cursor=&copied;break;
            case 13:cursor.callback_type=nullptr;break;
            case 14:sites[2]^=1u;break;
            case 15:cursor.service_sites=std::span(sites).first(2);break;
            case 16:cursor.first_trace=&trace;break;
            case 17:cursor.second_trace=&trace;break;
            case 18:cursor.observation_only_stores=false;break;
            case 19:cursor.callback_type=&typeid(int);break;
            case 20:state.instruction_cursor=nullptr;break;
            case 21:services.instruction={};state.instruction_cursor=nullptr;break;
            case 22:state.active_mmx.emplace();state.fp.top=1;break;
            case 23:break;
        }
        memory.pc=0xdeadbeefu;
        const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,xmm.size());
        const auto flags=state.flags.bits(),other=state.other_flags,depth=state.fp.count,top=state.fp.top;
        const auto status=state.fp.status;const bool opaque=bool(state.active_mmx);
        const auto host=native_raw_x87_detail::capture();const auto before=activity(entry);
        retained[route]={};stop_at_retained=true;bool stopped{};
        try {bound(state);}catch(const RetainedStop&) {stopped=true;}
        stop_at_retained=false;
        check(stopped&&retained[route].first==entry&&retained[route].count==1&&activity(entry).calls==before.calls,
              "Codec copy guard did not enter untouched authored fallback");
        check(state.gp==gp&&std::memcmp(xmm.data(),state.xmm,xmm.size())==0&&state.flags.bits()==flags&&state.other_flags==other&&
              state.fp.count==depth&&state.fp.top==top&&state.fp.status==status&&bool(state.active_mmx)==opaque,
              "Codec copy guard changed entry ABI");
        check(memory.pc==0xdeadbeefu&&!callbacks&&memory.reads.empty()&&memory.writes.empty()&&
              native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Codec copy guard changed entry I/O or host FP");
    }
    return 24;
}
}

int main(int argc,char** argv)try {
    const auto host=native_raw_x87_detail::capture();
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{host};
    check(argc==2,"Supply codec copy source inputs or --guards");
    if(std::string_view(argv[1])=="--guards") {
        unsigned checks{};
#define CODEC_GUARD(N,E,L) checks+=guards(N,E,&codec_copy_source##N::bound);
        CODEC_COPY_ROUTES(CODEC_GUARD)
#undef CODEC_GUARD
        std::cout<<J{{"passed",true},{"untouched_refusals",checks}}.dump()<<'\n';return 0;
    }
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    std::vector<Input> inputs;inputs.reserve(fixture.at("cases").size());
    for(const auto& row:fixture.at("cases"))inputs.push_back({&row,decode_banks(row)});
    Totals totals;
#define CODEC_RUN(N,E,L) run(inputs,N,E,L,&codec_copy_source##N::bound,&codec_copy_source##N::original,totals);
    CODEC_COPY_ROUTES(CODEC_RUN)
#undef CODEC_RUN
    std::cout<<J{{"passed",true},{"source_routes",CODEC_COPY_COUNT},{"source_comparisons",totals.comparisons},
        {"native_calls",totals.calls},{"completed_native",totals.completed},{"exact_continuations",totals.handoffs},
        {"frontier_handoffs",totals.frontiers},{"retained_statements",totals.statements},{"normal_modes",15},
        {"banks_predecoded_once_per_input",true}}.dump()<<'\n';return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
