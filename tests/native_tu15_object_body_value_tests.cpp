#include "halo2/object_creation_request.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

namespace halo2::object_body_predecessor {
std::uint32_t source_object_body_from_datum(std::uint32_t,std::uint32_t,
    std::uint32_t,const ObjectCreationRequestServices&);
void source_object_creation_request_initialize(std::uint32_t,std::uint32_t,
    std::uint32_t,std::uint32_t,std::uint32_t,const ObjectCreationRequestOperands&,
    const ObjectCreationRequestServices&,const ObjectCreationRequestObserver&);
}
namespace {
using J=nlohmann::json;
using namespace halo2;
void require(bool ok,const std::string&why){if(!ok)throw std::runtime_error(why);}
struct Stop{};
struct Memory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    J calls=J::array(),stores=J::array();
    unsigned mode{},failure{},ordinal{};
    bool extent_failure{};
    explicit Memory(const J&row){
        for(const auto&bank:row.at("banks")){
            const auto text=bank.at("before").get<std::string>();
            auto&out=banks[bank.at("address").get<std::uint32_t>()];
            for(unsigned n=0;n<text.size();n+=2)
                out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
        }
    }
    std::span<std::byte>get(std::uint32_t p,std::size_t size){
        calls.push_back(J::array({p,size}));
        if(++ordinal==failure)throw Stop{};
        auto it=banks.upper_bound(p);
        if(it==banks.begin())throw std::runtime_error("Original test backing unavailable");
        --it;
        if(std::uint64_t(p-it->first)+size>it->second.size())
            throw std::runtime_error("Original test extent unavailable");
        auto out=std::span(it->second).subspan(p-it->first,size);
        // Real provider feedback changes the bytes before returning its view.
        if(mode==1u&&!out.empty())out.front()^=std::byte{1};
        if(mode==2u&&size==4u)for(auto&byte:out)byte^=std::byte{0x80};
        if(extent_failure&&!out.empty())return out.first(out.size()-1u);
        return out;
    }
};
struct Result {
    std::uint32_t value{};bool stopped{};std::string error;
    friend bool operator==(const Result&,const Result&)=default;
};
Result run(Memory&memory,const J&row,bool original,bool initialize,bool missing){
    ObjectCreationRequestServices services;
    if(!missing)services.memory=[&](auto p,auto size){return memory.get(p,size);};
    Result result;
    try{
        if(initialize){
            ObjectCreationRequestOperands operands{};operands.one_bits=0x3f800000u;
            for(unsigned n=0;n<12;++n){operands.basis_forward[n]=std::byte(n*3u);operands.basis_up[n]=std::byte(255u-n*7u);}
            ObjectCreationRequestObserver observe=[&](const auto&event){memory.stores.push_back(J::array({event.instruction,event.address,event.bytes,event.before,event.after}));};
            const auto call=original?object_body_predecessor::source_object_creation_request_initialize:
                source_object_creation_request_initialize;
            call(0x61001800u,0xdeadbeefu,row.at("datum"),0u,row.at("pool"),operands,services,observe);
        }else{
            const auto call=original?object_body_predecessor::source_object_body_from_datum:
                source_object_body_from_datum;
            result.value=call(row.at("pool"),row.at("datum"),row.at("mask"),services);
        }
    }catch(const Stop&){result.stopped=true;}
    catch(const std::exception&error){result.error=error.what();}
    return result;
}
void equal(const Memory&a,const Memory&b,const Result&x,const Result&y){
    require(x==y,"Public object-body result/exception differs");
    require(a.calls==b.calls,"Public object-body provider sequence differs");
    require(a.stores==b.stores,"Public object-body observer events differ");
    require(a.banks==b.banks,"Public object-body provider banks differ");
}
}
int main(int argc,char**argv)try{
    require(argc==2,"Supply public object-body input corpus");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    unsigned comparisons{},failures{},provider_calls{},initializations{},zero_record{};
    for(const auto&row:rows)for(unsigned mode=0;mode<3;++mode)
        for(bool initialize:{false,true}){
            if(initialize&&row.at("id").get<unsigned>()%16u)continue;
            Memory a(row),b(row);a.mode=b.mode=mode;
            const auto x=run(a,row,true,initialize,false),y=run(b,row,false,initialize,false);
            equal(a,b,x,y);++comparisons;provider_calls+=a.calls.size();initializations+=initialize;
            if(row.value("zero_record",false)&&!initialize&&mode==0u){
                require(x.value==0xcafebabeu,"Public zero-record predecessor behavior changed");++zero_record;
            }
            for(unsigned n=1;n<=a.calls.size();++n){
                Memory c(row),d(row);c.mode=d.mode=mode;c.failure=d.failure=n;
                const auto old=run(c,row,true,initialize,false),now=run(d,row,false,initialize,false);
                equal(c,d,old,now);require(old.stopped,"Declared public provider prefix did not throw");
                ++comparisons;++failures;provider_calls+=c.calls.size();
            }
            for(bool missing:{false,true}){
                Memory c(row),d(row);c.mode=d.mode=mode;c.extent_failure=d.extent_failure=!missing;
                equal(c,d,run(c,row,true,initialize,missing),run(d,row,false,initialize,missing));++comparisons;
            }
        }
    require(zero_record==1u,"Public zero-record control absent");
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"provider_failure_prefixes",failures},
        {"complete_provider_calls",provider_calls},{"normal_initializations",initializations},
        {"public_zero_record_controls",zero_record},{"source_default_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
