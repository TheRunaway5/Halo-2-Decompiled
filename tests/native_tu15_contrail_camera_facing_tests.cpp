#include "native_tu15_contrail_draw_span_harness.hpp"
#include "rendering/effects/native_tu15_contrail_camera_facing.hpp"

using namespace contrail_draw_span_harness;

namespace fixture {
#include "contrail_camera_facing_cases.inc"

constexpr std::uint32_t stack=0x60001000u, header=0x61000000u, points=0x62000000u, context=0x63000000u;

Case make_case(std::mt19937 &random,unsigned n) {
    Case test;
    random_abi(random,test,stack);
    auto &m=test.memory;
    const auto point=points+0x38u;
    test.abi.gp[3]=n%2u ? points : 0;
    test.abi.gp[5]=point;
    test.abi.gp[2]=context;
    test.abi.xmm[4][0]=n%8u==5u ? 0xffc28721u : 0x3fc00000u;
    test.abi.xmm[7][0]=0x3f800000u;
    m.put(context+0x10u,n%8u!=4u,1);
    m.put(0x4f7210u,header);
    m.put(header+0x44u,points);
    m.put(point+0x34u,0x12340002u);
    m.put(0x46a0ecu,0x38d1b717u);
    for (unsigned offset=0;offset<0xb0u;offset+=4u) m.put(stack+offset,std::uint32_t(random()));

    using V=std::array<float,3>;
    V prev{-1.F,-2.F,-3.F}, current{0.F,0.F,0.F}, next{1.F,2.F,3.F}, camera{10.F,-7.F,4.F};
    switch (n%8u) {
    case 2:
        camera={2.F,4.F,6.F};
        break;
    case 3:
        prev={-0x1p-18f,0.F,0.F};
        next={0x1p-18f,0.F,0.F};
        camera={0.F,1e5F,0.F};
        break;
    case 5:
        camera[0]=std::bit_cast<float>(0x7fc83161u);
        current[1]=-0.F;
        break;
    case 6:
        next[0]=0x1p-144f;
        camera[2]=std::bit_cast<float>(0xff812345u);
        break;
    case 7:
        prev={-1.F,0.F,0.F};
        camera={-1.F,std::bit_cast<float>(0x38d1b717u+(n&0x10u ? 1u : 0u)),0.F};
        break;
    default:
        for (auto &axis : camera) axis+=float(random()%100u)/1024.F;
        break;
    }
    for (unsigned axis=0;axis<3u;++axis) {
        m.put(points+0x1cu+axis*4u,u(prev[axis]));
        m.put(point+0x1cu+axis*4u,u(current[axis]));
        m.put(point+0x38u+0x1cu+axis*4u,u(next[axis]));
        m.put(0x4c65c0u+axis*4u,u(camera[axis]));
    }
    return test;
}

} // namespace fixture

int main(int argc,char **argv) {
    if (argc!=3) return 2;
    const auto cases=unsigned(std::stoul(argv[1]));
    const bool forced=std::string_view(argv[2])=="reference";
    std::mt19937 random(0x432bdu);
    Stats stats;
    for (unsigned n=0;n<cases;++n) {
        const auto input=fixture::make_case(random,n);
        const auto packets=compare<fixture::camera_reference, fixture::camera_binding>(input,n,stats,forced);
        if (n<8u) for (unsigned cut=1;cut<=packets;++cut)
            compare<fixture::camera_reference, fixture::camera_binding>(input,n,stats,forced,Policy::draw,cut);
    }
    for (const auto policy : {Policy::unscoped,Policy::trace,Policy::observer})
        compare<fixture::camera_reference, fixture::camera_binding>(fixture::make_case(random,0),0,stats,forced,policy);
    const auto tiny_camera=stats.sites[0x43446u];
    const auto tiny_face=stats.sites[0x4350du]-stats.sites[0x4350fu];
    const auto skip_face=stats.sites[0x4349bu]-stats.sites[0x434a1u];
    if (!tiny_camera || !tiny_face || !skip_face || !stats.sites[0x432c9u] || !stats.sites[0x43329u])
        throw std::runtime_error("Missing camera-facing branch coverage");
    std::printf("{\"cases\":%u,\"faults\":%u,\"normal_returns\":%u,\"packets\":%u,\"writes\":%u,"
                "\"tiny_camera_side\":%u,\"tiny_face_normal\":%u,\"without_face_normal\":%u}\n",
        stats.cases,stats.faults,stats.admissions,stats.packets,stats.writes,tiny_camera,tiny_face,skip_face);
}
