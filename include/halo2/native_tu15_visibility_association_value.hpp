#pragma once
#include "halo2/native_tu15_visibility_association_kernel.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace halo2::native_visibility_association_detail {
struct HostView {
    std::vector<std::array<std::uint32_t,4>>&rows;bool&capacity_reached;std::size_t entry;
    std::size_t reserve_index()const{return rows.size();}
    bool valid_index(std::size_t index)const{return index!=512;}
    void saturate(){capacity_reached=true;}
    std::uint16_t unavailable()const{return 0;}
    void clear_mask(std::size_t){rows.push_back({});}
    std::size_t ordinal()const{return entry;}
    void mark(std::size_t index,std::size_t,BitLocation bit){rows.at(index)[std::size_t(bit.word)]|=bit.mask;}
    std::uint16_t completed(std::size_t index)const{return std::uint16_t(index);}
};
// The owning admission passes already validate at most512 prior groups and
// an ordinal below128. Their existing sky/format policy remains at the caller.
inline std::uint16_t reserve_mask(std::vector<std::array<std::uint32_t,4>>&rows,bool&capacity_reached,std::size_t ordinal){
    HostView view{rows,capacity_reached,ordinal};return reserve(view);
}
} // namespace halo2::native_visibility_association_detail
