#include "halo2/frontend.hpp"
namespace halo2 {
void MenuState::set_local_discord_login_factory(std::function<std::shared_ptr<DiscordLogin>()>factory){local_discord_factory_=std::move(factory);}
bool MenuState::online_couch_screen(const MenuFrame&f)const{return frontend_couch_flow_screen(f.screen,f.context);}
bool MenuState::online_couch_mutable()const{return couch_flow_drive(0).handled;}
void MenuState::open_online_local_player(unsigned controller){couch_flow_drive(1,nullptr,controller);}
void MenuState::populate_online_couch(MenuFrame&f)const{couch_flow_drive(2,&f);}
std::string MenuState::online_couch_text(const MenuFrame&,unsigned i)const{return couch_flow_drive(3,nullptr,i).text;}
bool MenuState::accept_online_couch(){return couch_flow_drive(4).handled;}
void MenuState::update_online_couch(){couch_flow_drive(5);}
void MenuState::clear_online_couch(){couch_flow_drive(6);}
std::vector<OnlineLocalPlayer>MenuState::online_local_players()const{return couch_players();}
bool MenuState::couch_back(bool joining){return couch_flow_drive(joining?12:11).handled;}
bool MenuState::couch_profile_entry(unsigned screen,const std::string&name)const{return couch_flow_drive(14,nullptr,screen,0,false,name).handled;}
} // namespace halo2
