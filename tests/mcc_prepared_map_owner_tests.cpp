#include "halo2/mcc_prepared_map_owner.hpp"
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
#include <string>

namespace {
namespace fs = std::filesystem;
using Json = nlohmann::json;
using namespace halo2;
std::string scalar_fixture;

void check(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
std::span<const std::byte> bytes(const std::string& value) { return std::as_bytes(std::span(value)); }
std::string hash(std::string_view value) {
    std::array<unsigned char, 32> out{}; unsigned size{};
    check(EVP_Digest(value.data(), value.size(), out.data(), &size, EVP_sha256(), nullptr) == 1 && size == out.size(), "SHA failure");
    constexpr char hex[] = "0123456789abcdef"; std::string result;
    for (const auto v : out) { result += hex[v >> 4]; result += hex[v & 15]; }
    return result;
}
void write(const fs::path& path, std::string_view data) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(data.data(), std::streamsize(data.size())); check(bool(file), "fixture write failed");
}
template<class F> void rejected(F&& call, const char* why) {
    bool failed{}; try { call(); } catch (const MccNativeMapOwnerError&) { failed = true; }
    check(failed, why);
}
struct Temporary {
    fs::path root;
    Temporary() {
        const auto* parent = std::getenv("HALO2_TEST_SCRATCH");
        check(parent && fs::is_directory(parent), "Run through tools/run_test.py owned scratch");
        root = fs::path(parent) / "prepared-map-owner";
        check(fs::create_directory(root), "No existing fixture reuse");
    }
    ~Temporary() { std::error_code ignored; fs::remove_all(root, ignored); }
};
struct Fixture {
    fs::path root;
    Json provenance{{"schema", "H2_MCC_TU15_PREPARATION_V1"}, {"external_installation_requirement", "Halo MCC only"},
        {"source_files", Json::object()}, {"native_shader_library", {{"maps", Json::object()}}}};
    Json conversion{{"profile", "H2_MCC_TU15_PREPARATION_V1"}, {"maps", Json::array()}};
    Json index;
    std::map<std::string, std::string> encoded, final_hashes, roles, bank_for, banks;
    explicit Fixture(fs::path path) : root(std::move(path)) {
        fs::create_directories(root / "halo2/h2_maps_win64_dx11");
        map("h2_maps_win64_dx11/lockout.map", "original stock map");
        map("h2_maps_win64_dx11/shared.map", "original resource map", "resources");
        bank("native-shaders/lockout", "lockout", true);
        bank_for["halo2/h2_maps_win64_dx11/lockout.map"] = "native-shaders/lockout";
    }
    void map(const std::string& name, std::string data, const std::string& role = "scenario") {
        provenance["source_files"][name] = {{"sha256", hash(data)}, {"bytes", data.size()}};
        const auto owner = "halo2/" + name; encoded[owner] = data; final_hashes[owner] = hash(data); roles[owner] = role;
        write(root / owner, data);
        if (fs::path(name).parent_path() == "h2_maps_win64_dx11")
            conversion["maps"].push_back({{"map", fs::path(name).filename().string()}, {"source_sha256", hash(data)},
                {"output_sha256", hash(data)}, {"role", role == "resources" ? "resources" : "multiplayer"}});
    }
    void bank(const std::string& directory, const std::string& name, bool stock = false) {
        const auto raw = Json{{"schema", "H2_PRESERVED_NATIVE_SHADERS_V1"}, {"map", name},
            {"source_sha256", std::string(64, 'a')}, {"fixture", directory}, {"shader_tags", Json::array()}}.dump() + '\n';
        banks[directory] = raw; write(root / directory / "library.json", raw);
        write(root / directory / "function-tables.bin", scalar_fixture);
        if (stock) provenance["native_shader_library"]["maps"][name] = hash(raw);
    }
    void save() {
        write(root / "provenance.json", provenance.dump() + '\n');
        write(root / "tag-conversion.json", conversion.dump() + '\n');
    }
    Json anchors() const {
        return {{"copied_source_inventory_sha256", hash(provenance.at("source_files").dump())},
            {"conversion_sha256", hash(conversion.dump())},
            {"ordered_output_chain_sha256", hash(Json::array({provenance.value("output_changes", Json::array()),
                provenance.value("graphics_corrections", Json::object()).value("changes", Json::array())}).dump())}};
    }
    void indexed() {
        index = {{"schema", "H2_MCC_NATIVE_MAP_OWNERS_V1"}, {"ownership_only", true}, {"maps", Json::array()},
            {"dependencies", Json::array()}, {"input_authority", anchors()}, {"stock_bank_directories", Json::array({"native-shaders/lockout"})}};
        for (const auto& [owner, data] : encoded) {
            const auto name = owner.substr(6); const bool stock = fs::path(name).parent_path() == "h2_maps_win64_dx11";
            Json row{{"map", owner}, {"input_bytes", data.size()}, {"input_sha256", final_hashes.at(owner)},
                {"source_sha256", provenance["source_files"][name]["sha256"]}, {"cache_role", roles.at(owner)},
                {"shared_directory", stock ? "halo2/h2_maps_win64_dx11" : "halo2/Mods/Shared Assets"},
                {"native_mode", "none"}, {"bank_directory", ""}, {"bank_namespace", ""}, {"bank_library_sha256", ""}, {"bank_owner_map", ""},
                {"compatibility_authority", {{"producer", "synthetic-test-only"}, {"profile", "fixture-v1"},
                    {"receipt_sha256", std::string(64, 'f')}, {"scope", stock ? "stock-conversion-owner" : "explicit-custom-owner"}}}};
            if (const auto bank = bank_for.find(owner); bank != bank_for.end()) {
                row["native_mode"] = "owned-bank"; row["bank_directory"] = bank->second;
                row["bank_namespace"] = Json::parse(banks.at(bank->second)).at("map");
                row["bank_library_sha256"] = hash(banks.at(bank->second)); row["bank_owner_map"] = owner;
            }
            fs::create_directories(root / row.at("shared_directory").get<std::string>());
            index["maps"].push_back(std::move(row));
        }
        publish();
    }
    void publish() {
        const auto raw = index.dump() + '\n';
        write(root / "native-shaders/map-owners.json", raw);
        provenance["native_shader_library"]["owner_bindings"] = {{"schema", "H2_MCC_NATIVE_MAP_OWNERS_V1"},
            {"path", "native-shaders/map-owners.json"}, {"bytes", raw.size()}, {"sha256", hash(raw)}};
        save();
    }
    std::optional<MccPreparedMapOwner> admit(const std::string& owner = "halo2/h2_maps_win64_dx11/lockout.map",
        bool explicit_root = false, const std::string& shared = "halo2/h2_maps_win64_dx11") {
        return admit_mcc_prepared_map_owner(root / owner, root / shared, bytes(encoded.at(owner)),
            explicit_root ? std::optional<fs::path>(root) : std::nullopt);
    }
};
}

