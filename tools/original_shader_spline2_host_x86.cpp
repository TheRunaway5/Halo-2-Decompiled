// Test-only unchanged XBE execution under the host i386 instruction set.
// The host RSQRTSS estimate is explicit; this is not Xbox precision proof.
#include <sys/mman.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
static std::uint32_t word(const std::vector<unsigned char>&b,unsigned at) {
  if(at>b.size() || b.size()-at<4)throw std::runtime_error("Truncated XBE word");
  std::uint32_t value;std::memcpy(&value,b.data()+at,4);return value;
}
int main(int argc,char **argv)try {
  if(argc!=3)return 2;
  std::ifstream file(argv[1],std::ios::binary);
  const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file),{}};
  if(bytes.empty())throw std::runtime_error("Original XBE unavailable");
  void *bank=mmap(reinterpret_cast<void*>(0x10000),0x800000,PROT_READ|PROT_WRITE|PROT_EXEC,
      MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
  if(bank!=reinterpret_cast<void*>(0x10000))throw std::runtime_error("Original fixed address bank unavailable");
  const auto base=word(bytes,0x104),count=word(bytes,0x11c),table=word(bytes,0x120)-base;
  for(unsigned n=0;n<count;++n) {
    const auto row=table+n*56,address=word(bytes,row+4),offset=word(bytes,row+12),size=word(bytes,row+16);
    if(address<0x10000 || address+size>0x810000 || offset>bytes.size() || bytes.size()-offset<size)
      throw std::runtime_error("Original section exceeds declared bank");
    std::memcpy(reinterpret_cast<void*>(address),bytes.data()+offset,size);
    if(std::memcmp(reinterpret_cast<void*>(address),bytes.data()+offset,size))throw std::runtime_error("Original bytes changed");
  }
  const auto entry=std::uint32_t(std::stoul(argv[2],nullptr,16));
  std::string line;
  while(std::getline(std::cin,line)) {
    std::istringstream row(line);std::string hex;std::uint32_t input,range,result;
    if(!(row>>hex>>std::hex>>input>>range) || hex.size()%2 || hex.size()>8192)
      throw std::runtime_error("Invalid spline2 ABI input");
    std::vector<unsigned char> curve;
    for(unsigned at=0;at<hex.size();at+=2)curve.push_back(std::stoul(hex.substr(at,2),nullptr,16));
    const std::uint32_t descriptor[]{std::uint32_t(curve.size()),std::uint32_t(curve.data())};
    const std::uint16_t control=0x027f;const std::uint32_t mxcsr=0x1f80;
    asm volatile("fninit; fldcw %0; ldmxcsr %1"::"m"(control),"m"(mxcsr):"memory");
    asm volatile("pushl %[range]; pushl %[input]; movl %[descriptor],%%edi; call *%[entry]; movd %%xmm0,%[result]"
        :[result]"=m"(result):[range]"m"(range),[input]"m"(input),[descriptor]"r"(descriptor),[entry]"r"(entry)
        :"eax","ecx","edx","edi","memory","xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
         "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::cout<<std::hex<<result<<'\n';
  }
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
