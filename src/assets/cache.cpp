#include "halo2/cache.hpp"
#include "halo2/decoded_allocation_budget.hpp"
#include "halo2/cache_file_snapshot.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/mcc_prepared_map_owner.hpp"
#include "halo2/mcc_resource_reader.hpp"
#include "halo2/texture.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/world_material.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer_balance.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>

namespace halo2 {
namespace {
constexpr char vista_build[] = "11081.07.04.30.0934.main";
bool vista_cache_file(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    std::array<char,332> prefix{};
    return file.read(prefix.data(), prefix.size()) &&
        std::equal(vista_build, vista_build + sizeof(vista_build), prefix.data() + 300);
}

std::vector<std::byte> read_file(const std::filesystem::path& path, std::uint64_t offset = 0,
                                std::uint64_t length = 0, bool whole = true) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw FormatError("cannot open " + path.string());
    const auto end = file.tellg();
    if (end < 0) throw FormatError("cannot measure " + path.string());
    const auto size = static_cast<std::uint64_t>(end);
    if (whole) length = size;
    if (offset > size || length > size - offset || length > 1024ull * 1024 * 1024)
        throw FormatError("invalid or excessive file range in " + path.string());
    std::vector<std::byte> bytes(static_cast<std::size_t>(length));
    file.seekg(static_cast<std::streamoff>(offset));
    if (length && !file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(length)))
        throw FormatError("short read from " + path.string());
    return bytes;
}

std::string json(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        if (ch == '"' || ch == '\\') out << '\\' << ch;
        else if (ch < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(ch) << std::dec;
        else out << ch;
    }
    out << '"';
    return out.str();
}

Vec3 position(const Bytes& data, std::size_t offset) {
    Vec3 p{data.f32(offset), data.f32(offset + 4), data.f32(offset + 8)};
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
        std::abs(p.x) > 1e6f || std::abs(p.y) > 1e6f || std::abs(p.z) > 1e6f)
        throw FormatError("invalid geometry position");
    return p;
}

} // namespace

void Bytes::require(std::size_t offset, std::size_t size) const {
    if (offset > data_.size() || size > data_.size() - offset)
        throw FormatError("binary range outside cache data");
}
std::uint16_t Bytes::u16(std::size_t offset) const {
    require(offset, 2);
    return std::uint16_t(std::to_integer<unsigned>(data_[offset]) |
                         (std::to_integer<unsigned>(data_[offset + 1]) << 8));
}
std::uint32_t Bytes::u32(std::size_t offset) const {
    require(offset, 4);
    std::uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i) result |= std::to_integer<std::uint32_t>(data_[offset + i]) << (i * 8);
    return result;
}
float Bytes::f32(std::size_t offset) const { return std::bit_cast<float>(u32(offset)); }
std::string Bytes::string(std::size_t offset, std::size_t max_size) const {
    require(offset, max_size);
    const auto span = slice(offset, max_size);
    auto end = std::find(span.begin(), span.end(), std::byte{0});
    if (end == span.end()) throw FormatError("unterminated cache string");
    return std::string(reinterpret_cast<const char*>(span.data()), static_cast<std::size_t>(end - span.begin()));
}
std::span<const std::byte> Bytes::slice(std::size_t offset, std::size_t size) const {
    require(offset, size);
    return data_.subspan(offset, size);
}

std::span<const std::byte> Cache::cache_bytes() const {
    return snapshot_ ? snapshot_->bytes() : std::span<const std::byte>(storage_);
}
std::span<std::byte> Cache::mutable_cache_bytes() {
    invalidate_bitmap_cache();
    return snapshot_ ? snapshot_->mutable_bytes() : std::span<std::byte>(storage_);
}

Cache::Cache(const std::filesystem::path& path, const std::filesystem::path& shared_directory,
             std::shared_ptr<const OriginalData> gameplay_data)
    : Cache(path,shared_directory,std::move(gameplay_data),{}) {}

