#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <span>

namespace halo2::test {
// Necessary display evidence, never an occlusion or surviving-pixel oracle.
// Clip submitted WORLD triangles with their actual source c0..3 constants;
// the acceptance recording still requires visual inspection.
struct SubmittedVehicleProjection {
    double left=std::numeric_limits<double>::infinity(),top=left;
    double right=-left,bottom=-left,triangle_area_pixels{};
    unsigned clipped_triangles{};
    bool readable()const {
        return clipped_triangles && right-left>=16 && bottom-top>=16 && triangle_area_pixels>=64;
    }
    void observe(std::span<const Vertex> vertices,const std::array<ShaderVector,7>& bank,Vec2 viewport) {
        if(!(viewport.x>0 && viewport.y>0))return;
        using Point=std::array<double,4>;
        for(std::size_t triangle=0;triangle+2<vertices.size();triangle+=3) {
            std::array<Point,12> polygon{},next{};unsigned count=3;
            bool finite=true;
            for(unsigned i=0;i<3;++i) {
                const auto p=vertices[triangle+i].position;
                for(unsigned row=0;row<4;++row) {
                    auto& value=polygon[i][row];
                    value=double(bank[row][0])*p.x+double(bank[row][1])*p.y+double(bank[row][2])*p.z+bank[row][3];
                    finite&=std::isfinite(value);
                }
            }
            if(!finite)continue;
            // Positive W and the four viewport edges. Source depth uses the
            // original W-buffer convention; no invented near/far plane here.
            for(unsigned plane=0;plane<5 && count;++plane) {
                const auto distance=[&](const Point& p) {
                    if(!plane)return p[3]-1.e-9;
                    const unsigned component=(plane-1)/2;
                    return p[3]+(plane%2?p[component]:-p[component]);
                };
                unsigned size=0;
                for(unsigned i=0;i<count;++i) {
                    const auto& a=polygon[i];const auto& b=polygon[(i+1)%count];
                    const auto da=distance(a),db=distance(b);
                    if(da>=0)next[size++]=a;
                    if((da>=0)!=(db>=0)) {
                        const auto t=da/(da-db);Point point;
                        for(unsigned j=0;j<4;++j)point[j]=a[j]+t*(b[j]-a[j]);
                        next[size++]=point;
                    }
                }
                count=size;polygon=next;
            }
            if(count<3)continue;
            std::array<std::array<double,2>,12> pixels{};
            double area=0;
            for(unsigned i=0;i<count;++i) {
                pixels[i]={(polygon[i][0]/polygon[i][3]+1)*.5*viewport.x,
                    (1-polygon[i][1]/polygon[i][3])*.5*viewport.y};
            }
            for(unsigned i=0;i<count;++i) {
                const auto& a=pixels[i];const auto& b=pixels[(i+1)%count];
                area+=a[0]*b[1]-a[1]*b[0];
            }
            area=std::abs(area)*.5;if(!(area>1.e-6))continue;
            ++clipped_triangles;triangle_area_pixels+=area;
            for(unsigned i=0;i<count;++i) {
                left=std::min(left,pixels[i][0]);right=std::max(right,pixels[i][0]);
                top=std::min(top,pixels[i][1]);bottom=std::max(bottom,pixels[i][1]);
            }
        }
    }
};
}
