#include "halo2/native_tu15_startup_timer.hpp"
#include <bit>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using J=nlohmann::json;using M=halo2::SourceRuntimeMemory;using A=halo2::SourcePhysicsContactRuntimeAbi;
struct TimerRequired {};
void check(bool v,const char *why) { if(!v)throw std::runtime_error(why); }
std::vector<std::byte> bytes(const std::string &s) {
    check(!(s.size()&1),"Invalid byte input");std::vector<std::byte> b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return b;
}
void put(M &m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);
}
A state(const J &j) {
    A a{};a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();
    a.eflags=j.at("eflags");a.fs_base=j.at("fs_base");a.x87_status=j.at("x87_status");
    a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{j.at("x87_control_word"),j.at("mxcsr")};return a;
}
void cpu_equal(const A &a,const J &j,std::uint32_t mask=0xcc5) {
    const auto b=state(j);
    check(a.gp==b.gp && a.xmm==b.xmm && !((a.eflags^b.eflags)&mask) && a.fs_base==b.fs_base &&
        a.x87_depth==0 && a.x87_status==b.x87_status && !a.active_mmx && a.floating_point &&
        a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr,"Native startup GP/flags/XMM/FP state differs");
}
void change_equal(const halo2::SourcePhysicsContactRecordStore &a,const J &b) {
    check(a.address==b.at("address") && a.before==bytes(b.at("before")) &&
        a.written==bytes(b.at("written")),"Native startup ordered store differs");
}
void bank_equal(M &m,std::uint32_t p,const std::vector<std::byte> &b) {
    check(std::ranges::equal(m.view(p,b.size()),b),"Native startup retained bank differs");
}
struct HeapContinuationRequired {};
struct ShutdownRequired {};
std::uint32_t word(const M &m,std::uint32_t p) {
 const auto b=m.view(p,4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
void callback_case(M &m,const J &row) {
 const auto &c=row.at("power_dpc");m.write(c.at("frame_address"),bytes(c.at("frame")));auto a=state(c.at("before"));
 std::size_t trace=0,stores=0;halo2::SourcePhysicsContactRuntimeServices services;
 services.instruction=[&](auto p) {check(trace<c.at("instructions").size() && p==c.at("instructions").at(trace++),"Original power callback trace differs");};
 services.required_owner=[&](auto &bank,const auto &call,auto &cpu,const auto&) {
  check(&bank==&m && call.instruction==0x2e00a6 && call.target==0x80000168 && !call.tail && !c.at("boundary").is_null(),"Unexpected original shutdown owner");
  bank_equal(m,cpu.gp[4],bytes(c.at("boundary").at("frame")));throw ShutdownRequired{};
 };
 bool stopped=false;try {halo2::source_run_tu15_power_dpc(m,a,{0x27f,0x1f80},services,[&](const auto &s) {
  check(stores<c.at("stores").size(),"Extra original power callback store");const auto &b=c.at("stores").at(stores++);check(s.instruction==b.at("instruction"),"Power callback store attribution differs");change_equal(s,b);
 });}catch(const ShutdownRequired&) {stopped=true;}
 check(stopped==row.at("automatic_power_down").get<bool>() && trace==c.at("instructions").size() && stores==c.at("stores").size(),"Power callback lost its original return/required owner");
 cpu_equal(a,c.at("after"),0xcd5);bank_equal(m,c.at("frame_address").get<std::uint32_t>()-4,bytes(c.at("final_stack")));
}
void real_expiration(const char *image) {
 for(bool shutdown:{false,true}) {
  halo2::NativeTu15StartupPlatform platform({shutdown});auto m=platform.load_image(image);const std::uint32_t sp=0x6000f000;
  m.mount(0x60000000,std::vector<std::byte>(0x10000),M::Access::ReadWrite,"real timer procedure stack");
  halo2::source_initialize_process_dpc(m,0x56edc0,0x2e009d,0);halo2::source_initialize_process_timer(m,0x56ede0,0);put(m,0x56ee08,shutdown?1:0);
  int fd=-1;
  {
   halo2::NativeTu15StartupTimer timer(m,halo2::native_tu15_timer_clock());fd=timer.native_descriptor();
   const auto arm=[&](std::int64_t interval,bool previous) {
    const auto raw=std::bit_cast<std::uint64_t>(interval);put(m,sp,0x2e009c);put(m,sp+4,0x56ede0);put(m,sp+8,std::uint32_t(raw));put(m,sp+12,std::uint32_t(raw>>32));put(m,sp+16,0x56edc0);
    A a{};a.gp[0]=0x12345678;a.gp[4]=sp;a.eflags=2;std::size_t observations=0;
    check(timer.try_call({0x2dc5e6,0x80000095,false},a,{},[&](const auto &o) {
      ++observations;check(o.interval==interval && o.due_time==o.interrupt_time+std::uint64_t(-interval) && o.previously_inserted==previous,"Real native timer clock/arm result differs");
    }),"Actual native timer call unhandled");
    check(observations==1 && a.gp[0]==(0x12345600u|(previous?1u:0u)) && a.gp[4]==sp+20 && a.eflags==2,"Native timer Boolean ABI differs");
   };
   arm(-216000000000ll,false);check(timer.remaining_100ns()>215990000000ull && timer.remaining_100ns()<=216000000000ull,"Original six-hour request not armed in native timer resource");
   arm(-216000000000ll,true);check(timer.cancel() && !timer.cancel() && timer.remaining_100ns()==0 && word(m,0x56ede4)==0,"Actual native cancellation/rearm state differs");
   // Reject wrong frames before changing the real resource or source record.
   for(unsigned fault=0;fault<5;++fault) {
    put(m,sp,0x2e009c);put(m,sp+4,0x56ede0);put(m,sp+8,0xb5659000);put(m,sp+12,0xffffffcd);put(m,sp+16,0x56edc0);
    A a{};a.gp[4]=sp;auto call=halo2::SourcePhysicsContactRuntimeCall{0x2dc5e6,0x80000095,false};
    if(fault==0)call.tail=true;if(fault==1)put(m,sp,0xbad);if(fault==2)put(m,sp+4,0x56ede4);if(fault==3)put(m,sp+16,0);if(fault==4)put(m,sp+12,0);
    const auto before=std::vector<std::byte>(m.view(0x56edc0,72).begin(),m.view(0x56edc0,72).end());bool rejected=false;
    try {timer.try_call(call,a);}catch(const std::exception&) {rejected=true;}
    check(rejected && a.gp[4]==sp && timer.remaining_100ns()==0,"Invalid native timer frame changed its resource");bank_equal(m,0x56edc0,before);
   }
   arm(-50000ll,false);pollfd p{fd,POLLIN,0};check(::poll(&p,1,250)==1 && (p.revents&POLLIN),"Real five-millisecond native timer did not expire");
   bool rejected=false;try {timer.dispatch_due({});}catch(const std::invalid_argument&) {rejected=true;}check(rejected,"Missing actual DPC executor consumed expiration");
   unsigned callbacks=0;bool requested=false;
   try {check(timer.dispatch_due([&](auto dpc) {
    ++callbacks;check(dpc.address==0x56edc0 && dpc.routine==0x2e009d && dpc.context==0 && word(m,0x56ede4)==1,"Real expiration lost its actual retained DPC");
    put(m,sp,0x3000000);put(m,sp+4,dpc.address);put(m,sp+8,dpc.context);put(m,sp+12,dpc.argument1);put(m,sp+16,dpc.argument2);
    A a{};a.gp[4]=sp;a.eflags=2;halo2::SourcePhysicsContactRuntimeServices services;
    services.required_owner=[&](auto&,const auto &call,auto&,const auto&) {check(call.instruction==0x2e00a6 && call.target==0x80000168,"Real DPC requested unexpected lifecycle owner");throw ShutdownRequired{};};
    halo2::source_run_tu15_power_dpc(m,a,{0x27f,0x1f80},services);check(!shutdown && a.gp[4]==sp+20,"Real original DPC return differs");
   }),"Real expiration was not dispatched");}catch(const ShutdownRequired&) {requested=true;}
   check(callbacks==1 && requested==shutdown && m.view(0x56ede3,1)[0]==std::byte{0} && word(m,0x56ede4)==1,"Real expiry/DPC lifecycle differs");
   check(!timer.dispatch_due([](auto){throw std::runtime_error("Duplicate timer callback");}),"One-shot native expiration replayed");
   arm(-216000000000ll,false);
  }
  check(::fcntl(fd,F_GETFD)==-1 && errno==EBADF && m.view(0x56ede3,1)[0]==std::byte{0},"Native timer retirement failed");
  bool released=false;try {m.view(0x90001000,8);}catch(const std::exception&) {released=true;}check(released,"Native timer list owner leaked");
 }
}
}
int main(int argc,char **argv) {try {
 check(argc==3,"Supply original native-timer fixture and owned TU1.5 image");for(unsigned i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
 std::ifstream input(argv[1]);const auto f=J::parse(input);std::map<std::uint32_t,std::uint32_t> canonical;
 for(auto it=f.at("source_instructions").begin();it!=f.at("source_instructions").end();++it)canonical.emplace(std::stoul(it.key(),nullptr,16),it.value().at("canonical"));
 std::size_t prefixes=0,total_trace=0,total_stores=0,heaps=0,returned=0;
 for(const auto &row:f.at("cases")) {
  halo2::NativeTu15StartupPlatform platform({row.at("automatic_power_down")});auto m=platform.load_image(argv[2]);
  m.mount(f.at("stack_address"),bytes(f.at("initial_stack_hex")),M::Access::ReadWrite,"fresh original stack");m.mount(0,std::vector<std::byte>(0x1000),M::Access::ReadWrite,"standalone FS chain");put(m,0,0xffffffff);put(m,row.at("stack_pointer"),0x3000000);
  halo2::SourceRuntimeHostVirtualMemory vm(m);halo2::PhysicsPlatformLocks locks;halo2::SourceProcessShutdownRegistry registry(m,f.at("primary_registry_owner").at("head"));
  const auto clock=f.at("native_environment").at("procedure_interrupt_time").get<std::uint64_t>();halo2::NativeTu15StartupTimer timer(m,{[clock]{return clock;},[]{return 0x0102030405060708ull;}});
  auto a=state(row.at("before"));std::size_t trace=0,stores=0,records=0,registrations=0,queries=0,timer_stores=0,critical=0,resources=0,timer_observations=0;
  std::vector<J> er,es,eq,et,ec;
  for(const auto &call:row.at("platform_calls"))for(const auto &s:call.at("stores")) {auto v=s;v["canonical_instruction"]=call.at("canonical_instruction");(call.at("target")==0x8000002f?es:call.at("target")==0x80000123?ec:er).push_back(v);}
  for(const auto &call:row.at("native_calls"))for(const auto &s:call.at("stores")) {auto v=s;v["trace_index"]=call.at("trace_index");(call.at("target")==0x80000095?et:eq).push_back(v);}
  halo2::SourcePhysicsContactRuntimeServices services;
  services.instruction=[&](auto p) {check(trace<row.at("instructions").size() && p==canonical.at(row.at("instructions").at(trace++)),"Original longer startup trace differs");};
  services.required_owner=[&](auto &bank,const auto &call,auto &cpu,const auto&) {
   check(&bank==&m && call.instruction==0x2d7ff3 && call.target==0x2dba2d && !call.tail,"Unexpected post-heap required owner");bank_equal(m,cpu.gp[4],bytes(row.at("boundary").at("frame")));throw HeapContinuationRequired{};
  };
  const auto observe=[&](const auto &s) {
   const bool timer_write=s.instruction==0x2dc5e6 && (s.address>=0x56ede0 && s.address<0x56ee08 || s.address==0x90001000 || s.address==0x90001004);
   const auto &b=timer_write?et.at(timer_stores++):row.at("stores").at(stores++);check(trace==b.at("trace_index") && s.instruction==(timer_write?0x2dc5e6u:b.at("canonical_instruction").template get<std::uint32_t>()),"Startup store order/source attribution differs");change_equal(s,b);
  };
  const auto record=[&](const auto &s) {const auto &b=er.at(records++);check(trace==b.at("trace_index") && s.instruction==b.at("canonical_instruction"),"Record source attribution differs");change_equal(s,b);};
  const auto shutdown=[&](const auto &s) {const auto &b=es.at(registrations++);check(trace==b.at("trace_index") && s.address==b.at("address") && s.before==bytes(b.at("before")) && s.written==bytes(b.at("written")),"Shutdown list differs");};
  const auto query=[&](const auto &s) {const auto &b=eq.at(queries++);check(trace==b.at("trace_index"),"Native query order differs");change_equal(s,b);};
  const auto lock=[&](const auto &s) {const auto &b=ec.at(critical++);check(trace==b.at("trace_index") && s.address==b.at("address") && s.before==bytes(b.at("before")) && s.written==bytes(b.at("written")),"Actual heap critical-section initialization differs");};
  const auto memory=[&](const auto &event) {
   const auto &b=row.at("resources").at(resources++);const auto &q=event.request;const auto &r=event.result;
   check(J::array({q.base,q.zero_bits,q.bytes,q.allocation_type,q.protection})==b.at("request") && J::array({r.status,r.base,r.bytes})==b.at("result"),"Actual heap reserve/commit request differs");
   if(!b.at("initial_hex").is_null())bank_equal(m,r.base,bytes(b.at("initial_hex")));
  };
  const halo2::PhysicsPlatformLockServices forbidden{[]()->std::uint32_t{throw std::runtime_error("Unexpected current-thread request");},[](auto){throw std::runtime_error("Unexpected wait");},[](auto){throw std::runtime_error("Unexpected wake");}};
  if(row.at("canonical_entry")==0x2d7f8eu) {
   auto bad=a;bad.eflags|=0x400u;const auto original=bad;bool rejected=false;
   try {halo2::source_run_native_tu15_startup_with_timer(platform,timer,m,0x2d7f8e,bad,{0x27f,0x1f80},services,vm,locks,forbidden,registry);}
   catch(const std::invalid_argument&) {rejected=true;}
   check(rejected && bad.gp==original.gp && bad.eflags==original.eflags && trace==0 && word(m,0x56ee28)==0 && timer.remaining_100ns()==0,
      "Invalid incoming full-startup direction ABI was executed");
  }
  bool stopped=false;try {halo2::source_run_native_tu15_startup_with_timer(platform,timer,m,row.at("canonical_entry"),a,{0x27f,0x1f80},services,vm,locks,forbidden,registry,observe,record,shutdown,query,[&](const auto &o) {
   ++timer_observations;check(o.interrupt_time==clock && o.due_time==clock+216000000000ull && o.interval==-216000000000ll && !o.previously_inserted,"Timer arm observation differs");
  },memory,lock);}catch(const HeapContinuationRequired&) {stopped=true;}
  check(stopped==(row.at("boundary").at("kind")=="required_post_heap_owner") && trace==row.at("instructions").size() && stores==row.at("stores").size() && records==er.size() && registrations==es.size() && queries==eq.size() && timer_stores==et.size() && critical==ec.size() && resources==row.at("resources").size() && timer_observations==1,"Longer original startup did not complete its exact prefix");
  cpu_equal(a,row.at("after"));for(const auto &bank:row.at("final_banks"))bank_equal(m,bank.at("address"),bytes(bank.at("hex")));
  check(word(m,0x56ee28)==row.at("heap_pointer") && timer.remaining_100ns()>215990000000ull,"Source heap publication or actual six-hour timer missing");
  if(stopped)++heaps;else ++returned;callback_case(m,row);++prefixes;total_trace+=trace;total_stores+=stores;
 }
 real_expiration(argv[2]);
 std::cout<<"PASS native TU1.5 timer/startup: "<<prefixes<<" prefixes, "<<total_trace<<" original instructions, "<<total_stores<<" source stores, "<<heaps<<" source-installed heaps, "<<returned<<" setup returns; real rearm/cancel/expiry/DPC/retirement pass\n";
 }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
