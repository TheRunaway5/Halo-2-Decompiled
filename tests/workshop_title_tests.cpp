#include "halo2/source_workshop_titles_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const std::string& why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string& text) {
    std::vector<std::byte> out;need(text.size()%2==0,"Incomplete original title bytes");
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    void read(std::uint32_t at,std::span<std::byte> out)const override {for(auto& b:out)b=bytes.at(at++);}
    void write(std::uint32_t at,std::span<const std::byte> in)override {for(auto b:in)bytes[at++]=b;}
    void put(unsigned at,unsigned value,unsigned size=4){for(unsigned n=0;n<size;++n)bytes[at+n]=std::byte(value>>(n*8));}
    unsigned word(unsigned at)const {unsigned value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes.at(at+n))<<(n*8);return value;}
};
}
int main(int argc,char** argv)try {
    need(argc==2,"Supply complete unchanged TU1.5 title cases");std::ifstream file(argv[1]);const auto reference=Json::parse(file);
    need(reference.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        reference.at("cases").size()==120 && reference.at("instruction_patches").empty() && reference.at("effects_intercepted").empty(),
        "Title reference lost its complete original owner");
    unsigned cases{},writes{},lazy_clock_controls{},parsed{},deferred{};
    for(const auto& row:reference.at("cases")) {
        const auto entry=row.at("entry").get<unsigned>();
        need(source_workshop_titles_tu15_owns_entry(entry),"Delayed title has no registered whole native evaluator/effect owner");
        Memory memory;memory.write(0x2100000,unhex(row.at("before")));
        memory.put(0x51df50,0x2100000);
        memory.put(0x51df54,row.at("first_free_slot")==4?0:0x2102000);
        if(row.at("first_free_slot")!=4)memory.put(0x2102002,std::uint16_t(row.at("tick_rate").get<int>()),2);
        else ++lazy_clock_controls;
        memory.put(0x2103000,row.at("title"));memory.put(0x2103004,row.at("delay_bits"));
        for(unsigned at=0x21ffb00;at<0x2200000;++at)memory.bytes[at]=std::byte{0xa5};
        memory.put(0x21fff00,0x3000000);
        const bool hs=row.at("hs");
        if(hs) {
            memory.put(0x480c00+561*4,0x459654);memory.put(0x459660,2,2);
            memory.put(0x459662,16,2);memory.put(0x459664,6,2);
            memory.put(0x21fff04,561);memory.put(0x21fff08,0x12340001);memory.put(0x21fff0c,0x23450002);
        }else {memory.put(0x21fff04,row.at("title"));memory.put(0x21fff08,row.at("delay_bits"));}
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("initial_registers").get<decltype(abi.gp)>();abi.eflags=0x202;
        for(unsigned i=0;i<8;++i)for(unsigned n=0;n<4;++n)abi.xmm[i][n]=0x3f000000+i*4+n;
        SourcePhysicsWorldRuntimeServices services;std::vector<unsigned> trace;Json calls=Json::array(),stores=Json::array();
        services.instruction=[&](unsigned pc){trace.push_back(pc);};
        services.required_owner=[&](auto&,const auto& call,auto& state,auto&) {
            const auto sp=state.gp[4];
            if(call.target==0x212010) {
                const std::array<unsigned,4> packet{memory.word(sp+4),memory.word(sp+8),memory.word(sp+12),memory.word(sp+16)};
                need(packet==std::array<unsigned,4>{0x12340001,2,0x459662,0x23450002},"Delayed title parser ABI differs");
                calls.push_back({{"target",call.target},{"arguments",packet}});
                state.gp[0]=row.at("parsed").get<bool>()?0x2103000:0;state.gp[4]+=20;
            }else if(call.target==0x211da0) {
                need(state.gp[0]==0 && state.gp[1]==0x12340001,"Delayed title completion ABI differs");
                calls.push_back({{"target",call.target},{"result",0},{"thread",0x12340001}});state.gp[4]+=4;
            }else throw std::runtime_error("Unexpected delayed-title dependency");
        };
        source_run_workshop_titles_tu15(memory,entry,abi,{0x27f,0x1f80},services);
        std::vector<std::byte> after(36);memory.read(0x2100000,after);
        need(after==unhex(row.at("after")),"Native delayed title slot or timing differs from unchanged original, case="+std::to_string(cases));
        need(abi.gp==row.at("registers").get<decltype(abi.gp)>() && abi.eflags==row.at("eflags"),"Native delayed title register/flags ABI differs");
        for(unsigned i=0;i<8;++i) {
            const auto expected=unhex(row.at("xmm").at(i));
            need(std::ranges::equal(std::as_bytes(std::span(abi.xmm[i])),expected),"Native delayed title complete SIMD state differs");
        }
        need(calls==row.at("calls"),"Native delayed title parser/completion schedule differs");
        need(trace==row.at("trace").get<std::vector<unsigned>>(),"Native delayed title instruction path differs");
        need(abi.x87_status==row.at("fpu_status"),"Native delayed title x87 status differs, case="+
            std::to_string(cases)+" expected="+row.at("fpu_status").dump()+" actual="+std::to_string(abi.x87_status));
        if(hs){if(row.at("parsed").get<bool>())++parsed;else ++deferred;}
        ++cases;writes+=row.at("stores").size();
    }
    std::cout<<"PASS delayed title cases="<<cases<<" original_slot_writes="<<writes
        <<" full_bank_missing_clock="<<lazy_clock_controls<<" parsed="<<parsed<<" deferred="<<deferred<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
