#pragma once
#include "halo2/renderer.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
namespace halo2_test {
// Declared isolation inputs, not replacement retail programs. The existing
// production world programs decode these MOV/combiner words and route all
// four asymmetric oT outputs to their own PROJECT2D stage.
inline unsigned append_coordinate_probe(halo2::Mesh& mesh) {
    const auto base=unsigned(mesh.materials.size());
    const halo2::Texture pattern{2,2,11,{std::byte{255},std::byte{0},std::byte{0},std::byte{255},
        std::byte{0},std::byte{255},std::byte{0},std::byte{255},
        std::byte{0},std::byte{0},std::byte{255},std::byte{255},
        std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};
    for(unsigned selected=0;selected<4;++selected) {
        halo2::Material m;m.forward_pass=true;m.forward_vertex=3;m.forward_blend=0;
        m.forward_raster.depth_enable=0;m.forward_raster.depth_write=0;m.forward_raster.cull=0;
        m.diffuse=m.detail=m.bump=m.auxiliary=pattern;
        m.forward_program.words[8]=8+selected; // Final D = selected texture RGB.
        m.forward_program.words[9]=0x2000; // Final G = unsigned-inverted ZERO (one).
        m.forward_program.words[54]=0x8421; // PROJECT2D in all four stages.
        m.program_constants[1][0]={.25f,.25f,0,1};m.program_constants[1][1]={.75f,.25f,0,1};
        m.program_constants[1][2]={.25f,.75f,0,1};m.program_constants[1][3]={.75f,.75f,0,1};
        m.vertex_program.resize(4+5*16);
        const auto put=[&](unsigned at,std::uint32_t value,unsigned bytes=4) {
            for(unsigned i=0;i<bytes;++i)m.vertex_program[at+i]=std::byte(value>>(i*8));
        };
        put(0,0x2078,2);put(2,5,2);
        for(unsigned instruction=0;instruction<5;++instruction) {
            const auto output=instruction?8+instruction:0;
            const auto source=instruction?3u:2u;
            const auto constant=instruction?17+instruction:0;
            put(4+instruction*16+4,(1u<<21)|(constant<<13)|0x1b);
            put(4+instruction*16+8,source<<26);
            put(4+instruction*16+12,0xf800|(output<<3)|(instruction==4?1:0));
        }
        mesh.materials.push_back(std::move(m));
    }
    return base;
}
inline nlohmann::json coordinate_gpu_probe(halo2::Renderer& renderer,unsigned base,
    const std::filesystem::path& directory) {
    nlohmann::json result=nlohmann::json::array();
    const std::array<std::array<unsigned char,3>,4> colors{{{255,0,0},{0,255,0},{0,0,255},{255,255,255}}};
    renderer.scene_clear_color({});
    for(unsigned stage=0;stage<4;++stage) {
        std::vector<halo2::Vertex> vertices;
        for(const auto p:std::array<halo2::Vec2,6>{{{64,64},{576,64},{576,416},{64,64},{576,416},{64,416}}}) {
            halo2::Vertex v;v.position={p.x,p.y,65536};v.color={1,1,1};v.material=base+stage;vertices.push_back(v);
        }
        halo2::ModelView view;view.width=640;view.height=480;view.full_surface=true;view.vertices=vertices;
        const auto path=directory/("coordinate-"+std::to_string(stage)+".ppm");
        if(!renderer.draw(view.camera,path,{},false,{},{},{},std::span(&view,1)))
            throw std::runtime_error("Native coordinate probe draw failed");
        std::ifstream input(path,std::ios::binary);std::string magic;unsigned width,height,maximum;
        input>>magic>>width>>height>>maximum;input.get();
        if(magic!="P6" || width!=640 || height!=480 || maximum!=255)
            throw std::runtime_error("Native coordinate readback extent differs");
        std::vector<unsigned char> pixels(640*480*3);input.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
        if(!input)throw std::runtime_error("Native coordinate readback is truncated");
        unsigned checked=0;
        for(unsigned y=80;y<400;++y)for(unsigned x=80;x<560;++x) {
            const auto at=(y*640+x)*3;
            for(unsigned c=0;c<3;++c)if(pixels[at+c]!=colors[stage][c])
                throw std::runtime_error("Native programmable coordinate stage "+std::to_string(stage)+" differs: "+
                    std::to_string(pixels[at])+","+std::to_string(pixels[at+1])+","+std::to_string(pixels[at+2]));
            ++checked;
        }
        result.push_back({{"stage",stage},{"output_register",9+stage},{"expected_rgb",colors[stage]},
            {"checked_pixels",checked},{"capture",path.string()}});
    }
    return result;
}
}
