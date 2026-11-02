#include "halo2/frontend.hpp"
namespace halo2 {
void MenuState::set_online_account(std::shared_ptr<OnlineAccount>client){clans_bind_account(std::move(client));}
void MenuState::set_online_voice(std::shared_ptr<OnlineVoice>client){clans_bind_voice(std::move(client));}
void MenuState::voice_push_to_talk(bool held){clans_flow_drive(2,nullptr,0,0,held);}
bool MenuState::online_clan_screen(const MenuFrame&f)const{return frontend_clans_flow_screen(f.screen,f.context);}
void MenuState::update_online_account(){account_flow_drive(false);}
void MenuState::populate_online_clans(MenuFrame&f)const{clans_flow_drive(3,&f);}
std::string MenuState::online_clan_column(const MenuFrame&f,unsigned row,unsigned column)const{return clans_flow_drive(4,const_cast<MenuFrame*>(&f),row,column).text;}
std::string MenuState::online_clan_text(const MenuFrame&f,unsigned i)const{return clans_flow_drive(5,const_cast<MenuFrame*>(&f),i).text;}
void MenuState::clan_command(OnlineClanCommand c){clans_flow_drive(6,nullptr,0,0,false,std::move(c));}
void MenuState::confirm_clan(OnlineClanCommand c,std::string message){clans_flow_drive(7,nullptr,0,0,false,std::move(c),std::move(message));}
bool MenuState::accept_online_clans(){return clans_flow_drive(8).handled;}
bool MenuState::clan_message_advance(){return clans_flow_drive(9).handled;}
} // namespace halo2
