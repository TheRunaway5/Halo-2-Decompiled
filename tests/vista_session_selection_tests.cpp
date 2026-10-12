#include "halo2/vista_session_selection.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

using namespace halo2;
using Json = nlohmann::json;
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void put(std::span<std::byte> bank, unsigned offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bank[offset + i] = std::byte(value >> (i * 8));
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char chars[] = "0123456789abcdef";
    std::string result;
    for (const auto byte : bytes) {
        const auto value = std::to_integer<unsigned>(byte);
        result += chars[value >> 4]; result += chars[value & 15];
    }
    return result;
}
std::string digest(std::span<const std::byte> bank) {
    std::array<std::byte, SHA256_DIGEST_LENGTH> hash{};
    SHA256(reinterpret_cast<const unsigned char*>(bank.data()), bank.size(),
           reinterpret_cast<unsigned char*>(hash.data()));
    return hex(hash);
}
auto initial(const Json& q) {
    std::array<std::byte, vista_session_source_size + 32> storage;
    const auto seed = q.at("seed").get<unsigned>();
    for (unsigned i = 0; i < vista_session_source_size; ++i)
        storage[i] = std::byte((i * 37 + seed * 13 + (i >> 8) * 17) & 255);
    std::fill(storage.begin() + vista_session_source_size, storage.end(), std::byte{0xa5});
    put(storage, 0x73a0, q.at("state")); put(storage, 0x71c0, 7);
    put(storage, 0x74, q.at("host").get<unsigned>() ? 7 : 8);
    put(storage, 0x4c60, q.at("revision"));
    return storage;
}
std::u16string path(const Json& q) {
    std::u16string result;
    const auto seed = q.at("seed").get<unsigned>();
    for (unsigned i = 0; i < q.at("length").get<unsigned>(); ++i)
        result += char16_t(((i * 19 + seed * 13) % 0xfffe) + 1);
    const auto nul = q.at("nul").get<int>();
    if (nul >= 0) result.at(nul) = u'\0';
    return result;
}
auto variant(const Json& q) {
    std::array<std::byte, 0x130> result;
    const auto seed = q.at("seed").get<unsigned>();
    for (unsigned i = 0; i < result.size(); ++i) result[i] = std::byte((i * 23 + seed * 17) & 255);
    put(result, 0x44, q.at("variant_kind"));
    return result;
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "original fixture argument required");
        std::ifstream file(argv[1]); Json records; file >> records;
        unsigned checked{};
        for (const auto& record : records) {
            const auto& q = record.at("input");
            auto storage = initial(q);
            const auto bank = std::span(storage).first(vista_session_source_size);
            require(digest(bank) == record.at("before").get<std::string>(), "native initial bank differs from original input");
            const auto selected_path = path(q);
            const auto selected_variant = variant(q);
            Json returns = Json::array();
            for (unsigned repeat = 0; repeat < q.at("repeat").get<unsigned>(); ++repeat) {
                if (q.at("op") == "map")
                    returns.push_back(unsigned(vista_session_select_map(bank, q.at("type"), q.at("map_id"), selected_path)));
                else
                    returns.push_back(unsigned(vista_session_select_variant(bank,
                        q.at("null_variant").get<unsigned>() ? nullptr : &selected_variant)));
            }
            const auto selected_hex = hex(bank.subspan(0x4c60, 0x4f6c - 0x4c60));
            if (digest(bank) != record.at("after").get<std::string>() || returns != record.at("returned") ||
                selected_hex != record.at("selected_hex").get<std::string>()) {
                std::cerr << "case=" << checked << " input=" << q.dump() << "\nexpected="
                          << record.dump() << "\nactual=" << digest(bank) << " returned="
                          << returns.dump() << " selected_hex=" << selected_hex << '\n';
                throw std::runtime_error("original Vista local selected-parameter setter mismatch");
            }
            require(std::all_of(storage.begin() + vista_session_source_size, storage.end(),
                [](auto byte) { return byte == std::byte{0xa5}; }), "selection setter overwrote source bank suffix");
            ++checked;
        }
        require(checked == 682, "complete original local selection corpus");

        // Execute the unchanged615EA9 corpus, including the distinct original
        // current-session/parameter-session gates and selected-map branches.
        const auto protocol_fixture=std::filesystem::path(argv[1]).parent_path()/
            "vista_session_frontend_protocol_original.json";
        std::ifstream protocol_file(protocol_fixture);Json protocol_records;protocol_file>>protocol_records;
        unsigned protocol_checked{};
        for(const auto& record:protocol_records) {
            const auto& p=record.at("input");
            std::array<std::byte,vista_session_source_size> bank{};
            for(const auto& [field,key]:std::array<std::pair<unsigned,const char*>,7>{{
                {0x73a0,"state"},{0x1c,"session_class"},{0x4c90,"subtype"},
                {0x4cb8,"campaign_id"},{0x4dd8,"map_id"},{0x4cbc,"path_word"},{0x4e80,"engine"}}})
                put(bank,field,std::uint32_t(p.at(key).get<std::int64_t>()));
            const auto before=bank;
            const auto result=vista_session_frontend_protocol(bank,p.at("enabled").get<unsigned>()!=0,
                p.at("lifecycle_kind").get<std::int32_t>());
            if(result!=record.at("returned").get<std::int32_t>()) {
                std::cerr<<"protocol input="<<p.dump()<<" actual="<<result<<'\n';
                throw std::runtime_error("Original Vista frontend protocol differs");
            }
            require(bank==before,"Frontend protocol query mutated its session");
            ++protocol_checked;
        }
        require(protocol_checked==1549,"Complete original frontend protocol corpus");
        // Regression: run the actual selected-parameter producers. The cache
        // type2 previously became a campaign ID and selected paired mode2.
        std::array<std::byte,vista_session_source_size> selected{};
        put(selected,0x73a0,5);put(selected,0x1c,1);
        std::array<std::byte,0x130> multiplayer_variant{};put(multiplayer_variant,0x44,2);
        require(vista_session_select_map(selected,0xffffffffu,17,u"levels\\multi\\lockout\\lockout") &&
            vista_session_select_variant(selected,&multiplayer_variant) &&
            vista_session_frontend_protocol(selected,true,1)==3,
            "Original multiplayer map producer did not derive protocol3");
        require(vista_session_select_map(selected,2,17,u"levels\\multi\\lockout\\lockout") &&
            vista_session_frontend_protocol(selected,true,1)==2,
            "Campaign ID2 no longer selects the original paired protocol2");
        bool protocol_truncated{};
        try{(void)vista_session_frontend_protocol(std::span(selected).first(selected.size()-1),true,1);}
        catch(const FormatError&){protocol_truncated=true;}
        require(protocol_truncated && vista_session_frontend_protocol({},false,1)==-1,
            "Protocol owner guards do not retain the original disabled query");

        // Remote transport is deliberately not represented by this local binding.
        // Every rejection must retain the entire input, including selected fields.
        unsigned rejected{};
        auto q = records.front().at("input");
        for (const auto state : {3u, 4u}) for (const auto host : {0u, 1u}) {
            q["state"] = state; q["host"] = host;
            for (const bool map : {false, true}) {
                auto storage = initial(q); const auto before = storage;
                const auto selected_variant = variant(q);
                bool threw{};
                try {
                    if (map) (void)vista_session_select_map(storage, 2, 17, u"levels\\multi\\lockout\\lockout");
                    else (void)vista_session_select_variant(storage, &selected_variant);
                } catch (const FormatError&) { threw = true; }
                require(threw && storage == before, "unbound remote transport mutated the local bank");
                ++rejected;
            }
        }
        for (const auto size : {std::size_t{0}, vista_session_source_size - 1}) {
            auto storage = initial(q); const auto before = storage;
            for (const bool map : {false, true}) {
                bool threw{};
                try {
                    const auto bank = std::span(storage).first(size);
                    if (map) (void)vista_session_select_map(bank, 2, 17, u"map");
                    else (void)vista_session_select_variant(bank, nullptr);
                } catch (const FormatError&) { threw = true; }
                require(threw && storage == before, "truncated selected-parameter owner mutated storage");
                ++rejected;
            }
        }
        std::cout << "Vista local map/variant setters original cases=" << checked
                  << " frontend protocol cases=" << protocol_checked
                  << " rejection guards=" << rejected << " PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
