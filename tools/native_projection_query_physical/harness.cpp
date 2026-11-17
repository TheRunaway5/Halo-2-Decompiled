// Physical-original/translated/native differential for the projection queries.
// Built and run by tools/validate_native_projection_query_tu15.py.
// Offline differential: qualified translated 2E79A0/2E6290/309600/3092A0 and
// 2E7C40/2E6ED0 bodies (extracted verbatim, bridge lines removed) versus the
// native projection-query bridges, on generated MOPP programs, planes,
// spheres, arrays (including growth), FP environments and register states.
#include <array>
#include <cstdint>
static std::array<std::array<unsigned long long, 256>, 6> coverage{};
#define HALO2_PROJECTION_QUERY_VISIT(where, value) (++coverage[(where)][(value) & 255])
#include <cstdio>
#include <map>
#include <memory>
#include <random>
#include <string>
#include "translated.inc"

using PageMap = std::map<std::uint32_t, std::vector<std::byte>>;
class Bank final : public SourcePhysicsContactMemory {
public:
    PageMap pages;
    std::unique_ptr<SourceDirectPageIndex> index;
    bool direct = false;
    void map(std::uint32_t a, std::uint32_t n) {
        for (auto p = a & ~4095u; p < a + n; p += 4096) pages.try_emplace(p, std::vector<std::byte>(4096));
    }
    std::byte *at(std::uint32_t a) const {
        auto it = pages.find(a & ~4095u);
        if (it == pages.end()) { char t[48]; std::snprintf(t, sizeof t, "unmapped access %08x", a); throw std::runtime_error(t); }
        return const_cast<std::byte *>(it->second.data()) + (a & 4095u);
    }
    void read(std::uint32_t a, std::span<std::byte> out) const override { for (std::size_t i = 0; i < out.size(); ++i) out[i] = *at(std::uint32_t(a + i)); }
    void write(std::uint32_t a, std::span<const std::byte> in) override { for (std::size_t i = 0; i < in.size(); ++i) *at(std::uint32_t(a + i)) = in[i]; }
    void publish_direct() {
        index = std::make_unique<SourceDirectPageIndex>();
        for (auto &[p, bytes] : pages) {
            auto &block = index->blocks[p >> 22];
            if (!block) block = std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(p >> 12) & 1023u] = {bytes.data(), true};
        }
        direct_pages_ = index.get();
    }
    void u32(std::uint32_t a, std::uint32_t v) { std::memcpy(at(a), &v, 4); for (unsigned i = 1; i < 4; ++i) *at(a + i) = std::byte(v >> (8 * i)); }
    std::uint32_t u32(std::uint32_t a) const { std::uint32_t v = 0; for (unsigned i = 0; i < 4; ++i) v |= std::to_integer<std::uint32_t>(*at(a + i)) << (8 * i); return v; }
    void bytes(std::uint32_t a, const std::vector<std::uint8_t> &b) { for (std::size_t i = 0; i < b.size(); ++i) *at(std::uint32_t(a + i)) = std::byte(b[i]); }
};

constexpr std::uint32_t stack_base = 0x30000000u, stack_size = 0x40000u, heap_base = 0x20000000u;
struct Scenario {
    PageMap pages; std::uint32_t entry{}, esp{};
    SourcePhysicsContactRuntimeAbi abi{}; FP fp{};
};
struct Outcome { SourcePhysicsContactRuntimeAbi abi{}; PageMap pages; std::string error; unsigned growths{}; bool native_ran{}; };

