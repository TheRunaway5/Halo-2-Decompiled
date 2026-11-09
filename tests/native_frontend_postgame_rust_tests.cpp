#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <type_traits>
using namespace halo2;
using Json=nlohmann::json;
namespace bridge=halo2::online_protocol_bridge;
#include "frontend_postgame_helpers.inc"
namespace fixture {
struct NativeFailure:std::exception{const char*what()const noexcept override{return "native postgame callback failed";}};
std::exception_ptr injected;const NativeFailure*original_exception{};
struct Model {
 OnlineAccountSnapshot account_;int mode_=3;unsigned revision{};std::vector<MenuFrame>stack_;MenuFrame view;std::shared_ptr<const VistaPcrScreenOwner>online_prior_report_;
 struct AccountIo {
  Model &menu;
  bool accepted;
  bool party_command(const OnlinePartyCommand& command) {
   menu.call("party",command.action);
   return accepted;
  }
 };
 std::unique_ptr<AccountIo> online_account_;
 std::function<void()> system_link_leave_;
 std::vector<std::shared_ptr<const VistaPcrScreenOwner>>kept;std::map<unsigned,std::weak_ptr<const VistaPcrScreenOwner>>issued;std::map<const VistaPcrScreenOwner*,unsigned>ids;
 Json calls=Json::array();std::function<void()>reenter;std::function<bool()>host_probe;std::string reenter_on;bool in_reentry{},keep_alive{};int fail=-1;unsigned reentries{},reads{},next_id=100;
 Json id(const std::shared_ptr<const VistaPcrScreenOwner>&p)const{return p?Json(ids.at(p.get())):Json(nullptr);}
 Json encoded(const MenuFrame&f)const{return {f.screen,f.context,f.selected,f.instance,f.pane_created_clock,id(f.vista_pcr)};}
 Json storage()const{Json frames=Json::array(),live=Json::array();for(const auto&f:stack_)frames.push_back(encoded(f));for(const auto&[i,w]:issued)live.push_back({i,w.use_count()});return {{"frames",frames},{"view",encoded(view)},{"prior",id(online_prior_report_)},{"mode",mode_},{"revision",revision},{"live",live}};}
 void call(const std::string&name,Json args=nullptr){const auto i=reads++;calls.push_back({{"name",name},{"arguments",args},{"storage",storage()}});if(host_probe){(void)host_probe();++reentries;}if(int(i)==fail)std::rethrow_exception(injected);if(!in_reentry&&reenter&&name==reenter_on){in_reentry=true;try{reenter();in_reentry=false;}catch(...){in_reentry=false;throw;}}}
 const MenuFrame&frame()const{return stack_.back();}
 void back(){call("back");if(stack_.size()>1)stack_.pop_back();}
 void unavailable(std::string message){call("unavailable",message);}
 void update_online_account(){call("refresh_account");}
 void return_online(){
  call("return_online");
  stack_.clear();
  stack_.emplace_back().screen=6;
  stack_.emplace_back().screen=186;
  mode_=3;
 }
 void open_vista_pcr(unsigned s,std::shared_ptr<const VistaPcrScreenOwner>p){call("open",{s,id(p)});if(!p||!p->results)throw FormatError("Vista PCR factory requires actual bounded owner and source16/185 definition");MenuFrame f;f.screen=vista_menu(s);f.context=p->pane;f.selected=p->selected_row;f.vista_pcr=std::move(p);stack_.push_back(std::move(f));++revision;}
 std::shared_ptr<const VistaPcrScreenOwner>report(const Json&j,unsigned default_id){if(j.is_null())return {};auto p=std::make_shared<VistaPcrScreenOwner>();const auto i=j.value("id",default_id);issued[i]=p;ids[p.get()]=i;if(keep_alive)kept.push_back(p);p->online=j.value("online",true);p->selected_row=j.value("selected",0u);p->pane=j.value("pane",0u);if(j.value("results",true)){auto bank=std::make_shared<VistaPcrResults>();bank->player_count=j.value("count",2u);p->results=bank;}if(j.value("continue",true))p->input.continue_session=[this,i]{call("continue",i);};if(j.value("leave",false))p->leave_session=[this,i]{call("leave_session",i);};if(j.value("select",true))p->select_row=[this,i](unsigned row){call("select",{i,row});};if(j.contains("review"))p->lobby_review=report(j["review"],i+1000);return p;}
};
}
#define POSTGAME_METHODS bool online_postgame_host()const;void update_online_postgame();bool has_online_prior_stats()const;bool review_online_postgame();void close_online_prior_stats();void postgame_capture(unsigned,const MenuFrame&);void postgame_navigation(unsigned,const MenuFrame&);void confirm_postgame_leave();
namespace before {
using namespace fixture;
class MenuState:public Model{public:POSTGAME_METHODS};
#include "frontend_postgame_before.inc"
}
namespace after {
using namespace fixture;
class MenuState:public Model{public:POSTGAME_METHODS bool postgame_flow_drive(unsigned,const MenuFrame* =nullptr,unsigned=0)const;};
#include "frontend_postgame_after.inc"
}
namespace {
using namespace fixture;
MenuFrame frame(Model&m,const Json&j,unsigned default_id){MenuFrame f;f.screen=j.value("screen",14u);f.context=j.value("context",0u);f.selected=j.value("selected",0u);f.instance=j.value("instance",std::uint64_t(-1));f.pane_created_clock=j.value("clock",0xffffffffu);if(j.contains("report"))f.vista_pcr=m.report(j["report"],default_id);return f;}
void configure(Model&m,const Json&j){m.keep_alive=j.value("keep_alive",false);m.account_.authenticated=j.value("authenticated",true);m.mode_=j.value("mode",3);m.revision=j.value("revision",0u);if(j.value("party",true)){OnlineParty p;p.leader=j.value("leader",false);for(const auto&r:j.value("members",Json::array({Json{{"self",true},{"postgame",true}},Json{{"leader",true},{"postgame",false}}}))){OnlinePartyMember v;v.self=r.value("self",false);v.leader=r.value("leader",false);if(r.contains("postgame")&&!r["postgame"].is_null())v.postgame=r["postgame"].get<bool>();p.members.push_back(v);}m.account_.party=std::move(p);}m.online_prior_report_=m.report(j.value("prior",Json::object()),10);unsigned index=0;for(const auto&f:j.value("frames",Json::array({Json{{"screen",6}},Json{{"screen",14}}})))m.stack_.push_back(frame(m,f,20+index++));m.view=frame(m,j.value("view",Json{{"report",Json::object()}}),50);m.fail=j.value("fail",-1);m.reenter_on=j.value("reenter_on",std::string{});
 if(j.value("account",true))m.online_account_=std::make_unique<Model::AccountIo>(Model::AccountIo{m,j.value("leave_accepted",true)});
 if(j.value("system_link",false))m.system_link_leave_=[&m]{m.call("system_link_leave");};
}
template<class M>Json run_one(const Json&j){M m;configure(m,j);m.host_probe=[&]{return m.online_postgame_host();};m.reenter=[&]{const auto op=j.value("reenter",std::string("replace-prior"));if(op=="replace-prior")m.online_prior_report_=m.report(j.value("replacement",Json{{"id",71},{"selected",99}}),71);else if(op=="clear-stack")m.stack_.clear();else if(op=="reset-prior")m.online_prior_report_.reset();else if(op=="replace-frame")m.stack_.at(j.value("index",0u)).vista_pcr=m.report(j.value("replacement",Json{{"id",71}}),71);else if(op=="host")m.account_.authenticated=false;else if(op=="capture")m.postgame_capture(16,m.view);};Json result;
 try{const auto op=j.value("op",std::string("review"));if(op=="host")result["value"]=m.online_postgame_host();else if(op=="update")m.update_online_postgame();else if(op=="prior")result["value"]=m.has_online_prior_stats();else if(op=="review")result["value"]=m.review_online_postgame();else if(op=="close")m.close_online_prior_stats();else if(op=="capture")m.postgame_capture(j.value("id",16u),m.view);else if(op=="navigation")m.postgame_navigation(j.value("id",185u),m.view);else if(op=="leave")m.confirm_postgame_leave();else throw std::logic_error("Invalid postgame comparison operation");
 }catch(const NativeFailure&e){result["error"]="native";result["same_object"]=&e==original_exception;}catch(int n){result["error"]="int";result["integer"]=n;}catch(const std::exception&e){result["error"]="std";result["what"]=e.what();}
 result["storage"]=m.storage();result["calls"]=m.calls;result["reentries"]=m.reentries;return result;
}
Json run(const Json&j){try{if(j.value("nonstd",false))throw 73;throw NativeFailure{};}catch(const NativeFailure&e){injected=std::current_exception();original_exception=&e;}catch(...){injected=std::current_exception();original_exception=nullptr;}return {{"before",run_one<before::MenuState>(j)},{"rust",run_one<after::MenuState>(j)}};}
}
int main()try{for(std::string line;std::getline(std::cin,line);)std::cout<<run(Json::parse(line)).dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
