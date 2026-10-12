#include "halo2/source_local_player_priority.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace halo2;
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::vector<std::uint32_t> words(const std::string &text){std::istringstream input(text);std::vector<std::uint32_t> result;std::string word;
    while(input>>word){std::size_t n;const auto v=std::stoul(word,&n,16);check(n==word.size()&&v<=0xfffffffful,"invalid priority fixture word");result.push_back(std::uint32_t(v));}return result;}
std::array<std::uint8_t,5> execute(const std::vector<std::uint32_t> &row){
    std::size_t at=0;auto take=[&](){check(at<row.size(),"truncated priority fixture");return row[at++];};
    check(take()==1,"unknown priority fixture version");const auto count=take(),owner=take(),slot=take(),special=take();
    check(count<=64&&special<=255,"priority fixture count/byte out of range");
    std::array<SourcePriorityObserver,4> observers;std::array<std::optional<std::uint32_t>,4> players;
    for(auto &observer:observers){observer.callback=take();observer.scalar_bits=take();}
    for(auto &player:players)player=take();
    std::vector<SourcePriorityOwner> owners;
    for(unsigned i=0;i<count;++i){SourcePriorityOwner value;value.identity=take();value.parent=take();const auto kind=take();check(kind<=255,"priority fixture kind out of range");value.header_kind=std::uint8_t(kind);value.player=take();owners.push_back(value);}
    check(at==row.size(),"extra priority fixture input");
    std::array<std::uint8_t,5> result;
    for(unsigned i=0;i<4;++i)result[i]=source_priority_observer_class(observers[i],std::uint8_t(special));
    result[4]=source_local_player_priority(owners,owner,{std::bit_cast<std::int32_t>(slot),observers,players,std::uint8_t(special)});
    return result;
}
void controls(){
    constexpr auto none=0xffffffffu;
    std::vector<SourcePriorityOwner> owners{{0xe2e60077u,none,0,0xec700000u}};
    std::array<SourcePriorityObserver,4> observers;for(auto &o:observers)o={0x23c110u,0};
    std::array<std::optional<std::uint32_t>,4> players{0xec700000u,none,none,none};
    SourceLocalPlayerPriorityContext context{0,observers,players,0};
    std::size_t rejected=0,unread=0;
    auto reject=[&](const auto &o,const auto &c){bool error=false;try{(void)source_local_player_priority(o,owners[0].identity,c);}catch(const FormatError&){error=true;}check(error,"unresolved priority input accepted");++rejected;};
    auto c=context;c.local_slot.reset();reject(owners,c);
    for(int bad:{-2,4,9}){c=context;c.local_slot=bad;reject(owners,c);}
    c=context;c.observers={};reject(owners,c);
    auto obs=observers;obs[0].callback.reset();c=context;c.observers=obs;reject(owners,c);
    obs=observers;obs[0].scalar_bits.reset();c.observers=obs;reject(owners,c);
    obs=observers;obs[0].callback=0x23d090u;c=context;c.observers=obs;c.observer_special.reset();reject(owners,c);
    for(unsigned f=0;f<3;++f){auto o=owners;if(f==0)o[0].parent.reset();if(f==1)o[0].header_kind.reset();if(f==2)o[0].player.reset();reject(o,context);}
    auto o=owners;o[0].parent=0xe0000140u;reject(o,context);o[0].parent=o[0].identity;reject(o,context);
    o=owners;o.push_back(o[0]);reject(o,context);
    c=context;c.local_players={};reject(owners,c);
    auto p=players;p[0].reset();c=context;c.local_players=p;reject(owners,c);
    check(source_local_player_priority({},none,{})==0,"ownerNONE read context");++unread;
    c={};c.local_slot=-1;check(source_local_player_priority({},owners[0].identity,c)==0,"slotNONE read observer/owner");++unread;
    obs=observers;obs[0]={0x23cbb0u,{}};c=context;c.observers=obs;c.local_players={};c.observer_special.reset();check(source_local_player_priority({},owners[0].identity,c)==0,"nonzero observer class read owner/local players");++unread;
    o=owners;o[0].header_kind=1;o[0].player.reset();c=context;c.local_players={};check(source_local_player_priority(o,owners[0].identity,c)==0,"nonunit root read player");++unread;
    o=owners;o[0].player=none;p=players;p[0]=none;c=context;c.local_players=p;check(source_local_player_priority(o,owners[0].identity,c)==1,"explicit playerNONE comparison was changed");++unread;
    for(auto callback:{0x23cbb0u,0x16c840u,0xdeadbeefu}){check(source_priority_observer_class({callback,{}},{})!=0,"unread observer scalar required");++unread;}
    std::cout<<rejected<<" unresolved controls and "<<unread<<" source early/read-branch controls pass\n";
}
}
int main(int argc,char **argv){try{
    if(argc==2&&std::string_view(argv[1])=="--probe"){std::string line;while(std::getline(std::cin,line)){if(line.empty())continue;const auto out=execute(words(line));for(unsigned i=0;i<out.size();++i)std::cout<<(i?" ":"")<<std::hex<<unsigned(out[i]);std::cout<<'\n';}return 0;}
    check(argc==1,"expected optional --probe");controls();
    std::ifstream file(std::filesystem::path(HALO2_SOURCE_DIR)/"tests/fixtures/retail_source_local_player_priority.tsv");check(bool(file),"original priority fixture missing");
    std::string line;std::size_t cases=0;std::array<bool,4> classes{};bool yes=false,no=false;
    while(std::getline(file,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');check(split!=std::string::npos,"priority fixture lacks outputs");const auto expected=words(line.substr(split+1));check(expected.size()==5,"priority fixture output count");
        const auto actual=execute(words(line.substr(0,split)));for(unsigned i=0;i<5;++i)check(actual[i]==expected[i],"original priority mismatch case "+std::to_string(cases));
        for(unsigned i=0;i<4;++i){check(actual[i]<=3,"observer class out of range");classes[actual[i]]=true;}yes|=actual[4]==1;no|=actual[4]==0;++cases;}
    check(cases>=1000&&yes&&no&&classes==std::array<bool,4>{true,true,true,true},"priority fixture coverage incomplete");
    std::cout<<cases<<" unchanged original 3E9C0/155760/BAF80 cases, "<<cases*5<<" AL/class bytes exact; explicit local-user/observer and ownership inputs\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
