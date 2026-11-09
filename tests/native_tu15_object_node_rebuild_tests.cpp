#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_node_rebuild_abi.hpp"
#include "physics/runtime/native_tu15_object_prediction_copy.hpp"
#include "physics/runtime/native_tu15_object_animation_delivery_abi.hpp"
#include "native_tu15_object_node_admission_checks.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
struct Boundary {std::uint32_t entry;};
struct MemoryFault:std::runtime_error {using std::runtime_error::runtime_error;};
constexpr std::array bases{0x45d000u,0x45e000u,0x45f000u,0x46a000u,0x474000u,0x485000u,0x4dc000u,0x4e8000u,0x4ec000u,0x4f0000u,0x51d000u,0x52b000u,0x553000u,0x555000u,0x56e000u,0x61000000u,0x62000000u};
constexpr std::array sizes{4096u,4096u,4096u,4096u,4096u,4096u,4096u,4096u,4096u,4096u,4096u,4096u,4096u,4096u,4096u,262144u,16384u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,bases.size()> banks;
    mutable std::vector<std::pair<std::uint32_t,unsigned>> reads;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)> feedback;
    Memory(){for(unsigned n=0;n<banks.size();++n)banks[n].resize(sizes[n]);}
    std::span<std::byte> view(std::uint32_t at,unsigned size){
        for(unsigned n=0;n<banks.size();++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+size<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],size);
        throw MemoryFault("Unbacked declared controller operand "+std::to_string(at)+" width "+std::to_string(size));
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override{reads.emplace_back(at,out.size());std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());}
    void write(std::uint32_t at,std::span<const std::byte> in)override{std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value,unsigned size=4){auto b=view(at,size);for(unsigned n=0;n<size;++n)b[n]=std::byte(value>>(8*n));}
};
struct Entry {std::uint32_t entry{},cursor{};SourcePhysicsWorldRuntimeAbi abi;std::vector<std::byte> arguments;};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{},boundary{};bool memory_fault{};
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<Entry> entries;std::set<std::uint32_t> parent_sites,family_sites,prediction_sites;unsigned native_prediction_calls{};
};
thread_local Result *current;
thread_local bool use_native_prediction;
thread_local bool use_native_animation;
void note(WorldState &s,std::uint32_t entry){
    unsigned count{};
    switch(entry){case 0xbf0c0:case 0xbac00:case 0x145560:case 0x10e180:case 0x1e3150:case 0x1e5c80:case 0xb3a50:count=1;break;
    case 0x1707a0:case 0x144cf0:case 0x1e1720:count=2;break;case 0x1dbbd0:case 0xbfb90:case 0xadcd0:case 0x10e100:count=3;break;
    case 0xbfc70:case 0x1dbfd0:count=5;break;case 0xbf9a0:count=6;break;case 0x1d5840:count=7;break;default:break;}
    auto& memory=static_cast<Memory&>(s.memory);Entry packet{entry,current->pc,s.save(),{}};
    const auto bytes=memory.view(s.gp[4]+4u,count*4u);packet.arguments.assign(bytes.begin(),bytes.end());current->entries.push_back(std::move(packet));
}
}
namespace reference {
#include "object_node_rebuild_reference.inc"
}
namespace default_primary {
#include "object_node_rebuild_default_primary.inc"
}
namespace default_frame {
#include "object_node_rebuild_default_frame.inc"
}
namespace {
bool same_abi(const auto&a,const auto&b){
    bool same=a.gp==b.gp&&a.eflags==b.eflags&&a.xmm==b.xmm&&a.x87_depth==b.x87_depth&&a.x87_status==b.x87_status&&
        a.floating_point->mxcsr==b.floating_point->mxcsr&&a.floating_point->x87_control_word==b.floating_point->x87_control_word;
    for(unsigned n=0;n<a.x87_depth;++n)same=same&&!std::memcmp(&a.x87[n],&b.x87[n],10);
    return same;
}
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi &abi,bool native,bool native_animation=false,unsigned copy=0){
    Result result;current=&result;use_native_prediction=native;use_native_animation=native_animation;SourcePhysicsContactRecordObserver observe=[&](const auto&e){result.stores.push_back(e);if(memory.feedback)memory.feedback(memory,e);};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){result.pc=pc;if(pc>=0xbf0c0u&&pc<=0xbf62fu)result.parent_sites.insert(pc);
        if((pc>=0xb8780u&&pc<=0xb87ccu)||(pc>=0xbfb90u&&pc<=0xbfc64u))result.family_sites.insert(pc);
        if(pc>=0xadcd0u&&pc<=0xadce8u)result.prediction_sites.insert(pc);};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&result.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{
        if(copy==1)default_primary::run_bf0c0(state);
        else if(copy==2)default_frame::run_bf0c0(state);
        else if(native){if(!native_object_node_rebuild_detail::bridge(state,[](auto&s,std::uint32_t target,std::uint32_t site){reference::dispatch(s,target,site,false);}))throw std::runtime_error("Qualified parent candidate refused");}
        else reference::run_bf0c0(state);
    }catch(const Boundary& stop){result.boundary=stop.entry;}
    catch(const MemoryFault&){result.memory_fault=true;}
    result.abi=state.save();return result;
}
constexpr std::uint32_t stack=0x62002000u,frame=stack-0xf8u,pool=0x61000000u,rows=0x61000400u,tags=0x61000800u,
    body=0x61001000u,definition=0x61002000u,hlmt=0x61004000u,model=0x61006000u,records=0x61008000u,parent_body=0x61010000u,
    type=0x61016000u,identity=0x61017000u,alternate=0x61018000u,alt_definition=0x6101a000u,alt_model=0x6101c000u;
