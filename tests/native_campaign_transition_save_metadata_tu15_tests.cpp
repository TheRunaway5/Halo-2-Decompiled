#include "halo2/native_campaign_transition_save_metadata_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> values){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:values){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4u];out+=digits[v&15u];}return out;}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned attempted_reads{};unsigned attempted_writes{};
    mutable Json reads=Json::array();Json writes=Json::array();
    auto locate(std::uint32_t address,std::size_t size)const{
        auto found=banks.upper_bound(address);if(found==banks.begin())throw Missing{address,size};--found;
        if(std::uint64_t(address-found->first)+size>found->second.size())throw Missing{address,size};return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override{
        if(phase=="before_read"&&attempted_reads++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
        reads.push_back(Json{{"address",address},{"bytes",out.size()},{"before",hex(out)},{"stores_before",writes.size()}});
    }
    void write(std::uint32_t address,std::span<const std::byte> data)override{
        if(phase=="before_write"&&attempted_writes==ordinal)throw Prefix{};
        const auto found=locate(address,data.size());const auto old=std::span(found->second).subspan(address-found->first,data.size());
        writes.push_back(Json{{"address",address},{"before",hex(old)},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),banks.at(found->first).begin()+address-found->first);
        if(phase=="after_write"&&attempted_writes++==ordinal)throw Prefix{};
        if(phase!="after_write")++attempted_writes;
    }
};
struct HostEnvironment {
    alignas(16)std::array<std::byte,108> original{},before{},after{};std::uint32_t mxcsr;
    HostEnvironment():mxcsr(_mm_getcsr()){
        asm volatile("fnsave %0;fninit;fld1;fldpi;fldln2":"=m"(original)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        asm volatile("fnsave %0;frstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    }
    bool unchanged(){
        asm volatile("fnsave %0;frstor %0":"=m"(after)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        for(auto offset:{0u,4u,8u})if(!std::equal(before.begin()+offset,before.begin()+offset+2u,after.begin()+offset))return false;
        return std::equal(before.begin()+28u,before.end(),after.begin()+28u)&&mxcsr==_mm_getcsr();
    }
    ~HostEnvironment(){asm volatile("frstor %0"::"m"(original):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr);}
};
}
int main(int argc,char **argv)try{
    check(argc==2,"Supply unchanged physical-original save-metadata fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Save-metadata original image changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Save-metadata reference lacks unchanged physical authority");
    std::map<std::string,std::vector<std::byte>> payloads;if(fixture.contains("bank_data"))for(const auto &[hash,value]:fixture.at("bank_data").items())payloads.emplace(hash,bytes(value));
    auto bank_bytes=[&](const Json &bank,const char *phase){return bank.contains(phase)?bytes(bank.at(phase)):payloads.at(bank.at(std::string(phase)+"_ref"));};
    auto events=[&](const Json &row,const char *field){
        if(row.contains(field))return row.at(field);
        const auto &ref=row.at(std::string(field)+"_prefix");const auto &seed=fixture.at("cases").at(ref.at("seed").get<std::size_t>());
        Json result=Json::array();for(std::size_t n=0;n<ref.at("count").get<std::size_t>();++n)result.push_back(seed.at(field).at(n));return result;
    };
    std::size_t count{},reads{},writes{},whole{},prefixes{},faults{},private_values{};
    for(const auto &row:fixture.at("cases")){
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>()&&row.at("live_x87_unchanged").get<bool>(),name+": missing save-metadata physical authority");
        Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bank_bytes(bank,"before"));
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        const bool value=row.value("private_options",false);NativeCampaignTransitionOptionsTu15 options;NativeCampaignTransitionSaveMetadataTu15 output;
        if(value){const auto raw=bytes(row.at("options_value")),before=bytes(row.at("output_before"));check(raw.size()==options.bytes.size()&&before.size()==output.bytes.size(),"Wrong private metadata/options sizes");std::copy(raw.begin(),raw.end(),options.bytes.begin());std::copy(before.begin(),before.end(),output.bytes.begin());++private_values;}
        bool stopped{};HostEnvironment host;
        try{
            const auto entry=row.at("entry").get<std::uint32_t>();
            if(entry==0x1acde0u){if(value)native_campaign_transition_save_metadata_value_tu15(memory,options,output);else native_campaign_transition_save_metadata_tu15(memory,row.at("options"),row.at("destination"));}
            else if(entry==0x1ad350u)native_campaign_transition_profile_metadata_tu15(memory,row.at("source"),row.at("destination"));
            else if(entry==0x1ad300u)native_campaign_transition_profile_subrecord15_tu15(memory,row.at("source"),row.at("destination"));
            else{check(entry==0x1acfc0u,"Unknown save-metadata owner");native_campaign_transition_profile_subrecord16_tu15(memory,row.at("source"),row.at("destination"));}
        }catch(const Prefix&){stopped=true;}catch(const Missing &error){
            check(row.value("actual_read_fault",false)||row.value("actual_write_fault",false),name+": unexpected save-metadata source address "+std::to_string(error.address));
            check(error.address==row.at("boundary").at("address")&&error.width==row.at("boundary").at("bytes"),name+": save-metadata fault operand differs");stopped=true;++faults;
        }
        check(host.unchanged(),name+": save-metadata kernel damaged ambient live FP");
        check(stopped==!row.at("completed").get<bool>(),name+": save-metadata completion differs");
        check(row.at("before_fp")==row.at("after_fp"),name+": original data-only metadata owner changed FP");
        auto expected_reads=events(row,"reads");const auto expected_writes=events(row,"writes");
        if(!row.at("boundary").is_null())for(const auto *field:{"uncommitted_copy_reads","uncommitted_reads"})if(row.at("boundary").contains(field))for(const auto &event:row.at("boundary").at(field))expected_reads.push_back(event);
        std::string first_read_difference;
        for(std::size_t n=0;n<std::min(memory.reads.size(),expected_reads.size());++n){const auto &a=memory.reads[n],&b=expected_reads[n];const auto &before=b.contains("before")?b.at("before"):b.at("value");if(a.at("address")!=b.at("address")||a.at("bytes")!=b.at("bytes")||a.at("before")!=before||a.at("stores_before")!=b.at("stores_before")){first_read_difference=" first at "+std::to_string(n)+" native="+a.dump()+" original="+b.dump();break;}}
        check(memory.reads.size()==expected_reads.size(),name+": save-metadata ordered read count differs native="+std::to_string(memory.reads.size())+" original="+std::to_string(expected_reads.size())+first_read_difference);
        for(std::size_t n=0;n<memory.reads.size();++n){const auto &a=memory.reads[n],&b=expected_reads[n];const auto &before=b.contains("before")?b.at("before"):b.at("value");check(a.at("address")==b.at("address")&&a.at("bytes")==b.at("bytes")&&a.at("before")==before&&a.at("stores_before")==b.at("stores_before"),name+": save-metadata ordered reads differ at "+std::to_string(n)+" native="+a.dump()+" original="+b.dump());}
        check(memory.writes.size()==expected_writes.size(),name+": save-metadata ordered store count differs");
        for(std::size_t n=0;n<memory.writes.size();++n){const auto &a=memory.writes[n],&b=expected_writes[n];check(a.at("address")==b.at("address")&&a.at("before")==b.at("before")&&a.at("written")==b.at("written"),name+": save-metadata ordered stores differ at "+std::to_string(n));}
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bank_bytes(bank,"after"),name+": retained save-metadata bytes differ");
        if(value){check(hex(options.bytes)==row.at("options_after").get<std::string>(),name+": private source options changed");check(hex(output.bytes)==row.at("output_after").get<std::string>(),name+": private metadata output differs");}
        ++count;reads+=memory.reads.size();writes+=memory.writes.size();whole+=!stopped;prefixes+=stopped;
    }
    check(count&&whole,"Save-metadata matrix is empty");std::cout<<count<<" physical original save-metadata cases, "<<reads<<" ordered reads, "<<writes<<" stores, "<<prefixes<<" prefixes, "<<faults<<" actual faults, "<<whole<<" whole and "<<private_values<<" private values passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
