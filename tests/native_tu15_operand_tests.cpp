#include "../tools/native_tu15_operand_visit.hpp"
#include <bit>
#include <iostream>
#include <map>

namespace {
struct Word {
  unsigned kind{},bytes{};
  std::uint64_t bits{};
  bool operator==(const Word&)const=default;
};
struct Projection {
  std::map<std::string,Word> words;
  std::size_t scalars{};
  template<class T> void scalar(T value,const std::string& path) {
    Word word;word.bytes=sizeof(T);++scalars;
    if constexpr(std::is_same_v<T,float>){word.kind=1;word.bits=std::bit_cast<std::uint32_t>(value);}
    else if constexpr(std::is_same_v<T,double>){word.kind=1;word.bits=std::bit_cast<std::uint64_t>(value);}
    else if constexpr(std::is_enum_v<T>){word.kind=2;word.bits=std::uint64_t(static_cast<std::underlying_type_t<T>>(value));}
    else {word.kind=3;word.bits=std::uint64_t(value);}
    if(!words.emplace(path,word).second)throw std::runtime_error("Duplicate operand field identity: "+path);
  }
  void optional(const std::string& path,bool present){words.emplace(path+"?",Word{4,0,present});}
  template<class T> std::string shared(const std::string& path,bool present) {
    words.emplace(path+"?",Word{5,0,present});return "(*"+path+")";
  }
  void shared_end(const std::string&,bool){}
  void map(const std::string& path,std::size_t size){words.emplace(path+"#",Word{6,0,size});}
  template<class T> std::string map_entry(const std::string& path,T key){return path+"["+std::to_string(std::uint64_t(key))+"]";}
  void sequence(const std::string& path,std::size_t size,bool){words.emplace(path+"#",Word{7,0,size});}
  std::uint64_t hash()const {
    std::uint64_t hash=14695981039346656037ull;
    const auto byte=[&](unsigned value){hash=(hash^(value&255u))*1099511628211ull;};
    for(const auto& [path,word]:words) {
      for(unsigned char c:path)byte(c);byte(0);byte(word.kind);byte(word.bytes);
      for(unsigned n=0;n<8;++n)byte(unsigned(word.bits>>(8*n)));
    }
    return hash;
  }
};
Projection project(const halo2::OriginalData& data) {
  Projection out;halo2::reference::operand_fields(out,data,"data");return out;
}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
int main(int argc,char** argv)try {
  check(argc==1 || argc==2,"Optional argument is the reference-only English TU1.5 executable");
  const auto data=halo2::OriginalData::native_tu15();
  check(data->image==halo2::OriginalData::Image::TitleUpdate15 && data->source=="native-tu15-engine-operands",
        "Native gameplay operands retained an executable dependency or wrong title update");
  check(data==halo2::OriginalData::native_tu15(),"Native immutable operand ownership changed");
  const auto actual=project(*data);
  check(actual.scalars==12261,"Native operand schema did not retain every classified scalar");
  check(actual.words.size()==12638 && actual.hash()==0x2a84073bd6ab09b7ull,
        "Native operands differ from the frozen TU1.5 field/ownership/extent projection");
  if(argc==2) {
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto reference=halo2::OriginalData::load_xbe(argv[1]);
    check(reference->image==halo2::OriginalData::Image::TitleUpdate15,"Wrong reference update for native operands");
    const auto expected=project(*reference);
    if(actual.words!=expected.words) {
      for(const auto& [path,value]:expected.words)
        if(!actual.words.contains(path) || actual.words.at(path)!=value)
          throw std::runtime_error("Native operand differs from authenticated TU1.5 reference: "+path);
      throw std::runtime_error("Native operands have unexpected field/container ownership");
    }
  }
  std::cout<<"PASS "<<actual.scalars<<" typed native TU1.5 scalar operands; "<<actual.words.size()
           <<" field/ownership/extent records; projection FNV64="<<std::hex<<actual.hash()
           <<"; no executable required by native factory\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
