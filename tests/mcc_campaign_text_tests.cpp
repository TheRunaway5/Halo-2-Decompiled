#include "halo2/campaign_text.hpp"
#include <algorithm>
#include <iostream>

namespace {
void check(bool value,const char* message) {
  if(!value)throw std::runtime_error(message);
}
bool contains(const halo2::UiStrings& strings,const char* value) {
  return std::ranges::any_of(strings,[&](const auto& row){return row.second==value;});
}
}
int main(int argc,char** argv)try {
  if(argc<2)throw std::runtime_error("Supply MCC or original campaign-text maps");
  for(int arg=1;arg<argc;++arg) {
    const halo2::Cache cache(argv[arg]);
    const auto text=halo2::load_campaign_text(cache,argv[arg]);
    check(contains(text.training,"Picked up an Energy Sword"),"Campaign HUD locale lost the authored weapon message");
    check(text.panel.size()==text.images.size(),"Campaign training panel resources were lost");
    if(cache.is_mcc()) {
      check(text.training.size()==332,"MCC campaign training locale range changed");
      if(cache.name()=="03a_oldmombasa") {
        check(text.subtitles.size()==19&&text.title_strings.size()==4,"MCC local campaign locale ranges changed");
        check(contains(text.subtitles,"Regret. Regret. Regret."),"MCC local decoded subtitle data changed");
        check(contains(text.title_strings,"They'll Regret That Too"),"MCC local decoded chapter title data changed");
      }
    }
    std::cout<<cache.name()<<": "<<text.training.size()<<" training strings, "
        <<text.subtitles.size()<<" subtitles, "<<text.title_strings.size()<<" title strings, "
        <<text.images.size()<<" actual panel images\n";
  }
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
