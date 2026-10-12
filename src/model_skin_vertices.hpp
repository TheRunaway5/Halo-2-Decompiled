#pragma once
#include "halo2/model.hpp"
#include <cstring>
#if defined(__x86_64__) || defined(_M_X64)
#include <emmintrin.h>
#endif

namespace halo2 {
inline void skin_model_vertices_scalar(std::span<Vertex> vertices,
    std::span<const SkinBinding> bindings,std::span<const std::array<float,12>> skin) {
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    int last = 3;
    while (last >= 0 && bindings[i].weights[unsigned(last)] == 0)
      --last;
    if (last < 0)
      continue; // Authored world-space geometry has no palette influences.
    std::array<float,12> blended{};
    // Source skinned VPs blend matrix rows, highest influence down to x,
    // then perform DPH/DP3, with separately rounded MUL/ADD.
    for (int j = last; j >= 0; --j) {
      const auto bone = bindings[i].bones[unsigned(j)];
      if (bone >= skin.size())
        throw FormatError("pose skin bone outside skeleton");
      const auto weight = bindings[i].weights[unsigned(j)];
      for (unsigned k=0;k<12;++k) {
        const volatile float product=skin[bone][k]*weight;
        blended[k]=j==last?product:blended[k]+product;
      }
    }
    const auto rotate = [&](Vec3 v, bool position) {
      std::array<float, 3> result;
      for (unsigned axis = 0; axis < 3; ++axis) {
        const auto row=axis*4;
        const volatile float x = blended[row] * v.x;
        const volatile float y = blended[row+1] * v.y;
        const volatile float z = blended[row+2] * v.z;
        const float dot = (x + y) + z;
        result[axis] = position ? dot + blended[row+3] : dot;
      }
      return Vec3{result[0], result[1], result[2]};
    };
    vertices[i].position = rotate(vertices[i].position, true);
    vertices[i].normal = rotate(vertices[i].normal, false);
    vertices[i].tangent = rotate(vertices[i].tangent, false);
    vertices[i].binormal = rotate(vertices[i].binormal, false);
  }
}

// The scalar control above preserves the source VP's descending influence
// order and separately rounded MUL/ADD. SSE processes independent rows and
// attributes in parallel; no horizontal sum, reassociation or FMA is used.
inline void skin_model_vertices(std::span<Vertex> vertices,
    std::span<const SkinBinding> bindings,std::span<const std::array<float,12>> skin,bool vectorized=true) {
    if(vertices.size()!=bindings.size())throw FormatError("invalid model skin mapping");
#if defined(__x86_64__) || defined(_M_X64)
    if(vectorized) {
        for(std::size_t i=0;i<vertices.size();++i) {
            const auto &binding=bindings[i];int last=3;
            while(last>=0 && binding.weights[unsigned(last)]==0)--last;
            if(last<0)continue;
            __m128 rows[3];
            for(int j=last;j>=0;--j) {
                const auto bone=binding.bones[unsigned(j)];
                if(bone>=skin.size())throw FormatError("pose skin bone outside skeleton");
                const auto weight=_mm_set1_ps(binding.weights[unsigned(j)]);
                for(unsigned r=0;r<3;++r) {
                    const auto product=_mm_mul_ps(_mm_loadu_ps(skin[bone].data()+r*4),weight);
                    rows[r]=j==last?product:_mm_add_ps(rows[r],product);
                }
            }
            auto &v=vertices[i];
            const auto x=_mm_set_ps(v.binormal.x,v.tangent.x,v.normal.x,v.position.x);
            const auto y=_mm_set_ps(v.binormal.y,v.tangent.y,v.normal.y,v.position.y);
            const auto z=_mm_set_ps(v.binormal.z,v.tangent.z,v.normal.z,v.position.z);
            __m128 result[4];
            for(unsigned r=0;r<3;++r) {
                const auto a=_mm_mul_ps(_mm_shuffle_ps(rows[r],rows[r],_MM_SHUFFLE(0,0,0,0)),x);
                const auto b=_mm_mul_ps(_mm_shuffle_ps(rows[r],rows[r],_MM_SHUFFLE(1,1,1,1)),y);
                const auto c=_mm_mul_ps(_mm_shuffle_ps(rows[r],rows[r],_MM_SHUFFLE(2,2,2,2)),z);
                const auto dot=_mm_add_ps(_mm_add_ps(a,b),c);
                // Only position receives the translation. ADDSS leaves the
                // normal/tangent/binormal lanes, including signed zero, alone.
                result[r]=_mm_add_ss(dot,_mm_shuffle_ps(rows[r],rows[r],_MM_SHUFFLE(3,3,3,3)));
            }
            result[3]=_mm_setzero_ps();
            _MM_TRANSPOSE4_PS(result[0],result[1],result[2],result[3]);
            Vec3 *attributes[]{&v.position,&v.normal,&v.tangent,&v.binormal};
            for(unsigned a=0;a<4;++a) {
                alignas(16) float words[4];_mm_store_ps(words,result[a]);
                static_assert(sizeof(Vec3)==3*sizeof(float));
                std::memcpy(attributes[a],words,sizeof(Vec3));
            }
        }
        return;
    }
#else
    (void)vectorized;
#endif
    skin_model_vertices_scalar(vertices,bindings,skin);
}
}
