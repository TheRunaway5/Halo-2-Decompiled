#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_emitter_bound.hpp"
#include "rendering/effects/native_tu15_particle_transform_matrix_calls.hpp"
#include "rendering/effects/native_tu15_particle_transform_projection.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace frame_misc_harness;
using Json = nlohmann::json;

namespace {
struct Stop {};
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::byte> decode(const std::string &hex) {
    std::vector<std::byte> bytes;
    for (std::size_t n=0;n<hex.size();n+=2)
        bytes.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
    return bytes;
}

struct Probe : Memory {
    std::uint32_t pc{};
    std::string phase;
    unsigned ordinal{}, accesses{}, reopen{};
    bool *trace{};
    std::vector<std::uint32_t> store_sites;
    std::vector<SourcePhysicsContactRecordStore> writes;
    mutable std::vector<std::array<std::uint32_t,3>> reads;
    mutable bool copy_preimage{};

    void completed() {
        if (++accesses==reopen) *trace=true;
    }

    void read(std::uint32_t address,std::span<std::byte> bytes) const override {
        const bool copying=pc==0x1459c7u || pc==0x1459dcu;
        const bool preimage=copying?copy_preimage:std::ranges::find(store_sites,pc)!=store_sites.end();
        if (!preimage && phase=="before_read" && reads.size()==ordinal) throw Stop{};
        Memory::read(address,bytes);
        if (copying) copy_preimage=!copy_preimage;
        if (!preimage) {
            reads.push_back({pc,address,std::uint32_t(bytes.size())});
            const_cast<Probe *>(this)->completed();
        }
    }

    void write(std::uint32_t address,std::span<const std::byte> bytes) override {
        if (phase=="before_write" && writes.size()==ordinal) throw Stop{};
        Memory::write(address,bytes);
        completed();
    }

