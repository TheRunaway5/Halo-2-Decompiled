// Native side of the TU1.5 collision ray 16C620 physical-original replay
// (tools/validate_native_tu15_collision_ray.py). Each stdin line is one
// live-captured case: the six stdcall arguments, the entry ESP, the x87
// CW/SW and MXCSR words and every declared 64-byte line before the call.
// The production binding core (native_collision_ray_detail::run_query: the
// journaled QueryMemory over native_tu15_query_bound_camera_scene) runs over
// exactly those lines; a read or write of any undeclared byte is a failure.
// Each stdout line holds EAX, the FP words and every line after the call.
#include "physics/runtime/native_tu15_collision_ray.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>

namespace {
using J=nlohmann::json;
using namespace halo2;
class LineMemory final : public SourcePhysicsContactMemory {
public:
    std::unordered_map<std::uint32_t,std::array<std::byte,64>> lines;
    bool borrow_lines{};
    std::byte *at(std::uint32_t a)const {
        const auto found=lines.find(a&~63u);
        if(found==lines.end())throw std::runtime_error("Native query touched an undeclared line "+std::to_string(a&~63u));
        return const_cast<std::byte*>(found->second.data())+(a&63u);
    }
    void read(std::uint32_t a,std::span<std::byte> out)const override{for(std::size_t n=0;n<out.size();++n)out[n]=*at(a+std::uint32_t(n));}
    void write(std::uint32_t a,std::span<const std::byte> in)override{for(std::size_t n=0;n<in.size();++n)*at(a+std::uint32_t(n))=in[n];}
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t a,std::size_t n)const noexcept override {
        const auto offset=a&63u;
        if(!borrow_lines||!n||n>64u-offset)return std::nullopt;
        const auto found=lines.find(a-offset);
        if(found==lines.end())return std::nullopt;
        return std::span<const std::byte>(found->second).subspan(offset,n);
    }
};
std::array<std::byte,64> decode(const std::string &s) {
    if(s.size()!=128)throw std::runtime_error("Declared line is not 64 bytes");
    std::array<std::byte,64> b{};
    for(std::size_t n=0;n<64;++n)b[n]=std::byte(std::stoul(s.substr(n*2,2),nullptr,16));
    return b;
}
std::string encode(const std::array<std::byte,64> &b) {
    static const char *d="0123456789abcdef";std::string s;s.reserve(128);
    for(const auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}
    return s;
}
}

