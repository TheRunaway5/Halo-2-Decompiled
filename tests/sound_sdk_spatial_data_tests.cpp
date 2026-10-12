#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
std::vector<std::byte> unhex(const std::string &text) {
  if (text.size() % 2) throw FormatError("Invalid original SDK byte fixture");
  std::vector<std::byte> raw;
  for (std::size_t i=0;i<text.size();i+=2)
    raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return raw;
}
std::uint32_t word(std::span<const std::byte> raw,std::size_t at=0) {
  std::uint32_t value=0;
  for (unsigned i=0;i<4;++i) value|=std::to_integer<std::uint32_t>(raw[at+i])<<(8*i);
  return value;
}
}
int main(int argc,char **argv) {
  try {
    if (argc!=5) throw FormatError("Supply original SDK data fixture and three owned images");
    for (unsigned image=0;image<3;++image)
      if (!std::filesystem::is_regular_file(argv[image+2])) return 77;
    nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    const auto &ranges=fixture.at("ranges");
    for (unsigned image=0;image<3;++image) {
      const auto imported=OriginalData::load_xbe(argv[image+2]);
      if (!imported->sound_sdk_spatial_data) throw FormatError("Original SDK immutable data missing");
      const auto &data=*imported->sound_sdk_spatial_data;
      const auto &c=data.constants;
      const std::array<std::uint32_t,10> words{c.zero_bits,c.one_bits,c.half_bits,c.position_sentinel_bits,
          c.angle_round_bias_bits,c.half_turn_bits,c.azimuth_scale_bits,c.octant_bits,c.quarter_turn_bits,
          c.volume_scale_bits};
      for (unsigned i=0;i<words.size();++i)
        if (words[i]!=word(unhex(ranges.at(i).at("hex"))))
          throw FormatError("Selected original SDK scalar differs");
      for (unsigned i=0;i<3;++i)
        if (c.mode_one_side[i]!=word(unhex(ranges.at(10).at("hex")),i*4)||
            c.mode_one_forward[i]!=word(unhex(ranges.at(11).at("hex")),i*4))
          throw FormatError("Selected original SDK basis differs");
      const auto address=[&](unsigned index) {
        return std::uint32_t(std::stoul(ranges.at(index).at("variants").at(image).at("address").get<std::string>(),nullptr,16));
      };
      if (c.kernel_table_a!=address(12)||c.kernel_table_b!=address(13)||
          data.null_kernel_identity!=address(14))
        throw FormatError("Selected original SDK kernel identities differ");
      if (data.cone_constants.octant_bits!=c.octant_bits||
          data.cone_constants.quarter_turn_bits!=c.quarter_turn_bits||
          data.cone_constants.angle_scale_bits!=word(unhex(ranges.at(15).at("hex"))))
        throw FormatError("Selected original SDK cone angle operands differ");
      const auto &voice=data.voice_constants;
      const std::array<std::uint32_t,11> voice_words{voice.unsigned_correction_bits,voice.gain_scale_bits,voice.volume_scale_bits,
        voice.doppler_max_bits,voice.doppler_min_bits,voice.doppler_reciprocal_bits,voice.environment_log_floor_bits,
        voice.sample_rate_bits,voice.two_pi_bits,voice.filter_scale_bits,voice.db_exponent_scale_bits};
      for(unsigned i=0;i<voice_words.size();++i)
        if(voice_words[i]!=word(unhex(ranges.at(16+i).at("hex"))))
          throw FormatError("Selected original SDK voice scalar differs");
      const auto voice_double=unhex(ranges.at(27).at("hex"));
      if(voice.zero_bits!=c.zero_bits||voice.one_bits!=c.one_bits||voice.one_double_bits!=
          (std::uint64_t(word(voice_double))|(std::uint64_t(word(voice_double,4))<<32)))
        throw FormatError("Selected original SDK voice shared/double operands differ");
      const auto kernels=unhex(ranges.at(12).at("hex")),null=unhex(ranges.at(14).at("hex"));
      if (!std::ranges::equal(data.kernels,kernels)||!std::ranges::equal(data.null_kernel,null))
        throw FormatError("Selected original SDK complete kernel bytes differ");
      for (unsigned i=0;i<122;++i)
        if (!std::ranges::equal(data.kernel(c.kernel_table_a+i*32),
                               std::span<const std::byte>(kernels).subspan(i*32,32)))
          throw FormatError("Original SDK kernel identity resolution differs");
      if (!std::ranges::equal(data.kernel(data.null_kernel_identity),null))
        throw FormatError("Original SDK null kernel resolution differs");
      const auto &construction=data.construction;
      const auto &expected=fixture.at("construction_images").at(image);
      const std::array<std::uint32_t,12> identities{
          construction.singleton_address,construction.speaker_override_address,
          construction.speaker_config_address,construction.busy_address,construction.device_vtable,
          construction.device_data_vtable,construction.buffer_base_vtable,construction.buffer_vtable,
          construction.voice_base_vtable,construction.voice_vtable,construction.plain_reference_method,
          construction.synchronized_reference_method};
      for(unsigned i=0;i<identities.size();++i)
        if(identities[i]!=std::stoul(expected.at("addresses").at(i).get<std::string>(),nullptr,16))
          throw FormatError("Selected original SDK construction identity differs");
      for(unsigned i=0;i<8;++i)
        if(construction.references[i].vtable_identity!=std::stoul(expected.at("references").at(i).at("vtable").get<std::string>(),nullptr,16)||
            construction.references[i].method_identity!=std::stoul(expected.at("references").at(i).at("method").get<std::string>(),nullptr,16))
          throw FormatError("Selected original SDK AddRef binding differs");
      const auto listener=unhex(ranges.at(28).at("hex")),environment=unhex(ranges.at(29).at("hex"));
      for(unsigned i=0;i<15;++i)if(construction.listener_defaults[i]!=word(listener,i*4))
        throw FormatError("Selected original SDK listener default differs");
      for(unsigned i=0;i<12;++i)if(construction.environment_defaults[i]!=word(environment,i*4))
        throw FormatError("Selected original SDK environment default differs");
      const auto &init=data.voice_initialization;
      const auto &initialization=fixture.at("voice_initialization_images").at(image);
      const auto source_defaults=unhex(ranges.at(30).at("hex")),environment_defaults=unhex(ranges.at(31).at("hex"));
      for(unsigned i=0;i<18;++i)if(init.source_defaults[i]!=word(source_defaults,i*4))
        throw FormatError("Original SDK source initialization default differs");
      for(unsigned i=0;i<9;++i)if(init.environment_defaults[i]!=word(environment_defaults,i*4))
        throw FormatError("Original SDK source environment default differs");
      const auto pcm_guid=unhex(ranges.at(33).at("hex")),adpcm_guid=unhex(ranges.at(32).at("hex"));
      for(unsigned i=0;i<4;++i)if(init.pcm_guid[i]!=word(pcm_guid,i*4)||init.adpcm_guid[i]!=word(adpcm_guid,i*4))
        throw FormatError("Original SDK format GUID differs");
      const std::array<std::uint32_t,4> init_identities{init.default_format_identity,init.spatial_bins_identity,
          init.parent_bins_identity,init.channel_bins_table_identity};
      for(unsigned i=0;i<4;++i)if(init_identities[i]!=std::stoul(initialization.at("identities").at(i).get<std::string>(),nullptr,16))
        throw FormatError("Original SDK initialization identity differs");
      if(init.unsigned_correction_bits!=voice.unsigned_correction_bits||
          init.frequency_reciprocal_bits!=word(unhex(ranges.at(35).at("hex"))))
        throw FormatError("Original SDK initialization numeric operand differs");
      if(init.immutable.size()!=initialization.at("regions").size())throw FormatError("Original SDK initialization region count differs");
      for(unsigned i=0;i<init.immutable.size();++i)
        if(init.immutable[i].identity!=std::stoul(initialization.at("regions").at(i).at("address").get<std::string>(),nullptr,16)||
            init.immutable[i].bytes!=unhex(initialization.at("regions").at(i).at("hex")))
          throw FormatError("Original SDK complete relocated initialization data differs");
      if(data.hardware_voice.voice_mapping_pointer_address!=std::stoul(
          fixture.at("hardware_voice_images").at(image).at("voice_mapping_pointer_address").get<std::string>(),nullptr,16))
        throw FormatError("Original SDK hardware voice mapping binding differs");
      const auto &dispatch=data.voice_dispatch;const auto &format=fixture.at("voice_dispatch_images").at(image);
      if(dispatch.base_format_method!=std::stoul(format.at("base_format_method").get<std::string>(),nullptr,16)||
          dispatch.reconfigure_method!=std::stoul(format.at("reconfigure_method").get<std::string>(),nullptr,16))
        throw FormatError("Original SDK voice format owner differs");
      for(unsigned i=0;i<dispatch.format_methods.size();++i)
        if(dispatch.format_methods[i].vtable_identity!=std::stoul(format.at("format_methods").at(i).at("vtable").get<std::string>(),nullptr,16)||
            dispatch.format_methods[i].method_identity!=std::stoul(format.at("format_methods").at(i).at("method").get<std::string>(),nullptr,16))
          throw FormatError("Original SDK voice virtual format binding differs");
      const auto &pages=fixture.at("sample_page_images").at(image);
      if(data.sample_pages.vtable_identity!=std::stoul(pages.at("vtable_identity").get<std::string>(),nullptr,16)||
          data.sample_pages.available_count_address!=std::stoul(pages.at("available_count_address").get<std::string>(),nullptr,16))
        throw FormatError("Original SDK sample-page manager binding differs");
      if(data.voice_attachment.stream_samples_pointer_address!=std::stoul(
          fixture.at("voice_attachment_images").at(image).at("stream_samples_pointer_address").get<std::string>(),nullptr,16))
        throw FormatError("Original SDK streamed voice backing binding differs");
      const auto &notifications=fixture.at("notification_images").at(image);
      if(data.notifications.voice_vtable!=std::stoul(notifications.at("voice_vtable").get<std::string>(),nullptr,16)||
          data.notifications.arm_method!=std::stoul(notifications.at("arm_method").get<std::string>(),nullptr,16)||
          data.notifications.cancel_method!=std::stoul(notifications.at("cancel_method").get<std::string>(),nullptr,16))
        throw FormatError("Original SDK voice notification binding differs");
      const auto &retirement=fixture.at("retirement_images").at(image);
      const std::array<std::uint32_t,7> actual_retirement{data.voice_retirement.group_register_table,
          data.voice_retirement.force_counter_address,data.voice_retirement.slot_mapping_pointer_address,
          data.voice_retirement.voice_mapping_pointer_address,data.voice_retirement.voice_vtable,
          data.voice_retirement.status_method,data.voice_retirement.arm_method};
      const std::array<const char *,7> retirement_fields{"group_register_table","force_counter_address",
          "slot_mapping_pointer_address","voice_mapping_pointer_address","voice_vtable","status_method","arm_method"};
      for(unsigned i=0;i<actual_retirement.size();++i)
        if(actual_retirement[i]!=std::stoul(retirement.at(retirement_fields[i]).get<std::string>(),nullptr,16))
          throw FormatError("Original SDK voice retirement binding differs");
      for (const auto invalid:{0u,c.kernel_table_a-32,c.kernel_table_a+1,
                               c.kernel_table_a+std::uint32_t(data.kernels.size()),data.null_kernel_identity+1}) {
        bool rejected=false;
        try {(void)data.kernel(invalid);} catch (const std::out_of_range &) {rejected=true;}
        if (!rejected) throw FormatError("Unavailable original kernel identity admitted");
      }
    }
    std::cout<<"PASS three owned image SDK imports: all scalar/basis words,61 complete stereo kernels,"
               "null kernel and bounded identity resolution; no live playback/DSP claim\n";
  } catch (const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