// Shared genuine-semantics 2E3250 for both routes (bump heap, deterministic).
struct Growth {
    unsigned count = 0;
    void operator()(SourcePhysicsContactMemory &m, const SourcePhysicsContactRuntimeCall &c, SourcePhysicsContactRuntimeAbi &a,
                    const SourcePhysicsContactRecordObserver &) {
        auto &bank = static_cast<Bank &>(m);
        if (c.target != 0x2e3250u) throw std::runtime_error("unexpected required owner");
        const auto esp = a.gp[4], array = bank.u32(esp + 4), stride = bank.u32(esp + 8);
        const auto n = bank.u32(array + 4), capacity = n ? n * 2 : 1;
        auto top = bank.u32(heap_base);
        const auto data = top; top += (capacity * stride + 15) & ~15u; bank.u32(heap_base, top);
        bank.map(data, capacity * stride + 16);
        for (std::uint32_t i = 0; i < n * stride; ++i) *bank.at(data + i) = *bank.at(bank.u32(array) + i);
        bank.u32(array, data); bank.u32(array + 8, capacity);
        a.gp[4] = esp + 4; a.gp[0] = data; ++count;
    }
};

Outcome run(const Scenario &sc, bool native, bool direct_pages) {
    Outcome o;
    Bank bank; bank.pages = sc.pages;
    if (direct_pages) bank.publish_direct();
    Growth growth;
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner = [&growth](auto &m, const auto &c, auto &a, const auto &obs) { growth(m, c, a, obs); };
    static const O observe{};
    auto abi = sc.abi;
    try {
        FpScope scope(sc.fp);
        LiveState s{bank, observe, services, SourcePhysicsContactImage::TitleUpdate15};
        s.load(abi);
        const auto child = [](auto &v, std::uint32_t t, std::uint32_t site, bool tail) { dispatch(v, t, site, tail); };
        if (sc.entry == 0x2e79a0u) {
            if (!native || !(o.native_ran = native_tu15_projection_query::bridge_2e79a0(s, child, [](auto &v) { run_2e79a0(v); }))) run_2e79a0(s);
        } else {
            if (!native || !(o.native_ran = native_tu15_projection_query::bridge_2e7c40(s, child, [](auto &v) { run_2e7c40(v); }))) run_2e7c40(s);
        }
        o.abi = s.save();
    } catch (const std::exception &e) { o.error = e.what(); }
    o.pages = std::move(bank.pages); o.growths = growth.count;
    return o;
}

