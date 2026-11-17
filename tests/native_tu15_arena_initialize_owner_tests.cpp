#include "halo2/native_tu15_arena_data.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include "halo2/physics_physical_storage_runtime.hpp"
#include "halo2/native_threads.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/runtime/native_tu15_arena_initialize_frame.hpp"
#include "physics/runtime/native_tu15_required_abi_fp.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <unistd.h>
using namespace halo2;using J=nlohmann::json;using State=physics_world_runtime_detail::WorldState;
namespace {
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t address){std::array<std::byte,4>bytes{};memory.read(address,bytes);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);return value;}
void put(SourcePhysicsContactMemory&memory,std::uint32_t address,std::uint32_t value){std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));memory.write(address,bytes);}
std::string hex(std::span<const std::byte>bytes){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:bytes){const auto value=std::to_integer<unsigned>(b);s+=d[value>>4];s+=d[value&15];}return s;}
std::string digest(std::span<const std::byte>bytes){std::array<unsigned char,EVP_MAX_MD_SIZE>out{};unsigned count{};check(EVP_Digest(bytes.data(),bytes.size(),out.data(),&count,EVP_sha256(),nullptr)==1,"SHA256 failed");return hex(std::span(reinterpret_cast<const std::byte*>(out.data()),count));}
J abi_packet(const SourcePhysicsWorldRuntimeAbi&abi){J live=J::array();for(unsigned n=0;n<abi.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10)));return J{{"gp",abi.gp},{"eflags",abi.eflags},{"fs_base",abi.fs_base},{"xmm",abi.xmm},{"fp",J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})},{"live_x87",live},{"active_mmx",bool(abi.active_mmx)}};}
struct Schedule {
    std::mutex mutex;std::condition_variable_any changed;unsigned idle{},epoch{};
    SourcePhysicsGameArenaSchedule services(){return {
        [this](auto token,std::stop_token stop){if(token!=0xf3000003u)return;std::unique_lock lock(mutex);++idle;changed.notify_all();const auto seen=epoch;if(!changed.wait(lock,stop,[&]{return epoch>seen;}))throw NativeWorkerStopped();},
        [this]{std::unique_lock lock(mutex);check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>0;}),"Real worker did not reach initial idle");const auto target=idle+1;++epoch;changed.notify_all();check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>=target;}),"Real worker did not retire its actual file job");}};}
    void initial_idle(){std::unique_lock lock(mutex);check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>0;}),"Initial worker wait unavailable");}
};
struct Directory {
    std::filesystem::path path;
    Directory(const std::filesystem::path&scratch,bool native){auto name=(scratch/(native?"arena-native-XXXXXX":"arena-reference-XXXXXX")).string();check(::mkdtemp(name.data()),"Cannot create a fresh owned arena file directory");path=name;}
    ~Directory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct OriginalStores {
    const J&fixture;std::ifstream stream;std::atomic<bool>active{};std::mutex mutex;std::size_t stores{},expected{};
    OriginalStores(const J&data,const std::filesystem::path&directory):fixture(data),stream(directory/data.at("store_file").get<std::string>(),std::ios::binary){
        check(bool(stream),"Missing original store stream");const auto first=data.at("steps")[1].at("stores").get<std::size_t>();expected=data.at("steps")[2].at("stores").get<std::size_t>()-first;stream.seekg(first*25);
    }
    void observe(const SourcePhysicsContactRecordStore&event){
        if(!active.load()||(event.address>=0x60000000u&&event.address<0x60200000u))return;
        std::lock_guard lock(mutex);std::array<std::byte,25>raw{};check(bool(stream.read(reinterpret_cast<char*>(raw.data()),raw.size())),"Original parent store stream exhausted");
        const auto integer=[](std::span<const std::byte>bytes){std::uint64_t value{};for(unsigned n=0;n<bytes.size();++n)value|=std::uint64_t(std::to_integer<unsigned>(bytes[n]))<<(8*n);return value;};
        std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<event.instruction;
        const auto pc=fixture.at("canonical_instruction_map").at(key.str()).get<std::uint32_t>();const auto data=std::span(raw);
        check(pc==integer(data.first(4))&&event.address==integer(data.subspan(4,4))&&event.before.size()==integer(data.subspan(8,1))&&integer(event.before)==integer(data.subspan(9,8))&&integer(event.written)==integer(data.subspan(17,8)),"Whole parent actual store differs from unchanged original at ordinal"+std::to_string(stores)+" pc="+key.str());++stores;
    }
};
struct Backend {
    SourcePhysicsGameArenaNative&owner;const SourcePhysicsWorldRuntimeServices&lower;const SourcePhysicsContactRecordObserver&observe;
    J calls=J::array();unsigned protects{};
    void call(State&state,std::uint32_t target,std::uint32_t site,bool tail){
        check(!tail,"Unexpected arena tail child");const auto expected_return=site==0x1252a4u?0x126669u:site==0x1252bau?0x12667fu:site==0x1252edu?0x1266b2u:site==0x125321u?0x1266e6u:0u;
        check(expected_return&&word(owner.memory(),state.gp[4])==expected_return,"Actual arena child return/caller ownership differs");
        auto abi=state.save();const auto before=abi_packet(abi);
        native_required_abi_fp_detail::invoke(state,abi,[&]{
            if(target==0x21b920u){
                check(site==0x1252a4u,"Foreign physical reservation caller");auto services=lower;
                services.required_owner=[&](auto&memory,const auto&call,auto&actual,const auto&){
                    check(&memory==&owner.memory()&&call.target==0x800000b6u&&call.instruction==0x2d7a21u&&call.tail&&word(memory,0x41b648u)==call.target,"Physical protection requires its real resource owner");
                    const auto sp=actual.gp[4];check(word(memory,sp)==0x21b970u,"Physical protect tail frame differs");
                    owner.protect_pages(word(memory,sp+4),word(memory,sp+8),word(memory,sp+12));actual.gp[4]=sp+16;++protects;
                };
                source_run_physics_physical_storage_runtime(owner.memory(),0x217020u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observe);
            }else if(target==0x21b990u){check(site==0x1252bau,"Foreign savegame initialization caller");owner.run(0x217090u,abi);}
            else if(target==0x166a80u){check(site==0x1252edu||site==0x125321u,"Foreign CRC caller");owner.run(0x1656a0u,abi);}
            else throw std::runtime_error("Unexpected actual arena child target");
        });
        calls.push_back(J{{"target",target},{"site",site},{"entry",before},{"returned",abi_packet(abi)}});
    }
};
Backend*backend{};
}
namespace retained {
using namespace halo2::physics_world_runtime_detail;using WorldState=halo2::physics_world_runtime_detail::WorldState;
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){backend->call(s,target,site,tail);}
void run_217090(WorldState&s){backend->call(s,0x21b990u,0x1252bau,false);}
void run_1656a0(WorldState&s){const auto return_word=word(s.memory,s.gp[4]);const auto pc=return_word==0x1266b2u?0x1252edu:return_word==0x1266e6u?0x125321u:0u;backend->call(s,0x166a80u,pc,false);}
#include "arena_initialize_parent.inc"
}
namespace {
J run(bool native,const J&fixture,const std::filesystem::path&fixture_directory){
    const auto scratch=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"));Directory owned_directory{scratch,native};const auto directory=owned_directory.path;
    SourceRuntimeMemory bank;mount_native_tu15_arena_data(bank);bank.mount(0x60000000u,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"private actual arena caller stack");
    Schedule schedule;OriginalStores stores{fixture,fixture_directory};std::atomic<std::uint64_t>child_observations{};SourcePhysicsWorldRuntimeServices lower;
    lower.instruction=[&](auto){++child_observations;};const SourcePhysicsContactRecordObserver observe=[&](const auto&event){stores.observe(event);};
    SourcePhysicsGameArenaNative owner{bank,{directory,0x60100000u,{0x27f,0x1f80},schedule.services()},lower,observe};
    for(auto entry:{0x12cb70u,0x1221b0u}){SourcePhysicsWorldRuntimeAbi input{};input.gp[4]=0x6000f000u;input.eflags=2;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(owner.memory(),input.gp[4],0x60300000u);owner.run(entry,input);}
    schedule.initial_idle();child_observations=0;Backend actual{owner,lower,observe};backend=&actual;
    unsigned parent_observations{};std::uint32_t position{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){position=pc;++parent_observations;};services.instruction_cursor={&position,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    SourcePhysicsWorldRuntimeAbi input{};input.gp={0x11111111u,0x22222222u,0x33333333u,0x44444444u,0x6000f000u,0x55555555u,0x66666666u,0x77777777u};input.eflags=0x202;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(owner.memory(),input.gp[4],0x60300000u);
    State state{owner.memory(),observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);stores.active=true;
    if(native){auto before=[](auto&,auto){};auto call=[&](auto&s,auto target,auto site,bool tail){actual.call(s,target,site,tail);};(void)native_arena_initialize_detail::run_private(state,before,call,{true,true});}
    else retained::run_125280(state);
    const auto result=abi_packet(state.save());stores.active=false;check(stores.stores==stores.expected,"Whole original parent store suffix not retired");
    check(native?parent_observations==0:parent_observations>0,"Parent control/reference ownership differs");check(actual.calls.size()==4&&actual.protects==1,"Genuine four-child resource closure differs");
    owner.stop();const auto resources=owner.ownership_checkpoint();check(resources.files.size()==2&&resources.synchronization.objects.size()==3&&resources.physical.allocations.size()==1,"Actual arena resource owners incomplete");
    J banks=J::array();bank.visit_committed([&](auto address,auto bytes){if(address>=0x60000000u&&address<0x60200000u)return;banks.push_back(J{{"address",address},{"bytes",bytes.size()},{"sha256",digest(bytes)}});});
    J files=J::array();for(unsigned index=0;index<2;++index){const auto file=directory/("savegame"+std::to_string(index)+".bin");check(std::filesystem::file_size(file)==4239360u,"Real file task extent differs");files.push_back(std::filesystem::file_size(file));}
    return J{{"abi",result},{"banks",banks},{"actual_child_abis",actual.calls},{"original_parent_stores",stores.stores},{"parent_instruction_observations",parent_observations},{"retained_child_instruction_observations",child_observations.load()},{"physical_protect_calls",actual.protects},{"real_file_bytes",files},{"retired_worker",true}};
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply authenticated original arena store fixture");const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch&&*scratch,"Actual arena owner needs disposable storage");setenv("XDG_CACHE_HOME",scratch,1);
    const std::filesystem::path fixture_path=argv[1];std::ifstream file(fixture_path);const auto fixture=J::parse(file);const auto reference=run(false,fixture,fixture_path.parent_path());std::cerr<<"genuine-reference-complete stores="<<reference.at("original_parent_stores")<<'\n';const auto native=run(true,fixture,fixture_path.parent_path());
    for(const auto*key:{"abi","banks","actual_child_abis","original_parent_stores","physical_protect_calls","real_file_bytes","retired_worker"})check(native.at(key)==reference.at(key),std::string("Native complete real-owner parent differs: ")+key+" actual="+native.at(key).dump()+" expected="+reference.at(key).dump());
    std::cout<<J{{"passed",true},{"native",native},{"reference",reference},{"scope","Private native parent with real backed page/protection/files/worker/CRC children; child CPU bodies and native platform ABI adaptation remain explicitly residual."}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
