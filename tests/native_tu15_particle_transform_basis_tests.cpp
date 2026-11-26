#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_transform_basis.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace basis = halo2::native_particle_transform_basis_detail;
using namespace frame_misc_harness;
using Json = nlohmann::json;

namespace prepared {
void run_1458a0(WorldState &) { throw std::runtime_error("Unexpected first matrix child after basis span"); }
void run_1459b0(WorldState &) { throw std::runtime_error("Unexpected second matrix child after basis span"); }
#include "particle_transform_basis_owner.inc"
} // namespace prepared

namespace {
struct Boundary {};

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::byte> decode(const std::string &hex) {
    std::vector<std::byte> bytes;
    for (std::size_t n=0; n<hex.size(); n+=2)
        bytes.push_back(std::byte(std::stoul(hex.substr(n,2), nullptr, 16)));
    return bytes;
}

struct MemoryProbe : Memory {
    std::uint32_t site{};
    std::string phase;
    unsigned ordinal{}, trace_frontier{}, accesses{};
    bool *trace{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    mutable std::vector<std::array<std::uint32_t,3>> reads;

    void complete_access() {
        if (++accesses==trace_frontier) *trace=true;
    }

    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        constexpr std::array stores{0x250acbu,0x250accu,0x250acdu,0x250ad8u,0x250addu,0x250ae3u,
            0x250aeeu,0x250af3u,0x250af9u,0x250b04u,0x250b09u,0x250b0fu,0x250b1cu,
            0x250b21u,0x250b27u,0x250b3au,0x250b44u,0x250b48u};
        const bool observed_preimage=std::ranges::find(stores,site)!=stores.end();
        if (!observed_preimage && phase=="before_read" && reads.size()==ordinal) throw Boundary{};
        Memory::read(address, bytes);
        if (!observed_preimage) {
            reads.push_back({site,address,std::uint32_t(bytes.size())});
            const_cast<MemoryProbe *>(this)->complete_access();
        }
    }

    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        if (phase=="before_write" && stores.size()==ordinal) throw Boundary{};
        Memory::write(address, bytes);
        complete_access();
    }

    void observe(const SourcePhysicsContactRecordStore &store) {
        stores.push_back(store);
        if (phase=="after_write" && stores.size()==ordinal+1) throw Boundary{};
    }
};

