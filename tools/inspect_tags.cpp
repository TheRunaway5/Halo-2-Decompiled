#include "halo2/tags/system.hpp"
#include "halo2/cache.hpp"
#include <iomanip>
#include <iostream>

int main(int argc,char** argv) {
    try {
        if(argc<2 || argc>3) throw halo2::FormatError("Usage: inspect_tags MAP [FOURCC]");
        auto cache=std::make_shared<halo2::Cache>(argv[1]);
        halo2::TagSystem tags(cache);
        std::optional<halo2::TagGroup> filter;
        if(argc==3) filter=halo2::TagGroup::from_string(argv[2]);
        std::cout<<cache->name()<<" groups="<<tags.groups().size()<<" tags="<<cache->tags().size()<<'\n';
        for(auto handle:tags.list(filter)) {
            const auto& tag=*cache->find_tag(handle);
            std::cout<<std::hex<<std::setfill('0')<<std::setw(8)<<handle.value<<' '
                     <<tag.group<<' '<<std::setw(8)<<tag.address<<' '<<std::dec<<tag.size<<' '
                     <<halo2::tag_group_name(tag.type())<<' '<<tag.name<<'\n';
        }
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n'; return 1;}
}
