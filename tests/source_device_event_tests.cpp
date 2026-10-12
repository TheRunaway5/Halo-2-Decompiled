#include "halo2/source_device_event.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::uint32_t> run(std::span<const std::uint32_t> row){
    check(row.size()==8,"Device event requires eight input words");
    std::array<std::byte,0x144> body{};
    const auto write=[&](unsigned at,std::uint32_t value,unsigned n=4){
        for(unsigned i=0;i<n;++i)body[at+i]=std::byte(value>>(8*i));
    };
    write(0xc2,row[3],2);write(0xc4,row[4]);write(0xc8,row[5]);write(0x134,row[6]);write(0x140,row[7]);
    std::vector<std::uint32_t> result(16);unsigned phase=0;
    SourceDeviceEventServices services;
    services.object=[&](std::uint32_t owner){check(owner==row[2] && phase++==0,"Device event object order/identity");
        return std::span<const std::byte>(body);};
    services.tag_group=[&](std::uint32_t tag){check(tag==row[0] && phase++==1,"Device event tag order/identity");return row[1];};
    services.sound=[&](const SourceDeviceSoundRequest &s){
        check(phase++==2,"Device event sound dispatch order");
        result[0]=1;result[1]=s.tag;result[2]=s.object;result[3]=s.gain_bits;result[4]=s.marker;
        std::copy(s.position.begin(),s.position.end(),result.begin()+10);
        std::copy(s.direction.begin(),s.direction.end(),result.begin()+13);
    };
    services.effect=[&](const SourceDeviceEffectRequest &s){
        check(phase++==2,"Device event effect dispatch order");
        result[0]=2;result[1]=s.tag;result[2]=s.object;result[3]=s.power_bits;result[4]=s.position_bits;
        for(unsigned i=0;i<10;++i)result[5+i/4]|=std::uint32_t(std::to_integer<unsigned>(s.location[i]))<<(8*(i%4));
        result[8]=s.argument_b;result[9]=s.argument_c;
    };
    dispatch_source_device_event(row[0],row[2],services);
    check(phase==(row[0]==~0u?0u:result[0]?3u:2u),"Device event service count");return result;
}
void guards(){
    dispatch_source_device_event(~0u,~0u,{});
    bool threw=false;try{dispatch_source_device_event(0x10001,0x10002,{});}catch(const std::runtime_error &){threw=true;}
    check(threw,"Missing actual device owner was accepted");
    std::array<std::byte,0xc8> truncated{};SourceDeviceEventServices services;
    services.object=[&](std::uint32_t){return std::span<const std::byte>(truncated);};
    services.tag_group=[](std::uint32_t){throw std::runtime_error("Tag selected before required body reads");return 0u;};
    threw=false;try{dispatch_source_device_event(1,2,services);}catch(const std::runtime_error &e){
        threw=std::string_view(e.what())=="Truncated source device event body";}
    check(threw,"Truncated body read extent changed");
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream fixture;
    if(!probe){guards();if(argc!=3 || std::string_view(argv[1])!="--fixture")throw std::runtime_error("Expected --probe or --fixture PATH");
        fixture.open(argv[2]);check(bool(fixture),"Missing device event fixture");}
    auto &input=probe?std::cin:fixture;std::string line;unsigned count=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        std::istringstream stream(line);std::vector<std::uint32_t> row;std::uint32_t value;
        while(stream>>std::hex>>value)row.push_back(value);
        check(stream.eof() && row.size()==(probe?8:24),"Invalid device event fixture row");
        const auto result=run(std::span(row).first(8));
        if(probe){for(auto word:result)std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
        else check(std::equal(result.begin(),result.end(),row.begin()+8),"Original device event comparison failed");
        ++count;
    }
    if(!probe){check(count>=1000,"Device event fixture lost coverage");std::cout<<count<<" unchanged original107980 dispatch cases passed\n";}
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
