#include "halo2/mcc_native_map_owner.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
using Json = nlohmann::json;
using namespace halo2;

void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::span<const std::byte> bytes(const std::string& value) { return std::as_bytes(std::span(value)); }
std::string sha(const std::string& value) {
    std::array<unsigned char, 32> digest{}; unsigned size{};
    require(EVP_Digest(value.data(), value.size(), digest.data(), &size, EVP_sha256(), nullptr) == 1 && size == 32,
            "fixture SHA failed");
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (const auto v : digest) { result += hex[v >> 4]; result += hex[v & 15]; }
    return result;
}
void write(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(value.data(), std::streamsize(value.size()));
    require(bool(stream), "fixture write failed");
}
template<class Function> void rejected(Function&& function, const char* message) {
    bool failed{};
    try { function(); } catch (const MccNativeMapOwnerError&) { failed = true; }
    require(failed, message);
}

struct OwnedTemporaryDirectory {
    fs::path path;
    OwnedTemporaryDirectory() {
        const char* scratch = std::getenv("HALO2_TEST_SCRATCH");
        require(scratch && fs::is_directory(scratch), "Run via tools/run_test.py owned scratch");
        path = fs::path(scratch) / "mcc-map-owner-contract";
        require(fs::create_directory(path), "Fixture directory already exists; no reuse");
    }
    ~OwnedTemporaryDirectory() { std::error_code ignored; fs::remove_all(path, ignored); }
};

struct Fixture {
    fs::path root;
    Json index;
    std::map<std::string, std::string> encoded_maps, metadata;
    explicit Fixture(fs::path directory) : root(std::move(directory)) {
        fs::create_directories(root / "halo2/h2_maps_win64_dx11");
        fs::create_directories(root / "halo2/Mods/Shared Assets");
        index = {{"schema", "H2_MCC_NATIVE_MAP_OWNERS_V1"}, {"ownership_only", true},
                 {"maps", Json::array()}, {"dependencies", Json::array()},
                 {"stock_bank_directories", Json::array()},
                 {"input_authority", {{"copied_source_inventory_sha256", std::string(64, '1')},
                     {"conversion_sha256", std::string(64, '2')}, {"ordered_output_chain_sha256", std::string(64, '3')}}}};
    }
    Json bank(std::string directory, std::string identity, std::string marker) {
        // Retained original Xbox provenance deliberately differs from MCC
        // input hashes. The resolver must never compare those source roles.
        const auto raw = Json{{"schema", "H2_PRESERVED_NATIVE_SHADERS_V1"}, {"map", identity},
                              {"source_sha256", std::string(64, 'a')}, {"fixture_owner", marker}}.dump();
        const auto path = directory + "/library.json";
        metadata[path] = raw;
        write(root / path, raw);
        return {{"bank_directory", directory}, {"bank_namespace", identity},
                {"bank_library_sha256", sha(raw)}, {"canonical_sha256", sha(raw)}};
    }
    void map(std::string owner, std::string snapshot, const Json* bank = nullptr, bool stock = false) {
        encoded_maps[owner] = snapshot;
        write(root / owner, snapshot);
        Json row{{"map", owner}, {"input_bytes", snapshot.size()}, {"input_sha256", sha(snapshot)},
                 {"source_sha256", sha("original copied MCC source: " + owner)}, {"cache_role", "scenario"},
                 {"shared_directory", stock ? "halo2/h2_maps_win64_dx11" : "halo2/Mods/Shared Assets"},
                 {"native_mode", bank ? "owned-bank" : "none"}, {"bank_directory", ""},
                 {"bank_namespace", ""}, {"bank_library_sha256", ""}, {"bank_owner_map", ""},
                 {"compatibility_authority", {{"producer", "fixture-producer"}, {"profile", "fixture-owner-v1"},
                     {"receipt_sha256", std::string(64, '4')}, {"scope", stock ? "stock-conversion-owner" : "explicit-custom-owner"}}}};
        if (bank) {
            for (const auto* key : {"bank_directory", "bank_namespace", "bank_library_sha256"}) row[key] = bank->at(key);
            row["bank_owner_map"] = owner;
            if (stock) index["stock_bank_directories"].push_back(bank->at("bank_directory"));
        }
        index["maps"].push_back(row);
    }
    std::optional<MccNativeMapBinding> resolve(const std::string& owner, const std::string* snapshot = nullptr,
                                              const fs::path* shared_override = nullptr) const {
        const auto raw = index.dump();
        const auto expected = sha(raw);
        std::vector<MccNativeMetadataSnapshot> admitted;
        for (const auto& [name, value] : metadata) admitted.push_back({name, bytes(value)});
        const auto found = std::find_if(index.at("maps").begin(), index.at("maps").end(), [&](const auto& row) {
            return row.at("map") == owner;
        });
        const auto shared = found == index.at("maps").end() ? root / "halo2/Mods/Shared Assets" :
                            root / found->at("shared_directory").template get<std::string>();
        const auto& data = snapshot ? *snapshot : encoded_maps.at(owner);
        return resolve_mcc_native_map_binding({root, raw, expected}, root / owner,
                                             shared_override ? *shared_override : shared, bytes(data), admitted);
    }
};

