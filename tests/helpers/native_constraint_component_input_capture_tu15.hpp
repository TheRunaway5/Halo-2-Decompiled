#pragma once
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <map>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>
namespace halo2::test {
// Read-only frame-level census. This does not claim a captured22D460 call ABI.
class NativeConstraintComponentInputCaptureTu15 {
    using Json=nlohmann::json;
    struct Bank {std::uint32_t address;std::size_t size;};
    struct View {
        std::vector<Bank> banks;bool truncated{};mutable bool page_bound{};
        mutable std::map<std::uint32_t,std::vector<std::byte>> pages;
        mutable std::uint64_t page_bytes{};
        std::function<std::optional<std::uint32_t>(std::uint32_t,unsigned)> load;
        template<class Memory>explicit View(const Memory&m){
            m.visit_committed([&](std::uint32_t a,std::span<const std::byte>b){if(banks.size()<8192)banks.push_back({a,b.size()});else truncated=true;});
            load=[&m,this](std::uint32_t address,unsigned width)->std::optional<std::uint32_t>{
                std::optional<std::uint32_t>result;
                m.visit_committed([&](std::uint32_t first,std::span<const std::byte>bank){
                    if(result||address<first||std::uint64_t(address)+width>std::uint64_t(first)+bank.size())return;
                    const auto start=std::max(first,address&~4095u);
                    const auto end=std::min(std::uint64_t(first)+bank.size(),std::uint64_t(address&~4095u)+4096u);
                    if(std::uint64_t(address)+width>end)return;
                    if(page_bytes+end-start>4194304u){page_bound=true;return;}
                    // The borrowed span is consumed only inside this callback.
                    auto&copy=pages[start];copy.assign(bank.begin()+(start-first),bank.begin()+(end-first));page_bytes+=copy.size();
                    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(copy[address-start+n])<<(n*8u);result=value;
                });return result;
            };
        }
        View(const View&)=delete;View&operator=(const View&)=delete;
        std::optional<std::uint32_t>word(std::uint32_t address,unsigned width=4)const{
            auto it=pages.upper_bound(address);
            if(it!=pages.begin()){--it;if(std::uint64_t(address)+width<=std::uint64_t(it->first)+it->second.size()){
                std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(it->second[address-it->first+n])<<(n*8u);return value;
            }}return load(address,width);
        }
    };
    std::filesystem::path directory_;std::ofstream census_;
    std::uint64_t frames_{},records_{},bytes_{},dropped_{},positive_frames_{};
    bool captured_{};
    static constexpr std::uint64_t record_limit=4096,byte_limit=4194304,bank_limit=201326592;
    static void require(bool value,const char*reason){if(!value)throw std::runtime_error(reason);}
    void log(Json row){const auto line=row.dump()+'\n';if(records_==record_limit||bytes_+line.size()>byte_limit){++dropped_;return;}census_<<line;census_.flush();require(bool(census_),"Cannot retain native component eligibility census");++records_;bytes_+=line.size();}
    template<class Memory>void capture(const Memory&memory,const View&view,std::uint64_t tick,const NativeTu15PhysicsFrameFp&fp,unsigned depth,const Json&candidate){
        if(depth){log({{"tick",tick},{"capture_refused","nonempty_live_x87_requires_additional_typed_values"},{"actual_x87_depth",depth}});return;}
        std::uint64_t bank_bytes{};for(const auto&bank:view.banks)bank_bytes+=bank.size;
        if(bank_bytes>bank_limit){log({{"tick",tick},{"capture_refused","committed_bank_bound"},{"actual_bank_bytes",bank_bytes}});return;}
        const auto path=directory_/"positive-frame-before";require(!std::filesystem::exists(path),"Native constraint BEFORE directory already exists");std::filesystem::create_directory(path);
        Json manifest{{"schema","native-tu15-constraint-component-frame-before-v1"},{"reference","English TU1.5 only"},{"boundary","Actual native world frame entry; component eligibility DATA only, not a captured22D460 ABI"},{"tick",tick},{"component",candidate},{"floating_point",{{"x87_control_word",fp.control_word},{"x87_status",fp.status},{"mxcsr",fp.mxcsr}}},{"preserved_x87_depth",depth},{"regions",Json::array()},{"bank_bytes",bank_bytes},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false},{"captured_after_consumed",false}};
        std::uint64_t written_bytes{};unsigned written_banks{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            const auto file="before-"+std::to_string(address)+".bin";std::ofstream out(path/file,std::ios::binary);
            out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot retain actual positive component BEFORE bank");
            manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});written_bytes+=bank.size();++written_banks;
        });
        require(written_bytes==bank_bytes&&written_banks==view.banks.size(),"Component capture topology changed during synchronous observation");
        std::ofstream out(path/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot retain actual component BEFORE manifest");captured_=true;
    }
