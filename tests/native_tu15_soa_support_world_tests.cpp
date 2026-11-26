// Compare the World 3077F0 bridge and unchanged source control flow on the
// same bank. Init the two dispatch tables from the authenticated TU1.5
// image, and exercise masks, faults, aliasing, FP state and World remaps.
#include "native_tu15_frame_misc_harness.hpp"
#include "native_tu15_world_leaves_remap.hpp"
#include "physics/runtime/native_tu15_soa_support_world.hpp"

using namespace frame_misc_harness;
namespace owner=halo2::native_soa_support_world_detail;

namespace original {
struct ObservedWorldState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t tu15,const ObservedWorldState &state) {
    return state.selected_observation_pcs&&state.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;
}
#include "resolvers.inc"
#include "body.inc"
}

namespace {
using original::ObservedWorldState;
constexpr std::uint32_t shape=0x10001000u,coordinates=0x10003000u,direction=0x10005000u;
constexpr std::uint32_t output=0x10007000u,stack=0x2000208cu;
constexpr std::array<std::uint8_t,43> tables{
#include "tables.inc"
};

Case author(std::mt19937 &random,unsigned index) {
    Case test;
    auto &memory=test.memory;
    const auto shape_case=(index/16u)%16u;
    const auto mask=index%16u;
    const auto entry=stack-4u*(index%4u);
    random_abi(random,test,entry);
    test.abi.gp[1]=shape;
    std::uint32_t points=coordinates,axis=direction,target=output,groups=1u;
    if(shape_case==1u)groups=0u;
    if(shape_case==2u)groups=0x20000000u;
    if(shape_case==3u)groups=0x40000000u;
    if(shape_case==4u||shape_case==5u||shape_case==6u)groups=3u;
    if(shape_case==7u)target=coordinates+4u;
    if(shape_case==8u)target=direction+4u;
    if(shape_case==9u)axis=((entry-4u)&0xfffffff0u)-0x14u;
    for(const auto address:{points,axis,target})
        for(std::uint32_t offset=0u;offset<0xe0u;offset+=4u)memory.put(address+offset,std::uint32_t(random()));
    for(std::uint32_t offset=0u;offset<0x90u;offset+=4u)
        memory.put(entry-0x50u+offset,std::uint32_t(random()));
    memory.put(entry,0x307600u+std::uint32_t(random()%64u));
    memory.put(entry+4u,axis);
    memory.put(entry+8u,target);
    memory.put(shape+0x30u,points);
    memory.put(shape+0x34u,groups);
    memory.put(axis,u(1.0f));
    memory.put(axis+4u,0u);
    memory.put(axis+8u,0u);
    for(std::uint32_t n=0;n<tables.size();++n)memory.put(0x307988u+n,tables[n],1u);

    for(unsigned group=0u;group<3u;++group)
        for(unsigned lane=0u;lane<4u;++lane) {
            const auto address=points+group*0x30u+lane*4u;
            const auto score=group==0u && (mask&(1u<<lane))==0u
                ?0xff7fffffu:u(float(int(random()%5u)+int(group)*10));
            memory.put(address,score);
            memory.put(address+0x10u,u(float(lane)));
            memory.put(address+0x20u,u(float(group)));
        }
    if(shape_case==5u||shape_case==6u) {
        std::mt19937_64 floats(random());
        for(unsigned n=0;n<3u*12u;++n)
            memory.put(points+n*4u,special_float(floats));
        for(unsigned n=0;n<3u;++n)
            memory.put(axis+n*4u,special_float(floats));
    }
    if(shape_case==10u)memory.drop(direction);
    if(shape_case==11u)memory.drop(coordinates);
    if(shape_case==12u)memory.drop(output);
    if(shape_case==13u)memory.drop(0x307988u);
    if(shape_case==14u)memory.put(0x307988u,0u);
    if(shape_case==15u)memory.put(0x307988u,0xdead1234u);
    return test;
}

bool native(ObservedWorldState &state) {
    return owner::bridge(state,[](auto &value){original::run_3037f0(value);},
        [&](std::uint32_t target){return original::canonical_branch(target,state.image);});
}

Result execute(Memory &memory,const Case &test,bool run_native) {
    Result result;
    SourcePhysicsContactRecordObserver stores=[&](const auto &event){result.stores.push_back(event);};
    SourcePhysicsWorldRuntimeServices services,outer;
    if(run_native) {
        services.instruction=[](std::uint32_t){throw std::runtime_error("Native World leaf executed translated instructions");};
        if(!census_mode&&leaves_remap::live_cursor) {
            auto &cursor=services.instruction_cursor;
            cursor.position=&result.cursor;
            cursor.service_sites=halo2::native_census_detail::service_sites;
            cursor.callback_type=&services.instruction.target_type();
            cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;
            cursor.observation_only_stores=true;
        }
    } else services.instruction=[&](std::uint32_t site){result.cursor=site;};
    leaves_remap::wrap(services,outer);
    const auto mxcsr=_mm_getcsr();
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    ObservedWorldState state{memory,stores,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.abi);
    state.selected_observation_pcs=!leaves_remap::enabled;
    try {
        if(run_native)result.admitted=native(state);
        else original::run_3037f0(state);
    } catch(const std::exception &error){result.fault=error.what();}
    result.abi=state.save();
    result.mxcsr=_mm_getcsr();
    asm volatile("fnclex\n\tfldcw %0"::"m"(control));
    _mm_setcsr(mxcsr);
    return result;
}

bool compare(const Case &test,unsigned index,Totals &totals) {
    auto a=test.memory.clone(),b=test.memory.clone();
    const auto native_result=execute(a,test,true),reference=execute(b,test,false);
    ++totals.compared;
    totals.stores+=reference.stores.size();
    if(!reference.fault.empty())++totals.faults;
    if(!native_result.admitted&&native_result.fault.empty()) {
        std::fprintf(stderr,"case %u refused\n",index);
        return false;
    }
    if(native_result.fault!=reference.fault) {
        std::fprintf(stderr,"case %u fault native '%s' vs source '%s'\n",index,native_result.fault.c_str(),reference.fault.c_str());
        return false;
    }
    if(native_result.stores!=reference.stores||!a.same(b)) {
        std::fprintf(stderr,"case %u store mismatch (%zu/%zu)\n",index,native_result.stores.size(),reference.stores.size());
        return false;
    }
    if(!census_mode&&native_result.cursor!=reference.cursor) {
        std::fprintf(stderr,"case %u cursor mismatch %x/%x\n",index,native_result.cursor,reference.cursor);
        return false;
    }
    if(reference.fault.empty()&&(!same_abi(native_result.abi,reference.abi)||native_result.mxcsr!=reference.mxcsr)) {
        std::fprintf(stderr,"case %u ABI mismatch flags %x/%x x87 %x/%x MXCSR %x/%x\n",index,
            native_result.abi.eflags,reference.abi.eflags,native_result.abi.x87_status,reference.abi.x87_status,
            native_result.mxcsr,reference.mxcsr);
        return false;
    }
    return true;
}
}

int main(int argc,char **argv)try {
    const auto count=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    const auto base=leaves_remap::parse(mode);
    set_mode(base,"HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x3077f0u+(census_mode?1u:0u)+(base=="verify"?2u:0u));
    Totals totals;
    for(unsigned index=0;index<count;++index)
        if(!compare(author(random,index),index,totals))return 1;
    if(leaves_remap::enabled&&!census_mode) {
        const auto runs=[&] {
            auto test=author(random,15u);
            const auto result=execute(test.memory,test,true);
            return result.admitted||!result.fault.empty();
        };
        if(!leaves_remap::refuses_bad_remaps(0x3037f0u,0x303983u,runs))
            throw std::runtime_error("World SoA support accepted an invalid remap");
    }
    report(count,totals,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &error){std::fprintf(stderr,"%s\n",error.what());return 1;}