Cache::Cache(const std::filesystem::path& path, const std::filesystem::path& shared_directory,
             std::shared_ptr<const OriginalData> gameplay_data,
             const std::filesystem::path& prepared_cache_root)
    : path_(path), shared_directory_(shared_directory.empty() ? path.parent_path() : shared_directory),
      original_data_(std::move(gameplay_data)) {
    // Vista canonicalization needs its existing resizable owned buffer.
    if (!vista_cache_file(path)) snapshot_ = CacheFileSnapshot::try_open(path);
    if (!snapshot_) storage_ = read_file(path);
    const Bytes data(cache_bytes());
    data.require(0,8);
    if(data.u32(4)==13) {
        // Resolve the exact encoded snapshot before decoding changes storage.
        // The local admission owns all metadata through both MCC initializers.
        const auto owner=admit_mcc_prepared_map_owner(path_,shared_directory_,cache_bytes(),
            prepared_cache_root.empty()?std::nullopt:std::optional(prepared_cache_root));
        custom_mcc_=owner && owner->binding && owner->binding->custom_authoring;
        storage_=decode_mcc_cache(cache_bytes());snapshot_.reset();
        source_file_size_=storage_.size();mcc_=true;
        if(!original_data_)original_data_=OriginalData::native_tu15();
        initialize_mcc(owner?&*owner:nullptr);publish_mcc_bitmap_cache();return;
    }
    data.require(0, 2048);
    if (data.u32(0) != 0x68656164 || data.u32(2044) != 0x666f6f74 || data.u32(4) != 8)
        throw FormatError("expected Halo 2 Xbox version 8 cache header and footer");
    if (data.u32(8) != data.size()) throw FormatError("cache file size does not match header");
    source_file_size_=data.size();
    if (data.string(300,32)==vista_build) {
        vista_=true;
        if (snapshot_) {
            const auto initial = snapshot_->bytes();
            storage_.assign(initial.begin(), initial.end());
            snapshot_.reset();
        }
        initialize_vista();
        return;
    }
    name_ = data.string(408, 32);
    build_ = data.string(288, 32);
    if (build_ != "02.09.27.09809") throw FormatError("unsupported cache build: " + build_);
    scenario_ = data.string(444, 256);
    const auto index = data.u32(16);
    data.require(index, 32);
    const auto count = data.u32(index + 24);
    if (!count || count > 65536) throw FormatError("invalid tag count");
    const std::int64_t header_magic = std::int64_t(data.u32(index)) - (std::int64_t(index) + 32);
    initialize_tag_directory(data, index, header_magic);
    const auto table = TagAddressSpace(data, header_magic).offset(data.u32(index + 8), std::size_t(count) * 16);
    metadata_magic_ = std::int64_t(data.u32(table + 8)) - (std::int64_t(index) + data.u32(20));
    const auto name_base = data.u32(708), name_size = data.u32(712), name_table = data.u32(716);
    data.require(name_base, name_size);
    data.require(name_table, std::size_t(count) * 4);
    tags_.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto t = table + i * 16;
        const auto name_offset = data.u32(name_table + i * 4);
        if (name_offset >= name_size) throw FormatError("tag name offset outside string table");
        Tag tag{TagGroup(data.u32(t)).string(), data.string(std::size_t(name_base) + name_offset, name_size - name_offset),
                data.u32(t + 4), data.u32(t + 8), data.u32(t + 12)};
        // Xbox BSP metadata uses its scenario mapping rather than the main metadata magic.
        if (tag.address && tag.datum != 0xffffffffu && tag.group != "sbsp" && tag.group != "ltmp")
            metadata_offset(tag.address, tag.size);
        tags_.push_back(std::move(tag));
    }
    validate_tag_directory();
    const auto* scenario_tag = find_tag(scenario_tag_, tag_groups::scenario);
    if (!scenario_tag) return; // Resource-only shared caches have no scenario.
    const auto scnr = metadata_offset(scenario_tag->address, scenario_tag->size);
    if (scenario_tag->size < 0x218) throw FormatError("truncated scenario metadata");
    // The original loader calls netgame corrections before combat tuning.
    // Both operate on the private owned buffer before any gameplay consumer.
    if (original_data_ && original_data_->multiplayer_netgame_postprocess_constants) {
        const auto image = original_data_->image == OriginalData::Image::TitleUpdate15
            ? MultiplayerNetgamePostprocessImage::TitleUpdate15
            : MultiplayerNetgamePostprocessImage::MapPack11;
        multiplayer_netgame_writes_ = apply_multiplayer_netgame_postprocess(
            {mutable_cache_bytes(), metadata_magic_, scenario_tag->address},
            *original_data_->multiplayer_netgame_postprocess_constants, image).stores;
    }
    if (original_data_ && original_data_->multiplayer_balance_constants && data.u16(scnr + 0x10) == 1) {
        const auto globals_index = data.u32(index + 0x10) & 0xffff;
        if (globals_index >= count) throw FormatError("missing multiplayer balance globals tag");
        const auto result = apply_multiplayer_balance(
            {mutable_cache_bytes(), metadata_magic_, table, count, scenario_tag->address,
             data.u32(table + std::size_t(globals_index) * 16 + 8)},
            *original_data_->multiplayer_balance_constants,
            original_data_->image == OriginalData::Image::TitleUpdate15 ? 0x13e0 : 0);
        multiplayer_balance_writes_ = result.stores;
    }
    const auto bsp_block = TagAddressSpace(data, metadata_magic_).block(scnr + 0x210, 0x44, 256);
    for (std::size_t i = 0; i < bsp_block.count; ++i) {
        const auto b = bsp_block.offset + i * 0x44;
        Bsp bsp{data.u32(b), data.u32(b + 4), data.u32(b + 8), data.u32(b + 0x14), {}};
        data.require(bsp.file_offset, bsp.size);
        if (bsp.size < 16 + 328) throw FormatError("BSP metadata is truncated");
        const auto* tag = find_tag({bsp.datum}, tag_groups::scenario_structure_bsp);
        if (!tag) throw FormatError("scenario references a missing BSP");
        bsp.name = tag->name;
        bsps_.push_back(std::move(bsp));
    }
    const auto spawn_block = TagAddressSpace(data, metadata_magic_).block(scnr + 0x100, 0x34, 65536);
    for (std::size_t i = 0; i < spawn_block.count; ++i) {
        const auto s = spawn_block.offset + i * 0x34;
        const float facing = data.f32(s + 12);
        if (!std::isfinite(facing)) throw FormatError("invalid spawn facing");
        spawns_.push_back({position(data, s), facing, data.u16(s + 0x12)});
        auto &spawn=spawns_.back();
        spawn.team=std::int16_t(data.u16(s+0x10));
        spawn.player_type=std::int16_t(data.u16(s+0x2c));
        for (unsigned j=0;j<4;++j) {
            spawn.game_types[j]=std::int16_t(data.u16(s+0x14+j*2));
            spawn.spawn_types[j]=std::int16_t(data.u16(s+0x1c+j*2));
        }
    }
}

