#include "halo2/cache.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_result_recording_support_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply original recording-support fixture");
  Json fixture;std::ifstream(argv[1])>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),
      "Original recipient fixture changed instructions or supplied returns");
  SourceRuntimeKernelTickCount clock{0x60212000u,{},"Explicit recording cold fixture clock"};
  const auto cold=native_tu15_world_memory(clock);
  unsigned cold_bytes{};
  for(const auto& row:fixture.at("cold_regions")) {
    const auto address=row.at("address").get<std::uint32_t>();
    std::vector<std::byte> expected(row.at("size").get<unsigned>());
    for(const auto& pair:row.at("nonzero")) {
      const auto index=pair[0].get<unsigned>();check(index<expected.size(),"Cold original sparse byte escapes its declaration");
      expected[index]=std::byte(pair[1].get<unsigned>());
    }
    check(std::ranges::equal(cold.view(address,expected.size()),expected),"New recording/context declaration differs from the unchanged original cold image");
    check(cold.is_read_only(address,expected.size())!=row.at("writable").get<bool>(),"Recording cold owner changed original section access");
    cold_bytes+=expected.size();
  }
  check(fixture.at("cold_regions").size()==20 && cold_bytes==50304,"Original recording cold ownership coverage changed");
  const auto verify_metadata=[&](const Json& rows) {
    unsigned bytes{};
    for(const auto& row:rows) {
    const auto address=row.at("address").get<std::uint32_t>();
    std::vector<std::byte> expected(row.at("size").get<unsigned>());
    for(const auto& pair:row.at("nonzero")) {
      const auto index=pair[0].get<unsigned>();check(index<expected.size(),"Original award byte escapes its declaration");
      expected[index]=std::byte(pair[1].get<unsigned>());
    }
    check(std::ranges::equal(cold.view(address,expected.size()),expected),"Retained award table/name differs from original metadata");
    check(cold.is_read_only(address,expected.size())!=row.at("writable").get<bool>(),"Retained award metadata changed original section access");
      bytes+=expected.size();
    }
    return bytes;
  };
  const auto award_bytes=verify_metadata(fixture.at("award_regions"));
  check(fixture.at("award_regions").size()==33 && award_bytes==1100,"Original award metadata extent changed");
  const auto combat_bytes=verify_metadata(fixture.at("combat_regions"));
  check(fixture.at("combat_regions").size()==11 && combat_bytes==303,"Original weapon/PvP metadata extent changed");
  constexpr std::uint32_t base=0x6000000,stack=0x200ff00,stop=0x3000000;
  unsigned cases{};
  for(const auto& row:fixture.at("cases")) {
    std::vector<std::byte> input(0x10000);
    const auto put=[&](unsigned offset,std::uint32_t value){for(unsigned n=0;n<4;++n)input[offset+n]=std::byte(value>>(n*8));};
    const auto local=row.at("local").get<unsigned>();check(local<16,"Original local-machine input escapes capacity");
    put(0xc,row.at("session").get<bool>()?base+0x2000:0);put(0x1c,row.at("present").get<bool>()?1u<<local:0);
    const std::array identity{std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte(local+1)};
    std::copy(identity.begin(),identity.end(),input.begin()+0x24+local*6);
    const auto query=row.at("matching").get<bool>()?identity:std::array{std::byte{9},std::byte{8},std::byte{7},std::byte{6},std::byte{5},std::byte{4}};
    std::copy(query.begin(),query.end(),input.begin()+0x1000);
    put(0x2000+0x741c,std::uint32_t(row.at("mode").get<int>()));put(0x2000+0x40,local);
    std::copy(identity.begin(),identity.end(),input.begin()+0x2000+0x58+local*0x10c+10);
    SourceRuntimeMemory memory;
    memory.mount(base,input,SourceRuntimeMemory::Access::ReadWrite,"Explicit original machine/session input");
    memory.mount(0x2000000,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"Explicit original caller stack");
    memory.write(stack,std::as_bytes(std::span(&stop,1)));
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={base+0x1000,base,0x13579bdf,0x2468ace0,stack,0x11223344,0x55667788,0x99aabbcc};abi.eflags=0x202;
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](auto& m,const auto& call,auto& a,const auto& observer) {
      check(call.target==0x877c0 && source_physics_live_support_tu15_owns_entry(call.target),"Recipient lookup reached an unowned service");
      source_run_physics_live_support_tu15(m,call.target,a,{0x27f,0x1f80},services,observer);
    };
    source_run_result_recording_support_tu15(memory,0x878c0,abi,{0x27f,0x1f80},services);
    check(abi.gp==row.at("gp").get<std::array<std::uint32_t,8>>() && (abi.eflags&0x8d5u)==row.at("eflags").get<unsigned>(),
        "Complete original recipient registers, stack or defined flags differ");
    check(std::ranges::equal(memory.view(base,input.size()),input),"Original recipient lookup mutated its input context");
    ++cases;
  }
  check(cases==144,"Original recipient admission coverage changed");
  unsigned award_cases{};
  for(const auto& row:fixture.at("award_cases")) {
    const auto field=row.at("field").get<unsigned>(),slot=row.at("slot").get<unsigned>();
    check(field<32 && slot<16,"Original award input escapes its actual32-row/player owner");
    std::vector<std::byte> bank(0x4040);
    const auto set=[&](unsigned offset,unsigned value){bank[offset]=std::byte(value);bank[offset+1]=std::byte(value>>8);};
    for(unsigned i=0;i<bank.size()/2;++i)set(i*2,(i*73+0x3141)&65535u);
    const auto offset=(slot*437+45+field)*2;set(offset,row.at("before").get<unsigned>());
    SourceRuntimeMemory memory;
    memory.mount(0x47a6e0,cold.view(0x47a6e0,32*16),SourceRuntimeMemory::Access::ReadWrite,"Actual cold original award descriptors");
    memory.mount(0x51eef4,bank,SourceRuntimeMemory::Access::ReadWrite,"Explicit original complete statistic input");
    memory.mount(0x51dfa0,std::array{std::byte(row.at("enabled").get<unsigned>()),std::byte(row.at("frozen").get<unsigned>())},
        SourceRuntimeMemory::Access::ReadWrite,"Explicit original recording gates");
    memory.mount(0x2000000,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"Explicit original caller stack");
    memory.write(stack,std::as_bytes(std::span(&stop,1)));
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={field,slot,0x13579bdf,std::uint32_t(row.at("delta").get<int>()),
        stack,0x11223344,0x55667788,0x99aabbcc};abi.eflags=0x202;
    check(source_physics_live_support_tu15_owns_entry(0x19baa0),"Whole original award writer lacks its actual native owner");
    source_run_physics_live_support_tu15(memory,0x19baa0,abi,{0x27f,0x1f80},{});
    check(abi.gp==row.at("gp").get<std::array<std::uint32_t,8>>() && (abi.eflags&0x8d5u)==row.at("eflags").get<unsigned>(),
        "Complete original award-writer registers, stack or defined flags differ");
    set(offset,row.at("after").get<unsigned>());
    check(std::ranges::equal(memory.view(0x51eef4,bank.size()),bank),"Original award writer changed its selected result or neighboring fields");
    ++award_cases;
  }
  check(award_cases==576,"Complete original award-writer coverage changed");
  unsigned rank_cases{};
  for(const auto& row:fixture.at("rank_cases")) {
    const auto entry=row.at("entry").get<std::uint32_t>();check(entry==0x19da10 || entry==0x19da60,"Unknown original retained ranking owner");
    const auto field=entry==0x19da10?0x51e3d4u:0x51ed75u,stride=entry==0x19da10?0xa4u:0x18u;
    const auto left=row.at("left").get<unsigned>(),right=row.at("right").get<unsigned>();
    check(left<16 && right<16,"Original retained ranking input escapes its property owner");
    std::vector<std::byte> input(0xf44);for(unsigned i=0;i<input.size();++i)input[i]=std::byte(i*17+7);
    for(unsigned n=0;n<2;++n) {
      const auto at=field+(n?right:left)*stride-0x51dfb0u,score_at=at+(entry==0x19da10?2u:1u);
      input[at]=std::byte(row.at("ranks")[n].get<int>());const auto score=row.at("scores")[n].get<int>();
      input[score_at]=std::byte(score);input[score_at+1]=std::byte(std::uint32_t(score)>>8);
    }
    SourceRuntimeMemory memory;memory.mount(0x51dfb0,input,SourceRuntimeMemory::Access::ReadWrite,"Explicit original retained properties");
    memory.mount(0x2000000,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"Explicit retained comparator caller stack");
    const std::array caller{stop,left,right,0xa5a5a5a5u};memory.write(stack,std::as_bytes(std::span(caller)));
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678,0x9abcdef0,0x13579bdf,0x2468ace0,stack,0x11223344,0x55667788,0x99aabbcc};abi.eflags=0x202;
    source_run_result_recording_support_tu15(memory,entry,abi,{0x27f,0x1f80},{});
    check(abi.gp==row.at("gp").get<std::array<std::uint32_t,8>>() && (abi.eflags&0x8d5u)==row.at("eflags").get<unsigned>(),
        "Complete retained comparator registers, stack or defined flags differ from original execution");
    check(std::ranges::equal(memory.view(0x51dfb0,input.size()),input),"Original retained comparator mutated its property inputs");++rank_cases;
  }
  check(rank_cases==96,"Original retained comparator coverage changed");
  std::cout<<cases<<" recipient and "<<award_cases<<" award-writer complete original comparisons; "
      <<rank_cases<<" retained ranking comparisons; "<<cold_bytes+award_bytes+combat_bytes<<" original cold metadata/context bytes\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