Memory seed(unsigned mode){
    Memory m;m.put(0x4ecbd0,pool);m.put(pool+0x44,rows);m.put(rows+8,body);m.put(rows+20,parent_body);m.put(0x4f0414,tags);
    m.put(tags+16+8,definition);m.put(tags+32+8,hlmt);m.put(tags+48+8,model);
    m.put(definition,1,1);m.put(definition+0x38,2u);m.put(hlmt+4,3u);m.put(model+0x48,4u);m.put(model+0x4c,records);
    m.put(alt_model+0x48,1u);m.put(alt_model+0x4c,records);m.put(alt_definition,0u);m.put(alt_definition+0x38,0xffffffffu);
    m.put(tags+4u*16u+8u,0x6102a000u);m.put(0x6102a030u,0x6102b000u);m.put(0x6102b016u,2u,1u);
    // Actual69220/85E70 registry publications and unchanged registered type9
    // descriptor/vtable words, authenticated by the owning Python validator.
    m.put(0x4dc04cu,0x5539b0u);m.put(0x4dc054u,0x555808u);m.put(0x5539b4u,0x61030000u);
    m.put(0x555830u,0x485fb4u);m.put(0x485fb4u,0x45dc78u);m.put(0x45dd00u,0xadcd0u);
    for(unsigned n=0;n<32;++n)m.put(0x474b90+n*4,type);
    m.put(type+0x84,0u);m.put(0x474d38,identity);m.put(identity+12,0x3f800000u);m.put(identity+28,0x3f800000u);
    for(unsigned n=0;n<4;++n)m.put(0x45fbc0+n*4,0x80000000u);
    m.put(0x46a0d0,0x3f800000u);m.put(0x46a0ec,0x34000000u);
    for(auto object:{body,parent_body,alternate}){
        m.put(object,1u);m.put(object+4,0u);m.put(object+0x14,0xffffffffu);m.put(object+0x18,0u,1);
        m.put(object+0x64,0x40400000u);m.put(object+0x68,0x40800000u);m.put(object+0x6c,0x40a00000u);
        m.put(object+0x70,0x3f800000u);m.put(object+0x84,0x3f800000u);m.put(object+0xa0,0x3f800000u);
        m.put(object+0xaa,1u,1);m.put(object+0xb4,0xffffffffu);m.put(object+0xd4,0xffffffffu);
        m.put(object+0x112,0x600u,2);m.put(object+0x114,208u,2);m.put(object+0x116,0xa00u,2);m.put(object+0x12a,0xffffu,2);
        m.put(object+0x180+0x68,0xffffffffu);
        for(unsigned n=0;n<40;++n){m.put(object+0x600+n*32+12,0x3f800000u);m.put(object+0x600+n*32+28,0x3f800000u);
            for(unsigned k:{0u,4u,20u,36u})m.put(object+0xa00+n*52+k,0x3f800000u);}
    }
    for(unsigned n=0;n<40;++n){m.put(records+n*96+4,n? n-1:0xffffu,2);m.put(records+n*96+0x24,0x3f800000u);}
    m.put(stack,0x63000000u);m.put(stack+4,0x12340000u);
    if(mode==0){m.put(definition,0u,1);m.put(body+0x34b,2u,1);}
    if(mode==1||mode==7)m.put(definition+0x38,0xffffffffu);
    if(mode==2)m.put(hlmt+4,0xffffffffu);
    if(mode==4)m.put(model+0x48,0u);
    if(mode==5||mode==22){m.put(model+0x48,1u);m.put(body+0x114,52u,2);}
    if(mode==6||mode==7||mode==24){m.put(body+0x14,0xabcd0001u);m.put(parent_body+4,0x400u);}
    if(mode==8||mode==26)m.put(body+0x112,0xffffu,2);
    if(mode==9)m.put(body+4,0x20000000u);
    if(mode==10){m.put(definition,0u,1);m.put(body+0x34b,1u,1);}
    if(mode>=11&&mode<=14){m.put(body+0x12a,0x180u,2);if(mode==11)m.put(body+0xb3,1u,1);if(mode>=13)m.put(body+0x180+0x68,0u);if(mode==14)m.put(hlmt+0xa4,1u);}
    if(mode==13||mode==14||mode==25){m.put(body+0x180,4u);m.put(body+0x184,0xffffu);}
    if(mode==15||mode==16){m.put(body+0xb4,0u);m.put(body+0xc0,0x40u,1);m.put(0x52be5c,0x61024000u);m.put(0x61024044,0x61025000u);if(mode==16){m.put(definition,0u,1);m.put(body+0x34b,1u,1);}}
    if(mode==17){m.put(body+0xd4,0u);m.put(body+0xd8,1u,1);}
    if(mode==23){m.put(model+0x48,33u);m.put(body+0x114,33u*52u,2);}
    if(mode==25){m.put(body+0x12a,0x180u,2);m.put(body+0x180+0x68,0u);}
    if(mode>=27&&mode<=30){
        m.put(body+0x12a,0x180u,2);m.put(body+0x114,0u,2);m.put(body+0x180,4u);m.put(body+0x184,0xffffu);m.put(body+0x180+0x68,0u);
        if(mode==28)m.put(0x6102b016u,0u,1);
        if(mode==29)m.put(hlmt+0xa4,1u);
        if(mode==30)m.put(body+0x180,0xffffffffu);
    }
    if(mode>=31&&mode<=33){
        m.put(body+0xd4,0u);m.put(body+0xd8,1u,1);m.put(0x5539b8u,4u);
        m.put(0x610320b0u,9u,2);m.put(0x610320b2u,1u,1);m.put(0x610320c8u,0x61035000u);m.put(0x610320ccu,0x61036000u);
        const std::array payload{mode-30u,0x41200000u,0x41a00000u,0x41f00000u,0u,0u,0x3f800000u,0u,0x3f800000u,0u};
        for(unsigned n=0;n<payload.size();++n)m.put(0x61036068u+n*4u,payload[n]);
    }
    m.feedback=[mode,done=false](auto&m,const auto&e)mutable{
        if(done)return;
        if(mode==18&&e.instruction==0xbf11fu){m.put(frame+0x20,alt_definition);done=true;}
        if(mode==19&&e.instruction==0xbf257u){m.put(frame+0x14,alt_model);done=true;}
        if(mode==20&&e.instruction==0xbf131u){m.put(frame+0x1c,alternate+0xa00u);done=true;}
        if(mode==21&&e.instruction==0xbf118u){m.put(rows+8,alternate);m.put(alternate+0x112,0xffffu,2);done=true;}
        if(mode==22&&e.instruction==0xbf114u){m.put(frame+0x1c,frame);done=true;}
        if(mode==24&&e.instruction==0xbf299u){m.put(frame+0x5c,alternate);done=true;}
        if(mode==25&&e.instruction==0xbf131u){m.put(frame+0x24,3u);done=true;}
        if(mode==26&&e.instruction==0xbf20cu){m.put(frame+0x18,alternate+0x600u);done=true;}
        if(mode==34&&e.instruction==0xbf16cu){m.put(frame+0x70,3u);done=true;}
    };
    return m;
}
std::string difference(const Result&a,const Result&b,const Memory&left,const Memory&right){
    for(unsigned n=0;n<8;++n)if(a.abi.gp[n]!=b.abi.gp[n])return "GP"+std::to_string(n)+"="+std::to_string(a.abi.gp[n])+"/"+std::to_string(b.abi.gp[n]);
    if(a.abi.eflags!=b.abi.eflags)return "flags="+std::to_string(a.abi.eflags)+"/"+std::to_string(b.abi.eflags);
    if(!same_abi(a.abi,b.abi))return "FP/XMM";
    if(a.boundary!=b.boundary||a.memory_fault!=b.memory_fault||a.pc!=b.pc)return "stop="+std::to_string(a.boundary)+"/"+std::to_string(b.boundary)+",pc="+std::to_string(a.pc)+"/"+std::to_string(b.pc);
    if(left.reads!=right.reads){auto n=std::mismatch(left.reads.begin(),left.reads.end(),right.reads.begin(),right.reads.end()).first-left.reads.begin();return "read ordinal="+std::to_string(n)+",sizes="+std::to_string(left.reads.size())+"/"+std::to_string(right.reads.size());}
    if(a.stores!=b.stores)return "stores";
    if(left.banks!=right.banks)return "banks";
    if(a.entries.size()!=b.entries.size())return "callee count";
    for(unsigned n=0;n<a.entries.size();++n){const auto&x=a.entries[n];const auto&y=b.entries[n];if(x.entry!=y.entry||x.cursor!=y.cursor||x.arguments!=y.arguments||!same_abi(x.abi,y.abi))return "callee packet "+std::to_string(n)+" entry="+std::to_string(x.entry);}
    return {};
}
}
SourcePhysicsWorldRuntimeAbi initial_abi(bool df){SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp={0x12345678u,0x23456789u,0x3456789au,0x456789abu,stack,0x56789abcu,0x6789abcdu,0x789abcdeu};abi.eflags=0x2d7u|(df?1024u:0u);
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};abi.x87_status=0x4100;abi.x87_depth=2;abi.x87[0]=1.234567890123456789L;abi.x87[1]=-9.876543210987654321L;
        for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=n*0x11111111u+k;
        return abi;
}
std::uint32_t word(){std::uint32_t v{};std::cin.read(reinterpret_cast<char*>(&v),4);if(!std::cin)throw std::runtime_error("Truncated whole-controller fixture");return v;}
void emit(std::uint32_t v){std::cout.write(reinterpret_cast<const char*>(&v),4);}
void emit_abi(const auto&a){
    for(auto v:a.gp)emit(v);emit(a.eflags);emit(a.floating_point->mxcsr);emit(a.x87_status);emit(a.floating_point->x87_control_word);
    for(const auto&x:a.xmm)for(auto v:x)emit(v);
}
void emit_live(const auto&a){emit(a.x87_depth);for(unsigned n=0;n<a.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&a.x87[n]),10);}
int main(int argc,char**argv){try{
    if(argc==2&&std::string_view(argv[1])=="--live-admission"){
        const auto cases=object_node_test_detail::check_live_admission_fallbacks(
            [](auto&s){return native_object_node_rebuild_detail::bridge(s,[](auto&,auto,auto){throw std::runtime_error("Rejected parent called child");});},
            [](auto&s){reference::run_bf0c0(s);});
        std::cout<<"live_protocol_fallbacks="<<cases<<'\n';return 0;
    }
    if(argc==2&&(std::string_view(argv[1])=="--admission"||std::string_view(argv[1])=="--reference")){
        const auto cases=object_node_test_detail::check_admission_refusals<Memory>([](auto&s){return native_object_node_rebuild_detail::bridge(s,[](auto&,auto,auto){throw std::runtime_error("Rejected parent called child");});},0xbf0c0u,0xbf62fu,std::string_view(argv[1])=="--reference");
        std::cout<<"admission_refusals="<<cases<<'\n';return 0;
    }
    if(argc==2&&std::string_view(argv[1])=="--emit-fixtures"){
        emit(70u);
        for(unsigned mode=0;mode<35;++mode)for(bool df:{false,true}){
            auto memory=seed(mode);const auto abi=initial_abi(df);auto trial=memory;const auto result=run(trial,abi,false);
            emit(mode);emit(df);emit(result.memory_fault);emit(result.boundary);emit_abi(abi);emit_live(abi);
            for(auto&b:memory.banks)std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());
        }
        return 0;
    }
    if(argc==2&&(std::string_view(argv[1])=="--physical"||std::string_view(argv[1])=="--physical-primary"||std::string_view(argv[1])=="--physical-frame")){
        const unsigned copy=std::string_view(argv[1])=="--physical-primary"?1u:std::string_view(argv[1])=="--physical-frame"?2u:0u;
        std::uint64_t calls{},parent_sites{},family_sites{},prediction_sites{};
        const auto parent_before=native_object_node_rebuild_detail::activity().entries,copy_before=native_object_prediction_copy_detail::activity().entries;
        const auto family_before=native_object_animation_detail::activity();
        while(std::cin.peek()!=std::char_traits<char>::eof()){
            SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=word();abi.eflags=word();const auto mxcsr=word(),status=word(),cw=word();
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=status;
            for(auto&x:abi.xmm)for(auto&v:x)v=word();abi.x87_depth=word();for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);
            Memory memory;for(auto&b:memory.banks)std::cin.read(reinterpret_cast<char*>(b.data()),b.size());
            const auto got=run(memory,abi,true,true,copy);emit_abi(got.abi);++calls;parent_sites+=got.parent_sites.size();family_sites+=got.family_sites.size();prediction_sites+=got.prediction_sites.size();
            for(auto&b:memory.banks)std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());
            emit(got.stores.size());for(const auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}
            emit(got.pc);emit(got.boundary);emit(got.memory_fault);emit(got.entries.size());
            for(const auto&e:got.entries){emit(e.entry);emit_abi(e.abi);emit_live(e.abi);emit(e.arguments.size());std::cout.write(reinterpret_cast<const char*>(e.arguments.data()),e.arguments.size());}
        }
        const auto family_after=native_object_animation_detail::activity();
        std::cerr<<"{\"calls\":"<<calls<<",\"native_parent_entries\":"<<native_object_node_rebuild_detail::activity().entries-parent_before
            <<",\"native_animation_entries\":"<<(family_after.delivery-family_before.delivery)+(family_after.selection-family_before.selection)+(family_after.presence-family_before.presence)
            <<",\"native_prediction_entries\":"<<native_object_prediction_copy_detail::activity().entries-copy_before
            <<",\"literal_parent_sites\":"<<parent_sites<<",\"literal_animation_sites\":"<<family_sites<<",\"literal_prediction_sites\":"<<prediction_sites<<"}\n";
        return 0;
    }
    unsigned cases{},returns{},stops{},faults{},packets{},prediction_calls{};std::set<std::uint32_t> sites;
    for(unsigned mode=0;mode<35;++mode)for(bool df:{false,true}){
        auto original=seed(mode),native=original;const auto abi=initial_abi(df);
        const auto a=run(original,abi,false),b=run(native,abi,true);const auto why=difference(a,b,original,native);
        if(!why.empty())throw std::runtime_error("Controller mismatch mode="+std::to_string(mode)+",DF="+std::to_string(df)+": "+why);
        if(!b.parent_sites.empty())throw std::runtime_error("Private native controller entered literal parent body");
        sites.insert(a.parent_sites.begin(),a.parent_sites.end());++cases;packets+=a.entries.size();prediction_calls+=b.native_prediction_calls;if(a.boundary)++stops;else if(a.memory_fault)++faults;else ++returns;
    }
    std::cout<<"{\"cases\":"<<cases<<",\"returns\":"<<returns<<",\"genuine_child_entry_stops\":"<<stops<<",\"retained_memory_faults\":"<<faults<<",\"callee_packets\":"<<packets<<",\"parent_sites\":"<<sites.size()<<",\"native_prediction_calls\":"<<prediction_calls<<",\"native_parent_instruction_callbacks\":0}\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