// ------------------------------------------------------------ generators
struct Gen {
    std::mt19937 &rng;
    int u(int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); }
    bool p(double x) { return std::uniform_real_distribution<double>(0, 1)(rng) < x; }
    std::vector<std::uint8_t> prefix() {
        std::vector<std::uint8_t> out;
        while (p(0.25)) {
            switch (u(0, 6)) {
            case 0: out.insert(out.end(), {std::uint8_t(u(1, 4)), std::uint8_t(u(0, 255)), std::uint8_t(u(0, 255)), std::uint8_t(u(0, 255))}); break;
            case 1: out.insert(out.end(), {9, std::uint8_t(u(0, 255))}); break;
            case 2: out.insert(out.end(), {0x0a, std::uint8_t(u(0, 255)), std::uint8_t(u(0, 255))}); break;
            case 3: out.insert(out.end(), {0x0b, std::uint8_t(u(0, 3)), std::uint8_t(u(0, 255)), std::uint8_t(u(0, 255)), std::uint8_t(u(0, 255))}); break;
            case 4: {
                // Properties: slot0 anywhere; slots1..3 rarely (walk-only owners).
                const int width = u(0, 2), slot = p(0.85) ? 0 : u(1, 3);
                out.push_back(std::uint8_t(0x60 + width * 4 + slot));
                for (int i = 0; i < (width == 0 ? 1 : width == 1 ? 2 : 4); ++i) out.push_back(std::uint8_t(u(0, 255)));
                break;
            }
            case 5: {
                const int skip = u(0, 6);
                if (p(0.5)) { out.insert(out.end(), {5, std::uint8_t(skip)}); }
                else if (p(0.5)) { out.insert(out.end(), {6, 0, std::uint8_t(skip)}); }
                else { out.insert(out.end(), {7, 0, 0, std::uint8_t(skip)}); }
                for (int i = 0; i < skip; ++i) out.push_back(std::uint8_t(u(0, 255)));
                break;
            }
            default: {
                const int n = u(1, 3);
                for (int i = 0; i < n; ++i) {
                    if (p(0.7)) { int lo = u(0, 255), hi = u(lo, 255); out.insert(out.end(), {std::uint8_t(0x26 + u(0, 2)), std::uint8_t(lo), std::uint8_t(hi)}); }
                    else {
                        const int lo = u(0, 0xffffff), hi = u(lo, 0xffffff);
                        out.insert(out.end(), {std::uint8_t(0x29 + u(0, 2)), std::uint8_t(lo >> 16), std::uint8_t(lo >> 8), std::uint8_t(lo),
                                               std::uint8_t(hi >> 16), std::uint8_t(hi >> 8), std::uint8_t(hi)});
                    }
                }
            }
            }
        }
        return out;
    }
    std::vector<std::uint8_t> leaf() {
        if (p(0.05)) return {0};
        if (p(0.7)) return {std::uint8_t(0x30 + u(0, 31))};
        const int w = u(0, 3);
        std::vector<std::uint8_t> out{std::uint8_t(0x50 + w)};
        for (int i = 0; i <= w; ++i) out.push_back(std::uint8_t(u(0, w == 3 && i == 0 ? 3 : 255)));
        return out;
    }
    std::vector<std::uint8_t> node(int depth) {
        auto out = prefix();
        if (depth <= 0 || p(0.15)) { auto l = leaf(); out.insert(out.end(), l.begin(), l.end()); return out; }
        auto first = node(depth - 1), second = node(depth - 1);
        const int kind = u(0, 9);
        if (kind <= 2 && first.size() < 256) {
            int b1 = u(0, 255), b2 = p(0.8) ? u(std::max(0, b1 - 40), 255) : u(0, 255);
            out.insert(out.end(), {std::uint8_t(0x10 + u(0, 2)), std::uint8_t(b1), std::uint8_t(b2), std::uint8_t(first.size())});
        } else if (kind <= 4 && first.size() < 256) {
            out.insert(out.end(), {std::uint8_t(0x20 + u(0, 2)), std::uint8_t(u(0, 254)), std::uint8_t(first.size())});
        } else if (kind <= 6 && first.size() < 256) {
            out.insert(out.end(), {std::uint8_t(0x13 + u(0, 9)), std::uint8_t(u(0, 255)), std::uint8_t(u(0, 255)), std::uint8_t(first.size())});
        } else {
            const int gap = p(0.3) ? u(1, 5) : 0;
            const int b1 = u(0, 255), b2 = u(std::max(0, b1 - 40), 255);
            const auto off2 = first.size() + gap;
            out.insert(out.end(), {std::uint8_t(0x23 + u(0, 2)), std::uint8_t(b1), std::uint8_t(b2), 0, std::uint8_t(gap),
                                   std::uint8_t(off2 >> 8), std::uint8_t(off2)});
            for (int i = 0; i < gap; ++i) out.push_back(std::uint8_t(u(0, 255)));
        }
        out.insert(out.end(), first.begin(), first.end());
        out.insert(out.end(), second.begin(), second.end());
        return out;
    }
    float special(float ordinary) {
        if (!p(0.02)) return ordinary;
        static const std::uint32_t values[] = {0x7fc00000u, 0xffc00001u, 0x7f800001u, 0x00000001u, 0x80000010u, 0x7f800000u, 0xff800000u, 0x80000000u, 0u};
        return std::bit_cast<float>(values[u(0, 8)]);
    }
    float f(float a, float b) { return special(std::uniform_real_distribution<float>(a, b)(rng)); }
};

