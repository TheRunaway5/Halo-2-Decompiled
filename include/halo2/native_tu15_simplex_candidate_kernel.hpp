#pragma once
#include "halo2/native_tu15_simplex_direction_kernel.hpp"
#include <cstring>

namespace halo2 {
// Post-support continuation inside30E930. Production callers resume from their
// actual support owner; direct-stage tests declare candidate DATA as an input
// and make no support-call/whole-parent claim. Comparisons are computed here.
enum class NativeTu15SimplexCandidateSide { first_early, second, first_late };
enum class NativeTu15SimplexCandidatePhase {
    count, first, direction, candidate, third, second,
    classify_count, score, publish_count, complete
};
struct NativeTu15SimplexCandidate {
    using Vector=native_simplex_direction_numeric::Vector;
    NativeTu15SimplexCandidateSide side{};
    std::uint32_t work{},count_address{},direction_address{},count{},candidate_address{};
    unsigned comparison{};
    Vector first{},direction{},candidate{},difference{},products{},xy{},sum{};
    bool accepted{};
};
inline std::uint32_t native_tu15_simplex_scalar_bits(__m128 value){
    std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;
}
// compare_score owns native FABS/FMUL/FCOMP and the lazy virtual scalar reads.
// The initial score requires ordered greater-than. Extra vertex scores reject
// ordered less-than; the original AH parity test also accepts unordered.
template<class Access>
bool native_tu15_test_simplex_candidate(Access&memory,NativeTu15SimplexCandidate&s){
    using P=NativeTu15SimplexCandidatePhase;
    using Side=NativeTu15SimplexCandidateSide;
    namespace N=native_simplex_direction_numeric;
    const bool second=s.side==Side::second;
    s.count=memory.word_read(P::count,s,s.count_address);
    s.first=memory.vector_read(P::first,s,s.work+(second?0xe0u:0x20u));
    s.direction=memory.vector_read(P::direction,s,s.direction_address);
    s.candidate_address=s.work+((s.count+(second?14u:2u))<<4u);
    s.candidate=memory.vector_read(P::candidate,s,s.candidate_address);
    const auto project=[&](unsigned index,__m128 point){
        s.comparison=index;
        s.difference=N::subtract(s.candidate,point);
        s.products=N::multiply(s.difference,s.direction);
        s.xy=N::add_low(N::shuffle<0x55>(s.products),s.products);
        s.sum=N::add_low(N::shuffle<0xaa>(s.products),s.xy);
        memory.score_write(s,native_tu15_simplex_scalar_bits(s.sum));
        return memory.compare_score(s);
    };
    const auto reject=[&]{s.accepted=false;memory.complete(s);return false;};
    if(project(0u,s.first)&0x4100u)return reject();
    memory.prepare(P::classify_count,s);
    if(s.count==3u){
        const auto point=memory.vector_read(P::third,s,s.work+(second?0x100u:0x40u));
        const auto relation=project(1u,point);
        if(bool(relation&0x100u)!=bool(relation&0x400u))return reject();
    }
    if(s.count==2u||s.count==3u){
        const auto point=memory.vector_read(P::second,s,s.work+(second?0xf0u:0x30u));
        const auto relation=project(2u,point);
        if(bool(relation&0x100u)!=bool(relation&0x400u))return reject();
    }
    s.accepted=true;
    if(!second)memory.word_write(P::publish_count,s,s.work,s.count+1u);
    memory.complete(s);return true;
}
} // namespace halo2
