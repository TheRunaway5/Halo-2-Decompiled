#include "halo2/collision.hpp"
#include <bit>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
bool same(float a,float b) {return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b) {return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
void compare(const std::optional<RayHit>& a,const std::optional<RayHit>& b) {
  check(bool(a)==bool(b),"Layered collision changed ray admission");
  if(!a)return;
  check(same(a->distance,b->distance)&&same(a->position,b->position)&&same(a->normal,b->normal),
        "Layered collision changed ray arithmetic");
  check(a->triangle==b->triangle&&a->source.bsp==b->source.bsp&&a->source.record==b->source.record&&
        a->source.surface==b->source.surface&&a->source.instance==b->source.instance&&
        a->source.plane==b->source.plane&&a->surface_flags==b->surface_flags&&
        a->backface==b->backface&&a->material==b->material,"Layered collision changed ray identity or ties");
}
void compare(const Body& a,const Body& b) {
  check(same(a.position,b.position)&&same(a.velocity,b.velocity)&&a.grounded==b.grounded&&
        same(a.support_normal,b.support_normal)&&a.support_triangle==b.support_triangle&&
        same(a.landing_speed,b.landing_speed),"Layered collision changed capsule contacts");
}
std::vector<CollisionTriangle> mesh(unsigned first,unsigned count,float height) {
  std::vector<CollisionTriangle> result;
  for(unsigned i=first;i<first+count;++i) {
    const auto x=float(i%4)-2,y=float((i/4)%4)-2;
    CollisionTriangle t{{x,y,height},{x+1,y,height},{x,y+1,height}};
    t.source={int(first),int(i/4),int(i),int(i%3),std::uint16_t(i)};
    t.material=std::uint16_t(i);t.flags=std::uint8_t(i);t.source_normal={0,0,1};
    result.push_back(t);
    // Coincident faces deliberately exercise last-hit selection and BVH ties.
    t.source.surface+=1000;t.material+=1000;result.push_back(t);
  }
  return result;
}
void exercise(const CollisionWorld &composed,const CollisionWorld &flat) {
  for(int x=-8;x<=8;++x)for(int y=-8;y<=8;++y) {
    const Vec3 origin{float(x)/4,float(y)/4,3};
    compare(composed.raycast(origin,{0,0,-1},6),flat.raycast(origin,{0,0,-1},6));
    compare(composed.raycast(origin,{.5f,.3f,-1},6),flat.raycast(origin,{.5f,.3f,-1},6));
    check(composed.overlaps({origin.x,origin.y,-.1f},1.7f,.25f)==
          flat.overlaps({origin.x,origin.y,-.1f},1.7f,.25f),"Layered collision changed overlap");
    Body a{{origin.x,origin.y,.1f},{.3f,-.2f,-2},1.7f,.25f},b=a;
    for(unsigned tick=0;tick<12;++tick) {
      composed.move(a,1.f/30);flat.move(b,1.f/30);compare(a,b);
    }
  }
}
}
int main() {
  try {
    const auto bsp=mesh(0,16,0),scenery=mesh(16,7,.3f),devices=mesh(23,5,.6f),props=mesh(28,8,.9f);
    std::vector<CollisionTriangle> fixed=bsp;fixed.insert(fixed.end(),scenery.begin(),scenery.end());
    fixed.insert(fixed.end(),devices.begin(),devices.end());
    auto all=fixed;all.insert(all.end(),props.begin(),props.end());
    std::shared_ptr<const CollisionWorld> base=std::make_shared<const CollisionWorld>(bsp);
    auto views=CollisionWorld::compose(base,{scenery,devices,props},{.exclusive_bsp=true});
    check(views.bsp->triangles().data()==views.fixed->triangles().data()&&
          views.fixed->triangles().data()==views.all->triangles().data(),"Collision layers duplicated their BSP prefix");
    check(views.bsp->size()==bsp.size()&&views.fixed->size()==fixed.size()&&views.all->size()==all.size(),
          "Collision layers exposed another layer's triangles");
    exercise(*views.bsp,CollisionWorld(bsp));exercise(*views.fixed,CollisionWorld(fixed));
    exercise(*views.all,CollisionWorld(all));
    base.reset();
    const auto storage=views.bsp->triangles().data();
    auto next=CollisionWorld::compose(views.bsp,{scenery,devices,props},
        {std::move(views.fixed),std::move(views.all),false,true});
    check(next.bsp->triangles().data()==storage,"Exclusive collision composition copied its warm BSP bank");
    views.bsp.reset();
    // Both enclosing-owner aliases and independently copied CollisionWorld
    // values must preserve the entire old snapshot, including prop geometry.
    const auto snapshot=next.all;
    const CollisionWorld copied=*next.all;
    auto moved_props=props;for(auto& t:moved_props){t.a.z+=1;t.b.z+=1;t.c.z+=1;}
    auto changed=CollisionWorld::compose(next.bsp,{scenery,devices,moved_props},
        {std::move(next.fixed),std::move(next.all),false,true});
    check(changed.all->triangles().data()!=snapshot->triangles().data(),"Retained collision snapshot was overwritten");
    exercise(*snapshot,CollisionWorld(all));exercise(copied,CollisionWorld(all));
    auto changed_all=fixed;changed_all.insert(changed_all.end(),moved_props.begin(),moved_props.end());
    exercise(*changed.all,CollisionWorld(changed_all));
    auto checkpoint_base=changed.bsp;
    auto restored=CollisionWorld::compose(changed.bsp,{scenery,devices,props},
        {std::move(changed.fixed),std::move(changed.all),true,false});
    check(restored.bsp->triangles().data()!=checkpoint_base->triangles().data(),
          "Enclosing checkpoint alias did not force copy-on-write");
    exercise(*restored.all,CollisionWorld(all));
    const auto empty=CollisionWorld::compose(restored.bsp,{},{.exclusive_bsp=false});
    check(empty.bsp==empty.fixed&&empty.fixed==empty.all,"Empty layers retained redundant owners");
    exercise(*empty.all,CollisionWorld(bsp));
    std::cout<<"Collision layer storage, exact ray ties, contacts, movement and snapshot ownership passed\n";
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
