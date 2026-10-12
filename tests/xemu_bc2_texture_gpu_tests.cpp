#include "menu_bitmap_gpu_support.hpp"
#include "xemu_bc2_texture_support.hpp"

namespace {
using namespace menu_bitmap_gpu;
struct Measurement {
  std::string label;
  unsigned mip_faces{};
  std::size_t words{},different{},roundtrip{};
};
Stage expected_stage(const xemu_bc2_test::Image &image) {
  Stage stage;
  stage.width=image.source.width;stage.height=image.source.height;
  stage.layers=image.source.layers;stage.mips=image.levels;stage.source_format=15;
  std::size_t offset{};
  for(unsigned i=0;i<image.source.mips.size();++i) {
    const auto &m=image.source.mips[i];const auto size=std::size_t(m.width)*m.height*4;
    require(offset+size<=image.expected.size(),"original BC2 expected mip escapes image");
    Data rgba(image.expected.begin()+offset,image.expected.begin()+offset+size);
    stage.expected.push_back({i/image.levels,i%image.levels,m.width,m.height,rgba,rgba});
    offset+=size;
  }
  require(offset==image.expected.size(),"original BC2 expected image has trailing bytes");
  return stage;
}
unsigned byte(std::uint32_t word) {
  const float value=std::bit_cast<float>(word);
  require(std::isfinite(value) && value>=0 && value<=1,"invalid BC2 sampled UNORM");
  return unsigned(std::lround(value*255.f));
}
}
int main(int argc,char **argv) {
  std::string failure,device,unavailable;bool available=false,raw=false,created=false;
  std::vector<Measurement> rows;std::size_t words{},mips{},different{},roundtrip{};
  try {
    require(argc==4 || argc==5,"BC2 GPU test needs oracle-dir,compiled-SPIRV,fresh-output,[--raw-compressed]");
    if(argc==5){require(std::string(argv[4])=="--raw-compressed","unknown BC2 GPU mode");raw=true;}
    if(!fs::exists(fs::path(argv[1])/"manifest.tsv") || !fs::exists(argv[2]))return 77;
    const fs::path output=argv[3];require(!fs::exists(output),"BC2 GPU output must be fresh");
    fs::create_directories(output);created=true;
    const auto images=xemu_bc2_test::load_images(argv[1]);
    std::vector<ImportedStage> textures;
    for(const auto &image:images) {
      auto texture=image.source;
      if(!raw)require(prepare_frontend_texture_storage(texture),"frontend did not prepare BC2 storage");
      textures.push_back({std::move(texture),{},15});
    }
    {
      Gpu gpu;available=gpu.initialize(argv[2],textures);device=gpu.device_name;unavailable=gpu.unavailable_reason;
      if(available)for(unsigned i=0;i<images.size();++i) {
        const auto expected=expected_stage(images[i]);const auto actual=gpu.run(textures[i].texture,expected);
        Measurement row{images[i].label,unsigned(images[i].source.mips.size()),actual.size()*4};
        Data native_words(actual.size()*16),original_words(actual.size()*16);std::size_t cursor{};
        for(unsigned level=0;level<expected.mips;++level)for(unsigned face=0;face<expected.layers;++face) {
          const auto &original=expected.expected[face*expected.mips+level];
          for(std::size_t pixel=0;pixel<std::size_t(original.width)*original.height;++pixel,++cursor)
            for(unsigned channel=0;channel<4;++channel) {
              row.different+=actual[cursor].native[channel]!=actual[cursor].original[channel];
              row.roundtrip+=byte(actual[cursor].original[channel])!=std::to_integer<unsigned>(original.sampled[pixel*4+channel]);
            }
        }
        require(cursor==actual.size(),"BC2 GPU comparison missed mip/face texels");
        for(std::size_t at=0;at<actual.size();++at) {
          std::memcpy(native_words.data()+at*16,actual[at].native.data(),16);
          std::memcpy(original_words.data()+at*16,actual[at].original.data(),16);
        }
        save(output/(row.label+"-native-float.bin"),native_words);
        save(output/(row.label+"-original-float.bin"),original_words);
        std::cout<<row.label<<": "<<row.words<<" RGBA float words, "<<row.different<<" differences\n";
        words+=row.words;mips+=row.mip_faces;different+=row.different;roundtrip+=row.roundtrip;
        rows.push_back(std::move(row));
      }
    }
  }catch(const std::exception &error){failure=error.what();std::cerr<<failure<<'\n';}
  const bool complete=available && failure.empty() && rows.size()==11 && mips==59 && words==9611464 &&
      !roundtrip && !validation_errors && !cleanup_errors;
  const bool success=complete && (raw || !different);
  if(created) {
    std::ofstream report(fs::path(argv[3])/"report.json");
    report<<"{\n  \"status\":"<<std::quoted(success?(raw?"diagnostic-complete":"passed"):"failed")
        <<",\n  \"mode\":"<<std::quoted(raw?"hardware-compressed-diagnostic":"production-source-expanded")
        <<",\n  \"device\":"<<std::quoted(device)<<",\n  \"images\":"<<rows.size()<<",\n  \"mip_faces\":"<<mips
        <<",\n  \"RGBA_float_words\":"<<words<<",\n  \"differing_float_words\":"<<different
        <<",\n  \"original_roundtrip_differences\":"<<roundtrip<<",\n  \"validation_errors\":"<<validation_errors.load()
        <<",\n  \"cleanup_errors\":"<<cleanup_errors.load()<<",\n  \"failure\":"<<std::quoted(failure)
        <<",\n  \"unavailable\":"<<std::quoted(unavailable)<<",\n  \"measurements\":[\n";
    for(unsigned i=0;i<rows.size();++i) {
      const auto &r=rows[i];report<<"    {\"label\":"<<std::quoted(r.label)<<",\"mip_faces\":"<<r.mip_faces
          <<",\"words\":"<<r.words<<",\"different\":"<<r.different<<"}"<<(i+1==rows.size()?"\n":",\n");
    }
    report<<"  ],\n  \"scope\":\"Production frontend BC2 expansion and Vulkan texelFetch versus independently original-decoded RGBA8. Synthetic cubes are layout coverage, not original frontend cube consumers. No filtering, residency, raster or whole-frame parity claim.\"\n}\n";
    if(!report)return 1;
  }
  if(!available && failure.empty()){std::cerr<<"SKIP: "<<unavailable<<'\n';return 77;}
  if(!success)return 1;
  std::cout<<"11 BC2 images / 59 mip-faces / 9611464 RGBA float words; "<<different
      <<" differences; zero validation/cleanup errors; "<<(raw?"raw diagnostic":"production source expansion")<<'\n';
  return 0;
}
