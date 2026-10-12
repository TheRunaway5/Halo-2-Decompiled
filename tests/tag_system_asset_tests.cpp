#include "halo2/tags/system.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>

namespace {
using namespace halo2;
void check(bool ok,const char* why) {if(!ok)throw std::runtime_error(why);}
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"Supply repository root");
        const std::filesystem::path root=argv[1];
        if(!std::filesystem::exists(root/"assets/disc/maps/zanzibar.map") ||
           !std::filesystem::exists(root/"Halo 2 Vista installer/maps/mainmenu.map")) return 77;
        std::vector<std::filesystem::path> maps;
        for(const auto& dir : {root/"assets/disc/maps",root/"assets/map-pack/maps",
                              root/"assets/multiplayer-map-pack/maps",root/"Halo 2 Vista installer/maps"}) {
            if(!std::filesystem::is_directory(dir)) continue;
            for(const auto& entry : std::filesystem::directory_iterator(dir))
                if(entry.path().extension()==".map")maps.push_back(entry.path());
        }
        std::sort(maps.begin(),maps.end());
        std::size_t rows=0,payloads=0,bsp_loads=0; std::uint64_t bytes=0;
        std::set<TagGroup> groups;
        for(const auto& path : maps) {
            try {
                auto cache=std::make_shared<Cache>(path);
                TagSystem tags(cache);
                check(tags.groups().size()==(cache->is_vista()?120:118),"Original group table count differs");
                for(const auto& group:tags.groups()) {
                    groups.insert(group.group);
                    check(!tag_group_name(group.group).empty(),"Original group missing from developer catalog");
                }
                for(const auto& group:tags.groups()) {
                    auto actual=tags.list(group.group);
                    std::vector<TagHandle> expected;
                    for(const auto& row:cache->tags())
                        if(!row.handle().is_null() && group.group!=tag_groups::none &&
                            cache->tag_group(row.type())->matches(group.group)) expected.push_back(row.handle());
                    check(actual==expected,"Original group iterator coverage differs");
                }
                std::size_t valid=0;
                for(const auto& row:cache->tags()) {
                    if(row.handle().is_null())continue;
                    ++valid;++rows;
                    check(cache->find_tag(row.handle())==&row,"Original salted slot lookup differs");
                    check(!cache->find_tag({row.datum^0x10000u}),"Different salt accepted");
                    for(auto group:{row.type(),cache->tag_group(row.type())->parent,
                                               cache->tag_group(row.type())->grandparent})
                        if(group!=tag_groups::none)check(cache->find_tag(row.handle(),group)==&row,"Original inherited lookup differs");
                    if(!row.address || row.group=="sbsp" || row.group=="ltmp")continue;
                    const auto view=tags.get(row.handle());
                    check(view.info().address==row.address && view.bytes().size()==row.size,"Original payload identity differs");
                    if(row.size) {
                        auto source=cache->load_source_tag_metadata(row.datum);
                        check(std::ranges::equal(view.bytes(),source.bytes_at(row.address,row.size)),"Original full tag payload differs");
                        ++payloads;bytes+=row.size;
                    }
                }
                check(tags.list().size()==valid,"Original unfiltered iterator differs");
                for(std::size_t i=0;i<cache->bsps().size();++i) {
                    tags.load_bsp(i);check(tags.selected_bsp()==i,"Original BSP selection differs");
                    const auto bsp=tags.get({cache->bsps()[i].datum},tag_groups::scenario_structure_bsp);
                    check(!bsp.bytes().empty(),"Selected BSP has no resident payload");
                    const auto mapping=tags.get(tags.scenario(),tag_groups::scenario).block(0x210,0x44).element(i);
                    const auto reference=mapping.reference(0x10),lightmap=mapping.reference(0x18);
                    check(reference.handle==bsp.info().handle() && tags.resolve(reference)->info().address==bsp.info().address,
                          "Authored BSP reference did not resolve through the shared module");
                    if(!lightmap.handle.is_null())
                        check(tags.resolve(lightmap)->info().group=="ltmp","Authored lightmap reference did not resolve");
                    bool rejected=false;
                    try {tags.load_bsp(cache->bsps().size());}catch(const FormatError&){rejected=true;}
                    check(rejected && tags.selected_bsp()==i,"Failed BSP load did not preserve the active selection");
                    if(!cache->is_vista()) {
                        const auto source=cache->load_source_bsp_metadata(i);
                        const Bytes header(source.bytes_at(source.source_address(),16));
                        check(bsp.info().address==header.u32(4),"Original BSP root publication differs");
                        if(!lightmap.handle.is_null())
                            check(tags.resolve(lightmap)->info().address==header.u32(8),"Original lightmap root publication differs");
                        check(std::ranges::equal(bsp.bytes(),source.bytes_at(bsp.info().address,bsp.info().size)),"Original complete BSP bytes differ");
                        auto retained=bsp;
                        tags.unload_bsp();
                        check(!tags.try_get({cache->bsps()[i].datum}),"Unloaded BSP still resolves");
                        if(!lightmap.handle.is_null())check(!tags.try_get(lightmap.handle),"Unloaded lightmap still resolves");
                        check(!retained.bytes().empty(),"Retained BSP snapshot lost ownership");
                    }
                    ++bsp_loads;
                }
                const auto file_header=tags.resource(0,16);
                check(Bytes(file_header).u32(0)==0x68656164u,"Local resource address decoding differs");
                for(const auto& resource:std::vector<std::pair<unsigned,const char*>>{{1,"mainmenu.map"},{2,"shared.map"},{3,"single_player_shared.map"}})
                    if(std::filesystem::exists(path.parent_path()/resource.second))
                        check(Bytes(tags.resource(resource.first<<30,16)).u32(0)==0x68656164u,"Shared resource address decoding differs");
                std::cout<<path.filename().string()<<": groups="<<tags.groups().size()<<" tags="<<valid<<" BSPs="<<cache->bsps().size()<<'\n';
            } catch(const std::exception& e) {throw std::runtime_error(path.string()+": "+e.what());}
        }
        check(groups.size()==120,"Both retail and Vista group catalogs were not covered");
        check(rows && payloads && bsp_loads,"Original asset coverage was empty");
        std::cout<<"maps="<<maps.size()<<" groupTypes="<<groups.size()<<" saltedRows="<<rows
                 <<" completePayloads="<<payloads<<" exactBytes="<<bytes<<" bspLoads="<<bsp_loads<<'\n';
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
