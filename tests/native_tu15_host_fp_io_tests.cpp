#include "physics/runtime/native_tu15_host_fp_io.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
struct Fault{};
void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
void poison(){
    // Do not create a callback-internal unmasked exception. The callback must
    // return or throw before the caller can restore its incoming environment.
    asm volatile("fninit;fld1;fldz":::"memory","st","st(1)");
    const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065u);
}
}
int main()try{
    using namespace halo2;
    const auto host=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{host};
    constexpr std::array<std::array<unsigned char,10>,6>values{{
        {{0,0,0,0,0,0,0,0x80,0xff,0x3f}}, // one
        {{0,0,0,0,0,0,0,0,0,0x80}},       // negative zero
        {{0x35,0xc2,0x68,0x21,0xa2,0xda,0x0f,0xc9,0,0x40}},
        {{1,0,0,0,0,0,0,0,0,0}},           // extended subnormal
        {{0x45,0x23,1,0,0,0,0,0xc0,0xff,0x7f}},
        {{0x56,0x34,2,0,0,0,0,0x80,0xff,0x7f}}
    }};
    unsigned comparisons{};
    for(auto cw:{0x007fu,0x027fu,0x037fu})for(unsigned rounding=0;rounding<4;++rounding)
    for(unsigned top=0;top<8;++top)for(unsigned topology=0;topology<7;++topology){
        auto source=host;source.word(0,std::uint16_t(cw|(rounding<<10u)));
        source.word(4,std::uint16_t((top<<11u)|((topology*0x101u)&0x473fu)));
        source.word(8,0xffffu);source.mxcsr=0x1f80u|(rounding<<13u)|((topology&1u)?0x8040u:0u)|(topology&0x3fu);
        for(unsigned lane=0;lane<8;++lane){
            const auto&payload=values[(lane+topology)%values.size()];
            std::memcpy(source.x87.data()+28u+lane*10u,payload.data(),10u);
            const unsigned count=topology==0?0:topology==1?1:topology==2?4:topology==3?6:8;
            const bool live=topology==5?(lane%2u)==0:topology==6?(lane%3u)!=0:lane<count;
            if(live){const auto tag=native_raw_x87_detail::value_tag(source.x87.data()+28u+lane*10u);
                const unsigned shift=((top+lane)&7u)*2u;source.word(8,std::uint16_t((source.word(8)&~(3u<<shift))|(tag<<shift)));}
        }
        for(unsigned mode=0;mode<3;++mode){
            native_raw_x87_detail::restore(source);const auto expected=native_raw_x87_detail::capture();
            unsigned writes{};bool threw{};
            try{
                native_host_fp_io_detail::preserve([&]{
                    ++writes;poison();
                    if(mode==2){const auto nested=native_raw_x87_detail::capture();
                        native_host_fp_io_detail::preserve([&]{poison();});
                        require(native_raw_x87_detail::equivalent(nested,native_raw_x87_detail::capture()),"Nested FP owner changed");}
                    if(mode==1)throw Fault{};
                });
            }catch(const Fault&){threw=true;}
            require(writes==1u&&threw==(mode==1),"FP guard changed ordinary side effects or exception propagation");
            require(native_raw_x87_detail::equivalent(expected,native_raw_x87_detail::capture()),"FP guard changed live payload/tags/control/status/MXCSR");
            ++comparisons;
        }
    }
    native_raw_x87_detail::restore(host);
    std::cout<<"{\"passed\":true,\"comparisons\":"<<comparisons<<",\"state_software_fp_or_mmx_modified\":false,\"return_throw_and_nested\":true}\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
