#include "halo2/sound_gp_cpu_tu15.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using Cpu=halo2::SoundGpCpuTu15;using Input=halo2::SoundGpCpuInputTu15;using Space=halo2::SoundGpSpaceTu15;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F action,const char *where){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}if(!rejected)throw std::runtime_error(std::string("GP owner accepted ")+where);}
Input input(){Input value{};value.read_peripheral=[](auto)->std::uint32_t{throw std::runtime_error("Absent test peripheral read");};value.write_peripheral=[](auto,auto){throw std::runtime_error("Absent test peripheral write");};return value;}
int original_blocks(const std::filesystem::path &path) {
  if(!std::filesystem::exists(path))return 77;
  std::ifstream file(path,std::ios::binary|std::ios::ate);check(bool(file)&&file.tellg()==15684,"Original GP fixture extent differs");file.seekg(0);
  std::vector<std::uint32_t> program(3921);
  for(auto &word:program)for(unsigned byte=0;byte<4;++byte){const auto c=file.get();check(c!=EOF,"Incomplete original GP fixture");word|=std::uint32_t(c)<<(byte*8);}
  const auto load=[&](Cpu &cpu){cpu.bootstrap(std::span(program).first(0x800));for(unsigned i=0x800;i<program.size();++i)cpu.write(Space::program,i,program[i]);};
  const auto context=[] {auto value=input();for(unsigned i=32;i<40;++i)value.registers[i]=0xffff;return value;};
  Cpu *owner=nullptr;std::vector<std::pair<std::uint32_t,std::uint32_t>> writes;auto callbacks=context();
  callbacks.write_peripheral=[&](auto address,auto value){writes.emplace_back(address,value);owner->request_stop();};
  Cpu cpu(callbacks);owner=&cpu;load(cpu);
  rejects([&]{cpu.request_stop();},"stop outside execution");
  rejects([&]{cpu.run_original_gp(0);},"zero block slice");
  const auto first=cpu.run_original_gp(1);
  check(!first.stopped&&cpu.pc()==0x155&&first.cycles>0,"GP slice did not preserve original JSR boundary");
  const auto call_state=cpu.execution_state();
  check(program[0]==0x0bf080&&program[1]==0x155&&call_state.registers[59]==1&&
        call_state.registers[60]==2&&call_state.stack[0][1]==2,
        "GP original two-word JSR did not retain its actual return stack");
  const auto second=cpu.run_original_gp(100000);
  check(second.stopped&&cpu.pc()==0x157,"GP block did not yield exactly after original peripheral write");
  check(writes==std::vector{std::pair<std::uint32_t,std::uint32_t>{0xffffc5,0xfff}},"Original GP write was lost or overrun");
  check(cpu.cycles()==first.cycles+second.cycles,"GP scheduling slice changed actual processor cycles");
  {
    Cpu *step_owner=nullptr;auto step_context=context();std::size_t step_writes{};
    step_context.write_peripheral=[&](auto,auto){++step_writes;step_owner->request_stop();};
    Cpu stepping(step_context);step_owner=&stepping;load(stepping);stepping.step();stepping.step();
    check(step_writes==1&&stepping.execution_state()==cpu.execution_state(),"Original GP stop register/cycle/stack/repeat state differs from single-step");
  }
  const auto pc=cpu.pc(),cycles=cpu.cycles();cpu.write(Space::program,0x900,program[0x900]^1u);
  rejects([&]{cpu.run_original_gp(100000);},"modified GP block program");
  check(cpu.pc()==pc&&cpu.cycles()==cycles&&writes.size()==1,"Rejected GP program executed or changed device state");
  cpu.write(Space::program,0x900,program[0x900]);
  const auto resumed=cpu.run_original_gp(100000);
  check(resumed.stopped&&cpu.pc()==0x10c&&writes.size()==2&&writes.back()==std::pair<std::uint32_t,std::uint32_t>{0xffffd6,2},"GP block resume did not reach original next peripheral boundary");
  {
    Cpu *upload_owner=nullptr;auto upload_context=context();unsigned upload_writes{};
    upload_context.write_peripheral=[&](auto,auto){
      if(++upload_writes==1)upload_owner->write(Space::program,0x900,program[0x900]^1u);
      else upload_owner->request_stop();
    };
    Cpu uploading(upload_context);upload_owner=&uploading;load(uploading);
    const auto changed=uploading.run_original_gp(100000);
    check(changed.stopped&&upload_writes==1&&uploading.pc()==0x157,"GP callback program upload did not yield before continuation");
    rejects([&]{uploading.run_original_gp(100000);},"callback-modified GP program");
    uploading.write(Space::program,0x900,program[0x900]);
    const auto reloaded=uploading.run_original_gp(100000);
    check(reloaded.stopped&&upload_writes==2&&uploading.pc()==0x10c,"GP callback program reload lost its exact continuation");
  }
  rejects([&]{cpu.run_original_gp(100000);},"failed block peripheral read");
  rejects([&]{cpu.pc();},"state after block callback failure");
  check(writes.size()==2,"Failed GP read allowed a later external device write");
  std::cout<<"PASS original GP block checksum, slice/resume, exact peripheral exit, register/cycle equality and persistent callback fault\n";
  return 0;
}
}
int main(int argc,char **argv)try {
  if(argc==3&&std::string(argv[1])=="--original-gp")return original_blocks(argv[2]);
  if(argc!=1)throw std::runtime_error("Supply no arguments or --original-gp PROGRAM");
  rejects([]{Cpu cpu(Input{});},"absent peripheral owners");
  auto invalid=input();invalid.registers[4]=0x1000000;rejects([&]{Cpu cpu(invalid);},"wide initial register");
  for(auto bit:{17,13,14}){auto modes=input();modes.registers[57]=1u<<bit;rejects([&]{Cpu cpu(modes);},"unsupported SR mode");}
  for(auto bit:{20,7}){auto modes=input();modes.registers[58]=1u<<bit;rejects([&]{Cpu cpu(modes);},"unsupported OMR mode");}
  std::array<std::uint32_t,0x800> program{};
  // Actual original GP P70/P71 MOVEC operand. DSP56300FM Rev5 sections
  // 4.2/4.3.4 specify 24-bit modifiers when SR.SC is clear. The pinned
  // reference interpreter truncates this write to 16 bits; that mismatch
  // remains reported rather than copied into the native execution owner.
  program[0]=0x05f420;program[1]=0xffffff;
  {
    auto initial=input();initial.registers[32]=0xffff;initial.registers[57]=0x15;
    Cpu cpu(initial);cpu.bootstrap(program);cpu.step();
    check(cpu.registers()[32]==0xffffff,"GP original MOVEC lost its 24-bit modifier operand");
  }
  // Original P1C2: ASL #4,A,A. FF:FE6418:000000 remains negative without
  // bit55 ever changing. DSP56300FM Rev5 sections5.4.1.2/13 ASL: carry is
  // set, V stays clear, L retains its previous value. The reference helper
  // instead sets L for the shifted-out sign extension; do not mask either.
  program={};program[0]=0x0c1d08;
  for(auto limit:{0u,0x40u}){
    auto initial=input();initial.registers[10]=0xff;initial.registers[12]=0xfe6418;initial.registers[57]=0x8018|limit;
    Cpu cpu(initial);cpu.bootstrap(program);cpu.step();const auto state=cpu.execution_state();
    check(state.registers[10]==0xff&&state.registers[12]==0xe64180&&state.registers[8]==0&&
          state.registers[57]==(0x8019|limit),"GP original ASL sign extension changed the sticky limit flag");
  }
  program={};
  {
    auto initial=input();initial.registers[4]=0x654321;initial.registers[32]=0xffffff;
    Cpu cpu(initial);check(cpu.pc()==0&&cpu.registers()==initial.registers&&cpu.cycles()==0,"GP cold state readback differs from actual core input");
    cpu.bootstrap(program);check(cpu.registers()==initial.registers,"GP bootstrap changed scalar readback without execution");
    rejects([&]{cpu.run_original_gp(100);},"unrecognized GP program");
  }
  {
    Cpu cpu(input());rejects([&]{cpu.step();},"execution before bootstrap");
    rejects([&]{cpu.bootstrap(std::span(program).first(1));},"short bootstrap");
    program[0]=0x1000000;rejects([&]{cpu.bootstrap(program);},"wide bootstrap word");program[0]=0;
    cpu.write(Space::x,0xc00,0x123456);check(cpu.read(Space::x,0x1400)==0x123456,"GP X/mix alias lost first word");
    cpu.write(Space::x,0x17ff,0xabcdef);check(cpu.read(Space::x,0xfff)==0xabcdef,"GP X/mix alias lost final word");
    rejects([&]{cpu.read(Space::x,0x1000);},"unmapped RAM read");
    rejects([&]{cpu.write(Space::y,0x800,1);},"out-of-range Y write");
    rejects([&]{cpu.write(Space::program,0x1000,1);},"out-of-range P write");
    cpu.bootstrap(program);rejects([&]{cpu.bootstrap(program);},"replacement bootstrap");
  }
  // Exercise the instruction core's memory map too: host API aliases alone
  // cannot detect an incorrect C-bridge base pointer for the second window.
  program={};program[0]=0x44f000;program[1]=0x1400;program[2]=0x447000;program[3]=0x17ff;
  {
    Cpu cpu(input());cpu.bootstrap(program);cpu.write(Space::x,0xc00,0x123456);cpu.step();
    check(cpu.registers()[4]==0x123456,"GP instruction read does not use the actual mix alias");
    cpu.step();check(cpu.read(Space::x,0xfff)==0x123456,"GP instruction write does not use the actual mix alias");
  }
  // Original GP MOVE-immediate encoding, followed by the ISA's absolute JMP.
  // A compiled old operand must not survive an external program update.
  program={};program[0]=0x44f400;program[1]=1;program[2]=0x0af080;program[3]=0;
  {
    Cpu cpu(input());cpu.bootstrap(program);cpu.step();check(cpu.registers()[4]==1,"GP immediate result differs");
    cpu.step();check(cpu.pc()==0,"GP loop target differs");cpu.write(Space::program,1,2);cpu.step();
    check(cpu.registers()[4]==2,"GP external PRAM update used stale translated code");
  }
  // A DMA owner can update RAM/P during the original peripheral instruction.
  // The update is invalidated after that instruction returns from the core.
  program={};program[0]=0x08f485;program[1]=0xfff;program[2]=0x44f400;program[3]=1;program[4]=0x0af080;program[5]=0;
  {
    Cpu *owner=nullptr;unsigned writes{};auto callbacks=input();
    callbacks.write_peripheral=[&](auto address,auto value){check(address==0xffffc5&&value==0xfff,"Original GP peripheral store differs");
      owner->write(Space::x,0x80,++writes);if(writes==2)owner->write(Space::program,3,2);};
    Cpu cpu(callbacks);owner=&cpu;cpu.bootstrap(program);cpu.step();cpu.step();check(cpu.registers()[4]==1,"GP initial callback program differs");
    cpu.step();cpu.step();cpu.step();check(cpu.registers()[4]==2&&cpu.read(Space::x,0x80)==2,"GP callback update used stale RAM/program");
  }
  // The original control's read encoding reaches the required owner. A
  // failing callback poisons this CPU; no later register/RAM result is usable.
  program={};program[0]=0x44f000;program[1]=0xffffb3;
  {
    Cpu cpu(input());cpu.bootstrap(program);rejects([&]{cpu.step();},"failed peripheral read");
    rejects([&]{cpu.pc();},"PC after peripheral failure");rejects([&]{cpu.write(Space::x,0,1);},"RAM write after peripheral failure");
  }
  program[1]=0x1000;
  {Cpu cpu(input());cpu.bootstrap(program);rejects([&]{cpu.step();},"instruction read in unmapped X gap");}
  std::cout<<"PASS native GP ownership, physical aliases, bootstrap/mode rejection, external and callback PRAM invalidation, and persistent callback faults\n";
  std::cout<<"LIMIT instruction owner contract; original GP/DMA comparison and actual native device/playback remain separate gates\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
