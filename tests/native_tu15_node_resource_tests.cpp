#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_node_resource.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
struct ChildBoundary {};
struct ReadBoundary {};
namespace copies {
void run_126200(WorldState&){throw ChildBoundary{};}
#include "node_resource_bodies.inc"
}
namespace {
void check(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&text){std::vector<std::byte>out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>input){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:input){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t*pc{};mutable J reads=J::array();mutable unsigned ordinal{};int fault{-1};
    std::function<void(Memory&,std::uint32_t)>after_read;
    std::span<std::byte>view(std::uint32_t at,std::size_t size) {
        auto it=banks.upper_bound(at);check(it!=banks.begin(),"Unmapped node-resource bank");--it;
        check(std::uint64_t(at-it->first)+size<=it->second.size(),"Node-resource access escaped bank");return std::span(it->second).subspan(at-it->first,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override {
        auto&self=*const_cast<Memory*>(this);
        if(*pc!=0x1e5b24u) {
            if(int(ordinal++)==fault)throw ReadBoundary{};
            reads.push_back(J{{"instruction",*pc},{"address",at},{"bytes",out.size()}});
        }
        std::ranges::copy(self.view(at,out.size()),out.begin());if(after_read)after_read(self,at);
    }
    void write(std::uint32_t at,std::span<const std::byte>input)override{std::ranges::copy(input,view(at,input.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){auto out=view(at,4);for(unsigned n=0;n<4;++n)out[n]=std::byte(value>>(8*n));}
};
SourcePhysicsWorldRuntimeAbi abi_for(const J&row) {
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");
    const auto&fp=row.at("before_fp");a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};
    a.x87_status=fp[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}return a;
}
void supplemental() {
    constexpr std::uint32_t descriptor=0x61000100,node=0x61001000,resource=0x61002000,stack=0x62001000;
    for(unsigned mode=0;mode<12;++mode) {
        Memory seed;seed.banks.emplace(0,std::vector<std::byte>(4096));seed.banks.emplace(0x61000000,std::vector<std::byte>(65536));seed.banks.emplace(0x62000000,std::vector<std::byte>(8192));
        seed.put(descriptor+0x30,node);seed.put(descriptor+0xb0,resource);seed.put(node+0x16,0x10);seed.put(node+0x24,0);seed.put(resource+0x10,0x80);seed.put(stack,0x63000000);
        if(mode==0)seed.put(0x16,0);
        if(mode==1){seed.put(0x16,0x10);seed.put(0x24,0);}
        seed.after_read=[mode,done=false](auto&m,std::uint32_t at)mutable {
            if(done)return;
            if(mode==2&&at==descriptor+0x30){m.put(at,node+0x100);done=true;}
            if(mode==3&&at==node+0x16){m.put(node+0x24,1);m.put(resource+0x24,0x80);done=true;}
            if(mode==4&&at==node+0x24){m.put(descriptor+0xb0,resource+0x100);m.put(resource+0x110,1);done=true;}
            if(mode==5&&at==descriptor+0xb0){m.put(at,resource+0x100);done=true;}
            if(mode==6&&at==resource+0x10){m.put(at,0);done=true;}
            if(mode==7&&at==node+0x16){m.put(at,0);done=true;}
            if(mode==8&&at==node+0x24){m.put(at,1);done=true;}
            if(mode==9&&at==resource+0x10){m.put(stack,0xdeadbeef);done=true;}
            if(mode==10&&at==node+0x16){m.put(node+0x24,0x40000000);done=true;}
            if(mode==11&&at==descriptor+0x30){m.put(node+0x16,0);done=true;}
        };
        const auto run=[&](Memory&m,bool native){
            std::uint32_t pc{};m.pc=&pc;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto at){pc=at;};
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
            const SourcePhysicsContactRecordObserver observe;WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp={mode<2?0xffffffffu:0u,0xabcdef12,descriptor,0x44444444,stack,0x55555555,0x66666666,0x77777777};abi.eflags=0xad7;state.load(abi);
            bool tail{};try{if(native)check(native_node_resource_detail::bridge(state,[](auto&){throw ChildBoundary{};}),"Supplemental node-resource admission refused");else copies::reference::run_1e5af0(state);}catch(const ChildBoundary&){tail=true;}
            return std::pair{state.save(),tail};
        };
        Memory a=seed,b=seed;const auto x=run(a,false),y=run(b,true);
        check(x.first.gp==y.first.gp&&x.first.eflags==y.first.eflags&&x.second==y.second&&a.banks==b.banks&&a.reads==b.reads,"Canonical node-resource alias differs mode="+std::to_string(mode));
    }
}
unsigned admission() {
    Memory memory;SourcePhysicsWorldRuntimeServices services;unsigned callbacks{},checked{};
    const auto refuses=[&](SourcePhysicsContactImage image=SourcePhysicsContactImage::TitleUpdate15,const SourcePhysicsContactRecordObserver&observe={}) {
        WorldState state{memory,observe,services,image};SourcePhysicsWorldRuntimeAbi initial{};initial.gp={1,2,3,4,5,6,7,8};initial.eflags=0xad7;state.load(initial);
        check(!native_node_resource_detail::bridge(state,[](auto&){throw ChildBoundary{};}),"Unqualified node-resource admission");
        check(state.save().gp==initial.gp&&state.save().eflags==initial.eflags&&memory.reads.empty(),"Refused node-resource changed caller state");++checked;
    };
    refuses(SourcePhysicsContactImage::MapPack11);services.instruction=[&](auto){++callbacks;};refuses();
    std::uint32_t pc{};bool trace=true;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};refuses();trace=false;
    cursor.executor={};refuses();cursor.executor=std::this_thread::get_id();
    const auto original=services.instruction;services.instruction=[&](auto){callbacks+=2;};refuses();services.instruction=original;
    constexpr std::array changed{0x38846eu,0x21f590u,0x1e5b01u};cursor.service_sites=changed;refuses();cursor.service_sites=sites;
    cursor.observation_only_stores=false;refuses(SourcePhysicsContactImage::TitleUpdate15,[](const auto&){});
    check(callbacks==0,"Admission checks replaced actual services");return checked;
}
}
int main(int argc,char**argv)try {
    check(argc==2,"Supply node-resource physical fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    unsigned refused{};if(!std::getenv("HALO2_NODE_RESOURCE_REFERENCE_TEST")){supplemental();refused=admission();}
    std::size_t comparisons{},tails{},prefixes{};
    for(const auto&row:fixture.at("cases"))for(unsigned route=0;route<3;++route) {
        Memory memory;std::uint32_t pc{};memory.pc=&pc;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        if(row.contains("fault"))memory.fault=row.at("fault").at("ordinal");
        SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};services.instruction=[&](auto at){++callbacks;pc=at;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        const auto incoming=abi_for(row);const SourcePhysicsContactRecordObserver observe;WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(incoming);
        bool tail{},prefix{};try {
            if(route==0)copies::primary::run_1e5af0(state);else if(route==1)copies::actor::run_1e5af0(state);else copies::frame::run_1e5af0(state);
        }catch(const ChildBoundary&){tail=true;}catch(const ReadBoundary&){prefix=true;}
        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);const auto result=state.save();
        check(prefix==row.contains("fault"),name+": read-prefix ownership differs");
        check(tail==(!prefix&&row.at("stopped_at")==0x126200),name+": genuine child ownership differs");
        if(result.gp!=row.at("boundary_gp").get<std::array<std::uint32_t,8>>()||result.eflags!=row.at("boundary_eflags"))
            throw std::runtime_error(name+": node-resource GP/flags mismatch "+J{{"gp",result.gp},{"flags",result.eflags},{"expected_gp",row.at("boundary_gp")},{"expected_flags",row.at("boundary_eflags")}}.dump());
        check(result.xmm==row.at("boundary_xmm").get<decltype(result.xmm)>()&&result.x87_status==row.at("after_fp")[1]&&result.floating_point->mxcsr==row.at("after_fp")[2]&&result.floating_point->x87_control_word==row.at("after_fp")[0],name+": FP state differs");
        check(result.x87_depth==incoming.x87_depth,name+": live FP depth differs");for(unsigned n=0;n<incoming.x87_depth;++n)check(!std::memcmp(&result.x87[n],&incoming.x87[n],10),name+": live80 value differs");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": full bank differs");
        check(memory.reads==row.at("reads"),name+": explicit source reads differ");check(row.at("writes").empty(),name+": unexpected original parent store");
        const auto expected_pc=prefix?row.at("boundary").at("instruction").get<std::uint32_t>():tail?0x1e5b1fu:0x1e5b24u;
        check(pc==expected_pc,name+": cursor differs");
        if(std::getenv("HALO2_NODE_RESOURCE_REFERENCE_TEST"))check(callbacks>0,name+": forced reference callback absent");else check(callbacks==0,name+": native entered retained body");
        ++comparisons;tails+=tail;prefixes+=prefix;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"genuine_child_entries",tails},{"before_read_prefixes",prefixes},{"admission_refusals",refused}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