public:
    explicit NativeConstraintComponentInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Component input capture requires owned test scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);auto a=owner.begin(),b=directory_.begin();
        for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Component input capture escapes owned scratch");require(b!=directory_.end(),"Component input capture requires a child directory");
        require(!std::filesystem::exists(directory_),"Component input capture destination already exists");std::filesystem::create_directories(directory_);census_.open(directory_/"constraint-component-eligibility.jsonl");require(bool(census_),"Cannot open component eligibility census");
    }
    bool complete()const noexcept{return captured_;}
    template<class Memory>void observe(const Memory&memory,std::uint64_t tick,const NativeTu15PhysicsFrameFp&fp,unsigned depth){
        ++frames_;if(captured_)return;if(records_==record_limit||bytes_>=byte_limit){++dropped_;return;}
        const View view(memory);if(view.truncated){log({{"tick",tick},{"status","committed_bank_topology_bound"}});return;}const auto pool=view.word(0x52be5cu);Json row{{"tick",tick},{"actual_x87_depth",depth},{"pool",pool?Json(*pool):Json(nullptr)}};
        if(!pool||!*pool){row["status"]="pool_unmapped_or_null";log(row);return;}
        const auto raw_extent=view.word(*pool+0x38u),stride=view.word(*pool+0x24u),rows=view.word(*pool+0x44u),bitmap=view.word(*pool+0x48u);
        if(!raw_extent||!stride||!rows||!bitmap){row["status"]="pool_header_unmapped";log(row);return;}
        const auto extent=std::bit_cast<std::int32_t>(*raw_extent);row["extent"]=extent;row["stride"]=*stride;row["rows"]=*rows;row["bitmap"]=*bitmap;
        if(extent<0||extent>4096||*stride!=0xa0u||!*rows||!*bitmap){row["status"]="pool_profile_unbounded_or_invalid";log(row);return;}
        unsigned occupied{},unmapped{},zero_salt{},positive{},flagged{},eligible{},body_bound{};Json samples=Json::array();std::optional<Json>first;
        for(unsigned index=0;index<unsigned(extent);++index){
            const auto mask=view.word(*bitmap+(index>>5u)*4u);if(!mask){++unmapped;continue;}if(!(*mask&(1u<<(index&31u))))continue;++occupied;
            const auto address=*rows+index*0xa0u;const auto salt=view.word(address,2),flags=view.word(address+4u),count=view.word(address+0x80u),bank=view.word(address+0x7cu);
            if(!salt||!flags||!count||!bank){++unmapped;continue;}if(!*salt){++zero_salt;continue;}
            const auto authored=std::bit_cast<std::int32_t>(*count);positive+=authored>0;flagged+=bool(*flags&0x800u);
            Json sample{{"datum",(*salt<<16u)|index},{"index",index},{"row",address},{"salt",*salt},{"flags",*flags},{"authored_constraint_count",authored},{"constraint_bank",*bank}};
            if(authored>0&&(*flags&0x800u)){
                const auto body_count=view.word(address+0x74u),entries=view.word(address+0x70u);bool active{},bad{};std::uint32_t active_body{},active_island{};
                if(!body_count||!entries){++unmapped;bad=true;}
                else{const auto n=std::bit_cast<std::int32_t>(*body_count);sample["body_count"]=n;
                    if(n>0)for(unsigned ordinal=0;ordinal<unsigned(n)&&ordinal<64;++ordinal){
                        const auto body=view.word(*entries+ordinal*0x60u+0x40u);if(!body){bad=true;break;}const auto island=view.word(*body+0x44u);if(!island){bad=true;break;}
                        if(*island){const auto flag=view.word(*island+0x29u,1);if(!flag){bad=true;break;}if(*flag){active=true;active_body=*body;active_island=*island;break;}}
                    }
                    if(n>64&&!active){++body_bound;bad=true;}
                }
                if(bad)sample["active_status"]="unmapped_or_body_bound";
                else{sample["active_body"]=active_body;sample["active_island"]=active_island;sample["active"]=active;
                    if(active){const auto first_record=view.word(*bank),first_handle=view.word(*bank+4u);if(!*bank||!first_record||!first_handle){++unmapped;sample["status"]="constraint_bank_unmapped";}else{sample["first_record_raw"]=*first_record;sample["first_handle"]=*first_handle;++eligible;if(!first)first=sample;}}
                }
            }
            if(samples.size()<64)samples.push_back(sample);
        }
        row["status"]=view.page_bound?"read_page_cache_bound":"complete_bounded_census";row["owned_read_page_bytes"]=view.page_bytes;row["occupied"]=occupied;row["unmapped"]=unmapped;row["occupied_zero_salt_inconsistent"]=zero_salt;row["positive_authored_count"]=positive;row["flag800"]=flagged;row["eligible_frame_components"]=eligible;row["unbounded_body_queries"]=body_bound;row["samples"]=samples;
        if(first){++positive_frames_;capture(memory,view,tick,fp,depth,*first);}log(row);
    }
    void finish(){Json status{{"frames",frames_},{"records",records_},{"bytes",bytes_},{"dropped",dropped_},{"record_limit",record_limit},{"byte_limit",byte_limit},{"before_bank_limit",bank_limit},{"positive_frames",positive_frames_},{"captured",captured_},{"scope","Read-only actual occupied-component eligibility; no22D460 call ABI or result supplied"}};std::ofstream out(directory_/"status.json");out<<status.dump(2)<<'\n';require(bool(out),"Cannot retain component input capture status");}
};
} //namespace halo2::test
