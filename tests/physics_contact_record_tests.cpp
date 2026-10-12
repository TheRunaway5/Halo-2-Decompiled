#include "halo2/physics_contact_record.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value, const std::string &context) {
    if (!value) throw std::runtime_error("Original contact construction differs: " + context);
}
std::uint32_t number(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
    check(s.size() % 2 == 0, "literal hex extent");
    std::vector<std::byte> result;
    for (std::size_t n = 0; n < s.size(); n += 2)
        result.push_back(std::byte(number(s.substr(n, 2))));
    return result;
}
struct Memory : halo2::SourcePhysicsContactMemory {
    std::map<std::uint32_t, std::vector<std::byte>> buffers;
    auto at(std::uint32_t address, std::size_t size) const {
        auto it = buffers.upper_bound(address);
        if (it == buffers.begin()) throw std::runtime_error("Actual source backing unavailable");
        --it;
        if (std::uint64_t(address - it->first) + size > it->second.size())
            throw std::runtime_error("Actual source backing unavailable");
        return it;
    }
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        const auto it = at(address, output.size());
        std::copy_n(it->second.begin() + (address - it->first), output.size(), output.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> input) override {
        const auto it = at(address, input.size());
        auto &raw = buffers.at(it->first);
        std::copy(input.begin(), input.end(), raw.begin() + (address - it->first));
    }
};
struct Case {
    std::string label;
    std::uint32_t entry, result;
    halo2::SourcePhysicsContactConstruction input;
    Memory before;
    std::map<std::uint32_t, std::vector<std::byte>> after;
    std::vector<halo2::SourcePhysicsContactRecordStore> stores;
};
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 2) throw std::runtime_error("Supply immutable original contact constructor fixture");
        if (!std::filesystem::exists(argv[1])) return 77;
        std::ifstream file(argv[1]); std::vector<Case> cases;
        for (std::string line; std::getline(file, line);) {
            std::istringstream stream(line); std::vector<std::string> r;
            for (std::string s; stream >> s;) r.push_back(s);
            if (r.empty()) continue;
            if (r[0] == "C") {
                check(r.size() == 16, "nine source stack arguments");
                Case c{}; c.label = r[1]; c.entry = number(r[2]); c.result = number(r[6]);
                c.input.row_address = number(r[3]); c.input.contact_index_word = number(r[4]);
                c.input.source_key = number(r[5]);
                for (unsigned n = 0; n < 9; ++n) c.input.stack_arguments[n] = number(r[7+n]);
                cases.push_back(std::move(c));
            } else {
                check(!cases.empty(), "owning call before literal buffer/store");
                if (r[0] == "M") {
                    check(r.size() == 3, "input buffer extent");
                    check(cases.back().before.buffers.emplace(number(r[1]), bytes(r[2])).second,
                          "unique source backing address");
                } else if (r[0] == "A") {
                    check(r.size() == 3, "output buffer extent");
                    check(cases.back().after.emplace(number(r[1]), bytes(r[2])).second,
                          "unique source output address");
                } else if (r[0] == "W") {
                    check(r.size() == 5, "ordered source store extent");
                    cases.back().stores.push_back({number(r[1]), number(r[2]), bytes(r[3]), bytes(r[4])});
                } else throw std::runtime_error("Unknown contact fixture record");
            }
        }
        check(cases.size() == 2, "complete1.1/1.5 original case roster");
        unsigned stores = 0, full_bytes = 0;
        for (const auto &c : cases) {
            const auto image = c.label == "map_pack_1_1" ? halo2::SourcePhysicsContactImage::MapPack11 :
                halo2::SourcePhysicsContactImage::TitleUpdate15;
            check((c.label == "map_pack_1_1" && c.entry == 0x1d2df0) ||
                  (c.label == "title_update_1_5" && c.entry == 0x1d76b0), "actual original entry");
            check(c.before.buffers.size() == 5 && c.after.size() == 5 && c.stores.size() == 14,
                  "full source row/body/property and owning store roster");
            auto memory = c.before;
            check(memory.buffers.at(c.input.row_address).size() == 72, "whole original contact row");
            std::vector<halo2::SourcePhysicsContactRecordStore> writes;
            const auto result = halo2::source_construct_physics_contact_record(memory, c.input, image,
                [&](const auto &write) { writes.push_back(write); });
            check(result == c.result, c.label + " original defined EAX");
            check(memory.buffers == c.after, c.label + " complete buffers including untouched bytes");
            check(writes == c.stores, c.label + " complete owning store order/sites/before/written bytes");
            for (const auto &[address, raw] : memory.buffers) { (void)address; full_bytes += unsigned(raw.size()); }
            stores += unsigned(writes.size());

            // These are interface availability checks, not new original contact
            // fixtures or claims that the class3/4 source branch was executed.
            for (unsigned kind : {3u, 4u}) {
                auto unavailable = c.input;
                unavailable.stack_arguments[0] = (kind << 29) | (c.input.stack_arguments[0] & 0xffffu);
                auto retained = c.before; bool rejected = false;
                try { halo2::source_construct_physics_contact_record(retained, unavailable, image); }
                catch (const std::runtime_error &error) {
                    rejected = std::string(error.what()).find("lookup owner unavailable") != std::string::npos;
                }
                check(rejected, "unmeasured encoded lookup cannot default success");
            }
            auto missing = c.before;
            missing.buffers.erase(c.input.stack_arguments[2]);
            bool rejected = false;
            try { halo2::source_construct_physics_contact_record(missing, c.input, image); }
            catch (const std::runtime_error &) { rejected = true; }
            check(rejected, "missing actual counterpart body cannot default to an empty property set");
        }
        std::cout << cases.size() << " original contact constructors matched " << full_bytes
                  << " complete row/body/property bytes and " << stores << " ordered stores\n";
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
