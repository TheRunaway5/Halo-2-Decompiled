#include "halo2/menu_world_vertex_source.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
std::string read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  require(bool(file),"vertex source fixture is missing");const auto size=file.tellg();
  require(size>=0 && size<1024*1024,"vertex source fixture size is invalid");
  std::string result(static_cast<std::size_t>(size),'\0');file.seekg(0);
  if(size && !file.read(result.data(),size))throw FormatError("vertex source fixture is truncated");
  return result;
}
std::string trim(std::string value) {
  const auto a=value.find_first_not_of(" \t\r\n");
  if(a==std::string::npos)return {};
  return value.substr(a,value.find_last_not_of(" \t\r\n")-a+1);
}
std::vector<std::string> arguments(const std::string &value) {
  std::vector<std::string> out;unsigned depth=0;std::size_t first=0;
  for(std::size_t i=0;i<value.size();++i) {
    if(value[i]=='(')++depth;
    else if(value[i]==')'){require(depth>0,"argument parenthesis underflow");--depth;}
    else if(value[i]==',' && depth==0){out.push_back(trim(value.substr(first,i-first)));first=i+1;}
  }
  require(depth==0,"argument parenthesis is unclosed");out.push_back(trim(value.substr(first)));return out;
}
unsigned operation(const std::string &name) {
  if(name=="MUL" || name=="h2MenuMultiply")return 2;
  if(name=="ADD" || name=="h2MenuAdd")return 3;
  if(name=="MAD" || name=="h2MenuMultiplyAdd")return 4;
  if(name=="DP3" || name=="h2MenuDot3")return 5;
  if(name=="DPH" || name=="h2MenuHomogeneousDot")return 6;
  if(name=="DP4" || name=="h2MenuDot4")return 7;
  if(name=="MOV" || name=="h2MenuMove")return 17;
  if(name=="RCC" || name=="h2MenuReciprocalClamp")return 19;
  if(name=="RSQ" || name=="h2MenuReciprocalSqrt")return 20;
  throw FormatError("unexpected arithmetic in semantic source fixture");
}
std::string target(const std::string &value,unsigned slot) {
  if(value=="_temp_vec")return "h2MenuPending"+std::to_string(slot);
  if(value.starts_with('R'))return value=="R12"?"vpOut[0]":"h2MenuTemp"+value.substr(1);
  if(value=="oPos")return "vpOut[0]";
  if(value=="oD0")return "vpOut[3]";
  if(value=="oD1")return "vpOut[4]";
  if(value=="oFog")return "vpOut[5]";
  if(value.size()==3 && value.starts_with("oT"))return "vpOut["+std::to_string(9+unsigned(value[2]-'0'))+"]";
  throw FormatError("unexpected destination in semantic source fixture");
}
std::string source(std::string value,unsigned slot) {
  bool negative=value.starts_with('-');if(negative)value.erase(0,1);
  const auto dot=value.find('.');auto swizzle=dot==std::string::npos?"xyzw":value.substr(dot+1);
  require(!swizzle.empty() && swizzle.size()<=4,"invalid source fixture swizzle");
  while(swizzle.size()<4)swizzle+=swizzle.back();
  value=value.substr(0,dot);
  if(value.starts_with('v'))value="vpInput["+value.substr(1)+"]";
  else if(value.starts_with("c["))value="vpConstant("+value.substr(2,value.size()-3)+"u)";
  else value=target(value,slot);
  return (negative?"-":"")+value+'.'+swizzle;
}
struct Semantic {std::vector<std::string> statements;std::vector<unsigned> operations;};
void compare_default_texture_w(const VertexProgram &program,const Semantic &original) {
  unsigned expected=15;
  for(unsigned stage=0;stage<4;++stage) {
    const auto destination="|vpOut["+std::to_string(9+stage)+"]|";
    for(const auto &statement:original.statements) {
      const auto at=statement.find(destination);if(at==std::string::npos)continue;
      const auto first=at+destination.size(),end=statement.find('|',first);
      if(statement.substr(first,end-first).find('w')!=std::string::npos)expected&=~(1u<<stage);
    }
  }
  require(menu_world_vertex_default_texture_w_mask(program)==expected,
      "default texture W mask differs from independent original shader writes");
}
Semantic expected_semantic(const std::string &text) {
  Semantic out;std::istringstream lines(text);std::string line;unsigned slot=0;
  const std::regex slot_pattern(R"(^\s*/\* Slot ([0-9]+):.*$)");
  const std::regex call_pattern(R"(^\s*(MUL|ADD|MAD|DP3|DPH|DP4|MOV|RCC|RSQ)\((.*)\);\s*$)");
  const std::regex commit_pattern(R"(^\s*(R[0-9]+)\.([xyzw]+) = _temp_vec\.([xyzw]+);\s*$)");
  bool body=false;
  while(std::getline(lines,line)) {
    std::smatch match;
    if(std::regex_match(line,match,slot_pattern)){slot=std::stoul(match[1]);body=true;continue;}
    if(!body)continue;
    if(std::regex_match(line,match,call_pattern)) {
      const auto parts=arguments(match[2]);require(parts.size()>=3,"source fixture call has too few arguments");
      const auto op=operation(match[1]);std::string normalized=std::to_string(op)+'|'+target(parts[0],slot)+'|'+parts[1];
      for(unsigned i=2;i<parts.size();++i)normalized+='|'+source(parts[i],slot);
      out.statements.push_back(normalized);out.operations.push_back(op);
    } else if(std::regex_match(line,match,commit_pattern)) {
      require(match[2]==match[3],"source paired commit masks differ");
      out.statements.push_back("commit|"+target(match[1],slot)+'.'+match[2].str()+"|h2MenuPending"+
          std::to_string(slot)+'.'+match[3].str());
    }
  }
  require(!out.operations.empty(),"source fixture has no semantic operations");return out;
}
Semantic actual_semantic(const std::string &text) {
  Semantic out;std::istringstream lines(text);std::string line;
  const std::regex call_pattern(R"(^\s*(vpOut\[[0-9]+\]|h2MenuTemp[0-9]+|h2MenuPending[0-9]+)\.([xyzw]+)=(h2Menu[A-Za-z0-9]+)\((.*)\)\.([xyzw]+);\s*$)");
  const std::regex commit_pattern(R"(^\s*(vpOut\[[0-9]+\]|h2MenuTemp[0-9]+)\.([xyzw]+)=(h2MenuPending[0-9]+)\.([xyzw]+);\s*$)");
  while(std::getline(lines,line)) {
    std::smatch match;
    if(std::regex_match(line,match,call_pattern)) {
      require(match[2]==match[5],"emitted write/result masks differ");
      const auto op=operation(match[3]);std::string normalized=std::to_string(op)+'|'+match[1].str()+'|'+match[2].str();
      for(const auto &arg:arguments(match[4]))normalized+='|'+arg;
      out.statements.push_back(normalized);out.operations.push_back(op);
    } else if(std::regex_match(line,match,commit_pattern)) {
      require(match[2]==match[4],"emitted paired commit masks differ");
      out.statements.push_back("commit|"+match[1].str()+'.'+match[2].str()+'|'+match[3].str()+'.'+match[4].str());
    }
  }
  require(!out.operations.empty(),"emitted snippet has no semantic operations");return out;
}
void write(const std::filesystem::path &path,const std::string &text) {
  require(!std::filesystem::exists(path),"refusing to overwrite emitted source artifact");
  std::ofstream file(path,std::ios::binary);require(bool(file),"cannot create emitted source artifact");
  file.write(text.data(),text.size());require(bool(file),"cannot write emitted source artifact");
}
template<class Change>void reject(const VertexProgram &base,unsigned &count,Change change) {
  auto value=base;change(value);bool rejected=false;
  try{(void)menu_world_vertex_source(value);}catch(const FormatError &){rejected=true;}
  require(rejected,"unsupported source emitter input was accepted");++count;
}
void bounds(const VertexProgram &base,unsigned &count) {
  reject(base,count,[](auto &p){p.count=0;});
  reject(base,count,[](auto &p){p.count=p.instructions.size()+1;});
  reject(base,count,[](auto &p){p.relative_constants=true;});
  reject(base,count,[](auto &p){p.instructions[0][3]|=2;});
  reject(base,count,[](auto &p){p.instructions[0][1]=(p.instructions[0][1]&~(15u<<21))|(13u<<21);});
  reject(base,count,[](auto &p){p.instructions[0][1]=(p.instructions[0][1]&~(15u<<21))|(15u<<21);});
  reject(base,count,[](auto &p){p.instructions[0][1]|=2u<<25;});
  reject(base,count,[](auto &p){p.instructions[0][1]=(p.instructions[0][1]&~(15u<<21))|(4u<<21)|(4u<<25);});
  reject(base,count,[](auto &p){p.instructions[0][3]=(p.instructions[0][3]&~(15u<<20))|(13u<<20);});
  reject(base,count,[](auto &p){p.instructions[0][1]=(p.instructions[0][1]&~(255u<<13))|(192u<<13);});
  reject(base,count,[](auto &p){p.instructions[0][1]=(p.instructions[0][1]&~(15u<<9))|(5u<<9);});
  reject(base,count,[](auto &p){p.instructions[0][2]=(p.instructions[0][2]&~(3u<<26))|(1u<<26)|(13u<<28);});
  reject(base,count,[](auto &p){p.instructions[0][2]&=~(3u<<26);});
  reject(base,count,[](auto &p){p.instructions[p.count-1][3]&=~1u;});
  reject(base,count,[](auto &p){p.instructions[0][3]|=1;});
  unsigned output=0;while(output<base.count && !(base.instructions[output][3]&0xf000))++output;
  require(output<base.count,"bound source fixture has no ordinary output write");
  reject(base,count,[&](auto &p){p.instructions[output][3]&=~2048u;});
  reject(base,count,[&](auto &p){p.instructions[output][3]=(p.instructions[output][3]&~(255u<<3))|(6u<<3);});
  reject(base,count,[&](auto &p){p.instructions[output][3]=
      (p.instructions[output][3]&~((255u<<3)|(15u<<12)))|(5u<<3)|(8u<<12);});
  reject(base,count,[&](auto &p){p.instructions[output][3]=(p.instructions[output][3]&~(255u<<3))|(13u<<3);});
  reject(base,count,[&](auto &p){p.instructions[output][3]|=4;});
  reject(base,count,[](auto &p){p.instructions[0][1]&=~(15u<<21);});
  reject(base,count,[](auto &p){p.instructions[0][3]|=1u<<16;});
  const auto position_output=[](auto &p) {
    auto &t=p.instructions[0];t[3]=(t[3]&~((255u<<3)|(15u<<12)|4u))|2048u|(14u<<12);
  };
  reject(base,count,[&](auto &p){
    position_output(p);auto &t=p.instructions[0];
    t[2]=(t[2]&~((15u<<28)|(3u<<26)))|(12u<<28)|(1u<<26);
  });
  const auto scalar_position=[](auto &t) {
    t[2]=(t[2]&~3u)|3u;t[3]=(t[3]&~((3u<<30)|(3u<<28)))|(1u<<28);
  };
  reject(base,count,[&](auto &p){
    position_output(p);auto &t=p.instructions[0];t[1]|=3u<<25;
    t[3]=(t[3]&~(15u<<24))|(8u<<16);scalar_position(t);
  });
  reject(base,count,[&](auto &p){
    position_output(p);auto &t=p.instructions[0];t[1]=(t[1]&~(15u<<21))|(3u<<25);
    t[3]=(t[3]&~(15u<<24))|(8u<<16)|4u;scalar_position(t);
  });
  reject(base,count,[&](auto &p){
    position_output(p);auto &t=p.instructions[0];t[1]|=3u<<25;
    t[3]=(t[3]&~((15u<<20)|(3u<<28)))|(12u<<20)|(3u<<28)|4u;
  });
  // A single output call can consume the old position without rereading it.
  auto single=base;position_output(single);auto &t=single.instructions[0];
  t[3]&=~(15u<<24);t[2]=(t[2]&~((15u<<28)|(3u<<26)))|(12u<<28)|(1u<<26);
  require(!menu_world_vertex_source(single).statements.empty(),"single noncolliding position call was rejected");
  // Two calls to a nonaliasing output retain the ordinary static sequence.
  auto separate=base;position_output(separate);auto &u=separate.instructions[0];
  u[3]|=3u<<3;u[2]=(u[2]&~((15u<<28)|(3u<<26)))|(12u<<28)|(1u<<26);
  require(!menu_world_vertex_source(separate).statements.empty(),"nonaliasing output calls were rejected");
}
VertexProgram single_token(unsigned mac,unsigned ilu,unsigned mm,unsigned im,unsigned om,
                           bool output_ilu=false,unsigned reg=0) {
  VertexProgram p;p.count=1;
  // Identity-swizzled R0, c0, R0; independently assembled from public token
  // fields, without an original generator field table or initializer.
  p.instructions[0]={0,(mac<<21)|(ilu<<25)|0x1bu,
      (1u<<26)|(3u<<11)|(1u<<21)|(2u<<19)|(3u<<17)|(1u<<6)|(2u<<4)|(3u<<2),
      (1u<<28)|(mm<<24)|(reg<<20)|(im<<16)|(om<<12)|2048u|(output_ilu?4u:0u)|1u};
  return p;
}
void temporary_source(VertexProgram &p,unsigned argument,unsigned reg,std::array<unsigned,4> swizzle={0,1,2,3}) {
  auto &t=p.instructions[0];
  if(argument==0) {
    t[2]=(t[2]&~((3u<<26)|(15u<<28)))|(1u<<26)|(reg<<28);
    t[1]=(t[1]&~255u)|(swizzle[0]<<6)|(swizzle[1]<<4)|(swizzle[2]<<2)|swizzle[3];
  } else if(argument==1) {
    t[2]=(t[2]&~((3u<<11)|(15u<<13)|(255u<<17)))|(1u<<11)|(reg<<13)|
        (swizzle[0]<<23)|(swizzle[1]<<21)|(swizzle[2]<<19)|(swizzle[3]<<17);
  } else {
    t[2]=(t[2]&~1023u)|(reg>>2)|(swizzle[0]<<8)|(swizzle[1]<<6)|(swizzle[2]<<4)|(swizzle[3]<<2);
    t[3]=(t[3]&~((3u<<28)|(3u<<30)))|(1u<<28)|((reg&3u)<<30);
  }
}
void component_alias_bounds(unsigned &rejections,unsigned &acceptances) {
  const auto accept=[&](const VertexProgram &p,const char *label) {
    try{require(!menu_world_vertex_source(p).statements.empty(),"accepted component control emitted no statements");}
    catch(const std::exception &e){throw FormatError(std::string(label)+": "+e.what());}
    ++acceptances;
  };
  const auto deny=[&](const VertexProgram &p,const char *label) {
    try{(void)menu_world_vertex_source(p);}
    catch(const UnsupportedVertexProgram &e) {
      require(std::string(e.what()).find("alias corridor is unproved")!=std::string::npos,
          "component alias control failed for an unrelated bound");++rejections;return;
    }
    throw FormatError(std::string(label)+": overlapping component alias was accepted");
  };
  auto p=single_token(2,0,1,0,14);temporary_source(p,0,12);
  accept(p,"MUL xyz output and w temporary read are disjoint");
  p=single_token(2,0,8,0,14);temporary_source(p,0,12,{3,0,0,0});
  accept(p,"MUL x result reads swizzled w only");
  temporary_source(p,0,12);deny(p,"MUL x result rereads changed x");
  p=single_token(5,0,1,0,1);temporary_source(p,0,12);
  accept(p,"DP3 ignores first-operand w");
  p.instructions[0][3]=(p.instructions[0][3]&~(15u<<12))|(8u<<12);
  deny(p,"DP3 broadcast result reads changed x");
  p=single_token(5,0,1,0,1);temporary_source(p,0,12,{3,3,3,0});
  deny(p,"DP3 swizzled xyz reads changed w");
  p=single_token(6,0,1,0,1);temporary_source(p,0,12);
  accept(p,"DPH substitutes first-operand w");
  p=single_token(6,0,1,0,1);temporary_source(p,1,12);
  deny(p,"DPH consumes second-operand w");
  p=single_token(7,0,1,0,1);temporary_source(p,0,12);
  deny(p,"DP4 consumes first-operand w");
  for(const auto ilu:{3u,4u}) {
    p=single_token(2,ilu,0,1,14);temporary_source(p,2,12,{3,0,1,2});
    accept(p,"scalar ILU reads only first swizzled component");
    temporary_source(p,2,12);deny(p,"scalar ILU rereads changed first component");
  }
  p=single_token(5,1,0,1,14);temporary_source(p,2,12);
  accept(p,"paired MOV w reads unmodified w");
  p.instructions[0][3]=(p.instructions[0][3]&~(15u<<16))|(8u<<16);
  deny(p,"paired MOV x rereads modified x");
  p=single_token(0,1,0,1,14,true);temporary_source(p,2,12);
  accept(p,"MOV output xyz and temporary w are disjoint");
  p.instructions[0][3]=(p.instructions[0][3]&~(15u<<16))|(8u<<16);
  deny(p,"MOV output and temporary overlap x");
  p=single_token(0,1,0,4,8,true);temporary_source(p,2,12,{3,0,1,2});
  deny(p,"MOV uses the written lane swizzle rather than scalar replication");
  p=single_token(0,3,0,1,14,true);temporary_source(p,2,12,{3,0,1,2});
  accept(p,"RCC output xyz leaves its repeated w input unchanged");
  p.instructions[0][3]=(p.instructions[0][3]&~(15u<<12))|(1u<<12);
  deny(p,"RCC output w changes its repeated scalar input");
  p=single_token(2,3,1,0,14,true,12);
  accept(p,"pending MAC w commit is disjoint from ILU xyz output");
  p.instructions[0][3]=(p.instructions[0][3]&~(15u<<24))|(8u<<24);
  deny(p,"pending MAC commit overlaps ILU output x");
  p=single_token(5,1,14,0,14,false,1);temporary_source(p,0,12);
  accept(p,"paired reg1 suppresses the later MAC temporary call");
}
void compare_semantic(const MenuWorldVertexSource &source,const Semantic &reference,const std::string &label) {
  const auto actual=actual_semantic(source.statements);
  auto helper_operations=source.helper_operations;
  for(auto &op:helper_operations)if(op==1)op=17; // MAC MOV and ILU MOV emit the same original helper.
  if(actual.statements==reference.statements && actual.operations==reference.operations &&
      actual.operations==helper_operations)return;
  std::cerr<<label<<" expected "<<reference.statements.size()<<" semantic statements / "<<reference.operations.size()
      <<" calls; actual "<<actual.statements.size()<<" statements / "<<actual.operations.size()<<" parsed calls / "
      <<source.helper_operations.size()<<" emitter calls\n";
  const auto count=std::max(reference.statements.size(),actual.statements.size());
  for(std::size_t i=0;i<count;++i) {
    const auto expected=i<reference.statements.size()?reference.statements[i]:"<missing>";
    const auto observed=i<actual.statements.size()?actual.statements[i]:"<missing>";
    if(expected!=observed){std::cerr<<"first semantic mismatch "<<i<<"\nexpected: "<<expected<<"\nactual: "<<observed<<'\n';break;}
  }
  throw FormatError("independent emitter changed original static call/operand/destination/paired-commit sequence");
}
}
int main(int argc,char **argv) {try {
  require(argc>=3 && argc<=11 && argc%2==1,
      "menu vertex source test needs mainmenu.map, frozen v2 directory, optional --extension fixture-directory and --emit fresh-directory");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  const std::filesystem::path directory=argv[2];
  for(const unsigned event:{1335u,1448u,1880u,2409u})
    if(!std::filesystem::exists(directory/("event-"+std::to_string(event)+"-source-straight-line.vert")))return 77;
  std::filesystem::path output,extension,sphere,sphere_map;
  for(int i=3;i<argc;i+=2) {
    const std::string option=argv[i];
    if(option=="--emit"){require(output.empty(),"duplicate emit option");output=argv[i+1];}
    else if(option=="--extension"){require(extension.empty(),"duplicate extension option");extension=argv[i+1];}
    else if(option=="--sphere"){require(sphere.empty(),"duplicate sphere option");sphere=argv[i+1];}
    else if(option=="--sphere-map"){require(sphere_map.empty(),"duplicate sphere map");sphere_map=argv[i+1];}
    else throw FormatError("unknown menu vertex source test option");
  }
  if(!output.empty())require(!std::filesystem::exists(output),"emitted source output already exists");
  if(!extension.empty() && !std::filesystem::exists(extension/"manifest.tsv"))return 77;
  const Cache cache(argv[1]);unsigned slots=0,calls=0,rejections=0,acceptances=0,extension_programs=0;
  std::vector<std::pair<unsigned,MenuWorldVertexSource>> emitted;
  const std::array<std::pair<unsigned,std::uint32_t>,4> programs{{{1335,0xe1cc0057u},{1448,0xe1dd0068u},{1880,0xe1ba0045u},{2409,0xe1ba0045u}}};
  for(const auto [event,datum]:programs) {
    const auto bytes=cache.load_vertex_shader_code(datum,1);
    const auto raw=read(directory/("event-"+std::to_string(event)+"-source1-program.bin"));
    require(bytes.size()==raw.size() && std::equal(bytes.begin(),bytes.end(),reinterpret_cast<const std::byte *>(raw.data())),
        "Cache class1 program differs from original imported token fixture");
    const auto program=decode_vertex_program(bytes);auto source=menu_world_vertex_source(program);
    const auto reference=expected_semantic(read(directory/("event-"+std::to_string(event)+"-source-straight-line.vert")));
    compare_default_texture_w(program,reference);
    compare_semantic(source,reference,"event "+std::to_string(event));
    const auto repeated=menu_world_vertex_source(program);
    require(source.helpers==repeated.helpers && source.statements==repeated.statements,
        "independent emitter is nondeterministic");
    require(source.statements.find("for(")==std::string::npos && source.statements.find("if(")==std::string::npos &&
        source.helpers.find("#define")==std::string::npos && source.helpers.find("_in(")==std::string::npos &&
        source.helpers.find("NoContraction")==std::string::npos && source.helpers.find("precise")==std::string::npos,
        "emitter imported original initializer or added dynamic dispatch/precision policy");
    require(source.consumed_inputs==(event==1335 || event==1448?std::vector<unsigned>{0,1,2,3,4}:std::vector<unsigned>{0}),
        "emitter source input contract differs");
    if(event==1335)bounds(program,rejections);
    slots+=program.count;calls+=source.helper_operations.size();emitted.emplace_back(event,std::move(source));
  }
  require(emitted[2].second.helpers==emitted[3].second.helpers && emitted[2].second.statements==emitted[3].second.statements,
      "shared fog program produced incompatible source snippets");
  component_alias_bounds(rejections,acceptances);
  struct Extra {std::uint32_t datum;unsigned classification;std::string key;MenuWorldVertexSource source;};
  std::vector<Extra> extra;
  std::optional<Cache> sphere_cache;
  if(!sphere.empty()){require(!sphere_map.empty(),"sphere programs require their actual Cache map");sphere_cache.emplace(sphere_map);}
  const auto additional=[&](const std::filesystem::path &directory,bool is_sphere) {
    const auto &program_cache=is_sphere?*sphere_cache:cache;
    std::istringstream lines(read(directory/(is_sphere?"programs.tsv":"manifest.tsv")));std::string line;
    require(bool(std::getline(lines,line)) && line=="vertex_shader\tclassification\tprogram\tsemantic\tslots\thelper_calls",
        "extension fixture manifest header differs");
    std::set<std::pair<std::uint32_t,unsigned>> seen;
    while(std::getline(lines,line)) {
      if(line.empty())continue;
      std::istringstream fields(line);std::uint32_t datum;unsigned classification,expected_slots,expected_calls;
      std::string raw_name,semantic_name,trailing;
      require(bool(fields>>datum>>classification>>raw_name>>semantic_name>>expected_slots>>expected_calls) && !(fields>>trailing),
          "invalid extension fixture manifest row");
      require((is_sphere?(datum==0xe1a2002du || datum==0xe1c0004bu):(datum==0xe1b80043u || datum==0xe1eb0076u)) && classification<2 && seen.emplace(datum,classification).second,
          "unexpected or duplicate extension program");
      require(std::filesystem::path(raw_name).filename()==raw_name && std::filesystem::path(semantic_name).filename()==semantic_name,
          "extension fixture file is outside fixture directory");
      const auto bytes=program_cache.load_vertex_shader_code(datum,classification);
      const auto raw=read(directory/raw_name);
      require(bytes.size()==raw.size() && std::equal(bytes.begin(),bytes.end(),reinterpret_cast<const std::byte *>(raw.data())),
          "Cache extension program differs from literal original token fixture");
      const auto program=decode_vertex_program(bytes);auto source=menu_world_vertex_source(program);
      const auto reference=expected_semantic(read(directory/semantic_name));
      compare_default_texture_w(program,reference);
      compare_semantic(source,reference,raw_name);
      require(program.count==expected_slots && source.helper_operations.size()==expected_calls,
          "extension fixture slot/static-call count differs");
      require(source.consumed_inputs==(is_sphere?std::vector<unsigned>{0,1,2,3,4}:(datum==0xe1b80043u?std::vector<unsigned>{0,1,2,3,4}:std::vector<unsigned>{0,1})),
          "extension input-register contract differs");
      require(source.helpers==emitted[0].second.helpers,"extension helper source differs");
      const auto repeat=menu_world_vertex_source(program);
      require(source.statements==repeat.statements,"extension source emission is nondeterministic");
      slots+=program.count;calls+=source.helper_operations.size();++extension_programs;
      extra.push_back({datum,classification,std::filesystem::path(raw_name).stem().string(),std::move(source)});
    }
    require(seen.size()==(is_sphere?3u:4u),"extension fixture is missing an actual datum/class pair");
  };
  if(!extension.empty())additional(extension,false);
  if(!sphere.empty())additional(sphere,true);
  if(!output.empty()) {
    std::filesystem::create_directories(output);write(output/"helpers.glsl",emitted[0].second.helpers);
    std::ostringstream manifest;manifest<<"event\tvertex_shader\tclassification\tstatements\thelper_calls\n";
    for(unsigned i=0;i<emitted.size();++i) {
      const auto &item=emitted[i];const auto name="event-"+std::to_string(item.first)+"-statements.glsl";
      write(output/name,item.second.statements);
      manifest<<item.first<<'\t'<<programs[i].second<<"\t1\t"<<name<<'\t'<<item.second.helper_operations.size()<<'\n';
    }
    write(output/"manifest.tsv",manifest.str());
    if(!extra.empty()) {
      std::ostringstream additional;
      additional<<"vertex_shader\tclassification\tstatements\thelper_calls\n";
      for(const auto &item:extra) {
        const auto name=item.key+"-statements.glsl";write(output/name,item.source.statements);
        additional<<item.datum<<'\t'<<item.classification<<'\t'<<name<<'\t'<<item.source.helper_operations.size()<<'\n';
      }
      write(output/"extension-manifest.tsv",additional.str());
    }
  }
  std::cout<<"Independent MIT menu vertex emitter: "<<programs.size()<<" original class1 fixtures + "<<extension_programs
      <<" additional actual class0/class1 programs, "<<slots<<" slots, "<<calls<<" exact semantic static calls, "
      <<rejections<<" unsupported-input rejections, "<<acceptances
      <<" disjoint-component controls. This CPU test makes no GPU equivalence claim.\n";
  return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
