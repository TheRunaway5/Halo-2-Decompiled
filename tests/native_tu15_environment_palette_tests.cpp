// Native TU1.5 campaign environment palette lookup (133A70) against the
// unmodified translated body of the campaign environment unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Fixtures author the palette ([4ECC20]: count, 8-byte entries, the +0x214
// default records), the [474C24] selector, cluster records with primary and
// secondary bytes (0xFF, negative, in range, beyond the count), entry names
// (-1 and tag handles whose flag byte has or lacks 0x10), default words
// (0xFFFF, negative, in range, beyond), output slots aliasing the record,
// plus fault injection (one region removed).
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_environment_palette.hpp"
using namespace frame_misc_harness;
namespace palette=halo2::native_environment_palette_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t header=0x10000000u,entries=0x10100000u,defaults=0x10200000u,tags=0x10300000u,
    tag_data=0x10400000u,records=0x10500000u,output=0x10600000u,stack=0x20010000u;
std::uint32_t index_byte(std::mt19937 &rng,std::uint32_t count) {
    const auto roll=rng()%8;
    if(roll==0)return 0xffu;
    if(roll==1)return 0x80u|std::uint32_t(rng()%0x7f);
    if(roll==2)return std::min<std::uint32_t>(count+std::uint32_t(rng()%3),0x7fu);
    return count?std::uint32_t(rng()%count):0u;
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    m.put(0x4ecc20u,header);m.put(0x4f0414u,tags);
    std::uint32_t count=pick(24);
    if(shape==1)count=0;
    if(shape==2)count=0x80000000u|pick(8);
    m.put(header+8u,count);m.put(header+0xcu,entries);m.put(header+0x214u,defaults);
    for(std::uint32_t at=header;at<header+0x220u;at+=4)if(at!=header+8u && at!=header+0xcu && at!=header+0x214u)
        m.put(at,std::uint32_t(rng()));
    const std::uint32_t authored=std::int32_t(count)<0?24u:count+4u;
    for(std::uint32_t k=0;k<authored+0x80u;++k) {
        m.put(entries+k*8u,std::uint32_t(rng()));
        m.put(entries+k*8u+4u,pick(5)==0?0xffffffffu:((std::uint32_t(rng())<<16)|pick(16)));
    }
    for(std::uint32_t k=0;k<16;++k) {
        m.put(tags+k*16u,std::uint32_t(rng()));m.put(tags+k*16u+4u,std::uint32_t(rng()));
        m.put(tags+k*16u+8u,tag_data+k*0x20u);m.put(tags+k*16u+12u,std::uint32_t(rng()));
        for(std::uint32_t at=0;at<0x20u;at+=4)m.put(tag_data+k*0x20u+at,std::uint32_t(rng()));
    }
    // The [474C24] selector picks one of the 0x44-byte default records.
    const std::uint32_t selector=pick(8);
    m.put(0x474c24u,shape==3?0xffffu:selector,2);
    for(std::uint32_t k=0;k<8;++k) {
        const auto record=defaults+k*0x44u;
        for(std::uint32_t at=0;at<0x44u;at+=4)m.put(record+at,std::uint32_t(rng()));
        std::uint32_t word=std::int32_t(count)>0?pick(std::min(count,0x40u)):0u;
        const auto roll=pick(8);
        if(roll==0)word=0xffffu;
        if(roll==1)word=0x8000u|pick(0x7fffu);
        if(roll==2)word=std::min<std::uint32_t>(count+pick(3),0x7fffu);
        m.put(record+0x40u,word,2);
    }
    for(std::uint32_t at=defaults-0x44u;at<defaults;at+=4)m.put(at,std::uint32_t(rng()));
    const auto record=records+pick(16)*0x80u;
    for(std::uint32_t at=records;at<records+16u*0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(record+0x6cu,shape==4?0xffu:index_byte(rng,std::int32_t(count)<0?0x7fu:count),1);
    m.put(record+0x6eu,index_byte(rng,std::int32_t(count)<0?0x7fu:count),1);
    for(std::uint32_t at=output;at<output+16u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x131c00u+pick(0x80));
    random_abi(rng,c,stack);
    c.abi.gp[0]=record;
    // The output slot, sometimes aliasing the record's index bytes.
    c.abi.gp[3]=shape==5?record+0x6cu:output+4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,entries,defaults,tags,tag_data,records,output,0x474000u};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s){return palette::bridge(s,[](auto &v){original::native_frame_misc_reference_133a70(v);});}
void reference(WorldState &s){original::run_133a70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ENVIRONMENT_PALETTE");
    std::mt19937 rng(0x133a70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,palette::coverage().load(),unsigned(palette::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
