#include "halo2/source_weather_plate_render.hpp"
#include "halo2/source_weather_plate_caller.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
void SourceWeatherPlateRender::prepare(const SourceWeatherPlateCallerInput &in,
    const SourceWeatherPlatePacket &packet) {
    // Recompute the proof from explicit source inputs. The public helper's
    // result fields are not an admission token and are never accepted here.
    SourceWeatherPlateCallerResult source;
    try {
        source=source_weather_plate_caller_profile(in);
    }catch(const std::invalid_argument &e) {
        throw FormatError(std::string("unsupported weather plate caller: ")+e.what());
    }
    if(source.profile!=SourceWeatherPlateCallerProfile::AboveNearWDepth ||
       !source.program || !source.depth || !source.projected_zw)
        throw FormatError("weather plate requires the original above-near W-depth caller profile");
    const auto &zw=*source.projected_zw;
    if(!std::isfinite(zw[0]) || !std::isfinite(zw[1]) || !(zw[1]>0.f))
        throw FormatError("weather plate caller projection requires finite Z and positive W");
    if(packet.pixel.words!=source.program->words)
        throw FormatError("weather plate packet differs from the complete source-derived mode3 program");
    for(const auto &vertex:packet.vertices)for(unsigned i=0;i<2;++i)
        if(std::bit_cast<std::uint32_t>(vertex.registers[0][2+i])!=
           std::bit_cast<std::uint32_t>(zw[i]))
            throw FormatError("weather plate quad Z/W differs from the source caller projection");
    // The common VS/texture/clip/surface/derived-output guards still run before
    // releasing any retained buffers or descriptor set. This private seam
    // never exposes arbitrary program admission to the caller.
    prepare_admitted(packet,source.program->words);
}
}
