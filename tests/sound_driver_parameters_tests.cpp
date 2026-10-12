#include "halo2/original_data.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <xmmintrin.h>

namespace {
using namespace halo2;
void check(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
std::uint32_t number(const std::string &s){return std::stoul(s,nullptr,16);}
std::uint32_t hex(std::istringstream &row){std::string s;check(bool(row>>s),"Incomplete source control");return number(s);}
std::array<std::uint32_t,16> words(const SoundDriverParameterConstants &c){
  return {c.source_pitch_to_driver_pitch_bits,c.driver_pitch_min_bits,c.driver_pitch_max_bits,
          c.gain_normalized_to_driver_volume_bits,c.minimum_source_gain_bits,c.parameter_change_threshold_bits,
          c.driver_pitch_change_threshold_bits,c.driver_pitch_octave_scale_bits,c.relative_rate_ln2_bits,
          c.mono_channel_count_bits,c.stereo_channel_count_bits,c.routing_db_ln10_bits,c.routing_db_scale_bits,
          c.routing_linear_db_scale_bits,c.normalized_parameter_range_epsilon_bits,c.silent_source_gain_bits};
}
std::uint32_t word(const std::string &s){
  check(s.size()==8,"Incomplete original operand");std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=number(s.substr(i*2,2))<<(i*8);return v;
}
template<class F> void rejects(F &&f){
  bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}
  check(rejected,"Missing source input was replaced by a default");
}
std::uint16_t control_word(){std::uint16_t out;asm volatile("fnstcw %0":"=m"(out));return out;}
}
int main(int argc,char **argv){
  try{
    const bool single_image=argc==4 && std::string(argv[1])=="--selected-image";
    check(single_image || argc==6,"Supply scalar/operand fixtures and base/1.1/u5 executables, or --selected-image scalars executable");
    for(int i=single_image?2:1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    const auto selected=OriginalData::load_xbe(argv[3]);
    check(bool(selected->sound_driver_parameter_constants),"Verified selected-image driver constants unavailable");
    std::vector<std::shared_ptr<const OriginalData>> images;
    if(!single_image)images={OriginalData::load_xbe(argv[4]),OriginalData::load_xbe(argv[5])};
    images.push_back(selected);
    if(!single_image){
      std::ifstream operands(argv[2]);std::size_t constants=0;
      for(std::string line;std::getline(operands,line);){
        std::istringstream row(line);char kind;unsigned image;
        check(bool(row>>kind>>image) && kind=='C' && image==constants+1 && image<=2,"Incomplete original operands");
        check(bool(images[image-1]->sound_driver_parameter_constants),"Verified driver operands unavailable");
        const auto actual=words(*images[image-1]->sound_driver_parameter_constants);
        for(auto value:actual){std::string raw;check(bool(row>>raw) && value==word(raw),"Original driver operand differs");}
        ++constants;
      }
      check(constants==2,"Missing verified original operands");
    }
    std::ifstream input(argv[single_image?2:1]);std::array<std::size_t,4> counts{};std::size_t index=0;
    for(std::string line;std::getline(input,line);){
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);std::string kind;row>>kind;
      const SoundDriverFloatingPointContext context{std::uint16_t(hex(row)),hex(row)};
      const auto caller_cw=control_word();const auto caller_mxcsr=_mm_getcsr();
      auto mismatch=[&](const std::string &what,std::uint32_t actual,std::uint32_t expected){
        if(actual!=expected){std::ostringstream message;message<<"Original "<<kind<<" row "<<index<<' '<<what
          <<" differs: "<<std::hex<<actual<<" != "<<expected<<" cw="<<context.x87_control_word<<" mxcsr="<<context.mxcsr;
          throw std::runtime_error(message.str());}
      };
      if(kind=="normalize"){
        const auto low=hex(row),high=hex(row),value=hex(row);unsigned direction;
        check(bool(row>>direction) && direction<=255,"Incomplete original CL");const auto expected=hex(row);
        for(const auto &image:images)mismatch("raw",source_sound_normalized_parameter(low,high,value,std::uint8_t(direction),
          *image->sound_driver_parameter_constants,context),expected);
        ++counts[0];
      }else if(kind=="pitch"){
        std::int64_t rate;check(bool(row>>rate),"Incomplete source rate index");
        const auto value=hex(row),clamped=hex(row),expected=hex(row);
        for(const auto &image:images){const auto actual=calculate_source_sound_driver_pitch(std::uint32_t(rate),value,
          *image->sound_driver_parameter_constants,context);mismatch("clamped",actual.clamped_float_bits,clamped);
          mismatch("CVTTSS2SI",std::uint32_t(actual.driver_pitch),expected);}
        ++counts[1];
      }else if(kind=="gain"){
        const auto value=hex(row),normalized=hex(row),scaled=hex(row),expected=hex(row);
        for(const auto &image:images){const auto actual=calculate_source_sound_driver_volume(value,
          *image->sound_driver_parameter_constants,context);mismatch("normalized",actual.normalized_bits,normalized);
          mismatch("scaled",actual.scaled_float_bits,scaled);mismatch("CVTTSS2SI",std::uint32_t(actual.driver_volume),expected);}
        ++counts[2];
      }else if(kind=="rate"){
        std::int64_t rate;std::int32_t pitch;check(bool(row>>rate>>pitch),"Incomplete source rate controls");const auto expected=hex(row);
        for(const auto &image:images)mismatch("raw",source_sound_relative_rate(pitch,std::uint32_t(rate),
          *image->sound_driver_parameter_constants,context),expected);
        if(counts[3]==0){
          // Declared selected-record ABI control, not a constructed source voice.
          std::array<std::byte,4> backing;
          for(unsigned i=0;i<4;++i)backing[i]=std::byte(std::uint32_t(pitch)>>(i*8));
          const auto before=backing;unsigned reads=0;
          const SoundDriverMemory memory{0x7d90000u,[&](auto address,auto bytes)->std::span<const std::byte>{
            check(address==0x7d90000u+12u+17u*0x34u+8u && bytes==4,"Original selected-driver addressing differs");
            ++reads;return backing;}};
          mismatch("selected-memory",source_sound_relative_rate(17u,std::uint32_t(rate),memory,
            *images[0]->sound_driver_parameter_constants,context),expected);
          check(reads==1 && backing==before,"Driver query modified source backing");
          rejects([&]{(void)source_sound_relative_rate(0u,0u,SoundDriverMemory{},*images[0]->sound_driver_parameter_constants,context);});
          rejects([&]{(void)source_sound_normalized_parameter(0,0,0,0,*images[0]->sound_driver_parameter_constants,{0,0});});
        }
        ++counts[3];
      }else throw std::runtime_error("Unknown original scalar row");
      check(control_word()==caller_cw && _mm_getcsr()==caller_mxcsr,"Native audio calculation changed caller FP context");
      ++index;
    }
    check(counts==std::array<std::size_t,4>{3168,4096,2048,2048},"Incomplete original scalar coverage");
    std::cout<<"supportedImages="<<images.size()<<" scalarRows="<<index
             <<" pairedNativeChecks="<<index*images.size()<<" callerFpContext=preserved missingInputs=passed "
               "driverRoutingAdmissionAndPlayback=unverified\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
