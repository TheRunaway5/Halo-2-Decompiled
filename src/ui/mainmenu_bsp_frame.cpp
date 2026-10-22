#include "halo2/mainmenu_bsp_frame.hpp"

namespace halo2 {
namespace {
BspAtmosphere mainmenu_atmosphere(const Cache &cache,std::size_t bsp) {
    if(cache.load_source_geometry_scenario_type()!=2)
        throw FormatError("mainmenu BSP frame requires source scenario type2");
    return cache.load_bsp_atmosphere(bsp);
}
}
MainmenuCameraLocation mainmenu_camera_location(const BspAtmosphere &bank,Vec3 eye,
    BspAtmosphere::CameraLocation previous) {
    MainmenuCameraLocation result;
    result.current=bank.location_at(eye);
    result.retained=previous;
    // 2B720 tests the returned leaf againstNONE, not the mapped cluster.
    result.query_succeeded=result.current.leaf!=-1;
    if(result.query_succeeded)result.retained=result.current;
    else if(previous.leaf>=0 && std::size_t(previous.leaf)<bank.leaves.size() &&
            previous.cluster>=0 && std::size_t(previous.cluster)<bank.clusters.size())
        result.current=previous;
    return result;
}
std::vector<std::uint8_t> mainmenu_bsp_fog_admission(const BspAtmosphere &bank,
    std::uint32_t datum,std::optional<std::uint32_t> flags) {
    if(flags)return source_bsp_fog_admission(bank,{true,true,datum,*flags});
    auto clear=source_bsp_fog_admission(bank,{true,true,datum,0});
    if(clear!=source_bsp_fog_admission(bank,{true,true,datum,0x10}))
        throw FormatError("mainmenu BSP frame requires actual4E64C8 flags for differing fog membership branches");
    return clear;
}
SourceBspShaderView MainmenuBspAdmissionFrame::shader_view() const & {
    return {render_camera,admission,0,480,1,view_modifier};
}
MainmenuBspFrameProducer::MainmenuBspFrameProducer(const Cache &cache,std::size_t bsp)
    :atmosphere_(mainmenu_atmosphere(cache,bsp)),
     portals_(cache.load_source_portal_bsp(bsp)),
     geometry_(cache.load_source_bsp_admission(bsp)) {}
MainmenuBspAdmissionFrame MainmenuBspFrameProducer::prepare(
    const MainmenuBspFrameInput &input) const {
    MainmenuBspAdmissionFrame result;
    const auto view=menu_render_view(input.camera,input.cinematic.present,
        input.cinematic.byte5);
    result.render_camera=view.bank;
    result.view_modifier=view.view_modifier;
    result.cinematic=input.cinematic;
    const auto location=mainmenu_camera_location(atmosphere_,input.camera.position,
        input.previous_location);
    if(!location.query_succeeded || location.current.cluster<0)
        throw FormatError("mainmenu BSP frame requires a current inside-BSP camera location");
    result.current_location=location.current;
    result.retained_location=location.retained;
    result.location_query_succeeded=location.query_succeeded;
    result.fog=atmosphere_.sample(input.camera.position,input.camera.forward);
    if(result.fog.cluster!=result.current_location.cluster)
        throw FormatError("mainmenu BSP frame aggregate fog location differs from the current camera");
    // 441B0 uses its own original basis constructor. Reusing the render
    // camera's reconstructed up would change the clipping arithmetic.
    result.clipping_camera=source_frustum_camera({input.camera.position,
        input.camera.forward,input.camera.up,.015625f,1024.f,
        result.render_camera.clipping_rectangle});
    result.visibility=source_portal_visibility(portals_,
        std::span(&result.clipping_camera,1),result.current_location.cluster,
        input.portals.words);
    const auto fog=mainmenu_bsp_fog_admission(atmosphere_,result.fog.planar_datum,
        input.flags_4e64c8);
    // 3F2C0's proved normal collector has count1,radius1024,flags0,mode0,
    // NULL filter and no split plane. The literal131FF0 capacities are128/384.
    SourceBspAdmissionInput admission;
    admission.containing_fog=fog;
    result.admission=source_bsp_admission(geometry_,result.visibility,admission);
    return result;
}
}
