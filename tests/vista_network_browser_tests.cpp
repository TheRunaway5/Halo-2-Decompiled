#include "halo2/vista_network_browser.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
int main(int argc,char**argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream file(argv[1]);Json rows;file>>rows;unsigned count{};
  for(const auto&row:rows) {
    const auto&q=row.at("input");std::uint32_t players=q.at("players"),teams=q.at("teams");Json events=Json::array();
    VistaBrowserCreateServices s;
    s.disconnect=[&](bool value){events.push_back({"disconnect",value?1:0});};
    s.clear_report_counts=[&](){events.push_back({"clear_report"});players=teams=0;};
    s.birth_local=[&](){events.push_back({"birth",q.at("birth")});return q.at("birth").get<unsigned>()!=0;};
    s.configure_selected=[&](auto kind){events.push_back({"configure",kind});};
    const auto created=vista_network_browser_create_local(s);
    if(created!=(q.at("birth").get<unsigned>()!=0) || events!=row.at("events") ||
        players!=row.at("players") || teams!=row.at("teams"))throw FormatError("Original PC13 Create callback sequence mismatch");
    ++count;
  }
  if(count!=48)throw FormatError("Incomplete original browser Create corpus");
  unsigned effects{};VistaBrowserCreateServices missing;missing.disconnect=[&](bool){++effects;};bool rejected{};
  try{(void)vista_network_browser_create_local(missing);}catch(const FormatError&){rejected=true;}
  if(!rejected || effects)throw FormatError("Missing Create owner mutated the live session");
  std::cout<<"PASS original Vista PC13 Create callback cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
