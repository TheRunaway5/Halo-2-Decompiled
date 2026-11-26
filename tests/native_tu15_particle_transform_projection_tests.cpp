#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_transform_projection.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace frame_misc_harness;
using Json = nlohmann::json;

namespace {
struct Stop { std::uint32_t child{}; };
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::byte> from_hex(const std::string &hex) {
    std::vector<std::byte> bytes;
    for (std::size_t n=0; n<hex.size(); n+=2)
        bytes.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
    return bytes;
}

struct MemoryProbe : Memory {
    std::uint32_t pc{};
    std::string phase;
    unsigned ordinal{}, accesses{}, reopen_after{};
    bool *active_trace{};
    std::vector<SourcePhysicsContactRecordStore> writes;
    mutable std::vector<std::array<std::uint32_t,3>> reads;

    void committed() {
        if (++accesses==reopen_after) *active_trace=true;
    }

    void read(std::uint32_t address,std::span<std::byte> bytes) const override {
        constexpr std::array stores{0x250bd0u,0x250bd6u,0x250bdcu,0x250c04u,0x250c06u,0x250c07u,
            0x250c0cu,0x250c17u,0x250c3cu,0x250c42u,0x250c47u};
        const bool preimage=std::ranges::find(stores,pc)!=stores.end();
        if (!preimage && phase=="before_read" && reads.size()==ordinal) throw Stop{};
        Memory::read(address,bytes);
        if (!preimage) {
            reads.push_back({pc,address,std::uint32_t(bytes.size())});
            const_cast<MemoryProbe *>(this)->committed();
        }
    }

    void write(std::uint32_t address,std::span<const std::byte> bytes) override {
        if (phase=="before_write" && writes.size()==ordinal) throw Stop{};
        Memory::write(address,bytes);
        committed();
    }

