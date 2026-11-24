// Native TU1.5 cluster pairs (118120) against the unmodified
// translated body of the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author objects with no, negative and
// several references, pairs of clusters (past 31 too) and bitmaps with and
// without their bits; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/native_tu15_cluster_pairs.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_cluster_pairs_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,structure=0x10030000u,indices=0x10040000u,
    pairs=0x10050000u,bitmaps=0x10060000u,seen=0x10070000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4ecc18u,structure);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<4;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x200u*k);}
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x200u*k;
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x1dau,pick(8),2);m.put(object+0x1dcu,pick(5)?pick(6):pick(2)?0u:0x8000u,2);
    }
    for(std::uint32_t at=structure;at<structure+0x240u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(structure+0x220u,structure+0x100u);m.put(structure+0x10cu,indices);m.put(structure+0x60u,pairs);
    for(std::uint32_t k=0;k<16;++k)m.put(indices+2u*k,pick(8),2);
    for(std::uint32_t k=0;k<8;++k){for(std::uint32_t at=pairs+36u*k;at<pairs+36u*k+36u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(pairs+36u*k,pick(64),2);m.put(pairs+36u*k+2u,pick(64),2);}
    for(std::uint32_t at=bitmaps;at<bitmaps+8u;at+=4)m.put(at,std::uint32_t(rng())&std::uint32_t(rng())&std::uint32_t(rng())&(pick(3)?0u:~0u));
    for(std::uint32_t at=seen;at<seen+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(stack+8u,bitmaps);m.put(stack+12u,seen+pick(8));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,structure,indices,pairs,bitmaps,seen};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_118120(v);});}
void reference(WorldState &s){original::run_118120(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CLUSTER_PAIRS");
    std::mt19937 rng(0x118120u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
