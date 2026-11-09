#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_animation_resource_wait.hpp"
#include "platform/native_tu15_async_promote.hpp"
#include "platform/native_tu15_kernel_yield.hpp"
#include "halo2/source_kernel_yield_tu15.hpp"
#include "halo2/animation_async_worker.hpp"
#include "halo2/native_threads.hpp"
#include <nlohmann/json.hpp>
#include <condition_variable>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
namespace reference {
static std::function<void(WorldState&,std::uint32_t)> owner;
static void run_1236a0(WorldState&s){owner(s,0x1236a0u);}
static void run_2dbf74(WorldState&s){owner(s,0x2dbf74u);}
#include "animation_resource_wait_reference.inc"
}
namespace reference_ai {
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){reference::owner(s,target);}
#include "animation_resource_wait_ai_reference.inc"
}
namespace {
void check(bool b,const std::string&why){if(!b)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string&s){std::vector<std::byte>r;for(std::size_t n=0;n<s.size();n+=2)r.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return r;}
std::string hex(std::span<const std::byte>s){std::string r;constexpr char d[]="0123456789abcdef";for(auto b:s){const auto n=std::to_integer<unsigned>(b);r+=d[n>>4];r+=d[n&15];}return r;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable std::recursive_mutex mutex;mutable J reads=J::array();J writes=J::array();std::function<void(std::uint32_t)> before_read;
    std::span<std::byte> span(std::uint32_t p,std::size_t n){auto i=banks.upper_bound(p);check(i!=banks.begin(),"Wait backing absent");--i;const auto off=std::uint64_t(p)-i->first;check(off<=i->second.size()&&n<=i->second.size()-off,"Wait backing escaped");return std::span(i->second).subspan(off,n);}
    void read(std::uint32_t p,std::span<std::byte>out)const override{if(before_read)before_read(p);std::lock_guard lock(mutex);reads.push_back(J{{"address",p},{"bytes",out.size()}});const auto v=const_cast<Memory*>(this)->span(p,out.size());std::copy(v.begin(),v.end(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>v)override{std::lock_guard lock(mutex);auto out=span(p,v.size());writes.push_back(J{{"address",p},{"before",hex(out)},{"written",hex(v)}});std::copy(v.begin(),v.end(),out.begin());}
    void put(std::uint32_t p,std::uint32_t v,unsigned n=4){std::lock_guard lock(mutex);auto out=span(p,n);for(unsigned k=0;k<n;++k)out[k]=std::byte(v>>(8*k));}
    std::uint32_t word(std::uint32_t p,unsigned n=4)const{std::lock_guard lock(mutex);const auto in=const_cast<Memory*>(this)->span(p,n);std::uint32_t v{};for(unsigned k=0;k<n;++k)v|=std::to_integer<std::uint32_t>(in[k])<<(8*k);return v;}
    J saved()const{std::lock_guard lock(mutex);J out=J::array();for(const auto&[p,v]:banks)out.push_back(J{{"address",p},{"before",hex(v)}});return out;}
};
SourcePhysicsWorldRuntimeAbi abi_for(const J&r){SourcePhysicsWorldRuntimeAbi a{};a.gp=r.at("gp");a.eflags=r.at("eflags");a.xmm=r.at("xmm");const auto fp=r.at("before_fp");a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};a.x87_status=fp[1];a.x87_depth=r.at("live_x87").size();for(unsigned n=0;n<a.x87_depth;++n){const auto v=bytes(r.at("live_x87")[n]);std::memcpy(&a.x87[n],v.data(),10);}return a;}
J abi_json(const SourcePhysicsWorldRuntimeAbi&a){J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));return J{{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"before_fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};}
struct PriorityBoundary{};
unsigned refusals(const J&row){
    for(unsigned n=0;n<7;++n){Memory memory;for(const auto&b:row.at("banks"))memory.banks.emplace(b.at("address"),bytes(b.at("before")));const auto banks=memory.banks;const auto incoming=abi_for(row);std::uint32_t pc{};bool trace=n==1;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){pc=p;};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f590u,0x1262b0u};services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};if(n==0)services.instruction_cursor.position=nullptr;if(n==2)services.instruction_cursor.executor={};if(n==3)services.instruction_cursor.callback_type=nullptr;if(n==4)services.instruction_cursor.service_sites=wrong;if(n==5)services.instruction_cursor.observation_only_stores=false;SourcePhysicsContactRecordObserver observer=[](const auto&){};WorldState state{memory,observer,services,n==6?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};state.load(incoming);check(!native_animation_resource_wait_detail::bridge(state,[](auto&,auto,auto,bool){throw std::runtime_error("Unqualified wait called child");}),"Unsafe wait admitted");check(abi_json(state.save())==abi_json(incoming)&&memory.banks==banks&&memory.reads.empty()&&memory.writes.empty(),"Wait refusal changed state");}return 7;
}
J run(const J&row,const std::filesystem::path&file,unsigned route){
    Memory memory;for(const auto&b:row.at("banks"))memory.banks.emplace(b.at("address"),bytes(b.at("before")));
    NativeSynchronization synchronization({0xf3000001u,0xf3000100ull});const auto native=synchronization.services();
    SourceAnimationResourceMemory raw;
    raw.read=[&](auto p,auto n){return std::span<const std::byte>(memory.span(p,n));};raw.write=[&](auto p,auto b){memory.write(p,b);};
    SourceAnimationAsyncWorkerServices base;base.memory=raw;
    base.bindings={{0x4ecc38u,0x4ef1b8u,0x4ef1bcu,0x4ecc28u,0x4ecc2cu,0x4ecc30u,0x61000000u,0x61000080u,0x61008000u,0x123600u},0x4ecc34u,0x4ef1c0u,0x61000220u,0x1aaa80u};
    base.wait=[&](auto h,auto ms){acquire_native_synchronization(native,h,ms,{},5);};
    base.release_mutex=[&](auto h){if(auto error=native.release_mutex(h))throw std::system_error(error);};
    base.release_semaphore=[&](auto h,auto n){if(auto result=native.release_semaphore(h,n);result.error)throw std::system_error(result.error);};
    base.yield=[](){native_kernel_yield_status();};
    SourceAsyncOwnedFiles files(std::array{SourceAsyncOwnedFile{0xf4000001u,file}});
    std::mutex gate_mutex;std::condition_variable_any gate;bool allowed=false;unsigned callbacks{},bytes_read{};
    std::promise<void> callback_entered,retired;auto entered=callback_entered.get_future();auto done=retired.get_future();
    const auto allow=[&](){std::lock_guard lock(gate_mutex);allowed=true;gate.notify_all();};
    std::uint32_t worker{};NativeThreads threads({0xf5000001u,0xf5000100ull});const auto workers=threads.services();NativeSynchronization::Checkpoint synchronization_before;
    struct Stop {const NativeThreadServices&s;std::uint32_t&worker;~Stop(){if(worker){s.request_stop(worker);s.join(worker);}}}stop{workers,worker};
    SourceAnimationAsyncConstructionServices construction;construction.memory=raw;construction.bindings=base.bindings.queue;
    construction.native_create_mutex=[&](auto,auto,auto){const auto result=native.create_mutex(false);if(result.error)throw std::system_error(result.error);return result.handle;};
    construction.native_create_semaphore=[&](auto,auto count,auto maximum,auto){const auto result=native.create_semaphore(count,maximum);if(result.error)throw std::system_error(result.error);return result.handle;};
    construction.native_create_worker=[&](auto,auto,auto,auto,auto,auto){
        synchronization_before=synchronization.checkpoint();
        const auto result=workers.create([&](std::stop_token token){
            auto owned=base;bool terminal=false,notified=false,announced=false;
            owned.wait=[&](auto h,auto ms){acquire_native_synchronization(native,h,ms,token,5);};
            owned.release_mutex=[&](auto h){base.release_mutex(h);if(terminal&&!notified&&h==memory.word(base.bindings.queue.free_mutex_slot)){notified=true;retired.set_value();}};
            owned.callback=[&](auto callback,auto payload){
                if(!announced){callback_entered.set_value();announced=true;}
                {std::unique_lock lock(gate_mutex);if(!gate.wait(lock,token,[&](){return allowed;}))throw NativeWorkerStopped();}
                check(callback==base.bindings.read_callback,"Worker callback owner differs");
                const auto result=read_source_animation_async_file(payload,row.at("chunk"),{0x64000000u,memory.banks.at(0x64000000u)},files,raw);
                ++callbacks;bytes_read+=result.io.bytes_read;terminal=result.completion.callback_result!=0;return result.completion.callback_result;
            };
            run_source_animation_async_worker(owned);
        });if(result.error)throw std::system_error(result.error);worker=result.handle;return worker;
    };
    construction.native_set_worker_priority=[](auto,auto){throw PriorityBoundary{};};
    try{construct_source_animation_async_queue(construction);}catch(const PriorityBoundary&){}
    const auto tracking=row.at("gp")[7].get<std::uint32_t>();const auto mode=row.at("mode").get<unsigned>();
    std::uint32_t job=0xffffffffu;
    if(mode){job=submit_source_animation_async_read({0x40u,row.at("size"),0x64000000u,0x61000200u,tracking+4u,6u,3},{0xf4000001u,0,0,0},base);memory.put(tracking+8u,job);
        check(entered.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Actual worker did not reach file callback");
        if(mode==1){allow();check(done.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Actual ready request did not retire");}
    }else memory.put(tracking+8u,0xffffffffu);
    std::uint32_t pc{};unsigned parent_callbacks{},yields{};J phases=J::array();
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){pc=p;if(p>=0x1262b0u&&p<=0x1262edu)++parent_callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    services.required_owner=[&](auto&m,const auto&call,auto&a,const auto&){
        const auto sp=a.gp[4];
        if(call.target==0x2e0478u){check(memory.word(sp+8u)==0xffffffffu,"Promotion did not wait infinitely");const auto result=native.wait(memory.word(sp+4u),0xffffffffu);check(!result.error&&result.state==NativeSynchronizationWaitState::Acquired,"Actual promotion mutex failed");std::array<std::byte,4>ret{};memory.read(sp,ret);a.gp[0]=0;a.gp[4]=sp+12u;return;}
        if(call.target==0x2e0342u){if(auto error=native.release_mutex(memory.word(sp+4u)))throw std::system_error(error);if(mode==4){allow();check(done.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Actual promotion-time completion did not retire");}std::array<std::byte,4>ret{};memory.read(sp,ret);a.gp[0]=1;a.gp[4]=sp+8u;return;}
        if(call.target==0x800000eeu){if(++yields==row.at("release_yield").get<unsigned>()){allow();check(done.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Actual pending request did not retire");}check(source_try_kernel_yield_tu15(m,call,a),"Real yield owner refused");return;}
        throw std::runtime_error("Unowned wait platform target");
    };
    const SourcePhysicsContactRecordObserver observer;WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi_for(row));
    const auto snapshot=[&](){auto out=abi_json(state.save());out["banks"]=memory.saved();out["reads"]=memory.reads;out["writes"]=memory.writes;return out;};
    const auto child=[&](WorldState&s,std::uint32_t target){auto phase=snapshot();phase["target"]=target;
        if(target==0x1236a0u){check(native_async_promote_detail::promote(s,[&](auto&v){check(native_async_order_detail::insert(v,0x1232c0u,[&](auto&comparison){check(native_async_order_detail::compare(comparison,0x1ab330u),"Actual comparator declined");}),"Actual insertion declined");}),"Actual promotion declined");}
        else if(target==0x2dbf74u)check(native_kernel_yield_detail::bridge(s,[](auto&v,auto t,auto p,bool tail){v.required(p,t,tail);}),"Actual yield declined");
        else throw std::runtime_error("Unowned wait child");phase["after"]=snapshot();phases.push_back(std::move(phase));};
    unsigned ready_reads{};
    memory.before_read=[&](auto address){if(mode==5&&address==tracking+4u&&++ready_reads==3u){check(pc==0x1262d0u,"Ready-read scheduling boundary moved");auto event=snapshot();event["target"]=pc;event["resume"]=pc;event["kind"]="real-worker-read-scheduling";allow();check(done.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Actual read-time completion did not retire");event["after"]=snapshot();phases.push_back(std::move(event));}};
    memory.reads=J::array();memory.writes=J::array();const auto before=snapshot();reference::owner=child;
    const bool forced=std::getenv("HALO2_REFERENCE_ANIMATION_RESOURCE_WAIT");
    if(route==1)reference::run_1262b0(state);else if(route==2)reference_ai::run_1262b0(state);else if(!native_animation_resource_wait_detail::bridge(state,[&](auto&s,auto t,auto,bool){child(s,t);})){check(forced,"Native wait declined");reference::run_1262b0(state);}
    memory.before_read={};auto result=snapshot();result["initial"]=before;result["phases"]=phases;result["pc"]=pc;result["parent_callbacks"]=parent_callbacks;result["yields"]=yields;result["job"]=job;result["worker_callbacks"]=callbacks;result["actual_bytes_read"]=bytes_read;
    check((route||forced)?parent_callbacks>0:parent_callbacks==0,"Wait parent route differs");check(memory.word(tracking+8u)==0xffffffffu,"Actual wait job not cleared");
    if(mode){
        check(memory.word(tracking+4u,1)==1,"Actual file worker did not own completion");check(bytes_read==row.at("size")&&memory.word(0x61000200u)==bytes_read,"Actual file byte count differs");
        std::ifstream actual_file(file,std::ios::binary);actual_file.seekg(0x40);std::vector<std::byte>expected(bytes_read);actual_file.read(reinterpret_cast<char*>(expected.data()),bytes_read);
        check(std::size_t(actual_file.gcount())==expected.size()&&std::ranges::equal(memory.span(0x64000000u,expected.size()),expected),"Worker payload differs from actual owned file request");
    }
    if(auto error=workers.request_stop(worker))throw std::system_error(error);if(auto error=workers.join(worker))throw std::system_error(error);
    const auto outcome=workers.outcome(worker);if(outcome.state==NativeThreadState::Failed&&outcome.failure)std::rethrow_exception(outcome.failure);check(outcome.state==NativeThreadState::Stopped,"Worker did not stop at native owner");check(synchronization.checkpoint()==synchronization_before,"Wait fixture retained mutex ownership or semaphore permits");worker=0;reference::owner={};return result;
}
}
int main(int argc,char**argv)try{check(argc==3,"Supply wait fixtures and actual owned file");std::ifstream input(argv[1]);const auto fixture=J::parse(input);const auto refused=refusals(fixture.at("cases")[0]);J result=J::array();for(const auto&row:fixture.at("cases"))for(unsigned route=0;route<3;++route)result.push_back(run(row,argv[2],route));std::cout<<J{{"cases",result},{"admission_refusals",refused}}.dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
