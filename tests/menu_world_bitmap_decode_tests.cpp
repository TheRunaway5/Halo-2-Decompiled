#include "menu_bitmap_gpu_support.hpp"

namespace {
using namespace menu_bitmap_gpu;
struct Measurement {unsigned stage{},storage{},mip_faces{};std::size_t components{},different_words{},different_rounded_bytes{},original_roundtrip_bytes{};};
unsigned rounded(std::uint32_t word) {
  const float value=std::bit_cast<float>(word);require(std::isfinite(value) && value>=0 && value<=1,"GPU returned invalid UNORM value");
  return unsigned(std::lround(value*255.f));
}
}
int main(int argc,char **argv) {
  std::vector<Measurement> measurements;std::vector<Capability> capabilities;std::string device,failure,unavailable;
  bool output_created=false,available=false,bc=false,bc_enabled=false,raw=false;std::size_t total_words{},total_mips{},total_diff{};
  try {
    require(argc==5 || argc==6,"bitmap decode test needs mainmenu.map,1335fixture,compiled-compute-SPIRV,fresh-output,[--raw-compressed]");
    if(argc==6){require(std::string(argv[5])=="--raw-compressed","unknown bitmap decode test mode");raw=true;}
    if(!fs::exists(argv[1]) || !fs::exists(fs::path(argv[2])/"manifest.tsv") || !fs::exists(argv[3]))return 77;
    const auto expected=fixture(argv[2]);const Cache cache(argv[1]);auto textures=imports(cache,expected);
    if(!raw)for(auto &stage:textures)prepare_frontend_texture_storage(stage.texture);
    const fs::path out=argv[4];require(!fs::exists(out),"bitmap decode output must be fresh");fs::create_directories(out);output_created=true;
    {
      Gpu gpu;available=gpu.initialize(argv[3],textures);device=gpu.device_name;capabilities=gpu.capabilities;bc=gpu.texture_compression_bc;bc_enabled=gpu.compression_feature_enabled;unavailable=gpu.unavailable_reason;
      if(available)for(unsigned stage=0;stage<4;++stage) {
        const auto &t=textures[stage].texture;const auto &e=expected.stages[stage];const auto actual=gpu.run(t,e);
        Measurement m{stage,t.format,unsigned(t.mips.size()),actual.size()*4};Data native_words,original_words;
        native_words.resize(actual.size()*16);original_words.resize(actual.size()*16);
        unsigned first_logged{};std::size_t cursor{};
        for(unsigned level=0;level<e.mips;++level)for(unsigned face=0;face<e.layers;++face) {
          const auto &original=e.expected[face*e.mips+level];
          for(std::size_t pixel=0;pixel<std::size_t(original.width)*original.height;++pixel,++cursor)for(unsigned c=0;c<4;++c) {
            const auto &at=actual[cursor];m.different_words+=at.native[c]!=at.original[c];
            m.different_rounded_bytes+=rounded(at.native[c])!=rounded(at.original[c]);
            m.original_roundtrip_bytes+=rounded(at.original[c])!=std::to_integer<unsigned>(original.sampled[pixel*4+c]);
            if(at.native[c]!=at.original[c] && first_logged++<4)std::cerr<<"stage "<<stage<<" mip "<<level<<" face "<<face<<" texel "<<pixel<<" channel "<<c
                <<" differing float bits native0x"<<std::hex<<at.native[c]<<" original0x"<<at.original[c]<<std::dec<<'\n';
          }
        }
        require(cursor==actual.size(),"GPU float comparison did not cover every original mip/face");
        for(std::size_t i=0;i<actual.size();++i){std::memcpy(native_words.data()+i*16,actual[i].native.data(),16);std::memcpy(original_words.data()+i*16,actual[i].original.data(),16);}
        save(out/("stage-"+std::to_string(stage)+"-native-float-words.bin"),native_words);save(out/("stage-"+std::to_string(stage)+"-original-float-words.bin"),original_words);
        measurements.push_back(m);total_words+=m.components;total_mips+=m.mip_faces;total_diff+=m.different_words;
        std::cout<<"stage "<<stage<<" "<<format_name(native_format(t.format))<<": "<<m.components<<" component words, "<<m.different_words
            <<" float differences, "<<m.different_rounded_bytes<<" rounded byte differences\n";
      }
    } // Count validation and cleanup errors after all Vulkan destruction.
  } catch(const std::exception &error){failure=error.what();std::cerr<<failure<<'\n';}
  const bool skipped=!available && failure.empty() && !validation_errors && !cleanup_errors;
  bool complete=available && failure.empty() && measurements.size()==4 && total_words==3429580 && total_mips==91 && !validation_errors && !cleanup_errors;
  for(const auto &m:measurements)complete&=m.original_roundtrip_bytes==0;
  const bool exact=complete && total_diff==0;
  const bool success=complete && (raw || exact);
  if(output_created) {
    std::ofstream report(fs::path(argv[4])/"report.json");report<<"{\n  \"status\":"<<std::quoted(skipped?"skipped":(raw && complete?"diagnostic-complete":(exact?"passed":"failed")))
        <<",\n  \"mode\":"<<std::quoted(raw?"raw-hardware-compressed-diagnostic":"frontend-source-expanded")
        <<",\n  \"exact_float_match\":"<<(exact?"true":"false")
        <<",\n  \"device\":"<<std::quoted(device)<<",\n  \"textureCompressionBC\":"<<(bc?"true":"false")
        <<",\n  \"compression_feature_enabled\":"<<(bc_enabled?"true":"false")
        <<",\n  \"unavailable_reason\":"<<std::quoted(unavailable)<<",\n  \"mip_faces\":"<<total_mips<<",\n  \"RGBA_component_words\":"<<total_words
        <<",\n  \"differing_float_words\":"<<total_diff<<",\n  \"validation_errors_after_cleanup\":"<<validation_errors.load()<<",\n  \"cleanup_errors\":"<<cleanup_errors.load()
        <<",\n  \"failure\":"<<std::quoted(failure)<<",\n  \"format_capabilities\":[\n";
    for(unsigned i=0;i<capabilities.size();++i){const auto &c=capabilities[i];report<<"    {\"format\":"<<std::quoted(format_name(c.format))<<",\"VkFormat\":"<<c.format
        <<",\"optimal_features\":"<<c.optimal<<",\"actual_image_supported\":"<<(c.image_supported?"true":"false")<<"}"<<(i+1==capabilities.size()?"\n":",\n");}
    report<<"  ],\n  \"measurements\":[\n";
    for(unsigned i=0;i<measurements.size();++i){const auto &m=measurements[i];report<<"    {\"stage\":"<<m.stage<<",\"native_storage\":"<<m.storage
        <<",\"mip_faces\":"<<m.mip_faces<<",\"component_words\":"<<m.components<<",\"different_float_words\":"<<m.different_words
        <<",\"different_rounded_bytes\":"<<m.different_rounded_bytes<<",\"original_roundtrip_byte_differences\":"<<m.original_roundtrip_bytes<<"}"<<(i+1==measurements.size()?"\n":",\n");}
    report<<"  ],\n  \"scope\":\"Actual source-identified Cache/material imports through production frontend BC1/BC3 source expansion, or explicitly labeled raw hardware BC diagnostic; original mip/face extents and alpha semantics. GPU texelFetch float words versus independent original RGBA8 images. No filtering, cube-coordinate policy, live residency or native raster/frame admission.\"\n}\n";
    if(!report){std::cerr<<"cannot retain bitmap decode report\n";return 1;}
  }
  if(skipped){std::cerr<<"SKIP: "<<unavailable<<'\n';return 77;}
  if(!success)return 1;
  std::cout<<"3429580 GPU RGBA component words / 91 mip/faces; "<<(raw?"raw diagnostic":"frontend source-expanded candidate")
      <<", "<<total_diff<<" differing words; validation/cleanup0; texel decoding only\n";return 0;
}
