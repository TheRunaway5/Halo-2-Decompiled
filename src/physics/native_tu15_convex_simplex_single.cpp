#include "halo2/native_tu15_convex_simplex_single.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint64_t end(std::uint32_t p,std::size_t n) {
    if(n>0x100000000ull-std::uint64_t(p))
        throw std::runtime_error("Native simplex DATA range wraps");
    return std::uint64_t(p)+n;
}
}
void NativeTu15ConvexSimplexMemory::withhold(std::uint32_t p,std::size_t n) {
    end(p,n);if(n)regions_.push_back({p,std::vector<bool>(n,false)});
}
void NativeTu15ConvexSimplexMemory::check(std::uint32_t p,std::size_t n)const {
    const auto finish=end(p,n);
    for(const auto&r:regions_) {
        const auto first=std::max<std::uint64_t>(p,r.address);
        const auto last=std::min(finish,std::uint64_t(r.address)+r.defined.size());
        for(auto i=first;i<last;++i)if(!r.defined[i-r.address]) {
            ++refused_reads_;
            throw std::runtime_error("Native simplex requires its unprovided DATA writer");
        }
    }
}
void NativeTu15ConvexSimplexMemory::read(std::uint32_t p,std::span<std::byte>b)const {
    check(p,b.size());storage_.read(p,b);
}
void NativeTu15ConvexSimplexMemory::read_vector(std::uint32_t p,std::span<std::byte,16>b)const {
    check(p,b.size());storage_.read_vector(p,b);
}
void NativeTu15ConvexSimplexMemory::write(std::uint32_t p,std::span<const std::byte>b) {
    const auto finish=end(p,b.size());storage_.write(p,b);
    for(auto&r:regions_) {
        const auto first=std::max<std::uint64_t>(p,r.address);
        const auto last=std::min(finish,std::uint64_t(r.address)+r.defined.size());
        for(auto i=first;i<last;++i)r.defined[i-r.address]=true;
    }
}

std::uint32_t native_tu15_solve_single_vertex_convex_simplex(
    NativeTu15ConvexSimplexMemory&memory,NativeTu15ConvexSimplexSingleInput input,
    NativeTu15ConvexSimplexSingleScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const NativeTu15ConvexSimplexSingleServices&services,
    const std::function<std::uint64_t()>&clock,NativeTu15ConvexSimplexSingleStats*activity) {
    NativeTu15ConvexSimplexSingleStats local;auto&stats=activity?*activity:local;++stats.attempts;
    try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native single simplex requires empty x87 DATA");
        const std::array<std::pair<std::uint32_t,unsigned>,5> owned{{
            {input.work,0x140u},{scratch.simplex,132u},{scratch.differences,64u},
            {scratch.output_plane,40u},{scratch.hull.context,84u}}};
        for(unsigned i=0;i<owned.size();++i) {
            const auto [p,n]=owned[i];if(p&15u)throw std::runtime_error("Native simplex requires aligned owned DATA");
            end(p,n);
            for(unsigned j=0;j<i;++j)if(std::uint64_t(p)<end(owned[j].first,owned[j].second)&&
                std::uint64_t(owned[j].first)<end(p,n))
                throw std::runtime_error("Native simplex owned DATA lifetimes overlap");
        }
        for(auto offset:{0x30u,0x40u,0xf0u,0x100u})memory.withhold(input.work+offset,16u);
        memory.withhold(scratch.simplex,132u);memory.withhold(scratch.differences,64u);
        memory.withhold(scratch.output_plane,40u);memory.withhold(scratch.hull.context,84u);
        const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);
        {
            Environment phase(fp,depth);
            const auto first=m.vector(input.work+0x20u),opposite=m.vector(input.work+0xa0u);
            const auto second_count=m.word(input.work+4u);
            auto difference=first;
            asm volatile("subps %1,%0":"+x"(difference):"x"(opposite):"memory");
            m.vector(0x30de8fu,scratch.differences,difference);
            const auto second=m.vector(input.work+0xe0u);
            m.word(0x30de9eu,scratch.simplex+0x80u,4u);
            m.vector(0x30dea9u,scratch.simplex,first);
            m.vector(0x30deaeu,scratch.simplex+0x40u,second);
            if(std::bit_cast<std::int32_t>(second_count)>=3) {
                ++stats.input_domain_refusals;
                throw std::runtime_error("Native simplex requires its second-count continuation");
            }
            const auto first_count=m.word(input.work);
            if(std::bit_cast<std::int32_t>(first_count)>=3) {
                ++stats.input_domain_refusals;
                throw std::runtime_error("Native simplex requires its first-count continuation");
            }
            m.word(0x30dfe7u,scratch.simplex+0x80u,1u);
        }
        NativeTu15SolverHullSupportServices hull_services;
        hull_services.support=[&](auto shape,auto owner,auto direction,auto output,auto&current){
            if(!services.support)throw std::runtime_error("Native simplex requires its native support owner");
            services.support(memory,shape,owner,direction,output,current);
        };
        const auto result=native_tu15_query_solver_hull(memory,
            {input.first,input.second,input.relative_transform,scratch.simplex,
                scratch.differences,scratch.output_plane},scratch.hull,fp,depth,
            hull_services,clock,{},&stats.hull);
        const auto first=m.vector(scratch.simplex);
        const auto count=m.word(scratch.simplex+0x80u);
        m.vector(0x30e024u,input.work+0x20u,first);
        const auto second=m.vector(scratch.simplex+0x40u);
        m.vector(0x30e03fu,input.work+0xe0u,second);
        // Source copies four never-defined local rows here. Their bounded
        // lifetime proof permits no value, store or read capability for them.
        if(count!=1u) {
            ++stats.result_domain_refusals;
            throw std::runtime_error("Native simplex requires its non-single result continuation");
        }
        m.word(0x30e070u,input.work+0x120u,0x3f800000u);
        m.word(0x30e07cu,input.work+0x124u,0u);m.word(0x30e082u,input.work+0x128u,0u);
        m.word(0x30e088u,input.work+0x12cu,0u);
        m.word(0x30e08eu,input.work+4u,1u);m.word(0x30e094u,input.work,1u);
        ++stats.completed;return result;
#else
        (void)memory;(void)input;(void)scratch;(void)fp;(void)depth;(void)services;(void)clock;
        throw std::runtime_error("Exact native single simplex backend unavailable");
#endif
    }catch(...) {++stats.refusals;throw;}
}
} // namespace halo2
