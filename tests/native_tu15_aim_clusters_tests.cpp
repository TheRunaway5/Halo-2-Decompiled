// Native TU1.5 aim cluster flood (14D200) against the unmodified translated
// body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 14D200 has no children. Fixtures author the structure BSP (cluster and
// portal tables), clusters with no, negative and many portals, portal sides
// of NONE and out-of-table indices, the visit stamp and table (start and far
// clusters already stamped), the cone slope, the arguments (point,
// direction, max distance, width, a capacity from zero up, the output and
// a null or live count output), DX (the start cluster) and XMM5 (the
// spread), with positions inside and outside every test, special floats and
// distinct NaN payloads; a star graph larger than the 512-entry stack
// overflows the frame as the original does; one region removed injects a
// fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_aim_clusters.hpp"
using namespace frame_misc_harness;
namespace aim=halo2::native_aim_clusters_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t bsp=0x10000000u,clusters=0x10100000u,portals=0x10200000u,portal_lists=0x10300000u,
    vectors=0x10400000u,output=0x10500000u,count_slot=0x10600000u,long_list=0x11000000u;
std::uint32_t stack=0x20010000u;   // the entry stack pointer; its low bits vary (the exit's ADD flags)
std::uint32_t quiet_nan(std::mt19937 &rng) {
    static constexpr std::uint32_t payloads[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7fc12345u,0xffc54321u,0xff800003u};
    return payloads[rng()%6];
}
std::uint32_t value(std::mt19937 &rng,unsigned shape,float scale) {
    if(shape==4 && rng()%7==0)return quiet_nan(rng);
    if(shape==3 && rng()%5==0) {
        static constexpr std::uint32_t special[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
            0x807fffffu,0x80000000u,0x00000000u};
        return special[rng()%9];
    }
    return std::bit_cast<std::uint32_t>(float(int(rng()%2001)-1000)/1000.0f*scale);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    stack=0x20010000u+pick(64)*4u;
    // The star graph (shape 13) has more clusters than the stack holds.
    const std::uint32_t cluster_count=shape==13?560u:4u+pick(28),portal_count=shape==13?600u:4u+pick(48);
    m.put(0x4ecc18u,bsp);
    for(std::uint32_t at=bsp;at<bsp+0xb0u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(bsp+0x5cu,portal_count);m.put(bsp+0x60u,portals);
    m.put(bsp+0x9cu,cluster_count);m.put(bsp+0xa0u,clusters);
    // Portals: two sides, a bounding sphere ahead of the point mostly.
    for(std::uint32_t k=0;k<portal_count;++k) {
        const auto p=portals+k*36u;
        for(std::uint32_t at=p;at<p+36u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto side=[&]()->std::uint32_t {
            if(pick(29)==0)return 0xffffu;                       // NONE: reads the stamp itself
            if(shape==12 && pick(9)==0)return cluster_count+pick(4);  // just past the table
            return shape==13?k%cluster_count:pick(cluster_count);
        };
        m.put(p,shape==13?0u:side(),2);m.put(p+2u,shape==13?(k%(cluster_count-1u))+1u:side(),2);
        m.put(p+8u,value(rng,shape,8.0f));m.put(p+12u,value(rng,shape,8.0f));
        m.put(p+16u,std::bit_cast<std::uint32_t>(4.0f+float(pick(4000))/100.0f));
        if(pick(3)==0)m.put(p+16u,value(rng,shape,30.0f));
        m.put(p+20u,shape==3||shape==4?value(rng,shape,3.0f):std::bit_cast<std::uint32_t>(float(pick(600))/100.0f));
    }
    // Clusters: portal index lists (none, negative counts, many).
    for(std::uint32_t k=0;k<cluster_count;++k) {
        const auto cl=clusters+k*0xb0u;
        for(std::uint32_t at=cl;at<cl+0xb0u;at+=4)m.put(at,std::uint32_t(rng()));
        std::uint32_t count=shape==13?(k==0?portal_count:0u):pick(7);
        if(pick(17)==0)count=0x80000000u|pick(5);
        const auto list=portal_lists+k*0x800u;
        m.put(cl+0x8cu,count);m.put(cl+0x90u,list);
        const auto listed=std::int32_t(count)>0?count:0u;
        for(std::uint32_t j=0;j<listed;++j)m.put(list+j*2u,shape==13?j:pick(portal_count),2);
    }
    // One cluster with more than 32767 portals (every far side NONE, so each
    // is visited): the position wraps to negative as a 16-bit index.
    if(shape==14 && n%64==14) {
        const auto cl=clusters;
        m.put(cl+0x8cu,0x8001u);m.put(cl+0x90u,long_list);
        for(std::uint32_t j=0;j<0x8001u;++j)m.put(long_list+j*2u,0,2);
        m.put(portals,0xffffu,2);m.put(portals+2u,0xffffu,2);
    }
    // The stamp and its table; some slots already carry the next stamp.
    const std::uint32_t stamp=pick(5)==0?0xffffffffu:std::uint32_t(rng());
    m.put(0x4f3ce4u,stamp);m.put(0x4f3ce0u,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<cluster_count+8u;++k)m.put(0x4f3ce8u+k*4u,pick(9)==0?stamp+1u:std::uint32_t(rng()));
    m.put(0x46a11cu,shape==4&&pick(3)==0?quiet_nan(rng):std::bit_cast<std::uint32_t>(0.5f+float(pick(200))/100.0f));
    // Point, direction (a unit-ish vector along +x mostly).
    const auto point=vectors,direction=vectors+0x40u;
    for(unsigned k=0;k<3;++k)m.put(point+4u*k,value(rng,shape,2.0f));
    m.put(direction,shape==3||shape==4?value(rng,shape,1.0f):std::bit_cast<std::uint32_t>(0.6f+float(pick(40))/100.0f));
    m.put(direction+4u,value(rng,shape,0.5f));m.put(direction+8u,value(rng,shape,0.5f));
    for(std::uint32_t at=output-0x40u;at<output+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(count_slot,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x3000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1ae17du);
    m.put(stack+0x4u,point);m.put(stack+0x8u,direction);
    m.put(stack+0xcu,shape==3||shape==4?value(rng,shape,40.0f):std::bit_cast<std::uint32_t>(float(pick(6000))/100.0f));
    m.put(stack+0x10u,shape==3||shape==4?value(rng,shape,4.0f):std::bit_cast<std::uint32_t>(float(pick(400))/100.0f));
    std::uint32_t capacity=shape==9?pick(3):8u+pick(64);
    if(shape==10)capacity=(std::uint32_t(rng())<<16)|(0x8000u+pick(16));   // negative as 16 bits
    if(shape==11)capacity=(std::uint32_t(rng())<<16)|pick(6);              // upper half ignored by the CMPs
    m.put(stack+0x14u,capacity);
    m.put(stack+0x18u,output);
    m.put(stack+0x1cu,pick(3)==0?0u:count_slot);
    random_abi(rng,c,stack);
    // DX = the start cluster (sometimes NONE or past the table).
    std::uint32_t start=pick(cluster_count);
    if(shape==7)start=pick(2)?0xffffu:cluster_count+pick(4);
    if(shape==13 || (shape==14 && n%64==14))start=0;
    c.abi.gp[2]=(std::uint32_t(rng())<<16)|start;
    // XMM5 = the spread.
    c.abi.xmm[5][0]=shape==3||shape==4?value(rng,shape,2.0f):std::bit_cast<std::uint32_t>(0.2f+float(pick(150))/100.0f);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{bsp,clusters,portals,portal_lists,vectors,output,0x4f3000u,0x46a000u};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s){return aim::bridge(s,[](auto &v){original::native_frame_misc_reference_14d200(v);});}
void reference(WorldState &s){original::run_14d200(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_AIM_CLUSTERS");
    std::mt19937 rng(0x14d200u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,aim::coverage().load(),unsigned(aim::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