void compare(const Json &input, const Json &expected, bool retained, unsigned trace_frontier=0, bool second_trace=false) {
    MemoryProbe memory;
    for (const auto &bank : input.at("banks")) {
        const auto bytes=decode(bank.at("before"));
        const auto address=bank.at("address").get<std::uint32_t>();
        for (unsigned n=0; n<bytes.size(); ++n) memory.put(address+n,std::to_integer<unsigned>(bytes[n]),1);
    }
    SourcePhysicsWorldRuntimeAbi initial;
    initial.gp=input.at("gp").get<decltype(initial.gp)>();
    initial.eflags=input.at("eflags");
    initial.xmm=input.at("xmm").get<decltype(initial.xmm)>();
    const auto &fp=input.at("before_fp");
    initial.floating_point={fp[0],fp[2]};
    initial.x87_status=fp[1];
    initial.x87_depth=input.at("live_x87").size();
    for (unsigned n=0; n<initial.x87_depth; ++n) {
        const auto lane=decode(input.at("live_x87")[n]);
        std::memcpy(&initial.x87[n],lane.data(),10);
    }
    memory.put(initial.gp[4],0x63000000);
    memory.put(initial.gp[4]+4,input.at("matrix"));
    memory.put(initial.gp[4]+8,input.at("source"));
    if (input.contains("fault")) {
        memory.phase=input["fault"].at("phase");
        memory.ordinal=input["fault"].at("ordinal");
    }
    bool first=retained, second=false;
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    const auto end=expected.at("completed").get<bool>() ? expected.at("stopped_at").get<std::uint32_t>() : 0u;
    std::vector<std::uint32_t> observed;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t site) {
        memory.site=site;
        if (site==end) throw Boundary{};
        observed.push_back(site);
    };
    services.instruction_cursor={&memory.site,&first,&second,service_sites,std::this_thread::get_id(),
        &services.instruction.target_type(),true,true};
    memory.trace=second_trace?&second:&first;
    memory.trace_frontier=trace_frontier;
    SourcePhysicsContactRecordObserver observer=[&](const auto &store) { memory.observe(store); };
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(initial);
    std::uint32_t resume{};
    bool stopped{};
    try {
        if (retained || trace_frontier) prepared::run_250ac0(state);
        else resume=basis::bridge(state);
    } catch (const Boundary &) {
        stopped=true;
    }
    if (retained || trace_frontier || !end) {
        require(stopped,"Expected precise monitored boundary");
        require(memory.site==(end?end:expected.at("boundary").at("instruction").get<std::uint32_t>()),
                "Original/provider instruction cursor differs");
    } else require(!stopped && resume==end,"Different native basis continuation");
    const auto actual=state.save();
    require(actual.gp==expected.at("boundary_gp").get<decltype(initial.gp)>(),"Original GP differs");
    require(actual.eflags==expected.at("boundary_eflags").get<std::uint32_t>(),"Original EFLAGS differs");
    require(actual.xmm==expected.at("boundary_xmm").get<decltype(initial.xmm)>(),"Original XMM differs");
    const auto &after=expected.at("after_fp");
    require(actual.x87_status==after[1] && actual.floating_point->x87_control_word==after[0] &&
            actual.floating_point->mxcsr==after[2] && actual.x87_depth==initial.x87_depth,"Original x87/MXCSR differs");
    for (unsigned n=0; n<actual.x87_depth; ++n)
        require(!std::memcmp(&actual.x87[n],&initial.x87[n],10),"Original live x87 lane differs");
    auto reads=expected.at("reads");
    require(memory.reads.size()==reads.size(),"Original ordered read count differs");
    for (unsigned n=0; n<reads.size(); ++n)
        require(memory.reads[n]==std::array<std::uint32_t,3>{reads[n]["instruction"],reads[n]["address"],reads[n]["bytes"]},
                "Original ordered read differs");
    auto writes=expected.at("writes");
    require(memory.stores.size()==writes.size(),"Original ordered store count differs");
    for (unsigned n=0; n<writes.size(); ++n)
        require(memory.stores[n]==SourcePhysicsContactRecordStore{writes[n]["instruction"],writes[n]["address"],
                decode(writes[n]["before"]),decode(writes[n]["written"])},"Original ordered store differs");
    for (const auto &bank : expected.at("banks")) {
        const auto bytes=decode(bank.at("after"));
        const auto address=bank.at("address").get<std::uint32_t>();
        std::vector<std::byte> actual(bytes.size());
        memory.Memory::read(address,actual);
        require(actual==bytes,"Original final bank differs");
    }
    if (trace_frontier) {
        std::vector<std::uint32_t> memory_sites;
        for (const auto &read : reads) memory_sites.push_back(read.at("instruction"));
        for (const auto &write : writes) memory_sites.push_back(write.at("instruction"));
        std::ranges::sort(memory_sites);
        const auto committed=memory_sites.at(trace_frontier-1);
        auto sites=expected.at("executed_sites").get<std::vector<std::uint32_t>>();
        sites.erase(sites.begin(), std::ranges::upper_bound(sites,committed));
        require(sites==observed,"Original instruction trace suffix differs");
    } else if (!retained) require(observed.empty(),"Quiet basis delivered original callbacks");
}
} // namespace

int main(int argc,char **argv) try {
    require(argc==2,"Supply the managed original basis fixture");
    std::ifstream file(argv[1]);
    const auto fixture=Json::parse(file);
    unsigned faults{}, resumes{};
    for (unsigned n=0; n<fixture.at("input").size(); ++n) try {
        const auto &input=fixture["input"][n], &expected=fixture["expected"][n];
        compare(input,expected,false);
        compare(input,expected,true);
        faults+=input.contains("fault");
        if (input.value("reopen",false)) {
            const auto count=expected.at("reads").size()+expected.at("writes").size();
            for (unsigned frontier=1; frontier<=count; ++frontier) {
                compare(input,expected,false,frontier);
                compare(input,expected,false,frontier,true);
                resumes+=2;
            }
        }
    } catch (const std::exception &error) {
        throw std::runtime_error(fixture["input"][n].at("name").get<std::string>()+" "+error.what());
    }
    std::cout<<Json{{"cases",fixture.at("input").size()},{"faults",faults},{"trace_resumes",resumes}}.dump()<<'\n';
} catch (const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
