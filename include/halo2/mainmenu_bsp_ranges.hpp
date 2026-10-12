#pragma once
#include "halo2/mainmenu_bsp_frame.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include <memory>
#include <string>

namespace halo2 {
struct MainmenuBspPartRanges {
    unsigned part{};
    MenuWorldSourceTopology topology=MenuWorldSourceTopology::triangle_list;
    // Complete empty is meaningful: the original submitted no indices.
    std::vector<MenuWorldSourceIndexRange> ranges;
};
struct MainmenuBspRangeUnit {
    int geometry=-1; // >=0 authored cluster; <=-2 authored instance.
    unsigned section{}; // Cluster or actual instance definition ordinal.
    std::uint32_t flags{}; // Original draw-unit word0, classification3 included.
    std::uint8_t coverage{},alpha{}; //2CBF0 metric byte and actual133520 alpha.
    std::optional<unsigned> mask_word_offset;
    std::optional<Transform> transform; // Literal instance+0 52-byte bank.
    std::vector<MainmenuBspPartRanges> parts; // Authored part order.
};
struct MainmenuBspRangeFrame {
    bool complete{};
    std::string diagnostic;
    SourceBspRangeArena arena;
    // Original133390 cluster queue, then positive-alpha133520 instance queue.
    std::vector<MainmenuBspRangeUnit> units;
    std::vector<unsigned> zero_alpha_instances;
    const MainmenuBspRangeUnit *unit_for_geometry(int) const;
};
// Source-owned normal-mode0 query0 BSP schedule/range producer. The loaded
// Cache supplies literal geometry independently of supported vertex formats.
// All resources are explicitly resident; no active lightmap gives2D000 class3.
// This is the BSP-only prefix before dynamic133720. It must not be used as the
// live global arena result until dynamic collectors' interaction is proved.
// The input must be this Cache/BSP's current canonical admission frame.
// An unsupported consumed service returns incomplete and exposes no partial
// units, ranges, arena or alpha-skip results. No captured runtime state is read.
class MainmenuBspRangeProducer {
public:
    MainmenuBspRangeProducer(const Cache &,std::size_t bsp=0);
    ~MainmenuBspRangeProducer();
    MainmenuBspRangeProducer(const MainmenuBspRangeProducer &)=delete;
    MainmenuBspRangeProducer &operator=(const MainmenuBspRangeProducer &)=delete;
    MainmenuBspRangeFrame prepare(const MainmenuBspAdmissionFrame &);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
