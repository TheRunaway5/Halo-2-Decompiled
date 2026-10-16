#include <zlib.h>
#include "halo2/source_ai_frame_tu15.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace halo2;
namespace {
constexpr std::uint32_t entry=0x1c9c90u,global=0x5020c8u,pool=0x2100100u;
constexpr std::uint32_t records=0x2101234u,stack_base=0x2200000u,stack=0x2200080u;
void require(bool value,const char *message) {
  if(!value)throw std::runtime_error(message);
}
void guard() {
  std::ifstream memberships("/proc/self/cgroup");std::string group,line;
  while(std::getline(memberships,line))if(line.starts_with("0::"))group=line.substr(3);
  require(!group.empty() && group[0]=='/',"Cannot establish runtime cgroup identity");
  const auto directory=std::string("/sys/fs/cgroup")+group;
  std::ifstream memory(directory+"/memory.max"),swap(directory+"/memory.swap.max");
  std::string limit,swap_limit;
  require(bool(memory>>limit) && bool(swap>>swap_limit),"Runtime cgroup limits unavailable");
  require(limit!="max" && swap_limit=="0","Parent must establish bounded zero-swap cgroup");
  const auto count=std::stoull(limit);
  require(count>0 && count<=64u*1024u*1024u,"Parent must establish aggregate<=64MiB cgroup");
}
// Only the input transport differs from the validated component fixture.
// Cases are streamed, and all expected outputs come from the original run.
class GzipFixture {
public:
  explicit GzipFixture(const char *path) : file_(gzopen(path, "rb")) {
    require(file_ != nullptr, "Original gzip fixture unavailable");
  }
  ~GzipFixture() { gzclose(file_); }
  GzipFixture(const GzipFixture &) = delete;
  GzipFixture &operator=(const GzipFixture &) = delete;

