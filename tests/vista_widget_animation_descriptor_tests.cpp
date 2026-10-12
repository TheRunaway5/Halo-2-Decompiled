#include "halo2/vista_widget_animation_descriptor.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <string_view>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(std::string_view raw) {
  const auto digit=[](char c)->unsigned {
    if(c>='0'&&c<='9')return unsigned(c-'0');
    if(c>='a'&&c<='f')return unsigned(c-'a')+10;
    if(c>='A'&&c<='F')return unsigned(c-'A')+10;
    throw FormatError("Invalid original animation hex digit");
  };
  if(raw.size()%2)throw FormatError("Invalid original animation hex extent");
  std::vector<std::byte> result(raw.size()/2);
  for(std::size_t i=0;i<result.size();++i)
    result[i]=std::byte{std::uint8_t(digit(raw[i*2])*16+digit(raw[i*2+1]))};
  return result;
}
std::string hex(std::span<const std::byte> raw) {
  constexpr char digits[]="0123456789abcdef";std::string result;
  for(const auto b:raw){const auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}
  return result;
}
std::uint32_t address(const Json& value) {
  const auto s=value.get<std::string>();return std::uint32_t(std::stoul(s,nullptr,16));
}
template<class F> void rejects(F&& f) {
  try{f();}catch(const FormatError&){return;}
  throw FormatError("Invalid original animation admission was accepted");
}
} // namespace

int main(int argc,char** argv) try {
  if(argc!=2)return 77;
  std::ifstream in(argv[1]);if(!in)return 77;
  Json fixture;in>>fixture;
  if(fixture.at("executable").at("sha256")!="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109" ||
      fixture.at("map").at("sha256")!="eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44")
    throw FormatError("Original animation fixture authority changed");
  std::size_t completed{},faults{},checks{},index{};
  for(const auto& row:fixture.at("cases")) {
    const auto& q=row.at("input");std::map<std::uint32_t,std::byte> registry;
    for(const auto& bank:row.at("source_reads")) {
      const auto at=address(bank.at("address"));const auto raw=unhex(bank.at("raw_hex").get<std::string>());
      for(std::size_t i=0;i<raw.size();++i)registry[at+std::uint32_t(i)]=raw[i];
    }
    auto descriptor=unhex(row.at("initial").get<std::string>());Json virtuals=Json::array(),writes=Json::array();
    const auto descriptor_address=row.at("descriptor_address").get<std::uint32_t>();
    VistaWidgetAnimationDescriptorServices services;
    std::uint32_t fault_address{};
    services.read=[&](std::uint32_t at,std::size_t bytes) {
      if(bytes!=2&&bytes!=4)throw FormatError("Unexpected original source read width");
      std::uint32_t value{};
      for(std::size_t i=0;i<bytes;++i) {
        const auto a=at+std::uint32_t(i);const auto found=registry.find(a);
        if(a>=descriptor_address && a-descriptor_address<descriptor.size()) {
          value|=std::uint32_t(std::to_integer<unsigned>(descriptor[a-descriptor_address]))<<(8*i);continue;
        }
        if(found==registry.end()){fault_address=a;throw FormatError("Unowned original animation registry read");}
        value|=std::uint32_t(std::to_integer<unsigned>(found->second))<<(8*i);
      }
      return value;
    };
    services.write_dword=[&](std::uint32_t at,std::uint32_t value) {
      writes.push_back(Json::array({at,4,value}));
      for(std::uint32_t i=0;i<4;++i)registry[at+i]=std::byte{std::uint8_t(value>>(8*i))};
    };
    const auto getroot=[&](std::uint32_t widget) {
      auto root=services.read(widget+0x10,4);
      if(root){while(const auto parent=services.read(root+0x10,4))root=parent;}
      else if(services.read(widget+4,4)==0)root=widget;
      return root;
    };
    services.invoke_virtual=[&](std::uint32_t widget,std::uint32_t slot)->std::uint32_t {
      virtuals.push_back(Json::array({widget,slot,hex(descriptor)}));
      if(slot==0x34 || slot==0x38) {
        const auto root=getroot(widget);
        if(!root&&slot==0x38)return 4;
        return services.read(root+(slot==0x34 ? 0x74u : 0x78u),4);
      }
      if(slot!=0x1c)throw FormatError("Unexpected original animation virtual slot");
      // The exact source getters selected by the fixture's original vtables.
      const auto type=q.value("widget",std::string("screen"));
      if(type=="screen")return 0;
      if(type=="row")return services.read(widget+0x74,4);
      if(type=="text")return services.read(widget+0x70,4);
      if(type=="bitmap") {
        const auto record=services.read(widget+0x70,4);
        if(!record)return 0;
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(services.read(record+6,2)))));
      }
      throw FormatError("Unowned original duration getter");
    };
    const auto type=q.value("widget",std::string("screen"));
    const std::uint32_t widget=0x1a00000u+(type=="row" ? 0x4000u : type=="text" ? 0x6000u : type=="bitmap" ? 0x8000u : 0u);
    std::uint8_t result{};bool rejected{};
    try {
      auto output=q.value("no_descriptor",false) ? std::span<std::byte>{} : std::span<std::byte>{descriptor};
      result=vista_widget_animation_descriptor(widget,q.value("index",0u),q.value("reason",0u),output,services);
    } catch(const FormatError& e) {
      rejected=true;
      if(row.at("completed").get<bool>()) {
        std::cerr<<"Case "<<index<<" "<<q.dump()<<" failed at "<<std::hex<<fault_address<<std::dec<<": "<<e.what()<<'\n';
        return 1;
      }
    }
    if(row.at("completed").get<bool>()) {
      if(rejected || result!=row.at("al").get<std::uint8_t>() || hex(descriptor)!=row.at("descriptor").get<std::string>()) {
        std::cerr<<"Case "<<index<<" descriptor differs "<<q.dump()<<'\n'<<hex(descriptor)<<'\n'<<row.at("descriptor")<<'\n';return 1;
      }
      if(virtuals!=row.at("virtuals") || writes!=row.at("source_writes")) {
        std::cerr<<"Case "<<index<<" callback/write order differs "<<q.dump()<<'\n'<<virtuals<<'\n'<<row.at("virtuals")<<'\n';return 1;
      }
      ++completed;checks+=5;
    } else {
      if(!rejected || fault_address!=0x38u || hex(descriptor)!=row.at("descriptor").get<std::string>())
        throw FormatError("Original invalid-bank fault boundary changed");
      ++faults;checks+=3;
    }
    ++index;
  }
  VistaWidgetAnimationDescriptorServices no_registry;
  std::vector<std::byte> small(0x33);
  rejects([&]{(void)vista_widget_animation_descriptor(0,0,0,small,no_registry);});++checks;
  std::cout<<"PASS: "<<completed<<" original completed cases, "<<faults<<" original memory-fault boundary, "<<checks<<" checks\n";
  return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
