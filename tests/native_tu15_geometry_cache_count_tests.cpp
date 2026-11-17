// Native TU1.5 geometry cache entry count (1306B0) against the unmodified
// translated body of the geometry cache frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Fixtures author the cache state ([4F2D60], enable byte +0x29) and the
// [4F2D5C] data array: counts from zero to past a bit-vector word, negative
// and huge counts, dense/sparse/upper-bit-garbage bit vectors, zero and
// wrapping strides, a record base that makes a datum address zero, record
// bytes zero/nonzero, both direction flags, and fault injection (the header,
// bit vector, a record page or the stack removed).
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_geometry_cache_count.hpp"
using namespace frame_misc_harness;
namespace count=halo2::native_geometry_cache_count_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t state=0x10000000u,array=0x10100000u,bits=0x10200000u,records=0x10300000u,stack=0x20010000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    m.put(0x4f2d60u,state);m.put(0x4f2d5cu,array);
    m.put(state+0x29u,shape==0?0u:(1u+pick(255)),1);
    for(std::uint32_t at=state;at<state+0x40u;at+=4)if(at!=state+0x28u)m.put(at,std::uint32_t(rng()));
    std::uint32_t count=pick(200);
    if(shape==2)count=pick(40);
    if(shape==3)count=0;
    if(shape==4)count=std::uint32_t(-1-int(pick(50)));
    if(shape==5)count=0x80000000u|pick(100);
    if(shape==6)count=31u+pick(3);
    m.put(array+0x38u,count);
    const std::uint32_t authored=std::min<std::uint32_t>(std::int32_t(count)<0?64u:count,300u);
    const std::uint32_t words=(authored+31u)/32u+2u;
    for(std::uint32_t w=0;w<words;++w) {
        std::uint32_t v=std::uint32_t(rng());
        if(shape==7)v&=std::uint32_t(rng())&std::uint32_t(rng());
        if(shape==8)v=pick(3)==0?0u:0xffffffffu;
        if(shape==9)v=0;
        m.put(bits+w*4u,v);
    }
    m.put(array+0x48u,bits);
    std::uint32_t stride=0x10u+pick(0x40u);
    if(shape==10)stride=0;
    m.put(array+0x24u,stride);
    std::uint32_t base=records;
    if(shape==11 && authored) {
        // Choose a base so one allocated datum's address is exactly zero.
        const auto victim=pick(authored);base=0u-stride*victim;
        if(victim==0)base=0;
    }
    m.put(array+0x44u,base);
    for(std::uint32_t at=array;at<array+0x50u;at+=4)if(at!=array+0x38u && at!=array+0x48u && at!=array+0x24u && at!=array+0x44u)
        m.put(at,std::uint32_t(rng()));
    if(base==records || shape==10) {
        for(std::uint32_t k=0;k<std::max(authored,1u);++k) {
            const auto record=records+k*stride;
            for(std::uint32_t b=0;b<std::max(stride,4u);b+=4)m.put(record+b,std::uint32_t(rng()));
            m.put(record+2u,pick(3)==0?(1u+pick(255)):0u,1);
        }
    } else if(shape==11) {
        for(std::uint32_t k=0;k<authored;++k) {
            const auto record=base+k*stride;
            if(record>=0x1000u)m.put(record+2u,pick(2)?0u:1u,1);
        }
    }
    for(std::uint32_t at=stack-0x40u;at<stack+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x130740u);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{array,bits,records,stack-0x40u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return count::bridge(s,[](auto &v){original::native_frame_misc_reference_1306b0(v);});
}
void reference(WorldState &s){original::run_1306b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_GEOMETRY_CACHE_COUNT");
    std::mt19937 rng(0x1306b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,count::coverage().load(),unsigned(count::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