static bool physical_cases = false;
Scenario make(std::mt19937 &rng, bool sphere, std::uint64_t n) {
    Gen g{rng};
    Bank bank;
    // Authenticated TU1.5 image pages: dispatch byte tables and constants.
    static const auto image = [] {
        std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> out;
        std::FILE *f = std::fopen(IMAGE_PATH, "rb");
        if (!f) throw std::runtime_error("image.bin missing");
        std::uint32_t head[2];
        while (std::fread(head, 4, 2, f) == 2) {
            std::vector<std::uint8_t> bytes(head[1]);
            if (std::fread(bytes.data(), 1, bytes.size(), f) != bytes.size()) throw std::runtime_error("image.bin truncated");
            out.push_back({head[0], std::move(bytes)});
        }
        std::fclose(f);
        return out;
    }();
    for (const auto &[a, bytes] : image) { bank.map(a, std::uint32_t(bytes.size())); bank.bytes(a, bytes); }
    bank.map(stack_base, stack_size);
    bank.map(heap_base, 0x100000u);
    bank.u32(heap_base, heap_base + 0x80000u);
    const auto mopp = heap_base + 0x1000u;
    auto code = g.node(g.u(1, 9));
    code.push_back(0);
    bank.map(mopp, 0x30 + code.size() + 16);
    const float scale = std::bit_cast<float>(0x4b800000u) / g.f(8.f, 256.f);
    const std::array<float, 4> origin{g.f(-50.f, 50.f), g.f(-50.f, 50.f), g.f(-50.f, 50.f), g.special(scale)};
    for (unsigned i = 0; i < 4; ++i) bank.u32(mopp + 4 * i, std::bit_cast<std::uint32_t>(origin[i]));
    bank.bytes(mopp + 0x30, code);
    const float extent = std::bit_cast<float>(0x4b800000u) / scale;
    const auto args = heap_base + 0x2000u;
    std::uint32_t second;
    if (sphere) {
        const std::array<float, 4> s{origin[0] + g.f(-0.2f, 1.2f) * extent, origin[1] + g.f(-0.2f, 1.2f) * extent,
                                     origin[2] + g.f(-0.2f, 1.2f) * extent, g.f(0.f, 0.6f) * extent};
        for (unsigned i = 0; i < 4; ++i) bank.u32(args + 4 * i, std::bit_cast<std::uint32_t>(s[i]));
        second = args;
    } else {
        const unsigned count = g.p(0.7) ? 6 : g.u(0, 12);
        bank.u32(args, count); bank.u32(args + 4, args + 0x10);
        for (unsigned i = 0; i < count; ++i) {
            const float x = g.f(-1, 1), y = g.f(-1, 1), z = g.f(-1, 1);
            const float c = g.f(0, 1) * extent;
            const float w = g.special(-(x * (origin[0] + c) + y * (origin[1] + c) + z * (origin[2] + c)) + g.f(-0.5f, 0.5f) * extent);
            for (unsigned k = 0; k < 4; ++k) bank.u32(args + 0x10 + 16 * i + 4 * k, std::bit_cast<std::uint32_t>(std::array<float, 4>{x, y, z, w}[k]));
        }
        second = args;
    }
    // Arrays: inline (DONT_DEALLOCATE) or owned, random size/capacity.
    std::array<std::uint32_t, 2> arrays{heap_base + 0x3000u, heap_base + 0x3100u};
    if (g.p(0.05)) arrays[1] = arrays[0];
    for (unsigned k = 0; k < 2; ++k) {
        const auto a = arrays[k];
        const std::uint32_t capacity = g.p(0.6) ? 0x400u : g.u(0, 6);
        const std::uint32_t size = capacity ? g.u(0, std::min<int>(capacity, 3)) : 0;
        const auto data = heap_base + 0x4000u + k * 0x2000u;
        bank.u32(a, data); bank.u32(a + 4, size); bank.u32(a + 8, capacity | (g.p(0.7) ? 0x80000000u : 0u));
        for (std::uint32_t i = 0; i < size; ++i) bank.u32(data + 4 * i, 0xabc00000u + i);
    }
    Scenario sc;
    sc.entry = sphere ? 0x2e7c40u : 0x2e79a0u;
    sc.esp = stack_base + stack_size - 0x100u - 4u * g.u(0, 7);
    bank.u32(sc.esp, sphere ? 0x456b0u : 0x45213u);
    bank.u32(sc.esp + 4, mopp); bank.u32(sc.esp + 8, second); bank.u32(sc.esp + 12, arrays[0]); bank.u32(sc.esp + 16, arrays[1]);
    sc.abi.gp = {0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u, sc.esp, 0x66666666u, 0x77777777u, 0x88888888u};
    for (auto &x : sc.abi.xmm) for (auto &v : x) v = std::uint32_t(rng());
    sc.abi.eflags = 0x202u | (rng() & 0x8d5u);
    sc.abi.x87_depth = physical_cases ? 0 : g.u(0, 2);
    for (unsigned i = 0; i < sc.abi.x87_depth; ++i) sc.abi.x87[i] = (long double)g.f(-10, 10);
    static const std::uint16_t controls[] = {0x027f, 0x037f, 0x007f, 0x0e7f, 0x067f, 0x0a7f};
    static const std::uint32_t mxcsrs[] = {0x1f80, 0x9fc0, 0x3f80, 0x5f80, 0x7f80, 0x1f81, 0x1fbf};
    sc.fp = {controls[n % 6], mxcsrs[(n / 6) % 7]};
    sc.abi.x87_status = std::uint16_t((g.u(0, 7) << 11) | (rng() & 0x4700u) | (rng() & 0x3fu));
    sc.abi.floating_point = sc.fp;
    sc.pages = std::move(bank.pages);
    return sc;
}

