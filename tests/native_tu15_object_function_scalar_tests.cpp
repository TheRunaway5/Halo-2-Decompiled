#include "native_tu15_object_function_scalar_physical.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace candidate = halo2::native_object_function_scalar_detail;
namespace {
struct DescendantFrontier {};
struct Fixture {
    const J& row;
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    explicit Fixture(const J& input) : row(input) {
        for (const auto& region : row.at("banks"))
            banks.emplace(region.at("address"), bytes(region.at("before")));
    }
};
struct Provider final : Memory {
    WorldState* state{};
    mutable J trace = J::array();
    int stop{-1};
    std::string_view feedback;
    void event(std::string_view kind, std::uint32_t site, J value) const {
        trace.push_back(J{{"kind",kind},{"pc",site},{"abi",packet(*state,site)},{"value",std::move(value)}});
        if (int(trace.size())-1 == stop) throw Boundary{};
    }
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        event("read", pc, J::array({address, output.size()}));
        Memory::read(address, output);
    }
    void write(std::uint32_t address, std::span<const std::byte> input) override {
        event("write", pc, J::array({address, hex(input)}));
        Memory::write(address, input);
    }
    void update_word(std::uint32_t address, std::uint32_t value) {
        const auto found = at(address, 4);
        auto& bank = banks.at(found->first);
        std::memcpy(bank.data()+address-found->first,&value,4);
    }
};
#include "retained_direct.inc"
#include "retained_literal.inc"

struct Result {
    J trace, abi, reads, writes;
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    int outcome{};
    friend bool operator==(const Result&, const Result&) = default;
};

Result run(const Fixture& input, bool literal, int mode, std::string_view feedback={}, int stop=-1) {
    Provider memory;
    memory.banks = input.banks;
    memory.stop = stop;
    memory.feedback = feedback;
    memory.observer_preimages = true;
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = input.row.at("gp");
    abi.eflags = input.row.at("eflags");
    abi.xmm = input.row.at("xmm");
    const auto& fp = input.row.at("before_fp");
    abi.floating_point = SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2]};
    abi.x87_status = fp[1];
    abi.x87_depth = input.row.at("live_x87").size();
    for (unsigned i=0; i<abi.x87_depth; ++i) {
        const auto lane = bytes(input.row.at("live_x87")[i]);
        std::memcpy(&abi.x87[i], lane.data(), 10);
    }
    SourcePhysicsWorldRuntimeServices services;
    bool full_trace = true;
    services.instruction = [&](std::uint32_t pc) {
        memory.pc = pc;
        memory.event("instruction", pc, J{});
        if (feedback == "row-key" && pc == 0xbcc59u)
            memory.update_word(memory.state->gp[4]+28u,memory.state->gp[2]);
    };
    static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if (literal) services.instruction_cursor = {&memory.pc,&full_trace,nullptr,sites,std::this_thread::get_id(),
                                                &services.instruction.target_type(),true};
    const SourcePhysicsContactRecordObserver observer = [&](const auto& event) {
        memory.event("observer",event.instruction,J::array({event.address,hex(event.before),hex(event.written)}));
        if (feedback == "replace-count" && event.instruction == 0xbcc6cu)
            memory.update_word(memory.state->gp[7]+0x64u,1u);
        if (feedback == "replace-output" && event.instruction == 0xbcb8eu)
            memory.state->gp[5] = 0x61002ea0u;
    };
    const physics_step_detail::FpScope scope{*abi.floating_point};
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);
    memory.state = &state;
    int outcome{};
    try {
        if (literal) { literal_source::native = mode; literal_source::run_bcb70(state); }
        else { direct_source::native = mode; direct_source::run_bcb70(state); }
    } catch (const DescendantFrontier&) { outcome=1; }
      catch (const Boundary&) { outcome=2; }
    auto reads=memory.architectural_reads();
    return {std::move(memory.trace),packet(state,memory.pc),std::move(reads),std::move(memory.writes),std::move(memory.banks),outcome};
}

void compare(const Result& expected, const Result& actual, const std::string& name) {
    if (expected == actual) return;
    std::size_t index{};
    while (index<expected.trace.size() && index<actual.trace.size() && expected.trace[index]==actual.trace[index]) ++index;
    const auto display = [index](const J& trace) {return index<trace.size()?trace[index].dump():"end";};
    throw std::runtime_error("BCB70 source difference "+name+" #"+std::to_string(index)+
        " retained="+display(expected.trace)+" typed="+display(actual.trace)+
        " escaped-retained="+expected.abi.dump()+" escaped-typed="+actual.abi.dump());
}

