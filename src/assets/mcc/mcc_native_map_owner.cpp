#include "halo2/mcc_native_map_owner.hpp"
#include <algorithm>
#include <array>
#include <iterator>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
#include <vector>

namespace halo2 {
namespace {
using Json = nlohmann::json;
namespace fs = std::filesystem;

void require(bool value, std::string_view message) {
    if (!value) throw MccNativeMapOwnerError("MCC native map owner: " + std::string(message));
}

std::string text(const Json& value, const char* key) {
    require(value.contains(key) && value.at(key).is_string(), std::string("missing text field: ") + key);
    return value.at(key).get<std::string>();
}

bool fingerprint(std::string_view value) {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

std::string hash(std::span<const std::byte> bytes) {
    std::array<unsigned char, 32> digest{};
    unsigned size{};
    require(EVP_Digest(bytes.data(), bytes.size(), digest.data(), &size, EVP_sha256(), nullptr) == 1 &&
            size == digest.size(), "cannot hash admitted snapshot");
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(64);
    for (const auto value : digest) { result += digits[value >> 4]; result += digits[value & 15]; }
    return result;
}

std::span<const std::byte> bytes(std::string_view value) {
    return std::as_bytes(std::span(value.data(), value.size()));
}

Json parse(std::string_view value) {
    // A normal JSON object parse silently discards duplicate keys. Reject them
    // at every nesting level before trusting the producer's ownership fields.
    std::vector<std::set<std::string>> keys;
    const auto callback = [&](int, Json::parse_event_t event, Json& parsed) {
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        else if (event == Json::parse_event_t::key) {
            require(!keys.empty() && keys.back().insert(parsed.get<std::string>()).second,
                    "duplicate JSON metadata key");
        } else if (event == Json::parse_event_t::object_end) keys.pop_back();
        return true;
    };
    return Json::parse(value.begin(), value.end(), callback);
}

fs::path relative(std::string_view name) {
    require(!name.empty() && name.front() != '/' && name.find_first_of("\\:\0", 0, 3) == std::string_view::npos,
            "invalid exact relative path");
    std::size_t begin{};
    while (begin <= name.size()) {
        const auto end = name.find('/', begin);
        const auto part = name.substr(begin, end == name.npos ? name.size() - begin : end - begin);
        require(!part.empty() && part != "." && part != "..", "aliased or traversing relative path");
        if (end == name.npos) break;
        begin = end + 1;
    }
    const fs::path result{std::string(name)};
    require(!result.is_absolute() && result.generic_string() == name, "noncanonical relative path");
    return result;
}

fs::path map_path(std::string_view value) {
    const auto path = relative(value);
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return char(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
    });
    require(*path.begin() == "halo2" && std::distance(path.begin(), path.end()) > 1 && extension == ".map",
            "map lost its exact halo2-relative owner");
    return path;
}

void bank_namespace(std::string_view value) {
    require(!value.empty() && value.size() <= 255 && value.find_first_of("/\\\0", 0, 3) == value.npos,
            "invalid native library namespace");
}

std::uint64_t extent(const Json& value, const char* key) {
    const auto& field = value.at(key);
    require(field.is_number_integer() && (field.is_number_unsigned() || field.get<std::int64_t>() >= 0),
            "invalid encoded byte extent");
    return field.get<std::uint64_t>();
}

void exact_component(const fs::path& parent, const fs::path& component) {
    bool found{};
    for (const auto& entry : fs::directory_iterator(parent))
        if (entry.path().filename().native() == component.native()) { found = true; break; }
    require(found, "filesystem path spelling/case differs");
}

bool exact_absolute_path(const fs::path& path) {
    // Windows manifest components use '/', while a native root uses '\\'.
    // Normalize separator direction only. Dots, traversal and repeated
    // separators must still fail the unchanged canonical-spelling check.
    auto preferred=path;
    preferred.make_preferred();
    return preferred.is_absolute() && preferred.lexically_normal().native()==preferred.native();
}

// Check path metadata, not its content. A later ordinary filename replacement
// cannot change the already supplied map or library snapshot used below.
void private_path(const fs::path& absolute, bool directory) {
    require(exact_absolute_path(absolute), "owner path must be exact absolute path");
    auto current = absolute.root_path();
    for (const auto& part : absolute.relative_path()) {
        exact_component(current, part);
        current /= part;
        require(!fs::is_symlink(fs::symlink_status(current)), "symlink in private owner path");
    }
    require(directory ? fs::is_directory(current) : fs::is_regular_file(current),
            "private owner has wrong filesystem type");
}

fs::path within(const fs::path& root, const fs::path& path, bool directory) {
    require(exact_absolute_path(path), "owner path must be exact absolute path");
    const auto rel = path.lexically_relative(root);
    const auto checked = relative(rel.generic_string());
    require(root / checked == path, "owner escaped explicit prepared root");
    private_path(path, directory);
    return checked;
}

const Json& array(const Json& value, const char* key) {
    require(value.contains(key) && value.at(key).is_array(), std::string("missing array: ") + key);
    return value.at(key);
}

void structure(const Json& index) {
    require(index.is_object() && text(index, "schema") == "H2_MCC_NATIVE_MAP_OWNERS_V1" &&
            index.at("ownership_only") == true, "invalid owner index schema");
    const auto& authority = index.at("input_authority");
    require(authority.is_object() && authority.size() == 3, "invalid producer input authority");
    for (const auto* key : {"copied_source_inventory_sha256", "conversion_sha256", "ordered_output_chain_sha256"})
        require(fingerprint(text(authority, key)), "invalid producer authority fingerprint");
    std::set<std::string> owners, owned_banks, stock_banks;
    for (const auto& value : array(index, "stock_bank_directories")) {
        require(value.is_string(), "invalid stock bank directory");
        const auto name = value.get<std::string>();
        require(*relative(name).begin() == "native-shaders" && stock_banks.insert(name).second,
                "invalid/duplicate stock bank directory");
    }
    for (const auto& row : array(index, "maps")) {
        const auto owner = text(row, "map"); map_path(owner);
        require(owners.insert(owner).second, "duplicate exact map owner");
        extent(row, "input_bytes");
        require(fingerprint(text(row, "input_sha256")) && fingerprint(text(row, "source_sha256")),
                "invalid source/final input fingerprint");
        const auto role = text(row, "cache_role");
        require(role == "scenario" || role == "resources", "invalid source-bound role");
        require(*relative(text(row, "shared_directory")).begin() == "halo2", "invalid private shared directory");
        const auto& receipt = row.at("compatibility_authority");
        require(!text(receipt, "producer").empty() && !text(receipt, "profile").empty() &&
                fingerprint(text(receipt, "receipt_sha256")), "missing producer receipt authority");
        const auto scope = text(receipt, "scope");
        require(scope == "stock-conversion-owner" || scope == "copied-input-only" || scope == "explicit-custom-owner",
                "invalid binding authority scope");
        const auto mode = text(row, "native_mode");
        if (mode == "owned-bank") {
            const auto directory = text(row, "bank_directory");
            require(*relative(directory).begin() == "native-shaders" && directory != "native-shaders" &&
                    text(row, "bank_owner_map") == owner && owned_banks.insert(directory).second &&
                    fingerprint(text(row, "bank_library_sha256")), "whole bank aliases or lost exact map owner");
            bank_namespace(text(row, "bank_namespace"));
            require(scope != "explicit-custom-owner" || !stock_banks.contains(directory), "custom owner aliases stock bank");
        } else {
            require(mode == "none" || mode == "authenticated-hud-bootstrap", "invalid native bank mode");
            for (const auto* key : {"bank_directory", "bank_namespace", "bank_library_sha256", "bank_owner_map"})
                require(text(row, key).empty(), "unbound/HUD mode owns a whole bank");
            if (mode == "authenticated-hud-bootstrap") {
                require(fingerprint(text(row, "bootstrap_policy_sha256")) &&
                        fingerprint(text(row, "bootstrap_source_canonical_sha256")), "missing HUD bootstrap authority");
                require(*relative(text(row, "bootstrap_dependency")).begin() == "native-shaders", "invalid HUD dependency owner");
            }
        }
    }
}

class Metadata {
public:
    Metadata(const fs::path& root, std::span<const MccNativeMetadataSnapshot> snapshots) : root_(root) {
        for (const auto& snapshot : snapshots) {
            const auto name = relative(snapshot.relative_path).generic_string();
            require(snapshots_.emplace(name, snapshot.bytes).second, "duplicate admitted metadata path");
        }
    }

