// Native TU1.5 cluster object query (BD460) against the unmodified
// translated body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BD460 has no children. Fixtures author the two cluster partitions (head
// tables and entry pools with ordinary and unusual datum sizes), acyclic
// entry lists of object owners (NONE heads, NONE owners, shared owners
// across clusters and partitions), the query stamp and mark table (some
// owners already marked), the arguments (zero and partial flags, cluster
// counts from negative up, cluster lists with negative and past-table
// indices, a capacity from negative to large, the output), and inject a
// fault by removing one region.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_cluster_objects.hpp"
using namespace frame_misc_harness;
namespace query=halo2::native_cluster_objects_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t heads_a=0x10000000u,heads_b=0x10010000u,pool_a=0x10100000u,pool_b=0x10110000u,
    data_a=0x10200000u,data_b=0x10300000u,lists=0x10400000u,output=0x10500000u,stack=0x20010000u;
constexpr std::uint32_t cluster_total=24u,owner_total=96u,entry_total=160u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    m.put(0x4eabb0u,heads_a);m.put(0x4eabb4u,pool_a);m.put(0x4eabb8u,std::uint32_t(rng()));
    m.put(0x4eaba4u,heads_b);m.put(0x4eaba8u,pool_b);m.put(0x4eabacu,std::uint32_t(rng()));
    const std::uint32_t stamp=pick(5)==0?0xffffffffu:std::uint32_t(rng());
    m.put(0x4eabc8u,std::uint32_t(rng()));m.put(0x4eabccu,stamp);
    // The mark table by owner index; some owners carry the next stamp.
    for(std::uint32_t k=0;k<owner_total+8u;++k)m.put(0x4eabd0u+k*4u,pick(7)==0?stamp+1u:std::uint32_t(rng()));
    // Two pools: DataArray headers (datum size, data) and their entries.
    const auto author_pool=[&](std::uint32_t pool,std::uint32_t data) {
        for(std::uint32_t at=pool;at<pool+0x4cu;at+=4)m.put(at,std::uint32_t(rng()));
        const std::uint32_t size=shape==6?16u+pick(3)*4u:12u;
        m.put(pool+0x24u,size);m.put(pool+0x44u,data);
        for(std::uint32_t k=0;k<entry_total;++k)for(std::uint32_t at=0;at<size;at+=4)m.put(data+k*size+at,std::uint32_t(rng()));
        // Acyclic lists: each entry links to a later one, or ends.
        for(std::uint32_t k=0;k<entry_total;++k) {
            const auto e=data+k*size;
            const auto salt=std::uint32_t(rng()%0x7fff)<<16;
            std::uint32_t owner=salt|pick(owner_total);
            if(pick(13)==0)owner=0xffffffffu;
            if(shape==12 && pick(9)==0)owner=salt|(owner_total+8u+pick(4));   // past the mark table
            m.put(e+4u,owner);
            const auto next=k+1u+pick(5);
            m.put(e+8u,next<entry_total && pick(6)?(std::uint32_t(rng()%0x7fff)<<16)|next:0xffffffffu);
        }
    };
    author_pool(pool_a,data_a);author_pool(pool_b,data_b);
    for(std::uint32_t k=0;k<cluster_total+4u;++k) {
        m.put(heads_a+k*4u,pick(5)==0?0xffffffffu:(std::uint32_t(rng()%0x7fff)<<16)|pick(entry_total));
        m.put(heads_b+k*4u,pick(4)==0?0xffffffffu:(std::uint32_t(rng()%0x7fff)<<16)|pick(entry_total));
    }
    m.put(heads_a-4u,0xffffffffu);m.put(heads_b-4u,(std::uint32_t(rng()%0x7fff)<<16)|pick(entry_total));
    // The cluster list.
    const auto list=lists;
    for(std::uint32_t k=0;k<64u;++k) {
        std::uint32_t cluster=pick(cluster_total);
        if(shape==7 && pick(5)==0)cluster=0xffffu;                 // -1
        if(shape==12 && pick(7)==0)cluster=cluster_total+pick(4);  // past the authored heads
        m.put(list+k*2u,cluster,2);
    }
    for(std::uint32_t at=output-0x40u;at<output+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x3000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1ae19au);
    std::uint32_t flags=pick(4);
    if(shape==5)flags=std::uint32_t(rng());
    m.put(stack+0x4u,flags);
    std::uint32_t count=1u+pick(12);
    if(shape==9)count=pick(2)?0u:(std::uint32_t(rng())<<16)|(0x8000u+pick(8));   // none, or negative
    if(shape==10)count=(std::uint32_t(rng())<<16)|(1u+pick(40));                // upper half ignored
    m.put(stack+0x8u,count);
    m.put(stack+0xcu,list);
    std::uint32_t capacity=pick(3)==0?pick(4):8u+pick(64);
    if(shape==11)capacity=(std::uint32_t(rng())<<16)|(0x8000u+pick(4));
    m.put(stack+0x10u,capacity);
    // Shape 13: the output overlaps the collideable pool pointer, so an
    // append rewrites it (the original reloads the pool after each append).
    m.put(stack+0x14u,shape==13?0x4eabb4u-4u*pick(3):output);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{heads_a,heads_b,pool_a,pool_b,data_a,data_b,lists,output,0x4eab00u};
        m.drop(regions[pick(9)]);
    }
    return c;
}
bool native(WorldState &s){return query::bridge(s,[](auto &v){original::native_frame_misc_reference_bd460(v);});}
void reference(WorldState &s){original::run_bd460(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CLUSTER_OBJECTS");
    std::mt19937 rng(0xbd460u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,query::coverage().load(),unsigned(query::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
