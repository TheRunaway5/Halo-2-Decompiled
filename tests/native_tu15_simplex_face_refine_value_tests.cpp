#define main refinement_core_main
#include "native_tu15_simplex_face_refine_tests.cpp"
#undef main
#include "reference/native_tu15_simplex_face_refine_value_before.cpp"
int main(){try{
    std::mt19937 rng(0x30d1a2u);constexpr std::array edge{0u,0x80000000u,1u,0x80000001u,0x007fffffu,0x00800000u,0x7f7fffffu,0xff7fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0xffc23456u,0x7f812345u,0xff823456u};unsigned cases{};
    for(unsigned k=0;k<256u;++k){Memory a;for(auto&v:a.bank)v=std::byte(rng());auto vertices=base+0x2000u,scores=base+0x3000u,point=base+0x1000u,scratch=base+0x5000u;if(k%16u==6u)vertices=scratch-0x30u;if(k%16u==10u)scores=scratch+0x10u;
        for(auto p:{point,vertices-0x100u})for(unsigned row=0;row<40u;++row)for(unsigned lane=0;lane<4u;++lane)a.put(p+row*16u+lane*4u,k<128u?std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f):edge[(k+row*3u+lane*5u)%edge.size()]);
        for(int n=-4;n<5;++n)a.put(scores+std::uint32_t(n*4),k<128u?std::bit_cast<std::uint32_t>(float(int(rng()%7u)-2)):edge[(k+unsigned(n+4)*3u)%edge.size()]);
        if(k==0u){for(unsigned row=0;row<4u;++row)for(unsigned lane=0;lane<4u;++lane)a.put(vertices+row*16u+lane*4u,std::bit_cast<std::uint32_t>(lane==0u?float(row)*1.0e20f:0.0f));for(unsigned lane=0;lane<4u;++lane)a.put(point+lane*4u,std::bit_cast<std::uint32_t>(lane==0u?1.0f:0.0f));for(unsigned n=0;n<3u;++n)a.put(scores+n*4u,0x3f800000u);}
        constexpr std::array<unsigned,5>table{2,0,1,2,0};for(unsigned n=0;n<5u;++n)a.table[0x760u+n]=std::byte(k%8u==7u?std::array{255u,254u,1u,0u,253u}[n]:table[n]);Memory b=a;
        NativeTu15PhysicsFrameFp afp{std::uint16_t(0x37fu|((k%4u)<<10u)),std::uint16_t(0x4100u|((k%8u)<<11u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)},bfp=afp;std::vector<SourcePhysicsContactRecordStore>astores,bstores;std::vector<std::array<std::uint32_t,2>>afp_events,bfp_events;
        auto observe=[&](auto&memory,auto&stores,auto&events,const auto&e){stores.push_back(e);std::uint16_t sw;asm volatile("fnstsw %0":"=a"(sw));events.push_back({_mm_getcsr(),sw});if(k%4u==1u&&e.instruction==0x30d25eu)memory.put(e.address,0x3f800000u);if(k%4u==2u&&e.instruction==0x30d2e3u)memory.put(point,0xbf800000u);};
        const auto expected=face_refine_value_reference::native_tu15_refine_simplex_face(a,vertices,point,scores,scratch,afp,k%7u,[&](const auto&e){observe(a,astores,afp_events,e);});const auto actual=native_tu15_refine_simplex_face(b,vertices,point,scores,scratch,bfp,k%7u,[&](const auto&e){observe(b,bstores,bfp_events,e);});
        if(expected!=actual||a.bank!=b.bank||a.table!=b.table||a.constant!=b.constant||a.reads!=b.reads||astores!=bstores||afp_events!=bfp_events||afp.control_word!=bfp.control_word||afp.status!=bfp.status||afp.mxcsr!=bfp.mxcsr)throw std::runtime_error("Face-refine DATA view mismatch "+std::to_string(k));++cases;
    }
    std::cerr<<"{\"value_cases\":"<<cases<<",\"frozen_previous_DATA_reference\":true,\"physical_table_mutations\":false}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