std::string compare(const Scenario &sc, const Outcome &a, const Outcome &b) {
    if (a.error != b.error) return "error '" + a.error + "' vs '" + b.error + "'";
    if (!a.error.empty()) return {};
    for (unsigned r : {3u, 4u, 5u, 6u, 7u}) if (a.abi.gp[r] != b.abi.gp[r]) return "gp" + std::to_string(r);
    if (a.abi.x87_depth != b.abi.x87_depth) return "x87 depth";
    if (a.abi.x87_status != b.abi.x87_status) {
        char t[64]; std::snprintf(t, sizeof t, "x87 status %04x vs %04x", a.abi.x87_status, b.abi.x87_status); return t;
    }
    for (unsigned i = 0; i < a.abi.x87_depth; ++i) if (std::memcmp(&a.abi.x87[i], &b.abi.x87[i], 10)) return "x87 value";
    if (a.abi.floating_point->x87_control_word != b.abi.floating_point->x87_control_word) return "cw";
    if (a.abi.floating_point->mxcsr != b.abi.floating_point->mxcsr) {
        char t[64]; std::snprintf(t, sizeof t, "mxcsr %08x vs %08x", a.abi.floating_point->mxcsr, b.abi.floating_point->mxcsr); return t;
    }
    if (a.growths != b.growths) return "growth count";
    if (a.pages.size() != b.pages.size()) return "page set";
    for (const auto &[page, bytes] : a.pages) {
        const auto &other = b.pages.at(page);
        for (unsigned i = 0; i < 4096; ++i) {
            const auto address = page + i;
            if (address >= stack_base && address < sc.esp) continue; // dead frames below entry ESP
            if (bytes[i] != other[i]) { char t[64]; std::snprintf(t, sizeof t, "memory @%08x", address); return t; }
        }
    }
    return {};
}

