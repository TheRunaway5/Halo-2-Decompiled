#include "halo2/source_prt.hpp"
#include "halo2/model.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
Vec3 vector(const nlohmann::json &value){return {std::bit_cast<float>(value[0].get<std::uint32_t>()),
    std::bit_cast<float>(value[1].get<std::uint32_t>()),std::bit_cast<float>(value[2].get<std::uint32_t>())};}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply MAP original TU1.5 PRT fixture");Cache cache(argv[1]);
    std::ifstream file(argv[2]);nlohmann::json fixture;file>>fixture;
    check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","PRT fixture is not TU1.5");
    unsigned cases=0,words=0;
    for(const auto &row:fixture.at("cases")) {
        const auto datum=row.at("render_model").get<std::uint32_t>();const auto model=cache.load_model(datum,false);
        const auto definition=*cache.load_source_prt_definition(datum);
        auto lighting=cache.load_source_prt_lighting();
        const auto control=row.at("control").get<std::string>();
        if(control=="empty")lighting.records.clear();
        else if(control=="invalid")lighting.structure_matches=false;
        else check(control=="valid","unknown original PRT control");
        const auto raw=row.at("world_nodes").get<std::vector<std::uint32_t>>();
        check(raw.size()%13==0,"incomplete original WORLD bank");std::vector<Transform> nodes(raw.size()/13);
        std::memcpy(nodes.data(),raw.data(),raw.size()*4);
        const auto sections=row.at("sections").get<std::vector<unsigned>>();
        const auto banks=source_prt_section_banks(cache,model,definition,row.at("lod"),sections,nodes,lighting,vector(row.at("retained_incident")));
        check(banks.size()==row.at("banks").size(),"original PRT section count differs");
        for(unsigned n=0;n<banks.size();++n) {
            const auto &expected=row.at("banks").at(n);const auto &bank=*banks[n].bank;
            check(banks[n].section==expected.at("section"),"original PRT section order differs");
            const auto values=expected.at("words").get<std::vector<std::uint32_t>>();
            check(bank.first_constant==68 && bank.constants.size()*4==values.size()+8,"original PRT bank extent differs");
            const auto incident=row.at("incident").get<std::array<std::uint32_t,3>>();
            for(unsigned i=0;i<3;++i){check(std::bit_cast<std::uint32_t>(bank.constants[0][i])==incident[i],"original retained PRT incident differs");++words;}
            for(unsigned i=0;i<values.size();++i) {
                if(i%16==3)continue; // Original ambient padding remains unwritten.
                if(std::bit_cast<std::uint32_t>(bank.constants[2+i/4][i%4])!=values[i]) {
                    std::cerr<<control<<" section="<<banks[n].section<<" component="<<i<<'\n';
                    throw FormatError("original TU1.5 PRT coefficient differs");
                }
                ++words;
            }
        }
        if(control!="valid") {
            bool rejected=false;try{source_prt_section_banks(cache,model,definition,row.at("lod"),sections,nodes,lighting);}
            catch(const FormatError&){rejected=true;}
            check(rejected,"PRT fallback fabricated an absent retained incident");
        }
        ++cases;
    }
    check(cases==3,"missing original valid/empty/invalid PRT case");
    std::cout<<"PASS "<<cases<<" original TU1.5 PRT banks / "<<words<<" bit-exact consumed words; absent retained incident rejects\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
