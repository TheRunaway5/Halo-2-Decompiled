#include "halo2/cache.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/mcc_loading_presentation.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

namespace {
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw halo2::FormatError(message);}
void write(const std::filesystem::path& path,std::span<const unsigned char> bytes) {
  std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
  check(bool(output),"Cannot write private loading component fixture");
}
std::string sha(std::span<const unsigned char> bytes) {
  std::array<unsigned char,SHA256_DIGEST_LENGTH> digest{};SHA256(bytes.data(),bytes.size(),digest.data());
  std::string result;
  for(auto byte:digest){result+="0123456789abcdef"[byte>>4];result+="0123456789abcdef"[byte&15];}
  return result;
}
void metadata(const std::filesystem::path& root,const Json& value) {
  std::ofstream output(root/"presentation/loading-v3/manifest.json");output<<value.dump();
  check(bool(output),"Cannot write private V3 loading manifest");
}
Json synthetic_metadata() {
  // These records exercise rejection routing only. They never authenticate a
  // real media payload, which has its own fixed source PCM digest below.
  Json source={{"path","MCC/Content/WwiseAudio/Windows/236657329.wem"},
    {"sha256","7ecbdce187ffda00077a593db8ef65a5e9e5ef72d0cd3d4474c523246f7b1695"},
    {"event","Play_Music"},{"media_id",236657329},{"switch_group",2949940656u},{"switch_key",1792793154},
    {"pak_index_sha1",std::string(40,'0')},
    {"archive",{{"path","MCC/Content/Paks/MCC-WindowsNoEditor.pak"},{"bytes",4096},{"mtime_ns",0},
      {"index_offset",100},{"index_bytes",200},{"entry_offset",0},{"entry_bytes",10},
      {"index_sha1",std::string(40,'0')},{"entry_sha1",std::string(40,'0')}}}};
  return {{"schema","H2_MCC_NATIVE_LOADING_V3"},{"profile","mcc-logo-glow-mcc-track-oneshot-v1"},{"complete",true},
    {"files",{{"glow-mask.dds",{{"bytes",128+512*512*4},{"sha256",std::string(64,'0')}}},
      {"loading.wav",{{"bytes",44+2744320*8},{"sha256",std::string(64,'0')}}}}},
    {"mask",{{"path","glow-mask.dds"},{"source_width",512},{"source_height",512},
      {"authored_reconstruction",true},{"exact_vista_pixels",false}}},
    {"audio",{{"path","loading.wav"},{"encoding","ieee-float32-le"},{"channels",2},{"sample_rate",44100},
      {"decoded_frames",2744320},{"decoded_pcm_sha256","4170c17c50ed19f4fd1fcd292431a78564c139ff6f1324e3ea56fdefcac34d6e"},
      {"native_policy","full-decoded-track-once-then-silence-until-end"},{"authored_frontend_timing",false},{"source",source},
      {"converter",{{"path","bin/halo2_mcc_loading_audio"},{"bytes",1},{"sha256",std::string(64,'0')}}},
      {"codebook",{{"path","external/ww2ogg/packed_codebooks_aoTuV_603.bin"},{"bytes",1},
        {"sha256","00a93eab267d281401b1efd54e888a2e183299b9e6c446c48d09f701a89d9d27"}}}}}};
}
void rejects(const halo2::Cache& menu,const std::filesystem::path& root,std::string_view reason) {
  halo2::DesktopSession desktop;bool rejected{};
  try{halo2::configure_mcc_loading_presentation_v3(desktop,menu,root,true);}
  catch(const halo2::FormatError& error){rejected=std::string_view(error.what()).find(reason)!=std::string_view::npos;}
  check(rejected,"V3 loading rejection did not reach its intended admission guard");
  check(!desktop.loading_snapshot()&&!desktop.loading_active(),"Rejected V3 loading bank published a partial bridge");
}
void v3_controls(const halo2::Cache& menu) {
  const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");
  check(scratch&&*scratch,"Loading component controls require owned HALO2_TEST_SCRATCH");
  const auto root=std::filesystem::path(scratch)/"mcc-loading-v3-controls";
  check(std::filesystem::create_directory(root),"Private V3 loading fixture already exists");
  struct Cleanup {std::filesystem::path root;~Cleanup(){std::error_code ignored;std::filesystem::remove_all(root,ignored);}} cleanup{root};
  std::filesystem::create_directories(root/"presentation/loading-v2");
  std::ofstream(root/"presentation/loading-v2/reference.txt")<<"preserved V2 evidence";
  rejects(menu,root,"regular owner"); // A V2 bank never supplies the V3 role.
  const auto bank=root/"presentation/loading-v3";std::filesystem::create_directory(bank);
  auto value=synthetic_metadata();value["schema"]="H2_MCC_NATIVE_LOADING_V2";metadata(root,value);
  rejects(menu,root,"V2 is not a fallback");
  value=synthetic_metadata();value["audio"]["path"]="loading-tone.xbox-adpcm";metadata(root,value);
  rejects(menu,root,"format or native policy");
  value=synthetic_metadata();value["audio"]["source"]["sha256"]=std::string(64,'0');metadata(root,value);
  rejects(menu,root,"media/archive identity");
  value=synthetic_metadata();value["audio"]["authored_frontend_timing"]=true;metadata(root,value);
  rejects(menu,root,"format or native policy");
  value=synthetic_metadata();
  value["audio"]["converter"]["runtime"]={{"schema","foreign closure"},{"files",Json::object()},
    {"manifest",{{"path","resources/mcc-loading-converter.json"},{"bytes",1},{"sha256",std::string(64,'0')}}}};
  metadata(root,value);rejects(menu,root,"packaged converter provenance");
  value=synthetic_metadata();
  const Json file={{"bytes",1},{"sha256",std::string(64,'0')}};
  value["audio"]["converter"]["runtime"]={{"schema","H2_MCC_LOADING_CONVERTER_RUNTIME_V1"},
    {"manifest",{{"path","resources/mcc-loading-converter.json"},{"bytes",1},{"sha256",std::string(64,'0')}}},
    {"files",{{"bin/halo2_mcc_loading_audio",file},{"bin/halo2_mcc_loading_audio.real",file},
      {"lib/ld-linux-x86-64.so.2",file},
      {"external/ww2ogg/packed_codebooks_aoTuV_603.bin",{{"bytes",1},
        {"sha256","00a93eab267d281401b1efd54e888a2e183299b9e6c446c48d09f701a89d9d27"}}}}}};
  const auto valid_runtime=value;
  metadata(root,value);rejects(menu,root,"regular owner"); // Valid profile reaches the absent WAV owner.
  for(const auto& bad:{std::string("lib/"),std::string("lib/a\nb"),std::string("lib/a\0b",7)}) {
    value=valid_runtime;
    value["audio"]["converter"]["runtime"]["files"][bad]=file;
    metadata(root,value);rejects(menu,root,"packaged converter file identity");
  }
  std::vector<unsigned char> wav(44+2744320*8);
  const auto text=[&](std::size_t at,const char* value,std::size_t size){std::copy_n(value,size,wav.begin()+at);};
  const auto word=[&](std::size_t at,std::uint32_t value,unsigned size=4){for(unsigned n=0;n<size;++n)wav[at+n]=value>>(n*8);};
  text(0,"RIFF",4);word(4,wav.size()-8);text(8,"WAVEfmt ",8);word(16,16);word(20,3,2);word(22,2,2);
  word(24,44100);word(28,44100*8);word(32,8,2);word(34,32,2);text(36,"data",4);word(40,wav.size()-44);
  write(bank/"loading.wav",wav);value=synthetic_metadata();value["files"]["loading.wav"]["sha256"]=sha(wav);metadata(root,value);
  rejects(menu,root,"PCM differs from its authenticated source");
  word(24,48000);write(bank/"loading.wav",wav);value["files"]["loading.wav"]["sha256"]=sha(wav);metadata(root,value);
  rejects(menu,root,"canonical stereo44100Hz");
  std::filesystem::rename(bank,root/"moved-bank");std::filesystem::create_directory_symlink(root/"moved-bank",bank);
  rejects(menu,root,"linked owner");
  const auto a=root/"traversal-a",b=root/"traversal-b";
  std::filesystem::create_directories(a/"cache/presentation/loading-v3");
  std::filesystem::create_directories(b/"subdir");
  std::filesystem::create_directories(b/"cache/presentation/loading-v3");
  std::filesystem::create_directory_symlink(b/"subdir",a/"link");
  metadata(a/"cache",synthetic_metadata());metadata(b/"cache",value);
  write(b/"cache/presentation/loading-v3/loading.wav",wav);
  const auto traversal=a/"link/../cache";
  check(std::filesystem::equivalent(traversal/"presentation/loading-v3/manifest.json",
      b/"cache/presentation/loading-v3/manifest.json")&&
      !std::filesystem::equivalent(traversal/"presentation/loading-v3/manifest.json",
        a/"cache/presentation/loading-v3/manifest.json"),
      "Private link/parent traversal fixture did not reach a different actual owner");
  rejects(menu,traversal,"parent traversal");
  check(std::filesystem::is_regular_file(root/"presentation/loading-v2/reference.txt"),"V3 controls modified legacy evidence");
  std::cout<<"PASS MCC V3 loading admission controls: no legacy fallback, strict profile/source/one-shot policy, exact PCM hash and format, linked-owner and link/parent-traversal rejection; no partial bridge\n";
}
void admitted_v3(const halo2::Cache& menu,const std::filesystem::path& root) {
  halo2::DesktopSession desktop;
  halo2::configure_mcc_loading_presentation_v3(desktop,menu,root,true);
  check(desktop.loading_snapshot().has_value()&&!desktop.loading_active(),"V3 loading admission lost its idle bridge");
  bool duplicate{};
  try{halo2::configure_mcc_loading_presentation_v3(desktop,menu,root,true);}catch(const halo2::FormatError&){duplicate=true;}
  check(duplicate&&!desktop.loading_active(),"Duplicate V3 loading configuration was accepted");
  std::cout<<"PASS genuine MCC V3 bank: authenticated stereo PCM and reconstructed mask, prepared NativeMccLoadingAudio callbacks and idle DesktopLoading bridge; graphics/device lifecycle remains separate\n";
}
}

