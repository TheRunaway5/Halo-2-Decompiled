#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <vector>

namespace halo2 {
// Charge capacity, not just logical elements, before requesting allocation.
// One owner is shared by parallel decoded streams and retained prototypes.
class DecodedAllocationBudget {
public:
  explicit DecodedAllocationBudget(std::size_t bytes):remaining_(bytes){}
  template<class T> void grow(std::vector<T> &v,std::size_t extra) {
    if(extra>v.max_size()-v.size())throw FormatError("decoded allocation overflow");
    const auto wanted=v.size()+extra;
    if(wanted<=v.capacity())return;
    const auto available=remaining_/sizeof(T);
    if(wanted-v.capacity()>available)throw FormatError("decoded allocation budget exceeded");
    const auto increment=std::min(available,std::max(wanted-v.capacity(),v.capacity()));
    v.reserve(v.capacity()+increment);remaining_-=increment*sizeof(T);
  }
  template<class T> void release(const std::vector<T> &v){remaining_+=v.capacity()*sizeof(T);}
private:
  std::size_t remaining_;
};
}
