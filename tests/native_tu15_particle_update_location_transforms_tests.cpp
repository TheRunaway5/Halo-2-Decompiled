#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_update_location_transforms.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace frame_misc_harness;
using Json = nlohmann::json;

namespace {
struct Stop {};
void require(bool condition,const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::byte> decode(const std::string &hex) {
    std::vector<std::byte> bytes;
    for (std::size_t n=0;n<hex.size();n+=2)
        bytes.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
    return bytes;
}

SourcePhysicsContactRecordStore event(const Json &physical) {
    return {physical.at("instruction"),physical.at("address"),decode(physical.at("before")),decode(physical.at("written"))};
}

void load_fp(SourcePhysicsWorldRuntimeAbi &abi,const Json &control,const Json &lanes) {
    abi.floating_point={control[0],control[2]};
    abi.x87_status=control[1];
    abi.x87_depth=lanes.size();
    for (unsigned n=0;n<abi.x87_depth;++n) {
        const auto lane=decode(lanes[n]);
        std::memcpy(&abi.x87[n],lane.data(),10);
    }
}

void verify(WorldState &state,const Json &gp,const Json &flags,const Json &xmm,
            const Json &control,const Json &lanes,const std::string &label) {
    const auto abi=state.save();
    require(abi.gp==gp.get<decltype(abi.gp)>(),label+" GP actual "+Json(abi.gp).dump()+" expected "+gp.dump());
    require(abi.eflags==flags.get<std::uint32_t>(),label+" flags actual "+Json(abi.eflags).dump()+" expected "+flags.dump());
    require(abi.xmm==xmm.get<decltype(abi.xmm)>(),label+" XMM differs");
    require(abi.floating_point->x87_control_word==control[0] && abi.x87_status==control[1] &&
            abi.floating_point->mxcsr==control[2] && abi.x87_depth==lanes.size(),label+" FP environment differs");
    for (unsigned n=0;n<abi.x87_depth;++n) {
        const auto bytes=decode(lanes[n]);
        require(!std::memcmp(&abi.x87[n],bytes.data(),10),label+" x87 lane differs");
    }
}

struct Probe : Memory {
    std::uint32_t pc{};
    std::string fault;
    unsigned ordinal{}, reopen{};
    bool *trace{};
    std::vector<std::uint32_t> observed;
    std::vector<std::size_t> successors;
    std::vector<SourcePhysicsContactRecordStore> writes;
    mutable Json reads=Json::array();

    void committed() {
        successors.push_back(observed.size());
        if (successors.size()==reopen) *trace=true;
    }

    void read(std::uint32_t address,std::span<std::byte> bytes) const override {
        constexpr std::array stores{0x251f70u,0x251f71u,0x251f72u,0x251f73u,0x251f79u,0x251f89u,0x251fb2u};
        const bool preimage=std::ranges::find(stores,pc)!=stores.end();
        if (!preimage && fault=="before_read" && reads.size()==ordinal) throw Stop{};
        Memory::read(address,bytes);
        if (!preimage) {
            reads.push_back(Json{{"instruction",pc},{"address",address},{"bytes",bytes.size()}});
            const_cast<Probe *>(this)->committed();
        }
    }

    void write(std::uint32_t address,std::span<const std::byte> bytes) override {
        if (fault=="before_write" && writes.size()==ordinal) throw Stop{};
        Memory::write(address,bytes);
        committed();
    }

    void observe(const SourcePhysicsContactRecordStore &store) {
        writes.push_back(store);
        if (fault=="after_write" && writes.size()==ordinal+1) throw Stop{};
    }
};

namespace prepared {
std::function<void(WorldState &,std::uint32_t)> child;
void run_178070(WorldState &state) {child(state,0x178070u);}
void run_250ac0(WorldState &state) {child(state,0x250ac0u);}
#include "particle_update_location_transforms_owner.inc"
} // namespace prepared

struct Result {
    unsigned calls{},accesses{};
    std::vector<std::uint32_t> instructions;
    std::vector<std::size_t> successors;
};

Result compare(const Json &input,const Json &physical,bool traced,unsigned reopen=0,bool second_trace=false,
               unsigned child_reopen=0) {
    Probe memory;
    for (const auto &bank:input.at("banks")) {
        const auto bytes=decode(bank.at("before"));
        const auto base=bank.at("address").get<std::uint32_t>();
        for (unsigned n=0;n<bytes.size();++n) memory.put(base+n,std::to_integer<unsigned>(bytes[n]),1);
    }
    SourcePhysicsWorldRuntimeAbi initial;
    initial.gp=input.at("gp").get<decltype(initial.gp)>();
    initial.eflags=input.at("eflags");
    initial.xmm=input.at("xmm").get<decltype(initial.xmm)>();
    load_fp(initial,input.at("before_fp"),input.at("live_x87"));
    memory.put(initial.gp[4],0x63000000u);
    memory.put(initial.gp[4]+4,input.at("matrix"));
    memory.put(initial.gp[4]+8,input.at("source"));
    if (input.contains("fault")) {
        memory.fault=input.at("fault").at("phase");
        memory.ordinal=input.at("fault").at("ordinal");
    }
    bool first=traced,second=false;
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc) {memory.pc=pc;memory.observed.push_back(pc);};
    services.instruction_cursor={&memory.pc,&first,&second,service_sites,std::this_thread::get_id(),
        &services.instruction.target_type(),true,true};
    SourcePhysicsContactRecordObserver observe=[&](const auto &store){memory.observe(store);};
    memory.trace=second_trace?&second:&first;
    memory.reopen=reopen;
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(initial);

    const auto &calls=physical.at("calls"),&reads=physical.at("reads"),&writes=physical.at("writes");
    std::vector<std::size_t> child_returns;
    unsigned boundary{};
    prepared::child=[&](WorldState &in,std::uint32_t target) {
        const auto first_pc=target==0x178070u?0x251f79u:0x251fb2u;
        const auto returned=target==0x178070u?0x251f7eu:0x251fb7u;
        const auto &entry=calls.at(boundary++),&exit=calls.at(boundary++);
        require(entry.at("entry")==target && exit.at("entry")==returned && memory.pc==first_pc,
                "Original child/return address or cursor differs");
        require(memory.get(in.gp[4])==returned,"Original pushed return word differs");
        verify(in,entry.at("gp"),entry.at("eflags"),entry.at("xmm"),entry.at("fp"),entry.at("live_x87"),"child entry");
        for (auto n=memory.reads.size();n<reads.size() && reads[n]["instruction"].get<std::uint32_t>()<0x251f70u;++n)
            memory.reads.push_back(reads[n]);
        for (auto n=memory.writes.size();n<writes.size() && writes[n]["instruction"].get<std::uint32_t>()<0x251f70u;++n) {
            auto stored=event(writes[n]);
            std::vector<std::byte> prior(stored.before.size());
            memory.Memory::read(stored.address,prior);
            require(prior==stored.before,"Original child store preimage differs");
            memory.Memory::write(stored.address,stored.written);
            memory.writes.push_back(std::move(stored));
        }
        auto result=in.save();
        result.gp=exit.at("gp").get<decltype(result.gp)>();
        result.eflags=exit.at("eflags");
        result.xmm=exit.at("xmm").get<decltype(result.xmm)>();
        load_fp(result,exit.at("fp"),exit.at("live_x87"));
        in.load(result);
        child_returns.push_back(memory.successors.size());
        if (child_reopen && child_returns.size()==child_reopen) *memory.trace=true;
    };

    bool stopped{};
    try {prepared::run_251f70(state);} catch (const Stop &) {stopped=true;}
    prepared::child={};
    require(stopped==input.contains("fault"),"Original return/fault outcome differs");
    require(memory.pc==(stopped?physical.at("boundary").at("instruction").get<std::uint32_t>():0x251fcau),
            "Original return/fault cursor differs");
    require(boundary==calls.size(),"Original child call count differs");
    verify(state,physical.at("boundary_gp"),physical.at("boundary_eflags"),physical.at("boundary_xmm"),
           physical.at("after_fp"),physical.at("boundary_x87"),"exit");
    require(memory.reads==reads,"Original ordered read stream differs");
    require(memory.writes.size()==writes.size(),"Original ordered store count differs");
    for (unsigned n=0;n<writes.size();++n)
        require(memory.writes[n]==event(writes[n]),"Original ordered store differs at "+std::to_string(n));
    for (const auto &bank:physical.at("banks")) {
        const auto bytes=decode(bank.at("after"));
        std::vector<std::byte> actual(bytes.size());
        memory.Memory::read(bank.at("address"),actual);
        require(bytes==actual,"Original final bank differs");
    }
    if (!traced && !reopen && !child_reopen) require(memory.observed.empty(),"Quiet parent delivered callbacks");
    const auto accesses=unsigned(memory.successors.size());
    if (child_reopen) {
        require(child_reopen<=child_returns.size(),"Missing trace activation from original child return");
        memory.successors={child_returns[child_reopen-1]};
    }
    return {boundary/2,accesses,std::move(memory.observed),std::move(memory.successors)};
}
} // namespace

