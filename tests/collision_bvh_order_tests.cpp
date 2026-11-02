#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include <bit>
#include <cstdlib>
#include <iostream>
#include <random>
using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
bool same(Vec3 a,Vec3 b) {
    return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
        std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
        std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);
}
void reference(bool enabled) {
#ifdef _WIN32
    _putenv_s("HALO2_REFERENCE_UNCACHED_COLLISION_BOUNDS",enabled?"1":"");
#else
    if(enabled)setenv("HALO2_REFERENCE_UNCACHED_COLLISION_BOUNDS","1",1);
    else unsetenv("HALO2_REFERENCE_UNCACHED_COLLISION_BOUNDS");
#endif
}
void compare(const std::vector<CollisionTriangle>& triangles) {
    reference(true);const CollisionWorld original(triangles);reference(false);
    const CollisionWorld cached(triangles);
    const auto a=original.authority_checkpoint(),b=cached.authority_checkpoint();
    need(a.order==b.order && a.nodes.size()==b.nodes.size(),"Cached bounds changed exact collision partition/tie order");
    for(std::size_t n=0;n<a.nodes.size();++n) {
        const auto& x=a.nodes[n];const auto& y=b.nodes[n];
        need(x.begin==y.begin && x.end==y.end && x.left==y.left && x.right==y.right &&
             same(x.low,y.low) && same(x.high,y.high),"Cached bounds changed original BVH node words/order");
    }
    for(unsigned n=0;n<128;++n) {
        const Vec3 origin{float(int(n%16)-8),float(int(n/16)-4),20};
        const auto x=original.raycast(origin,{.03f,-.02f,-1},40),y=cached.raycast(origin,{.03f,-.02f,-1},40);
        need(bool(x)==bool(y),"Cached bounds changed ray admission");
        if(x)need(std::bit_cast<std::uint32_t>(x->distance)==std::bit_cast<std::uint32_t>(y->distance) &&
            x->triangle==y->triangle && same(x->position,y->position) && same(x->normal,y->normal),
            "Cached bounds changed original ray/contact tie result");
    }
}
}
int main(int argc,char** argv)try {
    compare({});std::mt19937 random(0x5a4e5a49u);
    std::vector<CollisionTriangle> triangles;
    for(unsigned n=0;n<4096;++n) {
        const auto value=[&]{return float(int(random()%8192)-4096)/257.f;};
        CollisionTriangle t{{value(),value(),value()},{value(),value(),value()},{value(),value(),value()}};
        if(n%11==0)t.a.x=-0.f;
        triangles.push_back(t);if(n%5==0)triangles.push_back(t);
        if(n==0 || n==8 || n==63 || n==511)compare(triangles);
    }
    compare(triangles);
    if(argc==2 && std::filesystem::is_regular_file(argv[1])) {
        const Cache cache(argv[1]);need(cache.is_mcc(),"BVH live-map fixture requires MCC");
        compare(cache.load_content(0,true).collision);
    } else return 77;
    std::cout<<"PASS exact original BVH node words, partition and tie order: signed-zero/coincident/random cases and MCC Zanzibar\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