J compact_physical(const Result& result, const Fixture& input, const std::set<std::uint32_t>& trace_sites) {
    auto abi = result.abi;
    abi.erase("entry");
    J changes = J::array(), trace = J::array();
    for (const auto& [base, bank] : input.banks) {
        const auto& current = result.banks.at(base);
        for (std::size_t i=0;i<bank.size();++i) if (current[i]!=bank[i])
            changes.push_back(J::array({base+i,std::to_integer<unsigned>(current[i])}));
    }
    for (const auto& event : result.trace)
        if (event["kind"]=="instruction" && trace_sites.contains(event["pc"].get<std::uint32_t>())) trace.push_back(event["abi"]);
    return {{"name",input.row.at("name")},{"abi",abi},{"writes",result.writes},{"reads",result.reads},
            {"trace",trace},{"changes",changes}};
}
struct WriteFault {};
struct FaultProvider final : Memory {
    J attempted;
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        if (address == 0x61fffffcu || address == 0x64000000u) {
            attempted = {{"instruction",pc},{"address",address},{"bytes",bytes.size()}};
            throw WriteFault{};
        }
        Memory::write(address,bytes);
    }
};
J run_actual_fault(const J& row, bool physical) {
    FaultProvider memory;
    for (const auto& bank : row.at("banks")) memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    const auto before = memory.banks;
    SourcePhysicsContactRuntimeServices services;
    services.instruction=[&](auto pc){memory.pc=pc;};
    const SourcePhysicsContactRecordObserver observer;
    SourcePhysicsContactRuntimeAbi abi{};
    abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for (unsigned n=0;n<abi.x87_depth;++n) {
        const auto lane=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],lane.data(),10);
    }
    const physics_step_detail::FpScope scope{*abi.floating_point};
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    auto forbidden=[](auto&){throw std::runtime_error("BCB70 early fault unexpectedly reached a child");};
    const auto children=candidate::Children{forbidden,forbidden,forbidden,forbidden};
    bool stopped{};
    try {
        if (physical) candidate::run_physical(state,children,[&](auto&,std::uint32_t pc){memory.pc=pc;});
        else candidate::run_source<false>(state,children);
    } catch (const WriteFault&) { stopped=true; }
    check(stopped,"Missing BCB70 actual provider write attempt");
    J changes=J::array();
    for (const auto& [base,data] : before) {
        const auto& actual=memory.banks.at(base);
        for (std::size_t index=0;index<data.size();++index)
            if (data[index]!=actual[index]) changes.push_back(J::array({base+index,std::to_integer<unsigned>(actual[index])}));
    }
    auto escaped=packet(state,memory.pc);escaped.erase("entry");
    return {{"name",row.at("name")},{"abi",escaped},{"reads",memory.architectural_reads()},
            {"writes",memory.writes},{"changes",changes},{"attempt",memory.attempted}};
}
} // namespace

int main(int argc,char** argv) try {
    check(argc==4,"Supply authenticated BCB70 inputs, physical trace sites and result path");
    std::ifstream stream(argv[1]), sites_stream(argv[2]);
    const auto rows = J::parse(stream);
    const auto trace_sites = J::parse(sites_stream).get<std::set<std::uint32_t>>();
    J physical=J::array(), source=J::array(), faults=J::array();
    std::set<std::uint32_t> sites;
    unsigned returns{}, prefixes{}, packets{}, frontiers{};
    for (const auto& row : rows) {
        if (row.value("actual_write_fault",false)) {
            faults.push_back(J{{"physical",run_actual_fault(row,true)},{"source",run_actual_fault(row,false)}});
            continue;
        }
        const Fixture input{row};
        const auto name = row.at("name").get<std::string>();
        for (bool literal : {false,true}) {
            const auto expected=run(input,literal,false);
            const auto actual=run(input,literal,true);
            compare(expected,actual,name);
            check(actual.outcome==0,"Unexpected descendant frontier in BCB70 completed corpus: "+name);
            if (!literal) {
                source.push_back(compact_physical(actual,input,trace_sites));
                const auto raw = run(input,false,2);
                check(raw.outcome==0,"Unexpected physical BCB70 descendant frontier");
                physical.push_back(compact_physical(raw,input,trace_sites));
            }
            ++returns;packets+=expected.trace.size();
            for (const auto& event : expected.trace)
                if (event["kind"]=="instruction") sites.insert(event["pc"].get<std::uint32_t>());
            if (name.ends_with("-0") || name=="row-real-child-1") {
                for (const auto effect : {"row-key","replace-count","replace-output"}) {
                    const auto reference=run(input,literal,false,effect);
                    compare(reference,run(input,literal,true,effect),name+"/"+effect);
                    frontiers+=reference.outcome==1;
                    packets+=reference.trace.size();
                }
                for (std::size_t at=0;at<expected.trace.size();at+=5) {
                    const auto reference=run(input,literal,false,{},at);
                    check(reference.outcome==2,"Missing BCB70 provider prefix");
                    compare(reference,run(input,literal,true,{},at),name+"/prefix/"+std::to_string(at));
                    ++prefixes;packets+=reference.trace.size();
                }
            }
        }
    }
    std::ofstream output(argv[3]);output<<J{{"physical",physical},{"source",source},{"faults",faults}}.dump();
    unsigned parent_sites{};
    for (auto pc:sites) parent_sites+=pc>=0xbcb70u && pc<=0xbccdeu;
    check(parent_sites==138 && returns==112 && prefixes>0 && faults.size()==8,"BCB70 source proof lost actual parent sites or paths");
    std::cout<<J{{"passed",true},{"actual_complete_returns",returns},{"source_event_prefixes",prefixes},
        {"feedback_descendant_frontiers",frontiers},{"complete_packets",packets},{"parent_sites",parent_sites},
        {"genuine_write_fault_comparisons",faults.size()},{"supplied_child_outcomes",false}}.dump()<<'\n';
    return 0;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
