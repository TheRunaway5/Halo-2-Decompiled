#include "halo2/source_machine_door.hpp"
#include <bit>
#include <iostream>
#include <limits>
#include <vector>
using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
void oracle() {
    unsigned count;
    while(std::cin>>count) {
        std::vector<std::uint32_t> w(count);
        for(auto &value:w)std::cin>>std::hex>>value;
        std::cin>>std::dec;
        require(count>=24 && (count-24)%10==0,"invalid door oracle row");
        const auto real=[&](unsigned i){return std::bit_cast<float>(w.at(i));};
        SourceMachineDoorDefinition d{real(0),real(1)};
        SourceMachineDoorState s{w[2],std::bit_cast<std::int32_t>(w[3])};
        SourceMachineDoorInput in;in.handle=w[4];in.tick=w[5];in.editor=w[6];in.position_group_present=w[7];
        in.position=real(8);in.radius=real(9);in.center={real(10),real(11),real(12)};
        in.forward={real(13),real(14),real(15)};
        if(w[16])in.one_sided_marker=SourceMachineDoorMarker{
            {real(18),real(19),real(20)},{real(21),real(22),real(23)}};
        std::vector<SourceMachineDoorUnit> units;
        for(unsigned i=24;i<count;i+=10)units.push_back({w[i],w[i+1],w[i+2],w[i+3],std::uint8_t(w[i+4]),std::uint8_t(w[i+5]),{real(i+6),real(i+7),real(i+8)},real(i+9)});
        const auto result=source_machine_door_step(d,s,in,real(17),[&](float){return std::span<const SourceMachineDoorUnit>(units);});
        std::cout<<std::hex<<std::bit_cast<std::uint32_t>(s.open_ticks)<<' '<<result.queried<<' '<<std::bit_cast<std::uint32_t>(result.query_radius)<<' '<<result.target_count;
        for(unsigned i=0;i<result.target_count;++i)std::cout<<' '<<std::bit_cast<std::uint32_t>(result.position_targets[i]);
        std::cout<<'\n'<<std::dec;
    }
}
}
int main(int argc,char**)try {
    if(argc>1){oracle();return 0;}
    SourceMachineDoorDefinition definition{2.6f,1.5f};SourceMachineDoorState state;
    SourceMachineDoorInput input;input.handle=0x80010000;input.radius=2.6f;
    SourceMachineDoorUnit unit;unit.handle=0x80010001;
    const auto nearby=[&](float radius){require(radius==2.6f,"door activation radius changed");return std::span(&unit,1);};
    const auto empty=[](float){return std::span<const SourceMachineDoorUnit>{};};
    auto result=source_machine_door_step(definition,state,input,1.f/30,nearby);
    require(result.queried && result.target_count==1 && result.position_targets[0]==1,"closed automatic door did not request opening");
    //44 ticks rounds down to this authored float. Xbox compares that rounded
    //product; Vista's x87 product is slightly larger and closes a tick sooner.
    definition.open_seconds=44.f*(1.f/30);
    input.position=1;input.tick=1;state.open_ticks=43;
    result=source_machine_door_step(definition,state,input,1.f/30,empty);
    require(state.open_ticks==44 && !result.target_count,"Xbox timer closed on Vista's earlier rounding boundary");
    result=source_machine_door_step(definition,state,input,1.f/30,empty);
    require(state.open_ticks==45 && result.target_count==1 && result.position_targets[0]==0,"door did not close after authored delay");
    state.flags=4;input.position=0;input.tick=0;unit.center={1,0,0};
    require(!source_machine_door_step(definition,state,input,1.f/30,nearby).target_count,"one-sided door admitted wrong side");
    unit.center={-1,0,0};require(source_machine_door_step(definition,state,input,1.f/30,nearby).target_count==1,"one-sided door rejected allowed side");
    input.one_sided_marker=SourceMachineDoorMarker{{-2,0,0},{1,0,0}};
    require(!source_machine_door_step(definition,state,input,1.f/30,nearby).target_count,"door ignored its posed named marker");
    input.one_sided_marker.reset();
    state.flags=0;unit.unit_flags=0x4000;require(!source_machine_door_step(definition,state,input,1.f/30,nearby).target_count,"door admitted excluded unit definition");
    unit.unit_flags=0;unit.type=1;require(!source_machine_door_step(definition,state,input,1.f/30,nearby).target_count,"door admitted vehicle without opening flag");
    unit.vehicle_flags=0x100000;require(source_machine_door_step(definition,state,input,1.f/30,nearby).target_count==1,"door rejected eligible vehicle");
    state.flags=2;state.open_ticks=31;input.position=1;input.tick=1;
    source_machine_door_step(definition,state,input,1.f/30,empty);require(state.open_ticks==0,"disabled autoclose did not reset timer");
    std::cout<<"PASS automatic door opening, eligibility, side gates and Xbox timer boundary\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
