#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_world_position_transform_2e4690.hpp"
#include "world_position_accounting.inc"

using namespace frame_misc_harness;

namespace original {
struct ObservedWorldState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
    // The production type gets its cursor witness from the sealed World.
    // This fixture can supply the input to the same bound bridge directly.
    struct { const InstructionCursor *cursor{}; } immutable_cursor;
};
std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t selected,const ObservedWorldState &s) {
    return s.selected_observation_pcs && s.image==SourcePhysicsContactImage::TitleUpdate15 ? selected : disc;
}
#include "world_position_owner.inc"
} // namespace original

namespace {
using original::ObservedWorldState;
using Abi = SourcePhysicsWorldRuntimeAbi;

bool identical(const Abi &a,const Abi &b) {
    return same_abi(a,b) && a.fs_base==b.fs_base && a.floating_point && b.floating_point &&
           a.floating_point->mxcsr==b.floating_point->mxcsr && !a.active_mmx && !b.active_mmx;
}

struct Packet {
    char access;
    std::uint32_t pc,address;
    std::size_t width;
    Abi abi;
    bool operator==(const Packet &o) const {
        return access==o.access && pc==o.pc && address==o.address && width==o.width && identical(abi,o.abi);
    }
};

struct Provider : Memory {
    using Snapshot = std::function<Packet(char,std::uint32_t,std::size_t)>;
    Snapshot snapshot;
    mutable std::vector<Packet> packets;
    unsigned stop;
    bool lane_reads,change_fp;
    Provider(const Memory &backing,unsigned failure,bool lanes,bool fp)
        : Memory(backing.clone()), stop(failure), lane_reads(lanes), change_fp(fp) {}
    void visit(char kind,std::uint32_t address,std::size_t width) const {
        packets.push_back(snapshot(kind,address,width));
        if (change_fp) {
            const auto happened=packets.size();
            const auto cw=std::uint16_t(0x037fu | ((happened&3u)<<10u));
            asm volatile("fldcw %0"::"m"(cw):"memory");
            source_load_x87_exception_flags(std::uint16_t(happened&0x3fu));
            _mm_setcsr(0x1f80u | (unsigned(happened&3u)<<13u) | unsigned(happened&0x3fu));
        }
        if (packets.size()==stop)
            throw std::runtime_error("Injected position provider fault");
    }
    void read(std::uint32_t a,std::span<std::byte> out) const override {
        visit('r',a,out.size());
        Memory::read(a,out);
    }
    void read_vector(std::uint32_t a,std::span<std::byte,16> out) const override {
        if (lane_reads) {
            SourcePhysicsContactMemory::read_vector(a,out);
            return;
        }
        visit('v',a,out.size());
        Memory::read(a,out);
    }
    void write(std::uint32_t a,std::span<const std::byte> in) override {
        visit('w',a,in.size());
        Memory::write(a,in);
    }
};

enum class Policy { sealed,unsealed,trace,canonical,alternate,observer,second_trace };

struct Observation {
    std::vector<std::uint32_t> callbacks;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::string fault;
    Abi abi;
    std::uint32_t cursor{};
};

Observation run(Provider &memory,const Case &input,Policy policy,bool raw,bool observe) {
    Observation got;
    const bool trace=policy==Policy::trace, second_trace=policy==Policy::second_trace;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc) {got.cursor=pc;got.callbacks.push_back(pc);};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction_cursor={&got.cursor,&trace,&second_trace,sites,std::this_thread::get_id(),
        &services.instruction.target_type(),true,policy!=Policy::observer};
    SourcePhysicsContactRecordObserver observer;
    if (observe)
        observer=[&](const auto &event) {
            got.stores.push_back(event);
            memory.visit('o',event.address,event.written.size());
        };
    FpScope scope(*input.abi.floating_point);
    ObservedWorldState state{memory,observer,services,policy==Policy::alternate ?
        SourcePhysicsContactImage::MapPack11 : SourcePhysicsContactImage::TitleUpdate15};
    state.load(input.abi);
    state.selected_observation_pcs=policy!=Policy::canonical;
    if (policy!=Policy::unsealed)
        state.immutable_cursor.cursor=state.instruction_cursor;
    memory.snapshot=[&](auto access,auto address,auto width) {return Packet{access,got.cursor,address,width,state.save()};};
    try {
        if (raw)
            original::position_reference(state);
        else
            original::run_2e0ba0(state);
    } catch (const std::exception &error) {
        got.fault=error.what();
    }
    got.abi=state.save();
    memory.snapshot={};
    return got;
}

