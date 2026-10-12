#include "halo2/vista_loading_geometry.hpp"

#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string& label) { if(!value)throw std::runtime_error(label); }
std::uint32_t word(const std::string& text) { return std::uint32_t(std::stoul(text,nullptr,16)); }
std::string hex(std::uint32_t value) { std::ostringstream out;out<<std::hex<<value;return out.str(); }
std::uint32_t float_bits(float value) { return std::bit_cast<std::uint32_t>(value); }
}
int main(int argc,char** argv) {
  try {
    check(argc==2,"Supply original Vista loading command fixture");
    std::ifstream file(argv[1]);const auto fixture=Json::parse(file);
    check(fixture.at("schema")=="H2V_LOADING_PRESENTATION_COMMANDS_V1","Wrong loading command schema");
    check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","Wrong original Vista PE");
    for(unsigned i=0;i<5;++i) {
      const auto& src=fixture.at("vertex_declaration").at("elements")[i];
      const auto& native=vista_loading_vertex_declaration[i];
      check(native.stream==src.at("stream") && native.offset==src.at("offset") && native.type==src.at("type") &&
          native.method==src.at("method") && native.usage==src.at("usage") && native.usage_index==src.at("usage_index"),"Original expanded declaration mismatch");
    }
    unsigned cases{},draws{},vertices{};
    for(const auto& row:fixture.at("cases")) {
      if(row.at("mode")!=0 || row.at("throttled").get<bool>() || row.at("device_guard").get<bool>())continue;
      VistaLoadingFadeState fade{std::uint8_t(row.at("active").get<bool>()),float_bits(row.at("fade_start").get<float>()),0x3f800000};
      const auto elapsed=vista_loading_fade_update(fade,0,row.at("elapsed_ms"),{0x027f,0x1f80});
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
      std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
      const auto mxcsr=_mm_getcsr();const auto exceptions=std::fetestexcept(FE_ALL_EXCEPT);
#endif
      const auto actual=vista_loading_geometry(elapsed.elapsed_seconds_bits,row.at("width"),row.at("height"),
          float_bits(row.at("supplied_sweep_progress").get<float>()),elapsed.packed_argb,{0x027f,0x1f80});
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
      std::uint16_t after_cw;asm volatile("fnstcw %0":"=m"(after_cw));
      check(cw==after_cw && mxcsr==_mm_getcsr() && exceptions==std::fetestexcept(FE_ALL_EXCEPT),"Geometry leaked host FP state");
#endif
      check(actual.size()==row.at("draws").size(),"Original loading draw count mismatch");
      for(unsigned i=0;i<actual.size();++i) {
        const auto& native=actual[i];const auto& source=row.at("draws")[i];
        const auto label="case "+std::to_string(cases)+" draw "+std::to_string(i);
        check(native.primitive==source.at("kind") && native.shader==source.at("shader"),label+": primitive/shader mismatch");
        if(native.shader!=3)
          check(native.texture==source.at("texture_handles").at("0").get<unsigned>()-0x7000,label+": source texture mismatch");
        const unsigned source_blend=source.at("states").at("19"),destination_blend=source.at("states").at("20");
        if(native.blend==VistaLoadingBlend::one_one)check(source_blend==2 && destination_blend==2,label+": additive blend mismatch");
        else if(native.blend==VistaLoadingBlend::destination_color_zero)check(source_blend==9 && destination_blend==1,label+": multiply blend mismatch");
        else check(source_blend==5 && destination_blend==6,label+": alpha blend mismatch");
        for(unsigned v=0;v<4;++v) {
          std::array<std::uint32_t,7> native_words;
          std::memcpy(native_words.data(),&native.vertices[v],28);
          const auto& expected=source.at("vertices")[v].at("words");
          for(unsigned k=0;k<7;++k)
            check(native_words[k]==word(expected[k]),label+" vertex "+std::to_string(v)+" word "+std::to_string(k)+
                ": "+hex(native_words[k])+" != "+expected[k].get<std::string>());
          ++vertices;
        }
        if(native.pixel_constant0) {
          const auto& values=source.at("constants").at("pixel:0").at("raw_hex").get_ref<const std::string&>();
          for(unsigned k=0;k<4;++k) {
            const auto at=k*8;
            const auto text=values.substr(at+6,2)+values.substr(at+4,2)+values.substr(at+2,2)+values.substr(at,2);
            check((*native.pixel_constant0)[k]==word(text),label+": pixel constant word "+std::to_string(k)+" mismatch "+hex((*native.pixel_constant0)[k])+" != "+text);
          }
        }
        ++draws;
      }
      ++cases;
    }
    check(cases!=0,"No original normal mode cases");
    std::cout<<"PASS Vista loading original geometry cases="<<cases<<" draws="<<draws<<" vertices="<<vertices<<"\n";
  } catch(const std::exception& e) { std::cerr<<e.what()<<"\n";return 1; }
}
