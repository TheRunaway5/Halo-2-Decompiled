#include "halo2/source_control_target.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
constexpr unsigned input_words=38,output_words=3;
using Input=std::array<std::uint32_t,input_words>;
using Output=std::array<std::uint32_t,output_words>;
Output run(const Input &in){
    unsigned at=0;
    const auto word=[&](){return in.at(at++);};
    const auto scalar=[&](){return std::bit_cast<float>(word());};
    const auto vector=[&](){const auto x=scalar(),y=scalar(),z=scalar();return Vec3{x,y,z};};
    const auto mode=word();require(mode<=2,"unknown control target probe mode");
    SourceControlTargetQuery q;
    q.unit_parent=word();q.unit_bound_center=vector();q.unit_bound_radius=scalar();
    q.unit_center=vector();q.unit_forward=vector();q.unit_root=vector();
    SourceControlTargetCandidate c;
    c.object=word();c.bound_center=vector();c.bound_radius=scalar();c.root=vector();
    const auto position_present=word();SourceDeviceGroup position{0,std::uint16_t(word())};
    const auto power_present=word();SourceDeviceGroup power{scalar(),0};
    c.position=position_present?&position:nullptr;c.power=power_present?&power:nullptr;
    c.device_flags=word();c.control_flags=word();c.marker_count=std::uint16_t(word());
    c.marker_forward=vector();
    const auto action=word();const auto target=word();const auto current_root=vector();
    require(at==in.size(),"control target probe input layout drifted");
    if(mode==2)return {0,source_control_initial_flags(c.control_flags),0xffffffffu};
    SourcePlayerActionTarget current{std::bit_cast<std::int16_t>(std::uint16_t(action)),
        std::bit_cast<std::int16_t>(std::uint16_t(action>>16)),target};
    if(mode==0)current={}; //151110 clears the full record before enumeration.
    const auto before=current;
    if(mode==0)source_control_target_candidate(q,c,current,current_root);
    else source_control_action_candidate(q,c,current,current_root);
    return {std::uint32_t(current!=before),std::uint32_t(std::uint16_t(current.action))|
        (std::uint32_t(std::uint16_t(current.index))<<16),current.object};
}
}
int main(int argc,char **argv){try{
    if(argc==2 && std::string_view(argv[1])=="--probe"){
        Input input;
        while(std::cin>>std::hex>>input[0]){
            for(unsigned i=1;i<input.size();++i)require(bool(std::cin>>input[i]),"truncated control target probe");
            for(const auto value:run(input))std::cout<<std::hex<<value<<' ';
            std::cout<<'\n';
        }
        return 0;
    }
    if(argc==3 && std::string_view(argv[1])=="--fixture"){
        std::ifstream file(argv[2]);require(bool(file),"missing control target fixture");
        std::string line;unsigned count=0;
        while(std::getline(file,line)){
            if(line.empty()||line.front()=='#')continue;
            std::istringstream row(line);Input input;Output expected;
            for(auto &v:input)require(bool(row>>std::hex>>v),"truncated target fixture input");
            for(auto &v:expected)require(bool(row>>v),"truncated target fixture output");
            std::string extra;require(!(row>>extra),"extra target fixture data");
            require(run(input)==expected,"original control target mismatch");++count;
        }
        require(count>0,"empty control target fixture");
        std::cout<<count<<" original control target cases matched\n";return 0;
    }
    SourceDeviceGroup position{0,1},power{1,0};
    require(source_device_usable(&position,&power,0),"fresh powered control denied");
    position.flags=3;require(!source_device_usable(&position,&power,0),"used one-use control admitted");
    require(!source_device_usable(nullptr,nullptr,0),"NONE position group admitted");
    bool rejected=false;
    try{source_device_usable(&position,nullptr,0);}catch(const FormatError&){rejected=true;}
    require(rejected,"missing power group fabricated");
    require(source_control_side_allowed(false,0,0,{},{}),"missing typed side lookup denied");
    require(source_control_side_allowed(true,1,1,{1,0,0},{1,0,0}),"both-sides control denied");
    require(!source_control_side_allowed(true,0,1,{1,0,0},{1,0,0}),"wrong control side admitted");
    require(source_control_side_allowed(true,0,1,{1,0,0},{0,1,0}),"perpendicular control side denied");
    SourcePlayerActionTarget current{3,0,0x10002};
    require(!source_player_action_target_replace(current,{3,0,0x10003},{},{1,0,0},{-1,0,0}),
        "equal distance reordered source candidates");
    require(source_player_action_target_replace(current,{3,0,0x10003},{},{2,0,0},{1,0,0}),
        "closer equal-priority source candidate lost");
    require(!source_player_action_target_replace(current,{1,0,0x10004},{},{},{}) && current.object==0x10003,
        "lower priority replaced control");
    std::cout<<"control target guards, power/side admission and ranking passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
