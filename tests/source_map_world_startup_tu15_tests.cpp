#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;using J=nlohmann::json;
namespace {
std::vector<std::byte> bytes(const std::string&s){std::vector<std::byte>b;for(unsigned n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){static const char*d="0123456789abcdef";std::string s;for(auto x:b){auto v=std::to_integer<unsigned>(x);s+=d[v>>4];s+=d[v&15];}return s;}
J run(const J&c){SourceRuntimeMemory m;for(const auto&b:c.at("regions"))m.mount(b.at("address"),bytes(b.at("bytes")),SourceRuntimeMemory::Access::ReadWrite,"Original startup fixture");const SourcePhysicsFactoryFloatingPoint fp{std::uint16_t(c.value("cw",0x37fu)),c.value("mxcsr",0x1f80u)};SourcePhysicsWorldRuntimeAbi a{};a.gp=c.at("gp").get<decltype(a.gp)>();a.xmm=c.at("xmm").get<decltype(a.xmm)>();a.eflags=c.at("eflags");J trace=J::array(),writes=J::array();SourcePhysicsWorldRuntimeServices services;
// Estimate bits are explicit fixture inputs. Never default an absent CPU owner.
if(c.contains("rsqrt"))services.reciprocal_sqrt=[&](std::uint32_t bits){return c.at("rsqrt").at(std::to_string(bits)).get<std::uint32_t>();};services.instruction=[&](auto pc){trace.push_back(pc);};const SourcePhysicsContactRecordObserver observe=[&](const auto&w){writes.push_back(J::array({w.address,hex(w.written)}));};services.required_owner=[&](auto&memory,const auto&call,auto&abi,const auto&o){if(!source_physics_live_support_tu15_owns_entry(call.target))throw std::runtime_error("Missing real CRC owner");source_run_physics_live_support_tu15(memory,call.target,abi,fp,services,o);};source_run_map_world_support_tu15(m,c.at("entry"),a,fp,services,observe);J banks=J::array();for(const auto&b:c.at("regions"))banks.push_back(hex(m.view(b.at("address"),b.at("bytes").get<std::string>().size()/2)));return {{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"instructions",trace},{"writes",writes},{"banks",banks}};}
}
int main(int argc,char**argv)try{std::ifstream file;std::istream*in=&std::cin;if(argc>=2){file.open(argv[1]);if(!file)return 77;in=&file;}std::string line;unsigned count=0;while(std::getline(*in,line)){auto c=J::parse(line);auto got=run(argc>=2?c.at("input"):c);if(argc>=2){if(got!=c.at("original"))throw std::runtime_error("Original startup mismatch "+std::to_string(count));}else std::cout<<got.dump()<<'\n';++count;}if(argc>=2){if(count!=(argc==3?std::stoul(argv[2]):80u))throw std::runtime_error("Incomplete original startup fixtures");std::cout<<"PASS "<<count<<" original world routine cases, complete callees, full banks/GP/XMM/flags/instructions/stores\n";}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
