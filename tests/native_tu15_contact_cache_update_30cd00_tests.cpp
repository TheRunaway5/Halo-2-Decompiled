#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#include "physics/runtime/native_tu15_contact_cache_update_30cd00.hpp"
#include "contact_cache_update_accounting.inc"

using namespace frame_misc_harness;
namespace update = halo2::native_contact_cache_update_30cd00_detail;

namespace original {
struct ObservedWorldState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t tu15,const ObservedWorldState &s) {
    return s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
static void dispatch(ObservedWorldState &,std::uint32_t,std::uint32_t,bool);
static void run_2e0ba0(ObservedWorldState &);
#include "contact_cache_update_owner.inc"
#include "contact_cache_update_transform.inc"
} // namespace original

namespace reduce_reference {
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail) {
    if (halo2::native_allocator_owner_detail::dispatch(s,target,
            [](auto &v,std::uint32_t owner,std::uint32_t call_site,bool is_tail) {
                dispatch(v,owner,call_site,is_tail);
            }))
        return;
    s.required(site,target,tail);
}
#include "contact_cache_update_reduce.inc"
} // namespace reduce_reference

namespace {
using original::ObservedWorldState;
constexpr auto a=0x610000u, b=0x620000u, radius_owner=0x630000u, proposal=0x640000u;
constexpr auto receiver=0x650000u, radius=0x660000u, array=0x670000u, count_cell=0x680000u;
constexpr auto a_matrix=0x690000u, b_matrix=0x6a0000u, receive_vtable=0x6b0000u, receive_target=0x889100u;
constexpr auto remove_target=0x889200u;
constexpr std::array targets{0x2e4690u,0x2e4690u,receive_target,0x30caa0u};
constexpr std::array sites{0x30ce4eu,0x30ce5cu,0x3093cfu,0x309423u};
constexpr std::array resumes{0x30ce53u,0x30ce61u,0x30cec2u,0x30cf18u};

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    bool operator==(const Access &) const = default;
};
struct FaultMemory : Memory {
    mutable std::vector<Access> accesses;
    const unsigned fail_at;
    FaultMemory(const Memory &source,unsigned fault) : fail_at(fault) { pages=source.clone().pages; }
    void at(std::uint32_t address,std::size_t width,bool write) const {
        accesses.push_back({address,width,write});
        if (accesses.size() == fail_at)
            throw std::runtime_error("Injected contact-cache memory fault");
    }
    void read(std::uint32_t address,std::span<std::byte> bytes) const override {
        at(address,bytes.size(),false);
        Memory::read(address,bytes);
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes) override {
        at(address,bytes.size(),true);
        Memory::write(address,bytes);
    }
};
bool identical(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a,b) && a.fs_base==b.fs_base && a.floating_point->mxcsr==b.floating_point->mxcsr;
}
struct Child {
    std::uint32_t target, site;
    SourcePhysicsWorldRuntimeAbi abi;
    bool operator==(const Child &other) const {
        return target==other.target && site==other.site && identical(abi,other.abi);
    }
};
struct Observation {
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<Child> calls;
    std::vector<Child> removals;
    std::uint32_t cursor{}, literals{};
    unsigned fail_child{};
    std::string fault;
};
thread_local Observation *active{};
bool record_child(std::uint32_t target,std::uint32_t site,const SourcePhysicsWorldRuntimeAbi &abi) {
    const auto at=active->calls.size();
    if (at>=targets.size() || target!=targets.at(at) || site!=sites.at(at))
        throw std::logic_error("Wrong contact-cache update child route");
    active->calls.push_back({target,site,abi});
    return active->calls.size()==active->fail_child;
}

