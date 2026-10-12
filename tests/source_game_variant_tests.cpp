#include "halo2/source_game_variant.hpp"
#include "halo2/frontend.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool pass, const std::string &message) {
  if (!pass) throw std::runtime_error(message);
}
void compare(const SourceGameVariant &actual, const std::string &expected,
             const std::string &context) {
  check(expected.size()==actual.bytes.size()*2,"invalid original source buffer");
  for (unsigned n=0; n<actual.bytes.size(); ++n)
    check(actual.bytes[n]==std::stoul(expected.substr(n*2,2),nullptr,16),
          context+" buffer differs at byte"+std::to_string(n));
}
void parameters(std::istringstream &input,const SourceGameVariant &actual) {
  for (int p=0; p<112; ++p) {
    int expected; input>>expected;
    check(source_game_variant_get(actual,p)==expected,
          "original getter differs for parameter"+std::to_string(p));
  }
}
} // namespace
int main(int argc,char **argv) {
  try {
    check(argc==2,"original fixture argument required");
    std::ifstream file(argv[1]); check(bool(file),"missing original variant fixture");
    unsigned defaults=0,variants=0,setters=0;
    for (std::string line;std::getline(file,line);) {
      if (line.empty() || line[0]=='#') continue;
      std::istringstream input(line); char kind; input>>kind;
      unsigned engine; std::string expected;
      if (kind=='D') {
        input>>engine>>expected;
        const auto actual=source_game_variant_default(engine);
        compare(actual,expected,"default"); parameters(input,actual); ++defaults;
      } else if (kind=='V') {
        unsigned index,count; UiVariant variant;
        input>>index>>variant.engine>>std::quoted(variant.name)>>count;
        for (unsigned n=0;n<count;++n) {
          unsigned p;int value;input>>p>>value;variant.values.append(p,value);
        }
        input>>expected;const auto actual=source_game_variant(variant);
        compare(actual,expected,"built-in"+std::to_string(index));
        parameters(input,actual);++variants;
      } else if (kind=='S') {
        int p,value,result;input>>engine>>p>>value>>expected>>result;
        auto actual=source_game_variant_default(engine);
        source_game_variant_set(actual,p,value);
        compare(actual,expected,"setter"+std::to_string(p));
        check(source_game_variant_get(actual,p)==result,
              "original narrowed getter differs for parameter "+std::to_string(p)+
              " input "+std::to_string(value)+" expected "+std::to_string(result)+
              " actual "+std::to_string(source_game_variant_get(actual,p)));
        ++setters;
      } else throw std::runtime_error("unknown original fixture row");
      check(!input.fail(),"invalid original variant fixture row");
    }
    check(defaults==7 && variants==39 && setters==1347,"incomplete original variant fixture");
    std::cout<<"Matched 7 original defaults, 39 complete built-in buffers / 4368 reads and "
             <<setters<<" setter cases, independently agreed by retail and TU1.1\n";
    return 0;
  } catch (const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
