#!/usr/bin/env python3
"""Independent private-lifetime invalidation/nesting controls, with mutants."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include <type_traits>
#include <iostream>
#include <limits>
using namespace halo2;
static_assert(!std::is_copy_constructible_v<NativeTu15ContactCandidatePrivateData>);
static_assert(!std::is_copy_assignable_v<NativeTu15ContactCandidatePrivateData>);
static_assert(!std::is_move_constructible_v<NativeTu15ContactCandidatePrivateData>);
static_assert(!std::is_move_assignable_v<NativeTu15ContactCandidatePrivateData>);
void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
struct Bank:SourcePhysicsContactMemory{
    std::array<std::byte,256>data{};mutable unsigned reads{};unsigned writes{};
    void read(std::uint32_t p,std::span<std::byte>v)const override{
        check(p>=0x1000u&&std::uint64_t(p)+v.size()<=0x1100u,"outside bank");
        ++reads;std::copy_n(data.begin()+p-0x1000u,v.size(),v.begin());}
    void write(std::uint32_t p,std::span<const std::byte>v)override{
        check(p>=0x1000u&&std::uint64_t(p)+v.size()<=0x1100u,"outside bank");
        ++writes;std::copy(v.begin(),v.end(),data.begin()+p-0x1000u);}
};
struct Decline:NativeTu15RetainedStoreObservationDomain{
    bool observes_retained_store(std::uint32_t,std::uint32_t,std::size_t)const override{return true;}
};
int main(){try{
    unsigned controls=4;
    auto refuses=[&](auto f){bool caught{};try{f();}catch(const std::runtime_error&){caught=true;}
        check(caught,"negative admitted");++controls;};
    Bank bank;bank.data.fill(std::byte{0xa5});const auto bank_before=bank.data;
    Decline ordinary;check(!ordinary.withhold_defined_data(0x1000u,4),"default capability changed");++controls;
    const std::array<NativeTu15ContactPrivateRange,1>owned{{{0x2000u,64u}}},retained{{{0x1000u,256u}}};
    NativeTu15ContactCandidatePrivateData outer(bank,owned,retained);
    const std::array<std::byte,8>word{std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte{6},std::byte{7},std::byte{8}};
    outer.write(0x2000u,word);check(outer.withhold_defined_data(0x2002u,2),"withhold declined");
    check(outer.data_is_defined(0x2000u,2)&&!outer.data_is_defined(0x2000u,4)&&outer.data_is_defined(0x2004u,4),"partial mask differs");++controls;
    std::array<std::byte,4>read{};refuses([&]{outer.read(0x2000u,read);});
    check(outer.copy_defined_data(0x303ba0u,0x2010u,0x2000u,8)&&outer.data_is_defined(0x2010u,2)&&!outer.data_is_defined(0x2010u,4)&&outer.data_is_defined(0x2014u,4),"invalid mask copy differs");++controls;
    refuses([&]{outer.copy_defined_data(0x303ba0u,0x1000u,0x2010u,8);});
    check(outer.withhold_defined_data(0xffffffffu,0),"empty invalidation differs");++controls;
    refuses([&]{outer.observes_retained_store(0,0x2020u,std::numeric_limits<std::size_t>::max()-15u);});
    refuses([&]{outer.withhold_defined_data(0x2020u,std::numeric_limits<std::size_t>::max()-15u);});
    refuses([&]{outer.withhold_defined_data(0x203fu,2);});
    refuses([&]{outer.withhold_defined_data(0xffffffffu,2);});
    refuses([&]{outer.withhold_defined_data(0x1000u,4);});
    const std::array<NativeTu15ContactPrivateRange,1>nested_owned{{{0x3000u,64u}}};
    NativeTu15ContactCandidatePrivateData nested(static_cast<SourcePhysicsContactMemory&>(outer),nested_owned,retained);
    nested.write(0x3000u,word);outer.write(0x2020u,word);
    check(nested.withhold_defined_data(0x2020u,4)&&!nested.data_is_defined(0x2020u,4)&&!outer.data_is_defined(0x2020u,4),"nested invalidation forked mask");++controls;
    check(nested.withhold_defined_data(0x3001u,2)&&nested.data_is_defined(0x3000u,1)&&!nested.data_is_defined(0x3000u,4)&&nested.data_is_defined(0x3003u,5),"nested local mask differs");++controls;
    refuses([&]{nested.withhold_defined_data(0x1000u,4);});
    check(!bank.reads&&!bank.writes&&bank.data==bank_before,"invalidation touched retained bank");++controls;
    std::cout<<controls<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
'''


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    include = scratch / "include"
    shutil.copytree(ROOT / "include", include)
    shutil.copytree(args.stage / "staged/include", include, dirs_exist_ok=True)
    source = scratch / "lifetime.cpp"
    source.write_text(SOURCE)
    private = include / "halo2/native_tu15_contact_candidate_private_data.hpp"
    original = private.read_text()
    mutations = {
        "wrapping_extent": (
            'if(n>0x100000000ull-std::uint64_t(p))throw std::runtime_error("Native private DATA wraps");',
            '(void)n;'),
        "keep_stale_definition": (
            "std::fill_n(r.defined.begin()+(p-r.extent.first),n,false)", "(void)n"),
        "allow_retained_invalidation": (
            'throw std::runtime_error("Native private lifetime cannot invalidate retained DATA")', "return true"),
        "copyable_definition_mask": (
            "    NativeTu15ContactCandidatePrivateData(const NativeTu15ContactCandidatePrivateData&)=delete;\n"
            "    NativeTu15ContactCandidatePrivateData&operator=(const NativeTu15ContactCandidatePrivateData&)=delete;\n"
            "    NativeTu15ContactCandidatePrivateData(NativeTu15ContactCandidatePrivateData&&)=delete;\n"
            "    NativeTu15ContactCandidatePrivateData&operator=(NativeTu15ContactCandidatePrivateData&&)=delete;\n", ""),
    }
    report = {"passed": False, "owner": str(scratch), "results": [],
              "source_sha256": hashlib.sha256(SOURCE.encode()).hexdigest(),
              "header_sha256": hashlib.sha256(original.encode()).hexdigest(),
              "scope": "Typed metadata invalidation/nested ownership only; no raw private byte or numerical FP domain extension."}
    try:
        for compiler in ("g++", "clang++"):
            for name in ("baseline", *mutations):
                private.write_text(original)
                if name != "baseline":
                    before, after = mutations[name]
                    assert original.count(before) == 1, name
                    private.write_text(original.replace(before, after))
                executable = scratch / (compiler.replace("+", "p") + "-" + name)
                command = [compiler, "-std=c++20", "-O0", "-I" + str(include),
                           str(source), "-o", str(executable)]
                compile = subprocess.run(command, cwd=scratch, text=True, capture_output=True)
                run = subprocess.run([str(executable)], cwd=scratch, text=True, capture_output=True) if compile.returncode == 0 else None
                passed = (compile.returncode == 0 and run is not None and run.returncode == 0) if name == "baseline" else (compile.returncode != 0 or run is not None and run.returncode != 0)
                result = {"compiler": compiler, "variant": name, "passed": passed,
                          "compile_returncode": compile.returncode,
                          "run_returncode": run.returncode if run else None,
                          "diagnostics": (compile.stdout + compile.stderr + (run.stdout + run.stderr if run else ""))[-5000:]}
                report["results"].append(result)
                assert passed, result
        report["passed"] = True
    finally:
        private.write_text(original)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"passed": report["passed"], "report": str(args.report)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
