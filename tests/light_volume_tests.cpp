#include "halo2/light_volume.hpp"
#include "halo2/model.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace {
void check(bool value,const char *message) {
  if(!value)throw std::runtime_error(message);
}
template<class T> void read(std::ifstream &in,T &v) {
  in.read(reinterpret_cast<char*>(&v),sizeof(v));
  check(bool(in),"short light-volume input");
}
template<class T> void write(std::ofstream &out,const T &v) {
  out.write(reinterpret_cast<const char*>(&v),sizeof(v));
  check(bool(out),"short light-volume output");
}
}
int main(int argc,char **argv) {
  try {
    using namespace halo2;
    check(argc==2 || argc==4 || argc==5,"original map and optional input/output required");
    if(argc==4 && std::string_view(argv[1])=="--cameras") {
      std::ifstream input(argv[2],std::ios::binary);
      std::ofstream output(argv[3],std::ios::binary);
      check(bool(input)&&bool(output),"cannot open camera vectors");
      std::uint32_t count{};read(input,count);
      for(unsigned i=0;i<count;++i) {
        Vec3 eye,forward,up;Vec2 axes;RenderViewport viewport;
        read(input,eye);read(input,forward);read(input,up);read(input,axes);
        read(input,viewport.left);read(input,viewport.top);read(input,viewport.width);read(input,viewport.height);
        const auto camera=light_volume_title_projection(eye,forward,up,axes,viewport);
        write(output,camera.world_to_view);write(output,camera.projection);
      }
      std::cout<<count<<" title projection-camera fixtures\n";
      return 0;
    }
    if(argc==4 && std::string_view(argv[1])=="--quads") {
      std::ifstream input(argv[2],std::ios::binary);
      std::ofstream output(argv[3],std::ios::binary);
      check(bool(input)&&bool(output),"cannot open projection vectors");
      std::uint32_t count{};read(input,count);
      for(unsigned i=0;i<count;++i) {
        LightVolumeProjection view;LightVolumeSprite sprite;float cosine{},sine{};
        read(input,view.world_to_view);read(input,view.projection);
        read(input,view.viewport.left);read(input,view.viewport.top);
        read(input,view.viewport.width);read(input,view.viewport.height);
        read(input,view.far_distance);read(input,sprite);read(input,cosine);read(input,sine);
        const auto quad=light_volume_quad(sprite,cosine,sine,view);
        write(output,std::uint32_t(quad.has_value()));
        if(quad)write(output,*quad);
      }
      std::cout<<count<<" projected original-style sprite fixtures\n";
      return 0;
    }
    Cache cache(argv[1]);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){
      return t.group=="MGS2" && t.name.ends_with("capitol_ship_engine");});
    check(tag!=cache.tags().end(),"missing authored carrier volume");
    const auto definition=cache.load_light_volume(tag->datum);
    check(definition.volumes.size()==1,"original carrier volume count");
    const auto &volume=definition.volumes.front();
    std::cout<<"Carrier volume flags="<<volume.flags<<" sprites="<<volume.sprite_count
      <<" runtime_flags="<<volume.runtime_flags<<" phases="<<volume.phases.size()
      <<" aspect="<<volume.aspect.has_value()<<'\n';
    check(volume.flags==32 && volume.sprite_count==32 && volume.runtime_flags==2 &&
      volume.phases.size()==32 && volume.aspect.has_value(),"original carrier volume fields");
    const auto tables=cache.shader_function_tables();
    if(argc==5 && std::string_view(argv[2])=="--title-quads") {
      std::ifstream input(argv[3],std::ios::binary);
      std::ofstream output(argv[4],std::ios::binary);
      check(bool(input)&&bool(output),"cannot open captured title camera vectors");
      LightVolumeProjection projection;Vec3 eye;
      read(input,projection.world_to_view);read(input,projection.projection);
      read(input,projection.viewport.left);read(input,projection.viewport.top);
      read(input,projection.viewport.width);read(input,projection.viewport.height);
      read(input,projection.far_distance);read(input,eye);
      const LightVolumeView view{eye,projection.world_to_view,
        {projection.projection[0][0]*projection.viewport.width*.5f,
         projection.projection[1][1]*projection.viewport.height*.5f}};
      const auto level=cache.load_content(0);
      std::uint32_t count{};write(output,count);
      for(unsigned placement=0;placement<level.objects.size();++placement) {
        const auto &object=level.objects[placement];
        if(object.definition!=0xe1d50060u || object.flags&1)continue;
        const auto model=cache.load_model(object.definition,false,object.variant);
        const auto basis=authored_basis(object.rotation);
        const Transform world{object.scale,{basis.forward,basis.left,basis.up},object.position};
        const auto attachments=cache.load_light_volume_attachments(object.definition,model,0,&world);
        for(const auto &attachment:attachments) {
          const auto definition=cache.load_light_volume(attachment.datum);
          for(unsigned marker=0;marker<attachment.markers.size();++marker) {
            const LightVolumeState state{attachment.markers[marker]};
            const auto batches=light_volume_sprites(definition,state,view,*tables);
            for(const auto &batch:batches)for(unsigned i=0;i<batch.sprites.size();++i) {
              const auto &sprite=batch.sprites[i];
              if(const auto quad=light_volume_quad(sprite,batch.cosine,batch.sine,projection)) {
                write(output,placement);write(output,marker);write(output,i);
                write(output,sprite);write(output,batch.cosine);write(output,batch.sine);write(output,*quad);
                ++count;
              }
            }
          }
        }
      }
      output.seekp(0);write(output,count);
      std::cout<<count<<" title carrier quads with captured camera inputs\n";
      return 0;
    }
    const auto model=cache.load_model(0xe1d50060,false);
    const auto attachments=cache.load_light_volume_attachments(0xe1d50060,model);
    check(attachments.size()==1 && attachments.front().datum==tag->datum &&
      attachments.front().markers.size()==3,"original carrier attachment markers");
    const auto glow=cache.load_light_volume_default_bitmap();
    check(glow.width==64 && glow.height==64 && glow.format==11 && glow.mips.size()==7,
      "original default glow bitmap");
    const LightVolumeView view{{},Transform{}, {2.5f,3.f}};
    LightVolumeState state;
    state.marker.position={100,30,-5};
    const auto batches=light_volume_sprites(definition,state,view,*tables);
    check(batches.size()==1 && batches.front().sprites.size()==32,"carrier sprite generation");
    state.opacity=0;
    check(light_volume_sprites(definition,state,view,*tables).empty(),"zero-opacity volume is omitted");
    if(argc==4) {
      std::ifstream input(argv[2],std::ios::binary);
      std::ofstream output(argv[3],std::ios::binary);
      check(bool(input) && bool(output),"cannot open light-volume vectors");
      std::uint32_t cases{};read(input,cases);
      for(unsigned i=0;i<cases;++i) {
        LightVolumeState fixture;
        LightVolumeView camera;
        read(input,fixture.marker);read(input,fixture.opacity);read(input,fixture.color);
        read(input,camera.eye);read(input,camera.world_to_view);read(input,camera.projection_scale);
        const auto generated=light_volume_sprites(definition,fixture,camera,*tables);
        write(output,std::uint32_t(generated.size()));
        for(const auto &batch:generated) {
          write(output,batch.bitmap);write(output,std::uint32_t(batch.fuzzy));
          write(output,std::uint32_t(batch.sprites.size()));
          write(output,batch.cosine);write(output,batch.sine);
          for(const auto &sprite:batch.sprites)write(output,sprite);
        }
      }
      write(output,light_volume_program().words);
      std::ofstream image(std::string(argv[3])+".glow.rgba",std::ios::binary);
      image.write(reinterpret_cast<const char*>(glow.pixels.data()),64*64*4);
      check(bool(image),"cannot write default glow bitmap");
      std::ofstream markers(std::string(argv[3])+".markers.bin",std::ios::binary);
      const auto mode=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){
        return t.group=="mode" && t.name==model.name;});
      check(mode!=cache.tags().end(),"missing marker model tag");
      write(markers,mode->datum);write(markers,std::uint32_t(0x070008e3));
      write(markers,std::uint32_t(model.bones.size()));
      std::optional<AnimationClip> idle;
      if(const auto name=cache.find_animation_name(model.animation,"idle"))
        idle=cache.load_named_animation(model.animation,*name);
      for(unsigned i=0;i<model.bones.size();++i)
        write(markers,pose_model_node(model,i,idle?&*idle:nullptr,0));
      write(markers,std::uint32_t(attachments.front().markers.size()));
      for(const auto &marker:attachments.front().markers)write(markers,marker);
      const auto level=cache.load_content(0);
      const auto object=std::find_if(level.objects.begin(),level.objects.end(),[](const auto &o){
        return o.definition==0xe1d50060u && !(o.flags&1);});
      check(object!=level.objects.end(),"missing title carrier placement");
      const auto basis=authored_basis(object->rotation);
      const Transform world{object->scale,{basis.forward,basis.left,basis.up},object->position};
      const auto world_attachments=cache.load_light_volume_attachments(object->definition,model,0,&world);
      std::ofstream world_markers(std::string(argv[3])+".world.markers.bin",std::ios::binary);
      write(world_markers,mode->datum);write(world_markers,std::uint32_t(0x070008e3));
      write(world_markers,std::uint32_t(model.bones.size()));
      for(unsigned i=0;i<model.bones.size();++i)
        write(world_markers,compose_transform(world,pose_model_node(model,i,idle?&*idle:nullptr,0)));
      write(world_markers,std::uint32_t(world_attachments.front().markers.size()));
      for(const auto &marker:world_attachments.front().markers)write(world_markers,marker);
    }
    std::cout<<"Authored carrier volume fields and sprite generation verified\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
