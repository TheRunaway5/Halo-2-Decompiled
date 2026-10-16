#include "halo2/pstencil/geometry.hpp"
#include "halo2/pstencil/shadow.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2::pstencil {
VertexProgram position_prefix(const VertexProgram& program) {
    for(unsigned i=0;i<program.count;++i) {
        const auto& t=program.instructions[i];
        const auto mac=(t[1]>>21)&15u;
        // Authored world projection: DPH/DP4 oPos.x, world, c0. Capture
        // the actual source operand before projection, retaining every
        // preceding decompression, relative palette read and skin operation.
        if((mac==6 || mac==7) && ((t[2]>>11)&3u)==3 &&
           ((t[1]>>13)&255u)==0 && !(t[3]&2u) &&
           (t[3]&0xf800u)==0x8800u && ((t[3]>>3)&15u)==0 && !(t[3]&4u)) {
            auto prefix=program;prefix.count=i+1;prefix.relative_constants=false;
            for(unsigned n=0;n<i;++n)prefix.instructions[n][3]&=~1u;
            auto& probe=prefix.instructions[i];
            probe[1]=(t[1]&0x1fffffu)|(1u<<21); // MOV, same source A.
            probe[3]=0xf800u|(9u<<3)|1u; // Full oT0, last instruction.
            return prefix;
        }
    }
    throw FormatError("pStencil position adapter lacks an authored world/c0 projection boundary");
}
namespace {
unsigned components(MenuWorldSourceFormat format) {
    switch(format) {
    case MenuWorldSourceFormat::float32x3:case MenuWorldSourceFormat::snorm16x3:return 3;
    case MenuWorldSourceFormat::float32x2:case MenuWorldSourceFormat::snorm16x2:return 2;
    case MenuWorldSourceFormat::unorm8x1:return 1;
    case MenuWorldSourceFormat::unorm8x2:return 2;
    case MenuWorldSourceFormat::unorm8x3:return 3;
    case MenuWorldSourceFormat::unorm8x4:return 4;
    case MenuWorldSourceFormat::packed_signed_11_11_10:return 3;
    }
    throw FormatError("pStencil unsupported raw vertex format");
}
ShaderVector fetch(const MenuWorldSourceBinding& binding,unsigned input,unsigned vertex) {
    const auto& stream=binding.inputs()[input].stream;
    const auto raw=binding.resource_bytes(input);
    const Bytes bytes(raw);
    const auto offset=std::size_t(vertex)*stream.byte_stride+stream.byte_offset;
    ShaderVector result{0,0,0,1};
    if(stream.format==MenuWorldSourceFormat::packed_signed_11_11_10) {
        const auto packed=bytes.u32(offset);
        const auto signed_bits=[&](unsigned first,unsigned width) {
            const auto bits=(packed>>first)&((1u<<width)-1u);
            return std::bit_cast<std::int32_t>(bits<<(32-width))>>(32-width);
        };
        result[0]=float(signed_bits(0,11))/1023.f;
        result[1]=float(signed_bits(11,11))/1023.f;
        result[2]=float(signed_bits(22,10))/511.f;
        return result;
    }
    for(unsigned i=0;i<components(stream.format);++i) {
        if(stream.format==MenuWorldSourceFormat::float32x3 || stream.format==MenuWorldSourceFormat::float32x2)
            result[i]=bytes.f32(offset+i*4);
        else if(stream.format==MenuWorldSourceFormat::snorm16x3 || stream.format==MenuWorldSourceFormat::snorm16x2)
            result[i]=std::max(-1.f,float(std::bit_cast<std::int16_t>(bytes.u16(offset+i*2)))/32767.f);
        else {bytes.require(offset+i,1);result[i]=float(std::to_integer<unsigned>(raw[offset+i]))/255.f;}
    }
    return result;
}
bool finite(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
}
std::vector<Vec3> world_positions(const MenuWorldSourceBinding& binding) {
    const auto program=position_prefix(binding.program());
    std::vector<Vec3> out(binding.draw().geometry->vertex_count());
    std::vector<bool> used(out.size());
    for(const auto index:binding.draw().indices()) {
        if(index>=out.size())throw FormatError("pStencil raw index escapes vertex bank");
        if(used[index])continue;
        VertexProgramState state;state.constants=binding.constants();
        for(auto& attribute:state.attributes)attribute={0,0,0,1};
        for(unsigned input=0;input<binding.inputs().size();++input)
            state.attributes[binding.inputs()[input].vertex_register]=fetch(binding,input,index);
        execute_vertex_program(program,state);
        const auto& position=state.outputs[9];out[index]={position[0],position[1],position[2]};
        if(!finite(out[index]))throw FormatError("pStencil source position is nonfinite");
        used[index]=true;
    }
    return out;
}
void append_volume(std::vector<ShadowVertex>& output,const MenuWorldSourceBinding& binding,Vec3 light,unsigned stipple_level) {
    const auto points=world_positions(binding);
    std::vector<std::array<std::uint16_t,3>> triangles;
    triangles.reserve(binding.draw().triangle_count());
    for(std::size_t i=0;i<binding.draw().triangle_count();++i)triangles.push_back(binding.draw().triangle(i));
    append_volume(output,points,triangles,light,stipple_level);
}
} // namespace halo2::pstencil

