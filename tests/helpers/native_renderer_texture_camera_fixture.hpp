#pragma once
#include "halo2/source_texture_camera_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <map>

namespace halo2::renderer_test {
// Explicit controlled initial source state for the static all-map fixture.
// Production must observe its actual live memory and target lifetime instead.
class NativeRendererTextureCameraFixture final:public SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> rows_;
public:
    explicit NativeRendererTextureCameraFixture(const std::filesystem::path &path) {
        std::ifstream stream(path);if(!stream)throw FormatError("Missing authenticated initial texture-camera fixture");
        const auto fixture=nlohmann::json::parse(stream);
        if(fixture.at("schema")!=1 || fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
           fixture.at("source_blocks").at("0002B547")!="33c0b90c000000bf90614c00f3ab" ||
           fixture.at("source_blocks").at("0002B562")!="c70590614c00ffffffffa2dc5e5100")
            throw FormatError("Texture-camera initial source provenance changed");
        const std::map<std::uint32_t,std::size_t> extents{{0x4c1378u,39u*0x98u},{0x4c6190u,0x30u},{0x51df50u,4u},{0x491e27u,1u},{0x515edcu,1u}};
        for(const auto &row:fixture.at("rows")) {
            const auto address=row.at("address").get<std::uint32_t>();const auto text=row.at("hex").get<std::string>();
            if(!extents.contains(address)||text.size()!=extents.at(address)*2||rows_.contains(address))throw FormatError("Texture-camera initial source extent changed");
            std::vector<std::byte> bytes;
            for(unsigned n=0;n<text.size();n+=2)bytes.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
            for(unsigned n=0;n<bytes.size();++n)if(bytes[n]!=(address==0x4c6190u&&n<4?std::byte{255}:std::byte{0}))
                throw FormatError("Texture-camera initial source bytes changed");
            rows_.emplace(address,std::move(bytes));
        }
        if(rows_.size()!=extents.size())throw FormatError("Texture-camera initial source owner missing");
    }
    void read(std::uint32_t address,std::span<std::byte> output) const override {
        for(const auto &[base,bytes]:rows_)if(address>=base && std::uint64_t(address)-base+output.size()<=bytes.size()) {
            std::copy_n(bytes.begin()+(address-base),output.size(),output.begin());return;
        }
        throw FormatError("Texture-camera initial fixture does not own this source address");
    }
    void write(std::uint32_t,std::span<const std::byte>) override {throw FormatError("Read-only initial texture-camera fixture mutated");}
    SourceTextureCameraBindingTu15 binding() const {return source_texture_camera_binding_tu15(source_texture_camera_inputs_tu15(*this));}
};
}
