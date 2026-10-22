#include "halo2/mcc_prepared_map_owner.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
#include <string_view>
#include <tuple>
#include <vector>

namespace halo2 {
namespace {
using Json = nlohmann::json;
namespace fs = std::filesystem;
constexpr std::string_view profile = "H2_MCC_TU15_PREPARATION_V1";
constexpr std::string_view schema = "H2_MCC_NATIVE_MAP_OWNERS_V1";
constexpr std::string_view stock = "halo2/h2_maps_win64_dx11";
struct ClosureLibrary { std::string_view name, sha256; };
#include "runtime/mcc_native_library_closure.inc"
#include "ui/hud/mcc_native_hud_bootstrap.inc"

void require(bool value, std::string_view why) {
    if (!value) throw MccNativeMapOwnerError("Prepared MCC owner: " + std::string(why));
}
std::span<const std::byte> bytes(std::string_view value) {
    return std::as_bytes(std::span(value.data(), value.size()));
}
std::string digest(std::span<const std::byte> value) {
    std::array<unsigned char, 32> result{}; unsigned length{};
    require(EVP_Digest(value.data(), value.size(), result.data(), &length, EVP_sha256(), nullptr) == 1 &&
            length == result.size(), "cannot hash admitted metadata");
    constexpr char hex[] = "0123456789abcdef";
    std::string out;
    for (const auto v : result) { out += hex[v >> 4]; out += hex[v & 15]; }
    return out;
}
std::string digest(const Json& value) { return digest(bytes(value.dump())); }
bool fingerprint(std::string_view value) {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
std::string text(const Json& value, const char* field) {
    require(value.contains(field) && value.at(field).is_string(), std::string("missing text: ") + field);
    return value.at(field).get<std::string>();
}
std::uint64_t extent(const Json& value, const char* field) {
    const auto& v = value.at(field);
    require(v.is_number_integer() && (v.is_number_unsigned() || v.get<std::int64_t>() >= 0), "invalid byte extent");
    return v.get<std::uint64_t>();
}
Json parse(const std::string& raw) {
    std::vector<std::set<std::string>> keys;
    return Json::parse(raw, [&](int, Json::parse_event_t event, Json& value) {
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        else if (event == Json::parse_event_t::key)
            require(!keys.empty() && keys.back().insert(value.get<std::string>()).second, "duplicate JSON key");
        else if (event == Json::parse_event_t::object_end) keys.pop_back();
        return true;
    });
}
fs::path relative(std::string_view value) {
    require(!value.empty() && value.find_first_of("\\:\0", 0, 3) == value.npos, "invalid relative owner");
    const fs::path path{std::string(value)};
    require(!path.is_absolute() && path.generic_string() == value, "noncanonical relative owner");
    for (const auto& part : path) require(part != "." && part != ".." && !part.empty(), "aliased relative owner");
    // filesystem::path may normalize repeated separators while iterating.
    require(value.front() != '/' && value.back() != '/' && value.find("//") == value.npos, "aliased relative separator");
    return path;
}
fs::path exact_absolute(const fs::path& path) {
    const auto out = fs::absolute(path);
    require(out.lexically_normal().native() == out.native(), "noncanonical absolute owner");
    return out;
}
void private_path(const fs::path& path, bool directory) {
    auto current = path.root_path();
    for (const auto& component : path.relative_path()) {
        bool exact{};
        for (const auto& item : fs::directory_iterator(current))
            if (item.path().filename().native() == component.native()) { exact = true; break; }
        require(exact, "filesystem owner spelling differs");
        current /= component;
        require(!fs::is_symlink(fs::symlink_status(current)), "symlink in private prepared path");
    }
    require(directory ? fs::is_directory(path) : fs::is_regular_file(path), "prepared owner has wrong filesystem type");
}
std::string owner(const fs::path& root, const fs::path& path, bool directory) {
    const auto rel = relative(path.lexically_relative(root).generic_string());
    require(root / rel == path, "prepared owner escapes root");
    private_path(path, directory);
    return rel.generic_string();
}
std::string read(MccPreparedMapOwner& out, const std::string& name) {
    const auto path = out.cache_root / relative(name);
    (void)owner(out.cache_root, path, false);
    if (const auto found = out.metadata.find(name); found != out.metadata.end()) return found->second;
    constexpr std::uint64_t limit = 16u * 1024u * 1024u;
    const auto size = fs::file_size(path); const auto time = fs::last_write_time(path);
    require(size <= limit, "prepared metadata exceeds existing manifest bound");
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "prepared metadata unavailable");
    std::string raw(std::size_t(size), '\0');
    require(!size || bool(input.read(raw.data(), std::streamsize(size))), "truncated prepared metadata");
    require(input.peek() == std::char_traits<char>::eof() && fs::file_size(path) == size && fs::last_write_time(path) == time,
            "prepared metadata changed during snapshot admission");
    (void)owner(out.cache_root, path, false);
    out.metadata.emplace(name, raw);
    return raw;
}
bool map_name(const std::string& name) {
    auto extension = fs::path(name).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return char(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
    });
    return extension == ".map";
}
struct Inputs {
    std::map<std::string, std::string> final;
    std::map<std::string, std::string> roles;
};
Inputs derive(const Json& provenance, const Json& conversion) {
    require(text(provenance, "schema") == profile && text(provenance, "external_installation_requirement") == "Halo MCC only",
            "not a prepared MCC provenance");
    const auto& source = provenance.at("source_files");
    require(source.is_object() && !source.empty(), "missing copied-source inventory");
    Inputs out; std::set<std::string> stock_maps;
    for (const auto& [name, record] : source.items()) {
        const auto path = relative(name);
        require(fingerprint(text(record, "sha256")), "invalid copied source fingerprint");
        (void)extent(record, "bytes");
        out.final.emplace(name, text(record, "sha256"));
        if (path.parent_path() == "h2_maps_win64_dx11" && path.extension() == ".map") stock_maps.insert(name);
    }
    require(!stock_maps.empty() && text(conversion, "profile") == profile && conversion.at("maps").is_array(),
            "invalid source-bound stock conversion");
    for (const auto& row : conversion.at("maps")) {
        const auto leaf = relative(text(row, "map"));
        require(!leaf.has_parent_path() && leaf.extension() == ".map", "conversion lost immediate stock owner");
        const auto name = (fs::path("h2_maps_win64_dx11") / leaf).generic_string();
        require(stock_maps.contains(name) && !out.roles.contains(name), "unknown or duplicate converted owner");
        require(text(row, "source_sha256") == text(source.at(name), "sha256") && fingerprint(text(row, "output_sha256")),
                "stock conversion source/output differs");
        const auto role = text(row, "role");
        require(role == "resources" || role == "multiplayer" || role == "campaign-or-menu", "invalid stock conversion role");
        out.roles.emplace(name, role == "resources" ? "resources" : "scenario"); out.final[name] = text(row, "output_sha256");
    }
    require(out.roles.size() == stock_maps.size(), "missing converted stock owner");
    const auto changes = provenance.value("output_changes", Json::array());
    const auto graphics = provenance.value("graphics_corrections", Json::object());
    require(graphics.is_object(), "invalid graphics report");
    for (const auto& phase : {changes, graphics.value("changes", Json::array())}) {
        require(phase.is_array(), "invalid ordered output chain");
        for (const auto& row : phase) {
            const auto path = relative(text(row, "path"));
            require(*path.begin() == "halo2" && std::distance(path.begin(), path.end()) > 1, "output lost copied-source owner");
            const auto name = path.lexically_relative("halo2").generic_string();
            require(out.final.contains(name) && text(row, "donor_sha256") == out.final.at(name) &&
                    fingerprint(text(row, "converted_sha256")), "stale ordered output donor");
            out.final[name] = text(row, "converted_sha256");
        }
    }
    return out;
}
Json authority(const Json& provenance, const Json& conversion) {
    return {{"copied_source_inventory_sha256", digest(provenance.at("source_files"))},
            {"conversion_sha256", digest(conversion)},
            {"ordered_output_chain_sha256", digest(Json::array({provenance.value("output_changes", Json::array()),
                provenance.value("graphics_corrections", Json::object()).value("changes", Json::array())}))}};
}
void authenticate_rows(const Json& index, const Json& provenance, const Inputs& inputs) {
    std::set<std::string> expected, observed;
    const auto& banks = provenance.at("native_shader_library").at("maps");
    Json stock_directories = Json::array(); std::set<std::string> stock_names;
    for (const auto& [name, sha] : banks.items()) {
        require(!name.empty() && name.size() <= 255 && name.find_first_of("/\\\0", 0, 3) == name.npos &&
                sha.is_string() && fingerprint(sha.get<std::string>()), "invalid stock library authority");
        stock_names.insert((relative("native-shaders/" + name)).generic_string());
    }
    for (const auto& name : stock_names) stock_directories.push_back(name);
    require(index.at("stock_bank_directories") == stock_directories, "index stock bank set differs from provenance");
    const auto hud_policy = parse(std::string(native_hud_bootstrap_policy));
    for (const auto& [name, record] : provenance.at("source_files").items())
        if (map_name(name)) expected.insert("halo2/" + name);
    for (const auto& row : index.at("maps")) {
        const auto map = text(row, "map"); const auto path = relative(map);
        require(*path.begin() == "halo2", "index map lost private owner");
        const auto name = path.lexically_relative("halo2").generic_string();
        require(expected.contains(map) && observed.insert(map).second, "index owner differs from copied inventory");
        const auto& source = provenance.at("source_files").at(name);
        require(text(row, "source_sha256") == text(source, "sha256") && text(row, "input_sha256") == inputs.final.at(name),
                "index source/final identity differs from preparation");
        if (text(row, "input_sha256") == text(source, "sha256"))
            require(extent(row, "input_bytes") == extent(source, "bytes"), "unchanged map extent differs");
        const auto scope = text(row.at("compatibility_authority"), "scope");
        if (const auto role = inputs.roles.find(name); role != inputs.roles.end()) {
            require(text(row, "cache_role") == role->second && text(row, "shared_directory") == stock &&
                    scope == "stock-conversion-owner", "stock role/shared/scope differs from source conversion");
            const auto stem = path.stem().string();
            if (role->second == "resources") {
                require(text(row, "native_mode") == "none", "resource owner cannot adopt a whole bank");
            } else if (banks.contains(stem)) {
                require(text(row, "native_mode") == "owned-bank" &&
                        text(row, "bank_directory") == "native-shaders/" + stem &&
                        text(row, "bank_namespace") == stem && row.at("bank_library_sha256") == banks.at(stem),
                        "indexed stock bank differs from preparation");
            } else {
                const Json* hud{};
                for (const auto& item : hud_policy.at("maps"))
                    if (text(item, "map") == stem && text(row, "source_sha256") == text(item, "sha256") &&
                        text(row, "input_sha256") == text(item, "sha256") && extent(row, "input_bytes") == extent(item, "bytes")) hud = &item;
                if (!hud) require(text(row, "native_mode") == "none", "bankless stock map cannot invent native mode");
                else {
                    const auto common = text(hud_policy, "source_map"), directory = "native-shaders/" + common;
                    require(text(row, "native_mode") == "authenticated-hud-bootstrap" &&
                            text(row, "bootstrap_dependency") == directory &&
                            text(row, "bootstrap_source_canonical_sha256") == text(hud_policy, "source_canonical_sha256") &&
                            banks.contains(common), "indexed HUD policy differs from compiled source authority");
                    bool authenticated{};
                    for (const auto& dependency : index.at("dependencies"))
                        if (text(dependency, "kind") == "authenticated-common-templates" &&
                            text(dependency, "bank_directory") == directory && text(dependency, "bank_namespace") == common &&
                            dependency.at("bank_library_sha256") == banks.at(common) &&
                            text(dependency, "canonical_sha256") == text(hud_policy, "source_canonical_sha256") &&
                            text(dependency, "authority_sha256") == text(row, "bootstrap_policy_sha256")) authenticated = true;
                    require(authenticated, "indexed HUD lacks exact provenance-bound common dependency");
                }
            }
        } else {
            require(scope == "copied-input-only" || scope == "explicit-custom-owner", "nonstock owner invented stock scope");
            if (scope == "copied-input-only")
                require(text(row, "native_mode") == "none" && text(row, "cache_role") == "scenario" &&
                        text(row, "shared_directory") == stock, "copied input cannot invent a custom owner");
            else if (text(row, "native_mode") == "owned-bank")
                require(!stock_names.contains(text(row, "bank_directory")), "custom owner aliases a whole stock bank");
        }
    }
    require(observed == expected, "index omitted copied map owners");
}
void library(MccPreparedMapOwner& out, const Json& binding) {
    const auto directory = relative(text(binding, "bank_directory"));
    require(*directory.begin() == "native-shaders" && directory != "native-shaders", "invalid native bank root");
    (void)read(out, (directory / "library.json").generic_string());
}

void common_library(MccPreparedMapOwner& out, const Json& binding, const Json& native) {
    const auto raw = read(out, text(binding, "bank_directory") + "/library.json");
    const auto bank = parse(raw); const auto name = text(bank, "map");
    const auto admitted = std::find_if(closure_libraries.begin(), closure_libraries.end(),
        [&](const auto& row) { return row.name == name && row.sha256 == digest(bank); });
    if (admitted == closure_libraries.end()) return; // Custom definitions keep their own closure.
    std::set<std::pair<std::string, std::string>> identities;
    for (const auto& tag : bank.at("shader_tags"))
        identities.emplace(text(tag, "group"), text(tag, "name"));
    bool missing{};
    for (const auto& [group, tag] : closure_identities)
        missing |= !identities.contains({std::string(group), std::string(tag)});
    for (const auto& [owner, group, tag] : scoped_closure_identities)
        if (owner == name) missing |= !identities.contains({std::string(group), std::string(tag)});
    if (!missing) return;
    const auto source_name = std::string(closure_source_map);
    require(native.at("maps").contains(source_name), "required common bank lacks provenance authority");
    const auto source_raw = read(out, "native-shaders/" + source_name + "/library.json");
    require(digest(bytes(source_raw)) == native.at("maps").at(source_name).get<std::string>() &&
            digest(parse(source_raw)) == closure_source_sha256, "common bank snapshot differs from audited closure");
}

void scalar(MccPreparedMapOwner& out, const std::string& name, bool mandatory) {
    const auto path = out.cache_root / relative(name);
    if (!mandatory && !fs::exists(fs::symlink_status(path))) return;
    const auto raw = read(out, name);
    require(raw.size() == 28u * 1024u &&
            digest(bytes(raw)) == "1261274f49fbf8f8fd36e9a56f9b7a701a2bad1c5d84be168c69826a62484118",
            "retained native scalar table fingerprint differs");
}
}

