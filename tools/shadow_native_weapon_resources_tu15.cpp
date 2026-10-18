// Back the declared native animation, geometry and texture I/O with real sectors.
// This does not execute Xbox asynchronous worker/queue scheduling.
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_runtime_resources.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
using namespace halo2;
using json=nlohmann::json;
namespace {
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    result.reserve(bytes.size()*2);
    for(auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);result+=digits[value>>4];result+=digits[value&15];}
    return result;
}
json identity(const SourceRuntimeResources::Identity &value) {
    json shared=json::array();
    for(const auto &entry:value.shared)if(entry)shared.push_back(hex(*entry));else shared.push_back(nullptr);
    return {{"resident",hex(value.resident)},{"shared",shared}};
}
void emit(json value){std::cout<<value.dump()<<'\n'<<std::flush;}
}
int main(int argc,char **argv) {
    try {
        if(argc!=5)throw FormatError("Supply MAP SHARED TU15 OWNERSHIP");
        std::ifstream input(argv[4]);const auto saved=json::parse(input);
        if(!saved.at("exported").get<bool>() || !saved.at("actual_native_owner").get<bool>() ||
           !saved.at("source_memory_unchanged").get<bool>())throw FormatError("Unproven resource ownership");
        Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
        const auto content=cache.load_content(0,true);
        SourceRuntimeResources resources(cache);
        const auto catalog=identity(resources.identity());
        if(catalog!=saved.at("source_resources_identity"))throw FormatError("Actual resource catalog differs from captured owner");
        auto next=saved.at("native_resource_reads").at("next").get<std::uint64_t>();
        std::map<std::uint32_t,json> jobs;
        for(const auto &row:saved.at("native_resource_reads").at("jobs"))
            if(!jobs.emplace(row.at("job").get<std::uint32_t>(),row).second)throw FormatError("Duplicate native resource owner");
        std::uint64_t expected=0xf8000001u;
        for(const auto &[job,row]:jobs) {
            if(job!=expected++ || !row.at("bytes").get<std::uint32_t>())throw FormatError("Invalid restored native resource namespace");
        }
        if(expected!=next || next>0x100000000ull)throw FormatError("Invalid next native resource identity");
        emit({{"ready",true},{"identity",catalog}});
        std::optional<json> pending;
        for(std::string line;std::getline(std::cin,line);) {
            const auto request=json::parse(line);const auto operation=request.at("op").get<std::string>();
            if(operation=="read" || operation=="read-sectors") {
                if(pending)throw FormatError("Uncommitted native resource read");
                const auto descriptor=request.at("descriptor").get<std::uint32_t>();
                const auto destination=request.at("destination").get<std::uint32_t>();
                const auto bytes=request.at("bytes").get<std::uint32_t>();
                const auto offset=request.at("encoded_offset").get<std::uint32_t>();
                std::string descriptor_hex;std::size_t authored_bytes=bytes;
                if(operation=="read") {
                    const auto image=resources.animation(descriptor);
                    if(offset!=Bytes(image.descriptor).u32(8) || bytes!=((image.bytes.size()+511u)&~511u))
                        throw FormatError("Native animation read differs from actual cache descriptor");
                    descriptor_hex=hex(image.descriptor);authored_bytes=image.bytes.size();
                }
                // Geometry/texture caller ownership and exact descriptor/page
                // expressions are checked against the live original bank by
                // the CPU-side native adapter before this actual sector read.
                const auto payload=resources.load(offset,bytes);
                pending=json{{"descriptor",descriptor},{"destination",destination},{"bytes",bytes}};
                emit({{"descriptor_hex",descriptor_hex},{"authored_bytes",authored_bytes},{"payload_hex",hex(payload)}});
            } else if(operation=="commit") {
                if(!pending || next>UINT32_MAX)throw FormatError("Cannot retain completed native resource read");
                const auto job=std::uint32_t(next);
                auto row=*pending;row["job"]=job;
                if(!jobs.emplace(job,row).second)throw FormatError("Native resource identity collision");
                ++next;pending.reset();emit({{"job",job}});
            } else if(operation=="checkpoint") {
                if(pending)throw FormatError("Cannot checkpoint uncommitted native I/O");
                json rows=json::array();for(const auto &[job,row]:jobs)rows.push_back(row);
                emit({{"next",next},{"jobs",rows}});
            } else throw FormatError("Unknown native resource operation");
        }
        if(pending)throw FormatError("Abandoned native I/O");
        return 0;
    } catch(const std::exception &error) {emit({{"error",error.what()}});return 1;}
}
