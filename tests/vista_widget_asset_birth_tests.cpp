#include "halo2/vista_widget_asset_birth.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool value,const char* message) {++checks;if(!value)throw std::runtime_error(message);}
std::vector<std::byte> decode(const std::string& value) {
  std::vector<std::byte> result;check(value.size()%2==0,"bad hex extent");
  for(std::size_t i=0;i<value.size();i+=2)result.push_back(std::byte(std::stoul(value.substr(i,2),nullptr,16)));
  return result;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);out+=digits[value>>4];out+=digits[value&15];}
  return out;
}
struct Banks {
  std::map<std::uint32_t,std::vector<std::byte>> storage;
  std::span<std::byte> bytes(std::uint32_t at,std::size_t count) {
    auto it=storage.upper_bound(at);
    if(it==storage.begin())throw FormatError("unmapped source address");
    --it;const auto offset=std::uint64_t(at)-it->first;
    if(offset>it->second.size() || count>it->second.size()-std::size_t(offset) ||
        std::uint64_t(at)+count>0x100000000ull)throw FormatError("unmapped source extent");
    return std::span(it->second).subspan(std::size_t(offset),count);
  }
  std::uint32_t read(std::uint32_t at,std::size_t count) {
    const auto b=bytes(at,count);std::uint32_t value{};
    for(std::size_t i=0;i<count;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return value;
  }
  void mutate(const Json& mutations) {
    for(const auto& mutation:mutations) {
      const auto value=decode(mutation[1]);auto destination=bytes(mutation[0],value.size());
      std::copy(value.begin(),value.end(),destination.begin());
    }
  }
};
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)return 77;
    std::ifstream stream(argv[1]);check(bool(stream),"missing fixture");
    const auto fixture=Json::parse(stream);check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","exe pin");
    check(fixture.at("source_map_sha256")=="eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44","map pin");
    unsigned index{},faults{};
    for(const auto& expected:fixture.at("cases")) {
      try {
        const auto& q=expected.at("input");const auto id=VistaWidgetIdentity{q.at("identity")};
        Banks banks;banks.storage.emplace(id.value,decode(q.at("initial")));
        for(const auto& row:q.at("banks"))banks.storage.emplace(row[0],decode(row[1]));
        for(const auto& row:q.at("globals")) {
          auto value=row[1].get<std::uint32_t>();std::vector<std::byte> bytes(4);
          for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));
          banks.storage.emplace(row[0],std::move(bytes));
        }
        Json events=Json::array();unsigned clocks{};
        VistaWidgetAssetBirthServices services;
        services.mxcsr=q.at("mxcsr");
        services.bank=[&](auto widget){return banks.bytes(widget.value,0x200);};
        services.read=[&](auto at,auto count){return banks.read(at,count);};
        services.cached_ui_clock=[&] {
          events.push_back({{"kind","clock"},{"bank",hex(banks.bytes(id.value,0x200))}});
          banks.mutate(q.at("clock_mutations").at(clocks));return q.at("clocks").at(clocks++).template get<std::uint32_t>();
        };
        services.bitmap_residency=[&](auto datum) {
          events.push_back({{"kind","residency"},{"datum",datum},{"bank",hex(banks.bytes(id.value,0x200))}});
          banks.mutate(q.at("residency_mutations"));
        };
        bool rejected{};VistaWidgetIdentity result;
        try {
          result=q.at("owner")=="bitmap"?vista_widget_bitmap_construct(id,q.at("definition"),services):
              vista_widget_grid_construct(id,q.at("row"),q.at("definition"),services);
        }catch(const FormatError&){rejected=true;}
        check(rejected==!expected.at("fault").is_null(),"original fault boundary");
        if(!rejected)check(result.value==expected.at("result"),"returned identity");else ++faults;
        check(events==expected.at("events"),"callback order/snapshots");
        check(hex(banks.bytes(id.value,0x200))==expected.at("final").get<std::string>(),"full retained widget bank");
        for(const auto& row:expected.at("banks"))
          check(hex(banks.storage.at(row[0]))==row[1].get<std::string>(),"full retained source bank");
      }catch(const std::exception& e){throw std::runtime_error("case "+std::to_string(index)+": "+e.what());}
      ++index;
    }
    check(index==334,"case inventory");
    // Admission rejects unavailable services, truncated native storage and
    // wrapping identities instead of reading a substitute bank.
    VistaWidgetAssetBirthServices missing;std::vector<std::byte> short_bank(0x77);
    missing.bank=[&](auto){return std::span(short_bank);};
    bool rejected{};try{vista_widget_grid_construct({0x1000},0,0,missing);}catch(const FormatError&){rejected=true;}
    check(rejected,"short native bank admission");
    std::cout<<"PASS "<<checks<<" asset birth checks / "<<index<<" original cases / "<<faults<<" preserved fault prefixes\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
