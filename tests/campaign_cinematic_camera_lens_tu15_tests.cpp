#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "halo2/source_campaign_camera_lens_tu15.hpp"
#include "halo2/source_view_projection.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <bit>
#include <cstring>
#include <iostream>
#include "fixtures/campaign_cinematic_camera_lens_tu15.inc"
using namespace halo2;
namespace {
void check(bool condition,const char *reason) {if(!condition)throw FormatError(reason);}
bool equal_bits(float a,float b) {return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool equal_vec(Vec2 a,Vec2 b) {return equal_bits(a.x,b.x)&&equal_bits(a.y,b.y);}
float real(std::uint32_t x) {return std::bit_cast<float>(x);}
Camera camera(const LensFixture &f) {
    Camera c;
    c.position={real(f.camera[0]),real(f.camera[1]),real(f.camera[2])};
    c.forward={real(f.camera[3]),real(f.camera[4]),real(f.camera[5])};
    c.up={real(f.camera[6]),real(f.camera[7]),real(f.camera[8])};c.oriented=true;
    c.field_of_view=60;c.horizontal_field_of_view=true;
    return c;
}
// The renderer's actual perspective/clip operations, with no view transform:
// compare the projection itself independently of source constant generation.
glm::mat4 raster(const Camera &c,unsigned w,unsigned h) {
    auto p=glm::perspective(*c.projection_vertical_angle,float(w)/h,.015625f,1024.f);
    p[0][0]*=c.projection_clip_scale.x;p[1][1]*=c.projection_clip_scale.y;
    p[2][0]-=c.projection_clip_offset.x;p[2][1]+=c.projection_clip_offset.y;
    return p;
}
}
int main(int argc,char **argv)try {
    const bool legacy=argc==2 && std::string(argv[1])=="--legacy-cinematic-full-rectangle";
    check(argc==1 || legacy,"unexpected lens-test argument");
    unsigned contexts{},raster_checks{},scales{},gameplay{};
    for(const auto &f:lens_fixtures) {
        const auto original=std::bit_cast<SourceViewProjectionBank>(f.view);
        auto c=camera(f);const auto previous=c;
        auto lens=source_campaign_camera_lens_tu15(real(f.camera[10]),640,480,{48,36,544,408});
        if(legacy) {
            // Exact former campaign caller inputs: safe=outer and default
            // raster clip scale. This control must expose the captured error.
            lens.source.safe=lens.source.outer;
            lens.raster.clip_scale={1,1};lens.raster.clip_offset={};
        }
        lens.apply(c);
        check(c.position.x==previous.position.x&&c.position.y==previous.position.y&&c.position.z==previous.position.z&&
              c.forward.x==previous.forward.x&&c.forward.y==previous.forward.y&&c.forward.z==previous.forward.z&&
              c.up.x==previous.up.x&&c.up.y==previous.up.y&&c.up.z==previous.up.z&&
              c.field_of_view==previous.field_of_view,"lens changed original pose/observer FOV");
        check(equal_bits(*c.projection_vertical_angle,real(f.camera[10])),"original vertical angle changed");
        const auto projected=source_frustum_projection(lens.source);
        const auto program=std::bit_cast<std::array<std::uint32_t,16>>(projected.projection_program);
        check(program==f.program,"cinematic projection program differs from unchanged TU1.5 camera fixture");
        const auto native=source_view_projection({c.position,c.forward,c.up,real(f.camera[16]),real(f.camera[17]),{},projected.projection_program});
        check(std::bit_cast<std::array<std::uint32_t,48>>(native)==f.view,
              "cinematic full192B view/projection differs from original captured context");++contexts;
        const auto p=raster(c,640,480);
        for(unsigned i=0;i<16;++i) {
            const float actual=(&p[0][0])[i],expected=original.projection[i];
            // Zero sign is not a raster distinction; all nonzero coefficients
            // must match the literal original words, with no error tolerance.
            if(actual==0&&expected==0)continue;
            if(!equal_bits(actual,expected)) {
                std::cerr<<"raster field="<<i<<" actual="<<std::hex<<std::bit_cast<std::uint32_t>(actual)
                         <<" original="<<std::bit_cast<std::uint32_t>(expected)<<std::dec<<'\n';
                throw FormatError("raster projection coefficient differs from captured TU1.5 matrix");
            }
        }++raster_checks;
        for(const auto [w,h]:std::array{std::pair{320u,240u},std::pair{1280u,960u},std::pair{2560u,1920u}}) {
            const auto scaled=source_campaign_camera_lens_tu15(real(f.camera[10]),w,h,{48,36,544,408});
            auto output=previous;scaled.apply(output);const auto q=source_frustum_projection(scaled.source);
            check(q.projection_bounds==projected.projection_bounds&&equal_bits(q.projection_program[10],projected.projection_program[10])&&
                  equal_bits(q.projection_program[11],projected.projection_program[11]),"same-aspect resolution changed cinematic lens");
            const auto scaled_raster=raster(output,w,h);
            check(equal_bits(scaled_raster[0][0],p[0][0])&&equal_bits(scaled_raster[1][1],p[1][1]),"resolution changed raster lens");++scales;
        }
        // The previous gameplay caller already publishes this physical-safe
        // bank and this source_camera_projection result. Keep both unchanged.
        for(const auto [w,h]:std::array{std::pair{640u,480u},std::pair{1920u,1080u},std::pair{1001u,751u}}) {
            const auto safe=full_surface_viewport({48,36,544,408},w,h);
            const auto old_raster=source_camera_projection(real(f.camera[10]),{0,0,float(w),float(h)},safe,false);
            const auto shared=source_campaign_camera_lens_tu15(real(f.camera[10]),w,h,{48,36,544,408});
            check(equal_vec(shared.raster.clip_scale,old_raster.clip_scale)&&equal_vec(shared.raster.clip_offset,old_raster.clip_offset)&&
                  shared.raster.horizontal_scale==old_raster.horizontal_scale&&shared.raster.vertical_scale==old_raster.vertical_scale,
                  "shared cinematic lens altered existing gameplay lens arithmetic");++gameplay;
        }
        std::cout<<"MATCH "<<f.name<<" full192B context+64B original program+raster matrix\n";
    }
    std::cout<<"PASS contexts="<<contexts<<" raster="<<raster_checks<<" same_aspect_scales="<<scales<<" gameplay_controls="<<gameplay<<'\n';
    std::cout<<"LIMIT retained OG TU1.5 standard4:3 contexts; native physical resolution extension only, no original wide/split or full pose timeline claim\n";
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
