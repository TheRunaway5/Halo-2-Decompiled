#pragma once
#include "halo2/native_tu15_constraint_solver.hpp"
#include <bit>
#include <optional>
#include <stdexcept>
namespace halo2::native_tu15_solver_detail {
using Vector=NativeTu15PhysicsVector;
class Access {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Access(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint8_t byte(std::uint32_t p)const{std::array<std::byte,1>b{};memory_.read(p,b);return std::to_integer<std::uint8_t>(b[0]);}
    std::int32_t opcode(std::uint32_t p)const{return std::bit_cast<std::int8_t>(byte(p));}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
    Vector vector(std::uint32_t p)const{std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector v{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)v[n]|=std::to_integer<unsigned>(b[n*4u+k])<<(k*8u);return v;}
    void write(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>bytes){SourcePhysicsContactRecordStore s{pc,p,{},{}};if(observe_){s.before.resize(bytes.size());memory_.read(p,s.before);s.written.assign(bytes.begin(),bytes.end());}memory_.write(p,bytes);if(observe_)observe_(s);}
    void byte(std::uint32_t pc,std::uint32_t p,std::uint8_t value){const std::array bytes{std::byte(value)};write(pc,p,bytes);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));write(pc,p,bytes);}
    void vector(std::uint32_t pc,std::uint32_t p,const Vector&value){std::array<std::byte,16>bytes{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)bytes[n*4u+k]=std::byte(value[n]>>(k*8u));write(pc,p,bytes);}
};
// Retained actual schema/header state, with semantic identities rather than
// virtual registers. Child math updates actual row/schema/result cursors.
struct PrimaryState {
    std::uint32_t configuration{},schema{},jacobian{},first_accumulator{},
        second_accumulator{},result{};
    // Only source fast fallthroughs consume an already-read opcode. Ordinary
    // dispatch returns leave this empty, preserving the next source reread.
    std::optional<std::int32_t> next_opcode;
};
// Complete source row families2..13. Each family keeps its actual private
// x87 impulses live until result accumulation, including the FST/SIMD split.
void solve_primary_rows(Access&,PrimaryState&,unsigned schema_type);
} // namespace halo2::native_tu15_solver_detail
