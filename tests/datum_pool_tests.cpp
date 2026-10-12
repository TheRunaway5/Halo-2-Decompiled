#include "halo2/datum_pool.hpp"
#include "halo2/cache.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
using namespace halo2;
namespace {
void check(bool v,const char *s) { if (!v) throw FormatError(s); }
std::uint32_t word(std::istringstream &row) {
  std::string v; row >> v; return std::stoul(v,nullptr,16);
}
std::string hex(std::span<const std::uint8_t> bytes) {
  constexpr char table[]="0123456789abcdef"; std::string out;
  for (auto b:bytes) { out+=table[b>>4]; out+=table[b&15]; } return out;
}
}
int main(int argc,char **argv) {
  try {
    if (argc!=2 && argc!=3) throw FormatError("Supply original datum fixtures and optional source-bound mode");
    const bool bound=argc==3 && std::string_view(argv[2])=="source-bound";
    if(argc==3 && !bound)throw FormatError("Unknown source datum mode");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream file(argv[1]);std::unique_ptr<SourceRuntimeMemory> memory; std::unique_ptr<DatumPool> pool;
    unsigned allocations=0,snapshots=0,lookups=0;
    for (std::string line;std::getline(file,line);) {
      std::istringstream row(line); char kind; row>>kind;
      if (kind=='N') {
        std::string name;int capacity,stride;unsigned flags;
        row>>name>>capacity>>stride>>flags;
        if(bound) {
          pool.reset();memory=std::make_unique<SourceRuntimeMemory>();
          constexpr auto address=0x10000000u,names=0x20000000u;
          memory->mount(address,std::vector<std::byte>(0x4c+std::size_t(capacity)*stride+((capacity+31)/32)*4+16),
                        SourceRuntimeMemory::Access::ReadWrite,"original pool fixture allocation input");
          std::array<std::byte,32> text{};for(std::size_t i=0;i<std::min<std::size_t>(name.size(),31);++i)text[i]=std::byte(name[i]);
          memory->mount(names,text,SourceRuntimeMemory::Access::ReadOnly,"original pool fixture name input");
          SourceRawDatumPoolServices storage{[&](auto p,auto n){return memory->mutable_view(p,n);},
                                             [&](auto p,auto n){return memory->view(p,n);}};
          source_raw_datum_initialize_inplace(address,names,capacity,stride,0,0,storage);
          memory->mutable_view(address+0x2a,1)[0]=std::byte(flags); // Explicit original fixture flag input.
          source_raw_datum_make_valid(address,storage);
          pool=std::make_unique<DatumPool>(*memory,address);
        } else pool=std::make_unique<DatumPool>(name,capacity,stride,std::uint16_t(flags));
      } else if (kind=='W') pool->set_next_salt(std::uint16_t(word(row)));
      else if (kind=='A') { check(pool->allocate()==word(row),"Original automatic datum differs");++allocations; }
      else if (kind=='I') {
        int index;row>>index;check(pool->allocate_at(index)==word(row),"Original requested datum differs");++allocations;
      } else if (kind=='D') pool->release_unchecked(word(row));
      else if (kind=='Z') pool->reset();
      else if (kind=='F') {
        unsigned index;std::string bytes;row>>index>>bytes;
        check(hex(pool->elements().subspan(std::size_t(index)*pool->stride(),pool->stride()))==bytes,
              "Original free element poison bytes differ");
      }
      else if (kind=='L') {
        const auto datum=word(row);unsigned present;row>>present;
        check(!pool->lookup(datum).empty()==bool(present),"Original checked datum lookup differs");++lookups;
      } else if (kind=='S') {
        int cursor,high,count;row>>cursor>>high>>count;const auto next=word(row);
        std::string expected_bitmap;row>>expected_bitmap;
        std::vector<std::uint8_t> bitmap;
        for (auto w:pool->bitmap()) for (unsigned i=0;i<4;++i) bitmap.push_back(std::uint8_t(w>>(i*8)));
        check(pool->cursor()==cursor && pool->high_water()==high && pool->count()==count &&
            pool->next_salt()==next && hex(bitmap)==expected_bitmap,"Original datum pool state differs");
        unsigned entries,full;row>>entries>>full;
        unsigned actual=0;
        for (int i=0;i<pool->capacity();++i) {
          const auto bytes=pool->elements().subspan(std::size_t(i)*pool->stride(),pool->stride());
          if (bytes[0] || bytes[1]) ++actual;
        }
        check(actual==entries,"Original datum occupied count differs");
        for (unsigned i=0;i<entries;++i) {
          unsigned index;row>>index;const auto salt=word(row);
          const auto datum=(salt<<16)|index;
          const auto bytes=pool->lookup(datum);
          check(!bytes.empty(),"Original occupied datum does not resolve");
          if (full) { std::string record;row>>record;check(hex(bytes)==record,"Original allocated element bytes differ"); }
        }
        ++snapshots;
      } else throw FormatError("Unknown datum fixture operation");
      check(!row.fail(),"Incomplete datum fixture");std::string extra;
      check(!(row>>extra),"Trailing datum fixture");
    }
    check(allocations && snapshots && lookups,"Empty original datum comparisons");
    std::cout<<allocations<<" source allocation outcomes, "<<snapshots
             <<" pool snapshots and "<<lookups<<" full-datum lookups matched\n";
    return 0;
  } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
