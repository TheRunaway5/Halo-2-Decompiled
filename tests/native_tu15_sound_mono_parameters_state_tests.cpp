#include "halo2/native_tu15_sound_mono_parameters.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace api=halo2::tu15_mono_parameter_conversion;
struct alignas(16) Image { std::array<unsigned char,512> bytes{}; };
template<class T> void put(Image& image,unsigned offset,T value){std::memcpy(image.bytes.data()+offset,&value,sizeof(value));}
template<class T> T get(const Image& image,unsigned offset){T value;std::memcpy(&value,image.bytes.data()+offset,sizeof(value));return value;}
void save(Image& image){asm volatile("fxsave64 %0":"=m"(image)::"memory");}
void restore(const Image& image){
 asm volatile("fxrstor64 %0"::"m"(image):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)",
              "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15");
}
enum class Call { quantize,convert,nonfinite_quantize,nonfinite_convert,outside_convert };
std::uint64_t calls=0,fields=0,exceptions=0,wide_pointers=0,occupied=0;
Image original;
constexpr std::array<std::uint32_t,7> valid{0x3f000000,0x447a0000,0x40c00000,0x3f3504f3,0x44fa0000,0xc0c00000,0x3f3504f3};
constexpr std::array<api::FpMode,3> modes{api::FpMode::source_pretable_53,api::FpMode::double_53,api::FpMode::extended_64};
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void check_case(const Image& initial,api::FpMode mode,Call call,bool rejected){
 Image before,after;
 restore(initial);save(before);
 bool caught=false;
 try {
  switch(call){
   case Call::quantize:(void)api::quantize_finite_fraction(0.25,mode);break;
   case Call::convert:(void)api::convert_bounded_values(valid,mode);break;
   case Call::nonfinite_quantize:(void)api::quantize_finite_fraction(std::bit_cast<double>(UINT64_C(0x7ff8000000000001)),mode);break;
   case Call::nonfinite_convert:{auto bits=valid;bits[5]=0x7f800000;(void)api::convert_bounded_values(bits,mode);break;}
   case Call::outside_convert:{auto bits=valid;bits[6]=0;(void)api::convert_bounded_values(bits,mode);break;}
  }
 }catch(const std::invalid_argument&){caught=true;}
 save(after);restore(original);
 check(caught==rejected,"Unexpected admission/rejection");
 // Architected FXSAVE64 header contains full CW/SW/FTW/FOP/FIP/FDP,
 // MXCSR and MXCSR_MASK. Reserved header bytes also compare literally.
 if(std::memcmp(before.bytes.data(),after.bytes.data(),32)!=0){
  std::fprintf(stderr,"CW %04x -> %04x, SW %04x -> %04x, FTW %02x -> %02x, MXCSR %08x -> %08x\n",
               get<std::uint16_t>(before,0),get<std::uint16_t>(after,0),get<std::uint16_t>(before,2),get<std::uint16_t>(after,2),
               before.bytes[4],after.bytes[4],get<std::uint32_t>(before,24),get<std::uint32_t>(after,24));
  throw std::runtime_error("Floating-point header changed");
 }
 fields+=32;
#ifndef BASELINE_HEADER_ONLY
 for(unsigned slot=0;slot<8;++slot){
  check(std::memcmp(before.bytes.data()+32+16*slot,after.bytes.data()+32+16*slot,10)==0,"Physical x87 register payload changed");
  fields+=10;
 }
#endif
 check(get<std::uint64_t>(initial,8)>UINT64_C(0xffffffff)&&get<std::uint64_t>(initial,16)>UINT64_C(0xffffffff),"Wide diagnostic pointers not requested");
 // AMD may report zero for these diagnostic fields even after literal restore.
 // Preserve the actually observed fields; do not claim an unobserved nonzero case.
 if(get<std::uint64_t>(before,8)>UINT64_C(0xffffffff)&&get<std::uint64_t>(before,16)>UINT64_C(0xffffffff))++wide_pointers;
 ++calls;if(caught)++exceptions;if(before.bytes[4])++occupied;
}
Image image(std::uint16_t cw,std::uint16_t sw,std::uint32_t mxcsr,std::uint8_t tags,unsigned serial){
 Image value=original;
 put(value,0,cw);put(value,2,sw);value.bytes[4]=tags;
 put(value,6,static_cast<std::uint16_t>(0x345));
 put(value,8,UINT64_C(0x0000123456789000)+(static_cast<std::uint64_t>(serial)<<4));
 put(value,16,UINT64_C(0x000023456789a000)+(static_cast<std::uint64_t>(serial)<<4));
 put(value,24,mxcsr);
 for(unsigned slot=0;slot<8;++slot){
  put(value,32+16*slot,UINT64_C(0x8000000000000000)|(static_cast<std::uint64_t>(slot)<<52)|serial);
  put(value,40+16*slot,static_cast<std::uint16_t>(0x3fff+slot+((slot&1)?0x8000:0)));
 }
 return value;
}
int main(){
 try{
  save(original);
  const auto supported=get<std::uint32_t>(original,28);
  check((supported&0xffff)==0xffff,"This explicit matrix requires DAZ/FTZ and all standard MXCSR fields");
  // Exhaust every literal status image, including all CC/TOP/exception/SF/ES/B bits.
  for(unsigned sw=0;sw<65536;++sw)
   for(auto mode:modes)check_case(image(0x037f,static_cast<std::uint16_t>(sw),0x1f80,0,sw),mode,Call::quantize,false);
  const auto status_calls=calls;
  // All caller precision, rounding, infinity-control and exception-mask choices.
  for(unsigned precision:{0u,2u,3u})for(unsigned rounding=0;rounding<4;++rounding)
   for(unsigned infinity=0;infinity<2;++infinity)for(unsigned mask=0;mask<64;++mask)
    for(auto mode:modes){
     const auto serial=precision*8192+rounding*2048+infinity*1024+mask;
     const auto cw=static_cast<std::uint16_t>(0x40|mask|(precision<<8)|(rounding<<10)|(infinity<<12));
     const auto sw=static_cast<std::uint16_t>((serial*4051)&0xffff);
     check_case(image(cw,sw,0x1f80,0,serial),mode,Call::quantize,false);
    }
  const auto control_calls=calls-status_calls;
  // All MXCSR sticky flags/masks, rounding, and DAZ/FTZ combinations.
  for(unsigned flags=0;flags<64;++flags)for(unsigned mask=0;mask<64;++mask)
   for(unsigned rounding=0;rounding<4;++rounding)for(unsigned modeset=0;modeset<4;++modeset){
    const auto mxcsr=flags|(mask<<7)|(rounding<<13)|((modeset&1)<<6)|((modeset>>1)<<15);
    const auto serial=flags*1024+mask*16+rounding*4+modeset;
    check_case(image(0x037f,static_cast<std::uint16_t>(serial),mxcsr,0,serial),modes[serial%3],Call::quantize,false);
   }
  const auto mxcsr_calls=calls-status_calls-control_calls;
  // Every nonempty abridged-tag mask at each TOP, all explicit admitted modes.
  for(unsigned tags=1;tags<256;++tags)for(unsigned top=0;top<8;++top)for(auto mode:modes){
   const auto serial=tags*8+top;
   check_case(image(0x037f,static_cast<std::uint16_t>(0x4700|(top<<11)),0x1f80,static_cast<std::uint8_t>(tags),serial),mode,Call::quantize,true);
  }
  const auto occupied_calls=occupied;
  // Real conversion and every error boundary restore the same full environment.
  for(unsigned i=0;i<1024;++i)for(auto mode:modes){
   const auto mxcsr=0x1f80|((i&3)<<13)|(((i>>2)&1)<<6)|(((i>>3)&1)<<15)|(i&63);
   const auto initial=image(static_cast<std::uint16_t>(0x7f|((i%3==0?0:i%3==1?2:3)<<8)|((i&3)<<10)),static_cast<std::uint16_t>(i*4051),mxcsr,0,i);
   check_case(initial,mode,Call::convert,false);
   check_case(initial,mode,Call::nonfinite_quantize,true);
   check_case(initial,mode,Call::nonfinite_convert,true);
   check_case(initial,mode,Call::outside_convert,true);
  }
  const auto errors_before_invalid=exceptions;
  for(unsigned i=0;i<256;++i)for(unsigned invalid:{0u,1u,0x13fu,0x033fu,0xffffu}){
   const auto initial=image(0x037f,static_cast<std::uint16_t>(i*257),0x1f80,static_cast<std::uint8_t>(i),i);
   check_case(initial,static_cast<api::FpMode>(invalid),Call::quantize,true);
  }
  std::printf("{\"passed\":true,\"calls\":%lu,\"status_cases\":%lu,\"caller_control_cases\":%lu,\"MXCSR_cases\":%lu,\"occupied_stack_rejects\":%lu,\"invalid_mode_rejects\":%lu,\"all_rejections\":%lu,\"wide_FIP_FDP_requested_cases\":%lu,\"wide_FIP_FDP_observed_cases\":%lu,\"compared_header_and_x87_bytes\":%lu}\n",calls,status_calls,control_calls,mxcsr_calls,occupied_calls,exceptions-errors_before_invalid,exceptions,calls,wide_pointers,fields);
 }catch(const std::exception& error){restore(original);std::fprintf(stderr,"%s after %lu calls\n",error.what(),calls);return 1;}
}
