#include "halo2/campaign_text.hpp"
#include "halo2/content.hpp"
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
  if(argc==2 && !std::filesystem::is_regular_file(argv[1]))return 77;
  if(argc<2)throw std::runtime_error("Supply MCC or original campaign-text maps");
  for(int arg=1;arg<argc;++arg) {
    const halo2::Cache cache(argv[arg]);
    const auto text=halo2::load_campaign_text(cache,argv[arg]);
    check(contains(text.training,"Picked up an Energy Sword"),"Campaign HUD locale lost the authored weapon message");
    check(text.panel.size()==text.images.size(),"Campaign training panel resources were lost");
    if(cache.is_mcc()) {
      check(text.training.size()==332,"MCC campaign training locale range changed");
      if(cache.name()=="00a_introduction") {
        check(text.titles.size()==2 && text.title_strings.size()==2,"Opening location-title bank changed");
        check(contains(text.title_strings,"COVENANT HOLY CITY, HIGH CHARITY|nNINTH AGE OF RECLAMATION"),"Opening MCC location-title text changed");
        for(const auto& title:text.titles)check(title.bounds==std::array<std::int16_t,4>{350,0,480,595} &&
            title.justification==1 && title.font==2 && title.fade_in==1 && title.up_time==5 && title.fade_out==1,
            "Actual MCC opening title still has incompatible Xbox bounds or altered timing");
        const auto content=cache.load_content(0,false);
        check(content.cinematic_titles.size()==text.titles.size(),"Content/text title banks disagree");
        for(unsigned i=0;i<2;++i)check(content.cinematic_titles[i].bounds==text.titles[i].bounds &&
            content.cinematic_titles[i].text==text.titles[i].text,"Content/text title projection seams disagree");
        const auto* scenario=cache.query_tag({.group=halo2::TagGroup::from_string("scnr"),.require_address=true});
        check(scenario,"Missing opening scenario owner");
        const halo2::Bytes header(cache.read_tag_bytes(scenario->address,scenario->size));
        const halo2::Bytes raw(cache.read_tag_bytes(header.u32(0x1f4),72));
        check(raw.u32(0)==0x100009a2 && raw.u32(36)==0x100009a3 && raw.u16(4)==225 &&
            raw.u16(6)==1024 && raw.u16(8)==40 && raw.u16(10)==80,"Projection mutated MCC source title records");
      }
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
