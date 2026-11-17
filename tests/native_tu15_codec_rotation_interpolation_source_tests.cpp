#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_rotation_interpolation_source_owner.hpp"
namespace{
std::array<unsigned,12>codec_statements{};bool codec_stop{};
struct CodecRetainedStop{};
void codec_statement(unsigned owner){++codec_statements[owner];if(codec_stop)throw CodecRetainedStop{};}
struct EstimateContext{std::function<std::uint32_t(std::uint32_t)>invoke;};
struct EstimateProvider{EstimateContext*context;std::uint32_t operator()(std::uint32_t input)const{auto*const owner=context;return owner->invoke(input);}};
struct ReturnedEstimateProvider{EstimateContext*context;std::uint32_t operator()(std::uint32_t input)const{auto*const owner=context;return owner->invoke(input);}};
}
#include "codec_rotation_interpolation_sources.inc"
namespace{
struct SourceMemory:OwnerMemory{
    mutable std::function<void(bool)>feedback;
    mutable J preimages=J::array(),preimage_packets=J::array();mutable unsigned preimage_count{};
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(write_sites.contains(pc)){
            preimage_packets.push_back(capture());
            if(fault&&fault->at("phase")=="before_preimage"&&preimage_count==fault->at("ordinal"))throw Fault{};
            ++preimage_count;preimages.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});
        }
        OwnerMemory::read(address,out);if(feedback)feedback(false);
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{OwnerMemory::write(address,in);if(feedback)feedback(true);}
};
using CodecBanks=std::map<std::uint32_t,std::vector<std::byte>>;
CodecBanks decoded_banks(const J&row){CodecBanks result;for(const auto&bank:row.at("banks"))result.emplace(bank.at("address"),bytes(bank.at("before")));return result;}
template<class S>J execute(const J&row,unsigned variant,unsigned mutation,bool reference,void(*bound)(S&),void(*original)(S&),const CodecBanks*seed=nullptr){
    SourceMemory memory;memory.banks=seed?*seed:decoded_banks(row);memory.write_sites={0x29626au,0x29627du,0x296283u,0x296287u,0x2962cfu};if(row.contains("fault"))memory.fault=row.at("fault");
    SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;S*current{};bool trace{},changed{};unsigned observers{};J callbacks=J::array(),estimate_packets=J::array();unsigned estimates{};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},other{0x38846eu,0x21f590u,0x2962cfu};
    const auto record=[&](std::uint32_t pc){memory.pc=pc;if(current)callbacks.push_back(packet(*current,pc));};
    services.instruction=[&](std::uint32_t pc){memory.pc=pc;const auto&c=services.instruction_cursor;const bool valid=c.position&&c.callback_type&&*c.callback_type==services.instruction.target_type()&&c.executor==std::this_thread::get_id()&&c.literal_service_sites&&c.service_sites.size()==3&&c.service_sites[0]==sites[0]&&c.service_sites[1]==sites[1]&&c.service_sites[2]==sites[2];if(trace||!valid||std::find(c.service_sites.begin(),c.service_sites.end(),pc)!=c.service_sites.end())record(pc);};
    auto&c=services.instruction_cursor;c={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    observe=[&](const auto&event){
        if(event.instruction==0x2962cfu&&mutation==11)current->gp[2]+=32u;
        if(memory.fault&&memory.fault->at("phase")=="after_write"&&observers==memory.fault->at("ordinal"))throw Fault{};
        ++observers;
    };
    EstimateContext estimate;
    estimate.invoke=[&](std::uint32_t input){
        estimate_packets.push_back(packet(*current,memory.pc));++estimates;
        auto value=_mm_castsi128_ps(_mm_set_epi32(0,0,0,std::bit_cast<std::int32_t>(input)));
        asm volatile("rsqrtss %1,%0":"=x"(value):"x"(value):"memory");
        const auto result=std::bit_cast<std::array<std::uint32_t,4>>(value)[0];
        if(estimates==1&&((mutation>=20&&mutation<=25)||mutation==30||mutation==32)){
            changed=true;
            switch(mutation){
            case 20:trace=true;break;
            case 21:services.instruction={};break;
            case 22:services.instruction=[&,different=2u](std::uint32_t next){(void)different;record(next);current->xmm[2]=_mm_set1_ps(0.5f);};break;
            case 23:services.reciprocal_sqrt={};break;
            case 24:throw Fault{};
            case 25:services.reciprocal_sqrt=ReturnedEstimateProvider{&estimate};break;
            case 30:services.reciprocal_sqrt=EstimateProvider{&estimate};break;
            case 32:services.instruction=[&,different=3u](std::uint32_t next){(void)different;record(next);};break;
            }
        }
        return result;
    };
    services.reciprocal_sqrt=EstimateProvider{&estimate};
    if(mutation==16||mutation>=31){services.instruction={};c={};observe={};}
    if(mutation==18)observe={};
    if(mutation==17)observe={};
    S state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    if(row.contains("active_mmx")){SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}abi.active_mmx=opaque;}
    state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};
    memory.feedback=[&](bool write){
        if(changed||!mutation||mutation==16||mutation==17||(mutation>=20&&mutation<=25)||mutation==30||mutation==32)return;const auto pc=memory.pc;
        if(mutation==31||mutation==33){
            if(write!=(mutation==33))return;changed=true;
            if(mutation==31)services.instruction=[&,different=4u](std::uint32_t next){(void)different;record(next);};
            else observe=[&](const auto&event){callbacks.push_back(packet(*current,event.instruction));};
            return;
        }
        const bool trigger=mutation>=28?(!write&&pc==0x2962c9u):mutation>=26?(!write&&pc==0x29629cu):mutation==18?(write&&pc==0x29626au):mutation==19?(!write&&pc==0x29626au):(mutation<=8||mutation==14||mutation==15)?(!write&&pc==0x296243u):mutation==9?(!write&&pc==0x2962c9u):mutation==10||mutation==11?(write&&pc==0x2962cfu):mutation==12?(!write&&pc==0x296248u):(!write&&pc==0x296258u);
        if(!trigger)return;changed=true;
        switch(mutation){case 1:trace=true;break;case 2:services.instruction={};break;case 3:services.instruction=[&,different=1u](std::uint32_t next){(void)different;record(next);};break;case 4:c.position=nullptr;break;case 5:c.executor={};break;case 6:c.literal_service_sites=false;break;case 7:c.callback_type=nullptr;break;case 8:c.service_sites=other;break;case 9:state.gp[2]+=32u;break;case 10:state.gp[1]+=16u;break;case 11:c.observation_only_stores=false;break;case 12:state.gp[0]+=4u;break;case 13:state.gp[1]+=4u;break;case 14:state.xmm[7]=_mm_castsi128_ps(_mm_set_epi32(0x7fc12345,17,-1,0));state.other_flags^=0x400u;break;case 15:throw native_codec_rotation_interpolation_detail::SourceContinuation{0x296248u,nullptr};case 18:observe=[&](const auto&event){callbacks.push_back(packet(*current,event.instruction));};break;case 19:observe={};break;case 26:state.xmm[1]=_mm_set1_ps(0.25f);break;case 27:state.xmm[3]=_mm_set_ss(0.75f);break;case 28:state.xmm[2]=_mm_set1_ps(0.5f);break;case 29:state.xmm[0]=_mm_set1_ps(0.25f);break;default:throw std::runtime_error("Unknown rotation interpolation source mutation");}
    };
    const auto before=native_codec_rotation_interpolation_owner_detail::activity();const auto statements=codec_statements[variant];bool foreign{},stopped{},missing_provider{};
    try{if(reference)original(state);else bound(state);}catch(const native_codec_rotation_interpolation_detail::SourceContinuation&error){check(error.pc==0x296248u&&!error.owner,"Foreign codec provider exception changed identity");foreign=true;}catch(const Fault&){stopped=true;}catch(const std::runtime_error&error){if(std::string_view(error.what())!="Required original contact RSQRTSS owner unavailable")throw;missing_provider=true;}
    check(foreign==(mutation==15),"Rotation interpolation source swallowed/invented a provider exception");
    check(stopped==(row.contains("fault")||mutation==24)&&missing_provider==(mutation==23),"Rotation interpolation source failed to reach declared retained provider boundary");
    J banks=J::array();for(const auto&bank:row.at("banks"))banks.push_back(hex(memory.banks.at(bank.at("address"))));const auto after=native_codec_rotation_interpolation_owner_detail::activity();
    return J{{"abi",packet(state,memory.pc)},{"reads",memory.reads},{"writes",memory.writes},{"preimages",memory.preimages},{"preimage_packets",memory.preimage_packets},{"packets",memory.packets},{"banks",banks},{"callbacks",callbacks},{"estimate_packets",estimate_packets},{"actual_estimates",estimates},{"foreign_exception",foreign},{"stopped",stopped},{"missing_provider",missing_provider},{"changed",changed},{"calls",after.calls-before.calls},{"completed",after.completed-before.completed},{"continuations",after.continuations-before.continuations},{"statements",codec_statements[variant]-statements}};
}
template<class S>void owner(const J&fixture,unsigned variant,bool literal,void(*bound)(S&),void(*reference)(S&),unsigned&comparisons,unsigned&handoffs,unsigned&statements){
    for(const auto&row:fixture.at("cases")){const auto seed=decoded_banks(row);
      for(unsigned mutation=0;mutation<(row.contains("fault")?1u:34u);++mutation){
        J expected=execute(row,variant,mutation,true,bound,reference,&seed),actual;
        try{actual=execute(row,variant,mutation,false,bound,reference,&seed);}
        catch(const std::exception&error){throw std::runtime_error("Rotation interpolation source continuation differs: "+std::to_string(variant)+" / "+std::to_string(mutation)+" / "+row.at("name").get<std::string>()+": "+error.what());}
        if(native_codec_rotation_interpolation_owner_detail::reference())check(actual.at("calls")==0,"Own codec reference entered native prefix");
        else if(literal||mutation==16||mutation>=31){check(actual.at("calls")==1,"Rotation interpolation source native prefix was not entered");if(!mutation){
            if(row.value("retained_unaligned",false))check(actual.at("continuations").get<unsigned>()==1&&actual.at("statements").get<unsigned>()>0,"Unaligned retained source did not hand off before vector access");
            else check(actual.at("completed")==unsigned(!row.contains("fault"))&&actual.at("statements")==0,"Rotation interpolation source normal route retained instructions");}}
        else check(actual.at("calls")==0&&actual.at("statements").get<unsigned>()>0,"Raw interpolation source lost its retained callbacks");
        handoffs+=actual.at("continuations").get<unsigned>();statements+=actual.at("statements").get<unsigned>();
        for(const char*key:{"calls","completed","continuations","statements"}){expected.erase(key);actual.erase(key);}
        if(actual!=expected){
            std::string diagnostic;
            for(auto it=expected.begin();it!=expected.end();++it)if(actual.at(it.key())!=it.value()){
                diagnostic+="\n"+it.key()+": ";
                if(it.key()=="banks"){
                    for(unsigned n=0;n<it.value().size();++n){const auto left=it.value()[n].get<std::string>(),right=actual.at("banks")[n].get<std::string>();if(left!=right){std::size_t offset{};while(offset<std::min(left.size(),right.size())&&left[offset]==right[offset])++offset;diagnostic+=std::to_string(n)+" offset "+std::to_string(offset)+" expected "+left.substr(offset,64)+" actual "+right.substr(offset,64);}}
                }else diagnostic+="expected "+it.value().dump().substr(0,4000)+" actual "+actual.at(it.key()).dump().substr(0,4000);
            }
            throw std::runtime_error("Rotation interpolation source continuation differs: "+std::to_string(variant)+" / "+std::to_string(mutation)+" / "+row.at("name").get<std::string>()+diagnostic);
        }++comparisons;
      }
    }
}
template<class S>unsigned guards(unsigned variant,bool literal,void(*bound)(S&)){
    unsigned checks{};for(unsigned kind=0;kind<29;++kind){
        SourceMemory memory;unsigned callbacks{};SourcePhysicsContactRecordObserver observe=[](const auto&){};
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](std::uint32_t){++callbacks;};services.reciprocal_sqrt=[](std::uint32_t)->std::uint32_t{throw std::runtime_error("Guard reciprocal provider must not execute");};
        bool trace=true;std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&c=services.instruction_cursor;
        c={&memory.pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};if(kind==11)c.literal_service_sites=false;if(kind==28){services.instruction={};c={};}
        S state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.fp.count=0;state.fp.top=0;state.fp.status=0;
        if(kind!=11&&kind!=28&&!native_codec_rotation_interpolation_owner_detail::reference())check(literal?native_codec_rotation_interpolation_owner_detail::admitted<true>(state):!native_codec_rotation_interpolation_owner_detail::admitted<false>(state),"Rotation interpolation source format admission differs");
        auto copied=c;std::array extra{sites[0],sites[1],sites[2],sites[2]};const auto host=native_raw_x87_detail::capture();
        switch(kind){case 0:state.image=SourcePhysicsContactImage::MapPack11;break;case 1:state.image=SourcePhysicsContactImage(99);break;case 2:state.fp.count=9;break;case 3:state.fp.top=8;break;case 4:state.fp.status|=0x8080;break;case 5:state.active_mmx.emplace();state.fp.count=1;break;case 6:services.instruction={};break;case 7:services.instruction=[different=1u](std::uint32_t){(void)different;};break;case 8:c.position=nullptr;break;case 9:c.executor={};break;case 10:c.literal_service_sites=false;break;case 11:c.literal_service_sites=true;break;case 12:state.instruction_cursor=&copied;break;case 13:c.callback_type=nullptr;break;case 14:sites[2]^=1;break;case 15:c.service_sites=std::span(sites).first(2);break;case 16:c.first_trace=&trace;break;case 17:c.second_trace=&trace;break;case 18:c.observation_only_stores=false;break;case 19:c.callback_type=&typeid(int);break;case 20:_mm_setcsr(host.mxcsr&~0x80u);break;case 21:services.instruction={};state.instruction_cursor=nullptr;break;case 22:c.service_sites=extra;break;case 23:{const auto cw=std::uint16_t(host.word(0)&~1u);asm volatile("fldcw %0"::"m"(cw));break;}case 24:c.service_sites={};break;case 25:services.instruction={};c.position=nullptr;c.literal_service_sites=false;break;case 26:state.instruction_cursor=nullptr;break;case 27:state.active_mmx.emplace();state.fp.top=1;break;case 28:break;}
        memory.pc=0xdeadbeefu;const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);const auto flags=state.flags.bits(),other=state.other_flags,depth=state.fp.count,top=state.fp.top;const auto status=state.fp.status;const auto active=state.active_mmx;const auto native=native_codec_rotation_interpolation_owner_detail::activity();
        const auto refusal_host=native_raw_x87_detail::capture();const auto before=codec_statements[variant];bool stopped{};codec_stop=true;
        try{bound(state);}catch(const CodecRetainedStop&){stopped=true;}codec_stop=false;const bool host_unchanged=native_raw_x87_detail::equivalent(refusal_host,native_raw_x87_detail::capture());native_raw_x87_detail::restore(host);
        check(stopped&&codec_statements[variant]-before==1&&native_codec_rotation_interpolation_owner_detail::activity().calls==native.calls,"Rotation interpolation source did not enter untouched retained fallback "+std::to_string(kind));
        check(state.gp==gp&&std::memcmp(xmm.data(),state.xmm,128)==0&&state.flags.bits()==flags&&state.other_flags==other&&state.fp.count==depth&&state.fp.top==top&&state.fp.status==status&&bool(state.active_mmx)==bool(active),"Rotation interpolation source refusal changed entry ABI");
        check(!callbacks&&memory.reads.empty()&&memory.writes.empty()&&memory.preimages.empty()&&memory.pc==0xdeadbeefu&&host_unchanged,"Rotation interpolation source refusal changed I/O or host FP");++checks;
    }return checks;
}
template<class S>void legacy(const J&fixture,unsigned variant,void(*bound)(S&),void(*ordered)(S&),void(*original)(S&),J&result){
    const auto&row=fixture.at("cases")[2];auto expected=execute(row,variant,0,true,bound,ordered),actual=execute(row,variant,0,true,bound,original);
    const bool differs=expected.at("abi")!=actual.at("abi");
    result.push_back(J{{"variant",variant},{"unmodified_literal_differs",differs},{"ordered_xmm0",expected.at("abi").at("xmm")[0]},{"unmodified_xmm0",actual.at("abi").at("xmm")[0]}});
}
}
int main(int argc,char**argv)try{
    const auto host=native_raw_x87_detail::capture();struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{host};
    if(argc==3&&std::string_view(argv[1])=="--legacy"){
        std::ifstream input(argv[2]);const auto fixture=J::parse(input);J results=J::array();
#define CODEC_LEGACY(N) legacy(fixture,N,&codec_source##N::bound,&codec_source##N::reference,&codec_source##N::legacy_reference,results);
        CODEC_SOURCE_OWNERS(CODEC_LEGACY)
#undef CODEC_LEGACY
        std::cout<<J{{"passed",true},{"unmodified_literal_controls",results},{"payloads_masked",false}}.dump()<<'\n';return 0;
    }
    if(argc==2&&std::string_view(argv[1])=="--guards"){
        unsigned checks{};
#define CODEC_GUARD(N) checks+=guards(N,codec_source##N::literal_source,&codec_source##N::bound);
        CODEC_SOURCE_OWNERS(CODEC_GUARD)
#undef CODEC_GUARD
        std::cout<<J{{"passed",true},{"untouched_actual_source_refusals",checks}}.dump()<<'\n';return 0;
    }
    check(argc==2,"Supply real rotation interpolation source inputs");std::ifstream input(argv[1]);const auto fixture=J::parse(input);unsigned comparisons{},handoffs{},statements{};
#define CODEC_OWNER(N) owner(fixture,N,codec_source##N::literal_source,&codec_source##N::bound,&codec_source##N::reference,comparisons,handoffs,statements);
    CODEC_SOURCE_OWNERS(CODEC_OWNER)
#undef CODEC_OWNER
    std::cout<<J{{"passed",true},{"source_variants",CODEC_SOURCE_COUNT},{"source_comparisons",comparisons},{"authenticated_continuations",handoffs},{"retained_suffix_statements",statements},{"normal_input_modes",34},{"readonly_fp_metadata_domain",true},{"raw_callbacks_retained",true},{"banks_predecoded_once_per_input",true},{"standalone_source_fixture",true}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
