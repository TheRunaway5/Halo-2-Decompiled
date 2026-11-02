#pragma once
#include "halo2/transform.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <span>
#include <vector>

namespace halo2::test_reference {
// Independent scalar reference for the per-WORLD-node PC body policy. No GLM,
// hierarchy inversion, production interpolation or shared quaternion helper.
// Source family: pnill/cartographer d978132e, matrix_math.cpp378..400 and
// real_math.cpp159..169,248..258. This does not claim retail MCC arithmetic.
inline Transform first_person_world_matrix(const Transform& a,const Transform& b,float f) {
    if(!std::isfinite(f) || f<0 || f>1)throw FormatError("Invalid body reference fraction");
    if(f==0)return a;
    if(f==1)return b;
    const auto parity=[](const Transform& t) {
        const auto& x=t.basis[0];const auto& y=t.basis[1];const auto& z=t.basis[2];
        return (x.y*y.z-x.z*y.y)*z.x+(x.z*y.x-x.x*y.z)*z.y+(x.x*y.y-x.y*y.x)*z.z<0;
    };
    const auto q_from_matrix=[](const Transform& t,bool mirror) {
        std::array<std::array<double,3>,3> m{};
        for(unsigned column=0;column<3;++column) {
            const auto v=t.basis[column];const double sign=mirror&&column==1?-1:1;
            m[column]={v.x*sign,v.y*sign,v.z*sign};
        }
        std::array<double,4> q{};
        const auto trace=m[0][0]+m[1][1]+m[2][2];
        if(trace>0) {
            q[3]=std::sqrt(1+trace)*.5;const auto s=.25/q[3];
            q[0]=(m[1][2]-m[2][1])*s;q[1]=(m[2][0]-m[0][2])*s;q[2]=(m[0][1]-m[1][0])*s;
        }else {
            unsigned i=m[0][0]<m[1][1]?1:0;if(m[i][i]<m[2][2])i=2;
            const unsigned j=(i+1)%3,k=(i+2)%3;
            q[i]=std::sqrt(1+m[i][i]-m[j][j]-m[k][k])*.5;const auto s=.25/q[i];
            q[3]=(m[j][k]-m[k][j])*s;q[j]=(m[i][j]+m[j][i])*s;q[k]=(m[i][k]+m[k][i])*s;
        }
        if(q[3]<0)for(auto& value:q)value=-value;
        return q;
    };
    Transform result=b;
    const auto mirror_a=parity(a),mirror_b=parity(b);
    if(mirror_a==mirror_b && std::memcmp(a.basis.data(),b.basis.data(),sizeof(a.basis))) {
        const auto qa=q_from_matrix(a,mirror_a),qb=q_from_matrix(b,mirror_b);
        double product{};for(unsigned i=0;i<4;++i)product+=qa[i]*qb[i];
        const double sign=product<0?-1:1;
        std::array<double,4> q{};double square{};
        for(unsigned i=0;i<4;++i){q[i]=qa[i]*(1-double(f))+qb[i]*(sign*double(f));square+=q[i]*q[i];}
        const double s=2/square,x=q[0],y=q[1],z=q[2],w=q[3];
        result.basis={Vec3{float(1-s*(y*y+z*z)),float(s*(x*y+w*z)),float(s*(x*z-w*y))},
            Vec3{float(s*(x*y-w*z)),float(1-s*(x*x+z*z)),float(s*(y*z+w*x))},
            Vec3{float(s*(x*z+w*y)),float(s*(y*z-w*x)),float(1-s*(x*x+y*y))}};
        if(mirror_b)result.basis[1]=result.basis[1]*-1.f;
    }
    const float before=1.f-f;
    result.position={a.position.x*before+b.position.x*f,a.position.y*before+b.position.y*f,a.position.z*before+b.position.z*f};
    result.scale=a.scale*before+b.scale*f;
    return result;
}
inline std::vector<Transform> first_person_world_nodes(std::span<const Transform> a,
    std::span<const Transform> b,float fraction) {
    if(a.size()!=b.size())throw FormatError("Body reference changed its native node bank");
    std::vector<Transform> result;result.reserve(a.size());
    for(unsigned n=0;n<a.size();++n)result.push_back(first_person_world_matrix(a[n],b[n],fraction));
    return result;
}
}