int main(int argc,char** argv)try {
  if(argc<2)throw halo2::FormatError("Imported MCC cache required; optional --v3-cache explicit-bank-owner");
  std::filesystem::path cache(argv[1]);
  unsigned next=2;
  if(cache=="auto" && argc>=3) {
    next=3;
    const std::filesystem::path project=argv[2];
    std::ifstream source(project/"assets/mcc-source.tsv");
    std::string signature,key,path;std::getline(source,signature);
    if(signature!="H2_MCC_ASSET_SOURCE_V1")return 77;
    while(source>>key>>path)if(key=="cache")cache=project/path;
  }
  // Existing CTest invocations supply source-root for explicit caches too.
  // It is only used to resolve "auto", but remains a positional compatibility
  // argument before the new optional V3 bank selector.
  if(cache!="auto" && next==2 && argc>=3 && std::string_view(argv[2])!="--v3-cache")next=3;
  std::filesystem::path v3;
  if(unsigned(argc)>next) {
    if(unsigned(argc)!=next+2 || std::string_view(argv[next])!="--v3-cache")throw halo2::FormatError("Unexpected loading component options");
    v3=argv[next+1];
  }
  if(!std::filesystem::is_regular_file(cache/"halo2/h2_maps_win64_dx11/mainmenu.map"))return 77;
  const halo2::Cache menu(cache/"halo2/h2_maps_win64_dx11/mainmenu.map");
  v3_controls(menu);
  if(!v3.empty()){admitted_v3(menu,v3);return 0;} // Explicit V3 never depends on a V2 tone bank.
  else if(std::filesystem::is_regular_file(cache/"presentation/loading-v3/manifest.json"))admitted_v3(menu,cache);
  else std::cout<<"LIMIT genuine V3 positive bridge requires explicit prepared --v3-cache; rejection controls were exercised\n";
  if(!std::filesystem::is_regular_file(cache/"presentation/loading-v2/manifest.json"))return 0;
  halo2::DesktopSession desktop;
  // Failed admission must not publish a half-configured bridge.
  bool missing{};
  try {desktop.configure_loading(halo2::VistaLoadingAssets{});}
  catch(const halo2::FormatError&){missing=true;}
  if(!missing || desktop.loading_snapshot())throw halo2::FormatError("Partial loading bank was published");
  halo2::configure_mcc_loading_presentation(desktop,menu,cache,true);
  if(!desktop.loading_snapshot() || desktop.loading_active())throw halo2::FormatError("MCC loading admission lost its idle bridge");
  bool duplicate{};
  try {halo2::configure_mcc_loading_presentation(desktop,menu,cache,true);}
  catch(const halo2::FormatError&){duplicate=true;}
  if(!duplicate || desktop.loading_active())throw halo2::FormatError("Duplicate loading configuration accepted");
  std::cout<<"PASS legacy V2/reference bridge admission; original tone is not MCC-only runtime compliance\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
