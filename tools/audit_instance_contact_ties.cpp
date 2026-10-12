#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/math.hpp"
#include "halo2/transform.hpp"
#include <iostream>
#include <iomanip>
#include <set>
#include <bit>
using namespace halo2;
int main(int argc,char**argv){try{std::cout<<std::setprecision(9);
for(int m=1;m<argc;++m){Cache cache(argv[m]);auto b=cache.load_bsp_surface_lighting();std::size_t rays=0,ties=0,rejects=0,strict_equal_hits=0,spawns=0;
auto check=[&](Vec3 p,Vec3 d,const char*kind,int source,int face){++rays;
auto main=original_bsp_vector_query(b.collision[0],p,d);std::set<unsigned> eligible;
for(auto leaf:main.leaves){auto c=b.collision[0].leaves.at(leaf).cluster;if(c>=0)for(auto i:b.cluster_instances.at(c))eligible.insert(i);}
float closest=main.contact?main.contact->fraction:1;std::vector<MapVectorContact> hits;
for(auto i:eligible){auto h=original_instance_vector_contact(b,i,p,d);if(h){if(h->contact.fraction<closest){closest=h->contact.fraction;hits.clear();}if(h->contact.fraction==closest)hits.push_back(*h);}}
bool unresolved=false;auto native=original_map_vector_contact(b,p,d,&unresolved);rejects+=unresolved;
if(hits.size()>1){++ties;for(auto h:hits)strict_equal_hits+=bool(original_instance_vector_contact(b,std::uint16_t(h.source.instance),p,d,closest));
std::cout<<"TIE "<<cache.name()<<' '<<kind<<' '<<source<<' '<<face<<' '<<p.x<<' '<<p.y<<' '<<p.z<<' '<<d.x<<' '<<d.y<<' '<<d.z<<' '<<std::bit_cast<std::uint32_t>(closest)<<' '<<unresolved<<' '<<(native?native->source.instance:-99);
for(auto h:hits)std::cout<<' '<<h.source.instance<<':'<<h.source.surface<<':'<<h.contact.leaf;std::cout<<'\n';}
};
const Vec3 bank[]{{0,0,-10},{-10,0,0},{10,0,0},{0,-10,0},{0,10,0},{0,0,-40}};
for(auto s:cache.spawns()){for(auto d:bank){check(s.position-d*.001f,d,"spawn",int(spawns),-1);}++spawns;}
for(unsigned i=0;i<b.instances.size();++i){const auto& in=b.instances[i];const auto& c=b.definition_collision.at(in.definition);Transform root{in.scale,in.axes,in.translation};
for(unsigned si=0;si<c.surfaces.size();si+=std::max(1u,unsigned(c.surfaces.size()/12))){const auto&s=c.surfaces[si];if(s.flags&18)continue;auto e=s.edge;Vec3 p{};unsigned count=0;do{const auto&edge=c.edges.at(e);auto right=unsigned(edge.surface[1]==int(si));p=p+c.vertices.at(edge.vertex[right]);e=edge.next[right];if(++count>c.edges.size())throw std::runtime_error("cycle");}while(e!=s.edge);if(count<3)continue;p=p/float(count);
auto plane=c.planes.at(s.plane&0x7fff);Vec3 n{plane[0],plane[1],plane[2]};if(s.plane&0x8000)n=n*-1.f;
for(float height:{.1f,1.f}){auto rotate=[&](Vec3 q){if(root.scale!=1)q=q*root.scale;return Vec3{(root.basis[2].x*q.z+root.basis[1].x*q.y)+root.basis[0].x*q.x,(root.basis[2].y*q.z+root.basis[1].y*q.y)+root.basis[0].y*q.x,(root.basis[2].z*q.z+root.basis[1].z*q.y)+root.basis[0].z*q.x};};auto origin=rotate(p+n*height)+root.position;auto ray=rotate(n*(-height-2));check(origin,ray,"face",i,si);}
}
}
std::cerr<<cache.name()<<" instances="<<b.instances.size()<<" rays="<<rays<<" ties="<<ties<<" rejected="<<rejects<<" exact_max_hits="<<strict_equal_hits<<'\n';
}
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
