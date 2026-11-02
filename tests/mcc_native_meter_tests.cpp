#include "halo2/cache.hpp"
#include "halo2/mcc_native_meter.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char* message){if(!value)throw FormatError(message);}
std::vector<std::byte> unhex(const Json& value){const auto s=value.get<std::string>();std::vector<std::byte> out;
    need(s.size()%2==0,"Bad meter fixture bytes");for(unsigned i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;}
Tag tag(const Json& value){return {value.at("group"),value.at("name"),value.at("datum"),value.at("address"),value.at("size")};}
void word(std::vector<std::byte>& raw,unsigned at,std::uint32_t value){Bytes(raw).require(at,4);for(unsigned i=0;i<4;++i)raw[at+i]=std::byte(value>>(i*8));}
struct Fixture {
    Tag stem,pass,vertex;std::map<std::uint32_t,Tag> names;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    explicit Fixture(const Json& value):stem(tag(value.at("stem"))),pass(tag(value.at("pass"))),vertex(tag(value.at("vertex"))){
        for(const auto& t:value.at("namespace")){auto item=tag(t);names.emplace(item.datum,item);}
        for(const auto& r:value.at("regions"))regions.emplace_back(r.at(0),unhex(r.at(1)));
    }
    std::span<const std::byte> read(std::uint32_t at,std::size_t size)const {
        for(const auto& [first,raw]:regions)if(at>=first&&std::uint64_t(at)-first<=raw.size()&&size<=raw.size()-(at-first))
            return std::span(raw).subspan(at-first,size);
        throw FormatError("Meter fixture read escaped its literal owner");
    }
    auto run()const{return project_mcc_resident_meter_owners(stem,pass,vertex,
        [&](auto at,auto size){return read(at,size);},[&](auto handle)->const Tag*{
            const auto i=names.find(handle);return i==names.end()?nullptr:&i->second;});}
    void flip(std::uint32_t at){bool found=false;for(auto& [first,raw]:regions)
        if(at>=first&&std::uint64_t(at)-first<raw.size()){raw[at-first]^=std::byte{1};found=true;}
        need(found,"Meter mutation escaped its literal region");}
};
}
int main(int argc,char** argv)try{
    need(argc==3,"Expected literal meter fixture and policy");Json input,policy;std::ifstream(argv[1])>>input;std::ifstream(argv[2])>>policy;
    need(input.at("schema")=="H2_MCC_RESIDENT_METER_FIXTURE_V1","Wrong meter fixture schema");
    unsigned complete_bytes{},pointer_checks{},negative{};Json kernels=Json::array();
    const auto one=[&](const Json& input,const Json& setup){
    Fixture f(input);const auto before=f.regions;const auto out=f.run();need(bool(out)&&before==f.regions,"Actual weapon-pass setup rejected or source mutated");
    kernels.push_back(setup.at("existing_fragment_kernel"));
    const auto check=[&](const MccOwnedShaderProjection& projection,const Json& expected,const Tag& dependency){
        auto bytes=projection.bytes;const auto& record=expected.at("record");const auto& reference=record.at("references").front();
        need(Bytes(bytes).u32(reference.at(0).get<unsigned>())==dependency.datum,"Meter lost full-salt typed dependency");
        const auto original=unhex(expected.at("metadata_hex"));word(bytes,reference.at(0),Bytes(original).u32(reference.at(0)));
        need(bytes==original,"Meter changed declarative metadata beyond its one typed binding");complete_bytes+=bytes.size();
        need(projection.pointers.size()==record.at("pointers").size(),"Meter lost its exact pointer map");
        for(unsigned i=0;i<projection.pointers.size();++i){const auto [field,target]=projection.pointers[i];
            need(field==record.at("pointers").at(i).at(0)&&target==record.at("pointers").at(i).at(1)&&
                Bytes(projection.bytes).u32(field)==target,"Meter pointer changed");++pointer_checks;}
    };
    check(out->pass,setup.at("native_pass"),f.vertex);check(out->stem,setup.at("native_stem"),f.pass);
    const auto reject=[&](const Fixture& bad){need(!bad.run(),"Changed or unowned meter setup admitted");++negative;};
    std::set<std::uint32_t> addresses;for(const auto& [first,raw]:f.regions)for(unsigned i=0;i<raw.size();++i)addresses.insert(first+i);
    for(const auto address:addresses){auto bad=f;bad.flip(address);reject(bad);}
    for(const auto& [handle,unused]:f.names){(void)unused;auto bad=f;bad.names[handle].datum^=0x10000u;reject(bad);
        bad=f;bad.names[handle].group="mode";reject(bad);bad=f;++bad.names[handle].address;reject(bad);bad=f;++bad.names[handle].size;reject(bad);}
    {auto bad=f;bad.stem.datum^=0x10000u;reject(bad);}
    {auto bad=f;bad.pass.datum^=0x10000u;reject(bad);}
    {auto bad=f;bad.vertex.datum^=0x10000u;reject(bad);}
    };
    need(input.at("additional_cases").size()==1&&policy.at("additional_setups").size()==1,
        "Weapon-pass contract census changed");
    one(input,policy.at("setup"));one(input.at("additional_cases").front(),policy.at("additional_setups").front());
    const auto contracts=mcc_resident_weapon_pass_contracts();need(contracts.size()==2,"Lost finite weapon-pass descriptor census");
    for(unsigned i=0;i<contracts.size();++i){const auto& setup=i?policy.at("additional_setups").front():policy.at("setup");
        need(contracts[i].stem==setup.at("source_stem").at("name").get<std::string>()&&
            contracts[i].pass==setup.at("source_pass").at("name").get<std::string>()&&
            contracts[i].vertex==setup.at("resident_vertex_records").front().at("name").get<std::string>(),
            "Weapon-pass descriptors changed typed contract");}
    {Fixture a(input),b(input.at("additional_cases").front());a.pass=b.pass;a.names.insert(b.names.begin(),b.names.end());
        need(!a.run(),"Cross-family pass owner admitted");++negative;}
    std::cout<<Json{{"passed",true},{"complete_declarative_bytes",complete_bytes},{"pointer_controls",pointer_checks},
        {"strict_rejections",negative},{"existing_fragment_kernels",kernels},{"contracts",contracts.size()},
        {"executable_program_banks_added",0}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
