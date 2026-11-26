#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_physical_result_notify_e4d6d.hpp"
#include "physics/runtime/native_tu15_physical_result_impulse_e5094.hpp"

#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace {
using namespace frame_misc_harness;
using Route = std::function<void(WorldState &,std::uint32_t,std::uint32_t)>;
using ImpulseEntry = halo2::native_physical_result_impulse::Entry;

enum class Access { instruction, read, write, observer, child };
enum class Stage { notify, apply, publish };
struct Rejected {};
void require(bool condition,const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}
struct Provider : Memory {
    using Event = std::tuple<Access,std::uint32_t,std::uint32_t,std::size_t>;
    mutable std::vector<Event> events;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<std::uint64_t> children;
    std::uint32_t pc{};
    unsigned reject{};

    void visit(Access kind, std::uint32_t address = 0, std::size_t size = 0) const {
        events.emplace_back(kind,pc,address,size);
        if (events.size() == reject) throw Rejected{};
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        visit(Access::read,address,out.size());
        Memory::read(address,out);
    }
    void write(std::uint32_t address, std::span<const std::byte> in) override {
        visit(Access::write,address,in.size());
        Memory::write(address,in);
    }
    void observe(const SourcePhysicsContactRecordStore &record) {
        stores.push_back(record);
        visit(Access::observer,record.address,record.written.size());
    }
};

constexpr std::uint32_t object = 0x61002200u, table = 0x61000200u, manager = 0x61000400u;
constexpr std::uint32_t clock = 0x61000600u, copy_vector = 0x61000800u;

Case make_case(std::mt19937 &random, Stage stage, unsigned scenario, unsigned variant) {
    Case test;
    auto &mem = test.memory;
    const auto sp = (variant & 4 ? 0x62001002u : 0x62000400u) + (variant & 3u);
    random_abi(random,test,sp);
    test.abi.gp[5] = object;
    test.abi.gp[6] = 0xa11e0003u;
    for (unsigned offset = 4; offset <= 16; offset += 4) mem.put(sp-offset,random());
    if (stage == Stage::notify) {
        const int displacement = variant & 8 ? -16 : 0x1b0;
        const auto description = object + std::uint32_t(displacement);
        mem.put(sp+0x30,scenario ? 4u : 0u,1);
        mem.put(object+0x3dc,scenario == 1 ? 4u : scenario == 2 ? 3u : 6u,1);
        mem.put(object+0x346,std::uint32_t(displacement),2);
        mem.put(description+0x2c,test.abi.gp[7] + (scenario >= 4));
        mem.put(description+0x32,scenario == 4 ? 0x80u : 0x7fu,1);
        mem.put(description+0x33,variant & 16 ? 0x80u : 1u,1);
        test.policy = scenario == 3 || variant & 16;
        return test;
    }
    if (stage == Stage::publish) {
        mem.put(sp,0u);
        return test;
    }
    const auto target = variant & 8 ? object : 0x61003200u;
    mem.put(0x51df54u,clock);
    mem.put(0x4ecbd0u,manager);
    mem.put(0x474d04u,scenario & 1 ? copy_vector : 0u);
    mem.put(manager+0x44,table);
    mem.put(table+0x2c,target);
    mem.put(sp+0x14,0x24u);
    mem.put(sp+0x3c,0x3f000000u);
    mem.put(sp+0x40,0xbf000000u);
    mem.put(target+4u,scenario & 1 ? 0x100u : 0u);
    mem.put(target+0x14,scenario == 2 ? 0u : 0xffffffffu);
    mem.put(target+0xd4,scenario & 1 ? 6u : 0xffffffffu);
    mem.put(0x46aa04u,0xc7000000u);
    mem.put(0x46a530u,0x47000000u);
    const std::array raw{0x3f800000u,0xbf800000u,0x7f800000u,0xff800000u,
                         0x80000000u,0x00000001u,0x7fc1000au,0x7f811235u};
    mem.put(clock+4u,scenario <= 2 ? 0x3d800000u : raw[variant % raw.size()]);
    for (unsigned axis = 0; axis < 3; ++axis) {
        mem.put(object+0x64+4*axis,scenario <= 2 ? 0x3f800000u : raw[(variant+axis+scenario)%raw.size()]);
        mem.put(copy_vector+4*axis,random());
    }
    test.abi.xmm[3][0] = scenario <= 2 ? 0x3f000000u : raw[(variant+3)%raw.size()];
    return test;
}

