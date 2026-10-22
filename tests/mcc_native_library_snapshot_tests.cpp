#include "halo2/cache.hpp"
#include "halo2/mcc_native_library.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace halo2;
using Json = nlohmann::json;
using Key = std::pair<std::string, std::string>;
namespace fs = std::filesystem;
unsigned checks{}, supplement_rejections{}, bootstrap_rejections{};

void check(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}

Key identity(const Json& record) {
    return {record.at("group").get<std::string>(), record.at("name").get<std::string>()};
}

Json load_json(const fs::path& path) {
    std::ifstream stream(path);
    if (!stream) throw std::runtime_error("Missing owned metadata fixture");
    return Json::parse(stream);
}

void write_json(const fs::path& path, const Json& value) {
    fs::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << value.dump();
    if (!stream) throw std::runtime_error("Cannot write owned metadata control");
}

void supplement_cases(const Json& fixture, const fs::path& root) {
    const auto& original = fixture.at("recipient");
    const auto& source = fixture.at("source");
    const auto& policy = fixture.at("closure_policy");
    const auto name = original.at("map").get<std::string>();
    check(name == "lockout", "Use the real shipped non-introduction recipient");
    const auto row = std::ranges::find_if(policy.at("libraries"), [&](const Json& value) {
        return value.at("map") == name;
    });
    check(row != policy.at("libraries").end() && !row->at("missing").empty(),
          "The canonical recipient must genuinely lack closure records");
    check(source.at("map") == policy.at("source_map"), "Cairo namespace differs");
    const auto source_path = root / policy.at("source_map").get<std::string>() / "library.json";
    std::set<Key> wanted;
    for (const auto& missing : row->at("missing"))
        wanted.emplace(missing.at(0).get<std::string>(), missing.at(1).get<std::string>());
    std::vector<Json> expected_additions;
    std::map<std::string, fs::path> expected_paths;
    for (const auto& record : source.at("shader_tags")) if (wanted.contains(identity(record))) {
        expected_additions.push_back(record);
        expected_paths.emplace(record.at("payload").get<std::string>(),
            source_path.parent_path() / "payloads" / record.at("payload").get<std::string>());
    }
    check(expected_additions.size() == wanted.size(), "Shipped Cairo metadata lacks its actual closure");
    const auto original_count = original.at("shader_tags").size();
    check(original_count == row->at("existing_records").get<std::size_t>(),
          "Recipient beforeimage differs from the shipped closure authority");

    auto positive = [&](bool poison_disk) {
        if (poison_disk) write_json(source_path, Json{{"custom_disk_replacement", true}});
        else check(!fs::exists(source_path), "Missing-file control unexpectedly exists");
        unsigned reads{};
        const MccNativeLibraryRead admitted = [&](const fs::path& path) {
            ++reads;
            check(path == source_path, "Callback requested a metadata path outside the admitted snapshot");
            return source.dump();
        };
        auto bank = original;
        const auto paths = supplement_mcc_native_library(bank, root, {}, admitted);
        check(reads == 1 && paths == expected_paths && !paths.empty(),
              "The real missing closure was not supplied by the exact snapshot");
        check(bank.at("shader_tags").size() == original_count + expected_additions.size(),
              "Unexpected number of supplemented definitions");
        for (std::size_t index = 0; index < original_count; ++index)
            check(bank.at("shader_tags").at(index) == original.at("shader_tags").at(index),
                  "Supplementation changed an existing definition/program/reference");
        for (std::size_t index = 0; index < expected_additions.size(); ++index)
            check(bank.at("shader_tags").at(original_count + index) == expected_additions[index],
                  "Added definition is not the exact ordered Cairo record");
        auto header = bank;
        header["shader_tags"] = original.at("shader_tags");
        check(header == original, "Supplementation changed rasterizer or other library metadata");
        for (const auto& [filename, path] : paths) {
            (void)filename;
            check(!fs::exists(path), "This metadata-only fixture must not provide program payloads");
        }
        auto again = bank;
        reads = 0;
        check(supplement_mcc_native_library(again, root, {}, admitted).empty() &&
              again == bank && reads == 0, "Repeated supplementation changed the complete bank");
    };
    positive(false);
    positive(true);
    check(load_json(source_path) == Json{{"custom_disk_replacement", true}},
          "Snapshot consumption overwrote the unrelated on-disk metadata");

    auto rejects = [&](const MccNativeLibraryRead& reader) {
        auto bank = original;
        bool rejected{};
        try { (void)supplement_mcc_native_library(bank, root, {}, reader); }
        catch (const std::exception&) { rejected = true; }
        check(rejected && bank == original, "Rejected snapshot published partial library metadata");
        ++supplement_rejections;
    };
    auto altered_source = source;
    altered_source["snapshot_changed"] = true;
    rejects([&](const fs::path& path) {
        check(path == source_path, "Wrong-snapshot control requested an unowned path");
        return altered_source.dump();
    });
    rejects([&](const fs::path& path) {
        check(path == source_path, "Malformed snapshot requested an unowned path");
        return std::string("{");
    });
    rejects([](const fs::path&) -> std::string {
        throw FormatError("No admitted library snapshot for this path");
    });
    auto customized = original;
    customized["customized"] = true;
    const auto before = customized;
    unsigned reads{};
    const MccNativeLibraryRead unavailable = [&](const fs::path&) -> std::string {
        ++reads;
        throw FormatError("A custom recipient must not request a common library");
    };
    check(supplement_mcc_native_library(customized, root, {}, unavailable).empty() &&
          customized == before && reads == 0, "Customized metadata was supplemented");
    // Exercise the existing public wrapper without requiring a source file.
    check(supplement_mcc_native_library(customized, root).empty() && customized == before,
          "The existing supplement wrapper changed custom metadata");
}

