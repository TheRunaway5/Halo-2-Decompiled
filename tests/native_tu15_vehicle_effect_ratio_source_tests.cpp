#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/physics_instruction_remap_tu15.hpp"
#include "physics/vehicles/native_tu15_vehicle_effect_ratio_source_frame.hpp"
#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"

using namespace halo2::physics_vehicle_runtime_detail;
#if defined(HALO2_EFFECT_RATIO_BOUND_TEST)
#include "physics/vehicles/native_tu15_vehicle_effect_ratio_source_owner.hpp"
namespace {
struct EffectRatioRetainedStop{};
unsigned retained_entries{};
bool stop_before_retained{};
void effect_ratio_retained_entry() {
    ++retained_entries;
    if(stop_before_retained)throw EffectRatioRetainedStop{};
}
bool force_effect_ratio_reference() {
    const auto* value=std::getenv("HALO2_REFERENCE_VEHICLE_EFFECT_RATIO");
    return value&&*value;
}
}
#endif
#include "vehicle_effect_ratio_reference.inc"

namespace {
struct InstructionStop {};
struct SourceMemory : Memory {
    std::function<void(std::uint32_t)> after_read;
    void read(std::uint32_t address,std::span<std::byte> out) const override {
        Memory::read(address,out);
        if(after_read)after_read(address);
    }
    std::uint32_t stored_word(std::uint32_t address) const {
        std::uint32_t value{};std::memcpy(&value,at(address,4),4);return value;
    }
    void replace_word(std::uint32_t address,std::uint32_t value) {
        std::memcpy(const_cast<std::byte*>(at(address,4)),&value,4);
    }
};

enum class Policy { empty, direct, phantom, quiet, trace_on_read, rebind_on_read, clear_on_read,
    live_register, live_provider, dual_nan, live_fp, callback_stop, provider_stop };

struct Result {
    J abi, reads, packets, callbacks;
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    unsigned exception{};
    bool operator==(const Result&)const=default;
};

template<bool Literal,bool Candidate> Result run(const J& row,Policy policy) {
    SourceMemory memory;
    for(const auto& bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    SourcePhysicsContactRuntimeServices services,outer;SourcePhysicsContactRecordObserver observer;
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    J calls=J::array();bool trace{};VehicleState* active{};unsigned exception{};
    const auto id=row.at("id").get<unsigned>();
    auto record=[&](std::uint32_t pc,unsigned route) {
        memory.pc=pc;
        calls.push_back(J{{"route",route},{"packet",packet(*active,pc)}});
        auto& s=*active;
        if(policy==Policy::live_register) {
            if(pc==0xd26c6u) {
                const auto rows=memory.stored_word(s.gp[1]+0x44u);
                memory.replace_word(s.gp[1]+0x54u,rows);s.gp[1]+=16u;
            }
            if(pc==0xd26e3u && (id&1u))s.flags.zf=true;
            if(pc==0xd2759u){s.gp[4]+=16u;memory.replace_word(s.gp[4],0x63100000u);}
        }
        if(policy==Policy::dual_nan&&pc==0xd2749u) {
            s.xmm[0]=std::bit_cast<__m128>(std::array<std::uint32_t,4>{0x7fc12345u,1u,2u,3u});
            memory.replace_word(s.gp[1]+0x338u,0xffc54321u);
        }
        if(policy==Policy::live_fp&&pc==0xd26e0u)_mm_setcsr(_mm_getcsr()|0x8040u);
        if(policy==Policy::callback_stop&&calls.size()==1u+id%18u)throw InstructionStop{};
    };
    if(policy==Policy::phantom) {
        outer.instruction=[&](auto pc){record(pc,1u);};
        services.instruction=PhysicsInstructionRemapTu15{&outer,vehicle_effect_ratio_actual_sites};
    } else if(policy!=Policy::empty)services.instruction=[&](auto pc){record(pc,0u);};
    if(policy==Policy::quiet||policy==Policy::trace_on_read||policy==Policy::rebind_on_read||policy==Policy::clear_on_read)
        services.instruction_cursor={&memory.pc,&trace,nullptr,service_sites,std::this_thread::get_id(),
            &services.instruction.target_type(),true,true};
    VehicleState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};active=&state;
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");
    abi.x87_status=row.at("before_fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    native_raw_x87_detail::Image host=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{host};
    state.load(abi);
    memory.capture=[&]{return packet(state,memory.pc);};
    if(policy==Policy::provider_stop)memory.fault=J{{"ordinal",id%14u}};
    memory.after_read=[&](std::uint32_t) {
        if(memory.ordinal==1u) {
            if(policy==Policy::trace_on_read)trace=true;
            if(policy==Policy::rebind_on_read)services.instruction=[&](auto pc){record(pc,2u);};
            if(policy==Policy::clear_on_read)services.instruction={};
        }
        if(policy==Policy::live_provider&&memory.pc==0xd26e7u) {
            const auto body=state.gp[1];
            const auto elapsed=memory.stored_word(body+0x334u), baseline=memory.stored_word(body+0x338u);
            memory.replace_word(body+8u,memory.stored_word(body));
            memory.replace_word(body+0x33cu,elapsed);memory.replace_word(body+0x340u,baseline);
            state.gp[1]=body+8u;
        }
    };
    try {
        if constexpr(Candidate) {
#if defined(HALO2_EFFECT_RATIO_BOUND_TEST)
            retained_entries=0;
            if constexpr(Literal)effect_ratio_bound_prepared::run_d26c0(state);
            else effect_ratio_bound_raw::run_d26c0(state);
#else
            native_vehicle_effect_ratio_detail::run_source_private<Literal>(state);
#endif
        }
        else if constexpr(Literal)effect_ratio_prepared::run_d26c0(state);
        else effect_ratio_raw::run_d26c0(state);
    } catch(const Fault&) {exception=1;}
      catch(const InstructionStop&) {exception=2;}
    return {packet(state,memory.pc),std::move(memory.reads),std::move(memory.packets),std::move(calls),
            std::move(memory.banks),exception};
}

template<bool Literal> J compare(const J& fixture) {
    unsigned comparisons{},callback_packets{},operand_packets{},provider_stops{},callback_stops{};
    std::set<std::uint32_t> observed;
    for(const auto& row:fixture)for(unsigned p=0;p<=unsigned(Policy::provider_stop);++p) {
        const auto policy=static_cast<Policy>(p);
        const Result reference=run<Literal,false>(row,policy),candidate=run<Literal,true>(row,policy);
        const auto name=row.at("name").get<std::string>()+" policy "+std::to_string(p);
#if defined(HALO2_EFFECT_RATIO_BOUND_TEST)
        check(retained_entries==unsigned(force_effect_ratio_reference()),name+": effect ratio actual route did not enter the expected body");
#endif
        check(candidate==reference,name+": effect ratio retained route differs (ABI "+
            std::to_string(candidate.abi==reference.abi)+", operands "+std::to_string(candidate.packets==reference.packets)+
            ", callbacks "+std::to_string(candidate.callbacks==reference.callbacks)+")");
        if(policy==Policy::phantom) {
            check(!candidate.callbacks.empty()&&candidate.callbacks[0].at("packet").at("entry")==0xd3750u,name+": effect ratio real remap did not execute");
            for(const auto& call:candidate.callbacks)observed.insert(call.at("packet").at("entry").get<std::uint32_t>());
        }
        if(policy==Policy::quiet)
            check(candidate.callbacks.empty()==Literal,name+": effect ratio exact direct/literal transport differs");
        ++comparisons;callback_packets+=candidate.callbacks.size();operand_packets+=candidate.packets.size();
        provider_stops+=candidate.exception==1u;callback_stops+=candidate.exception==2u;
    }
    check(observed.size()==34,"Effect ratio actual Phantom mapping missed source sites");
    return J{{"comparisons",comparisons},{"callback_packets",callback_packets},{"operand_packets",operand_packets},
             {"provider_stops",provider_stops},{"callback_stops",callback_stops},{"actual_sites",observed}};
}

#if defined(HALO2_EFFECT_RATIO_BOUND_TEST)
template<bool Literal> unsigned refusals() {
    unsigned tested{};
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}};
    for(unsigned policy=0;policy<7;++policy) {
        if(policy==6&&!force_effect_ratio_reference())continue;
        Restore restore{native_raw_x87_detail::capture()};
        asm volatile("fninit":::"memory","st","st(1)");
        SourceMemory memory;SourcePhysicsContactRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        unsigned callbacks{};services.instruction=[&](auto){++callbacks;};
        VehicleState state{memory,observer,services,policy==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={7,6,5,4,0x62000000u,3,2,1};abi.eflags=0x246u;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x037fu,0x1f80u};state.load(abi);
        switch(policy) {
        case 1:state.active_mmx=SourcePhysicsMmxAlias{};break;
        case 2:state.fp.count=9u;break;
        case 3:state.fp.status|=0x0080u;break;
        case 4:{const std::uint16_t cw=0x37eu;asm volatile("fldcw %0"::"m"(cw));break;}
        case 5:_mm_setcsr(0x1f00u);break;
        default:break;
        }
        const auto saved=state.save();const auto host=native_raw_x87_detail::capture();
        retained_entries=0;stop_before_retained=true;bool stopped{};
        try {
            if constexpr(Literal)effect_ratio_bound_prepared::run_d26c0(state);
            else effect_ratio_bound_raw::run_d26c0(state);
        } catch(const EffectRatioRetainedStop&){stopped=true;}
          catch(...){} // An unexpected route is reported by the untouched-state assertion below.
        stop_before_retained=false;
        const auto after=state.save();
        check(stopped&&retained_entries==1&&!callbacks&&memory.reads.empty()&&memory.packets.empty()&&
            saved.gp==after.gp&&saved.eflags==after.eflags&&saved.xmm==after.xmm&&saved.x87_depth==after.x87_depth&&
            saved.x87_status==after.x87_status&&saved.active_mmx.has_value()==after.active_mmx.has_value()&&
            native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),
            "Effect ratio refusal did not reach untouched first original statement");
        ++tested;
    }
    return tested;
}

void old_image_shift() {
    SourceMemory memory;SourcePhysicsContactRuntimeServices services;SourcePhysicsContactRecordObserver observer;
    VehicleState state{memory,observer,services,SourcePhysicsContactImage::MapPack11};
    for(const auto seed:{0x202u,0x213u,0xa46u})for(const auto word:{0x13u,0u,0x80000000u}) {
        state.flags.load(seed);auto expected=state.flags;
        const auto shifted=expected.shift(word,4u,4u);
        check(native_vehicle_effect_ratio_ordered_detail::shift_index(state,word)==shifted&&
            state.flags.bits()==expected.bits(),"MapPack D26FA fallback did not retain the modeled shift");
    }
}
#endif
}

int main(int argc,char** argv) try {
    check(argc==2,"Supply the exact effect ratio input fixture");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    J result{{"passed",true},{"raw",compare<false>(fixture)},{"prepared",compare<true>(fixture)}};
#if defined(HALO2_EFFECT_RATIO_BOUND_TEST)
    old_image_shift();
    result["untouched_entry_refusals"]=refusals<false>()+refusals<true>();
    result["forced_reference"]=force_effect_ratio_reference();
    result["mappack_modeled_shift_controls"]=9;
#endif
    std::cout<<result.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
