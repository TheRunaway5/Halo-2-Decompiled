#include "halo2/menu_world_vertex_catalog.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
std::string json_string(std::string_view text) {
  std::ostringstream out;out<<'"';
  for(const unsigned char c:text) {
    if(c=='"' || c=='\\')out<<'\\'<<char(c);
    else if(c<32)out<<"\\u00"<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c)<<std::dec;
    else out<<char(c);
  }
  out<<'"';return out.str();
}
template<class Range>void numbers(std::ostream &out,const Range &values) {
  out<<'[';bool first=true;
  for(const auto value:values){if(!first)out<<',';first=false;out<<value;}
  out<<']';
}
void write(const std::filesystem::path &path,std::span<const std::byte> bytes) {
  std::ofstream file(path,std::ios::binary|std::ios::trunc);
  if(!file || !file.write(reinterpret_cast<const char *>(bytes.data()),std::streamsize(bytes.size())))
    throw FormatError("cannot write menu vertex catalog artifact: "+path.string());
}
void write(const std::filesystem::path &path,const std::string &text) {
  write(path,std::as_bytes(std::span(text.data(),text.size())));
}
struct Entry {
  std::vector<std::byte> program;
  SourceVertexDeclaration declaration;
  MenuWorldVertexShaderSource source;
  std::vector<Tag> aliases;
};
struct Rejected {Tag tag;unsigned vertex_class;std::string kind,reason;};
}
int main(int argc,char **argv) {try {
  std::filesystem::path map,output;
  for(int i=1;i<argc;i+=2) {
    if(i+1>=argc)throw FormatError("menu vertex catalog option has no value");
    const std::string option=argv[i];
    if(option=="--map" && map.empty())map=argv[i+1];
    else if(option=="--output" && output.empty())output=argv[i+1];
    else throw FormatError("unknown or duplicate menu vertex catalog option");
  }
  if(map.empty() || output.empty())
    throw FormatError("usage: generate_menu_world_vertex_catalog --map mainmenu.map --output directory");
  const Cache cache(map);std::vector<Tag> tags;
  for(const auto &tag:cache.tags())if(tag.group=="vrtx")tags.push_back(tag);
  std::sort(tags.begin(),tags.end(),[](const auto &a,const auto &b){return a.datum<b.datum;});
  std::vector<Entry> entries;std::vector<Rejected> rejected;
  for(const auto &tag:tags)for(unsigned vertex_class:{0u,1u}) {
    try {
      auto program=cache.load_vertex_shader_code(tag.datum,vertex_class);
      auto declaration=cache.load_source_vertex_declaration(tag.datum,vertex_class);
      const auto existing=std::find_if(entries.begin(),entries.end(),[&](const auto &entry) {
        return entry.program==program && entry.declaration==declaration;
      });
      if(existing!=entries.end()){existing->aliases.push_back(tag);continue;}
      auto source=menu_world_vertex_shader_source(program,declaration);
      entries.push_back({std::move(program),std::move(declaration),std::move(source),{tag}});
    } catch(const UnsupportedVertexProgram &error) {
      rejected.push_back({tag,vertex_class,"unsupported",error.what()});
    } catch(const FormatError &error) {
      // A malformed/missing class is not silently relabeled as supported.
      rejected.push_back({tag,vertex_class,"invalid_or_absent_source",error.what()});
    }
  }
  if(entries.empty())throw FormatError("menu cache contains no supported raw class0/class1 vertex programs");
  std::filesystem::create_directories(output);
  std::ostringstream manifest;
  manifest<<"{\n  \"schema\":\"halo2-menu-world-vertex-source-v1\",\n  \"abi\":"
      <<menu_world_vertex_catalog_abi<<",\n  \"target\":\"vulkan1.3\",\n  \"cache\":"
      <<json_string(std::filesystem::absolute(map).string())<<",\n  \"vrtx_count\":"<<tags.size()
      <<",\n  \"entries\":[\n";
  for(std::size_t i=0;i<entries.size();++i) {
    const auto &entry=entries[i];const auto stem="menu-world-vs-"+std::to_string(i);
    write(output/(stem+".vert"),entry.source.glsl);write(output/(stem+".tokens.bin"),entry.program);
    if(i)manifest<<",\n";
    manifest<<"    {\"source\":"<<json_string(stem+".vert")<<",\"program\":"<<json_string(stem+".tokens.bin")
        <<",\"class_index\":"<<entry.declaration.class_index<<",\"semantics\":";
    numbers(manifest,entry.declaration.semantics);manifest<<",\"consumed_inputs\":";
    numbers(manifest,entry.source.consumed_inputs);manifest<<",\"consumed_constants\":";
    numbers(manifest,entry.source.consumed_constants);manifest<<",\"aliases\":[";
    for(std::size_t j=0;j<entry.aliases.size();++j) {
      if(j)manifest<<',';manifest<<"{\"datum\":"<<entry.aliases[j].datum
          <<",\"name\":"<<json_string(entry.aliases[j].name)<<'}';
    }
    manifest<<"]}";
  }
  manifest<<"\n  ],\n  \"rejected\":[\n";
  for(std::size_t i=0;i<rejected.size();++i) {
    if(i)manifest<<",\n";const auto &entry=rejected[i];
    manifest<<"    {\"datum\":"<<entry.tag.datum<<",\"name\":"<<json_string(entry.tag.name)
        <<",\"class_index\":"<<entry.vertex_class<<",\"kind\":"<<json_string(entry.kind)<<",\"reason\":"<<json_string(entry.reason)<<'}';
  }
  manifest<<"\n  ]\n}\n";write(output/"sources.json",manifest.str());
  std::cout<<"menu raw vertex source catalog: "<<entries.size()<<" unique supported programs, "
      <<rejected.size()<<" explicit rejections, "<<tags.size()<<" actual vrtx tags\n";
  return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