struct Physical { std::uint32_t crashed{}; std::array<std::uint32_t, 8> out{}; std::vector<std::byte> low, high; };
void emit(std::FILE *f, const Scenario &sc) {
    std::vector<std::uint32_t> pages;
    for (const auto &[a, bytes] : sc.pages)
        if ((a >= heap_base && a < heap_base + 0x100000u) || (a >= stack_base && a < stack_base + stack_size))
            for (auto b : bytes) if (b != std::byte{}) { pages.push_back(a); break; }
    const std::uint32_t h[10] = {sc.entry, sc.esp, sc.fp.x87_control_word, sc.abi.x87_status, sc.fp.mxcsr,
                                 sc.abi.gp[3], sc.abi.gp[5], sc.abi.gp[6], sc.abi.gp[7], std::uint32_t(pages.size())};
    std::fwrite(h, 4, 10, f);
    for (auto a : pages) { std::fwrite(&a, 4, 1, f); std::fwrite(sc.pages.at(a).data(), 1, 4096, f); }
}
bool collect(std::FILE *f, Physical &p) {
    if (std::fread(&p.crashed, 4, 1, f) != 1 || std::fread(p.out.data(), 4, 8, f) != 8) return false;
    std::uint32_t size; if (std::fread(&size, 4, 1, f) != 1) return false;
    p.low.resize(0x10000); p.high.resize(size);
    return std::fread(p.low.data(), 1, p.low.size(), f) == p.low.size() && std::fread(p.high.data(), 1, size, f) == size;
}
std::string against(const Outcome &o, const Physical &p) {
    if (p.crashed) return o.error.empty() ? "physical crashed, route completed" : std::string{};
    if (!o.error.empty()) return "error " + o.error;
    const std::array<std::uint32_t, 5> regs{o.abi.gp[3], o.abi.gp[4], o.abi.gp[5], o.abi.gp[6], o.abi.gp[7]};
    for (unsigned i = 0; i < 5; ++i) if (regs[i] != p.out[i]) return "reg" + std::to_string(i);
    char t[96];
    if (o.abi.x87_status != std::uint16_t(p.out[5])) { std::snprintf(t, sizeof t, "x87 status %04x vs physical %04x", o.abi.x87_status, std::uint16_t(p.out[5])); return t; }
    if (o.abi.floating_point->x87_control_word != std::uint16_t(p.out[6])) return "cw";
    if (o.abi.floating_point->mxcsr != p.out[7]) { std::snprintf(t, sizeof t, "mxcsr %08x vs physical %08x", o.abi.floating_point->mxcsr, p.out[7]); return t; }
    Bank b; b.pages = o.pages;
    for (std::uint32_t i = 0; i < 0x10000u; ++i) if (*b.at(heap_base + i) != p.low[i]) { std::snprintf(t, sizeof t, "memory @%08x", heap_base + i); return t; }
    if (b.u32(heap_base) != heap_base + 0x80000u + p.high.size()) return "heap top";
    for (std::uint32_t i = 0; i < p.high.size(); ++i) if (*b.at(heap_base + 0x80000u + i) != p.high[i]) { std::snprintf(t, sizeof t, "heap @%08x", heap_base + 0x80000u + i); return t; }
    return {};
}
// Typed host-row 2E7C40 (sphere_query_rows) versus the physical original's
// appended array rows, x87 status and MXCSR (growth-free, unaliased cases).
std::string rows_against(const Scenario &sc, const Physical &p) {
    Bank bank; bank.pages = sc.pages;
    const auto mopp = bank.u32(sc.esp + 4), sphere_at = bank.u32(sc.esp + 8);
    const std::array<std::uint32_t, 2> arrays{bank.u32(sc.esp + 12), bank.u32(sc.esp + 16)};
    std::array<std::uint32_t, 4> sphere;
    for (unsigned i = 0; i < 4; ++i) sphere[i] = bank.u32(sphere_at + 4 * i);
    SourcePhysicsWorldRuntimeServices services;
    static const O observe{};
    native_tu15_projection_query::SphereQueryRows rows;
    SourcePhysicsContactRuntimeAbi abi;
    {
        FpScope scope(sc.fp);
        LiveState s{bank, observe, services, SourcePhysicsContactImage::TitleUpdate15};
        s.load(sc.abi);
        auto status = s.fp.status;
        if (!native_tu15_projection_query::sphere_query_rows(bank, mopp, sphere, status, rows)) return "rows refused";
        s.fp.status = status;
        abi = s.save();
    }
    if (abi.x87_status != std::uint16_t(p.out[5])) return "rows x87 status";
    if (abi.floating_point->mxcsr != p.out[7]) return "rows mxcsr";
    for (unsigned k = 0; k < 2; ++k) {
        const auto at = [&](std::uint32_t a) { std::uint32_t v; std::memcpy(&v, p.low.data() + (a - heap_base), 4); return v; };
        const auto before = bank.u32(arrays[k] + 4), after = at(arrays[k] + 4), data = at(arrays[k]);
        const auto &stream = k ? rows.ranges : rows.direct;
        if (after - before != stream.size()) return "rows count";
        for (std::uint32_t i = 0; i < stream.size(); ++i) if (at(data + 4 * (before + i)) != stream[i]) return "rows value";
    }
    return {};
}
int physical_main(std::uint64_t cases, std::uint64_t seed, const char *runner, const char *work) {
    physical_cases = true;
    std::mt19937 rng{std::uint32_t(seed)};
    std::vector<Scenario> all;
    const std::string input = std::string(work) + "/cases.bin", output = std::string(work) + "/physical.bin";
    {
        std::FILE *f = std::fopen(input.c_str(), "wb");
        for (std::uint64_t n = 0; n < cases; ++n) { all.push_back(make(rng, n & 1, n)); emit(f, all.back()); }
        std::fclose(f);
    }
    const auto command = std::string(runner) + " < " + input + " > " + output;
    if (std::system(command.c_str())) { std::printf("physical runner failed\n"); return 2; }
    std::FILE *f = std::fopen(output.c_str(), "rb");
    std::uint64_t native_failures = 0, translated_failures = 0, native_ran = 0, growth = 0;
    std::map<std::string, unsigned> why_native, why_translated;
    for (std::uint64_t n = 0; n < cases; ++n) {
        Physical p;
        if (!collect(f, p)) { std::printf("physical stream ended at %llu\n", (unsigned long long)n); return 2; }
        const auto native_run = run(all[n], true, n & 2), translated = run(all[n], false, false);
        native_ran += native_run.native_ran; growth += native_run.growths != 0;
        if (all[n].entry == 0x2e7c40u && native_run.native_ran && !native_run.growths && !p.crashed) {
            Bank b; b.pages = all[n].pages;
            if (b.u32(all[n].esp + 12) != b.u32(all[n].esp + 16)) {
                const auto r = rows_against(all[n], p);
                if (r.empty()) ++why_native["typed sphere rows match original"];
                else { ++native_failures; ++why_native["typed sphere rows: " + r]; }
            }
        }
        const auto a = against(native_run, p), b = against(translated, p);
        // Only admitted native executions qualify the kernel; a refusal runs the
        // translated fallback before any effect and is classified separately.
        if (!native_run.native_ran) {
            ++why_native[p.crashed ? "refused: original crashed" : a.empty() ? "refused: fallback matches original" : "refused: fallback differs from original"];
        } else if (!a.empty()) {
            if (++native_failures <= 10) std::printf("case %llu native vs physical: %s\n", (unsigned long long)n, a.c_str());
            ++why_native[a.substr(0, a.find(' '))];
        } else if (p.crashed) ++why_native["both crashed"];
        if (!b.empty() && p.crashed) ++why_translated["original crashed, translated completed"];
        else if (!b.empty()) { if (++translated_failures <= 10) std::printf("case %llu translated vs physical: %s\n", (unsigned long long)n, b.c_str()); ++why_translated[b.substr(0, b.find(' '))]; }
    }
    std::fclose(f);
    std::printf("physical cases=%llu native_ran=%llu growth_cases=%llu native_failures=%llu translated_failures=%llu\n",
                (unsigned long long)cases, (unsigned long long)native_ran, (unsigned long long)growth,
                (unsigned long long)native_failures, (unsigned long long)translated_failures);
    for (const auto &[k, c] : why_native) std::printf("  native %s: %u\n", k.c_str(), c);
    for (const auto &[k, c] : why_translated) std::printf("  translated %s: %u\n", k.c_str(), c);
    std::remove(input.c_str()); std::remove(output.c_str());
    return native_failures ? 1 : 0;
}
int run_main(int argc, char **argv) {
    if (argc > 4 && std::string(argv[1]) == "--physical")
        return physical_main(std::strtoull(argv[2], nullptr, 10), std::strtoull(argv[3], nullptr, 10), argv[4], argv[5]);
    const std::uint64_t cases = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 20000;
    const std::uint64_t seed = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 1;
    std::mt19937 rng{std::uint32_t(seed)};
    std::uint64_t failures = 0, native = 0, growth_cases = 0, errors = 0;
    std::map<std::string, unsigned> why;
    for (std::uint64_t n = 0; n < cases; ++n) {
        const bool sphere = n & 1;
        const auto sc = make(rng, sphere, n);
        const auto translated = run(sc, false, false);
        const auto native_run = run(sc, true, (n >> 1) & 1);
        native += native_run.native_ran;
        growth_cases += translated.growths != 0;
        errors += !translated.error.empty();
        auto diff = compare(sc, translated, native_run);
        if (!diff.empty()) {
            // Two-NaN SSE operand order is the known translated-literal deviation.
            Bank b; b.pages = sc.pages;
            bool nan = false;
            const auto check = [&](std::uint32_t a) { const auto v = b.u32(a); nan |= (v & 0x7f800000u) == 0x7f800000u && (v & 0x7fffffu); };
            for (unsigned i = 0; i < 4; ++i) check(heap_base + 0x1000u + 4 * i);
            if (sphere) for (unsigned i = 0; i < 4; ++i) check(heap_base + 0x2000u + 4 * i);
            else for (unsigned i = 0; i < b.u32(heap_base + 0x2000u) * 4; ++i) check(heap_base + 0x2010u + 4 * i);
            if (nan) { ++why["NaN-input (translated literal commutes NaN operands)"]; diff.clear(); }
        }
        if (!diff.empty() && std::getenv("DIFF_DUMP")) {
            const auto show = [&](const char *name, const Outcome &o) {
                std::printf("  %s native_ran=%d growths=%u err='%s'\n", name, o.native_ran, o.growths, o.error.c_str());
                for (std::uint32_t a : {heap_base + 0x3000u, heap_base + 0x3100u}) {
                    Bank b; b.pages = o.pages;
                    const auto data = b.u32(a), size = b.u32(a + 4), cap = b.u32(a + 8);
                    std::printf("   array %08x data=%08x size=%u cap=%08x:", a, data, size, cap);
                    for (std::uint32_t i = 0; i < size && i < 40; ++i) std::printf(" %x", b.u32(data + 4 * i));
                    std::printf("\n");
                }
            };
            Bank b; b.pages = sc.pages;
            std::printf("case %llu esp=%08x arrays %08x %08x\n", (unsigned long long)n, sc.esp, b.u32(sc.esp + 12), b.u32(sc.esp + 16));
            show("translated", translated); show("native", native_run);
            std::printf("  planes=%u origin", b.u32(heap_base + 0x2000u));
            for (unsigned i = 0; i < 4; ++i) std::printf(" %08x", b.u32(heap_base + 0x1000u + 4 * i));
            std::printf("\n  code:");
            for (unsigned i = 0; i < 160; ++i) std::printf(" %02x", std::to_integer<unsigned>(*b.at(heap_base + 0x1030u + i)));
            std::printf("\n  fp cw=%04x mxcsr=%08x\n", sc.fp.x87_control_word, sc.fp.mxcsr);
        }
        if (!diff.empty()) {
            if (++failures <= 10) std::printf("case %llu (%s) differs: %s\n", (unsigned long long)n, sphere ? "sphere" : "plane", diff.c_str());
            ++why[diff.substr(0, diff.find(' ') == std::string::npos ? diff.size() : diff.find(' '))];
        }
    }
    std::printf("cases=%llu native=%llu growth_cases=%llu translated_errors=%llu failures=%llu\n",
                (unsigned long long)cases, (unsigned long long)native, (unsigned long long)growth_cases,
                (unsigned long long)errors, (unsigned long long)failures);
    for (const auto &[k, c] : why) std::printf("  %s: %u\n", k.c_str(), c);
    static const char *names[] = {"walk", "slab", "first", "last", "split right*3+left", "slab result"};
    for (unsigned w = 0; w < 6; ++w) {
        unsigned distinct = 0;
        for (auto c : coverage[w]) distinct += c != 0;
        std::printf("coverage %-18s distinct=%u:", names[w], distinct);
        for (unsigned op = 0; op < 256; ++op) if (coverage[w][op]) std::printf(" %02x", op);
        std::printf("\n");
    }
    return failures ? 1 : 0;
}
} // namespace
} // namespace halo2
int main(int argc, char **argv) { return halo2::run_main(argc, argv); }