std::optional<MccPreparedMapOwner> admit_mcc_prepared_map_owner(const fs::path& map_input,
    const fs::path& shared_input, std::span<const std::byte> encoded,
    std::optional<fs::path> explicit_root) {
    try {
        const auto map = exact_absolute(map_input), shared = exact_absolute(shared_input);
        const bool immediate = map.parent_path() == shared && shared.filename() == "h2_maps_win64_dx11" &&
            shared.parent_path().filename() == "halo2";
        const bool indexed_custom = shared.filename() == "h2_maps_win64_dx11" &&
            shared.parent_path().filename() == "halo2" &&
            fs::exists(shared.parent_path().parent_path() / "native-shaders/map-owners.json");
        if (!explicit_root && !immediate && !indexed_custom) return std::nullopt;
        const auto root = exact_absolute(explicit_root.value_or(shared.parent_path().parent_path()));
        private_path(root, true);
        if (!explicit_root && !fs::exists(root / "provenance.json")) return std::nullopt;
        MccPreparedMapOwner out; out.cache_root = root;
        const auto map_owner = owner(root, map, false), shared_owner = owner(root, shared, true);
        require(*relative(map_owner).begin() == "halo2" && *relative(shared_owner).begin() == "halo2", "prepared input lost halo2 owner");
        const auto provenance_raw = read(out, "provenance.json"); const auto provenance = parse(provenance_raw);
        const auto conversion = parse(read(out, "tag-conversion.json"));
        const auto inputs = derive(provenance, conversion);
        const auto& native = provenance.at("native_shader_library");
        require(native.is_object() && native.at("maps").is_object(), "missing native bank authorities");
        scalar(out, "native-shaders/function-tables.bin", false);
        Json index;
        std::string raw_index, expected;
        if (native.contains("owner_bindings")) {
            const auto& pointer = native.at("owner_bindings");
            require(pointer.is_object() && pointer.size() == 4 && text(pointer, "schema") == schema &&
                    text(pointer, "path") == "native-shaders/map-owners.json" && fingerprint(text(pointer, "sha256")),
                    "invalid indexed owner pointer");
            raw_index = read(out, "native-shaders/map-owners.json"); expected = text(pointer, "sha256");
            require(raw_index.size() == extent(pointer, "bytes") && digest(bytes(raw_index)) == expected, "index pointer fingerprint differs");
            index = parse(raw_index); require(index.at("input_authority") == authority(provenance, conversion), "stale index preparation authority");
            authenticate_rows(index, provenance, inputs); out.indexed = true;
        } else {
            // Legacy compatibility is a source-bound projection, never an
            // unauthenticated on-disk index or a nested map's basename guess.
            if (!immediate || map.parent_path() != root / std::string(stock)) return out;
            const auto name = map.filename().generic_string(), key = "h2_maps_win64_dx11/" + name;
            const auto role = inputs.roles.find(key);
            if (role == inputs.roles.end()) return out;
            const auto& source = provenance.at("source_files").at(key);
            require(digest(encoded) == inputs.final.at(key), "legacy final encoded map differs");
            if (inputs.final.at(key) == text(source, "sha256")) require(encoded.size() == extent(source, "bytes"), "legacy unchanged map extent differs");
            Json row = {{"map", map_owner}, {"source_sha256", text(source, "sha256")}, {"input_sha256", inputs.final.at(key)},
                {"input_bytes", encoded.size()}, {"cache_role", role->second}, {"shared_directory", shared_owner},
                {"native_mode", "none"}, {"bank_directory", ""}, {"bank_namespace", ""}, {"bank_library_sha256", ""}, {"bank_owner_map", ""},
                {"compatibility_authority", {{"producer", "prepared-stock-legacy-projection"}, {"profile", profile},
                    {"receipt_sha256", digest(bytes(provenance_raw))}, {"scope", "stock-conversion-owner"}}}};
            index = {{"schema", schema}, {"ownership_only", true}, {"maps", Json::array()},
                {"dependencies", Json::array()}, {"stock_bank_directories", Json::array()}, {"input_authority", authority(provenance, conversion)}};
            const auto stem = map.stem().string();
            if (role->second == "scenario" && native.at("maps").contains(stem)) {
                row["native_mode"] = "owned-bank"; row["bank_directory"] = "native-shaders/" + stem;
                row["bank_namespace"] = stem; row["bank_library_sha256"] = native.at("maps").at(stem); row["bank_owner_map"] = map_owner;
                index["stock_bank_directories"].push_back(row["bank_directory"]);
            } else if (role->second == "scenario") {
                const auto policy = parse(std::string(native_hud_bootstrap_policy));
                for (const auto& hud : policy.at("maps")) if (text(hud, "map") == stem &&
                        inputs.final.at(key) == text(source, "sha256") && text(hud, "sha256") == inputs.final.at(key) &&
                        extent(hud, "bytes") == encoded.size()) {
                    const auto source_map = text(policy, "source_map");
                    require(native.at("maps").contains(source_map), "HUD common bank is absent from provenance");
                    // Legacy authority hashes the exact compiled UTF-8 policy
                    // literal, not the differently formatted shipped JSON file.
                    const auto policy_sha = digest(bytes(native_hud_bootstrap_policy));
                    Json dependency = {{"kind", "authenticated-common-templates"}, {"bank_directory", "native-shaders/" + source_map},
                        {"bank_namespace", source_map}, {"bank_library_sha256", native.at("maps").at(source_map)},
                        {"canonical_sha256", text(policy, "source_canonical_sha256")}, {"authority_sha256", policy_sha}};
                    index["dependencies"].push_back(dependency);
                    row["native_mode"] = "authenticated-hud-bootstrap"; row["bootstrap_policy_sha256"] = policy_sha;
                    row["bootstrap_dependency"] = dependency["bank_directory"]; row["bootstrap_source_canonical_sha256"] = dependency["canonical_sha256"];
                }
            }
            index["maps"].push_back(std::move(row)); raw_index = index.dump(); expected = digest(bytes(raw_index));
        }
        // Retain dependencies before the resolver. No payload/program or map
        // filename is reopened to replace the admitted encoded source bytes.
        for (const auto& dependency : index.at("dependencies")) {
            if (text(dependency, "kind") == "authenticated-common-templates") library(out, dependency);
            else if (text(dependency, "kind") == "scalar-function-tables") scalar(out, text(dependency, "path"), true);
        }
        for (const auto& row : index.at("maps"))
            if (text(row, "map") == map_owner && text(row, "native_mode") == "owned-bank") {
                library(out, row);
                common_library(out, row, native);
            }
        std::vector<MccNativeMetadataSnapshot> snapshots;
        for (const auto& [name, raw] : out.metadata) snapshots.push_back({name, bytes(raw)});
        out.binding = resolve_mcc_native_map_binding({root, raw_index, expected}, map, shared, encoded, snapshots);
        if (out.binding) {
            if (out.binding->mode == MccNativeBankMode::Owned)
                scalar(out, out.binding->bank_directory.lexically_relative(root).generic_string() + "/function-tables.bin", true);
            else if (out.binding->mode == MccNativeBankMode::AuthenticatedHudBootstrap)
                scalar(out, out.binding->bootstrap_dependency.lexically_relative(root).generic_string() + "/function-tables.bin", true);
        }
        return out;
    } catch (const MccNativeMapOwnerError&) { throw; }
      catch (const std::exception& error) { throw MccNativeMapOwnerError("Prepared MCC owner: " + std::string(error.what())); }
}
}