  bool next_line(std::string &line) {
    line.clear();
    for (;;) {
      if (offset_ == available_) {
        const int count = gzread(file_, buffer_.data(), buffer_.size());
        require(count >= 0, "Original gzip fixture is corrupt or unreadable");
        if (count == 0) {
          int error = Z_OK;
          gzerror(file_, &error);
          require(gzeof(file_) && (error == Z_OK || error == Z_STREAM_END),
              "Original gzip fixture ended with a decompression error");
          require(line.empty(), "Unterminated original fixture row");
          return false;
        }
        total_ += static_cast<std::size_t>(count);
        require(total_ <= 4u * 1024u * 1024u,
            "Fixture exceeded bounded export budget");
        offset_ = 0;
        available_ = static_cast<std::size_t>(count);
      }
      const char value = buffer_[offset_++];
      require(value != '\0', "Embedded NUL in original fixture row");
      if (value == '\n') return true;
      require(line.size() < 16384, "Original fixture row exceeded its bound");
      line.push_back(value);
    }
  }

private:
  gzFile file_{};
  std::array<char, 4096> buffer_{};
  std::size_t offset_{}, available_{}, total_{};
};
std::uint32_t number(std::istream &input) {
  std::string token;require(bool(input>>token) && token.size()<=8,"Missing or oversized fixture word");
  std::size_t used{};const auto value=std::stoul(token,&used,16);
  require(used==token.size() && value<=0xfffffffful,"Invalid fixture word");
  return std::uint32_t(value);
}
std::vector<std::byte> bytes(const std::string &text) {
  require(text.size()%2==0 && text.size()<=8192,"Invalid bounded fixture region");
  std::vector<std::byte> result;result.reserve(text.size()/2);
  for(std::size_t index=0;index<text.size();index+=2) {
    std::istringstream input(text.substr(index,2));result.push_back(std::byte(number(input)));
  }
  return result;
}
struct Region {std::uint32_t address;std::vector<std::byte> initial,data;};
using Read=std::pair<std::uint32_t,std::uint32_t>;
struct Memory : SourcePhysicsContactMemory {
  std::vector<Region> regions;
  mutable std::vector<Read> reads;
  const Region &find(std::uint32_t address,std::size_t count)const {
    for(const auto &region:regions)
      if(address>=region.address && std::uint64_t(address-region.address)+count<=region.data.size())
        return region;
    throw std::runtime_error("Native leaf accessed undeclared component storage");
  }
  void read(std::uint32_t address,std::span<std::byte> output)const override {
    const auto &region=find(address,output.size());
    require(reads.size()<16,"Native leaf exceeded original read budget");
    reads.emplace_back(address,std::uint32_t(output.size()));
    std::copy_n(region.data.begin()+(address-region.address),output.size(),output.begin());
  }
  void write(std::uint32_t,std::span<const std::byte>)override {
    throw std::runtime_error("Native read-only leaf attempted a memory write");
  }
};
std::uint32_t word(const Region &region,unsigned offset) {
  require(offset+4<=region.data.size(),"Fixture metadata read escaped its declared region");
  std::uint32_t value{};
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(region.data[offset+i])<<(8*i);
  return value;
}
struct Case {
  std::string name;
  unsigned expected{};
  SourcePhysicsWorldRuntimeAbi input{};
  std::array<std::uint32_t,8> returned{};
  std::uint32_t flags{},control{},mxcsr{},status{};
  Memory memory;
  std::vector<std::uint32_t> trace;
  std::vector<Read> reads;
};
void run(Case &test) {
  require(test.memory.regions.size()==4 && !test.trace.empty() && !test.reads.empty(),
      "Incomplete original descriptor-leaf case");
  const auto &header=test.memory.find(pool,0x4c);
  const auto &argument=test.memory.find(stack_base,256);
  const auto datum=word(argument,stack-stack_base+4u);
  const auto index=datum&65535u;
  require(header.address==pool && header.data.size()==0x4c && word(header,0x20)==256 &&
      word(header,0x24)==0x888 && word(header,0x44)==records && index<256,
      "Fixture changed the actual actor capacity/stride or selected record");
  require(test.memory.find(global,4).address==global &&
      word(test.memory.find(global,4),0)==pool &&
      test.memory.find(records+index*0x888u,0x888u).data.size()==0x888u,
      "Fixture lost its literal actor pool/record identity");
  auto abi=test.input;
  std::vector<std::uint32_t> trace;
  SourcePhysicsWorldRuntimeServices services{};
  services.instruction=[&](std::uint32_t pc) {
    require(trace.size()<32,"Native leaf exceeded original instruction budget");trace.push_back(pc);
  };
  // This existing runner accepts admitted internal entries independently of
  // the world dispatcher's public ownership selector. No owner is fabricated.
  source_run_ai_frame_tu15(test.memory,entry,abi,{0x27f,0x1f80},services);
  require(abi.gp==test.returned,"Original/native full GP or RET4 stack cleanup differs");
  require(abi.eflags==test.flags,"Original/native complete EFLAGS differ");
  require(abi.gp[0]==test.expected,"Original/native full EAX return differs");
  require(abi.fs_base==test.input.fs_base && abi.xmm==test.input.xmm &&
      abi.x87==test.input.x87 && abi.x87_depth==test.input.x87_depth &&
      abi.x87_status==test.status && !abi.active_mmx,
      "Original/native untouched FP/XMM/segment ABI differs");
  require(abi.floating_point && abi.floating_point->x87_control_word==test.control &&
      abi.floating_point->mxcsr==test.mxcsr,"Original/native FP controls differ");
  require(trace==test.trace,"Original/native complete instruction path differs");
  require(test.memory.reads==test.reads,"Original/native complete read ordering/width differs");
  for(const auto &region:test.memory.regions)
    require(region.data==region.initial,"Native leaf changed complete declared bank/stack bytes");
  std::cout<<"PASS "<<test.name<<'\n';
}
}
int main(int argc,char **argv)try {
  guard();require(argc==2,"Supply frozen original.tsv.gz");
  GzipFixture input(argv[1]);
  std::size_t total{},count{};Case test;bool active=false;unsigned seen{};
  for(std::string line;input.next_line(line);) {
    total+=line.size()+1;require(total<=4u*1024u*1024u,"Fixture exceeded bounded export budget");
    std::istringstream row(line);char kind{};require(bool(row>>kind),"Empty fixture row");
    if(kind=='C') {
      require(!active && count<432,"Nested or oversized descriptor matrix");
      test=Case{};active=true;seen=0;
      require(bool(row>>test.name>>test.expected) && (test.expected==0 || test.expected==3),
          "Invalid descriptor case metadata");
    } else {
      require(active,"Fixture row outside a case");
      if(kind=='I') {
        require(!(seen&1),"Duplicate input ABI");seen|=1;
        for(auto &value:test.input.gp)value=number(row);
        test.input.eflags=number(row);test.input.fs_base=number(row);
        const auto control=number(row),mxcsr=number(row);
        test.input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(control),mxcsr};
        test.input.x87_status=std::uint16_t(number(row));
      } else if(kind=='X') {
        require(!(seen&2),"Duplicate XMM ABI");seen|=2;
        for(auto &lanes:test.input.xmm)for(auto &value:lanes)value=number(row);
      } else if(kind=='R') {
        require(test.memory.regions.size()<4,"Too many declared regions");
        const auto address=number(row);std::string initial;require(bool(row>>initial),"Missing bank bytes");
        Region region{address,bytes(initial),{}};region.data=region.initial;
        test.memory.regions.push_back(std::move(region));
      } else if(kind=='O') {
        require(!(seen&4),"Duplicate output ABI");seen|=4;
        for(auto &value:test.returned)value=number(row);
        test.flags=number(row);test.control=number(row);test.mxcsr=number(row);test.status=number(row);
      } else if(kind=='T') {
        require(!(seen&8),"Duplicate original trace");seen|=8;
        unsigned size{};require(bool(row>>size) && size>0 && size<=32,"Invalid original trace size");
        for(unsigned i=0;i<size;++i)test.trace.push_back(number(row));
      } else if(kind=='D') {
        require(!(seen&16),"Duplicate original reads");seen|=16;
        unsigned size{};require(bool(row>>size) && size>0 && size<=16,"Invalid original read size");
        for(unsigned i=0;i<size;++i) {
          const auto address=number(row),width=number(row);
          require(width==1 || width==4,"Invalid original read width");test.reads.emplace_back(address,width);
        }
      } else if(kind=='E') {
        require(seen==31,"Missing original ABI/path/read fields");run(test);++count;active=false;
      } else throw std::runtime_error("Unknown original fixture row");
    }
    std::string extra;require(!(row>>extra),"Trailing original fixture data");
  }
  require(!active && count==432,"Incomplete declared-capacity256 actor matrix");
  std::cout<<"PASS original/native1C9C90: 432 full ABI/trace/read/bank comparisons\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