Case fixture(std::mt19937 &random,unsigned row,unsigned depth=0u) {
    Case c;
    const auto sp=row%13u==12u ? 0x78u : 0x70ff00u+row%19u;
    random_abi(random,c,sp);
    c.abi.fs_base=random();
    if (depth) {
        c.abi.x87_depth=depth;
        for (unsigned n=0;n<depth;++n)
            c.abi.x87[n]=(long double)(n+1u)/3.0L;
    }
    for (unsigned offset=0u;offset<0x240u;offset+=4u)
        c.memory.put(sp-offset,random());
    const std::array arguments{0x12345678u,a,b,radius_owner,proposal,receiver,array,count_cell};
    for (unsigned n=0;n<arguments.size();++n)
        c.memory.put(sp+4u*n,arguments[n]);
    c.memory.put(a+8u,a_matrix);
    c.memory.put(b+8u,b_matrix);
    c.memory.put(radius_owner+0xcu,radius);
    c.memory.put(radius+4u,row%8u==5u ? 0x7fc00082u : 0x40000000u);
    c.memory.put(receiver,receive_vtable);
    c.memory.put(receive_vtable+4u,receive_target);
    c.memory.put(receive_vtable+8u,remove_target);
    c.memory.put(0x46a560u,0x3c23d70bu);
    c.memory.put(0x46a0e8u,0u);
    constexpr std::array count{0u,1u,2u,4u,0xffffffffu,1u,5u,2u};
    c.memory.put(count_cell,count.at(row%8u));
    constexpr std::array edge{0u,0x3f800000u,0x80000000u,0xff800011u,0x7fc00091u,0x00000001u};
    for (unsigned n=0u;n<16u;n+=4u) {
        c.memory.put(0x41c500u+n,0x3f800000u);
        c.memory.put(proposal+0x10u+n,0u);
        c.memory.put(proposal+0x30u+n,edge.at(random()%edge.size()));
        c.memory.put(proposal+0x40u+n,0u);
    }
    c.memory.put(proposal+0x20u,0x1934u);
    c.memory.put(proposal+0x24u,row%9u==0u ? 0xff800023u : 0x3f000000u);
    for (unsigned n=0u;n<0x40u;n+=4u) {
        c.memory.put(a_matrix+0x20u+n,edge.at(random()%edge.size()));
        c.memory.put(b_matrix+0x20u+n,edge.at(random()%edge.size()));
    }
    for (unsigned record=0u;record<6u;++record) {
        const auto slot=array+record*0x30u;
        const bool close=row%8u==1u || row%8u==2u || (row%8u==7u && !record);
        for (unsigned lane=0u;lane<0x30u;lane+=4u)
            c.memory.put(slot+lane,edge.at(random()%edge.size()));
        for (unsigned lane=0u;lane<16u;lane+=4u) {
            c.memory.put(slot+lane,close ? 0u : 0x41100000u);
            c.memory.put(slot+0x20u+lane,row%8u==2u ? 0x41100000u : 0u);
        }
    }
    return c;
}

Observation execute(FaultMemory &memory,const Case &c,unsigned row,bool raw,bool selected,unsigned store_fault,unsigned child_fault) {
    Observation result;
    result.fail_child=child_fault;
    const SourcePhysicsContactRecordObserver observer=[&](const auto &store) {
        result.stores.push_back(store);
        if (result.stores.size()==store_fault)
            throw std::runtime_error("Injected contact-cache observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc) {
        result.cursor=pc;
        const auto start=selected ? update::first : update::first-update::observation_offset;
        result.literals+=pc>=start && pc<=start+update::last-update::first;
    };
    if (!census_mode && !std::getenv("HALO2_REFERENCE_CONTACT_CACHE_UPDATE_30CD00")) {
        auto &cursor=services.instruction_cursor;
        cursor.position=&result.cursor;
        cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();
        cursor.service_sites=halo2::native_census_detail::service_sites;
        cursor.literal_service_sites=true;
        cursor.observation_only_stores=true;
    }
    services.required_owner=[&](auto &,const auto &call,auto &abi,const auto &observed) {
        if (call.target==remove_target && call.instruction==0x30ccb5u && !call.tail) {
            result.removals.push_back({call.target,call.instruction,abi});
            if (r(memory,abi.gp[4])!=0x30ccb8u)
                throw std::logic_error("Wrong real contact-cache reducer child return");
            abi.gp[4]+=8u;
            return;
        }
        const auto at=result.calls.size();
        if (call.tail)
            throw std::logic_error("Contact cache expected an ordinary call");
        if (record_child(call.target,call.instruction,abi)) {
            abi.gp[0]=0xabaddeadu;
            throw std::runtime_error("Injected contact-cache child fault");
        }
        if (r(memory,abi.gp[4])!=resumes.at(at))
            throw std::logic_error("Wrong contact-cache child return address");
        if (at==3u) {
            FpScope frame(*abi.floating_point);
            WorldState child{memory,observed,services,SourcePhysicsContactImage::TitleUpdate15};
            child.load(abi);
            try {
                reduce_reference::run_30caa0(child);
            } catch (...) {
                abi=child.save();
                throw;
            }
            abi=child.save();
            return;
        }
        abi.gp[4]+=20u;
        abi.gp[0]=row%8u==2u ? 0xabbaFFFFu : (0xabba0000u | row);
        abi.eflags^=0x41u;
        if (row%5u==0u) {
            abi.floating_point->x87_control_word^=0x400u;
            abi.x87_status|=0x20u;
        }
    };
    FpScope fp(*c.abi.floating_point);
    ObservedWorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(c.abi);
    state.selected_observation_pcs=selected;
    active=&result;
    try {
        if (raw)
            original::native_contact_cache_update_reference_309210_live(state);
        else
            original::run_309210(state);
    } catch (const std::exception &error) {
        result.fault=error.what();
    }
    active=nullptr;
    result.abi=state.save();
    return result;
}
} // namespace

