#include "halo2/source_transparent_sort.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
void require(bool yes,const char *message){if(!yes)throw std::runtime_error(message);}
Words evaluate(const Words &row) {
    unsigned at=0;const auto word=[&]{return row.at(at++);};
    const auto real=[&]{return std::bit_cast<float>(word());};
    const auto vector=[&]{return Vec3{real(),real(),real()};};
    const auto command=[&]{std::array<std::uint32_t,28> v;
        for(auto &w:v)w=word();
        return std::bit_cast<SourceTransparentCommand>(v);};
    Words result;const auto append=[&](const SourceTransparentCommand &c){std::array<std::uint32_t,28> v;
        std::memcpy(v.data(),&c,sizeof(c));result.insert(result.end(),v.begin(),v.end());};
    const auto mode=word();
    if(mode==0){const auto n=word(),first=word();const auto eye=vector();
        std::vector<SourceTransparentCommand> commands;for(unsigned i=0;i<n;++i)commands.push_back(command());
        const auto order=source_transparent_sort(commands,eye,first);result.push_back(unsigned(order.size()));
        if(n<=64)for(const auto &a:commands)for(const auto &b:commands)result.push_back(source_transparent_compare(a,b,eye));
        for(auto id:order)result.push_back(id);
    }else if(mode==1){const auto n=word(),existing=word(),limit=word(),argument=word();
        std::vector<SourceTransparentSection> sections(n);for(auto &s:sections){s.depth=real();s.center=vector();for(auto &w:s.rectangle)w=real();s.kind=word();}
        std::vector<SourceTransparentCommand> commands;for(unsigned i=0;i<existing;++i)commands.push_back(command());
        source_transparent_append_groups(sections,commands,argument,limit);result.push_back(unsigned(commands.size()));
        for(const auto &c:commands)append(c);
    }else if(mode==2){SourceTransparentPartInput p;p.selected_template_layers=word();p.part_type=std::uint16_t(word());p.material_present=word();
        p.view_type=word();p.grouped_context=word();p.shader_flags=std::uint16_t(word());p.selected_shader=word();p.draw_token=word();
        p.center=vector();p.camera_position=vector();p.camera_forward=vector();p.depth_bias=real();p.plane_present=word();for(auto &w:p.plane)w=real();
        const auto c=source_transparent_part_command(p);result.push_back(bool(c));if(c)append(*c);
    }else throw std::runtime_error("unknown transparent fixture operation");
    require(at==row.size(),"transparent fixture trailing input");return result;
}
void boundaries() {
    std::vector<SourceTransparentCommand> commands(9);for(auto &c:commands)c.depth=0;
    const auto order=source_transparent_sort(commands,{});
    require(order!=std::vector<std::uint16_t>{0,1,2,3,4,5,6,7,8},"equal transparent keys incorrectly used stable sorting");
    require(source_transparent_sort(commands,{},9).empty(),"empty command suffix mutated source order");
    bool rejected=false;try{source_transparent_sort(commands,{},10);}catch(const std::invalid_argument &){rejected=true;}
    require(rejected,"invalid command range silently accepted");
    std::vector<SourceTransparentSection> sections(32);rejected=false;
    try{source_transparent_append_groups(sections,commands,0);}catch(const std::invalid_argument &){rejected=true;}
    require(rejected,"sort-section31 NONE entered populated queue");
    sections.resize(2);sections[0].kind=1;sections[1].kind=3;
    sections[0].rectangle={0,1,0,1};sections[1].rectangle={1,2,0,1};commands.clear();
    source_transparent_append_groups(sections,commands,42);
    require(commands.size()==2 && commands[0].payload[0]==2 && commands[1].callback==0x423c0,
        "kind3 candidate or touching rectangle lost original duplicate-group semantics");
    commands.clear();source_transparent_append_groups(sections,commands,42,1);
    require(commands.size()==1,"group builder ignored remaining source command capacity");
    std::array<std::uint32_t,28> retained;
    for(unsigned i=0;i<retained.size();++i)retained[i]=0x89abcdefu+i*0x1020304u;
    commands={std::bit_cast<SourceTransparentCommand>(retained)};
    source_transparent_append_groups(sections,commands,42,3);
    require(std::bit_cast<std::array<std::uint32_t,28>>(commands.front())==retained,
        "group append changed an existing opaque callback/payload bank");
}
}
int main(int argc,char **argv){try{
    require(argc==2,"transparent fixture required");const bool probe=std::string(argv[1])=="--probe";
    std::ifstream file;if(!probe){file.open(argv[1]);require(bool(file),"transparent source fixture missing");}
    auto &stream=probe?std::cin:file;std::string line;unsigned count=0;
    while(std::getline(stream,line)){if(line.empty() || line[0]=='#')continue;std::istringstream input(line);input>>std::hex;
        unsigned n;require(bool(input>>n),"transparent input count missing");Words row(n);for(auto &w:row)require(bool(input>>w),"transparent input truncated");
        const auto result=evaluate(row);if(probe){for(auto w:result)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
        else{for(auto w:result){unsigned expected;require(bool(input>>expected),"transparent expected output missing");
            if(w!=expected)throw std::runtime_error("original transparent mismatch case"+std::to_string(count));}
            std::string extra;require(!(input>>extra),"transparent fixture trailing output");}++count;}
    require(count!=0,"empty transparent fixtures");if(!probe){boundaries();std::cout<<count<<" original transparent comparator/sort/group/part cases matched\n";}
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
