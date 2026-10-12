#include "halo2/source_bsp_part_commands.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
struct Reader {
    std::span<const std::uint32_t> words;unsigned cursor{};
    std::uint32_t word(){if(cursor==words.size())throw FormatError("part command fixture truncated");return words[cursor++];}
    float real(){return std::bit_cast<float>(word());}
    void done(){if(cursor!=words.size())throw FormatError("part command fixture trailing words");}
};
void emit(std::vector<std::uint32_t>& out,const SourceBspPartCommandState& s,bool trace) {
    for(const auto& bank:s.banks) {
        out.insert(out.end(),bank.header.begin(),bank.header.end());out.push_back(unsigned(bank.storage.size()));
        for(const auto& c:bank.storage)out.insert(out.end(),c.words.begin(),c.words.end());
    }
    out.push_back(std::uint16_t(s.special_count));
    for(const auto& u:s.special_units)out.insert(out.end(),u.words.begin(),u.words.end());
    if(trace){out.push_back(unsigned(s.calls.size()));out.insert(out.end(),s.calls.begin(),s.calls.end());}
}
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> words,unsigned version) {
    Reader r{words};const auto selector=version>=2?r.word():0;
    if(version>=2 && r.word()!=0)throw FormatError("source command frame needs nonempty41C20 owner");
    SourceBspPartCommandContext c;c.mode=r.word();c.flag_mask=r.word();c.group=r.word();c.main_prefix=r.word();c.draw_count=r.word();c.mask_arena_address=r.word();
    SourceBspPartCommandWorld w;w.bsp_selector=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));c.player_multiplier=r.real();c.view_modifier=r.word()!=0;
    for(auto& f:c.lod_view.multipliers)f=r.real();
    std::vector<SourceCullDrawUnit> units(r.word());for(auto& u:units)for(auto& v:u.words)v=r.word();
    std::vector<std::uint8_t> coverage(units.size()),slots(units.size());for(auto& v:coverage)v=std::uint8_t(r.word());for(auto& v:slots)v=std::uint8_t(r.word());
    std::array<std::uint32_t,256> overrides;for(auto& v:overrides)v=r.word();c.coverage=coverage;c.override_slots=slots;c.overrides=overrides;
    SourceBspRangeArena arena;for(auto& v:arena.words)v=r.word();
    const auto geometry=[&](){std::vector<SourceBspCommandGeometry> bank(r.word());for(auto& g:bank) {g.resident=r.word()!=0;g.parts.resize(r.word());for(auto& p:g.parts)for(unsigned i=0;i<18;++i){const auto v=r.word();std::memcpy(p.data()+i*4,&v,4);}}return bank;};
    auto clusters=geometry(),definitions=geometry();w.clusters=clusters;w.definitions=definitions;
    std::vector<std::int16_t> instances(r.word());for(auto& v:instances)v=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));w.instance_definitions=instances;
    std::vector<SourceBspCommandMaterial> materials(r.word());for(auto& v:materials){v.shader=r.word();v.visibility_index=std::uint8_t(r.word());}w.materials=materials;
    const auto visibility_words=r.word();std::vector<std::byte> visibility(visibility_words*4);for(unsigned i=0;i<visibility_words;++i){const auto v=r.word();std::memcpy(visibility.data()+i*4,&v,4);}w.material_visibility=visibility;
    std::map<std::uint32_t,SourceBspCommandShader> shaders;const auto count=r.word();
    for(unsigned i=0;i<count;++i) {const auto id=r.word();auto& s=shaders[id];s.lod.bias=std::uint16_t(r.word());const auto n=r.word();for(unsigned j=0;j<n;++j)s.lod.thresholds.push_back(r.real());for(unsigned j=0;j<n;++j)s.layers.push_back(r.word());}
    w.shader=[&](std::uint32_t id)->const SourceBspCommandShader& {const auto it=shaders.find(id);if(it==shaders.end())throw FormatError("fixture shader service missing");return it->second;};
    SourceBspPartCommandState state;
    for(auto& bank:state.banks) {for(auto& v:bank.header)v=r.word();bank.storage.resize(r.word());for(auto& command:bank.storage)for(auto& v:command.words)v=r.word();}
    state.special_count=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));for(auto& u:state.special_units)for(auto& v:u.words)v=r.word();r.done();
    if(version>=2)source_bsp_build_part_commands(state,units,arena,w,c,selector,
        [&](auto&,auto mode){if(mode!=c.mode)throw FormatError("object-command mode differs");});
    else source_bsp_append_part_commands(state,units,arena,w,c);
    std::vector<std::uint32_t> out;emit(out,state,true);return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/effect_light_part_commands.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw FormatError("part command fixture unavailable");const auto size=f.tellg();if(size<12 || size%4)throw FormatError("part command fixture invalid length");std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);f.read(reinterpret_cast<char*>(data.data()),size);if(!f)throw FormatError("part command fixture read failed");
    Reader r{data};if(r.word()!=0x434d5042)throw FormatError("part command fixture signature");const auto version=r.word();if(version<1 || version>2)throw FormatError("part command fixture version");const auto count=r.word();std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i) {const auto ni=r.word(),no=r.word();if(std::uint64_t(r.cursor)+ni+no>data.size())throw FormatError("part command fixture row truncated");const auto result=solve(r.words.subspan(r.cursor,ni),version);r.cursor+=ni;if(result.size()!=no)throw FormatError("part command output length differs case"+std::to_string(i)+" got"+std::to_string(result.size())+" expected"+std::to_string(no));for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("part command mismatch case"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(result[j])+" expected"+std::to_string(r.words[r.cursor+j]));r.cursor+=no;checked+=no;}
    r.done();std::cout<<"PASS "<<count<<" original secondary BSP part-command cases / "<<checked<<" exact words\n";
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