void bootstrap_cases(const Json& policy, const fs::path& root) {
    const auto& map = policy.at("maps").at(0);
    const auto name = map.at("map").get<std::string>();
    const auto source = root / "no-map-file" / (name + ".map");
    const auto bytes = map.at("bytes").get<std::uint64_t>();
    const auto fingerprint = map.at("sha256").get<std::string>();
    check(!fs::exists(source), "No encoded map is supplied by this metadata-only test");
    unsigned body_reads{}, string_reads{}, library_reads{};
    const MccShaderRead unavailable_body = [&](std::uint32_t, std::size_t) -> std::span<const std::byte> {
        ++body_reads;
        throw FormatError("No actual cached source metadata is admitted by this test");
    };
    const auto unavailable_string = [&](std::uint32_t) -> std::string {
        ++string_reads;
        throw FormatError("No actual source StringID namespace is admitted by this test");
    };
    const MccNativeLibraryRead unavailable_library = [&](const fs::path&) -> std::string {
        ++library_reads;
        throw FormatError("Early bootstrap rejection must not request a library or payload");
    };
    auto rejects = [&](const fs::path& path, std::uint64_t extent, const std::string& hash,
                       std::span<const Tag> directory, Json initial = Json{},
                       unsigned expected_body_reads = 0) {
        body_reads = string_reads = library_reads = 0;
        auto bank = initial;
        const auto paths = bootstrap_mcc_native_hud_library(bank, path, directory, unavailable_body,
            unavailable_string, root, extent, hash, unavailable_library);
        check(paths.empty() && bank == initial, "Invalid bootstrap input changed its bank");
        check(body_reads == expected_body_reads && string_reads == 0 && library_reads == 0,
              "Bootstrap rejection crossed an unadmitted metadata/program boundary");
        ++bootstrap_rejections;
    };
    rejects(source, bytes + 1, fingerprint, {});
    auto wrong_hash = fingerprint;
    wrong_hash.front() = wrong_hash.front() == '0' ? '1' : '0';
    rejects(source, bytes, wrong_hash, {});
    rejects(root / "unknown-custom.map", bytes, fingerprint, {});
    rejects(source, bytes, fingerprint, {}, Json{{"custom_existing_bank", true}});
    // Literal authority values here only reach negative namespace/body gates.
    // No actual encoded input or successful metadata hash is fabricated.
    rejects(source, bytes, fingerprint, {});
    const auto& first = policy.at("owners").at(0);
    Tag owner{first.at("group").get<std::string>(), first.at("name").get<std::string>(),
              first.at("datum").get<std::uint32_t>(), first.at("address").get<std::uint32_t>(),
              first.at("size").get<std::uint32_t>()};
    auto wrong_name = owner;
    wrong_name.name += "_different_owner";
    const std::vector<Tag> wrong_namespace{wrong_name};
    rejects(source, bytes, fingerprint, wrong_namespace);
    const std::vector<Tag> wrong_slot{owner};
    rejects(source, bytes, fingerprint, wrong_slot);
    const std::vector<Tag> duplicate{owner, owner};
    rejects(source, bytes, fingerprint, duplicate);
    std::vector<Tag> directory(owner.handle().index() + 1);
    for (auto& tag : directory) tag.datum = 0xffffffffu;
    directory.at(owner.handle().index()) = owner;
    directory.at(owner.handle().index()).address += 4;
    rejects(source, bytes, fingerprint, directory);
    directory.at(owner.handle().index()) = owner;
    ++directory.at(owner.handle().index()).size;
    rejects(source, bytes, fingerprint, directory);
    directory.at(owner.handle().index()) = owner;
    rejects(source, bytes, fingerprint, directory, Json{}, 1);
    write_json(root / name / "library.json", Json{{"custom_owned_namespace", true}});
    rejects(source, bytes, fingerprint, {});
    Json unknown;
    check(bootstrap_mcc_native_hud_library(unknown, root / "unrecognized.map", {}, unavailable_body,
          unavailable_string, root).empty() && unknown.is_null(),
          "The existing bootstrap wrapper opened an unknown source namespace");
}
} // namespace

int main(int argc, char** argv) try {
    check(argc == 3, "Supply owned metadata fixture and test directory");
    const auto* scratch = std::getenv("HALO2_TEST_SCRATCH");
    check(scratch && *scratch, "The fixture requires tools/run_test.py owned scratch");
    const auto owned = fs::weakly_canonical(fs::path(scratch));
    const auto fixture_path = fs::weakly_canonical(fs::path(argv[1]));
    const auto root = fs::weakly_canonical(fs::path(argv[2]));
    const auto within = [&](const fs::path& path) {
        const auto relative = path.lexically_relative(owned);
        return !relative.empty() && *relative.begin() != ".." && !relative.is_absolute();
    };
    check(within(fixture_path) && within(root), "All generated fixture inputs must stay in owned scratch");
    const auto fixture = load_json(fixture_path);
    fs::create_directories(root);
    supplement_cases(fixture, root / "supplement");
    bootstrap_cases(fixture.at("bootstrap_policy"), root / "bootstrap");
    std::cout << Json{{"checks", checks}, {"supplement_positive_cases", 2},
        {"supplement_atomic_rejections", supplement_rejections},
        {"bootstrap_negative_cases", bootstrap_rejections},
        {"private_MCC_or_payload_reads", 0}, {"actual_positive_bootstrap_claim", false}}.dump() << '\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