int main(int argc,char **argv) try {
    require(argc==2,"Supply original location transform fixture");
    std::ifstream file(argv[1]);
    const auto fixture=Json::parse(file);
    unsigned cases{},faults{},reopened{},child_reopens{},children{};
    for (unsigned n=0;n<fixture.at("input").size();++n) try {
        const auto &input=fixture["input"][n],&physical=fixture["expected"][n];
        const auto traced=compare(input,physical,true);
        const auto quiet=compare(input,physical,false);
        require(traced.calls==quiet.calls && traced.accesses==quiet.accesses,"Quiet and traced route extent differs");
        ++cases;
        faults+=input.contains("fault");
        children+=quiet.calls;
        if (!input.value("reopen",false)) continue;
        for (unsigned at=1;at<=quiet.accesses;++at) {
            const auto tail=traced.instructions.begin()+traced.successors.at(at-1);
            for (bool secondary:{false,true}) {
                auto observed=compare(input,physical,false,at,secondary);
                require(std::ranges::equal(std::ranges::subrange(tail,traced.instructions.end()),observed.instructions),
                        "Original successor trace differs after provider "+std::to_string(at));
                ++reopened;
            }
        }
        for (unsigned call=1;call<=quiet.calls;++call) {
            auto observed=compare(input,physical,false,0,true,call);
            const auto tail=traced.instructions.begin()+traced.successors.at(observed.successors.at(0)-1);
            require(std::ranges::equal(std::ranges::subrange(tail,traced.instructions.end()),observed.instructions),
                    "Original successor trace differs after genuine child return "+std::to_string(call));
            ++child_reopens;
        }
    } catch (const std::exception &error) {
        throw std::runtime_error(fixture["input"][n]["name"].get<std::string>()+" "+error.what());
    }
    std::cout<<Json{{"cases",cases},{"faults",faults},{"child_calls",children},{"ordered_trace_resumes",reopened},
                    {"trace_resumes_from_child",child_reopens}}.dump()<<'\n';
} catch (const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
