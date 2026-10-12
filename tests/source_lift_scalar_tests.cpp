#include "halo2/source_lift_scalar.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>

using namespace halo2;
int main(int argc,char**argv){try{
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  if(std::fesetround(FE_TONEAREST)!=0)throw FormatError("Cannot set original rounding mode");
  std::ifstream file(argv[1]);std::string schema;file>>schema;
  if(schema!="H2_LIFT_SCALAR_V1")throw FormatError("Invalid lift scalar fixtures");
  unsigned banks,total{};file>>banks;
  for(unsigned bank=0;bank<banks;++bank){
    std::uint32_t words[5]{};for(auto&v:words)file>>std::hex>>v;
    const SourceLiftScalarConstants c{std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1]),
      std::bit_cast<float>(words[2]),std::bit_cast<float>(words[3]),std::bit_cast<float>(words[4])};
    unsigned count;file>>std::dec>>count;
    for(unsigned n=0;n<count;++n){
      std::uint32_t row[13]{};for(auto&v:row)file>>std::hex>>v;
      if(!file)throw FormatError("Truncated lift scalar fixture");
      const auto f=[&](unsigned p){return std::bit_cast<float>(row[p]);};
      const SourceLiftScalarInput i{row[0]!=0,row[1]!=0,row[2]!=0,
        f(3),f(4),f(5),f(6),f(7),f(8),f(9),f(10)};
      const auto result=source_lift_scalar(i,c);
      if(std::bit_cast<std::uint32_t>(result.velocity_delta)!=row[11] ||
          std::bit_cast<std::uint32_t>(result.velocity_ratio)!=row[12]){
        std::cerr<<"bank="<<bank<<" case="<<n<<" actual="<<std::hex
          <<std::bit_cast<std::uint32_t>(result.velocity_delta)<<","<<std::bit_cast<std::uint32_t>(result.velocity_ratio)
          <<" expected="<<row[11]<<","<<row[12]<<'\n';
        throw FormatError("Lift scalar original result mismatch");
      }
      ++total;
    }
    SourceLiftScalarInput power_required{};power_required.exponent=c.one;
    bool rejected{};try{source_lift_scalar(power_required,c);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("Missing original power owner was replaced");
  }
  std::string extra;if(file>>extra)throw FormatError("Trailing lift scalar fixture data");
  if(banks!=2 || total<1024)throw FormatError("Missing independently executed original image bank");
  std::cout<<"PASS "<<total<<" original lift scalar delta/ratio pairs; both update images; no fabricated contact or world force\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