namespace halo2::pstencil {
std::vector<Vertex> posed_triangles(const MenuWorldSourceBinding& binding,unsigned material) {
    const auto points=world_positions(binding);const auto classification=binding.declaration().class_index;
    const auto semantic=[&](unsigned value)->std::optional<unsigned> {
        const auto& values=binding.declaration().semantics;
        const auto at=std::find(values.begin(),values.end(),value);
        if(at==values.end())return {};return unsigned(at-values.begin());
    };
    const auto byte=[&](unsigned input,unsigned index,unsigned component) {
        const auto& stream=binding.inputs().at(input).stream;const auto bytes=binding.resource_bytes(input);
        const auto offset=std::size_t(index)*stream.byte_stride+stream.byte_offset+component;
        if(offset>=bytes.size())throw FormatError("pStencil influence escapes raw source stream");
        return std::to_integer<std::uint8_t>(bytes[offset]);
    };
    std::vector<Vertex> vertices;vertices.reserve(binding.draw().triangle_count()*3);
    for(std::size_t triangle=0;triangle<binding.draw().triangle_count();++triangle)
      for(const auto index:binding.draw().triangle(triangle)) {
        Vertex vertex;vertex.position=points.at(index);vertex.material=material;
        vertex.source_section=binding.draw().geometry->section();vertex.source_part=binding.draw().part;
        const auto& part=binding.draw().geometry->parts().at(binding.draw().part);
        vertex.source_part_type=part.type;vertex.source_part_flags=part.flags;
        if(const auto uv=semantic(3)) {
            const auto input=fetch(binding,*uv,index);const auto& stream=binding.inputs()[*uv].stream;
            const auto& c=binding.constants()[172];
            vertex.uv=stream.format==MenuWorldSourceFormat::snorm16x2?
                Vec2{input[0]*c[0]+c[2],input[1]*c[1]+c[3]}:Vec2{input[0],input[1]};
        }
        std::array<float,12> rows{};
        if(classification) {
            std::array<unsigned,4> indices{};std::array<float,4> weights{1,0,0,0};
            const unsigned count=classification>=3?classification-2:1;
            if(classification>=2) {
                const auto input=semantic(1);if(!input)throw FormatError("pStencil skin indices absent");
                for(unsigned n=0;n<count;++n)indices[n]=byte(*input,index,n);
            }
            if(count>1) {
                const auto input=semantic(2);if(!input)throw FormatError("pStencil skin weights absent");
                for(unsigned n=1;n<count;++n){weights[n]=float(byte(*input,index,n))/255.f;weights[0]-=weights[n];}
            }
            for(int n=int(count)-1;n>=0;--n) {
                if(indices[unsigned(n)]>=binding.draw().object_rows.size())throw FormatError("pStencil skin influence escapes current palette");
                for(unsigned k=0;k<12;++k) {
                    const volatile float product=binding.draw().object_rows[indices[unsigned(n)]][k]*weights[unsigned(n)];
                    rows[k]=n==int(count)-1?product:rows[k]+product;
                }
            }
        }
        const auto basis=[&](unsigned value) {
            const auto input=semantic(value);if(!input)throw FormatError("pStencil specular basis is absent");
            const auto raw=fetch(binding,*input,index);Vec3 out{raw[0],raw[1],raw[2]};
            if(classification) {
                std::array<float,3> result{};
                for(unsigned axis=0;axis<3;++axis) {
                    const volatile float x=rows[axis*4]*out.x,y=rows[axis*4+1]*out.y,z=rows[axis*4+2]*out.z;
                    result[axis]=(x+y)+z;
                }
                out={result[0],result[1],result[2]};
            }
            return out;
        };
        vertex.normal=basis(4);vertex.tangent=basis(5);vertex.binormal=basis(6);
        vertices.push_back(vertex);
      }
    return vertices;
}
}
