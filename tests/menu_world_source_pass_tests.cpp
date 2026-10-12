#include "halo2/menu_world_source_pass.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
unsigned number(const std::string &text,unsigned base=10) {
  std::size_t end{};const auto value=std::stoull(text,&end,base);
  require(end==text.size() && value<=std::numeric_limits<unsigned>::max(),"invalid explicit pass fixture integer");
  return unsigned(value);
}
std::vector<std::byte> hexadecimal(const std::string &text) {
  require(text.size()%2==0,"invalid explicit pass fixture hex length");std::vector<std::byte> out;
  for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(number(text.substr(i,2),16)));
  return out;
}
std::vector<std::byte> read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"original explicit pass program missing");
  const auto size=file.tellg();require(size>0 && size<65536,"invalid original explicit pass program size");
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.seekg(0);
  require(bool(file.read(reinterpret_cast<char *>(bytes.data()),size)),"short original explicit pass program");return bytes;
}
template<std::size_t N>void record(const std::array<std::byte,N> &actual,const std::string &expected) {
  const auto bytes=hexadecimal(expected);
  require(bytes.size()==N && std::equal(actual.begin(),actual.end(),bytes.begin()),"explicit pass record differs from unchanged original output");
}
void backing(const MenuWorldSourcePass &pass,std::uint32_t address,std::size_t extent,std::uint32_t datum) {
  require(std::find(pass.metadata_reads().begin(),pass.metadata_reads().end(),
      MenuWorldSourceMetadataRead{datum,address,extent})!=pass.metadata_reads().end(),
      "explicit pass lost original containing-tag metadata identity");
}
template<class Function>void rejects(Function function,unsigned &count) {
  bool rejected=false;try{function();}catch(const FormatError &){rejected=true;}
  require(rejected,"invalid explicit source pass choice accepted");++count;
}
}
int main(int argc,char **argv){try {
  require(argc==4,"explicit pass test needs mainmenu.map, original TSV and Stage3 directory");
  for(unsigned i=1;i<4;++i)if(!std::filesystem::exists(argv[i]))return 77;
  const Cache cache(argv[1]);std::ifstream file(argv[2]);std::string line;
  require(bool(std::getline(file,line)) && line=="H2_MENU_WORLD_SOURCE_PASS_V1","invalid original explicit pass fixture header");
  unsigned rows{},record_bytes{},program_bytes{},rejections{},alias_rows{};
  std::map<unsigned,unsigned> events;
  std::map<unsigned,std::map<unsigned,std::vector<std::array<std::byte,306>>>> implementation_records;
  while(std::getline(file,line)) {
    if(line.empty())continue;
    std::istringstream input(line);std::vector<std::string> fields;std::string field;
    while(std::getline(input,field,'\t'))fields.push_back(field);
    require(fields.size()==29,"invalid original explicit pass fixture columns");
    const auto event=number(fields[0]),shader=number(fields[1],16);
    MenuWorldSourcePassChoice choice{number(fields[2]),number(fields[3]),number(fields[4]),number(fields[5])};
    const auto preserved=choice;const auto pass=load_menu_world_source_pass(cache,shader,choice);
    require(pass.shader()==shader && pass.choice()==preserved && pass.shader_template()==number(fields[6],16) &&
        pass.shader_pass()==number(fields[7],16) && pass.template_pass()==number(fields[8]) &&
        pass.template_remap()==number(fields[9]) && pass.vertex_shader()==number(fields[10],16),
        "explicit pass chain identity differs from original16B90");
    require(pass.lod_layers().packed==number(fields[11],16) && pass.layer_passes().packed==number(fields[12],16) &&
        pass.implementations().packed==number(fields[13],16) && pass.implementation_address()==number(fields[14],16) &&
        pass.pass_postprocess_address()==number(fields[15],16),"explicit pass ranges/pointers differ from original16B90");
    record(pass.lod_record(),fields[16]);record(pass.pass_record(),fields[17]);record(pass.remap_record(),fields[18]);
    record(pass.implementation_record(),fields[19]);record_bytes+=10+10+6+306;
    require(pass.lod_address()==number(fields[25],16) && pass.layer_address()==number(fields[26],16) &&
        pass.template_pass_address()==number(fields[27],16) && pass.template_remap_address()==number(fields[28],16),
        "explicit pass lost original selected record addresses");
    backing(pass,pass.lod_address(),10,number(fields[20],16));
    backing(pass,pass.layer_address(),2,number(fields[21],16));
    backing(pass,pass.template_pass_address(),10,number(fields[22],16));
    backing(pass,pass.template_remap_address(),6,number(fields[23],16));
    backing(pass,pass.implementation_address(),306,number(fields[24],16));
    if(number(fields[20],16)!=pass.shader_template() || number(fields[21],16)!=pass.shader_template())++alias_rows;
    const auto captured=read(std::filesystem::path(argv[3])/("event-"+std::to_string(event)+"-source1-program.bin"));
    const auto imported=cache.load_vertex_shader_code(pass.vertex_shader(),1);
    require(imported==captured,"resolved explicit pass did not import original captured class1 program");
    const auto declaration=cache.load_source_vertex_declaration(pass.vertex_shader(),1);
    const std::vector<std::uint16_t> semantics=event==1335 || event==1448?
        std::vector<std::uint16_t>{0,3,4,5,6}:std::vector<std::uint16_t>{0};
    require(declaration.class_index==1 && declaration.semantics==semantics,
        "resolved explicit pass class1 declaration differs from original capture");
    const auto before=pass.implementation_record();choice.implementation=99;
    require(pass.choice()==preserved && pass.implementation_record()==before,
        "explicit pass result retained a mutable caller choice");
    implementation_records[event][preserved.shader_lod].push_back(pass.implementation_record());
    program_bytes+=imported.size();++events[event];++rows;
  }
  require(rows==24 && events[1335]==4 && events[1448]==4 && events[1880]==8 && events[2409]==8 &&
      alias_rows==24 && record_bytes==7968 && program_bytes==9312,"not all original explicit pass alternatives were compared");
  for(const auto &[event,lods]:implementation_records)for(const auto &[lod,records]:lods) {
    (void)lod;const unsigned count=event==1335 || event==1448?2:4;
    require(records.size()==count,"explicit pass lost original matching implementation ambiguity");
    for(unsigned i=0;i<count;++i)for(unsigned j=i+1;j<count;++j)
      require(records[i]!=records[j],"explicit pass conflated distinct source implementations sharing one vrtx");
  }
  const MenuWorldSourcePassChoice good{0,3,0,3};
  rejects([&]{(void)load_menu_world_source_pass(cache,0xffffffffu,good);},rejections);
  rejects([&]{(void)load_menu_world_source_pass(cache,0xe1cc0057u,good);},rejections);
  rejects([&]{auto c=good;c.shader_lod=2;(void)load_menu_world_source_pass(cache,0xe777054du,c);},rejections);
  rejects([&]{auto c=good;c.shader_lod=std::numeric_limits<unsigned>::max();(void)load_menu_world_source_pass(cache,0xe777054du,c);},rejections);
  rejects([&]{auto c=good;c.layer=32;(void)load_menu_world_source_pass(cache,0xe777054du,c);},rejections);
  rejects([&]{auto c=good;c.layer=0;(void)load_menu_world_source_pass(cache,0xe777054du,c);},rejections);
  rejects([&]{auto c=good;c.pass_in_layer=1;(void)load_menu_world_source_pass(cache,0xe777054du,c);},rejections);
  rejects([&]{auto c=good;c.implementation=4;(void)load_menu_world_source_pass(cache,0xe777054du,c);},rejections);
  std::cout<<rows<<" original16B90 explicit tuples / "<<record_bytes<<" record bytes / "<<program_bytes
      <<" original class1 bytes; "<<alias_rows<<" cross-tag array cases; "<<rejections<<" invalid choices; no selector\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
