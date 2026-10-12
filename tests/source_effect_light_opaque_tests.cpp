#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_bsp_admission.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
struct Reader {std::span<const std::uint32_t> data;unsigned cursor{};std::uint32_t word(){if(cursor==data.size())throw FormatError("opaque selection fixture truncated");return data[cursor++];}};
}
int main(int argc,char** argv)try {
    const char* fixture=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/effect_light_opaque_selection.original.bin";
    const char* map=argc>2?argv[2]:HALO2_SOURCE_DIR "/assets/disc/maps/lockout.map";
    auto cache=std::make_shared<Cache>(map);SourceTagMetadataStore store(cache,{"shad","stem","spas","vrtx"});const auto original=store.providers();
    unsigned aliases=0;
    for(const auto& tag:cache->tags())if(tag.group=="shad" && tag.address && !tag.size) {
        bool rejected=false;try{(void)original.definition(tag.datum);}catch(const FormatError&){rejected=true;}
        if(!rejected)throw FormatError("empty shader alias manufactured resident payload");++aliases;
    }
    if(!aliases)throw FormatError("Lockout zero-sized shader aliases missing");
    const auto bsp=cache->load_source_bsp_admission(0);
    using Key=std::pair<MenuWorldSourceSectionKind,unsigned>;
    std::map<Key,SourceBspRangeGeometry> geometries;std::map<Key,SourceOpaqueGeometryMetadata> selectors;
    const auto geometry=[&](const SourceCullDrawUnit& unit) {
        if(unit.words[1]!=0xffffffffu)throw FormatError("opaque fixture dynamic geometry missing");
        unsigned section=unit.words[3]>>9&511u;auto kind=MenuWorldSourceSectionKind::bsp_cluster;
        if(unit.words[0]&0x1000u) {const auto instance=unit.words[3]>>18&2047u;if(instance>=bsp.instances.size() || bsp.instances[instance].definition<0)throw FormatError("opaque fixture instance missing");section=unsigned(bsp.instances[instance].definition);kind=MenuWorldSourceSectionKind::bsp_instance_definition;}
        const Key key{kind,section};if(!geometries.contains(key)) {geometries.emplace(key,load_source_bsp_range_geometry(*cache,0,kind,section));selectors.emplace(key,load_source_opaque_bsp_geometry_metadata(*cache,0,kind,section));}
        return SourceOpaqueGeometrySelection{selectors.at(key),geometries.at(key).parts};
    };
    std::ifstream f(fixture,std::ios::binary|std::ios::ate);if(!f)throw FormatError("opaque selection fixture unavailable");const auto size=f.tellg();if(size<12 || size%4)throw FormatError("opaque selection fixture length");std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);f.read(reinterpret_cast<char*>(data.data()),size);if(!f)throw FormatError("opaque fixture read failed");Reader r{data};if(r.word()!=0x534c504f)throw FormatError("opaque selection fixture signature");const auto version=r.word();if(version!=1 && version!=2)throw FormatError("opaque selection fixture version");const auto count=r.word();std::uint64_t checked=0,selections=0;
    for(unsigned case_index=0;case_index<count;++case_index) {
        std::vector<SourceCullDrawUnit> units(r.word());for(auto& u:units)for(auto& v:u.words)v=r.word();
        SourcePartCommandBank bank;for(auto& v:bank.header)v=r.word();bank.storage.resize(r.word());for(auto& c:bank.storage)for(auto& v:c.words)v=r.word();
        // The baseline is independent Cache metadata. Only declared original
        // data mutations overlay it; no source instruction/callee is replaced.
        std::map<std::uint32_t,std::vector<std::byte>> overlays;const auto patch_count=r.word();
        for(unsigned p=0;p<patch_count;++p) {const auto at=r.word(),n=r.word();auto& bytes=overlays[at];bytes.resize(n);for(auto& b:bytes)b=std::byte(r.word());}
        std::map<std::pair<std::uint32_t,std::size_t>,std::vector<std::byte>> reads;
        auto metadata=original;metadata.read=[&](std::uint32_t at,std::size_t n)->std::span<const std::byte> {
            auto [it,inserted]=reads.try_emplace({at,n});if(inserted) {const auto raw=original.read(at,n);it->second.assign(raw.begin(),raw.end());for(const auto& [start,patch]:overlays)for(unsigned j=0;j<patch.size();++j)if(std::uint64_t(start)+j>=at && std::uint64_t(start)+j<std::uint64_t(at)+n)it->second[start+j-at]=patch[j];}return it->second;
        };
        std::vector<SourceOpaqueSortRecord> records(r.word());for(auto& record:records)for(auto& v:record.words)v=r.word();
        const auto selected=source_effect_light_opaque_select(bank,units,metadata,[&](unsigned i){return geometry(units.at(i));},records);
        if(selected!=r.word())throw FormatError("opaque selected count differs case"+std::to_string(case_index));selections+=selected;
        for(unsigned i=0;i<records.size();++i)for(unsigned j=0;j<6;++j) {const auto wanted=r.word();++checked;if(records[i].words[j]!=wanted)throw FormatError("opaque selection differs case"+std::to_string(case_index)+" record"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(records[i].words[j])+" expected"+std::to_string(wanted));}
    }
    unsigned dispatch=0;
    if(version==2) {dispatch=r.word();for(unsigned i=0;i<dispatch;++i) {const auto selector=std::uint16_t(r.word());const auto auxiliary=std::bit_cast<std::int32_t>(r.word());const auto classes=r.word(),expected=r.word();unsigned consumed=0;const auto got=source_opaque_vertex_class(selector,auxiliary,[&]{++consumed;return classes;});if(got!=expected || consumed!=unsigned(selector==3))throw FormatError("source opaque vertex dispatcher mismatch");}}
    if(r.cursor!=data.size())throw FormatError("opaque selection trailing words");std::cout<<"PASS "<<count<<" original sphere opaque selections / "<<selections<<" selected records / "<<checked<<" exact words / "<<dispatch<<" vertex dispatcher cases; native Cache metadata and geometry\n";
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
