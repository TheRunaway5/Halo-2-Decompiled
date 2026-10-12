#include "halo2/object_light_spatial.hpp"
#include <bit>
#include <iostream>

using namespace halo2;
namespace {
void require(bool test,const char* message) {if(!test)throw std::runtime_error(message);}
float fword() {std::uint32_t value;std::cin>>value;return std::bit_cast<float>(value);}
Vec3 vector() {const float x=fword(),y=fword(),z=fword();return {x,y,z};}
void print(const ObjectLightSpatialMembership& m) {
    std::cout<<m.total<<' '<<m.truncated<<' '<<m.clusters.size();
    for(auto c:m.clusters)std::cout<<' '<<c;std::cout<<'\n';
}
}
int main(int argc,char** argv) {try {
    if(argc==3 && std::string(argv[1])=="--definitions") {
        const Cache cache(argv[2]);std::uint32_t datum;
        while(std::cin>>datum) {
            const auto d=cache.load_object_light_spatial_definition(datum);std::cout<<d.flags;
            for(float f:{d.dynamic_radius,d.dynamic_offset.x,d.dynamic_offset.y,d.dynamic_offset.z})
                std::cout<<' '<<std::bit_cast<std::uint32_t>(f);
            std::cout<<'\n';
        }
        require(std::cin.eof(),"invalid spatial definition fixture");return 0;
    }
    if(argc==2 && std::string(argv[1])=="--bounds") {
        unsigned flags,registered;
        while(std::cin>>flags) {
            ObjectLightSpatialDefinition d;d.flags=std::uint16_t(flags);d.dynamic_radius=fword();d.dynamic_offset=vector();
            ObjectLightSpatialSphere query;query.center=vector();query.radius=fword();const float scale=fword();
            ObjectLightSpatialSphere previous;previous.center=vector();previous.radius=fword();std::cin>>registered;
            require(bool(std::cin),"truncated source spatial bounds fixture");
            const auto bounds=source_object_light_spatial_bounds(d,query,scale);
            for(const auto s:{bounds.dynamic,bounds.primitive})for(float f:{s.center.x,s.center.y,s.center.z,s.radius})
                std::cout<<std::bit_cast<std::uint32_t>(f)<<' ';
            std::cout<<source_object_light_spatial_bounds_changed(previous,bounds.primitive,registered)<<'\n';
        }
        require(std::cin.eof(),"invalid source spatial bounds fixture");return 0;
    }
    if(argc==3 && std::string(argv[1])=="--portal") {
        const auto bank=Cache(argv[2]).load_object_light_spatial_bsp();unsigned index;
        while(std::cin>>index) {const auto p=vector();const float r=fword();
            require(bool(std::cin),"truncated spatial portal fixture");
            std::cout<<source_object_light_portal_intersects(bank,std::uint16_t(index),p,r)<<'\n';}
        require(std::cin.eof(),"invalid spatial portal fixture");return 0;
    }
    if(argc==3 && std::string(argv[1])=="--membership") {
        const auto bank=Cache(argv[2]).load_object_light_spatial_bsp();std::int16_t cluster;
        while(std::cin>>cluster) {
            const auto p=vector();const float r=fword();unsigned count;std::cin>>count;
            std::vector<std::uint32_t> forced(count);for(auto& word:forced)std::cin>>word;
            require(bool(std::cin),"truncated spatial membership fixture");
            print(source_object_light_spatial_membership(bank,cluster,p,r,forced));
        }
        require(std::cin.eof(),"invalid spatial membership fixture");return 0;
    }
    if(argc==3 && std::string(argv[1])=="--metadata") {
        const auto bank=Cache(argv[2]).load_object_light_spatial_bsp();
        std::cout<<bank.planes.size();
        for(auto p:bank.planes)for(auto v:p)std::cout<<' '<<std::bit_cast<std::uint32_t>(v);
        std::cout<<' '<<bank.clusters.size();
        for(const auto& cluster:bank.clusters) {std::cout<<' '<<cluster.size();for(auto p:cluster)std::cout<<' '<<p;}
        std::cout<<' '<<bank.portals.size();
        for(const auto& p:bank.portals) {
            std::cout<<' '<<p.back<<' '<<p.front<<' '<<p.plane;
            for(float v:{p.center.x,p.center.y,p.center.z,p.radius})std::cout<<' '<<std::bit_cast<std::uint32_t>(v);
            std::cout<<' '<<p.vertices.size();
            for(auto v:p.vertices)for(float f:{v.x,v.y,v.z})std::cout<<' '<<std::bit_cast<std::uint32_t>(f);
        }
        std::cout<<'\n';return 0;
    }
    if(argc==2 && std::string(argv[1])=="--segment") {
        unsigned dummy;
        while(std::cin>>dummy) {const auto origin=vector(),delta=vector(),center=vector();const float radius=fword();
            require(bool(std::cin),"truncated spatial segment fixture");
            std::cout<<source_object_light_segment_intersects(origin,delta,center,radius)<<'\n';}
        require(std::cin.eof(),"invalid spatial segment fixture");return 0;
    }
    if(argc==2 && std::string(argv[1])=="--lists") {
        unsigned count;std::cin>>count;ObjectLightSpatialLists bank(count);
        unsigned op;
        while(std::cin>>op) {
            std::uint32_t object;std::cin>>object;
            if(op==2)bank.erase(object);
            else if(op==3) {
                const auto records=bank.cluster(std::int16_t(object));std::cout<<records.size();
                for(const auto& r:records) {
                    std::cout<<' '<<r.object<<' '<<r.primitive.type<<' '<<r.primitive.flags;
                    for(float v:{r.primitive.center.x,r.primitive.center.y,r.primitive.center.z,r.primitive.radius})
                        std::cout<<' '<<std::bit_cast<std::uint32_t>(v);
                }
                std::cout<<'\n';
            } else {
                ObjectLightSpatialPrimitive p;std::cin>>p.type>>p.flags;p.center=vector();p.radius=fword();
                if(op==1)bank.update(object,p);
                else {
                    unsigned n;std::cin>>n;std::vector<std::int16_t> members(n);for(auto& c:members)std::cin>>c;
                    bank.insert(object,p,members);
                }
            }
            require(bool(std::cin),"truncated spatial list fixture");
        }
        require(std::cin.eof(),"invalid spatial list fixture");return 0;
    }
    if(argc==2 && std::string(argv[1])=="--scan") {
        unsigned clusters,records;std::cin>>clusters>>records;ObjectLightSpatialLists bank(clusters);
        for(unsigned i=0;i<records;++i) {
            std::uint32_t object;ObjectLightSpatialPrimitive p;unsigned count;
            std::cin>>object>>p.type>>p.flags;p.center=vector();p.radius=fword();std::cin>>count;
            std::vector<std::int16_t> memberships(count);for(auto& c:memberships)std::cin>>c;
            bank.insert(object,p,memberships);
        }
        std::uint32_t query,excluded_a,excluded_b;unsigned initial,count,success_object;int success_cluster;
        while(std::cin>>query>>initial>>excluded_a>>excluded_b) {
            const auto origin=vector(),delta=vector();std::cin>>count;
            std::vector<std::int16_t> leaves(count);for(auto& c:leaves)std::cin>>c;
            std::cin>>success_cluster>>success_object;require(bool(std::cin),"truncated source spatial scan fixture");
            std::vector<std::pair<unsigned,std::uint32_t>> calls;
            const bool hit=source_object_light_spatial_scan(bank,leaves,query,origin,delta,excluded_a,excluded_b,
                initial,[&](std::int16_t cluster) {calls.emplace_back(0,cluster);return cluster==success_cluster;},
                [&](const ObjectLightSpatialRecord& r) {calls.emplace_back(1,r.object);return r.object==success_object;});
            std::cout<<hit<<' '<<calls.size();for(auto [kind,id]:calls)std::cout<<' '<<kind<<' '<<id;
            std::cout<<'\n';
        }
        require(std::cin.eof(),"invalid source spatial scan fixture");return 0;
    }
    require(argc==1,"unknown source spatial fixture mode");
    ObjectLightSpatialDefinition definition{0x1000,2,{1,2,3}};
    const auto bounds=source_object_light_spatial_bounds(definition,{{10,20,30},4},.5f);
    require(bounds.primitive.center.x==11 && bounds.primitive.center.y==22 &&
        bounds.primitive.center.z==33 && bounds.primitive.radius==1,
        "source primitive offset/scale selection changed");
    require(!source_object_light_spatial_bounds_changed({{},1},{{.1f,0,0},1},true),
        "source retained primitive motion threshold changed");
    require(source_object_light_spatial_bounds_changed({{},1},{{.10000001f,0,0},1},true),
        "source registered primitive reinsertion lost");
    ObjectLightSpatialBsp bank;bank.planes.push_back({1,0,0,0});bank.clusters={{0},{0}};
    bank.portals.push_back({0,1,0,{},2,{{0,-1,-1},{0,1,-1},{0,1,1},{0,-1,1}}});
    require(source_object_light_spatial_membership(bank,0,{},0).clusters==std::vector<std::int16_t>{0},
        "source zero-radius registration crossed portal");
    require(source_object_light_spatial_membership(bank,0,{},.25f).clusters==std::vector<std::int16_t>{0,1},
        "source sphere registration lost adjacent cluster");
    require(!source_object_light_portal_intersects(bank,0,{.25f,0,0},.25f),
        "source sphere plane tangency was admitted");
    ObjectLightSpatialLists lists(2);ObjectLightSpatialPrimitive p{6,12,{},1};
    const std::array<std::int16_t,2> both{0,1};lists.insert(1,p,both);lists.insert(2,p,both);
    p.radius=3;lists.update(1,p);
    require(lists.cluster(0)[0].object==2 && lists.cluster(0)[1].object==1 &&
        lists.cluster(0)[1].primitive.radius==3,"source update changed spatial list ordering");
    lists.erase(1);lists.insert(1,p,both);
    require(lists.cluster(0)[0].object==1 && lists.cluster(1)[0].object==1,
        "source reinsertion did not prepend");
    const std::array<std::int16_t,3> leaves{1,0,1};std::vector<unsigned> calls;
    require(!source_object_light_spatial_scan(lists,leaves,0x04800c0du,{},
        {0,0,-10},0xffffffffu,0xffffffffu,false,
        [&](std::int16_t c){calls.push_back(unsigned(c)+10);return false;},
        [&](const ObjectLightSpatialRecord& r){calls.push_back(r.object);return false;}),
        "source contact dispatcher invented a hit");
    require(calls==std::vector<unsigned>{11,1,2,10},
        "source per-cluster contact interleaving/stamps changed");
    std::cout<<"Source object spatial membership/list fixtures passed\n";return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
