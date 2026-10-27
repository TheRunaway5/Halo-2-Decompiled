#!/usr/bin/env python3
"""Compile the complete1DB800 body and compare original/MCC named inputs.

Run through tools/run_test.py. This is a bounded native compiler probe, with
declared component bytes; it does not claim live lift or speaker acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("map", type=Path)
    parser.add_argument("--report-output", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    if not scratch.is_absolute() or not scratch.is_dir():
        raise RuntimeError("Run through tools/run_test.py")
    source = ROOT / "src/runtime/source_map_world_support_tu15_generated.cpp"
    text = source.read_text()
    begin = text.index("static void run_1db800(WorldState &s) {")
    end = text.index("\nstatic void ", begin + 1)
    body = text[begin:end]
    # Preserve each selected immediate exactly from the production mapping.
    operands = sorted(set(re.findall(r"selected\((0x[0-9a-f]+u),s.image\)", body)))
    cases = []
    for operand in operands:
        match = re.search(r"case " + operand + r":return (0x[0-9a-f]+u);", text)
        if not match:
            raise RuntimeError("Property operand has no compiled selected-image owner")
        cases.append(f"case {operand}:return {match[1]};")
    probe = scratch / "phantom-namespace.cpp"
    program = r'''
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
struct Memory final:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::byte> bytes;
 void read(std::uint32_t p,std::span<std::byte> out)const override {
  for(auto &b:out){const auto at=bytes.find(p);if(at==bytes.end())throw std::runtime_error("Missing declared byte at "+std::to_string(p));b=at->second;++p;}
 }
 void write(std::uint32_t p,std::span<const std::byte> in)override {
  for(auto b:in)bytes[p++]=b;
 }
 void put(std::uint32_t p,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)bytes[p+i]=std::byte(v>>(8*i));
 }
 std::uint32_t get(std::uint32_t p,unsigned n=4)const {
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(bytes.at(p+i))<<(8*i);return v;
 }
};
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage) {
 switch(p){@CASES@default:throw std::runtime_error("Missing production operand");}
}
@BODY@
struct Result {unsigned recognized,value,active,stack;bool operator==(const Result&)const=default;};
Result run(Memory &m,unsigned key,unsigned power,unsigned speed,unsigned flags) {
 constexpr unsigned component=0x102000,stack=0x200000,out=0x100000;
 m.put(component+4,flags);m.put(component+0x1a,power,1);m.put(component+0x1b,speed,1);
 m.put(stack,0x11223344);m.put(stack+4,out+4);m.put(out,0xdeadbeef);m.put(out+4,0xdeadbeef);
 SourcePhysicsWorldRuntimeAbi a{};a.gp[1]=key;a.gp[2]=out;a.gp[4]=stack;a.gp[6]=component;a.eflags=2;
 const SourcePhysicsContactRuntimeServices services{};const SourcePhysicsContactRecordObserver observer{};
 WorldState s{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};s.load(a);
 run_1db800(s);a=s.save();
 return {a.gp[0]&255,m.get(out),m.get(out+4,1),a.gp[4]};
}
int main(int argc,char **argv)try {
 if(argc!=2)throw std::runtime_error("Supply actual prepared MCC map");
 std::ifstream file(argv[1],std::ios::binary);
 std::vector<char> data((std::istreambuf_iterator<char>(file)),{});
 const auto word=[&](unsigned p){unsigned v{};for(unsigned i=0;i<4;++i)v|=unsigned(static_cast<unsigned char>(data.at(p+i)))<<(8*i);return v;};
 if(word(4)!=13 || word(8)!=data.size())throw std::runtime_error("Expected prepared native MCC metadata");
 const auto name=[&](unsigned id){const auto index=id&65535;
  if(index>=word(48))throw std::runtime_error("Actual Cache name index escapes its owner");
  const auto offset=word(word(60)+index*4);if(offset>=word(56))throw std::runtime_error("Actual Cache name escapes its owner");
  const auto p=word(52)+offset;return std::string(data.data()+p);
 };
 const auto find=[&](const char *target){for(unsigned i=0;i<word(48);++i) {
  const auto candidate=name(i);if(candidate==target)return unsigned(candidate.size())<<24|i;
 }throw std::runtime_error("Actual named phantom input absent");};
 const std::array original{0x0d000691u,0x0d000692u,0x0a000693u};
 const std::array authored{find("phantom_speed"),find("phantom_power"),find("phantom_on")};
 Memory original_bank,mcc_bank;
 // Exact scalar in the authenticated complete body:1/255 stored as m32.
 original_bank.put(0x46a0c4,0x3b808081);mcc_bank.put(0x46a0c4,0x3b808081);
 original_bank.put(0x46a0d0,0x3f800000);mcc_bank.put(0x46a0d0,0x3f800000);
 SourceEffectCompiledSymbolsTu15 binding(mcc_bank,true,name);
 FpScope fp({0x27f,0x1f80});unsigned cases{},mismatches{},return_errors{},namespace_controls{};
 for(unsigned flags:{0u,0x2000u})for(unsigned byte=0;byte<256;++byte)for(unsigned which=0;which<3;++which) {
  const auto a=run(original_bank,original[which],byte,255-byte,flags);
  const auto b=run(mcc_bank,authored[which],byte,255-byte,flags);
  ++cases;mismatches+=a!=b;return_errors+=a.stack!=0x200008 || b.stack!=0x200008;
 }
 for(unsigned key:original) {
  const auto result=run(mcc_bank,key,255,255,0x2000);
  namespace_controls+=result.recognized!=0 || result.value!=0xdeadbeef || result.active!=0xef;
 }
 for(unsigned key:authored) {
  const auto result=run(original_bank,key,255,255,0x2000);
  namespace_controls+=result.recognized!=0 || result.value!=0xdeadbeef || result.active!=0xef;
 }
 std::cout<<"{\"cases\":"<<cases<<",\"mismatches\":"<<mismatches
  <<",\"return_errors\":"<<return_errors<<",\"namespace_control_errors\":"<<namespace_controls
  <<",\"actual_MCC_keys\":["<<authored[0]<<','<<authored[1]<<','<<authored[2]<<"]}\n";
 return mismatches||return_errors||namespace_controls?1:0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 2;}
'''
    probe.write_text(program.replace("@CASES@", "".join(cases)).replace("@BODY@", body))
    binary = scratch / "phantom-namespace"
    compile_command = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-pthread",
                       "-I" + str(ROOT / "include"), "-I" + str(ROOT / "src"),
                       str(probe), str(ROOT / "src/rendering/effects/source_effect_compiled_symbols_tu15.cpp"),
                       "-o", str(binary)]
    subprocess.run(compile_command, check=True)
    result = subprocess.run([str(binary), str(args.map.resolve())], capture_output=True, text=True)
    report = {"passed": result.returncode == 0, "native_compiled_body": "1DB800",
              "production_source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
              "extracted_body_sha256": hashlib.sha256(body.encode()).hexdigest(),
              "compiled_symbol_source_sha256": hashlib.sha256((ROOT / "src/rendering/effects/source_effect_compiled_symbols_tu15.cpp").read_bytes()).hexdigest(),
              "compiled_symbol_table_sha256": hashlib.sha256((ROOT / "src/rendering/effects/source_object_function_symbols_tu15.inc").read_bytes()).hexdigest(),
              "actual_map": str(args.map.resolve()),
              "actual_map_sha256": hashlib.sha256(args.map.read_bytes()).hexdigest(),
              "scope": "Complete compiled property body with declared component bytes and actual MCC name table; live lift/audio/network acceptance remains separate.",
              "stdout": result.stdout, "stderr": result.stderr, "exit_code": result.returncode}
    if result.stdout:
        report["comparison"] = json.loads(result.stdout)
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
