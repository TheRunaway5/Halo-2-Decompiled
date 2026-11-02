#include "halo2/source_workshop_titles_tu15.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string& text) {
    need(text.size()%2==0,"Incomplete original title bytes");std::vector<std::byte> out;
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto b:bytes){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    void read(std::uint32_t at,std::span<std::byte> out)const override {for(auto& b:out)b=bytes.at(at++);}
    void write(std::uint32_t at,std::span<const std::byte> in)override {for(auto b:in)bytes[at++]=b;}
    void put(unsigned at,unsigned value){for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(n*8));}
};
}
int main(int argc,char** argv)try {
    need(argc==2,"Supply unchanged original title lifecycle cases");std::ifstream file(argv[1]);const auto reference=Json::parse(file);
    need(reference.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        reference.at("cases").size()==120 && reference.at("host_cpu_cases")==120 &&
        reference.at("instruction_patches").empty() && reference.at("intercepted_calls").empty(),"Original title lifecycle authority differs");
    unsigned count{},writes{};
    for(const auto& row:reference.at("cases")) {
        need(source_workshop_titles_tu15_owns_entry(0x13f160),"No whole native title lifecycle owner");
        Memory memory;memory.write(0x2100000,unhex(row.at("before")));
        memory.put(0x51df50,0x2100000);memory.put(0x51df54,0x2102000);memory.put(0x2102004,row.at("delta_bits"));
        memory.put(0x4ecc20,0x2104000);memory.put(0x21041f0,row.at("title_count"));memory.put(0x21041f4,0x2105000);
        for(unsigned n=0;n<3;++n){memory.put(0x210501c+n*36,row.at("timings").at(n).at(0));memory.put(0x2105020+n*36,row.at("timings").at(n).at(1));}
        for(const auto& input:reference.at("image_inputs"))memory.write(input.at("address"),unhex(input.at("bytes")));
        for(unsigned at=0x21ffb00;at<0x2200000;++at)memory.bytes[at]=std::byte{0xa5};memory.put(0x21fff00,0x3000000);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("initial_registers").get<decltype(abi.gp)>();abi.eflags=0x202;
        for(unsigned n=0;n<8;++n)for(unsigned j=0;j<4;++j)abi.xmm[n][j]=0x3f000000+n*4+j;
        SourcePhysicsWorldRuntimeServices services;std::vector<unsigned> trace;Json stores=Json::array();
        services.instruction=[&](unsigned pc){trace.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,auto&){throw std::runtime_error("Unexpected title lifecycle dependency");};
        const SourcePhysicsContactRecordObserver observe=[&](const auto& store) {
            if(store.address>=0x2100000 && store.address<0x2100024)
                stores.push_back({{"pc",store.instruction},{"address",store.address},{"bytes",store.written.size()},
                    {"before",hex(store.before)},{"after",hex(store.written)}});
        };
        source_run_workshop_titles_tu15(memory,0x13f160,abi,{0x27f,0x1f80},services,observe);
        std::vector<std::byte> after(36);memory.read(0x2100000,after);
        need(after==unhex(row.at("after")),"Native title lifecycle state differs");
        need(abi.gp==row.at("registers").get<decltype(abi.gp)>() && abi.eflags==row.at("eflags"),"Native title lifecycle GP/flags differ");
        for(unsigned n=0;n<8;++n)need(hex(std::as_bytes(std::span(abi.xmm[n])))==row.at("xmm").at(n).get<std::string>(),"Native title lifecycle SIMD differs");
        need(abi.x87_status==row.at("fpu_status"),"Native title lifecycle x87 status differs");
        need(trace==row.at("trace").get<std::vector<unsigned>>(),"Native title lifecycle instruction path differs");
        need(stores==row.at("stores"),"Native title lifecycle ordered stores differ");
        writes+=stores.size();++count;
    }
    std::cout<<"PASS original CPU/native title lifecycle cases="<<count<<" ordered_stores="<<writes<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
