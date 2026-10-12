#include "halo2/source_control.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message) {
    if(!value)throw FormatError(message);
}
std::array<std::uint32_t,6> run(const std::array<std::uint32_t,8> &input) {
    SourceControlDefinition control;
    control.type=std::uint16_t(input[0]);control.call_value=std::bit_cast<float>(input[1]);
    control.on=input[5];control.off=input[6];control.deny=input[7];
    SourceDeviceGroup group{std::bit_cast<float>(input[2]),std::uint16_t(input[3])};
    const auto result=source_control_activate(control,input[4]?&group:nullptr);
    return {std::uint32_t(result.attempted),std::uint32_t(result.changed),
        std::bit_cast<std::uint32_t>(result.requested),result.effect,
        std::bit_cast<std::uint32_t>(group.value),group.flags};
}
}
int main(int argc,char **argv) {try {
    if(argc==2 && std::string_view(argv[1])=="--probe") {
        std::array<std::uint32_t,8> input;
        while(std::cin>>std::hex>>input[0]) {
            for(unsigned i=1;i<input.size();++i)require(bool(std::cin>>input[i]),"truncated control probe");
            for(auto word:run(input))std::cout<<std::hex<<word<<' ';
            std::cout<<'\n';
        }
        return 0;
    }
    if(argc==3 && std::string_view(argv[1])=="--fixture") {
        std::ifstream file(argv[2]);require(bool(file),"missing retail control fixture");
        std::string line;unsigned count{};
        while(std::getline(file,line)) {
            if(line.empty() || line.front()=='#')continue;
            std::istringstream row(line);std::array<std::uint32_t,8> input;
            std::array<std::uint32_t,6> expected;
            for(auto &word:input)require(bool(row>>std::hex>>word),"truncated control input");
            for(auto &word:expected)require(bool(row>>word),"truncated control result");
            std::string extra;require(!(row>>extra),"trailing control fixture data");
            require(run(input)==expected,"original control/group result mismatch");++count;
        }
        require(count>0,"empty control fixture");
        std::cout<<count<<" original control/group cases matched\n";
        return 0;
    }
    if(argc==3 && std::string_view(argv[1])=="--assets") {
        std::ifstream file(argv[2]);require(bool(file),"missing control asset fixture");
        std::string line,previous;std::unique_ptr<Cache> cache;unsigned count{};
        while(std::getline(file,line)) {
            if(line.empty() || line.front()=='#')continue;
            std::istringstream row(line);std::string path;
            std::array<std::uint32_t,9> expected;
            require(bool(row>>path),"missing control map path");
            for(auto &word:expected)require(bool(row>>std::hex>>word),"truncated control asset row");
            std::string extra;require(!(row>>extra),"trailing control asset data");
            if(!std::filesystem::exists(path))return 77;
            if(path!=previous){cache=std::make_unique<Cache>(path);previous=path;}
            const auto d=cache->load_source_control_definition(expected[0]);
            const std::array<std::uint32_t,9> actual{d.datum,d.model,d.type,d.triggers_when,
                std::bit_cast<std::uint32_t>(d.call_value),d.action_string,d.on,d.off,d.deny};
            require(actual==expected,"original control asset mismatch");++count;
        }
        require(count==4,"expected all four authored multiplayer control placements");
        std::cout<<count<<" actual multiplayer control definitions matched\n";
        return 0;
    }
    SourceDeviceGroup gate{0,1};SourceControlDefinition control;
    control.on=1;control.off=2;control.deny=3;
    const auto opened=source_control_activate(control,&gate);
    require(opened.changed && opened.effect==1 && gate.value==1 && gate.flags==3,
        "one-use shared gate did not open");
    const auto repeated=source_control_activate(control,&gate);
    require(!repeated.changed && repeated.effect==3 && gate.value==1 && gate.flags==3,
        "one-use shared gate closed on repeated activation");
    require(!source_control_activate(control,nullptr).attempted,"NONE group activated");
    const auto retained=gate;
    control.type=4;
    bool rejected{};
    try {source_control_activate(control,&gate);}catch(const FormatError &){rejected=true;}
    require(rejected && gate.value==retained.value && gate.flags==retained.flags,
        "unsupported control mutated group");
    control.type=3;control.call_value=std::bit_cast<float>(0x7fc00000u);
    rejected=false;
    try {source_control_activate(control,&gate);}catch(const FormatError &){rejected=true;}
    require(rejected && gate.value==retained.value && gate.flags==retained.flags,
        "nonfinite control mutated group");
    std::cout<<"shared one-use group, repeated denial, NONE and rejection guards passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