    Json library(const Json& binding) const {
        const auto directory = relative(text(binding, "bank_directory"));
        require(*directory.begin() == "native-shaders" && directory != "native-shaders", "invalid bank owner directory");
        bank_namespace(text(binding, "bank_namespace"));
        const auto snapshot = get((directory / "library.json").generic_string());
        require(fingerprint(text(binding, "bank_library_sha256")) &&
                hash(snapshot) == text(binding, "bank_library_sha256"), "admitted native library fingerprint differs");
        const auto value = parse(std::string_view(reinterpret_cast<const char*>(snapshot.data()), snapshot.size()));
        require(text(value, "schema") == "H2_PRESERVED_NATIVE_SHADERS_V1" &&
                text(value, "map") == text(binding, "bank_namespace"), "native library namespace differs");
        // source_sha256/source_xbe_sha256 describe the retained Xbox bank;
        // neither is the MCC copied-input or final encoded-map hash.
        return value;
    }

    void dependency(const Json& dependency) const {
        const auto kind = text(dependency, "kind");
        if (kind == "authenticated-common-templates") {
            const auto value = library(dependency);
            require(fingerprint(text(dependency, "canonical_sha256")) &&
                    fingerprint(text(dependency, "authority_sha256")) &&
                    hash(bytes(value.dump())) == text(dependency, "canonical_sha256"), "common dependency canonical identity differs");
        } else if (kind == "scalar-function-tables") {
            require(text(dependency, "path") == "native-shaders/function-tables.bin", "invalid scalar dependency owner");
            const auto snapshot = get(text(dependency, "path"));
            require(snapshot.size() == extent(dependency, "bytes") &&
                    fingerprint(text(dependency, "sha256")) && hash(snapshot) == text(dependency, "sha256"),
                    "scalar dependency snapshot differs");
        } else require(false, "unknown common dependency kind");
    }

private:
    std::span<const std::byte> get(const std::string& name) const {
        const auto found = snapshots_.find(name);
        require(found != snapshots_.end(), "required admitted library/scalar snapshot unavailable");
        within(root_, root_ / relative(name), false);
        return found->second;
    }
    const fs::path& root_;
    std::map<std::string, std::span<const std::byte>> snapshots_;
};
}

std::optional<MccNativeMapBinding> resolve_mcc_native_map_binding(
    const MccNativeMapOwnerContext& context, const fs::path& map, const fs::path& shared,
    std::span<const std::byte> admitted, std::span<const MccNativeMetadataSnapshot> snapshots) {
    try {
        require(fingerprint(context.expected_index_sha256) && hash(bytes(context.encoded_index)) == context.expected_index_sha256,
                "index differs from independently authenticated producer fingerprint");
        const auto index = parse(context.encoded_index);
        structure(index);
        private_path(context.cache_root, true);
        const auto owner = within(context.cache_root, map, false).generic_string(); map_path(owner);
        const auto shared_relative = within(context.cache_root, shared, true).generic_string();
        require(*relative(shared_relative).begin() == "halo2", "shared directory lost its private owner");
        const Json* selected{};
        for (const auto& row : index.at("maps")) if (text(row, "map") == owner) selected = &row;
        if (!selected) return std::nullopt;
        const auto& row = *selected;
        require(text(row, "shared_directory") == shared_relative, "explicit shared directory differs from owner binding");
        require(admitted.size() == extent(row, "input_bytes") && hash(admitted) == text(row, "input_sha256"),
                "admitted encoded map differs from final producer output");
        const Metadata metadata(context.cache_root, snapshots);
        for (const auto& dependency : array(index, "dependencies")) metadata.dependency(dependency);
        MccNativeMapBinding binding;
        binding.map_relative = owner;
        binding.source_sha256 = text(row, "source_sha256");
        binding.input_sha256 = text(row, "input_sha256");
        binding.input_bytes = extent(row, "input_bytes");
        binding.custom_authoring = text(row.at("compatibility_authority"), "scope") == "explicit-custom-owner";
        binding.shared_directory = shared;
        binding.role = text(row, "cache_role") == "resources" ? MccCacheRole::Resources : MccCacheRole::Scenario;
        const auto mode = text(row, "native_mode");
        if (mode == "owned-bank") {
            metadata.library(row);
            binding.mode = MccNativeBankMode::Owned;
            binding.bank_directory = context.cache_root / relative(text(row, "bank_directory"));
            binding.library_namespace = text(row, "bank_namespace");
            binding.library_sha256 = text(row, "bank_library_sha256");
        } else if (mode == "authenticated-hud-bootstrap") {
            bool found{};
            for (const auto& dependency : index.at("dependencies")) {
                if (text(dependency, "kind") == "authenticated-common-templates" &&
                    text(dependency, "bank_directory") == text(row, "bootstrap_dependency") &&
                    text(dependency, "canonical_sha256") == text(row, "bootstrap_source_canonical_sha256") &&
                    text(dependency, "authority_sha256") == text(row, "bootstrap_policy_sha256")) found = true;
            }
            require(found, "HUD bootstrap lacks its exact authenticated dependency");
            binding.mode = MccNativeBankMode::AuthenticatedHudBootstrap;
            binding.bootstrap_dependency = context.cache_root / relative(text(row, "bootstrap_dependency"));
            binding.bootstrap_policy_sha256 = text(row, "bootstrap_policy_sha256");
            binding.bootstrap_source_canonical_sha256 = text(row, "bootstrap_source_canonical_sha256");
        }
        return binding;
    } catch (const MccNativeMapOwnerError&) { throw; }
      catch (const std::exception& error) { throw MccNativeMapOwnerError("MCC native map owner: " + std::string(error.what())); }
}
}
