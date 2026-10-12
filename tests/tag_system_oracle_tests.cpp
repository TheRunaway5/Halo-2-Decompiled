#include "halo2/cache.hpp"
#include "halo2/tags/system.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
void check(bool ok,const char* why) {if(!ok)throw std::runtime_error(why);}
std::uint32_t number(const std::string& word) {return std::stoul(word,nullptr,16);}
}
int main(int argc,char** argv) {
    try {
        check(argc==3,"Supply oracle fixture and repository root");
        if(!std::filesystem::exists(argv[1]))return 77;
        std::ifstream input(argv[1]);
        std::shared_ptr<halo2::Cache> cache;
        std::size_t maps=0,groups=0,lookups=0,iterations=0;
        for(std::string line;std::getline(input,line);) {
            std::istringstream row(line);char kind;row>>kind;
            if(kind=='M') {
                std::string path;check(bool(row>>std::quoted(path)),"Missing map path");
                const auto actual=std::filesystem::path(argv[2])/path;
                if(!std::filesystem::exists(actual))return 77;
                cache=std::make_shared<halo2::Cache>(actual);++maps;
            } else if(kind=='G') {
                std::string a,b,c;check(bool(row>>a>>b>>c),"Missing group result");
                const auto* group=cache->tag_group(halo2::TagGroup(number(a)));
                check(group && group->parent.value==number(b) && group->grandparent.value==number(c),"Retail group lookup differs");++groups;
            } else if(kind=='L') {
                std::string datum,expected,address;check(bool(row>>datum>>expected>>address),"Missing lookup result");
                const auto* tag=cache->find_tag({number(datum)},halo2::TagGroup(number(expected)));
                check((tag?tag->address:0u)==number(address),"Retail salted/group tag lookup differs");++lookups;
            } else if(kind=='I') {
                std::string group,current;std::uint32_t next;
                check(bool(row>>group>>current>>next),"Missing iterator state");
                halo2::TagIterator iterator{0,{0x13572468u},{}};
                iterator.group=halo2::TagGroup(number(group));
                for(std::string datum;row>>datum;) {
                    check(cache->next_tag(iterator).value==number(datum),"Retail iterator sequence differs");++iterations;
                }
                check(cache->next_tag(iterator).is_null() && iterator.current.value==number(current) && iterator.next_index==next,
                      "Retail iterator exhausted state differs");
            } else throw std::runtime_error("Unknown oracle row");
        }
        check(maps==3 && groups==354 && lookups && iterations,"Incomplete retail oracle coverage");
        std::cout<<"retailMaps="<<maps<<" groupLookups="<<groups<<" saltedGroupLookups="<<lookups<<" iteratorResults="<<iterations<<'\n';
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
