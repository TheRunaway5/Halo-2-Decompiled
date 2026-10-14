#include "halo2/physics_game_arena_native.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/source_runtime_image.hpp"
#include <nlohmann/json.hpp>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <future>
#include <thread>
#include <cstdlib>
#include <unistd.h>

using namespace halo2;
namespace {
using J=nlohmann::json;
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f) {
  bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}
  check(rejected,"Unsafe or invalid arena continuation was accepted");
}
void put(SourcePhysicsContactMemory &bank,std::uint32_t address,std::uint32_t value) {
  std::array<std::byte,4> bytes;
  for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
  bank.write(address,bytes);
}
void put(SourceRuntimeMemory::Checkpoint &bank,std::uint32_t address,std::uint32_t value) {
  for(auto &region:bank.regions)if(address>=region.address &&
      std::uint64_t(address)+4<=std::uint64_t(region.address)+region.bytes.size()) {
    for(unsigned i=0;i<4;++i)region.bytes[address-region.address+i]=std::byte(value>>(8*i));
    return;
  }
  throw std::runtime_error("Test checkpoint source word missing");
}
SourcePhysicsWorldRuntimeAbi abi(const J &j) {
  SourcePhysicsWorldRuntimeAbi state{};state.gp=j.at("gp").get<decltype(state.gp)>();
  state.eflags=j.at("eflags");state.x87_status=j.at("x87_status");
  state.floating_point=SourcePhysicsFactoryFloatingPoint{j.at("x87_control").get<std::uint16_t>(),j.at("mxcsr")};
  for(unsigned i=0;i<8;++i) {
    const auto hex=j.at("xmm")[i].get<std::string>();check(hex.size()==32,"Fixture XMM extent differs");
    for(unsigned lane=0;lane<4;++lane)state.xmm[i][lane]=std::stoul(hex.substr(24-8*lane,8),nullptr,16);
  }
  return state;
}
struct TemporaryDirectory {
  std::filesystem::path path;
  TemporaryDirectory() {
    auto name=(std::filesystem::temp_directory_path()/"halo2-arena-continuation-XXXXXX").string();
    check(::mkdtemp(name.data()),"Cannot create arena test directory");path=name;
  }
  ~TemporaryDirectory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
struct Schedule {
  std::mutex mutex;
  std::condition_variable_any changed;
  unsigned idle{},epoch{};
  SourcePhysicsGameArenaSchedule services() {
    return {
      [this](std::uint32_t handle,std::stop_token stop) {
        if(handle!=0xf3000003u)return;
        std::unique_lock lock(mutex);++idle;changed.notify_all();const auto seen=epoch;
        if(!changed.wait(lock,stop,[&]{return epoch>seen;}))throw NativeWorkerStopped();
      },
      [this] {
        std::unique_lock lock(mutex);
        check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>0;}),"Worker missed initial idle frontier");
        const auto next=idle+1;++epoch;changed.notify_all();
        check(changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>=next;}),"Source file job did not retire");
      }};
  }
};
void startup(SourcePhysicsGameArenaNative &arena,const J &fixture) {
  for(const auto &step:fixture.at("steps")) {
    auto state=abi(step.at("entry_abi"));put(arena.memory(),state.gp[4],0x60200000u);
    unsigned index=0;for(const auto &value:step.at("arguments"))put(arena.memory(),state.gp[4]+4+4*index++,value);
    arena.run(std::stoul(step.at("entry").get<std::string>(),nullptr,16),state);
    check(state.gp==abi(step.at("exit_abi")).gp,"Original arena startup GP return differs");
  }
}
std::size_t descriptors() {
  std::size_t count=0;for(const auto &entry:std::filesystem::directory_iterator("/proc/self/fd")){(void)entry;++count;}return count;
}
}
int main(int argc,char **argv) try {
  check(argc==3,"Supply the original arena fixture and TU1.5 executable");
  if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
  std::ifstream input(argv[1]);const auto fixture=J::parse(input);
  auto source=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
  auto target=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
  for(auto *bank:{&source,&target})
    bank->mount(0x60000000u,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"native arena continuation caller stack");
  TemporaryDirectory source_files,target_files;Schedule source_schedule,target_schedule;
  SourcePhysicsGameArenaNative *callback_owner=nullptr;
  bool rejected_reentry=false,protected_reentry=false;
  const auto caller_thread=std::this_thread::get_id();
  const auto observe=[&](const SourcePhysicsContactRecordStore &) {
    if(!callback_owner || std::this_thread::get_id()!=caller_thread)return;
    if(!rejected_reentry) {
      rejects([&]{callback_owner->ownership_checkpoint();});
      rejects([&]{callback_owner->stop();});rejected_reentry=true;
    }
    if(!protected_reentry) {
      std::uint32_t pointer=0;const auto bytes=source.view(0x4f2950u,4);
      for(unsigned i=0;i<4;++i)pointer|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
      // The original constructor publishes this arena after allocating its
      // actual physical pages. Reentrant local callbacks may protect them.
      if(pointer)try {
        callback_owner->protect_pages(pointer,1,4);
        auto other=std::async(std::launch::async,[&]{callback_owner->protect_pages(pointer,1,4);});
        check(other.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Arena call gate blocked a worker resource callback");
        other.get();protected_reentry=true;
      } catch(const std::runtime_error &){}
    }
  };
  auto original=std::make_unique<SourcePhysicsGameArenaNative>(source,
      SourcePhysicsGameArenaConfiguration{source_files.path,0x60100000u,{0x27f,0x1f80},source_schedule.services()},
      SourcePhysicsWorldRuntimeServices{},observe);
  callback_owner=original.get();
  SourcePhysicsGameArenaNative resumed(target,
      {target_files.path,0x60100000u,{0x27f,0x1f80},target_schedule.services()});
  rejects([&]{original->ownership_checkpoint();});
  startup(*original,fixture);startup(resumed,fixture);
  check(rejected_reentry && protected_reentry,"Arena call gate lost callback reentry or admitted a mid-call checkpoint/stop");
  rejects([&]{original->ownership_checkpoint();}); // Idle but still running is not an exported worker.
  original->stop();resumed.stop();
  const auto initial=original->ownership_checkpoint();check(initial.files.size()==2 && !initial.physical.allocations.empty(),"Original arena resources missing");
  const auto &allocation=initial.physical.allocations.back();
  const auto retained=allocation.base+allocation.bytes-1;
  source.write(retained,std::array{std::byte{0x9d}});
  original->protect_pages(retained,1,2);
  {
    std::fstream file(source_files.path/"savegame0.bin",std::ios::binary|std::ios::in|std::ios::out);
    check(bool(file),"Cannot write retained source file marker");file.seekp(37);file.put(char(0xa7));file.flush();
    check(bool(file),"Source file marker write failed");
  }
  const auto saved=original->ownership_checkpoint(),old=resumed.ownership_checkpoint();
  const auto bank=source.checkpoint(),old_bank=target.checkpoint();
  SourceRuntimeMemory replacement;replacement.restore(bank);
  {
    SourceRuntimeMemory empty;
    SourcePhysicsGameArenaNative cold(empty,{target_files.path,0x60100000u,{0x27f,0x1f80},{}});
    cold.stop();
    rejects([&]{cold.prepare_ownership(saved,bank,replacement);}); // Imported bytes cannot fabricate a local startup thread.
    check(empty.checkpoint().regions.empty(),"Rejected cold-owner import changed its bank");
  }
  const auto old_native=target.native_address(retained,1);
  const auto fd_count=descriptors();const auto wait_count=target_schedule.idle;
  {
    auto stage=resumed.prepare_ownership(saved,bank,replacement);
    check(stage.for_owner(resumed) && descriptors()==fd_count+2,"Arena files were not staged as real independent descriptors");
    rejects([&]{resumed.ownership_checkpoint();});
    rejects([&]{resumed.prepare_ownership(saved,bank,replacement);});
    rejects([&]{resumed.protect_pages(retained,1,4);});
    rejects([&]{resumed.stop();});
    check(!original->commit_ownership(std::move(stage)),"Foreign arena accepted staged resources");
    auto moved=std::move(stage);check(moved.for_owner(resumed) && !stage.for_owner(resumed),"Arena staging move lost owner");
    check(target.checkpoint()==old_bank,"Arena staging changed the byte bank");
  }
  check(descriptors()==fd_count && resumed.ownership_checkpoint()==old,"Cancelled arena staging leaked files or changed resources");
  auto bad=saved;bad.version=2;rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  bad=saved;++bad.worker;rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  bad=saved;++bad.scratch.next;rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  bad=saved;bad.files[0].name="../savegame0.bin";rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  bad=saved;bad.files[1].alias=bad.files[0].alias;rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  bad=saved;bad.files.pop_back();--bad.next_file;rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  bad=saved;bad.synchronization.objects.back().count=1;rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  bad=saved;bad.synchronization.version=2;rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  bad=saved;bad.physical.allocations.back().read_only.back()=2;rejects([&]{resumed.prepare_ownership(bad,bank,replacement);});
  auto wrong_bank=bank;put(wrong_bank,0x4ef1bcu,1);rejects([&]{resumed.prepare_ownership(saved,wrong_bank,replacement);});
  wrong_bank=bank;put(wrong_bank,0x4ecc30u,0);rejects([&]{resumed.prepare_ownership(saved,wrong_bank,replacement);});
  put(source,0x4ef1bcu,1);rejects([&]{original->ownership_checkpoint();});put(source,0x4ef1bcu,0);
  // Fail real file staging after page/synchronization resources were already
  // prepared. Existing open files and the current bank must survive unwind.
  const auto moved_directory=target_files.path.string()+"-temporarily-moved";
  std::filesystem::rename(target_files.path,moved_directory);
  try {rejects([&]{resumed.prepare_ownership(saved,bank,replacement);});}
  catch(...) {std::filesystem::rename(moved_directory,target_files.path);throw;}
  std::filesystem::rename(moved_directory,target_files.path);
  check(descriptors()==fd_count && target.checkpoint()==old_bank && resumed.ownership_checkpoint()==old,
        "Rejected arena import leaked resources or changed current ownership");
  replacement.restore(bank);
  auto stage=resumed.prepare_ownership(saved,bank,replacement);
  check(stage.matches_memory(replacement),"Staged arena pages differ from replacement bank");
  check(!resumed.commit_ownership(std::move(stage)),"Arena committed resources before replacement publication");
  target.swap_backing(replacement);check(resumed.commit_ownership(std::move(stage)),"Arena ownership publication failed");
  check(descriptors()==fd_count && resumed.ownership_checkpoint()==saved && target.checkpoint()==source.checkpoint(),
        "Imported arena files, protections, handles or bytes differ");
  check(target_schedule.idle==wait_count,"Import restarted or replaced the local source worker/schedule");
  rejects([&]{target.view(old_native,1);});
  rejects([&]{target.write(retained,std::array{std::byte{1}});});
  {
    std::ifstream file(target_files.path/"savegame0.bin",std::ios::binary);file.seekg(37);
    check(file.get()==0,"Staging overwrote the replacement PC's savegame file");
  }
  // The actual retired native worker remains retired after resource rebinding.
  resumed.stop();auto state=abi(fixture.at("steps")[0].at("entry_abi"));
  rejects([&]{resumed.run(0x1221b0u,state);});
  for(int repeat=0;repeat<3;++repeat) {
    const auto snapshot=source.checkpoint();SourceRuntimeMemory next;next.restore(snapshot);
    auto prepared=resumed.prepare_ownership(original->ownership_checkpoint(),snapshot,next);
    target.swap_backing(next);check(resumed.commit_ownership(std::move(prepared)),"Repeated arena handoff failed");
    check(descriptors()==fd_count,"Repeated arena handoff leaked a descriptor");
  }
  for(auto *arena:{original.get(),&resumed})arena->protect_pages(retained,1,4);
  for(auto *owner:{&source,&target})owner->write(retained,std::array{std::byte{0x53}});
  check(target.checkpoint()==source.checkpoint() && resumed.ownership_checkpoint()==original->ownership_checkpoint(),
        "Future arena protection/resource use diverged after import");
  const auto independent=resumed.ownership_checkpoint();original.reset();
  std::filesystem::remove_all(source_files.path);
  check(resumed.ownership_checkpoint()==independent && descriptors()==fd_count-2,"Imported arena depended on source descriptors/files");
  std::cout<<"PASS original arena startup, joined idle worker guard, private file/position and physical/sync restoration, atomic rejection/cancellation, repeated handoff and independent native resource lifetime\n";
  return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
