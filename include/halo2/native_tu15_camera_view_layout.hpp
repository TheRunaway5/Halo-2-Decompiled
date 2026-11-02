#pragma once
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
struct NativeTu15CameraViewGrid {std::uint16_t columns{},rows{};};
struct NativeTu15CameraViewRectangle {std::array<std::uint16_t,4> edges{};};
class NativeTu15CameraViewLayoutDivideError final:public std::runtime_error {
    std::int32_t dividend_,divisor_;std::uint32_t site_;
public:
    NativeTu15CameraViewLayoutDivideError(std::int32_t,std::int32_t,std::uint32_t);
    std::int32_t dividend()const{return dividend_;}
    std::int32_t divisor()const{return divisor_;}
    std::uint32_t original_site()const{return site_;}
};
NativeTu15CameraViewGrid native_tu15_camera_view_grid(std::int32_t count,std::int32_t split_mode);
void native_tu15_camera_view_grid_publish(SourcePhysicsContactMemory &,std::int32_t count,
    std::int32_t split_mode,std::uint32_t destination,const SourcePhysicsContactRecordObserver & = {});
class NativeTu15CameraViewTileArguments {
public:
    virtual ~NativeTu15CameraViewTileArguments()=default;
    virtual std::int32_t count()const=0;
    virtual std::int32_t split_mode()const=0;
    virtual std::uint32_t tile_output()const=0;
};
// Real span publication and tile selection; aliases and fresh grid-width reads
// remain visible. The argument view supplies only actual lazy caller inputs.
void native_tu15_camera_view_tile(SourcePhysicsContactMemory &,std::int32_t ordinal,
    std::uint32_t grid,std::uint32_t span_output,const NativeTu15CameraViewTileArguments &,
    const SourcePhysicsContactRecordObserver & = {});
class NativeTu15CameraViewRectangleArguments {
public:
    virtual ~NativeTu15CameraViewRectangleArguments()=default;
    virtual std::uint32_t grid()const=0;
    virtual std::uint32_t span()const=0;
    virtual std::uint32_t rectangle_output()const=0;
};
// Actual safe-area/window tiling. Private packed rectangle words are native
// values, with no guest stack image or reconstruction of dead high halves.
// Inset outputs in the authenticated caller are ordinary side-effect-free RAM.
// A source ADD-WORD before-write checkpoint has one pending operand read;
// separate native read/write access exposes that same single read before the
// write is attempted. Arbitrary MMIO or atomic write-fault equivalence is not
// this caller profile; completed operations and other fault prefixes are exact.
void native_tu15_camera_view_rectangles(SourcePhysicsContactMemory &,std::uint32_t tile,
    std::uint32_t inset_output,const NativeTu15CameraViewRectangleArguments &,
    const SourcePhysicsContactRecordObserver & = {});
}