    void observe(const SourcePhysicsContactRecordStore &store) {
        writes.push_back(store);
        if (phase=="after_write" && writes.size()==ordinal+1) throw Stop{};
    }
};

namespace prepared {
std::uint32_t fixture_entry, fixture_return;
#include "particle_emitter_bound_owner.inc"
} // namespace prepared

void compare(const Json &input,const Json &expected,const std::vector<std::uint32_t> &store_sites,
             bool traced,unsigned reopen=0,bool second_trace=false) {
    Probe memory;
    memory.store_sites=store_sites;
    for (const auto &bank:input.at("banks")) {
        const auto bytes=decode(bank.at("before"));
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
        const auto lane=decode(input.at("live_x87")[n]);
        std::memcpy(&initial.x87[n],lane.data(),10);
    }
    memory.put(initial.gp[4],input.value("return_pc",0x63000000u));
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
    memory.trace=second_trace?&second:&first;
    memory.reopen=reopen;
    SourcePhysicsContactRecordObserver observe=[&](const auto &store) { memory.observe(store); };
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    s.load(initial);
    const auto entry=input.at("entry").get<std::uint32_t>();
    bool stopped{};
    try {
        if (entry==0x22ad60u) prepared::run_22ad60(s);
        else {
            prepared::fixture_entry=entry;
            prepared::fixture_return=input.at("stop_before_entries")[0];
            prepared::run_250ac0(s);
        }
    } catch (const Stop &) { stopped=true; }
    if (expected.at("completed").get<bool>()) {
        require(!stopped && memory.pc==input.at("last_pc").get<std::uint32_t>(),"Original returned cursor differs");
    } else {
        require(stopped && memory.pc==expected.at("boundary").at("instruction").get<std::uint32_t>(),"Original fault cursor differs");
    }
    const auto actual=s.save();
    require(actual.gp==expected.at("boundary_gp").get<decltype(initial.gp)>(),"Original GP differs");
    require(actual.eflags==expected.at("boundary_eflags").get<std::uint32_t>(),"Original EFLAGS differs");
    require(actual.xmm==expected.at("boundary_xmm").get<decltype(initial.xmm)>(),"Original XMM differs");
    const auto &after=expected.at("after_fp"), &x87=expected.at("boundary_x87");
    if (!(actual.x87_status==after[1] && actual.floating_point->x87_control_word==after[0] &&
          actual.floating_point->mxcsr==after[2] && actual.x87_depth==x87.size()))
        throw std::runtime_error("Original FP differs, actual "+
            Json::array({actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr,actual.x87_depth}).dump()+
            " expected "+after.dump());
    for (unsigned n=0;n<actual.x87_depth;++n) {
        const auto lane=decode(x87[n]);
        require(!std::memcmp(&actual.x87[n],lane.data(),10),"Original extended value differs");
    }
    const auto &reads=expected.at("reads"),&writes=expected.at("writes");
    if (memory.reads.size()!=reads.size() || memory.writes.size()!=writes.size())
        throw std::runtime_error("Original access count differs: "+std::to_string(memory.reads.size())+"/"+
            std::to_string(memory.writes.size())+" native, "+std::to_string(reads.size())+"/"+std::to_string(writes.size())+" i386");
    for (unsigned n=0;n<reads.size();++n)
        require(memory.reads[n]==std::array<std::uint32_t,3>{reads[n]["instruction"],reads[n]["address"],reads[n]["bytes"]},"Original ordered read differs");
    for (unsigned n=0;n<writes.size();++n)
        require(memory.writes[n]==SourcePhysicsContactRecordStore{writes[n]["instruction"],writes[n]["address"],
                decode(writes[n]["before"]),decode(writes[n]["written"])},"Original ordered store differs");
    for (const auto &bank:expected.at("banks")) {
        const auto bytes=decode(bank.at("after"));
        std::vector<std::byte> actual(bytes.size());
        memory.Memory::read(bank.at("address"),actual);
        require(actual==bytes,"Original final bank differs");
    }
    if (reopen) {
        // The call shell executes before lower-address matrix instructions.
        const auto order=[entry](std::uint32_t a,std::uint32_t b) {
            if (a==b) return false;
            if (a==entry) return true;
            if (b==entry) return false;
            return a<b;
        };
        std::vector<std::uint32_t> memory_sites;
        for (const auto &read:reads) memory_sites.push_back(read.at("instruction"));
        for (const auto &write:writes) memory_sites.push_back(write.at("instruction"));
        std::ranges::sort(memory_sites,order);
        auto sites=expected.at("executed_sites").get<std::vector<std::uint32_t>>();
        std::ranges::sort(sites,order);
        sites.erase(sites.begin(),std::ranges::upper_bound(sites,memory_sites.at(reopen-1),order));
        for (const auto replicate:{0x1459c7u,0x1459dcu}) {
            const auto at=std::ranges::find(sites,replicate);
            if (at!=sites.end()) {
                const auto copies=std::ranges::count_if(reads,[replicate](const auto &event) {return event.at("instruction")==replicate;});
                // The exact existing matrix owner calls the observer after
                // each REP iteration as well as at the initial instruction.
                sites.insert(at,copies,replicate);
            }
        }
        require(sites==observed,"Original trace reentry suffix differs");
    } else if (!traced) {
        std::vector<std::uint32_t> repeated_copy;
        for (const auto &read:reads)
            if (read["instruction"]==0x1459c7u || read["instruction"]==0x1459dcu)
                repeated_copy.push_back(read["instruction"]);
        require(repeated_copy==observed,"Quiet run delivered unexpected callbacks");
    }
}
} // namespace

int main(int argc,char **argv) try {
    require(argc==2,"Supply the managed original effect fixture");
    std::ifstream file(argv[1]);
    const auto fixture=Json::parse(file);
    const auto store_sites=fixture.at("store_sites").get<std::vector<std::uint32_t>>();
    std::array<unsigned,2> completed{},faults{},resumed{};
    for (unsigned n=0;n<fixture["input"].size();++n) try {
        const auto &input=fixture["input"][n],&expected=fixture["expected"][n];
        const unsigned scope=input["entry"]==0x22ad60u?0:1;
        compare(input,expected,store_sites,false);
        compare(input,expected,store_sites,true);
        (input.contains("fault")?faults:completed)[scope]++;
        if (input.value("reopen",false)) {
            // A matrix CALL hands a newly active trace to the real callee.
            // Emitter bounds can reopen after any committed memory access.
            const auto count=scope?1u:expected["reads"].size()+expected["writes"].size();
            for (unsigned frontier=1;frontier<=count;++frontier) {
                compare(input,expected,store_sites,false,frontier);
                compare(input,expected,store_sites,false,frontier,true);
                resumed[scope]+=2;
            }
        }
    } catch (const std::exception &error) {
        throw std::runtime_error(fixture["input"][n].at("name").get<std::string>()+" "+error.what());
    }
    std::cout<<Json{{"completed",completed},{"faults",faults},{"resumed",resumed}}.dump()<<'\n';
} catch (const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
