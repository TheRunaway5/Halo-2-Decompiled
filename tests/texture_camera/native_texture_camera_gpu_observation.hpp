#pragma once
#include "halo2/renderer.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <fstream>
#include <set>

namespace halo2::texture_camera_test {
using Json=nlohmann::json;
inline void gpu_require(bool ok,const char *why){if(!ok)throw FormatError(why);}
inline std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> out;unsigned count{};
    auto *state=EVP_MD_CTX_new();gpu_require(state,"Texture-camera digest allocation failed");
    const auto ok=EVP_DigestInit_ex(state,EVP_sha256(),nullptr)==1 &&
        EVP_DigestUpdate(state,bytes.data(),bytes.size())==1 &&
        EVP_DigestFinal_ex(state,out.data(),&count)==1;
    EVP_MD_CTX_free(state);gpu_require(ok && count==32,"Texture-camera digest failed");
    static constexpr char hex[]="0123456789abcdef";std::string result;
    for(const auto b:out){result+=hex[b>>4];result+=hex[b&15];}return result;
}
struct PixelImage {unsigned width{},height{};std::vector<std::byte> rgb;std::string sha256;};
inline PixelImage pixels(const std::filesystem::path &path) {
    std::ifstream file(path,std::ios::binary);std::string format;unsigned limit;
    PixelImage result;file>>format>>result.width>>result.height>>limit;
    gpu_require(format=="P6" && limit==255 && result.width==640 && result.height==480,
        "Texture-camera capture lost its declared real640x480 target");
    gpu_require(file.get()=='\n',"Texture-camera PPM header terminator differs");
    result.rgb.resize(std::size_t(result.width)*result.height*3u);
    file.read(reinterpret_cast<char *>(result.rgb.data()),result.rgb.size());
    gpu_require(file && file.peek()==EOF,"Texture-camera capture is truncated or has trailing bytes");
    result.sha256=digest(result.rgb);return result;
}
inline std::size_t colors(const PixelImage &image) {
    std::set<std::array<std::byte,3>> values;
    for(std::size_t i=0;i<image.rgb.size();i+=3)values.insert({image.rgb[i],image.rgb[i+1],image.rgb[i+2]});
    return values.size();
}
// Actual post-vkCmdDraw observations. Image handles are copied from the real
// descriptor bind; they are never inferred from a CPU enum or supplied bank.
struct CameraGpuObservation {
    bool reference{};
    std::set<std::uint32_t> camera_shaders;
    unsigned copy_calls{},selected_target{};
    std::uint64_t copy_image{},copy_view{};
    std::size_t vertex_words{},uploads{},scene_camera_draws{},ordinary_camera_draws{},owner_camera_draws{};
    std::map<unsigned,std::pair<std::uint64_t,std::uint64_t>> fallback_images;
    std::set<std::uint64_t> recursive_images,recursive_views;
    Json uploaded=Json::array(),draws=Json::array();
    void upload(const RendererCompletedMaterialUpload &value,bool producer) {
        ++uploads;vertex_words+=value.uploaded_vertex_instruction_words;
        if(!reference)gpu_require(value.uploaded_vertex_instruction_words==0,
            "Native texture-camera draw uploaded executable Xbox vertex instructions");
        if(camera_shaders.contains(value.source_shader)) {
            const auto &fallback=value.cached_material_pages[0];
            gpu_require(fallback.image_identity && fallback.view_identity && fallback.source_image,
                "Actual texture-camera upload lost its independently resident authored default2D image");
            fallback_images[unsigned(value.material_slot)]={fallback.image_identity,fallback.view_identity};
        }
        if(camera_shaders.contains(value.source_shader))uploaded.push_back({{"producer",producer},{"slot",value.material_slot},
            {"vertex_words",value.uploaded_vertex_instruction_words},
            {"uniform_sha256",digest(value.material_uniform)},{"vertex_bank_sha256",digest(value.vertex_bank_uniform)}});
    }
    void copy(unsigned target,std::uint64_t image,std::uint64_t view) {
        gpu_require(!copy_calls && (target==18 || target==20) && image && view,
            "Texture-camera producer did not record exactly one real selected native image");
        ++copy_calls;selected_target=target;copy_image=image;copy_view=view;
        gpu_require(!recursive_images.contains(image)&&!recursive_views.contains(view),
            "Recursive texture-camera draw sampled its subsequently selected destination");
    }
    template<class Geometry> void draw(const Geometry &value,bool producer) {
        if(!camera_shaders.contains(value.source_material[0]))return;
        gpu_require(value.descriptor && value.count,"Actual viewscreen draw lost its descriptor or vertices");
        const auto &bound=value.descriptor->images[0][0];
        gpu_require(bound.present && bound.image_owner_known && bound.image && bound.view &&
            bound.width && bound.height && bound.layers==1 && bound.mip_count,
            "Actual viewscreen sampler lost its real image owner");
        if(producer) {
            ++scene_camera_draws;
            recursive_images.insert(bound.image);recursive_views.insert(bound.view);
            gpu_require(fallback_images.contains(value.material_slot) &&
                fallback_images.at(value.material_slot)==std::pair{bound.image,bound.view},
                "Recursive texture-camera producer did not bind its actual independently resident default2D");
        } else {
            ++ordinary_camera_draws;
            if(copy_calls)gpu_require(bound.image==copy_image && bound.view==copy_view &&
                bound.width==640 && bound.height==480,
                "Ordinary viewscreen did not sample the actual same-frame produced target");
            else gpu_require(fallback_images.contains(value.material_slot) &&
                fallback_images.at(value.material_slot)==std::pair{bound.image,bound.view},
                "Inactive viewscreen did not bind its actual independently resident default2D");
        }
        draws.push_back({{"producer",producer},{"shader",value.source_material[0]},
            {"shader_lod",value.source_material[1]},{"slot",value.material_slot},{"count",value.count},
            {"image",bound.image},{"view",bound.view},{"extent",{bound.width,bound.height}},
            {"format",bound.format},{"layout",bound.layout},{"viewport",value.viewport},{"scissor",value.scissor}});
    }
    void ordinary_geometry(const RendererObservedGeometryDraw &value,bool owner_route) {
        if(!camera_shaders.contains(value.source_material[0]))return;
        gpu_require(value.owner_constants==owner_route && value.pipeline && value.descriptor &&
            value.pipeline_handle && value.vertex_buffer && value.count && value.camera_push.size()==112 && !value.material_uniform.empty(),
            "Controlled camera geometry lost its actual command/independent upload facts");
        if(owner_route)++owner_camera_draws;
        draw(value,false);
        auto &row=draws.back();row["owner_route"]=owner_route;row["owner_constants"]=value.owner_constants;
        row["descriptor"]=value.descriptor->handle;row["material_buffer"]=value.descriptor->buffers[0];
        row["vertex_bank_buffer"]=value.descriptor->buffers[1];row["dynamic_offset"]=value.dynamic_offset;
        row["material_uniform_sha256"]=digest(value.material_uniform);row["vertex_bank_sha256"]=digest(value.vertex_bank_uniform);
        row["camera_push_sha256"]=digest(value.camera_push);
    }
    void bind(ModelView &view,bool producer,bool owner_route=false) {
        gpu_require(!view.completed_material_upload_observer && !view.completed_bsp_draw_observer,
            "Texture-camera fixture must not replace another retained observation owner");
        view.completed_material_upload_observer=[this,producer](const auto &v){upload(v,producer);};
        if(producer)view.completed_bsp_draw_observer=[this](const auto &v){draw(v,true);};
        else {
            gpu_require(!view.completed_geometry_draw_observer,"Controlled geometry would replace an existing observation");
            view.completed_geometry_draw_observer=[this,owner_route](const auto &v){ordinary_geometry(v,owner_route);};
            if(owner_route) {
                gpu_require(!view.completed_object_draw_observer,"Controlled object would replace an existing observation");
                // Request the renderer's genuine object ordinal publication.
                view.completed_object_draw_observer=[](const auto &){};
            }
        }
    }
    Json report()const {
        return {{"copy_calls",copy_calls},{"selected_target",selected_target},{"copy_image",copy_image},
            {"copy_view",copy_view},{"material_uploads",uploads},{"vertex_instruction_words",vertex_words},
            {"actual_recursive_viewscreen_draws",scene_camera_draws},
            {"actual_ordinary_viewscreen_draws",ordinary_camera_draws},{"actual_owner_viewscreen_draws",owner_camera_draws},{"uploads",uploaded},{"draws",draws}};
    }
};
} // namespace halo2::texture_camera_test
