#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <filesystem>
#include <fstream>
#include <map>
#include <tuple>
#include <nlohmann/json.hpp>

namespace native_allocator_fixture {
using namespace halo2;using J=nlohmann::json;
inline void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
inline std::vector<std::byte> unhex(const std::string &s){std::vector<std::byte>b;for(unsigned n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
inline std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
inline std::vector<std::byte> file(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary);check(bool(f),"Missing owned allocator ABI bank "+p.string());std::vector<char>raw((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b(raw.size());for(unsigned n=0;n<raw.size();++n)b[n]=std::byte(std::uint8_t(raw[n]));return b;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;mutable J reads=J::array();bool logging{};
    mutable unsigned operations{};unsigned fail_at{};mutable std::vector<std::tuple<char,std::uint32_t,std::size_t>> accesses;
    auto at(std::uint32_t p,std::size_t size)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked allocator ABI operand");--i;check(std::uint64_t(p-i->first)+size<=i->second.size(),"Unbacked allocator ABI extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{accesses.emplace_back('r',p,out.size());if(++operations==fail_at)throw std::runtime_error("injected allocator read fault");if(logging)reads.push_back(J::array({p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>in)override{accesses.emplace_back('w',p,in.size());if(++operations==fail_at)throw std::runtime_error("injected allocator write fault");const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);}
};
inline Memory load(const J &c,const std::filesystem::path &directory){Memory m;for(const auto &bank:c.at("banks"))m.banks.emplace(bank.at("address"),file(directory/bank.at("before_file").get<std::string>()));return m;}
inline SourcePhysicsWorldRuntimeAbi abi(const J &c){const auto &b=c.at("before_abi");SourcePhysicsWorldRuntimeAbi a{};a.gp=b.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=b.at("eflags");const auto x=unhex(b.at("xmm"));std::memcpy(a.xmm.data(),x.data(),128);a.x87_status=b.at("fp")[1];a.floating_point=SourcePhysicsFactoryFloatingPoint{b.at("fp")[0],b.at("fp")[2]};return a;}
inline J packet(const physics_world_runtime_detail::WorldState &s,std::uint32_t pc){const auto a=s.save();return J{{"entry",pc},{"abi",J{{"gp",a.gp},{"eflags",a.eflags&0xcd5u},{"xmm",hex(std::as_bytes(std::span(a.xmm)))},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})}}}};}
inline bool same_abi(const SourcePhysicsWorldRuntimeAbi &x,const SourcePhysicsWorldRuntimeAbi &y){
    if(x.gp!=y.gp||x.eflags!=y.eflags||x.xmm!=y.xmm||x.x87_depth!=y.x87_depth||x.x87_status!=y.x87_status||x.floating_point->x87_control_word!=y.floating_point->x87_control_word||x.floating_point->mxcsr!=y.floating_point->mxcsr)return false;
    for(unsigned n=0;n<x.x87_depth;++n)if(std::memcmp(&x.x87[n],&y.x87[n],10))return false;return true;
}
inline void cursor(SourcePhysicsWorldRuntimeServices &s,std::uint32_t &pc){static const std::array<std::uint32_t,3>sites{0x38846eu,0x21f590u,0x33b595u};auto &c=s.instruction_cursor;c.position=&pc;c.callback_type=&s.instruction.target_type();c.executor=std::this_thread::get_id();c.literal_service_sites=true;c.observation_only_stores=true;c.service_sites=sites;}
} // namespace native_allocator_fixture
