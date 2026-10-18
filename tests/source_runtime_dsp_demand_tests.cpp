#include "halo2/source_runtime_dsp_section.hpp"
#include "halo2/cache.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
template<class F> void rejects(F&& operation,const char* message) {
    bool rejected=false;
    try { operation(); } catch(const FormatError&) { rejected=true; }
    check(rejected,message);
}
}
int main() try {
    SourceRuntimeMemory bank;
    const auto missing=std::filesystem::temp_directory_path()/
        ("halo2-unrequested-dsp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))/
        "default.xbe";
    check(!std::filesystem::exists(missing),"DSP absence control unexpectedly exists");
    SourceRuntimeDspSection section(bank,missing,SourceRuntimeDspSection::LoadPolicy::on_demand);
    const auto unloaded=[&] {
        check(section.address()==0 && section.active_references()==0 && section.resident_references()==0,
              "Unrequested or failed DSP load published ownership");
        section.require_quiescent();
    };
    unloaded();
    rejects([&]{ section.release(); },"Unmatched DSP release was accepted");
    rejects([&]{ section.retained_bytes(); },"Unrequested DSP bytes were supplied");
    unloaded();
    // A real request must still fail without its resource. Failure is atomic
    // and retryable; it must not grant a reference or manufacture a result.
    for(unsigned attempt=0;attempt<2;++attempt) {
        rejects([&]{ SourceRuntimeDspSection::CallScope scope(&section); section.retain(); },
                "Missing requested DSP resource was accepted");
        unloaded();
    }
    rejects([&]{ SourceRuntimeDspSection eager(bank,missing); },
            "Original eager reference adapter stopped requiring its image");
    rejects([&]{ SourceRuntimeDspSection invalid(bank,missing,SourceRuntimeDspSection::LoadPolicy(255)); },
            "Invalid DSP loading policy was accepted");
    std::cout<<"PASS native PCM has no unrequested DSP image prerequisite; real requests reject absent resources atomically\n";
    return 0;
} catch(const std::exception& e) {
    std::cerr<<e.what()<<'\n';return 1;
}
