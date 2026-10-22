#include "halo2/particle_movement.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include <iostream>
#include <set>

using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=2)throw FormatError("Supply prepared MCC map");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const std::filesystem::path path=argv[1];
    auto cache=std::make_shared<Cache>(path,path.parent_path(),OriginalData::native_tu15());
    if(!cache->is_mcc())throw FormatError("Particle movement validation requires MCC data");
    // Curves may be shared across other genuine resident tag allocations.
    // The independent retained provider must include those authored owners.
    std::set<std::string> groups;
    for(const auto& tag:cache->tags())if(tag.address && tag.group!="sbsp" && tag.group!="ltmp")
        groups.insert(tag.group);
    SourceTagMetadataStore store(cache,{groups.begin(),groups.end()});
    const auto provider=store.providers();
    unsigned definitions{},controllers{},failures{};
    for(const auto& tag:cache->tags())if(tag.group=="pmov" && tag.address) {
        try {
            const auto actual=load_particle_movement_definition(*cache,tag.datum);
            const auto retained=load_particle_movement_definition(tag.datum,provider);
            if(actual.datum!=tag.datum || actual.flags!=retained.flags ||
                actual.controllers.size()!=retained.controllers.size())
                throw FormatError("Particle movement changed its retained definition identity");
            for(unsigned i=0;i<actual.controllers.size();++i) {
                const auto& a=actual.controllers[i];const auto& b=retained.controllers[i];
                if(a.type!=b.type || a.constant_parameters!=b.constant_parameters ||
                    a.required_values!=b.required_values || a.parameters.size()!=b.parameters.size())
                    throw FormatError("Particle movement changed its retained controller");
                for(unsigned n=0;n<a.parameters.size();++n) {
                    const auto& p=a.parameters[n];const auto& q=b.parameters[n];
                    if(p.input!=q.input || p.range!=q.range || p.modifier_input!=q.modifier_input ||
                        p.modifier!=q.modifier || p.curve!=q.curve)
                        throw FormatError("Particle movement changed its retained curve");
                }
            }
            ++definitions;controllers+=actual.controllers.size();
        }catch(const std::exception& e) {
            ++failures;std::cerr<<std::hex<<tag.datum<<std::dec<<" "<<tag.name<<": "<<e.what()<<'\n';
        }
    }
    std::cout<<"MCC particle movement definitions="<<definitions<<" controllers="<<controllers
        <<" failures="<<failures<<'\n';
    return definitions && !failures?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