std::vector<std::byte> Cache::raw_data(std::uint32_t pointer, std::uint32_t size) const {
    if (!size || size > 128 * 1024 * 1024) throw FormatError("invalid geometry resource size");
    const unsigned location = pointer >> 30;
    const std::uint32_t offset = pointer & 0x3fffffffu;
    if (location == 0) {
        if(pc_geometry() && (offset>source_file_size_ || size>source_file_size_-offset))
            throw FormatError("Vista raw resource escapes original cache file");
        const auto span = Bytes(cache_bytes()).slice(offset, size);
        return {span.begin(), span.end()};
    }
    constexpr std::array<const char*, 4> names{"", "mainmenu.map", "shared.map", "single_player_shared.map"};
    if(mcc_) {
        return read_mcc_resource_range(shared_directory_/names[location],offset,size);
    }
    return read_file(shared_directory_ / names[location], offset, size, false);
}

BspLighting Cache::load_bsp_lighting(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("BSP index outside scenario");
    const auto& bsp=bsps_[index];
    return load_bsp_lighting(Bytes(Bytes(cache_bytes()).slice(bsp.file_offset,bsp.size)),
                             std::int64_t(bsp.memory_address));
}

Mesh Cache::load_bsp(std::size_t index, bool textures) const {
    if (index >= bsps_.size()) throw FormatError("BSP index outside scenario");
    const auto& bsp = bsps_[index];
    // Work in BSP-relative offsets so its pointers cannot escape its metadata allocation.
    const Bytes metadata(Bytes(cache_bytes()).slice(bsp.file_offset, bsp.size));
    const auto magic = std::int64_t(bsp.memory_address);
    constexpr std::size_t base = 16;
    const auto clusters = TagAddressSpace(metadata, magic).block(base + 156, 176);
    const auto sections = TagAddressSpace(metadata, magic).block(base + 312, 200);
    const auto instances = TagAddressSpace(metadata, magic).block(base + 320, 88);
    Mesh result;
    BspLighting lighting;
    if(textures) {
        lighting=load_bsp_lighting(metadata,magic);
        result.lightmaps=std::move(lighting.images);
        if(!lighting.structure_matches && !lighting.instance_buckets.empty())
            result.material_warnings.push_back("authored lightmap geometry checksum mismatch; static bucket streams not admitted");
    }
    const auto materials = TagAddressSpace(metadata, magic).block(base + 164, 32, 65536);
    result.materials.resize(materials.count + 1); // Last slot is for absent material references.
    if (textures) {
        const Bytes cache(cache_bytes());
        for (std::size_t i = 0; i < materials.count; ++i) {
            auto& material = result.materials[i];
            const auto datum = metadata.u32(materials.offset + i * 32 + 12);
            try {
                material = load_material(datum);
                prepare_world_material(*this,material);
                for(const auto& gap:material.rendering_gaps)
                    result.material_warnings.push_back(material.name + ": " + gap);
            } catch (const FormatError& error) {
                result.material_warnings.push_back("material " + std::to_string(i) + " " + material.name + ": " + error.what());
            }
        }
    }
    result.minimum = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    result.maximum = {-result.minimum.x, -result.minimum.y, -result.minimum.z};

    struct DecodedSection {
        std::vector<Vertex> vertices;
        std::vector<std::uint16_t> source_vertices;
    };
    DecodedAllocationBudget geometry_budget(256u*1024u*1024u);
    auto decode = [&](std::size_t section) -> DecodedSection {
        const auto vertex_count = metadata.u16(section), face_count = metadata.u16(section + 2);
        if (!vertex_count) return {};
        const auto resources = TagAddressSpace(metadata, magic).block(section + 56, 16, 128);
        if (!resources.count) throw FormatError("geometry has no resources");
        const auto raw = raw_data(metadata.u32(section + 40), metadata.u32(section + 44));
        const Bytes data(raw);
        const std::size_t raw_base = std::size_t(metadata.u32(section + 48)) + 8;
        data.require(0, 42);
        data.require(raw_base, 0);
        struct Resource { std::size_t offset{}, size{}; bool found{}; };
        Resource parts, indices, positions, uvs, basis, lightmap_uvs, incident, buffers;
        for (std::size_t r = 0; r < resources.count; ++r) {
            const auto resource = resources.offset + r * 16;
            const auto size = metadata.u32(resource + 8), offset = metadata.u32(resource + 12);
            data.require(raw_base + offset, size);
            Resource info{raw_base + offset, size, true};
            const auto type = metadata.u16(resource + 4), channel = metadata.u16(resource + 6);
            if (r == 0) parts = info;
            if (type == 32) indices = info;
            if (type == 56 && channel == 0) positions = info;
            if (type == 56 && channel == 1) uvs = info;
            if (type == 56 && channel == 2) basis = info;
            if (type == 56 && channel == 3) lightmap_uvs = info;
            if (type == 56 && channel == 4) incident = info;
            if (type == 56 && channel == 32) buffers = info;
        }
        const auto index_count = data.u16(40);
        if (!positions.found || !indices.found || !parts.found || !parts.size || parts.size % 72 ||
            indices.size < std::size_t(index_count) * 2 || positions.size % vertex_count)
            throw FormatError("invalid geometry resource layout");
        if (!uvs.found || uvs.size < std::size_t(vertex_count) * 8) throw FormatError("missing geometry UV data");
        const std::size_t stride = positions.size / vertex_count;
        if (stride < 12 || stride > 64) throw FormatError("unsupported BSP vertex stride");
        std::vector<Vec3> vertices;
        vertices.reserve(vertex_count);
        for (std::size_t v = 0; v < vertex_count; ++v)
            vertices.push_back(position(data, positions.offset + v * stride));
        std::vector<Vec2> coordinates(vertex_count);
        for (std::size_t v = 0; v < vertex_count; ++v) {
            coordinates[v] = {data.f32(uvs.offset + v * 8), data.f32(uvs.offset + v * 8 + 4)};
            if (!std::isfinite(coordinates[v].x) || !std::isfinite(coordinates[v].y)) throw FormatError("invalid geometry UV");
        }
        const unsigned basis_stride=pc_geometry()?36:12,lightmap_stride=pc_geometry()?8:4,incident_stride=pc_geometry()?12:4;
        if(basis.found && basis.size!=std::size_t(vertex_count)*basis_stride) throw FormatError("invalid tangent basis stream");
        if(lightmap_uvs.found && lightmap_uvs.size!=std::size_t(vertex_count)*lightmap_stride) throw FormatError("invalid lightmap UV stream");
        if(incident.found && incident.size!=std::size_t(vertex_count)*incident_stride) throw FormatError("invalid incident light stream");
        if(pc_geometry()) {
            // Unchanged Vista 0066E828 emits D3D declarations from these
            // 32-byte descriptors: FLOAT3 position, FLOAT2 UV, and three
            // FLOAT3 basis vectors in NORMAL/BINORMAL/TANGENT order.
            auto declared=[&](unsigned channel,unsigned type,unsigned bytes) {
                if(!buffers.found || buffers.size<(channel+1)*32 ||
                   std::to_integer<unsigned>(data.slice(buffers.offset+channel*32,1)[0])!=type ||
                   std::to_integer<unsigned>(data.slice(buffers.offset+channel*32+1,1)[0])!=bytes)
                    throw FormatError("unsupported Vista BSP vertex declaration");
            };
            declared(0,1,12);declared(1,23,8);
            if(basis.found)declared(2,25,36);
            if(lightmap_uvs.found)declared(3,29,8);
            if(incident.found)declared(4,47,12);
        }
        auto make_vertex=[&](unsigned v,Vec3 color,std::uint32_t material) {
            Vertex out{vertices[v],color,coordinates[v],material};
            if(basis.found) {
                if(is_mcc()) {
                    // Restore the original authored physical slots and
                    // precision before feeding the existing Xbox shaders.
                    // PC FLOAT3 slot1/2 must not exchange the original
                    // tangent/binormal banks during this conversion.
                    out.normal=decode_bsp_incident(pack_bsp_incident_tu15(position(data,basis.offset+v*36)));
                    out.tangent=decode_bsp_incident(pack_bsp_incident_tu15(position(data,basis.offset+v*36+12)));
                    out.binormal=decode_bsp_incident(pack_bsp_incident_tu15(position(data,basis.offset+v*36+24)));
                } else if(pc_geometry()) {
                    out.normal=position(data,basis.offset+v*36);
                    out.binormal=position(data,basis.offset+v*36+12);
                    out.tangent=position(data,basis.offset+v*36+24);
                } else {
                    out.normal=decode_bsp_incident(data.u32(basis.offset+v*12));
                    out.tangent=decode_bsp_incident(data.u32(basis.offset+v*12+4));
                    out.binormal=decode_bsp_incident(data.u32(basis.offset+v*12+8));
                }
            }
            if(lightmap_uvs.found) {
                if(is_mcc()) {
                    const auto normalized=[](float value) {
                        return std::max(-1.f,float(pack_bsp_lightmap_uv_tu15(value))/32767.f);
                    };
                    out.lightmap_uv={normalized(data.f32(lightmap_uvs.offset+v*8)),
                        normalized(data.f32(lightmap_uvs.offset+v*8+4))};
                } else if(pc_geometry()) {
                    out.lightmap_uv={data.f32(lightmap_uvs.offset+v*8),data.f32(lightmap_uvs.offset+v*8+4)};
                    if(!std::isfinite(out.lightmap_uv.x) || !std::isfinite(out.lightmap_uv.y))
                        throw FormatError("invalid Vista lightmap UV");
                } else {
                    auto normalized=[](std::uint16_t value){return std::max(-1.f,float(static_cast<std::int16_t>(value))/32767.f);};
                    out.lightmap_uv={normalized(data.u16(lightmap_uvs.offset+v*4)),normalized(data.u16(lightmap_uvs.offset+v*4+2))};
                }
            }
            if(incident.found) {
                if(is_mcc())out.incident=decode_bsp_incident(pack_bsp_incident_tu15(position(data,incident.offset+v*12)));
                else out.incident=pc_geometry()?position(data,incident.offset+v*12):decode_bsp_incident(data.u32(incident.offset+v*4));
            }
            return out;
        };
        bool list = std::size_t(face_count) * 3 == index_count;
        if (list) {
            for (std::size_t i = 0; i + 2 < std::min<std::size_t>(index_count, 30); i += 3) {
                auto a = data.u16(indices.offset + i * 2), b = data.u16(indices.offset + (i + 1) * 2), c = data.u16(indices.offset + (i + 2) * 2);
                if (a == b || b == c || a == c) { list = false; break; }
            }
        }
        DecodedSection triangles;
        for (std::size_t part = 0; part < parts.size / 72; ++part) {
            const auto p = parts.offset + part * 72;
            // The retail visible color pass never draws "not drawn", shadow
            // only, or lightmap-only parts. The latter have no bucket lighting
            // vertices (Waterworks signs have 44 visible plus 4 lightmap-only).
            const auto type=data.u16(p);
            if(textures && (type==0 || type==1 || type==5))continue;
            const auto material = data.u16(p + 4), first = data.u16(p + 6), length = data.u16(p + 8);
            if (first > index_count || length > index_count - first || (list && length % 3))
                throw FormatError("submesh index range outside buffer");
            // Unresolved materials retain an explicit diagnostic palette.
            const std::uint32_t slot = material < materials.count ? material : static_cast<std::uint32_t>(materials.count);
            const bool textured = !result.materials[slot].diffuse.pixels.empty();
            const Vec3 color = textured ? Vec3{1, 1, 1} : Vec3{0.28f + float((material * 73u) % 131u) / 255.f,
                             0.35f + float((material * 47u) % 107u) / 255.f,
                             0.40f + float((material * 31u) % 113u) / 255.f};
            for (std::size_t i = 0; i + 2 < length; i += list ? 3 : 1) {
                auto a = data.u16(indices.offset + (first + i) * 2);
                auto b = data.u16(indices.offset + (first + i + 1) * 2);
                auto c = data.u16(indices.offset + (first + i + 2) * 2);
                if (a >= vertex_count || b >= vertex_count || c >= vertex_count)
                    throw FormatError("triangle index outside vertex buffer");
                if (a == b || b == c || a == c) continue;
                geometry_budget.grow(triangles.vertices,3);
                geometry_budget.grow(triangles.source_vertices,3);
                if (!list && i % 2) std::swap(a, b);
                for(const auto source:{a,b,c}) {
                    auto vertex=make_vertex(source,color,slot);
                    vertex.source_part=std::uint32_t(part);
                    vertex.source_part_type=type;vertex.source_part_flags=data.u16(p+2);
                    triangles.vertices.push_back(vertex);
                }
                triangles.source_vertices.push_back(a);
                triangles.source_vertices.push_back(b);
                triangles.source_vertices.push_back(c);
            }
        }
        return triangles;
    };

    auto append = [&](const std::vector<Vertex>& vertices, std::uint32_t lightmap,
                      const std::array<float, 12>* transform = nullptr, float scale = 1.f,
                      std::int32_t containing_cluster = -1,
                      std::uint32_t source_section=0xffffffffu) {
        geometry_budget.grow(result.vertices,vertices.size());
        for (Vertex vertex : vertices) {
            vertex.lightmap=lightmap;
            vertex.bsp_cluster=containing_cluster;
            vertex.source_section=source_section;
            if (transform) {
                const auto& m = *transform;
                const Vec3 p = vertex.position;
                vertex.position = {scale * (p.x * m[0] + p.y * m[3] + p.z * m[6]) + m[9],
                                   scale * (p.x * m[1] + p.y * m[4] + p.z * m[7]) + m[10],
                                   scale * (p.x * m[2] + p.y * m[5] + p.z * m[8]) + m[11]};
                auto rotate=[&](Vec3 v){return Vec3{v.x*m[0]+v.y*m[3]+v.z*m[6],v.x*m[1]+v.y*m[4]+v.z*m[7],v.x*m[2]+v.y*m[5]+v.z*m[8]};};
                vertex.normal=rotate(vertex.normal); vertex.tangent=rotate(vertex.tangent);
                vertex.binormal=rotate(vertex.binormal);
                // Original lightmap programs transform v2/v3/v4 through the
                // instance matrix but consume bucket v5 directly. Authored
                // incident directions already belong to world space.
            }
            const auto& p = vertex.position;
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) throw FormatError("invalid transformed geometry");
            result.minimum = {std::min(result.minimum.x, p.x), std::min(result.minimum.y, p.y), std::min(result.minimum.z, p.z)};
            result.maximum = {std::max(result.maximum.x, p.x), std::max(result.maximum.y, p.y), std::max(result.maximum.z, p.z)};
            result.vertices.push_back(vertex);
        }
    };
    for (std::size_t c = 0; c < clusters.count; ++c) {
        auto vertices = decode(clusters.offset + c * 176);
        if (!vertices.vertices.empty()) { append(vertices.vertices,c<lighting.clusters.size()?lighting.clusters[c]:0,nullptr,1.f,static_cast<std::int32_t>(c),std::uint32_t(c)); ++result.clusters; }
        geometry_budget.release(vertices.vertices);geometry_budget.release(vertices.source_vertices);
    }
    std::vector<DecodedSection> prototypes(sections.count);
    std::vector<bool> decoded(sections.count, false);
    std::map<std::pair<std::uint32_t,bool>,std::uint32_t> variants;
    auto material_variant=[&](std::uint32_t original,bool per_vertex) {
        if(!per_vertex)return original;
        const auto key=std::pair{original,per_vertex};
        if(const auto it=variants.find(key);it!=variants.end())return it->second;
        Material variant=result.materials.at(original);
        prepare_world_material(*this,variant,true);
        const auto slot=static_cast<std::uint32_t>(result.materials.size());
        result.materials.push_back(std::move(variant));
        variants.emplace(key,slot);
        return slot;
    };
    std::vector<Vertex> instance_vertices;
    for (std::size_t i = 0; i < instances.count; ++i) {
        const auto instance = instances.offset + i * 88;
        const auto section = metadata.u16(instance + 52);
        if (section >= sections.count) throw FormatError("instance references an invalid BSP section");
        if (!decoded[section]) { prototypes[section] = decode(sections.offset + section * 200); decoded[section] = true; }
        std::array<float, 12> matrix;
        for (std::size_t j = 0; j < matrix.size(); ++j) {
            matrix[j] = metadata.f32(instance + 4 + j * 4);
            if (!std::isfinite(matrix[j])) throw FormatError("invalid instance transform");
        }
        const float scale = metadata.f32(instance);
        if (!std::isfinite(scale)) throw FormatError("invalid instance scale");
        // Admit the final expansion and reusable lighting scratch before any
        // instance copy. Repeated instances cannot bypass the byte budget.
        geometry_budget.grow(result.vertices,prototypes[section].vertices.size());
        geometry_budget.grow(instance_vertices,prototypes[section].vertices.size()>instance_vertices.size()?
            prototypes[section].vertices.size()-instance_vertices.size():0);
        instance_vertices.assign(prototypes[section].vertices.begin(),prototypes[section].vertices.end());
        auto &vertices=instance_vertices;
        if(textures && lighting.structure_matches && !vertices.empty() && !lighting.instance_buckets.empty() &&
           i<lighting.instance_buckets.size()) {
            // Original 2D000 selects the static per-pixel/per-vertex category
            // from the instance's authored lightmapping policy, not from the
            // bitmap index (empty geometry also legitimately has bitmap -1).
            const bool per_vertex=metadata.u16(instance+86)==1;
            const auto& ref=lighting.instance_buckets[i];
            if(ref.bucket<0 || ref.section_offsets.size()!=1 || ref.section_offsets[0]==0xffff)
                throw FormatError("unsupported instance lightmap section binding");
            const auto& bucket=lighting.buckets.at(std::size_t(ref.bucket));
            if(!bsp_incident_count(bucket) || (per_vertex && bucket.radiance.empty()))
                throw FormatError("instance is missing authored baked lighting");
            for(std::size_t v=0;v<vertices.size();++v) {
                const auto source=std::size_t(ref.section_offsets[0])+prototypes[section].source_vertices[v];
                if(source>=bsp_incident_count(bucket) ||
                   (per_vertex && source>=bucket.radiance.size()))
                    throw FormatError("instance lightmap vertex escapes bucket");
                vertices[v].incident=bsp_incident(bucket,source);
                if(per_vertex)vertices[v].color=decode_bsp_radiance(bucket.radiance[source]);
                vertices[v].material=material_variant(vertices[v].material,per_vertex);
            }
        }
        if(textures && lighting.structure_matches && !vertices.empty() && !lighting.instance_buckets.empty() &&
           i>=lighting.instance_buckets.size())
            result.material_warnings.push_back("instance "+std::to_string(i)+" has no authored static lightmap bucket");
        append(vertices,i<lighting.instances.size()?lighting.instances[i]:0, &matrix, scale,
               -2-static_cast<std::int32_t>(i),std::uint32_t(section));
        ++result.instances;
    }
    if (result.vertices.empty()) result.minimum = result.maximum = {};
    return result;
}

std::string Cache::describe() const {
    std::ostringstream out;
    out << "{\"name\":" << json(name_) << ",\"build\":" << json(build_) << ",\"scenario\":" << json(scenario_)
        << ",\"tags\":" << tags_.size() << ",\"bsps\":" << bsps_.size() << ",\"spawns\":" << spawns_.size() << '}';
    return out.str();
}
} // namespace halo2
