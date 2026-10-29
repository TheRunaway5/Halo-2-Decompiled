#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> values) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto value:values){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;
}
struct Prefix {};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    bool recording{},raw_stores{};mutable Json reads=Json::array();
    mutable std::optional<std::pair<std::uint32_t,std::size_t>> failed_access;
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing vector-math storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){
            failed_access={{address,size}};throw std::runtime_error("Vector-math storage extent differs");
        }
        return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());
        if(recording)reads.push_back(Json::array({address,out.size()}));
        std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());
        if(raw_stores)stores.push_back({0,address,
            {found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},
            {values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),banks.at(found->first).begin()+address-found->first);
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original vector-math fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Vector-math original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() &&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Vector-math reference lacks unchanged physical authority");
    std::size_t count{},reads{},stores{},prefixes{},pending_count{},normalized_count{},value_count{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>() && (!row.at("completed").get<bool>() || row.at("live_x87_unchanged").get<bool>()),name+": original physical evidence missing");
        for(bool reading:{false,true}) {
            Memory memory;
            for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            const auto fault=row.value("fault",Json::object());
            memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading && memory.phase=="after_write")continue;
            memory.recording=reading;
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading && !memory.raw_stores)observe=[&](const auto &event) {
                events.push_back(event);if(memory.phase=="after_write" && observed++==memory.ordinal)throw Prefix{};
            };
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            alignas(16) std::array<std::byte,108> host{},expected_host{},actual_host{};
            asm volatile("fnsave %0":"=m"(host)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            asm volatile("fld1\n\tfldz\n\tfldpi\n\tfnsave %0":"=m"(expected_host)::"memory","st","st(1)","st(2)");
            // Linux may clear unused saved pointer metadata on a context
            // switch. The live values/tags/CW/SW/TOP remain a full witness.
            std::fill(expected_host.begin()+12,expected_host.begin()+24,std::byte{});
            asm volatile("frstor %0"::"m"(expected_host):"memory");
            bool stopped{};std::optional<std::uint32_t> normalized;
            try {
                if(row.at("kind")=="distance") {
                    const auto publication=native_campaign_ai_publish_perception_distance_tu15(memory,row.at("first"),row.at("second"),row.at("destination"),fp,observe);
                    const auto pending=row.at("pending_extended").get<std::string>();
                    check(publication.pending.has_value()==!pending.empty(),name+": pending distance presence differs");
                    if(publication.pending) {
                        check(hex(publication.pending->extended)==pending,name+": pending distance value differs");
                        check(!publication.committed && !publication.written_bits && bool(publication.failure),name+": rejected publication fabricated success");
                        ++pending_count;
                    }else {
                        check(publication.committed && publication.written_bits.has_value(),name+": completed distance publication differs");
                        const auto written=bytes(row.at("writes")[0].at("written"));std::uint32_t original{};
                        for(unsigned n=0;n<4;++n)original|=std::to_integer<unsigned>(written[n])<<(n*8u);
                        check(*publication.written_bits==original,name+": published distance REAL differs");
                    }
                    try{publication.rethrow_if_failed();}
                    catch(const NativeCampaignAiPendingDistanceErrorTu15 &error) {
                        check(publication.pending && error.pending().extended==publication.pending->extended,name+": typed failure discarded owed distance");
                        check(bool(error.cause()),name+": typed failure discarded source cause");
                        if(row.value("actual_write_fault",false)) {
                            check(memory.failed_access && memory.failed_access->first==row.at("boundary").at("address") &&
                                memory.failed_access->second==row.at("boundary").at("bytes"),name+": original unmapped publication operand differs");
                            bool actual_cause{};
                            try{std::rethrow_exception(error.cause());}catch(const std::runtime_error &){actual_cause=true;}
                            check(actual_cause,name+": original missing-store cause was replaced");throw Prefix{};
                        }
                        std::rethrow_exception(error.cause());
                    }
                }else normalized=native_campaign_ai_normalize_vector_tu15(memory,row.at("first"),fp,observe);
            }catch(const Prefix &){stopped=true;}
            catch(...){asm volatile("frstor %0"::"m"(host):"memory");throw;}
            asm volatile("fnsave %0\n\tfrstor %1":"=m"(actual_host):"m"(host):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            check(expected_host==actual_host,name+": native query changed live host x87 values/environment");
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw && actual_sw==host_sw && _mm_getcsr()==host_mxcsr,name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": vector-math completion differs");
            check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],
                name+": vector-math FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            if(normalized){check(*normalized==row.at("result_bits").get<std::uint32_t>(),name+": vector magnitude bits differ");++normalized_count;}
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained vector-math bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                check(memory.reads==expected,name+": vector-math ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;
                check(events.size()==row.at("writes").size(),name+": vector-math store count differs");
                for(std::size_t n=0;n<events.size();++n) {
                    const auto &a=events[n];const auto &b=row.at("writes")[n];
                    check((memory.raw_stores || a.instruction==b.at("instruction")) && a.address==b.at("address") &&
                        hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": vector-math ordered store differs");
                }
                stores+=events.size();
            }
            prefixes+=stopped;
        }
        if(row.at("kind")!="distance"&&row.at("completed").get<bool>()) {
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            const auto address=row.at("first").get<std::uint32_t>();std::array<std::uint32_t,3> value{};
            for(unsigned axis=0;axis<3;++axis) {
                std::array<std::byte,4> raw{};memory.read(address+axis*4u,raw);
                for(unsigned n=0;n<4;++n)value[axis]|=std::to_integer<unsigned>(raw[n])<<(n*8u);
            }
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            memory.recording=true;const auto magnitude=native_campaign_ai_normalize_vector_value_tu15(memory,value,fp);
            check(magnitude==row.at("result_bits").get<std::uint32_t>(),name+": native value magnitude differs");
            check(Json::array({fp.control_word,fp.status,fp.mxcsr})==row.at("after_fp"),name+": native value FP differs");
            Json expected_reads=Json::array();for(const auto &read:row.at("reads"))
                if(read.at("address")==0x46a0d0u||read.at("address")==0x46a0ecu)
                    expected_reads.push_back(Json::array({read.at("address"),read.at("bytes")}));
            check(memory.reads==expected_reads,name+": native value constant order differs");
            for(const auto &bank:row.at("banks")) {
                check(memory.banks.at(bank.at("address"))==bytes(bank.at("before")),name+": native private value changed retained storage");
                const auto first=bank.at("address").get<std::uint32_t>();const auto after=bytes(bank.at("after"));
                if(address>=first&&std::uint64_t(address-first)+12<=after.size())
                    for(unsigned axis=0;axis<3;++axis) {
                        std::uint32_t expected{};for(unsigned n=0;n<4;++n)expected|=std::to_integer<unsigned>(after[address-first+axis*4u+n])<<(n*8u);
                        check(value[axis]==expected,name+": native value vector bits differ");
                    }
            }
            ++value_count;
        }
        ++count;
    }
    check(count && prefixes && pending_count && normalized_count && value_count==191,"Vector-math qualification matrix incomplete");
    std::cout<<count<<" physical original vector-math geometry cases, "<<reads<<" ordered reads, "<<stores<<" stores, "<<prefixes<<" prefixes; exact distance/normalization/FP/pending quantity,191 typed private values and ordered operands passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