#include "physical_result_final_branches_reference.inc"

struct Outcome { Provider memory; SourcePhysicsWorldRuntimeAbi abi; bool rejected{}; };

Outcome execute(const Case &test, Stage stage, bool native, unsigned fault) {
    Outcome out;
    static_cast<Memory&>(out.memory)=test.memory.clone();
    out.memory.reject=fault;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){out.memory.pc=pc;out.memory.visit(Access::instruction);};
    const SourcePhysicsContactRecordObserver observer=[&](const auto &record){out.memory.observe(record);};
    WorldState live{out.memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    live.load(test.abi);
    const Route route=[&](WorldState &child,std::uint32_t site,std::uint32_t target) {
        out.memory.visit(Access::child,target);
        out.memory.children.push_back(entry_hash(child,target));
        const auto resume=child.pop();
        require(resume == site+5u,"Child return address differs");
        switch (target) {
        case 0xe3310u: child.set(0,test.policy,1,0); break;
        case 0xe31c0u: case 0xe94e0u: case 0xe7e80u: case 0xbda50u: break;
        case 0xba7e0u: child.gp[4]+=4u; break;
        case 0xb7e50u: case 0x1cd620u: case 0xba630u: case 0x1cd6a0u: child.gp[4]+=8u; break;
        default: throw std::runtime_error("Unexpected child route");
        }
    };
    try {
        const auto entry=stage==Stage::publish?ImpulseEntry::publish:ImpulseEntry::apply;
        if (native) {
            const bool admitted=stage==Stage::notify ? selected_notify(live,route) : selected_impulse(live,route,entry);
            require(admitted,"Bound physical-result branch refused fixture");
        } else if (stage==Stage::notify) retained_notify(live,route);
        else retained_impulse(live,route,entry);
    } catch (const Rejected &) {out.rejected=true;}
    out.abi=live.save();
    return out;
}

Outcome check(const Case &test,Stage stage,unsigned fault) {
    const auto old=execute(test,stage,false,fault);
    auto now=execute(test,stage,true,fault);
    require(old.rejected==bool(fault) && now.rejected==old.rejected && old.memory.events==now.memory.events &&
            old.memory.children==now.memory.children && old.memory.stores==now.memory.stores &&
            old.memory.same(now.memory) && same_abi(old.abi,now.abi) &&
            old.abi.floating_point->mxcsr==now.abi.floating_point->mxcsr,
            "Physical-result child, provider or ABI differs at "+std::to_string(now.memory.pc));
    return now;
}
}

int main() try {
    std::mt19937 random{0xe45094u};
    std::set<std::uint32_t> sites,children;
    std::set<std::tuple<Access,std::uint32_t,unsigned>> faults;
    unsigned fixtures{},stores{};
    for (const auto stage : {Stage::notify,Stage::apply,Stage::publish})
        for (unsigned scenario=0;scenario<(stage==Stage::publish?1u:6u);++scenario)
            for (unsigned variant=0;variant<32;++variant) try {
                const auto test=make_case(random,stage,scenario,variant);
                const auto actual=check(test,stage,0);
                ++fixtures;
                stores+=actual.memory.stores.size();
                std::map<std::pair<Access,std::uint32_t>,unsigned> multiplicity;
                for (unsigned n=0;n<actual.memory.events.size();++n) {
                    const auto &[access,pc,address,width]=actual.memory.events[n];
                    if (access==Access::instruction) sites.insert(pc);
                    if (access==Access::child) children.insert(pc);
                    if (faults.emplace(access,pc,multiplicity[{access,pc}]++).second) (void)check(test,stage,n+1);
                }
            } catch (const std::exception &error) {
                throw std::runtime_error(std::to_string(unsigned(stage))+"/"+std::to_string(scenario)+"/"+
                                         std::to_string(variant)+" "+error.what());
            }
    std::cout<<nlohmann::json{{"cases",fixtures},{"sites",sites},{"children",children},
        {"fault_cases",faults.size()},{"stores",stores}}.dump()<<'\n';
} catch (const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
