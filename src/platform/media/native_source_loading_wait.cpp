#include "halo2/native_source_loading_wait.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/frontend_text_style.hpp"
#include <algorithm>
#include <bit>
namespace halo2 {
NativeSourceLoadingWait::NativeSourceLoadingWait(const GameplayFontSource&source)
    :font_(load_gameplay_font(source,3)) {
    if(source.domain!=GameplayFontDomain::Mcc || !font_.atlas.width || !font_.atlas.height ||
        font_.atlas.pixels.empty() || font_.ascent<0 || font_.descent<0)
        throw FormatError("Source loading wait requires its admitted MCC/H2EK font3 owner");
    Material material;material.name="source-loading-wait/font3";material.diffuse=font_.atlas;
    material.ui_full_surface=true;material.texture_states[0].address_u=material.texture_states[0].address_v=3;
    mesh_.materials.push_back(std::move(material));vertices_.reserve(512);
}
void NativeSourceLoadingWait::update(std::uint32_t progress,unsigned language) {
    state_=original_loading_wait_text(progress,language,{0,0,480,640},{0x27f,0x1f80});
    vertices_.clear();
    // Same original13EF40/141A20 accumulation used by the source font owner.
    float width{};unsigned previous{};
    for(const auto c:state_.text) {
        if(const auto it=font_.glyphs.find(c);it!=font_.glyphs.end()) {
            const auto kern=font_.kerning.find({previous,c});
            width=float(it->second.advance)+float(it->second.bearing_x)+
                (kern==font_.kerning.end()?0.f:float(kern->second))+width;
        }previous=c;
    }
    float x=(float(state_.rectangle[3]-state_.rectangle[1])-width)*.5f+float(state_.rectangle[1]);
    const float baseline=float(state_.rectangle[0])+float(font_.ascent);
    const auto foreground=frontend_text_pixel_color(frontend_text_pixel(frontend_text_color({1,1,1,1})));
    // Literal229DB0 shadow arguments, supplied before141890 copies its
    // complete font state. They are independent of the frame's black clear.
    const auto shadow=frontend_text_pixel_color(frontend_text_pixel({1,
        std::bit_cast<float>(0x3c808081u),std::bit_cast<float>(0x3cc0c0c1u),std::bit_cast<float>(0x3d20a0a1u)}));
    const auto quad=[&](float l,float t,float w,float h,const std::array<float,4>&uv,UiColor color) {
        const Vertex a{{l,t,color.alpha},{color.red,color.green,color.blue},{uv[0],uv[1]},0},
            b{{l+w,t,color.alpha},{color.red,color.green,color.blue},{uv[2],uv[1]},0},
            c{{l+w,t+h,color.alpha},{color.red,color.green,color.blue},{uv[2],uv[3]},0},
            d{{l,t+h,color.alpha},{color.red,color.green,color.blue},{uv[0],uv[3]},0};
        vertices_.insert(vertices_.end(),{a,b,d,b,c,d});
    };
    previous=0;
    for(const auto c:state_.text) {
        const auto it=font_.glyphs.find(c);if(it==font_.glyphs.end()){previous=c;continue;}
        const auto &g=it->second;const auto k=font_.kerning.find({previous,c});
        const float kern=k==font_.kerning.end()?0.f:float(k->second);
        const float gx=x+float(g.bearing_x)+kern,gy=baseline-float(g.bearing_y);
        const float l=std::max(float(state_.rectangle[1]),gx),r=std::min(float(state_.rectangle[3]),gx+float(g.width)),
            t=std::max(float(state_.rectangle[0]),gy),b=std::min(float(state_.rectangle[2]),gy+float(g.height));
        if(g.width && g.height && r>l && b>t && baseline<float(state_.rectangle[2])) {
            // Source1F470 truncates cropped glyph width/offset before1F380.
            const auto uv=retail_font_sample_uvs(font_,c,int(l-gx),int(t-gy),int(r-l),int(b-t));
            quad(l+1,t+1,float(int(r-l)),float(int(b-t)),uv,shadow);
            quad(l,t,float(int(r-l)),float(int(b-t)),uv,foreground);
        }
        x=float(g.advance)+x+float(g.bearing_x)+kern;previous=c;
    }
}
}
