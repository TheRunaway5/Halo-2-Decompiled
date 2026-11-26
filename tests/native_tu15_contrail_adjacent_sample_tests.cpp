#include "native_tu15_contrail_draw_span_harness.hpp"
#include "rendering/effects/native_tu15_contrail_adjacent_sample.hpp"

using namespace contrail_draw_span_harness;

namespace fixture {
#include "contrail_adjacent_sample_cases.inc"

Case make_case(std::mt19937 &random,unsigned n) {
    constexpr std::uint32_t stack=0x60001000u, point=0x61000080u, segments=0x62000300u;
    Case test;
    random_abi(random,test,stack);
    auto &m=test.memory;
    auto &g=test.abi.gp;
    g[5]=n%13u==3u ? stack+0x80u : point;
    g[2]=n%13u==4u ? stack+0x34u : segments;
    const int index=n%17u==12u ? -3 : n%13u==4u ? -1 : int(n%5u)-1;
    const auto next=g[2]+std::uint32_t((index+1)*0x40);

    for (unsigned offset=0;offset<=0xc8u;offset+=4) m.put(stack+offset,std::uint32_t(random()));
    constexpr std::uint32_t values[]{0x3e4ccccdu,0x3f000000u,0x3f800000u,0xbe800000u,0x80000000u,
                                     0x00000001u,0x807fffffu,0x7f800000u,0x7f800001u,0xffc01913u};
    for (unsigned lane=0;lane<4u;++lane) {
        m.put(next+0x1cu+lane*4u,values[(n+lane)%10u]);
        m.put(next+0x2cu+lane*4u,values[(n+lane*3u+2u)%10u]);
        m.put(stack+0x30u+lane*4u,values[(n+lane*2u+1u)%10u]);
    }
    m.put(0x46a0d0u,0x3f800000u);
    m.put(next+0x18u,values[(n+2u)%10u]);
    m.put(next+0x3cu,(n%2u ? 0x10u : 0u)|0x81u,1);
    m.put(g[5]+2u,(n%4u ? 2u : 0u)|0x80u,1);
    m.put(g[5]+3u,std::uint8_t(index),1);
    m.put(g[5]+4u,values[(n+1u)%10u]);
    m.put(g[5]+0xcu,values[(n+3u)%10u]);
    m.put(stack+0x4cu,values[(n+4u)%10u]);
    test.abi.xmm[4][0]=m.get(stack+0x34u);
    test.abi.xmm[5][0]=m.get(stack+0x38u);
    test.abi.xmm[6][0]=values[(n+5u)%10u];
    return test;
}
} // namespace fixture

int main(int argc,char **argv) {
    if (argc!=3) return 2;
    const auto cases=unsigned(std::stoul(argv[1]));
    const bool forced=std::string_view(argv[2])=="reference";
    std::mt19937 random(0x42eabu);
    Stats stats;
    for (unsigned n=0;n<cases;++n) {
        const auto input=fixture::make_case(random,n);
        const auto packets=compare<fixture::adjacent_reference, fixture::adjacent_binding>(input,n,stats,forced);
        if (n<8u) for (unsigned cut=1;cut<=packets;++cut)
            compare<fixture::adjacent_reference, fixture::adjacent_binding>(input,n,stats,forced,Policy::draw,cut);
    }
    for (const auto policy : {Policy::unscoped,Policy::trace,Policy::observer})
        compare<fixture::adjacent_reference, fixture::adjacent_binding>(fixture::make_case(random,0),0,stats,forced,policy);
    const auto blend=stats.sites[0x42eb5u], scaled=stats.sites[0x42ed9u], joined=stats.sites[0x42fbdu];
    if (!scaled || scaled==blend || blend>=joined || joined!=cases+3u)
        throw std::runtime_error("Missing adjacent sampling branch coverage");
    std::printf("{\"cases\":%u,\"faults\":%u,\"native_returns\":%u,\"packets\":%u,\"writes\":%u,"
                "\"blended\":%u,\"width_scaled\":%u,\"no_blend\":%u}\n",
        stats.cases,stats.faults,stats.admissions,stats.packets,stats.writes,blend,scaled,joined-blend);
}
