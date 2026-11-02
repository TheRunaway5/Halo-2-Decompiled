#include "halo2/frontend.hpp"
namespace halo2 {
void MenuState::set_discord_social(std::shared_ptr<DiscordFriends> social){discord_social_=std::move(social);(void)friends_flow_drive(0);}
bool MenuState::online_friends_screen(const MenuFrame&f)const{return frontend_friends_flow_screen(f.screen,f.context);}
bool MenuState::native_friends()const{return friends_flow_drive(1).handled;}
bool MenuState::online_friends_available()const{return friends_flow_drive(2).handled;}
void MenuState::update_online_friend_join(){account_flow_drive(true);}
void MenuState::populate_native_friends(MenuFrame&f)const{(void)friends_flow_drive(3,&f);}
void MenuState::populate_online_friends(MenuFrame&f)const{(void)friends_flow_drive(4,&f);}
bool MenuState::accept_native_friends(){return friends_flow_drive(5).handled;}
bool MenuState::accept_online_friends(){return friends_flow_drive(6).handled;}
void MenuState::update_online_friends(){(void)friends_flow_drive(7);}
std::string MenuState::online_friends_text(const MenuFrame&f,unsigned index)const{return friends_flow_drive(8,const_cast<MenuFrame*>(&f),index).text;}
std::string MenuState::online_friends_column(const MenuFrame&f,unsigned row,unsigned column)const{return friends_flow_drive(9,const_cast<MenuFrame*>(&f),row,column).text;}
} // namespace halo2
