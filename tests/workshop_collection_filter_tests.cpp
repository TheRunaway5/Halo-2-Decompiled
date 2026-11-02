#include "halo2/native_tu15_collection_filter.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;using Json=nlohmann::json;
namespace {
constexpr std::uint32_t base=0x2100000,negative=0x90000000;
void check(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> hex(const std::string& text){
    check(text.size()%2==0,"Odd reference hex size");std::vector<std::byte> out;
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;
}
std::string hex(std::span<const std::byte> bytes){
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto byte:bytes){const auto v=std::to_integer<unsigned>(byte);out+=digits[v>>4];out+=digits[v&15];}return out;
}
class Memory final:public SourcePhysicsContactMemory {
public:
    std::vector<std::byte> bank=std::vector<std::byte>(0x4000,std::byte{0xa5}),minus=std::vector<std::byte>(4096,std::byte{0xa5});
    std::array<std::byte,4> global{};mutable Json reads=Json::array();Json writes=Json::array();
    std::span<std::byte> at(std::uint32_t p,std::size_t size){
        if(p>=base&&std::uint64_t(p)+size<=std::uint64_t(base)+bank.size())return std::span(bank).subspan(p-base,size);
        if(p>=negative&&std::uint64_t(p)+size<=std::uint64_t(negative)+minus.size())return std::span(minus).subspan(p-negative,size);
        if(p>=0x48b860&&std::uint64_t(p)+size<=0x48b864)return std::span(global).subspan(p-0x48b860,size);
        throw std::runtime_error("Native filter read/write outside declared ordinary fields "+std::to_string(p));
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        const auto data=const_cast<Memory*>(this)->at(p,out.size());std::copy(data.begin(),data.end(),out.begin());reads.push_back({p,out.size()});
    }
    void write(std::uint32_t p,std::span<const std::byte> input)override {
        auto data=at(p,input.size());writes.push_back({{"address",p},{"before",hex(data)},{"after",hex(input)}});std::copy(input.begin(),input.end(),data.begin());
    }
    void put(std::uint32_t p,unsigned width,std::uint32_t value){auto data=at(p,width);for(unsigned n=0;n<width;++n)data[n]=std::byte(value>>(n*8u));}
};
std::uint32_t type(NativeTu15ContactChild,NativeTu15ContactVirtualOwner owner,NativeTu15PhysicsFrameFp&){
    if(owner.identity==0xa9520)return native_tu15_contact_child_class_23();
    if(owner.identity==0xab4b0)return native_tu15_contact_child_class_24();
    if(const auto value=native_tu15_activation_type(owner.identity))return *value;
    throw std::runtime_error("Native filter lacks verified constant owner="+std::to_string(owner.identity));
}
Json reference(const char* path){
    std::ifstream input(path,std::ios::binary);const std::vector<unsigned char> compressed{std::istreambuf_iterator<char>(input),{}};
    check(!compressed.empty(),"Missing original filter reference");std::vector<unsigned char> raw(2*1024*1024);uLongf size=raw.size();
    check(uncompress(raw.data(),&size,compressed.data(),compressed.size())==Z_OK,"Cannot decode bounded original filter reference");
    return Json::parse(raw.begin(),raw.begin()+size);
}
}
int main(int argc,char** argv)try{
    check(argc==2,"Supply original host-verified collection filter reference");const auto data=reference(argv[1]);
    check(data.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&data.at("host_cpu_cases")==381&&data.at("main_body_sites")==131&&data.at("main_body_all_sites_executed")==true,"Original filter scope/provenance differs");
    unsigned cases{},filters{},counts{},children{};
    for(const auto& row:data.at("cases")) {
        const auto label=row.at("label").get<std::string>();Memory memory;
        for(const auto& cell:row.at("cells"))memory.put(cell[0],cell[1],cell[2]);
        auto expected_bank=memory.bank,expected_negative=memory.minus;auto expected_global=memory.global;Json expected_writes=Json::array();
        for(const auto& store:row.at("stores")) {
            const auto address=store.at("address").get<std::uint32_t>();const auto bytes=hex(store.at("after").get<std::string>());
            auto data=memory.at(address,bytes.size());check(hex(data)==store.at("before").get<std::string>(),label+" reference store preimage differs");
            expected_writes.push_back({{"address",address},{"before",store.at("before")},{"after",store.at("after")}});
            if(address>=base)std::copy(bytes.begin(),bytes.end(),(address>=negative?expected_negative:expected_bank).begin()+(address>=negative?address-negative:address-base));
            else std::copy(bytes.begin(),bytes.end(),expected_global.begin()+address-0x48b860);
        }
        NativeTu15CollectionFilterServices services;services.classify_shape=type;
        services.resolve_child=[&](auto collection,auto owner,auto index,auto,auto&){check(owner.identity==0x306220,label+" resolver owner differs");return native_tu15_collection_child(memory,collection,index);};
        services.motion_type=[&](auto motion,auto owner,auto& fp){return type({motion},owner,fp);};
        NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};const auto entry=row.at("entry").get<std::uint32_t>(),receiver=row.at("receiver").get<std::uint32_t>();const auto& args=row.at("args");std::uint32_t returned{};
        if(entry==0x1cbe70){returned=native_tu15_filter_collection_child(memory,receiver,args[0],{args[2]},{args[3]},{args[4]},args[5],{0x2103d00},fp,services);++filters;}
        else if(entry==0x3061f0){returned=native_tu15_collection_child_count(memory,{receiver});++counts;}
        else{check(entry==0x306220,label+" unexpected original entry");returned=native_tu15_collection_child(memory,{receiver},args[0]).identity;++children;}
        check(returned==row.at("returned"),label+" semantic return differs");
        if(memory.reads!=row.at("reads")) {
            std::cerr<<label<<" original reads="<<row.at("reads").dump()<<" native reads="<<memory.reads.dump()<<'\n';
            throw std::runtime_error(label+" ordered original field reads differ");
        }
        check(memory.writes==expected_writes,label+" ordered original stores differ");
        check(memory.bank==expected_bank&&memory.minus==expected_negative&&memory.global==expected_global,label+" complete ordinary memory differs");
        check(Json::array({fp.control_word,fp.status,fp.mxcsr})==row.at("fp"),label+" FP environment differs");++cases;
    }
    check(cases==381&&counts==64&&children==6,"Original filter/leaf cohort incomplete");
    std::cout<<Json{{"passed",true},{"cases",cases},{"filters",filters},{"count_owners",counts},{"child_owners",children},{"all_131_original_filter_sites",true}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
