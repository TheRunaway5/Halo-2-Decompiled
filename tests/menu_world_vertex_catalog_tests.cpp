#include "halo2/menu_world_vertex_catalog.hpp"
#include "menu_world_vertex_catalog.generated.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::string read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  require(bool(file),"catalog original wrapper fixture is missing");
  const auto size=file.tellg();require(size>=0 && size<1024*1024,"catalog fixture extent is invalid");
  std::string result(static_cast<std::size_t>(size),'\0');file.seekg(0);
  if(size && !file.read(result.data(),size))throw FormatError("catalog fixture is truncated");
  return result;
}
std::string body(const std::string &text) {
  const auto at=text.find("void main() {\n");require(at!=std::string::npos,"catalog fixture lacks a main function");
  return text.substr(at);
}
std::string fetch_helpers(const std::string &text) {
  const auto at=text.find("float sourceClampW(float t) {");
  const auto end=text.find("void main() {\n",at);
  require(at!=std::string::npos && end!=std::string::npos,"catalog fetch/epilogue helper fixture is missing");
  return text.substr(at,end-at);
}
template<class Function>void rejects(Function function,unsigned &count,const char *message) {
  bool rejected=false;try{function();}catch(const FormatError &){rejected=true;}
  require(rejected,message);++count;
}
}
int main(int argc,char **argv) {try {
  require(argc==3,"menu vertex catalog tests need mainmenu.map and original independent-wrapper directory");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  const Cache cache(argv[1]);const std::filesystem::path fixtures=argv[2];
  const auto catalog=std::span(generated_menu_world_vertex_catalog::entries);
  require(!catalog.empty(),"embedded menu vertex catalog is empty");
  unsigned original_rows=0,supported_tags=0,rejected_tags=0,rejections=0;
  std::vector<std::byte> first_program;
  for(const auto [event,datum]:std::array<std::pair<unsigned,std::uint32_t>,4>{{
      {1335,0xe1cc0057u},{1448,0xe1dd0068u},{1880,0xe1ba0045u},{2409,0xe1ba0045u}}}) {
    const auto bytes=cache.load_vertex_shader_code(datum,1);
    const auto declaration=cache.load_source_vertex_declaration(datum,1);
    const auto actual=menu_world_vertex_shader_source(bytes,declaration);
    const auto reference=read(fixtures/("event-"+std::to_string(event)+"-independent-straight-line.vert"));
    require(body(actual.glsl)==body(reference),"production catalog changed the independently measured fetch/body/epilogue");
    require(fetch_helpers(actual.glsl)==fetch_helpers(reference),"production catalog changed the independently measured wrapper helpers");
    require(actual.glsl.find("layout(set=1,binding=0,std140)")!=std::string::npos &&
        actual.glsl.find("layout(offset=5336)")==std::string::npos &&
        actual.glsl.find("vec2 fogParam;\n    vec2 surfaceSize;")!=std::string::npos,
        "production catalog did not expose the agreed compact caller UBO");
    const auto &entry=menu_world_vertex_catalog_lookup(catalog,bytes,declaration);
    require(std::equal(actual.consumed_inputs.begin(),actual.consumed_inputs.end(),entry.consumed_inputs.begin(),entry.consumed_inputs.end()) &&
        std::equal(actual.consumed_constants.begin(),actual.consumed_constants.end(),entry.consumed_constants.begin(),entry.consumed_constants.end()),
        "compiled catalog consumed inputs differ from production source");
    require(entry.source_sha256.size()==64 && entry.spirv_sha256.size()==64 && entry.spirv.size()>5,
        "compiled catalog provenance/module is incomplete");
    auto changed=bytes;changed.back()^=std::byte{0x80};
    rejects([&]{(void)menu_world_vertex_catalog_lookup(catalog,changed,declaration);},rejections,
        "catalog accepted a different literal program payload");
    auto wrong=declaration;wrong.class_index=0;
    rejects([&]{(void)menu_world_vertex_catalog_lookup(catalog,bytes,wrong);},rejections,
        "catalog accepted another compiled class");
    wrong=declaration;wrong.semantics[0]=std::uint16_t(SourceVertexSemantic::UV);
    rejects([&]{(void)menu_world_vertex_catalog_lookup(catalog,bytes,wrong);},rejections,
        "catalog ignored the literal ordered declaration identity");
    if(declaration.semantics.size()>2) {
      wrong=declaration;std::swap(wrong.semantics[1],wrong.semantics[2]);
      rejects([&]{(void)menu_world_vertex_catalog_lookup(catalog,bytes,wrong);},rejections,
          "catalog accepted reordered source vertex registers");
      wrong=declaration;wrong.semantics.resize(1);
      rejects([&]{(void)menu_world_vertex_shader_source(bytes,wrong);},rejections,
          "catalog source fabricated a missing constant vertex attribute");
    }
    ++original_rows;
  }
  // Every supported class0..6 payload is generated and found by its complete
  // program identity, or
  // rejected explicitly by this bounded producer. No supported tag is omitted.
  for(const auto &tag:cache.tags())if(tag.group=="vrtx")for(unsigned classification:{0u,1u,2u,3u,4u,5u,6u}) {
    std::vector<std::byte> bytes;SourceVertexDeclaration declaration;
    try {
      bytes=cache.load_vertex_shader_code(tag.datum,classification);
      declaration=cache.load_source_vertex_declaration(tag.datum,classification);
      (void)menu_world_vertex_shader_source(bytes,declaration);
    }catch(const FormatError &){++rejected_tags;continue;}
    const auto& entry=menu_world_vertex_catalog_lookup(catalog,bytes,declaration);++supported_tags;
    if(&entry==&catalog.front())first_program=bytes;
  }
  const auto &first=catalog.front();
  require(!first_program.empty(),"first catalog identity has no original program fixture");
  SourceVertexDeclaration declaration;declaration.class_index=first.class_index;
  declaration.semantics.assign(first.semantics.begin(),first.semantics.end());
  std::array<MenuWorldVertexCatalogEntry,2> duplicate{first,first};
  rejects([&]{(void)menu_world_vertex_catalog_lookup(duplicate,first_program,declaration);},rejections,
      "catalog accepted two modules for one literal identity");
  auto damaged=first;damaged.abi=menu_world_vertex_attribute_catalog_abi+1;
  rejects([&]{(void)menu_world_vertex_catalog_lookup(std::span(&damaged,1),first_program,declaration);},rejections,
      "catalog accepted an incompatible wrapper ABI");
  const std::array<std::uint32_t,5> invalid_spirv{0,0x00010600,0,1,0};damaged=first;damaged.spirv=invalid_spirv;
  rejects([&]{(void)menu_world_vertex_catalog_lookup(std::span(&damaged,1),first_program,declaration);},rejections,
      "catalog accepted a malformed embedded module");
  damaged=first;damaged.program_bytes++;
  rejects([&]{(void)menu_world_vertex_catalog_lookup(std::span(&damaged,1),first_program,declaration);},rejections,
      "catalog ignored the complete source program extent");
  damaged=first;damaged.program_sha256="0000000000000000000000000000000000000000000000000000000000000000";
  rejects([&]{(void)menu_world_vertex_catalog_lookup(std::span(&damaged,1),first_program,declaration);},rejections,
      "catalog ignored the complete source program SHA-256");
  declaration.semantics[0]=std::uint16_t(SourceVertexSemantic::Incident);
  rejects([&]{(void)menu_world_vertex_shader_source(first_program,declaration);},rejections,
      "catalog silently supplied an unsupported raw semantic");
  // Caller values are not catalog state: two arbitrary banks, including a
  // negative zero payload, retain distinct bytes while resolving one module.
  MenuWorldVertexUniforms a{},b{};a.constants[50][3]=1;b.constants[50][3]=2;
  const std::uint32_t negative_zero=0x80000000u;std::memcpy(&b.fog_parameters[1],&negative_zero,4);
  require(std::memcmp(&a,&b,sizeof(a))!=0 && sizeof(a)==3104,"catalog conflated caller draw banks");
  require(original_rows==4 && supported_tags>=3 && rejected_tags>0,"catalog did not cover actual mainmenu inventory");
  std::cout<<"menu vertex catalog: "<<original_rows<<" independent source rows, "<<catalog.size()
      <<" compiled modules, "<<supported_tags<<" supported / "<<rejected_tags<<" rejected actual tags, "
      <<rejections<<" negative controls; compact ABI3104; no renderer admission\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
