#include "halo2/source_object_render_flags.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const std::string &message) {if(!value)throw FormatError(message);}
struct Row {
    std::string label;
    std::array<std::uint32_t,13> words{};
};
Row read(const std::string &line) {
    std::istringstream input(line);std::string kind;Row row;
    require(bool(input>>kind>>row.label) && kind=="F","unknown original flag31 row");
    for(auto &word:row.words)require(bool(input>>std::hex>>word),"short original flag31 row "+row.label);
    std::string extra;require(!(input>>extra),"trailing original flag31 row "+row.label);return row;
}
ObjectLightContactDefinition definition(const Row &row) {
    ObjectLightContactDefinition value;
    value.datum=row.words[0];value.model=row.words[1];value.render_model=row.words[2];
    value.collision=row.words[3];value.prt_count=row.words[4];value.section_render_leaves_count=row.words[5];
    return value;
}
void authored(const Cache &cache,const Row &row,const ObjectLightContactDefinition &literal) {
    const auto loaded=cache.load_object_light_contact_definition(literal.datum);
    require(bool(loaded),"missing actual authored flag31 definition "+row.label);
    require(loaded->datum==literal.datum && loaded->model==literal.model &&
        loaded->render_model==literal.render_model && loaded->collision==literal.collision &&
        loaded->prt_count==literal.prt_count && loaded->section_render_leaves_count==literal.section_render_leaves_count,
        "actual Cache flag31 metadata differs from original "+row.label);
}
void unresolved_control() {
    constexpr std::uint32_t before=0x91b357dfu;
    const SourceObjectRenderAttributionOutputs caller{0x13572468u,0x24681357u};
    bool rejected=false;
    try{(void)source_object_render_flag31_construction_event(before,std::nullopt,caller);}
    catch(const FormatError &){rejected=true;}
    require(rejected,"unresolved authored lookup became a false flag31 predicate");
}
}
int main(int argc,char **argv) {try {
    require(argc==3,"source flag31 test needs mainmenu.map and original event TSV");
    for(unsigned i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
    const Cache cache(argv[1]);std::ifstream input(argv[2]);require(bool(input),"original flag31 fixture missing");
    std::string line;std::size_t count{},available{},rejected{},signed_rejected{},preserved_outputs{},written_outputs{};
    std::set<std::uint32_t> actual;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        const auto row=read(line);const auto literal=definition(row);const auto &w=row.words;
        require(w[9]<=1,"original BF9A0 AL exceeds boolean range "+row.label);
        const SourceObjectRenderAttributionOutputs caller{w[6],w[7]},expected{w[10],w[11]};
        const auto result=source_object_render_flag31_construction_event(w[8],literal,caller);
        require(result.model_attribution_available==bool(w[9]) && result.attribution==expected && result.body_flags==w[12],
            "original BF9A0/B7B40 event differs "+row.label);
        require(((result.body_flags^w[8])&0x7fffffffu)==0,"flag31 event changed unrelated body flags "+row.label);
        if(row.label.starts_with("actual_")) {authored(cache,row,literal);actual.insert(literal.datum);}
        available+=result.model_attribution_available;rejected+=!result.model_attribution_available;
        if(literal.model!=0xffffffffu && literal.render_model!=0xffffffffu && literal.collision!=0xffffffffu) {
            ++written_outputs;
            signed_rejected+=std::bit_cast<std::int32_t>(literal.prt_count)<=0 ||
                std::bit_cast<std::int32_t>(literal.section_render_leaves_count)<=0;
        } else ++preserved_outputs;
        ++count;
    }
    require(input.eof(),"original flag31 fixture read failed");
    require(actual.size()==15 && count>15 && available && rejected && signed_rejected && preserved_outputs && written_outputs,
        "original flag31 fixture lacks authored/reference/signed-count coverage");
    unresolved_control();
    std::cout<<count<<" original BF9A0/B7B40 flag31 events / "<<actual.size()
        <<" actual authored definitions; signed counts, caller outputs and all32bodyflag bits exact; unresolved lookup rejects; CPU only\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