namespace original {
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool tail) {
    s.required(site,target,tail);
}
static void run_2e0ba0(ObservedWorldState &s) {
    const auto at=active->calls.size();
    if (record_child(0x2e4690u,sites.at(at),s.save())) {
        s.gp[0]=0xabaddeadu;
        throw std::runtime_error("Injected contact-cache child fault");
    }
    fixture_2e0ba0(s);
}
} // namespace original

namespace {
struct Counts {unsigned comparisons{}, faults{}, accesses{}, stores{}, children{}, removals{}, literals{};};
std::pair<unsigned,unsigned> compare(const Case &c,unsigned row,const std::string &mode,Counts &counts,
                                    unsigned access=0u,unsigned store=0u,unsigned child=0u,bool selected=true,bool overflow=false) {
    FaultMemory left(c.memory,access), right(c.memory,access);
    const auto ref=execute(left,c,row,true,selected,store,child);
    const auto got=execute(right,c,row,false,selected,store,child);
    const bool injected=access || store || child || overflow;
    if (ref.fault.empty()==injected || ref.fault!=got.fault || !identical(ref.abi,got.abi) ||
        ref.calls!=got.calls || ref.removals!=got.removals || ref.stores!=got.stores ||
        left.accesses!=right.accesses || !left.same(right) ||
        (!census_mode && ref.cursor!=got.cursor) || bool(got.literals)!=(!selected || mode=="reference") || !ref.literals) {
        std::fprintf(stderr,"contact update row %u mode %s access %u store %u child %u pc %x/%x literals %u/%u "
                    "fault '%s'/'%s'\n",row,mode.c_str(),access,store,child,got.cursor,ref.cursor,
                    got.literals,ref.literals,got.fault.c_str(),ref.fault.c_str());
        for (unsigned reg=0u;reg<8u;++reg)
            if (got.abi.gp[reg]!=ref.abi.gp[reg])
                std::fprintf(stderr,"GP %u native %x reference %x\n",reg,got.abi.gp[reg],ref.abi.gp[reg]);
        throw std::runtime_error("Selected World contact-cache update differs from pinned body");
    }
    if (!injected) {
        constexpr std::array expected_children{3u,0u,3u,4u,3u,3u,4u,0u};
        if (got.calls.size()!=expected_children.at(row%8u) || got.abi.gp[4]!=c.abi.gp[4]+4u)
            throw std::runtime_error("Wrong World contact-cache result or return frame");
    }
    ++counts.comparisons;
    counts.faults+=injected;
    counts.accesses+=unsigned(left.accesses.size());
    counts.stores+=unsigned(ref.stores.size());
    counts.children+=unsigned(got.calls.size());
    counts.removals+=unsigned(got.removals.size());
    counts.literals+=got.literals;
    return {unsigned(left.accesses.size()),unsigned(ref.stores.size())};
}
} // namespace

int main(int argc,char **argv) {
    if (argc!=3)
        return 2;
    try {
        const auto rows=std::stoul(argv[1]);
        const std::string mode=argv[2];
        set_mode(mode,"HALO2_VERIFY_CONTACT_CACHE_UPDATE_30CD00");
        if (mode=="reference")
            setenv("HALO2_REFERENCE_CONTACT_CACHE_UPDATE_30CD00","1",1);
        std::mt19937 random(0x30cd00u);
        Counts counts, canonical;
        for (unsigned row=0u;row<rows;++row)
            compare(fixture(random,row),row,mode,counts);
        for (const auto row : {0u,1u,2u,3u,12u}) {
            const auto c=fixture(random,row);
            const auto [accesses,stores]=compare(c,row,mode,counts);
            for (unsigned cut=1u;cut<=accesses;++cut)
                compare(c,row,mode,counts,cut);
            for (unsigned cut=1u;cut<=stores;++cut)
                compare(c,row,mode,counts,0u,cut);
        }
        const auto c=fixture(random,3u);
        for (unsigned child=1u;child<=4u;++child)
            compare(c,3u,mode,counts,0u,0u,child);
        compare(fixture(random,3u,6u),3u,mode,counts);
        compare(fixture(random,3u,7u),3u,mode,counts,0u,0u,0u,true,true);
        compare(c,3u,mode,canonical,0u,0u,0u,false);
        const auto coverage=update::coverage().load();
        if (coverage!=(mode=="reference" ? 0u : 0x1ffu) || !counts.removals)
            throw std::runtime_error("Missing World contact-cache update path");
        std::printf("{\"comparisons\":%u,\"faults\":%u,\"accesses\":%u,\"stores\":%u,\"children\":%u,"
                    "\"canonical_fallbacks\":%u,\"canonical_fallback_statements\":%u,"
                    "\"real_reducer_removals\":%u,\"coverage\":%llu}\n",counts.comparisons,counts.faults,
                    counts.accesses,counts.stores,counts.children,canonical.comparisons,canonical.literals,counts.removals,
                    static_cast<unsigned long long>(coverage));
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr,"%s\n",error.what());
        return 1;
    }
}
