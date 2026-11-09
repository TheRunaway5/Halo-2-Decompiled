#include "halo2/native_tu15_command_emit.hpp"
#include "halo2/native_tu15_command_part_prepare.hpp"
#include "halo2/native_tu15_command_walk.hpp"
#include "halo2/native_tu15_material_selection.hpp"
#include "halo2/source_opaque_sort.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
using Json=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};
    std::vector<Bank> banks;
    bool writable{};
    Json* observed_writes{};
    void read(std::uint32_t at,std::span<std::byte> out) const override {
        for(const auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+out.size()<=b.bytes.size()) {
            std::copy_n(b.bytes.begin()+(at-b.address),out.size(),out.begin());return;
        }
        throw std::out_of_range("Unmapped Native41030 input: "+std::to_string(at));
    }
    void write(std::uint32_t at,std::span<const std::byte> in) override {
        if(!writable)throw std::logic_error("Native41030 planning wrote its input banks");
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+in.size()<=b.bytes.size()) {
            std::copy(in.begin(),in.end(),b.bytes.begin()+(at-b.address));
            if(observed_writes) {
                std::uint32_t value=0;
                for(std::size_t i=0;i<in.size();++i)value|=std::to_integer<std::uint32_t>(in[i])<<(8u*i);
                observed_writes->push_back({at,value,in.size()});
            }
            return;
        }
        throw std::out_of_range("Unmapped native command group output: "+std::to_string(at));
    }
};
unsigned nibble(char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);}
std::string hex(std::span<const std::byte> bytes) {
    constexpr auto digits="0123456789abcdef";
    std::string out;
    for(auto byte:bytes) {
        const auto value=std::to_integer<unsigned>(byte);
        out.push_back(digits[value>>4]);out.push_back(digits[value&15u]);
    }
    return out;
}
Json run_group(Memory& memory,const Json& row,NativeTu15PhysicsFrameFp& fp) {
    memory.writable=true;
    const auto gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    const auto stack=gp[4],packet=stack-44u;
    const auto& a=row.at("args");
    NativeTu15CommandGroup44c80 args{a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]};
    auto stores=Json::array();
    const auto store=[&](auto instruction,auto address,auto value,unsigned width) {
        std::array<std::byte,4> bytes{};
        for(unsigned i=0;i<width;++i)bytes[i]=std::byte(value>>(8*i));
        memory.write(address,std::span(bytes).first(width));
        if(address<0x62000000u||address>=0x62020000u)stores.push_back({instruction,address,value,width});
    };
    NativeTu15CommandPartServices services;
    services.packet=packet;
    services.prepare_45cd0=[&](auto unit,auto part,auto matrix,auto& arithmetic) {
        std::array<std::byte,44> initial{};
        memory.read(packet,initial);
        const auto result=native_tu15_command_part_prepare_45cd0(memory,unit,part,matrix,initial,arithmetic);
        for(const auto& s:result.stores)store(s.instruction,packet+s.offset,s.value,s.bytes);
        return result.accepted;
    };
    services.emit_41030=[&](const auto& emit,auto& arithmetic) {
        const auto result=native_tu15_command_emit_41030(memory,emit,arithmetic);
        for(const auto& s:result.rows())store(s.instruction,s.address,s.value,s.width);
    };
    const auto end=native_tu15_command_group_44c80(memory,args,stack,fp,services);
    std::array<std::byte,44> final_packet{};
    std::array<std::byte,8> controls{};
    memory.read(packet,final_packet);memory.read(stack+0x10u,controls);
    return Json{{"stores",stores},{"rejected",false},{"fp",{fp.control_word,fp.status,fp.mxcsr}},
        {"eax",end},{"esp",stack+36u},{"packet",hex(final_packet)},{"controls",hex(controls)}};
}
Json run(const Json& row) {
    Memory memory;
    for(const auto& bank:row.at("banks")) {
        const auto& hex=bank.at("before").get_ref<const std::string&>();
        std::vector<std::byte> data(hex.size()/2);
        for(std::size_t i=0;i<data.size();++i)data[i]=std::byte(16*nibble(hex[2*i])+nibble(hex[2*i+1]));
        memory.banks.push_back({bank.at("address"),std::move(data)});
    }
    const auto& f=row.at("before_fp");NativeTu15PhysicsFrameFp fp{f[0],f[1],f[2]};
    if(row.at("entry")==0x44c80)return run_group(memory,row,fp);
    if(row.at("entry")==0x4cbe0) {
        const auto& a=row.at("args");
        try {
            const auto result=native_tu15_material_selection_4cbe0(memory,{a[0],a[1],a[2],a[3],a[4],a[5],row.at("gp")[4]});
            auto stores=Json::array();
            for(const auto& s:result.rows())stores.push_back({s.instruction,s.address,s.value,s.width});
            return Json{{"residual",false},{"accepted",result.accepted},{"stores",stores}};
        } catch(const std::invalid_argument&) {return Json{{"residual",true}};}
    }
    if(row.at("entry")==0x42750 || row.at("entry")==0x42090) {
        auto stores=Json::array();
        memory.writable=true;memory.observed_writes=&stores;
        const auto appends=row.at("entry")==0x42090?native_tu15_queue_camouflage_42090(memory,fp):
            native_tu15_queue_sections_42750(memory,row.at("matrix"),fp);
        return Json{{"stores",stores},{"fp",{fp.control_word,fp.status,fp.mxcsr}},{"appends",appends}};
    }
    if(row.at("entry")==0x140550) {
        std::vector<SourceOpaqueSortRecord> records(row.at("gp")[1].get<unsigned>());
        memory.read(row.at("gp")[0],std::as_writable_bytes(std::span(records)));
        const auto order=row.at("source")==0x3bbc0?SourceOpaqueSortOrder::layer5:SourceOpaqueSortOrder::material;
        source_opaque_sort(records,order);
        return Json{{"records",hex(std::as_bytes(std::span(records)))},
                    {"fp",{fp.control_word,fp.status,fp.mxcsr}}};
    }
    const auto& a=row.at("args");auto stores=Json::array();bool rejected=false;
    try {
        const auto plan=native_tu15_command_emit_41030(memory,{a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]},fp);
        for(const auto& s:plan.rows())stores.push_back({s.instruction,s.address,s.value,s.width});
    } catch(const std::invalid_argument&) {rejected=true;}
    return Json{{"stores",stores},{"rejected",rejected},{"fp",{fp.control_word,fp.status,fp.mxcsr}}};
}
}

int main() try {
    std::string line;
    while(std::getline(std::cin,line))std::cout<<run(Json::parse(line)).dump()<<'\n'<<std::flush;
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
