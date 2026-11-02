#include "halo2/native_tu15_light_receiver_flags.hpp"
#include "halo2/effect_light_receiver.hpp"
#include "halo2/source_object_admission.hpp"
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
#include <algorithm>
#include <iostream>
using namespace halo2;
using J=nlohmann::json;
namespace {
struct Fault {std::uint32_t address;};
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;
}
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};
    std::vector<Bank> banks;mutable J reads=J::array();J writes=J::array();
    std::span<const std::byte> view(std::uint32_t at,std::size_t size)const {
        for(const auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+size<=b.bytes.size())return std::span(b.bytes).subspan(at-b.address,size);
        throw Fault{at};
    }
    std::span<std::byte> view(std::uint32_t at,std::size_t size) {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+size<=b.bytes.size())return std::span(b.bytes).subspan(at-b.address,size);
        throw Fault{at};
    }
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t at,std::size_t size)const noexcept override {
        try{return view(at,size);}catch(...){return {};}
    }
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        const auto source=view(at,bytes.size());std::copy(source.begin(),source.end(),bytes.begin());reads.push_back({at,bytes.size()});
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {
        auto destination=view(at,bytes.size());writes.push_back(J{{"address",at},{"before",hex(destination)},{"written",hex(bytes)}});
        std::copy(bytes.begin(),bytes.end(),destination.begin());
    }
    std::uint32_t get(std::uint32_t at,unsigned size=4)const {
        const auto bytes=view(at,size);std::uint32_t word{};for(unsigned i=0;i<size;++i)word|=std::to_integer<unsigned>(bytes[i])<<(8*i);return word;
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned size) {
        auto bytes=view(at,size);for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
    }
};
J run(const J& row) {
    Memory m;for(const auto& input:row.at("banks")) {
        const auto& text=input.at("before").get_ref<const std::string&>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
        m.banks.push_back({input.at("address"),std::move(bytes)});
    }
    const auto gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    m.put(gp[4],row.at("return_pc"),4);m.put(gp[4]+4u,row.at("matrix"),4);m.put(gp[4]+8u,row.at("source"),4);
    NativeTu15LightReceiverFlagResult result;J stores=J::array();bool stopped{};
    const bool typed=row.value("projection",std::string{})=="typed";
    const auto old_mxcsr=_mm_getcsr();_mm_setcsr(0x5f80u);
    struct Restore {unsigned csr;~Restore(){_mm_setcsr(csr);}} restore{old_mxcsr};
    try {
        if(typed) {
            SourceBspAdmission bsp;SourceObjectAdmission objects;
            const auto read_rows=[&](unsigned offset,auto& rows) {
                const auto queue=m.get(gp[6]+offset),count=m.get(queue+4u,2);
                if(count>=32768u)throw std::runtime_error("Typed fixture has a negative source count");
                for(unsigned n=0;n<count;++n)rows.push_back({m.get(m.get(queue+0xcu)+4*n),std::uint16_t(m.get(m.get(queue+8u)+2*n,2)),
                    std::uint16_t(m.get(m.get(queue+0x14u)+2*n,2)),m.get(m.get(queue+0x10u)+4*n)});
            };
            read_rows(12,bsp.clusters);read_rows(20,objects.objects);read_rows(24,bsp.instances);
            const auto definition=m.get(m.get(0x4f0414u)+16u*(gp[1]&65535u)+8u),flags=m.get(definition);
            const auto a=source_effect_light_bsp_flags_31650(bsp,flags);
            const auto b=source_effect_light_object_flags_31650(objects,{flags,m.get(gp[4]+4u),m.get(gp[4]+8u)});
            result.bsp_rows=a.rows;result.object_rows=b.rows;result.changed_rows=a.changed+b.changed;
            result.owner_none_passes=b.owner_none_passes;result.root_passes=b.root_passes;
            const auto publish=[&](unsigned offset,const auto& rows) {
                const auto queue=m.get(gp[6]+offset);for(unsigned n=0;n<rows.size();++n)m.put(m.get(queue+8u)+2*n,rows[n].flags,2);
            };
            publish(12,bsp.clusters);publish(20,objects.objects);publish(24,bsp.instances);
            result.f8_written=source_effect_light_full_f8_31650(flags,std::uint16_t(bsp.clusters.size()));
            if(result.f8_written){result.xmm0_word=m.get(0x46a0d0u);m.put(m.get(m.get(0x4ecbdcu)+0x44u)+0x110u*(gp[0]&65535u)+0xf8u,result.xmm0_word,4);}
        }else {
            SourcePhysicsContactRecordObserver observe;
            if(row.value("observe",true))observe=[&](const auto& s){stores.push_back(J{{"instruction",s.instruction},{"address",s.address},{"before",hex(s.before)},{"written",hex(s.written)}});};
            native_tu15_light_receiver_flags_31650(m,{gp[0],gp[1],gp[6],gp[4]},result,observe);
        }
    }catch(const Fault& f) {
        if(!row.value("expect_zero_fault",false)||f.address)throw std::runtime_error("Native31650 reached undeclared address "+std::to_string(f.address));stopped=true;
    }
    if(_mm_getcsr()!=0x5f80u)throw std::runtime_error("Integer31650 kernel changed host MXCSR");
    J banks=J::array();for(const auto& b:m.banks)banks.push_back(J{{"address",b.address},{"after",hex(b.bytes)}});
    return {{"banks",banks},{"reads",m.reads},{"writes",m.writes},{"stores",stores},{"stopped",stopped},
        {"result",{{"bsp_rows",result.bsp_rows},{"object_rows",result.object_rows},{"changed_rows",result.changed_rows},
            {"owner_none_passes",result.owner_none_passes},{"root_passes",result.root_passes},{"f8_written",result.f8_written},{"xmm0_word",result.xmm0_word}}},
        {"host_mxcsr_preserved",true}};
}
}
int main()try {
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
