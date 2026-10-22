#include "rendering/renderer/frame_vertex_reuse.hpp"
#include "rendering/renderer/frame_vertex_upload_plan.hpp"
#include <deque>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <type_traits>
using namespace halo2;
static void require(bool value) {if(!value)throw std::runtime_error("frame vertex alias differs from append-only oracle");}
static void verify_borrowed_upload_plan() {
  std::size_t checks{};
  for(bool share:{false,true}) {
    FrameVertexUploadPlan<unsigned> plan(share);
    std::vector<Vertex> original;FrameVertexReuse<unsigned> oracle(original,share);
    std::deque<std::vector<Vertex>> banks;
    unsigned seed=0x137a249u;
    banks.emplace_back(12);
    for(auto &byte:std::as_writable_bytes(std::span{banks.front()})) {
      seed=1664525u*seed+1013904223u;byte=std::byte(seed>>24);
    }
    for(unsigned iteration=0;iteration<320;++iteration) {
      banks.push_back(banks.front());auto &bank=banks.back();
      if(iteration<sizeof(Vertex))std::as_writable_bytes(std::span{bank})[sizeof(Vertex)+iteration]^=std::byte{1};
      if(iteration%5==0)bank.resize(6);
      if(iteration%17==0)bank.clear();
      const auto span=std::span<const Vertex>{bank};
      const auto split=span.size()/2;
      const std::array parts{span.first(split),std::span<const Vertex>{},span.subspan(split)};
      const auto key=iteration%4;
      const auto expected=oracle.append_parts(key,parts);
      const auto transformed=parts | std::views::transform([](auto part){return part;});
      const auto actual=iteration%3==0?plan.append_parts(key,transformed):iteration%3==1?plan.append_parts(key,parts):plan.append(key,span);
      if(expected!=actual || original.size()!=plan.size())throw std::runtime_error("upload range alias differs");
      std::vector<Vertex> materialized(plan.size());plan.copy_to(materialized.data());
      if(!original.empty() && std::memcmp(original.data(),materialized.data(),original.size()*sizeof(Vertex)))
        throw std::runtime_error("complete upload bytes differ");
      ++checks;
    }
    // Unkeyed UI tails can be edited while all keyed source streams stay immutable.
    banks.push_back(banks.front());auto &ui=banks.back();const auto first=plan.append_raw(ui);
    original.insert(original.end(),ui.begin(),ui.end());
    ui.front().position.x=13.75f;original[first].position.x=13.75f;
    std::vector<Vertex> materialized(plan.size());plan.copy_to(materialized.data());
    if(std::memcmp(original.data(),materialized.data(),original.size()*sizeof(Vertex)))throw std::runtime_error("UI upload tail differs");
    ++checks;
  }
  std::cout<<checks<<" full original-staging/upload comparisons passed, including all metadata bytes, segmentation, empty runs, keyed alias ranges, sharing off and mutable UI tails\n";
}

int main() {
    verify_borrowed_upload_plan();
    static_assert(std::is_trivially_copyable_v<Vertex>);
    std::vector<Vertex> actual,reference;
    FrameVertexReuse<unsigned> reuse(actual,true);
    std::vector<Vertex> input(6);
    unsigned seed=0x137a249u;
    for(auto &v:input)for(auto &byte:std::as_writable_bytes(std::span{&v,1})) {
        seed=1664525u*seed+1013904223u;byte=std::byte(seed>>24);
    }
    std::size_t comparisons{};
    const auto check=[&](unsigned key,const std::vector<Vertex> &bank,bool segmented) {
        const auto first=segmented?reuse.append_parts(key,std::array{
            std::span<const Vertex>{bank}.first(2),std::span<const Vertex>{bank}.subspan(2)}):reuse.append(key,bank);
        reference.insert(reference.end(),bank.begin(),bank.end());
        require(first+bank.size()<=actual.size());
        require(std::memcmp(actual.data()+first,reference.data()+reference.size()-bank.size(),bank.size()*sizeof(Vertex))==0);
        ++comparisons;return first;
    };
    const auto first=check(0,input,false);
    require(check(0,input,true)==first);require(actual.size()==input.size());
    require(check(1,input,false)!=first); // Full key remains independent.
    for(std::size_t byte=0;byte<sizeof(Vertex);++byte) {
        auto changed=input;
        auto raw=std::as_writable_bytes(std::span{changed});raw[sizeof(Vertex)+byte]^=std::byte{1};
        const auto previous=actual.size();check(0,changed,byte%2);require(actual.size()==previous+changed.size());
        require(check(0,changed,!(byte%2))==previous);
    }
    for(unsigned iteration=0;iteration<1000;++iteration) {
        auto changed=input;
        if(iteration%3)changed.resize(3);
        if(iteration%7==0)std::as_writable_bytes(std::span{changed})[iteration%sizeof(Vertex)]^=std::byte{2};
        check(iteration%5,changed,iteration%2);
    }
    // A subsequent source mutation cannot alter a retained stream.
    const auto held=check(9,input,false);auto saved=input;
    std::fill(std::as_writable_bytes(std::span{input}).begin(),std::as_writable_bytes(std::span{input}).end(),std::byte{});
    require(std::memcmp(actual.data()+held,saved.data(),saved.size()*sizeof(Vertex))==0);
    std::vector<Vertex> serial;FrameVertexReuse<unsigned> disabled(serial,false);
    disabled.append(0,saved);disabled.append(0,saved);require(serial.size()==saved.size()*2);
    std::cout<<comparisons<<" complete byte comparisons, "<<actual.size()<<" retained vs "<<reference.size()<<" reference vertices\n";
}