// Journal semantics of the binding's QueryMemory: reads see journaled
// stores, nothing reaches the bank before commit, private-window DATA is
// host-only and unobserved, commit publishes in program order with each
// observed event after its store, and a refused query leaves the bank intact.
int self_test() {
    using native_collision_ray_detail::QueryMemory;
    const auto require=[](bool ok,const char *what){if(!ok)throw std::runtime_error(what);};
    LineMemory bank;
    for(std::uint32_t line=0x1000u;line<0x1100u;line+=64)bank.lines[line].fill(std::byte{0x11});
    std::vector<std::byte> host(native_collision_ray_detail::scratch_bytes);
    const std::uint32_t window=0x61000000u;
    QueryMemory memory(bank,window,host.data());
    NativeTu15MoppPrivateRayActivity activity;
    NativeTu15MoppPrivateRayMemory guarded(memory,window+0x100u,0x1000u,activity);
    require(!guarded.try_read_view(0x1008u,16u),"foreign provider refuses a retained read view");
    bank.borrow_lines=true;
    {
        const auto ordinary=guarded.try_read_view(0x1008u,16u);
        require(ordinary && ordinary->front()==std::byte{0x11},"ordinary retained read view");
    }
    require(!guarded.try_read_view(0x103eu,4u) && !activity.unprovided_reads,"filter-info read view is refused without effects");
    std::array<std::byte,4> raw{};
    bool filter_fault=false;
    try{guarded.read(0x103eu,raw);}catch(const std::runtime_error&){filter_fault=true;}
    require(filter_fault && activity.unprovided_reads==1,"filter-info read retains its ordered fault");
    const std::array<std::byte,4> a{std::byte{1},std::byte{2},std::byte{3},std::byte{4}};
    const std::array<std::byte,2> b{std::byte{9},std::byte{8}};
    memory.write(0x103eu,a);memory.write(0x1040u,b);
    require(!guarded.try_read_view(0x1044u,4u) && guarded.try_read_view(0x1050u,4u),"journal lines retain their read owners");
    std::array<std::byte,8> seen{};memory.read(0x103cu,seen);
    require(seen[0]==std::byte{0x11}&&seen[2]==std::byte{1}&&seen[3]==std::byte{2}&&seen[4]==std::byte{9}&&seen[5]==std::byte{8}&&seen[6]==std::byte{0x11},"journal read-back");
    bank.read(0x103eu,raw);require(raw[0]==std::byte{0x11},"journal escaped before commit");
    std::array<std::byte,16> vector{};memory.read_vector(0x1038u,vector);require(vector[6]==std::byte{1}&&vector[8]==std::byte{9},"journal vector read-back");
    memory.write(window+0x20u,a);std::array<std::byte,4> back{};memory.read(window+0x20u,back);
    require(back==a&&host[0x20]==std::byte{1},"private window");
    require(!guarded.try_read_view(window+0x20u,4u) && !guarded.try_read_view(window-2u,4u),"private DATA never borrows a retained read view");
    require(!memory.observes_retained_store(0,window+0x20u,4)&&memory.observes_retained_store(0,0x1040u,2),"private window observation");
    bool overlapped=false;try{memory.read(window-2u,back);}catch(const std::runtime_error&){overlapped=true;}
    require(overlapped,"window overlap");
    std::vector<std::pair<std::size_t,SourcePhysicsContactRecordStore>> events{{1,{0x103eu,0x103eu,{},{}}},{2,{0x1040u,0x1040u,{},{}}}};
    std::vector<std::uint32_t> order;
    memory.commit([&](const SourcePhysicsContactRecordStore &e){std::array<std::byte,2> now{};bank.read(e.address,now);order.push_back(e.address);
        require(now[0]==(e.address==0x103eu?std::byte{1}:std::byte{9}),"event before its store");},events);
    bank.read(0x103cu,std::span(seen));
    require(order==std::vector<std::uint32_t>{0x103eu,0x1040u}&&seen[2]==std::byte{1}&&seen[5]==std::byte{8},"commit");
    // A query over a bank without its globals refuses; nothing escapes.
    LineMemory empty;for(std::uint32_t line=0x2000u;line<0x2100u;line+=64)empty.lines[line].fill(std::byte{0x22});
    const auto before=empty.lines;
    NativeTu15PhysicsFrameFp fp{0x027fu,0,0x1f80u};std::uint32_t eax{};std::string refusal;
    const bool done=native_collision_ray_detail::run_query(empty,window,host.data(),{0x2040u,0x8000fu,0x2000u,0x2010u,~0u,~0u,0x2080u},{},eax,fp,refusal);
    require(!done&&!refusal.empty(),"refusal");
    require(empty.lines==before,"refused query escaped");
    std::cout<<"collision ray journal self-test passed ("<<refusal<<")\n";
    return 0;
}

int main(int argc,char **argv) {
    if(argc==2 && std::string(argv[1])=="--self-test") {
        try{return self_test();}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
    }
    try {
        std::string text;
        while(std::getline(std::cin,text)) {
            if(text.empty())continue;
            const auto c=J::parse(text);
            LineMemory memory;
            for(const auto &l:c.at("lines"))memory.lines.emplace(l.at(0).get<std::uint32_t>(),decode(l.at(1).get<std::string>()));
            const auto &args=c.at("args");
            const native_collision_ray_detail::Arguments a{c.at("esp"),args[0],args[1],args[2],args[3],args[4],args[5]};
            // The physical stack holds the return address and arguments.
            const std::array<std::uint32_t,7> block{0x63000000u,a.mask,a.origin,a.displacement,a.ignore_first,a.ignore_second,a.result};
            for(unsigned n=0;n<7;++n)for(unsigned k=0;k<4;++k)*memory.at(a.entry_esp+n*4u+k)=std::byte(block[n]>>(k*8u));
            std::uint32_t window=0x61000000u;
            for(;;window+=native_collision_ray_detail::scratch_bytes) {
                bool clear=true;
                for(const auto &[line,_]:memory.lines)if(line>=window && line<window+native_collision_ray_detail::scratch_bytes){clear=false;break;}
                if(clear)break;
            }
            std::vector<std::byte> host(native_collision_ray_detail::scratch_bytes);
            NativeTu15PhysicsFrameFp fp{c.at("fp")[0].get<std::uint16_t>(),c.at("fp")[1].get<std::uint16_t>(),c.at("fp")[2].get<std::uint32_t>()};
            std::uint32_t eax{};std::string refusal;
            const bool done=native_collision_ray_detail::run_query(memory,window,host.data(),a,{},eax,fp,refusal);
            J out{{"id",c.at("id")},{"completed",done},{"eax",eax},{"fp",J::array({fp.control_word,fp.status,fp.mxcsr})}};
            if(!done)out["refusal"]=refusal;
            J after=J::array();
            std::vector<std::uint32_t> order;for(const auto &[line,_]:memory.lines)order.push_back(line);
            std::sort(order.begin(),order.end());
            for(const auto line:order)after.push_back(J::array({line,encode(memory.lines.at(line))}));
            out["lines"]=after;
            std::cout<<out.dump()<<'\n'<<std::flush;
        }
        return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