void contract_tests(const fs::path& scratch) {
    Fixture f(scratch / "positive");
    const std::string stock = "halo2/h2_maps_win64_dx11/lockout.map";
    const std::string a = "halo2/Mods/A/Maps/lockout.map", b = "halo2/Mods/B/Maps/lockout.map";
    const auto stock_bank = f.bank("native-shaders/lockout", "lockout", "stock");
    const auto a_bank = f.bank("native-shaders/maps/halo2/Mods/A/Maps/lockout", "lockout", "A");
    const auto b_bank = f.bank("native-shaders/maps/halo2/Mods/B/Maps/lockout", "lockout", "B");
    f.map(stock, "final stock encoded map", &stock_bank, true);
    f.map(a, "final A encoded map", &a_bank);
    f.map(b, "final B encoded map", &b_bank);
    const auto stock_result = f.resolve(stock), a_result = f.resolve(a), b_result = f.resolve(b);
    require(stock_result && a_result && b_result && stock_result->bank_directory != a_result->bank_directory &&
            a_result->bank_directory != b_result->bank_directory && stock_result->library_sha256 != a_result->library_sha256 &&
            a_result->library_sha256 != b_result->library_sha256 && a_result->library_namespace == "lockout" &&
            b_result->library_namespace == "lockout", "same-leaf owners selected the wrong bank");
    require(a_result->source_sha256 != a_result->input_sha256 && a_result->input_sha256 == sha(f.encoded_maps.at(a)),
            "copied source/final encoded identities were conflated");
    // Ordinary filename replacement never replaces an already admitted snapshot.
    write(f.root / a, "later unadmitted filename replacement");
    write(f.root / "native-shaders/maps/halo2/Mods/A/Maps/lockout/library.json", "later metadata filename replacement");
    require(f.resolve(a)->library_sha256 == a_result->library_sha256, "path replacement changed admitted bank snapshot");
    const std::string altered = "changed admitted bytes";
    rejected([&] { f.resolve(a, &altered); }, "changed admitted map snapshot passed");
    const std::string unknown = "halo2/Mods/C/Maps/lockout.map";
    write(f.root / unknown, "unbound same-leaf file");
    const std::string unbound = "unbound same-leaf file";
    require(!f.resolve(unknown, &unbound), "missing exact row inherited stock bank");
    f.map("halo2/Mods/Scenario/shared.map", "scenario named shared");
    require(f.resolve("halo2/Mods/Scenario/shared.map")->role == MccCacheRole::Scenario,
            "resource role inferred from basename");
    f.map("halo2/Mods/Resources/custom.map", "explicit resources");
    f.index["maps"].back()["cache_role"] = "resources";
    require(f.resolve("halo2/Mods/Resources/custom.map")->role == MccCacheRole::Resources,
            "explicit resource role lost");
    // Values own their storage beyond metadata/index lifetimes.
    const auto saved = *a_result;
    require(saved.map_relative == a && saved.input_bytes == f.encoded_maps.at(a).size(), "binding lost local string ownership");

    Fixture malformed(scratch / "negative");
    const auto bank = malformed.bank("native-shaders/maps/A", "custom", "A");
    malformed.map(a, "A snapshot", &bank);
    auto original = malformed.index;
    malformed.index["maps"].push_back(malformed.index["maps"][0]);
    rejected([&] { malformed.resolve(a); }, "duplicate map owner passed");
    malformed.index = original;
    malformed.index["maps"][0]["bank_owner_map"] = b;
    rejected([&] { malformed.resolve(a); }, "other map bank owner passed");
    malformed.index = original;
    malformed.map(b, "B snapshot", &bank);
    rejected([&] { malformed.resolve(a); }, "whole bank aliased two owners");
    malformed.index = original;
    const auto before_metadata = malformed.metadata;
    malformed.metadata["native-shaders/maps/A/library.json"] = Json{{"schema", "H2_PRESERVED_NATIVE_SHADERS_V1"}, {"map", "wrong"}}.dump();
    rejected([&] { malformed.resolve(a); }, "changed bank metadata passed");
    malformed.metadata = before_metadata;
    const auto wrong_shared = malformed.root / "halo2/h2_maps_win64_dx11";
    rejected([&] { malformed.resolve(a, nullptr, &wrong_shared); }, "wrong explicit shared root passed");
    const auto raw = original.dump();
    const std::string wrong_sha(64, 'f');
    rejected([&] { resolve_mcc_native_map_binding({malformed.root, raw, wrong_sha}, malformed.root / a,
        malformed.root / "halo2/Mods/Shared Assets", bytes(malformed.encoded_maps.at(a)), {}); }, "untrusted index hash passed");
    const std::string duplicate = "{\"schema\":\"first\",\"schema\":\"second\"}";
    const auto duplicate_sha = sha(duplicate);
    rejected([&] { resolve_mcc_native_map_binding({malformed.root, duplicate, duplicate_sha}, malformed.root / a,
        malformed.root / "halo2/Mods/Shared Assets", bytes(malformed.encoded_maps.at(a)), {}); }, "duplicate JSON key passed");

    Fixture paths(scratch / "paths");
    const std::string exact = "halo2/Mods/My Pack/Maps/Lockout.map";
    paths.map(exact, "case-preserved snapshot");
    require(paths.resolve(exact).has_value(), "spaces/exact case did not roundtrip");
    const std::string lower = "halo2/Mods/My Pack/Maps/lockout.map", lower_data = "other lowercase map";
    write(paths.root / lower, lower_data);
    // Exact directory spelling is enforced even on a case-insensitive host.
    if (!fs::equivalent(paths.root / exact, paths.root / lower))
        require(!paths.resolve(lower, &lower_data), "casefolded map inherited another owner");
    const auto path_raw = paths.index.dump(), path_sha = sha(path_raw);
    rejected([&] { resolve_mcc_native_map_binding({paths.root, path_raw, path_sha},
        paths.root / "halo2/Mods/../Mods/My Pack/Maps/Lockout.map", paths.root / "halo2/Mods/Shared Assets",
        bytes(paths.encoded_maps.at(exact)), {}); }, "traversing path passed");
    write(scratch / "outside.map", "outside snapshot");
    rejected([&] { resolve_mcc_native_map_binding({paths.root, path_raw, path_sha}, scratch / "outside.map",
        paths.root / "halo2/Mods/Shared Assets", bytes(paths.encoded_maps.at(exact)), {}); }, "outside map escaped root");
    // Symlink tests use only this test's owned temporary tree.
    fs::create_directories(scratch / "outside");
    write(scratch / "outside/linked.map", "outside data");
    std::error_code symlink_error;
    fs::create_directory_symlink(scratch / "outside", paths.root / "halo2/Linked", symlink_error);
    require(!symlink_error, "owned symlink fixture unavailable");
    rejected([&] { resolve_mcc_native_map_binding({paths.root, path_raw, path_sha}, paths.root / "halo2/Linked/linked.map",
        paths.root / "halo2/Mods/Shared Assets", bytes(paths.encoded_maps.at(exact)), {}); }, "symlink component passed");
    fs::remove(paths.root / exact);
    fs::create_symlink(scratch / "outside/linked.map", paths.root / exact);
    rejected([&] { paths.resolve(exact); }, "symlink map passed");

    Fixture hud(scratch / "hud");
    const auto cairo = hud.bank("native-shaders/01b_spacestation", "01b_spacestation", "common templates");
    const std::string needle = "halo2/h2_maps_win64_dx11/needle.map";
    hud.map(needle, "authenticated HUD encoded fixture", nullptr, true);
    auto& row = hud.index["maps"][0];
    row["native_mode"] = "authenticated-hud-bootstrap";
    row["bootstrap_policy_sha256"] = std::string(64, '5');
    row["bootstrap_source_canonical_sha256"] = cairo.at("canonical_sha256");
    row["bootstrap_dependency"] = cairo.at("bank_directory");
    Json dependency = cairo;
    dependency["kind"] = "authenticated-common-templates";
    dependency["authority_sha256"] = std::string(64, '5');
    hud.index["dependencies"].push_back(dependency);
    require(!fs::exists(hud.root / "halo2/h2_maps_win64_dx11/01b_spacestation.map"), "unrelated gameplay fixture appeared");
    const auto result = hud.resolve(needle);
    require(result && result->mode == MccNativeBankMode::AuthenticatedHudBootstrap && result->bank_directory.empty() &&
            result->bootstrap_dependency == hud.root / "native-shaders/01b_spacestation", "HUD dependency became whole-bank owner");
    hud.index["dependencies"][0]["canonical_sha256"] = std::string(64, '6');
    rejected([&] { hud.resolve(needle); }, "modified common dependency passed");
}

