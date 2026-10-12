#include "halo2/vista_lobby_chat.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned nibble(char c) {
  if(c>='0' && c<='9')return unsigned(c-'0');
  if(c>='a' && c<='f')return unsigned(c-'a'+10);
  throw FormatError("Invalid original chat context hex");
}
std::vector<std::byte> context(const std::string& hex) {
  if(hex.size()%2)throw FormatError("Odd original chat context hex");
  std::vector<std::byte> out;
  for(std::size_t i=0;i<hex.size();i+=2)out.push_back(std::byte((nibble(hex[i])<<4)|nibble(hex[i+1])));
  return out;
}
}
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream input(argv[1]);Json rows;input>>rows;
  unsigned count{},actual_count{},boundary_count{},producer_count{},privilege_count{};
  for(const auto& row:rows) {
    auto ui=context(row.at("ui_context_hex"));const auto before=ui;
    const auto& calls=row.at("calls");Json actual_calls=Json::array();unsigned index{};
    VistaLobbyChatServices services;
    if(!calls.empty())services.check_privilege=[&](auto user,auto privilege,auto& allowed) {
      if(allowed!=0 || index>=calls.size())throw FormatError("Original chat privilege call initialization/order mismatch");
      const auto& expected=calls.at(index++);
      allowed=expected.at("allowed").get<std::uint32_t>();
      const auto hr=expected.at("hresult").get<std::uint32_t>();
      actual_calls.push_back({{"user",user},{"privilege",privilege},{"hresult",hr},{"allowed",allowed}});
      return hr;
    };
    const auto result=vista_lobby_chat_available(ui,services);
    if(unsigned(result)!=row.at("returned").get<unsigned>() || actual_calls!=calls || ui!=before) {
      std::cerr<<"case="<<count<<" expected="<<row.dump()<<" actual_calls="<<actual_calls.dump()<<'\n';
      throw FormatError("Original Vista lobby chat predicate mismatch");
    }
    const auto kind=row.at("kind").get<std::string>();
    if(kind=="actual_dll")++actual_count;
    else if(kind=="declared_privilege_service")++boundary_count;
    else if(kind=="source_produced_offline_context")++producer_count;
    else throw FormatError("Unknown original chat fixture provenance");
    privilege_count+=index;++count;
  }
  if(count!=76 || actual_count!=24 || boundary_count!=48 || producer_count!=4 || privilege_count!=58)
    throw FormatError("Incomplete original Vista lobby chat corpus");
  std::array<std::byte,12> ui{};ui[8]=std::byte{2};
  bool missing{};try{(void)vista_lobby_chat_available(ui,{});}catch(const FormatError&){missing=true;}
  if(!missing)throw FormatError("Missing consumed chat privilege service accepted");
  for(std::size_t size=0;size<12;++size) {
    bool short_context{};
    try{(void)vista_lobby_chat_available(std::span<const std::byte>(ui).first(size),{});}
    catch(const FormatError&){short_context=true;}
    if(!short_context)throw FormatError("Truncated chat presentation context accepted");
  }
  std::cout<<"PASS original Vista lobby chat cases="<<count<<" privilege_calls="<<privilege_count<<'\n';
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
