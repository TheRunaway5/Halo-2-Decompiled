#include "halo2/native_tu15_base_lock.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::byte> bytes;
    mutable Json reads=Json::array();Json writes=Json::array();
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        for(unsigned n=0;n<out.size();++n)out[n]=bytes.at(address+n);
        reads.push_back({address,out.size()});
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        for(unsigned n=0;n<in.size();++n)bytes.at(address+n)=in[n];
    }
    void put(std::uint32_t address,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)bytes[address+n]=std::byte(value>>(n*8u));
    }
};
}
int main(int argc,char **argv){try{
    check(argc==2,"Expected unchanged TU1.5 numeric reference");std::ifstream input(argv[1]);Json reference;input>>reference;
    check(reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Wrong original image");
    unsigned cases=0,missing=0,registered=0;std::size_t reads=0,stores=0;
    for(const auto &row:reference.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory;
        const auto object=row.at("object").get<std::uint32_t>(),registry=row.at("registry").get<std::uint32_t>();
        const auto keys=row.at("keys_address").get<std::uint32_t>();
        memory.put(0x48c93cu,registry);memory.put(registry+8u,keys);memory.put(registry+16u,row.at("mask"));
        for(unsigned n=0;n<row.at("keys").size();++n)memory.put(keys+n*4u,row.at("keys")[n]);
        memory.put(object+5u,row.at("before_byte"),1);const auto before=memory.bytes;
        bool refused=false;
        try{native_tu15_release_unregistered_base_lock(memory,object,[&](const auto &event){
            check(memory.bytes.at(event.address)==event.written.at(0),name+" observer preceded the real store");
            memory.writes.push_back({{"instruction",event.instruction},{"address",event.address},
                {"before",std::to_integer<unsigned>(event.before.at(0))},
                {"written",std::to_integer<unsigned>(event.written.at(0))}});
        });}catch(const std::runtime_error &e){
            check(std::string(e.what()).find("registered base lock requires")!=std::string::npos,name+" unexpected refusal: "+e.what());
            refused=true;
        }
        check(refused==row.at("registered").get<bool>(),name+" registry ownership differs");
        check(memory.reads==row.at("reads"),name+" ordered source reads differ");
        check(memory.writes==row.at("writes"),name+" committed source stores differ");
        auto expected=before;expected.at(object+5u)=std::byte(row.at("after_byte").get<unsigned>());
        check(memory.bytes==expected,name+" complete declared fields differ");
        reads+=memory.reads.size();stores+=memory.writes.size();++cases;refused?++registered:++missing;
    }
    check(cases>=16 && missing && registered,"Missing source branch coverage");
    std::cout<<Json{{"passed",true},{"cases",cases},{"missing",missing},{"registered_refusals",registered},
        {"ordered_reads",reads},{"ordered_stores",stores}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
