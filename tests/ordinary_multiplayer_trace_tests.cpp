#include "ordinary_multiplayer_trace.hpp"
#include <cfenv>
#include <fstream>
#include <iostream>
#include <tuple>

using namespace halo2;
using namespace halo2::ordinary_trace;
namespace {
unsigned cases{};
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class Fn>void fails(Fn body){bool failed=false;try{body();}catch(const std::exception&){failed=true;}check(failed,"Expected explicit trace rejection");++cases;}
std::vector<std::byte> bytes(const std::filesystem::path &path){std::ifstream in(path,std::ios::binary|std::ios::ate);
  const auto n=in.tellg();check(n>=0 && n<=8*1024*1024,"Read bounded component trace");std::vector<std::byte> out(std::size_t(n),std::byte{});
  in.seekg(0);in.read(reinterpret_cast<char*>(out.data()),out.size());check(bool(in),"Read component trace");return out;}
std::uint64_t word(std::span<const std::byte> value,std::size_t at){check(at+8<=value.size(),"Truncated trace word");
  std::uint64_t out{};for(unsigned i=0;i<8;++i)out|=std::uint64_t(std::to_integer<unsigned>(value[at+i]))<<(8*i);return out;}
struct Entry{std::uint64_t kind{},sequence{};std::vector<std::byte> payload;};
std::vector<Entry> parse(std::span<const std::byte> raw,bool complete){
  check(raw.size()>=40 && std::memcmp(raw.data(),"H2OTR02\n",8)==0,"Trace magic/header differs");
  std::size_t cursor=40;std::vector<Entry> entries;
  while(cursor<raw.size()){
    const auto kind=word(raw,cursor),n=word(raw,cursor+8),seq=word(raw,cursor+16);cursor+=24;
    check(n<=raw.size()-cursor,"Truncated trace payload");
    if(kind==0xffffffffu){check(n==96 && seq==entries.size() && word(raw,cursor)==complete && cursor+n==raw.size(),"Missing/incomplete trace footer");return entries;}
    check(seq==entries.size(),"Trace sequence is not globally ordered");entries.push_back({kind,seq,{raw.begin()+cursor,raw.begin()+cursor+n}});cursor+=n;
  }
  throw std::runtime_error("Trace footer absent");
}
void mount(SourceRuntimeMemory &memory,bool reverse=false,unsigned mutation=0){
  const std::array<std::byte,8> a{std::byte(mutation),std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte{6},std::byte{7}};
  const std::array<std::byte,4> b{std::byte{9},std::byte{8},std::byte{7},std::byte{6}};
  if(reverse){memory.mount(0x3000,b,SourceRuntimeMemory::Access::ReadOnly,"second");memory.mount(0x1000,a,SourceRuntimeMemory::Access::ReadWrite,"first");}
  else{memory.mount(0x1000,a,SourceRuntimeMemory::Access::ReadWrite,"first");memory.mount(0x3000,b,SourceRuntimeMemory::Access::ReadOnly,"second");}
}
std::vector<std::byte> run(const std::filesystem::path &path,std::span<const Transform> nodes,bool reverse=false,unsigned mutation=0){
  SourceRuntimeMemory memory;mount(memory,reverse,mutation);Sink sink(path);MultiplayerOptions options;attach(options,sink);
  check(!options.animation_observer && !options.effect_observer && !options.unit_source_draw_observer &&
      !options.shield_layer_observer && !options.bsp_submission_observer,"Trace selected a CPU-only vertex observer");
  options.source_world_observer(memory);MultiplayerState state;state.tick=1;
  state.source_body_poses={{17,{nodes.begin(),nodes.end()},{{1,2,3},4}}};state.source_body_poses_authoritative=true;
  SourceMultiplayerEventTu15 event;event.serial=1;event.tick=1;event.words[3]=0x80000000u;state.source_engine_events={event};
  std::array<Controls,1> controls{};controls[0].yaw=std::bit_cast<float>(0x80000000u);options.input_observer(0,controls,state);
  for(unsigned stage=1;stage<=4;++stage)options.node_observer(0,1,
      {MultiplayerNodeStage(stage),17,23,stage>2?1u:0u,.5f,nodes,91,3,true});
  options.view_rectangle_observer(0,{0,0,240,320});Camera camera;camera.yaw=std::bit_cast<float>(0x80000000u);
  options.view_state_observer(0,state,camera);sink.finish();return bytes(path);
}
auto routing(const MultiplayerOptions &o){return std::array{bool(o.animation_observer),bool(o.effect_observer),
    bool(o.unit_source_draw_observer),bool(o.shield_layer_observer),bool(o.bsp_submission_observer)};}
}
int main(){try{
  const auto root=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"));
  Transform node;node.scale=std::bit_cast<float>(0x7fc12345u);node.position.x=std::bit_cast<float>(0x80000000u);
  const std::array<Transform,1> nodes{node};const auto raw=run(root/"baseline.bin",nodes),same=run(root/"reversed-mount.bin",nodes,true);
  check(raw==same,"Source region insertion order changed address-ordered raw-byte trace");++cases;
  const auto entries=parse(raw,true);check(entries.size()==8,"Actual callback record count differs");
  const std::array<unsigned,8> order{1,2,5,5,5,5,4,3};for(unsigned i=0;i<order.size();++i)check(entries[i].kind==order[i],"Callback order differs");++cases;
  check(word(entries.back().payload,16+3*8)==0x80000000u,"Camera signed zero was formatted or normalized");++cases;
  check(word(entries[2].payload,7*8)==1 && word(entries[2].payload,8*8)==1 &&
      word(entries[2].payload,9*8)==91 && word(entries[2].payload,10*8)==3,"Node count/actual attachment metadata absent");++cases;
  for(unsigned index=0;index<13;++index){auto changed=nodes;auto &n=changed[0];
    std::array<float*,13> fields{&n.scale,&n.basis[0].x,&n.basis[0].y,&n.basis[0].z,&n.basis[1].x,&n.basis[1].y,&n.basis[1].z,
      &n.basis[2].x,&n.basis[2].y,&n.basis[2].z,&n.position.x,&n.position.y,&n.position.z};
    *fields[index]=std::bit_cast<float>(std::bit_cast<std::uint32_t>(*fields[index])^1u);
    auto value=parse(run(root/("node-"+std::to_string(index)+".bin"),changed),true);
    check(value[2].payload!=entries[2].payload,"A node scalar bit was omitted from its complete digest");++cases;
  }
  check(run(root/"changed-source.bin",nodes,false,1)!=raw,"A raw committed source byte was omitted");++cases;
  auto truncated=raw;truncated.pop_back();fails([&]{parse(truncated,true);});
  for(unsigned policy=0;policy<32;++policy){Sink sink(root/("route-"+std::to_string(policy)+".bin"));MultiplayerOptions options;
    if(policy&1)options.animation_observer=[](auto,auto,const auto&){};
    if(policy&2)options.effect_observer=[](auto,auto,auto){};
    if(policy&4)options.unit_source_draw_observer=[](auto,auto,auto,auto,auto,auto,auto){};
    if(policy&8)options.shield_layer_observer=[](auto,auto,auto,auto,auto,auto){};
    if(policy&16)options.bsp_submission_observer=[](auto,auto,const auto&,auto){};
    const auto before=routing(options);attach(options,sink);check(routing(options)==before && options.node_observer,
        "Passive trace attachment changed an actual GPU-disqualifying observer");sink.finish();++cases;
  }
  auto cap=[&](const char *name,Limits limit,auto body){const auto path=root/name;
    {Sink sink(path,limit);fails([&]{body(sink);});check(sink.failed(),"Trace cap/order did not mark incomplete");}
    parse(bytes(path),false);++cases;};
  Limits byte_limit;byte_limit.bytes=256;cap("byte-cap.bin",byte_limit,[&](Sink &s){SourceRuntimeMemory m;mount(m);s.source(m);MultiplayerState state;std::array<Controls,1> c{};s.input(0,c,state);});
  Limits node_limit;node_limit.nodes=1;cap("node-cap.bin",node_limit,[&](Sink &s){std::array<Transform,2> n{};s.nodes(0,1,{MultiplayerNodeStage::world_completed,1,2,0,0,n});});
  Limits bank_limit;bank_limit.bank_bytes=4;cap("bank-cap.bin",bank_limit,[&](Sink &s){SourceRuntimeMemory m;mount(m);s.source(m);});
  cap("ordinal-gap.bin",{},[&](Sink &s){SourceRuntimeMemory m;mount(m);s.source(m);MultiplayerState state;std::array<Controls,1> c{};s.input(1,c,state);});
  {
    SourceRuntimeMemory memory;mount(memory);Sink sink(root/"checkpoints.bin");sink.source(memory);
    sink.checkpoint(BankCheckpoint::preparation,0,11);
    const std::array<std::byte,1> changed{std::byte{0x80}};memory.write(0x1000,changed);
    sink.checkpoint(BankCheckpoint::preparation,1,12);sink.checkpoint(BankCheckpoint::final,0,13);
    const auto counts=sink.counts();check(counts.preparations==2 && counts.finals==1,"Actual checkpoint counts differ");++cases;
    sink.finish();const auto records=parse(bytes(root/"checkpoints.bin"),true);
    check(records.size()==4 && records[1].kind==6 && word(records[1].payload,0)==1 &&
        word(records[1].payload,8)==0 && word(records[1].payload,16)==11 && word(records[3].payload,0)==2 &&
        word(records[3].payload,16)==13,"Actual checkpoint stage/ordinal/tick or global order differs");++cases;
    check(!std::equal(records[1].payload.begin()+24,records[1].payload.end(),records[2].payload.begin()+24) &&
        std::equal(records[2].payload.begin()+24,records[2].payload.end(),records[3].payload.begin()+24),
        "Checkpoint omitted actual source bytes or sampled another owner");++cases;
  }
  const auto checkpoint_source=[&](Sink &sink){static SourceRuntimeMemory memory;static bool mounted{};
    if(!mounted){mount(memory);mounted=true;}sink.source(memory);};
  cap("checkpoint-gap.bin",{},[&](Sink &s){checkpoint_source(s);s.checkpoint(BankCheckpoint::preparation,1,1);});
  cap("checkpoint-kind.bin",{},[&](Sink &s){checkpoint_source(s);s.checkpoint(BankCheckpoint(99),0,1);});
  cap("checkpoint-final-ordinal.bin",{},[&](Sink &s){checkpoint_source(s);s.checkpoint(BankCheckpoint::final,1,1);});
  cap("checkpoint-repeat-final.bin",{},[&](Sink &s){checkpoint_source(s);s.checkpoint(BankCheckpoint::final,0,1);s.checkpoint(BankCheckpoint::final,0,1);});
  cap("checkpoint-after-final.bin",{},[&](Sink &s){checkpoint_source(s);s.checkpoint(BankCheckpoint::final,0,1);s.checkpoint(BankCheckpoint::preparation,0,2);});
  Limits prep_limit;prep_limit.ticks=1;cap("checkpoint-count-cap.bin",prep_limit,[&](Sink &s){checkpoint_source(s);s.checkpoint(BankCheckpoint::preparation,0,1);s.checkpoint(BankCheckpoint::preparation,1,2);});
  cap("checkpoint-byte-cap.bin",byte_limit,[&](Sink &s){checkpoint_source(s);s.checkpoint(BankCheckpoint::preparation,0,1);});
  Limits total_limit;total_limit.total_bank_bytes=12;cap("checkpoint-bank-cap.bin",total_limit,[&](Sink &s){checkpoint_source(s);s.checkpoint(BankCheckpoint::preparation,0,1);});
  const auto collision=root/"collision.bin";{std::ofstream file(collision);file<<"reference";}fails([&]{Sink sink(collision);});check(bytes(collision).size()==9,"Collision modified input");
  const auto link=root/"link.bin";std::filesystem::create_symlink(collision,link);fails([&]{Sink sink(link);});
  Sink fp(root/"fp.bin");
  for(int round:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO})for(unsigned controls:{0u,0x40u,0x8000u,0x8040u}){
    std::fesetenv(FE_DFL_ENV);std::fesetround(round);_mm_setcsr(_mm_getcsr()|controls);errno=EDOM;
    asm volatile("fld1\n\tfldz":::"st","st(1)");alignas(16)std::array<std::byte,512> before{},after{};
    asm volatile("fxsave64 %0":"=m"(before)::"memory");
    fp.nodes(0,1,{MultiplayerNodeStage::world_completed,1,2,0,.5f,nodes});
    asm volatile("fxsave64 %0":"=m"(after)::"memory");const auto error=errno;
    asm volatile("fstp %%st(0)\n\tfstp %%st(0)":::"st","st(1)");
    check(std::equal(before.begin(),before.begin()+160,after.begin()) && error==EDOM,"Trace altered full x87 environment/live values, MXCSR or errno");++cases;
  }
  std::fesetenv(FE_DFL_ENV);fp.finish();
  std::cout<<"PASS ordinary passive node/source trace "<<cases<<" controls; no engine/Vulkan launched\n";return 0;
}catch(const std::exception &error){std::cerr<<"Ordinary trace component: "<<error.what()<<'\n';return 1;}}
