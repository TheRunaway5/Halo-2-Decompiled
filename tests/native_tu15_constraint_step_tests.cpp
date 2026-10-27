#include "halo2/native_tu15_constraint_step.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>bytes(const std::string&s){check(s.size()%2==0,"Invalid constraint fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{},log_writes{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};
    std::vector<SourcePhysicsContactRecordStore>raw_stores;std::function<void(std::uint32_t,std::size_t)>on_write;
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing constraint bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing constraint extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
    void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);if(on_write)on_write(p,b.size());}
    void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8u*i));write(p,b);}
    std::uint32_t get(std::uint32_t p,unsigned n=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
};
constexpr std::uint32_t base=0x1000000,island=base+0x100,step=base+0x200,solver=base+0x300,input=base+0x400,workspace=base+0x500,allocator=base+0x600,body_array=base+0x700,constraint_array=base+0x800,body=base+0x1000,motion=base+0x2000,arena=base+0x4000,extra=base+0x8000;
NativeTu15PhysicsConstraintInput arguments(std::uint32_t bodies=2,std::uint32_t constraints=0){return{{step},solver,input,{base+0x900},{island},{body_array},bodies,constraint_array,constraints};}
Memory model(){Memory m;m.banks[base].resize(0x10000);m.banks[0x41e000].resize(0x2000);m.banks[0x45f000].resize(0x1000);m.banks[0x48c000].resize(0x1000);m.banks[0x491000].resize(0x1000);
    m.put(0x491350,workspace);m.put(workspace+8,arena);m.put(workspace+0xc,0x1000);m.put(0x48c938,allocator);m.put(allocator,0x45f4cc);m.put(allocator+8,extra);m.put(allocator+0xc,0x2000);m.put(allocator+0x14,extra+0x1000);m.put(0x45f4f0,0x2e10a0);m.put(0x45f4f4,0x2e1110);
    m.put(island+8,0x40);m.put(island+0xc,0x100);m.put(island+0x10,0x40);m.put(island+0x14,0x100);m.put(island+0x18,0);m.put(solver+0x20,0x3f000000);
    for(unsigned n=0;n<3;++n){m.put(body_array+n*4,body+n*0x100);m.put(body+n*0x100+0x3c,motion+n*0x100);m.put(motion+n*0x100,0x41f260);}
    m.put(0x41f26c,0x3248d0);m.put(0x41f270,0x324800);m.put(0x41f2ec,0x325410);m.put(0x41f2f0,0x3249e0);m.put(0x41edb4,0x31f770);
    m.put(constraint_array,base+0xa00);m.put(constraint_array+4,base+0xb00);for(auto c:{base+0xa00,base+0xb00}){m.put(c,0x41ed90);m.put(c+0xc,body);m.put(c+0x10,body+0x100);}return m;
}
void models(){
    {auto m=model();NativeTu15PhysicsFrameFp fp;NativeTu15ConstraintStepServices s;NativeTu15ConstraintStepStats stats;
     s.solve_rows=[&](auto config,auto schemas,auto accumulators,auto slots,auto&){check(config==solver&&schemas==arena+0x298&&accumulators.identity==arena&&slots==arena+0x290&&m.get(schemas)==0x400,"Solver actual packet/marker");return 1u;};
     native_tu15_step_island_constraints(m,arguments(),fp,0,s,{},&stats);check(stats.native_accumulator_setups==2&&stats.native_fixed_commits==2&&m.get(workspace+0x10,1)==0&&m.get(motion+8)==0x80&&m.get(motion+0x108)==0x100,"Native fixed setup/commit and offset state");}
    {auto m=model();m.put(workspace+0xc,0x298);NativeTu15PhysicsFrameFp fp;NativeTu15ConstraintStepServices s;s.solve_rows=[&](auto,auto,auto,auto,auto&){m.put(0x491350,workspace+0x40);return 0u;};
     native_tu15_step_island_constraints(m,arguments(),fp,0,s);check(m.get(allocator+8)==extra&&m.get(allocator+0xc)==0x2000&&m.get(workspace+0x10,1)==1&&m.get(workspace+0x50,1)==0,"Schema spill rewinds real allocator and rereads workspace identity at cleanup");}
    {auto m=model();m.put(workspace+0xc,0x10);m.put(allocator+0xc,0);m.put(allocator+0x14,0);NativeTu15PhysicsFrameFp fp;NativeTu15ConstraintStepServices s;unsigned allocated{},released{};
     s.allocate_stack_block=[&](auto a,auto owner,auto size,auto&context){check(a.identity==allocator&&owner.identity==0x2e10a0&&size==0x2e0,"Whole workspace actual allocation ABI");++allocated;context.status=0x2801;return extra;};s.solve_rows=[](auto,auto,auto,auto,auto&){return 0u;};
     s.release_stack_block=[&](auto a,auto owner,auto p,auto n,auto&context){check(a.identity==allocator&&owner.identity==0x2e1110&&p==extra&&n==0x2e0&&context.status==0x2801,"Actual release pointer/extent/FP");++released;};
     native_tu15_step_island_constraints(m,arguments(),fp,0,s);check(allocated==1&&released==1,"Whole workspace allocate/release lifetime");}
    {auto m=model();NativeTu15PhysicsFrameFp fp;NativeTu15ConstraintStepServices s;unsigned builders{};
     s.build_constraint=[&](auto receiver,auto owner,auto packet,auto&context,auto&fp){check(owner.identity==0x31f770&&packet==input&&receiver.identity==base+0xa00+builders*0x100&&m.get(packet)==arena+0x80&&m.get(packet+4)==arena+0x100,"Actual constraint pair and receiver");if(!builders){context.jacobians=context.jacobian_limit;context.secondary_jacobians=base+0xc000;}else check(context.jacobians==base+0xc000&&context.jacobian_limit==0xffffffff,"Secondary jacobian rollover must observe prior builder state");context.schemas+=4;++builders;fp.status=0x4501;};
     s.solve_rows=[&](auto,auto schemas,auto,auto,auto&fp){check(builders==2&&m.get(schemas+8)==0x400&&fp.status==0x4501,"Builder schema publication/end marker/FP");return 0u;};native_tu15_step_island_constraints(m,arguments(2,2),fp,0,s);}
    {auto m=model();m.put(motion,0x41f2e0);NativeTu15PhysicsFrameFp fp;NativeTu15ConstraintStepServices s;unsigned calls{};
     s.setup_motion=[&](auto actual,auto owner,auto packet,auto gravity,auto out,auto&){check(actual.identity==motion&&owner.identity==0x325410&&packet.identity==step&&gravity.identity==base+0x900,"Actual dynamic accumulator setup ABI");++calls;m.put(motion,0x41f260);return NativeTu15ConstraintAccumulator{out.identity+0xa0};};s.solve_rows=[](auto,auto,auto,auto,auto&){return 1u;};native_tu15_step_island_constraints(m,arguments(),fp,0,s);check(calls==1&&m.get(motion+0x108)==0x120,"Next accumulator consumes real setup cursor; commit owner is reread");}
    {auto m=model();m.put(0x48c974,base+0xe000);m.put(0x48c978,base+0xe010);NativeTu15PhysicsFrameFp fp;NativeTu15ConstraintStepServices s;s.solve_rows=[&](auto,auto,auto,auto,auto&){m.put(island+0x18,16777217);return 0u;};native_tu15_step_island_constraints(m,arguments(0),fp,0,s);check(m.get(base+0xe004)==0x4b800000&&(fp.status&0x20)&&m.get(0x48c974)==base+0xe008,"Native profiling signed count conversion and complete FP");}
    std::cout<<"Constraint allocation/lifetime, mutable workspace, typed solver/builder/setup results, source cursor/owner reloads, fixed native leaves and native profiling models passed; row solver remains explicit\n";
}
void oracle(const std::filesystem::path&path){
    check(std::filesystem::exists(path),"Required original constraint fixture absent");std::ifstream in(path);const auto f=J::parse(in);
    check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-constraint-step-physical-operands-actual-abi-v1","Required physical constraint authority absent");check(f.at("body_sites")==J::array({306,25,17,3}),"Authenticated constraint closure changed");
    std::size_t count{},writes{},reads{},faults{};
    for(const auto&c:f.at("cases")){
        const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" physical original proof missing");Memory original;for(const auto&b:c.at("banks"))check(original.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate constraint bank");original.fault=c.value("fault",J{});
        const auto&values=c.at("arguments");NativeTu15PhysicsConstraintInput input{{values[0]},values[1],values[2],{values[3]},{values[4]},{values[5]},values[6],values[7],values[8]};
        for(bool read_pass:{false,true}){
            auto m=original;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);};
            NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};bool boundary{},failed{};NativeTu15ConstraintStepServices services;
            auto stop=[&](const std::string&kind,std::uint32_t receiver,const J&args,const J&packet=J{}){const auto&e=c.at("boundary");check(e!=nullptr&&e.at("kind")==kind&&e.at("receiver")==receiver&&e.at("arguments")==args&&e.at("packet")==packet,name+" actual constraint dependency ABI differs");boundary=true;throw std::runtime_error("original-constraint-boundary");};
            services.allocate_stack_block=[&](auto a,auto owner,auto n,auto&)->std::uint32_t{check(owner.identity==c.at("stop_pc"),name+" actual allocator owner differs");stop("allocate",a.identity,J::array({n}));return 0;};
            services.setup_motion=[&](auto motion,auto owner,auto step,auto gravity,auto cursor,auto&)->NativeTu15ConstraintAccumulator{check(owner.identity==c.at("stop_pc"),name+" actual motion setup owner differs");stop("setup",motion.identity,J::array({step.identity,gravity.identity,cursor.identity}));return{};};
            services.build_constraint=[&](auto receiver,auto owner,auto input,auto&context,auto&){check(owner.identity==c.at("stop_pc"),name+" actual constraint builder owner differs");stop("builder",receiver.identity,J::array({input}),J::array({context.jacobians,context.schemas,context.jacobian_limit,context.secondary_jacobians}));};
            services.solve_rows=[&](auto config,auto schemas,auto accumulators,auto results,auto&)->std::uint32_t{stop("solver",0,J::array({config,schemas,accumulators.identity,results}));return 0;};
            try{native_tu15_step_island_constraints(m,input,fp,0,services,observer);}catch(const Fault&){failed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="original-constraint-boundary")throw;}
            check(failed==fault_case&&boundary==!c.at("boundary").is_null(),name+" actual stop/read-fault completion differs");
            if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original constraint read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}reads+=m.reads.size();}
            else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered constraint store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered constraint store differs at "+std::to_string(n));}writes+=stores.size();}
            for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained constraint bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete constraint FP differs");
        }++count;faults+=!c.value("fault",J{}).is_null();
    }check(count>0,"Empty required constraint fixture");std::cout<<count<<" actual original constraint prefixes, "<<writes<<" ordered stores, "<<reads<<" ordered reads, "<<faults<<" read-fault prefixes and actual dependency arguments passed; solver/postsolver live proof remains open\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--oracle-only")oracle(argv[2]);else{models();if(argc==2)oracle(argv[1]);else check(argc==1,"Supply optional required constraint fixture");}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