std::string unhex(const std::string& text) {
    require(text.size() % 2 == 0 && text.size() <= 2 * 1024 * 1024, "fixture hex extent invalid");
    const auto digit = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        throw std::runtime_error("fixture has invalid hex digit");
    };
    std::string result;
    result.reserve(text.size() / 2);
    for (std::size_t i = 0; i < text.size(); i += 2) result += char((digit(text[i]) << 4) | digit(text[i + 1]));
    return result;
}

void producer_fixture(const fs::path& fixture_path) {
    const char* scratch = std::getenv("HALO2_TEST_SCRATCH");
    require(scratch && fs::is_directory(scratch), "Producer bridge requires owned test scratch");
    const auto owner = fs::canonical(scratch);
    const auto within = [&](const fs::path& path) {
        const auto absolute = fs::canonical(path);
        const auto relative = absolute.lexically_relative(owner);
        require(!relative.empty() && !relative.is_absolute() && *relative.begin() != "..", "producer fixture escaped scratch");
        return absolute;
    };
    const auto fixture = within(fixture_path);
    require(!fs::is_symlink(fixture_path) && fs::file_size(fixture) <= 1024 * 1024, "producer fixture metadata bound/type invalid");
    std::ifstream stream(fixture, std::ios::binary);
    const auto envelope = Json::parse(stream);
    require(envelope.at("schema") == "H2_MCC_NATIVE_MAP_OWNER_CROSS_LANGUAGE_FIXTURE_V1", "producer fixture schema differs");
    const auto root = within(envelope.at("cache_root").get<std::string>());
    const auto index = envelope.at("encoded_index").get<std::string>();
    const auto expected_sha = envelope.at("expected_index_sha256").get<std::string>();
    require(envelope.at("producer_controls").at("stale_graphics_donor_rejected") == true &&
            envelope.at("producer_controls").at("wrong_final_output_rejected") == true,
            "actual Python producer controls did not execute");
    std::map<std::string, std::string> admitted_metadata;
    for (const auto& row : envelope.at("metadata"))
        require(admitted_metadata.emplace(row.at("path").get<std::string>(), unhex(row.at("bytes_hex"))).second,
                "duplicate exported metadata snapshot");
    // The exported snapshots above remain the consumed authority even when the
    // corresponding ordinary filesystem filename is replaced after admission.
    const auto& replacement = envelope.at("replacement_after_snapshot");
    const auto replacement_map = replacement.at("map").get<std::string>();
    const auto replacement_library = replacement.at("library").get<std::string>();
    const auto& declared_owners = envelope.at("expected_distinct_bank_owners");
    const auto& declared_cases = envelope.at("cases");
    require(std::any_of(declared_owners.begin(), declared_owners.end(), [&](const auto& value) { return value == replacement_map; }) &&
            std::any_of(declared_cases.begin(), declared_cases.end(), [&](const auto& value) {
                return value.at("map") == replacement_map && value.contains("expected") && !value.at("expected").is_null();
            }) && admitted_metadata.contains(replacement_library), "replacement path lacks its declared fixture owner");
    const auto replacement_target = [&](const std::string& name) {
        const fs::path relative(name);
        require(!name.empty() && name.find_first_of("\\:\0", 0, 3) == name.npos && !relative.is_absolute() &&
                !relative.has_root_path() && relative.generic_string() == name &&
                relative.lexically_normal().generic_string() == name, "replacement path is not exact relative metadata");
        auto path = root;
        for (const auto& component : relative) {
            require(!component.empty() && component != "." && component != "..", "replacement path has aliased components");
            path /= component;
            require(!fs::is_symlink(fs::symlink_status(path)), "replacement path has a symlink component");
        }
        const auto canonical = fs::canonical(path);
        const auto descendant = canonical.lexically_relative(root);
        require(!descendant.empty() && !descendant.is_absolute() && *descendant.begin() != ".." &&
                fs::is_regular_file(canonical), "replacement target escaped private cache root");
        return canonical;
    };
    // Validate both targets before mutating either owned fixture filename.
    const auto map_target = replacement_target(replacement_map);
    const auto library_target = replacement_target(replacement_library);
    write(map_target, "unadmitted later map filename replacement");
    write(library_target, "unadmitted later metadata filename replacement");
    unsigned positive{}, negative{}, unbound{};
    std::map<std::string, fs::path> selected_banks;
    for (const auto& test : envelope.at("cases")) {
        auto metadata = admitted_metadata;
        if (test.contains("metadata_override")) {
            const auto& override = test.at("metadata_override");
            metadata.at(override.at("path").get<std::string>()) = unhex(override.at("bytes_hex"));
        }
        std::vector<MccNativeMetadataSnapshot> snapshots;
        for (const auto& [name, value] : metadata) snapshots.push_back({name, bytes(value)});
        const auto map_relative = test.at("map").get<std::string>();
        const auto snapshot = unhex(test.at("snapshot_hex"));
        const auto map_path = root / test.value("path_override", map_relative);
        const auto shared = root / test.at("shared_directory").get<std::string>();
        const auto index_sha = test.value("index_sha_override", expected_sha);
        const auto resolve = [&] {
            return resolve_mcc_native_map_binding({root, index, index_sha}, map_path, shared, bytes(snapshot), snapshots);
        };
        if (test.value("expect_error", false)) {
            rejected(resolve, ("producer negative control passed: " + test.at("name").get<std::string>()).c_str());
            ++negative;
            continue;
        }
        const auto result = resolve();
        if (test.at("expected").is_null()) {
            require(!result, "Python unbound same-leaf map inherited a C++ owner");
            ++unbound;
            continue;
        }
        require(result.has_value(), "Python-produced exact owner became unbound in C++");
        const auto& expected = test.at("expected");
        require(result->map_relative == map_relative && result->source_sha256 == expected.at("source_sha256").get<std::string>() &&
                result->input_sha256 == expected.at("input_sha256").get<std::string>() && result->input_bytes == expected.at("input_bytes") &&
                result->shared_directory == root / expected.at("shared_directory").get<std::string>(),
                "Python/C++ source, final encoded extent or shared owner mismatch");
        const auto expected_role = expected.at("cache_role") == "resources" ? MccCacheRole::Resources : MccCacheRole::Scenario;
        require(result->role == expected_role, "Python/C++ explicit cache role mismatch");
        if (expected.at("native_mode") == "owned-bank") {
            require(result->mode == MccNativeBankMode::Owned &&
                    result->bank_directory == root / expected.at("bank_directory").get<std::string>() &&
                    result->library_namespace == expected.at("bank_namespace").get<std::string>() &&
                    result->library_sha256 == expected.at("bank_library_sha256").get<std::string>(), "Python/C++ exact owned bank mismatch");
            selected_banks.emplace(map_relative, result->bank_directory);
        } else if (expected.at("native_mode") == "authenticated-hud-bootstrap") {
            require(result->mode == MccNativeBankMode::AuthenticatedHudBootstrap && result->bank_directory.empty() &&
                    result->bootstrap_dependency == root / expected.at("bootstrap_dependency").get<std::string>() &&
                    result->bootstrap_policy_sha256 == expected.at("bootstrap_policy_sha256").get<std::string>() &&
                    result->bootstrap_source_canonical_sha256 == expected.at("bootstrap_source_canonical_sha256").get<std::string>(),
                    "Python/C++ HUD dependency/canonical Unicode-u32 identity mismatch");
        } else require(result->mode == MccNativeBankMode::None && result->bank_directory.empty(), "Python/C++ unbound native mode mismatch");
        ++positive;
    }
    std::set<fs::path> distinct;
    for (const auto& map : envelope.at("expected_distinct_bank_owners"))
        distinct.insert(selected_banks.at(map.get<std::string>()));
    require(distinct.size() == 3 && positive == 8 && negative == 4 && unbound == 1,
            "producer-consumer fixture coverage/owner census differs");
    require(!fs::exists(root / "halo2/h2_maps_win64_dx11/01b_spacestation.map"), "common dependency invented a gameplay map");
    std::cout << "Actual Python producer/C++ resolver: positive=" << positive << " negative=" << negative
              << " unbound=" << unbound << " distinct_same_leaf_banks=" << distinct.size() << '\n';
}
}

int main(int argc, char** argv) try {
    if (argc == 3 && std::string_view(argv[1]) == "--producer-fixture") {
        producer_fixture(argv[2]);
        return 0;
    }
    require(argc == 1, "Unknown contract test arguments");
    OwnedTemporaryDirectory temporary;
    contract_tests(temporary.path);
    std::cout << "Exact map-owner snapshot/path/metadata contract passed\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
