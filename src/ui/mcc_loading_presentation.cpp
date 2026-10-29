#include "halo2/mcc_loading_presentation.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/mcc_loading_inputs.hpp"
#include "halo2/native_vista_loading_audio.hpp"
#include "halo2/native_mcc_loading_audio.hpp"
#include "halo2/texture.hpp"
#include "halo2/ui_dds.hpp"
#include "halo2/mcc_loading_converter_metadata.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <bit>
#include <cmath>
#include <vector>

namespace halo2 {
namespace {
using Json=nlohmann::json;
std::vector<std::byte> read(const std::filesystem::path& path,std::size_t limit) {
  if(std::filesystem::is_symlink(path) || !std::filesystem::is_regular_file(path))
    throw FormatError("MCC loading asset has no private regular owner: "+path.string());
  std::ifstream input(path,std::ios::binary|std::ios::ate);
  const auto size=input.tellg();
  if(!input || size<=0 || std::uint64_t(size)>limit)throw FormatError("MCC loading asset extent is invalid");
  std::vector<std::byte> result(static_cast<std::size_t>(size));input.seekg(0);
  if(!input.read(reinterpret_cast<char*>(result.data()),size))throw FormatError("Incomplete MCC loading asset");
  return result;
}
std::string digest(std::span<const std::byte> bytes) {
  std::array<unsigned char,32> value{};
  SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),value.data());
  std::string result;for(auto c:value){result+="0123456789abcdef"[c>>4];result+="0123456789abcdef"[c&15];}
  return result;
}
std::vector<std::byte> asset(const std::filesystem::path& bank,const Json& metadata,
                            const char* name,std::size_t limit) {
  const auto bytes=read(bank/name,limit);const auto& record=metadata.at("files").at(name);
  if(record.at("bytes")!=bytes.size() || record.at("sha256")!=digest(bytes))
    throw FormatError("MCC loading asset differs from its converted owner: "+std::string(name));
  return bytes;
}
constexpr std::size_t mcc_frames=2744320,mcc_pcm_bytes=mcc_frames*8;
constexpr const char* mcc_pcm_sha="4170c17c50ed19f4fd1fcd292431a78564c139ff6f1324e3ea56fdefcac34d6e";
constexpr const char* mcc_media_sha="7ecbdce187ffda00077a593db8ef65a5e9e5ef72d0cd3d4474c523246f7b1695";
constexpr const char* mcc_codebook_sha="00a93eab267d281401b1efd54e888a2e183299b9e6c446c48d09f701a89d9d27";
constexpr const char* mcc_policy="full-decoded-track-once-then-silence-until-end";
void private_path(const std::filesystem::path& path) {
  // Normalizing link/.. would inspect a different owner from the original
  // path subsequently opened. Reject parent traversal before normalization.
  for(const auto& part:path)
    if(part=="..")throw FormatError("MCC V3 loading path contains parent traversal");
  std::filesystem::path current;
  for(const auto& part:std::filesystem::absolute(path).lexically_normal()) {
    current/=part;
    if(std::filesystem::is_symlink(current))throw FormatError("MCC V3 loading path has a linked owner");
  }
}
bool hex(const Json& value,std::size_t count) {
  if(!value.is_string())return false;
  const auto& text=value.get_ref<const std::string&>();
  return text.size()==count&&text.find_first_not_of("0123456789abcdef")==std::string::npos;
}
bool natural(const Json& value,bool positive=false) {
  return value.is_number_integer()&&value.get<std::int64_t>()>=(positive?1:0);
}
void v3_profile(const Json& value) {
  if(value.at("schema")!="H2_MCC_NATIVE_LOADING_V3" || value.at("profile")!="mcc-logo-glow-mcc-track-oneshot-v1" ||
      value.at("complete")!=true || !value.at("files").is_object() || value.at("files").size()!=2 ||
      !value.at("files").contains("glow-mask.dds") || !value.at("files").contains("loading.wav"))
    throw FormatError("Unsupported MCC V3 loading profile; V2 is not a fallback");
  const auto& mask=value.at("mask");const auto& audio=value.at("audio");
  if(mask.at("path")!="glow-mask.dds" || mask.at("source_width")!=512 || mask.at("source_height")!=512 ||
      mask.at("authored_reconstruction")!=true || mask.at("exact_vista_pixels")!=false ||
      audio.at("path")!="loading.wav" || audio.at("encoding")!="ieee-float32-le" || audio.at("channels")!=2 ||
      audio.at("sample_rate")!=44100 || audio.at("decoded_frames")!=mcc_frames ||
      audio.at("decoded_pcm_sha256")!=mcc_pcm_sha || audio.at("native_policy")!=mcc_policy ||
      audio.at("authored_frontend_timing")!=false)
    throw FormatError("MCC V3 loading mask/audio format or native policy differs");
  const auto& source=audio.at("source");const auto& archive=source.at("archive");
  if(source.at("path")!="MCC/Content/WwiseAudio/Windows/236657329.wem" || source.at("sha256")!=mcc_media_sha ||
      source.at("event")!="Play_Music" || source.at("media_id")!=236657329 ||
      source.at("switch_group")!=2949940656u || source.at("switch_key")!=1792793154 ||
      archive.at("path")!="MCC/Content/Paks/MCC-WindowsNoEditor.pak" ||
      !hex(archive.at("index_sha1"),40) || !hex(archive.at("entry_sha1"),40) ||
      source.at("pak_index_sha1")!=archive.at("index_sha1"))
    throw FormatError("MCC V3 loading media/archive identity differs");
  for(const auto* key:{"bytes","mtime_ns","index_offset","index_bytes","entry_offset","entry_bytes"})
    if(!natural(archive.at(key),std::string_view(key)=="bytes" || std::string_view(key)=="index_bytes" ||
        std::string_view(key)=="entry_bytes"))throw FormatError("MCC V3 loading archive extent is invalid");
  const auto bytes=archive.at("bytes").get<std::uint64_t>();
  const auto index=archive.at("index_offset").get<std::uint64_t>(),index_bytes=archive.at("index_bytes").get<std::uint64_t>();
  const auto entry=archive.at("entry_offset").get<std::uint64_t>(),entry_bytes=archive.at("entry_bytes").get<std::uint64_t>();
  if(bytes<61 || index>bytes-61 || index_bytes>bytes-61-index || entry>bytes || entry_bytes>bytes-entry)
    throw FormatError("MCC V3 loading archive extents escape the recorded owner");
  for(const auto* role:{"converter","codebook"}) {
    const auto& row=audio.at(role);const auto path=std::filesystem::path(row.at("path").get<std::string>());
    if(path.empty() || path.is_absolute() || !natural(row.at("bytes"),true) || !hex(row.at("sha256"),64))
      throw FormatError("MCC V3 loading decoder provenance is incomplete");
    for(const auto& part:path)if(part=="..")throw FormatError("MCC V3 loading decoder path escapes its owner");
  }
  if(audio.at("codebook").at("path")!="external/ww2ogg/packed_codebooks_aoTuV_603.bin" ||
      audio.at("codebook").at("sha256")!=mcc_codebook_sha)
    throw FormatError("MCC V3 loading codebook identity differs");
  validate_mcc_loading_converter_metadata(audio);
  for(const auto& row:value.at("files"))
    if(!row.is_object() || row.size()!=2 || !natural(row.at("bytes"),true) || !hex(row.at("sha256"),64))
      throw FormatError("MCC V3 loading file identity is invalid");
}
std::uint32_t word(std::span<const std::byte> data,std::size_t at,unsigned count=4) {
  std::uint32_t result{};
  for(unsigned n=0;n<count;++n)result|=std::to_integer<std::uint32_t>(data[at+n])<<(n*8);
  return result;
}
std::vector<float> v3_pcm(std::span<const std::byte> data) {
  const auto text=[&](std::size_t at,const char* value,std::size_t count) {
    return std::equal(data.begin()+at,data.begin()+at+count,reinterpret_cast<const std::byte*>(value));
  };
  if(data.size()!=44+mcc_pcm_bytes || !text(0,"RIFF",4) || word(data,4)!=mcc_pcm_bytes+36 ||
      !text(8,"WAVEfmt ",8) || word(data,16)!=16 || word(data,20,2)!=3 || word(data,22,2)!=2 ||
      word(data,24)!=44100 || word(data,28)!=44100*8 || word(data,32,2)!=8 || word(data,34,2)!=32 ||
      !text(36,"data",4) || word(data,40)!=mcc_pcm_bytes)
    throw FormatError("MCC V3 loading requires canonical stereo44100Hz IEEE-F32 WAV");
  if(digest(data.subspan(44))!=mcc_pcm_sha)throw FormatError("MCC V3 loading PCM differs from its authenticated source");
  std::vector<float> result(mcc_frames*2);
  for(std::size_t n=0;n<result.size();++n) {
    result[n]=std::bit_cast<float>(word(data,44+n*4));
    if(!std::isfinite(result[n]))throw FormatError("MCC V3 loading PCM contains nonfinite samples");
  }
  return result;
}
// H2EK loading_dx11.bin keeps the original PC normal-mode loading slots as
// plain DDS extents (flags0x1007, no linear size), which the UI DDS reader
// rejects. Their exact header is checked here; BC decode reuses the MCC path.
constexpr std::array<unsigned,6> h2ek_texture_slots{0,1,3,4,5,11};
VistaLoadingImage h2ek_texture(std::span<const std::byte> dds,const Json& row) {
  const auto width=row.at("width").get<std::uint32_t>(),height=row.at("height").get<std::uint32_t>();
  const auto fourcc=row.at("fourcc").get<std::string>();
  if((fourcc!="DXT1" && fourcc!="DXT3") || !width || !height || width>4096 || height>4096)
    throw FormatError("Unsupported H2EK loading texture format");
  const bool bc1=fourcc=="DXT1";
  const std::size_t blocks=std::size_t((width+3)/4)*((height+3)/4)*(bc1?8:16);
  if(dds.size()!=128+blocks)throw FormatError("H2EK loading texture extent differs from its original bank");
  bool reserved=true;
  for(std::size_t at:{20u,24u,28u,88u,92u,96u,100u,104u,112u,116u,120u,124u})reserved=reserved && !word(dds,at);
  for(std::size_t at=32;at<76;at+=4)reserved=reserved && !word(dds,at);
  if(word(dds,0)!=0x20534444u || word(dds,4)!=124 || word(dds,8)!=0x1007 ||
     word(dds,12)!=height || word(dds,16)!=width || word(dds,76)!=32 || word(dds,80)!=4 ||
     word(dds,84)!=(bc1?0x31545844u:0x33545844u) || word(dds,108)!=0x1000 || !reserved)
    throw FormatError("H2EK loading texture header differs from its original bank");
  Texture texture;texture.width=width;texture.height=height;texture.format=bc1?14:15;
  texture.pixels=TexturePixels(dds.subspan(128));texture.mips={TextureMip{width,height,0,blocks}};
  auto image=prepare_xemu_bc_texture(texture);
  if(image.format!=11 || image.width!=width || image.height!=height || image.layers!=1 || image.depth || image.pixels.empty())
    throw FormatError("H2EK loading texture storage conversion is unsupported");
  return {width,height,std::move(image)};
}
}
bool configure_h2ek_original_loading_presentation(DesktopSession& desktop,
    const std::filesystem::path& imported,bool mute)try {
  const auto bank=imported/"presentation/original-loading-v2";
  if(!std::filesystem::exists(std::filesystem::symlink_status(bank)))return false;
  private_path(bank);
  const auto raw=read(bank/"manifest.json",65536);
  const auto metadata=Json::parse(reinterpret_cast<const char*>(raw.data()),reinterpret_cast<const char*>(raw.data()+raw.size()));
  const auto& audio=metadata.at("audio");const auto& textures=metadata.at("textures");
  if(metadata.at("schema")!="H2_H2EK_ORIGINAL_LOADING_V2" || metadata.at("complete")!=true ||
     metadata.at("runtime_activation")!=true || metadata.at("pe_executed")!=false ||
     !metadata.at("files").is_object() || metadata.at("files").size()!=1+h2ek_texture_slots.size() ||
     audio.at("path")!="ambience.xbox-adpcm" || !textures.is_array() || textures.size()!=h2ek_texture_slots.size())
    throw FormatError("Unsupported H2EK original loading profile");
  VistaLoadingAssets assets;
  for(unsigned i=0;i<h2ek_texture_slots.size();++i) {
    const auto& row=textures.at(i);
    const auto slot=h2ek_texture_slots[i];
    const auto path=row.at("path").get<std::string>();
    if(row.at("slot")!=slot || path!=(slot<10?"loading-0":"loading-")+std::to_string(slot)+".dds")
      throw FormatError("H2EK original loading texture slots differ");
    const auto bytes=asset(bank,metadata,path.c_str(),8u*1024u*1024u);
    if(row.at("sha256")!=digest(bytes))throw FormatError("H2EK loading texture differs from its manifest: "+path);
    assets[slot]=h2ek_texture(bytes,row);
  }
  // Same exact tone and decode checks as the earlier bundled-original bank.
  const auto encoded=asset(bank,metadata,"ambience.xbox-adpcm",18396);
  if(encoded.size()!=18396 || digest(encoded)!="4f61e827be9b54b71e3d37a145c9b88fb32eaf517e23746af85236ca41ba8fc9")
    throw FormatError("H2EK loading ambience differs from the original tone");
  const auto tone=std::span(reinterpret_cast<const std::uint8_t*>(encoded.data()),encoded.size());
  const auto pcm=vista_loading_audio_decode(tone);
  if(pcm.size()!=65408 || digest(std::as_bytes(std::span(pcm)))!="b740acf4ce447da6583b4f6c7fd6696fabbb249664a53fb780b18c4dd58cd73d")
    throw FormatError("H2EK loading ambience decoder differs");
  auto voice=std::make_shared<NativeVistaLoadingAudio>(tone);
  desktop.configure_loading(assets,true);
  desktop.set_loading_audio_services({[voice,mute]{
      voice->begin(mute);
      const auto state=voice->snapshot();
      if(!mute && (!state.device_open || !state.failure.empty() || state.frequencies!=vista_loading_audio_frequencies)) {
        voice->end();throw FormatError("H2EK loading ambience could not start: "+state.failure);
      }
      std::cout<<"Original loading audio: H2EK four-voice ambience started"<<(mute?" (muted)":"")<<"\n";
    },[voice](std::uint32_t gain){voice->gain(gain);},[voice]{
      voice->end();std::cout<<"Original loading audio: stopped after graphics join\n";
    }});
  std::cout<<"Original loading: H2EK logo/sweep art and four-voice ambience admitted\n";
  return true;
}catch(const Json::exception& error) {
  throw FormatError("Invalid H2EK original loading manifest: "+std::string(error.what()));
}
void configure_mcc_loading_presentation(DesktopSession& desktop,const Cache& menu,
    const std::filesystem::path& imported,bool mute) {
  if(!menu.is_mcc())throw FormatError("Native MCC loading requires an imported MCC menu");
  const auto bank=imported/"presentation/loading-v2";
  if(std::filesystem::is_symlink(bank))throw FormatError("MCC loading bank must have a private owner");
  const auto raw=read(bank/"manifest.json",65536);
  const auto metadata=Json::parse(reinterpret_cast<const char*>(raw.data()),reinterpret_cast<const char*>(raw.data()+raw.size()));
  if(metadata.at("schema")!="H2_MCC_NATIVE_LOADING_V2" || metadata.at("profile")!="mcc-logo-glow-original-tone-v1" ||
     metadata.at("complete")!=true || metadata.at("mask").at("path")!="glow-mask.dds" ||
     metadata.at("audio").at("path")!="loading-tone.xbox-adpcm" ||
     metadata.at("audio").at("original_tone_bytes")!=true)
    throw FormatError("Unsupported MCC loading presentation profile");
  VistaLoadingAssets assets;
  for(auto& input:load_mcc_loading_bitmap_inputs(menu))assets.at(input.texture_slot)=std::move(input.image);
  const auto mask=asset(bank,metadata,"glow-mask.dds",128+512*512*4);
  assets[11]={512,512,read_ui_dds_texture(Bytes(mask))};
  if(assets[11].texture.width!=512 || assets[11].texture.height!=512 || assets[11].texture.format!=11)
    throw FormatError("MCC loading reconstruction shape changed");
  const auto encoded=asset(bank,metadata,"loading-tone.xbox-adpcm",18396);
  if(encoded.size()!=18396 || digest(encoded)!="4f61e827be9b54b71e3d37a145c9b88fb32eaf517e23746af85236ca41ba8fc9")
    throw FormatError("Bundled loading ambience differs from the original tone");
  const auto tone=std::span(reinterpret_cast<const std::uint8_t*>(encoded.data()),encoded.size());
  const auto pcm=vista_loading_audio_decode(tone);
  if(pcm.size()!=65408 || digest(std::as_bytes(std::span(pcm)))!="b740acf4ce447da6583b4f6c7fd6696fabbb249664a53fb780b18c4dd58cd73d")
    throw FormatError("Original loading ambience decoder differs");
  auto voice=std::make_shared<NativeVistaLoadingAudio>(tone);
  desktop.configure_loading(assets,true);
  desktop.set_loading_audio_services({[voice,mute]{
      voice->begin(mute);
      const auto state=voice->snapshot();
      if(!mute && (!state.device_open || !state.failure.empty() || state.frequencies!=vista_loading_audio_frequencies)) {
        voice->end();throw FormatError("Original loading ambience could not start: "+state.failure);
      }
      std::cout<<"MCC loading audio: original four-voice ambience started"<<(mute?" (muted)":"")<<"\n";
    },[voice](std::uint32_t gain){voice->gain(gain);},[voice]{
      voice->end();std::cout<<"MCC loading audio: stopped after graphics join\n";
    }});
  std::cout<<"Legacy V2 loading reference: MCC artwork reconstruction and original ambience; not MCC-only runtime admission\n";
}
void configure_mcc_loading_presentation_v3(DesktopSession& desktop,const Cache& menu,
    const std::filesystem::path& imported,bool mute)try {
  if(!menu.is_mcc())throw FormatError("Native MCC V3 loading requires an imported MCC menu");
  const auto bank=imported/"presentation/loading-v3";private_path(bank);
  const auto raw=read(bank/"manifest.json",65536);
  const auto metadata=Json::parse(reinterpret_cast<const char*>(raw.data()),reinterpret_cast<const char*>(raw.data()+raw.size()));
  v3_profile(metadata);
  const auto encoded=asset(bank,metadata,"loading.wav",44+mcc_pcm_bytes);
  const auto pcm=v3_pcm(encoded);
  const auto mask=asset(bank,metadata,"glow-mask.dds",128+512*512*4);
  VistaLoadingAssets assets;
  for(auto& input:load_mcc_loading_bitmap_inputs(menu))assets.at(input.texture_slot)=std::move(input.image);
  assets[11]={512,512,read_ui_dds_texture(Bytes(mask))};
  if(mask.size()!=128+512*512*4 || assets[11].texture.width!=512 || assets[11].texture.height!=512 || assets[11].texture.format!=11)
    throw FormatError("MCC V3 loading reconstruction shape changed");
  auto voice=std::make_shared<NativeMccLoadingAudio>(NativeMccLoadingPcm{pcm,mcc_frames,44100,2});
  DesktopLoadingAudioServices services{[voice,mute] {
    voice->begin(mute);const auto state=voice->snapshot();
    if(!mute&&(!state.device_open || !state.stream_open || !state.failure.empty())) {
      voice->end();throw FormatError("MCC loading track could not start: "+state.failure);
    }
    std::cout<<"MCC V3 loading audio: genuine track started with finite native one-shot policy"<<(mute?" (muted)":"")<<"\n";
  },[voice](std::uint32_t gain){voice->gain(gain);},[voice] {
    voice->end();std::cout<<"MCC V3 loading audio: stopped after graphics join\n";
  }};
  desktop.configure_loading(assets,true);
  desktop.set_loading_audio_services(std::move(services));
  std::cout<<"MCC V3 loading: genuine PCM and reconstructed artwork admitted; authored loop/transition timing unqualified\n";
}catch(const Json::exception& error) {
  throw FormatError("Invalid MCC V3 loading manifest: "+std::string(error.what()));
}
}