Case fixture(std::mt19937 &random,unsigned n) {
    constexpr auto stack=0x60000fe0u, matrix=0x62000000u, source=0x63000ff8u, destination=0x64000ffcu;
    Case c;
    random_abi(random,c,stack+(n%4u)*4u);
    const auto sp=c.abi.gp[4];
    c.memory.put(sp-4u,0xabcd0123u);
    c.memory.put(sp,0x400000u);
    c.memory.put(sp+4u,matrix+(n%2u)*4u);
    c.memory.put(sp+8u,source);
    c.abi.gp[1]=n%4u==0u ? source : n%4u==1u ? matrix+28u : destination;
    c.abi.fs_base=0x65100000u+n;
    for (const auto address : {matrix,source,c.abi.gp[1]})
        for (unsigned i=0;i<80;i+=4)
            c.memory.put(address+i,std::uint32_t(random()));
    c.memory.put(matrix+(n%2u)*4u,0x7f800003u);
    c.memory.put(matrix+(n%2u)*4u+16u,0xffc00004u);
    c.memory.put(source,0xffc00005u);
    return c;
}

struct Stats {
    unsigned cases{},faults{},packets{},stores{},native_calls{},retained_calls{};
};

unsigned compare(const Case &input,unsigned id,Stats &stats,bool forced,Policy policy=Policy::sealed,
                 bool observe=true,unsigned cut=0) {
    const bool native=policy==Policy::sealed && !forced;
    Provider left(input.memory,cut,id%2u,id%3u==0u),right(input.memory,cut,id%2u,id%3u==0u);
    const auto ref=run(left,input,policy,true,observe);
    const auto got=run(right,input,policy,false,observe);
    if (ref.fault.empty()==bool(cut) || ref.fault!=got.fault || ref.cursor!=got.cursor ||
        !identical(ref.abi,got.abi) || left.packets!=right.packets || ref.stores!=got.stores || !left.same(right) ||
        ref.callbacks.empty() || (native ? !got.callbacks.empty() : ref.callbacks!=got.callbacks)) {
        std::fprintf(stderr,"Position %u cut %u policy %u expected native %d pc %x/%x callbacks %zu/%zu\n",
                     id,cut,unsigned(policy),native,ref.cursor,got.cursor,ref.callbacks.size(),got.callbacks.size());
        throw std::runtime_error("World 2E4690 differs from retained provider behavior");
    }
    if (!cut && (ref.callbacks.size()!=26u || got.abi.gp[4]!=input.abi.gp[4]+12u ||
            (observe && got.stores.size()!=3u)))
        throw std::runtime_error("World 2E4690 exit or store partition changed");
    ++stats.cases;
    stats.faults+=bool(cut);
    stats.packets+=unsigned(left.packets.size());
    stats.stores+=unsigned(ref.stores.size());
    stats.native_calls+=native;
    stats.retained_calls+=!native;
    return unsigned(left.packets.size());
}
} // namespace

int main(int argc,char **argv) try {
    if (argc!=3) return 2;
    const unsigned cases=std::stoul(argv[1]);
    const bool forced=std::string_view{argv[2]}=="reference";
    if (forced)
        setenv("HALO2_REFERENCE_WORLD_POSITION_TRANSFORM_2E4690","1",1);
    std::mt19937 random(0x2e4690u);
    Stats stats;
    for (unsigned n=0;n<cases;++n)
        compare(fixture(random,n),n,stats,forced,Policy::sealed,n%3u!=1u);
    for (unsigned n=0;n<8;++n) {
        const auto c=fixture(random,n);
        const bool observe=n%3u!=1u;
        const auto boundaries=compare(c,n,stats,forced,Policy::sealed,observe);
        for (unsigned cut=1;cut<=boundaries;++cut)
            compare(c,n,stats,forced,Policy::sealed,observe,cut);
    }
    for (const auto policy : {Policy::unsealed,Policy::trace,Policy::second_trace,Policy::canonical,Policy::alternate,Policy::observer}) {
        const auto c=fixture(random,1);
        compare(c,1,stats,forced,policy);
        compare(c,1,stats,forced,policy,true,7);
    }
    std::printf("{\"cases\":%u,\"faults\":%u,\"packets\":%u,\"stores\":%u,\"native\":%u,\"retained\":%u}\n",
                stats.cases,stats.faults,stats.packets,stats.stores,stats.native_calls,stats.retained_calls);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr,"%s\n",error.what());
    return 1;
}