    void observe(const SourcePhysicsContactRecordStore &store) {
        writes.push_back(store);
        if (phase=="after_write" && writes.size()==ordinal+1) throw Stop{};
    }
};

namespace prepared {
std::uint32_t fixture_entry;
void run_1458a0(WorldState &) { throw Stop{0x1458a0u}; }
void run_1459b0(WorldState &) { throw Stop{0x1459b0u}; }
#include "particle_transform_projection_owner.inc"
} // namespace prepared

void compare(const Json &input,const Json &expected,bool traced,unsigned reopen_after=0,bool second_trace=false) {
    MemoryProbe memory;
    for (const auto &bank:input.at("banks")) {
        const auto bytes=from_hex(bank.at("before"));
        const auto address=bank.at("address").get<std::uint32_t>();
        for (unsigned n=0;n<bytes.size();++n) memory.put(address+n,std::to_integer<unsigned>(bytes[n]),1);
    }
    SourcePhysicsWorldRuntimeAbi initial;
    initial.gp=input.at("gp").get<decltype(initial.gp)>();
    initial.eflags=input.at("eflags");
    initial.xmm=input.at("xmm").get<decltype(initial.xmm)>();
    const auto &fp=input.at("before_fp");
    initial.floating_point={fp[0],fp[2]};
    initial.x87_status=fp[1];
    initial.x87_depth=input.at("live_x87").size();
    for (unsigned n=0;n<initial.x87_depth;++n) {
        const auto lane=from_hex(input.at("live_x87")[n]);
        std::memcpy(&initial.x87[n],lane.data(),10);
    }
    memory.put(initial.gp[4],0x63000000u);
    memory.put(initial.gp[4]+4,input.at("matrix"));
    memory.put(initial.gp[4]+8,input.at("source"));
    if (input.contains("fault")) {
        memory.phase=input["fault"].at("phase");
        memory.ordinal=input["fault"].at("ordinal");
    }
    bool first=traced,second=false;
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    std::vector<std::uint32_t> observed;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc) { memory.pc=pc;observed.push_back(pc); };
    services.instruction_cursor={&memory.pc,&first,&second,service_sites,std::this_thread::get_id(),
        &services.instruction.target_type(),true,true};
    memory.active_trace=second_trace?&second:&first;
    memory.reopen_after=reopen_after;
    SourcePhysicsContactRecordObserver observe=[&](const auto &store) { memory.observe(store); };
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(initial);
    prepared::fixture_entry=input.at("entry");
    Stop stop;
    bool intercepted{};
    try { prepared::run_250ac0(state); }
    catch (Stop received) { stop=received;intercepted=true; }
    if (!expected.at("completed").get<bool>()) {
        require(intercepted && !stop.child && memory.pc==expected.at("boundary").at("instruction").get<std::uint32_t>(),
                "Projection rejected access cursor differs");
    } else {
        const auto successor=expected.at("stopped_at").get<std::uint32_t>();
        if (successor==0x63000000u) require(!intercepted && memory.pc==0x250c50u,"Original return differs");
        else require(intercepted && stop.child==successor && memory.pc==(successor==0x1458a0u?0x250c0cu:0x250c17u),
                     "Genuine matrix call boundary or cursor differs");
    }
    const auto actual=state.save();
    require(actual.gp==expected.at("boundary_gp").get<decltype(initial.gp)>(),"Projection GP differs");
    require(actual.eflags==expected.at("boundary_eflags").get<std::uint32_t>(),"Projection EFLAGS differs");
    require(actual.xmm==expected.at("boundary_xmm").get<decltype(initial.xmm)>(),"Projection XMM differs");
    const auto &after=expected.at("after_fp"), &x87=expected.at("boundary_x87");
    require(actual.x87_status==after[1] && actual.floating_point->x87_control_word==after[0] &&
            actual.floating_point->mxcsr==after[2] && actual.x87_depth==x87.size(),"Projection x87/MXCSR differs");
    for (unsigned n=0;n<actual.x87_depth;++n) {
        const auto lane=from_hex(x87[n]);
        require(!std::memcmp(&actual.x87[n],lane.data(),10),"Projection extended value differs");
    }
    const auto &reads=expected.at("reads"), &writes=expected.at("writes");
    if (memory.reads.size()!=reads.size() || memory.writes.size()!=writes.size())
        throw std::runtime_error("Projection ordered access count differs, expected "+
            std::to_string(reads.size())+"/"+std::to_string(writes.size())+" read/write, actual "+
            std::to_string(memory.reads.size())+"/"+std::to_string(memory.writes.size())+
            " reads "+Json(memory.reads).dump()+" expected "+reads.dump());
    for (unsigned n=0;n<reads.size();++n)
        require(memory.reads[n]==std::array<std::uint32_t,3>{reads[n]["instruction"],reads[n]["address"],reads[n]["bytes"]},
                "Projection ordered read differs");
    for (unsigned n=0;n<writes.size();++n)
        require(memory.writes[n]==SourcePhysicsContactRecordStore{writes[n]["instruction"],writes[n]["address"],
                from_hex(writes[n]["before"]),from_hex(writes[n]["written"])},"Projection ordered store differs");
    for (const auto &bank:expected.at("banks")) {
        const auto bytes=from_hex(bank.at("after"));
        std::vector<std::byte> actual(bytes.size());
        memory.Memory::read(bank.at("address"),actual);
        require(actual==bytes,"Projection final bank differs");
    }
    if (reopen_after) {
        std::vector<std::uint32_t> memory_sites;
        for (const auto &read:reads) memory_sites.push_back(read.at("instruction"));
        for (const auto &write:writes) memory_sites.push_back(write.at("instruction"));
        std::ranges::sort(memory_sites);
        auto sites=expected.at("executed_sites").get<std::vector<std::uint32_t>>();
        sites.erase(sites.begin(),std::ranges::upper_bound(sites,memory_sites.at(reopen_after-1)));
        require(sites==observed,"Projection resumed at a different original instruction");
    } else if (!traced) require(observed.empty(),"Quiet projection delivered unexpected callbacks");
}
} // namespace

int main(int argc,char **argv) try {
    require(argc==2,"Supply the managed original projection fixture");
    std::ifstream file(argv[1]);
    const auto fixture=Json::parse(file);
    unsigned faults{},resumed{},children{};
    for (unsigned n=0;n<fixture["input"].size();++n) try {
        const auto &input=fixture["input"][n], &expected=fixture["expected"][n];
        compare(input,expected,false);
        compare(input,expected,true);
        faults+=input.contains("fault");
        children+=expected.value("completed",false) && expected.value("stopped_at",0u)!=0x63000000u;
        if (input.value("reopen",false)) {
            const auto count=expected["reads"].size()+expected["writes"].size();
            for (unsigned frontier=1;frontier<=count;++frontier) {
                compare(input,expected,false,frontier);
                compare(input,expected,false,frontier,true);
                resumed+=2;
            }
        }
    } catch (const std::exception &error) {
        throw std::runtime_error(fixture["input"][n].at("name").get<std::string>()+" "+error.what());
    }
    std::cout<<Json{{"cases",fixture["input"].size()},{"faults",faults},{"resumed",resumed},{"true_child_entries",children}}.dump()<<'\n';
} catch (const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
