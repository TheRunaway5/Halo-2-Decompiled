#include "halo2/frontend_matchmaking_search.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
namespace halo2 {
namespace bridge=online_protocol_bridge;
struct FrontendMatchmakingSearch::Impl {
 rust::Box<bridge::FrontendSearch> state;
 Impl():state(bridge::new_frontend_search()){}
 explicit Impl(const bridge::FrontendSearch&other):state(bridge::clone_frontend_search(other)){}
};
FrontendMatchmakingSearch::FrontendMatchmakingSearch():impl_(std::make_unique<Impl>()){}
FrontendMatchmakingSearch::~FrontendMatchmakingSearch()=default;
FrontendMatchmakingSearch::FrontendMatchmakingSearch(const FrontendMatchmakingSearch&o):impl_(std::make_unique<Impl>(*o.impl_->state)){}
FrontendMatchmakingSearch&FrontendMatchmakingSearch::operator=(const FrontendMatchmakingSearch&o){if(this!=&o){auto p=std::make_unique<Impl>(*o.impl_->state);impl_.swap(p);}return *this;}
FrontendMatchmakingSearch::FrontendMatchmakingSearch(FrontendMatchmakingSearch&&)noexcept=default;
FrontendMatchmakingSearch&FrontendMatchmakingSearch::operator=(FrontendMatchmakingSearch&&)noexcept=default;
bool FrontendMatchmakingSearch::active()const{return bridge::frontend_search_scalar(*impl_->state,0)!=0;}
bool FrontendMatchmakingSearch::quick()const{return bridge::frontend_search_scalar(*impl_->state,1)!=0;}
bool FrontendMatchmakingSearch::pending()const{return bridge::frontend_search_scalar(*impl_->state,2)!=0;}
std::int32_t FrontendMatchmakingSearch::countdown()const{return std::int32_t(bridge::frontend_search_scalar(*impl_->state,3));}
unsigned FrontendMatchmakingSearch::title()const{return unsigned(bridge::frontend_search_scalar(*impl_->state,6));}
std::uint32_t FrontendMatchmakingSearch::clock()const{return std::uint32_t(bridge::frontend_search_scalar(*impl_->state,7));}
std::string FrontendMatchmakingSearch::selected()const{return std::string(bridge::frontend_search_selected(*impl_->state));}
void FrontendMatchmakingSearch::stop()const{bridge::frontend_search_stop(*impl_->state);}
void FrontendMatchmakingSearch::quick(bool value)const{bridge::frontend_search_quick(*impl_->state,value);}
void FrontendMatchmakingSearch::left()const{bridge::frontend_search_left(*impl_->state);}
void FrontendMatchmakingSearch::reset(std::uint32_t clock)const{bridge::frontend_search_reset(*impl_->state,clock);}
bool FrontendMatchmakingSearch::run(FrontendSearchAction action,const bridge::NativeFrontendSearchIo&io,unsigned index)const{return bridge::frontend_search_run(*impl_->state,unsigned(action),index,io);}
int FrontendMatchmakingSearch::ui(bridge::SearchSnapshot s)const{return bridge::frontend_search_ui(*impl_->state,std::move(s));}
unsigned FrontendMatchmakingSearch::pane(bridge::SearchSnapshot s)const{return bridge::frontend_search_pane(*impl_->state,std::move(s));}
}