int main(int argc, char** argv) try {
    check(argc == 2, "Supply the owned authentic shipped scalar fixture through the Python driver");
    const auto* scratch_parent = std::getenv("HALO2_TEST_SCRATCH");
    check(scratch_parent && fs::is_directory(scratch_parent), "Scalar input requires owned scratch");
    const auto scalar_path = fs::canonical(argv[1]), scratch = fs::canonical(scratch_parent);
    const auto scalar_relative = scalar_path.lexically_relative(scratch);
    check(!scalar_relative.empty() && !scalar_relative.is_absolute() && *scalar_relative.begin() != ".." &&
          !fs::is_symlink(argv[1]) && fs::file_size(scalar_path) == 28u * 1024u, "Scalar fixture escaped owned scratch");
    std::ifstream scalar_input(scalar_path, std::ios::binary);
    scalar_fixture.assign(std::istreambuf_iterator<char>(scalar_input), {});
    check(hash(scalar_fixture) == "1261274f49fbf8f8fd36e9a56f9b7a701a2bad1c5d84be168c69826a62484118", "Shipped scalar fixture fingerprint differs");
    Temporary temporary;
    {
        Fixture f(temporary.root / "legacy"); f.save();
        const auto a = f.admit(); check(a && a->binding && !a->indexed, "Legacy stock projection absent");
        check(a->binding->mode == MccNativeBankMode::Owned && a->binding->library_namespace == "lockout", "Wrong legacy bank");
        check(!a->binding->custom_authoring, "Stock owner acquired custom authoring");
        check(a->metadata.at("native-shaders/lockout/library.json") == f.banks.at("native-shaders/lockout"), "Legacy bank snapshot not owned");
        const auto resource = f.admit("halo2/h2_maps_win64_dx11/shared.map");
        check(resource->binding->role == MccCacheRole::Resources && resource->binding->mode == MccNativeBankMode::None, "Stock source role lost");
        f.map("Mods/A/Maps/lockout.map", "nested stock-looking map"); f.save();
        check(!f.admit("halo2/Mods/A/Maps/lockout.map"), "Nested map inferred stock context");
        const auto nested = f.admit("halo2/Mods/A/Maps/lockout.map", true);
        check(nested && !nested->binding, "Legacy nested map inherited whole stock bank");
        f.map("h2_maps_win64_dx11/needle.map", "non-policy-preimage bankless stock map"); f.save();
        check(f.admit("halo2/h2_maps_win64_dx11/needle.map")->binding->mode == MccNativeBankMode::None,
              "HUD-looking stock name bypassed compiled input fingerprint");
    }
    {
        Fixture f(temporary.root / "chain");
        const auto owner = std::string("halo2/h2_maps_win64_dx11/lockout.map");
        const auto intermediate = hash("conversion output");
        f.conversion["maps"][0]["output_sha256"] = intermediate;
        f.provenance["output_changes"] = Json::array({{{"path", owner}, {"donor_sha256", intermediate}, {"converted_sha256", hash("post output")}}});
        f.provenance["graphics_corrections"] = {{"changes", Json::array({{{"path", owner}, {"donor_sha256", hash("post output")}, {"converted_sha256", hash("long final encoded map")}}})}};
        f.encoded[owner] = "long final encoded map"; f.final_hashes[owner] = hash(f.encoded[owner]); f.save();
        check(f.admit()->binding->input_bytes == f.encoded[owner].size(), "Legacy final extent was copied from source");
        f.provenance["graphics_corrections"]["changes"][0]["donor_sha256"] = std::string(64, '0'); f.save();
        rejected([&] { f.admit(); }, "Stale graphics donor admitted");
    }
    {
        Fixture f(temporary.root / "indexed");
        const std::string a = "halo2/Mods/Pack A/Maps/lockout.map", b = "halo2/Mods/Pack B/Maps/lockout.map";
        f.map(a.substr(6), "custom A"); f.map(b.substr(6), "custom B");
        f.map("Mods/Resources/shared.map", "explicit mod resources", "resources");
        f.bank("native-shaders/maps/A", "lockout"); f.bank("native-shaders/maps/B", "lockout");
        f.bank_for[a] = "native-shaders/maps/A"; f.bank_for[b] = "native-shaders/maps/B"; f.indexed();
        // Normal gameplay has no preview-only explicit-root option. A nested
        // custom map uses the authenticated index under its shared cache root.
        for (auto& row : f.index["maps"])
            if (row["map"] == a) row["shared_directory"] = "halo2/h2_maps_win64_dx11";
        f.publish();
        const auto implicit_custom = f.admit(a);
        check(implicit_custom && implicit_custom->indexed &&
            implicit_custom->binding->bank_directory == f.root / "native-shaders/maps/A",
            "Normal gameplay lost the indexed custom owner");
        check(implicit_custom->binding->custom_authoring, "Custom authoring receipt lost its runtime domain");
        for (auto& row : f.index["maps"])
            if (row["map"] == a) row["shared_directory"] = "halo2/Mods/Shared Assets";
        f.publish();
        const auto aa = f.admit(a, true, "halo2/Mods/Shared Assets"), bb = f.admit(b, true, "halo2/Mods/Shared Assets");
        check(aa->indexed && aa->binding->bank_directory != bb->binding->bank_directory, "Indexed same-leaf owners alias");
        check(aa->metadata.at("native-shaders/maps/A/function-tables.bin") == scalar_fixture,
              "Custom bank did not retain authentic scalar snapshot");
        const auto resource = f.admit("halo2/Mods/Resources/shared.map", true, "halo2/Mods/Shared Assets");
        check(resource->binding->role == MccCacheRole::Resources, "Explicit mod resource role lost");
        const auto saved = aa->metadata.at("native-shaders/maps/A/library.json");
        write(f.root / "native-shaders/maps/A/library.json", "later filename replacement");
        write(f.root / a, "later map filename replacement");
        check(aa->metadata.at("native-shaders/maps/A/library.json") == saved && aa->binding->input_sha256 == hash("custom A"), "Admitted snapshot followed later filename");
        rejected([&] { f.admit(a, true, "halo2/Mods/Shared Assets"); }, "Changed bank admitted");
        write(f.root / "native-shaders/maps/A/library.json", saved);
        f.provenance["native_shader_library"]["owner_bindings"]["sha256"] = std::string(64, '0'); f.save();
        rejected([&] { f.admit(); }, "Wrong provenance index pointer admitted");
    }
    {
        Fixture f(temporary.root / "stale-index"); f.indexed();
        f.conversion["maps"][0]["output_sha256"] = hash("new producer output"); f.save();
        rejected([&] { f.admit(); }, "Stale index conversion anchor admitted");
        f.conversion["maps"][0]["output_sha256"] = hash("original stock map");
        f.provenance["native_shader_library"]["maps"]["lockout"] = std::string(64, '0'); f.save();
        rejected([&] { f.admit(); }, "Stale indexed stock bank mapping admitted");
    }
    {
        Fixture f(temporary.root / "indexed-shape");
        f.map("h2_maps_win64_dx11/bankless.map", "ordinary source-bound bankless scenario"); f.indexed();
        const auto original = f.index;
        const auto row = [&](const char* map) -> Json& {
            for (auto& item : f.index["maps"]) if (item.at("map") == map) return item;
            throw std::runtime_error("missing indexed fixture row");
        };
        row("halo2/h2_maps_win64_dx11/shared.map")["native_mode"] = "owned-bank"; f.publish();
        rejected([&] { f.admit(); }, "Stock resource invented whole bank");
        f.index = original; row("halo2/h2_maps_win64_dx11/lockout.map")["shared_directory"] = "halo2/Mods/Shared Assets"; f.publish();
        rejected([&] { f.admit(); }, "Stock row changed shared owner");
        f.index = original; row("halo2/h2_maps_win64_dx11/lockout.map")["compatibility_authority"]["scope"] = "explicit-custom-owner"; f.publish();
        rejected([&] { f.admit(); }, "Stock row changed authority scope");
        f.index = original; f.index["stock_bank_directories"] = Json::array(); f.publish();
        rejected([&] { f.admit(); }, "Index omitted provenance stock bank set");
        f.index = original; row("halo2/h2_maps_win64_dx11/bankless.map")["native_mode"] = "authenticated-hud-bootstrap"; f.publish();
        rejected([&] { f.admit(); }, "Bankless stock row invented HUD authority");
        f.index = original; f.publish();
        check(f.admit()->binding->mode == MccNativeBankMode::Owned, "Restored source authorities no longer admit");
    }
    {
        Fixture f(temporary.root / "scalar"); f.indexed();
        const auto& scalar = scalar_fixture;
        write(f.root / "native-shaders/function-tables.bin", scalar);
        f.index["dependencies"].push_back({{"kind", "scalar-function-tables"}, {"path", "native-shaders/function-tables.bin"}, {"bytes", scalar.size()}, {"sha256", hash(scalar)}}); f.publish();
        check(f.admit()->metadata.at("native-shaders/function-tables.bin") == scalar, "Scalar snapshot lost binary bytes");
        write(f.root / "native-shaders/lockout/function-tables.bin", "not authentic own-bank constants");
        rejected([&] { f.admit(); }, "Bad mandatory own-bank scalar admitted");
        write(f.root / "native-shaders/lockout/function-tables.bin", scalar);
        f.provenance["native_shader_library"]["owner_bindings"]["path"] = "native-shaders/../map-owners.json"; f.save();
        rejected([&] { f.admit(); }, "Pointer path alias admitted");
    }
    {
        Fixture f(temporary.root / "copied-only");
        f.map("Mods/Maps/shared.map", "scenario with a resource-looking filename"); f.indexed();
        for (auto& row : f.index["maps"]) if (row.at("map") == "halo2/Mods/Maps/shared.map") {
            row["compatibility_authority"]["scope"] = "copied-input-only";
            row["shared_directory"] = "halo2/h2_maps_win64_dx11";
        }
        f.publish();
        const auto a = f.admit("halo2/Mods/Maps/shared.map", true);
        check(a->binding->role == MccCacheRole::Scenario && a->binding->mode == MccNativeBankMode::None, "Copied-only scenario inherited a resource role");
        for (auto& row : f.index["maps"]) if (row.at("map") == "halo2/Mods/Maps/shared.map") row["cache_role"] = "resources";
        f.publish(); rejected([&] { f.admit(); }, "Copied-only input invented custom role");
    }
    {
        Fixture f(temporary.root / "paths"); f.save();
        const auto manifest = f.root / "native-shaders/lockout/library.json";
        const auto outside = temporary.root / "outside.json"; write(outside, f.banks.at("native-shaders/lockout"));
        fs::remove(manifest); fs::create_symlink(outside, manifest);
        rejected([&] { f.admit(); }, "Symlink metadata owner admitted");
        fs::remove(manifest); write(manifest, f.banks.at("native-shaders/lockout"));
        write(f.root / "provenance.json", "{\"schema\":\"first\",\"schema\":\"second\"}");
        rejected([&] { f.admit(); }, "Duplicate provenance key admitted");
    }
    {
        const auto root = temporary.root / "unprepared"; fs::create_directories(root / "halo2/h2_maps_win64_dx11");
        const auto map = root / "halo2/h2_maps_win64_dx11/shared.map"; write(map, "unprepared input");
        const std::string raw = "already read encoded input";
        check(!admit_mcc_prepared_map_owner(map, map.parent_path(), bytes(raw)), "Raw map inferred unowned preparation");
        rejected([&] { admit_mcc_prepared_map_owner(map, map.parent_path(), bytes(raw), root); }, "Explicit missing preparation admitted");
    }
    std::cout << "PASS prepared-owner synthetic groups: legacy stock/resources/nested, final chain, indexed custom snapshots, stale authorities, scalar, paths, raw mode\n";
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
