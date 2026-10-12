#include "halo2/object_lighting.hpp"
#include "halo2/renderer.hpp"
#include "halo2/texture.hpp"
#include <vulkan/vulkan.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void require(bool ok,const std::string &message) {if(!ok)throw std::runtime_error(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path) {
  std::ifstream in(path,std::ios::binary);Image image;std::string magic;unsigned maximum;
  in>>magic>>image.width>>image.height>>maximum;in.get();
  require(magic=="P6" && maximum==255,"invalid special-texture framebuffer");
  image.pixels.resize(std::size_t(image.width)*image.height*3);
  in.read(reinterpret_cast<char*>(image.pixels.data()),image.pixels.size());
  require(bool(in),"short special-texture framebuffer");return image;
}
Material material(const Cache &cache,const std::string &name) {
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t) {
    return t.group=="shad" && t.name==name;
  });
  require(tag!=cache.tags().end(),"missing original special texture shader");
  auto result=cache.load_material(tag->datum);
  require(prepare_dynamic_material(cache,result,{},true),"original special texture material not admitted");
  return result;
}
Texture white() {return {1,1,11,std::vector<std::byte>(4,std::byte{255}),{}};}
void identity(Material &m,const Material &coordinate_source) {
  // Controlled planar geometry uses the original stencil_texture microcode.
  // The observed original pixel program and authored image modes stay intact.
  m.vertex_program=coordinate_source.vertex_program;m.forward_vertex=3;
  m.program_constants=coordinate_source.program_constants;
  m.program_constants[1][0]={1,1,0,0};m.program_constants[1][1]={1,1,0,0};
  m.forward_transforms[0]={1,1,0,0};m.forward_transforms[1]={1,1,0,0};
  m.forward_raster.depth_enable=0;m.forward_raster.depth_write=0;m.forward_raster.cull=0;
  m.environment_pass.reset();m.world_overlays_before.clear();m.world_overlays_after.clear();
  m.world_bloom_passes.clear();m.world_planar_passes={};
  m.animation.overlays.clear();for(auto &frames:m.animated_textures)frames.clear();
}
std::vector<Vertex> quad(unsigned material,float aspect,float distance=1) {
  std::vector<Vertex> result;
  for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
    Vertex v;v.position={distance,(1-uv.x*2)*aspect*distance,(1-uv.y*2)*distance};
    v.uv=uv;v.color={1,1,1};v.normal={-1,0,0};v.tangent={0,-1,0};v.binormal={0,0,-1};
    v.material=material;result.push_back(v);
  }
  return result;
}
float linear(const Image &image,float u,float v,unsigned channel) {
  const float x=u*image.width-.5f,y=v*image.height-.5f;
  const int ix=int(std::floor(x)),iy=int(std::floor(y));const float dx=x-ix,dy=y-iy;
  const auto tap=[&](int xx,int yy) {
    xx=std::clamp(xx,0,int(image.width)-1);yy=std::clamp(yy,0,int(image.height)-1);
    return float(image.pixels[(std::size_t(yy)*image.width+xx)*3+channel]);
  };
  return (tap(ix,iy)*(1-dx)+tap(ix+1,iy)*dx)*(1-dy)+
    (tap(ix,iy+1)*(1-dx)+tap(ix+1,iy+1)*dx)*dy;
}
float volume_linear(const Texture &texture,unsigned level,float u,float v,unsigned channel) {
  const auto &mip=texture.mips.at(level);
  const float x=u*mip.width-.5f,y=v*mip.height-.5f,z=-.5f;
  const int ix=int(std::floor(x)),iy=int(std::floor(y)),iz=int(std::floor(z));
  const float dx=x-ix,dy=y-iy,dz=z-iz;
  const auto wrap=[](int p,unsigned size) {return unsigned((p%int(size)+int(size))%int(size));};
  float sum=0;
  for(int zz=0;zz<2;++zz)for(int yy=0;yy<2;++yy)for(int xx=0;xx<2;++xx) {
    const auto at=mip.offset+((std::size_t(wrap(iz+zz,mip.depth))*mip.height+
      wrap(iy+yy,mip.height))*mip.width+wrap(ix+xx,mip.width))*4+channel;
    sum+=float(std::to_integer<unsigned>(texture.pixels.at(at)))*(xx?dx:1-dx)*(yy?dy:1-dy)*(zz?dz:1-dz);
  }
  return sum;
}
}
int main(int argc,char **argv) {
  try {
    require(argc==3,"special texture test needs original maps and output directory");
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
#endif
    const Cache cache(std::filesystem::path(argv[1])/"lockout.map");
    const auto directory=std::filesystem::path(argv[2]);std::filesystem::create_directories(directory);
    auto elite=material(cache,"objects\\characters\\elite\\fp_arms\\shaders\\elite_fp_glow");
    const auto glow=std::find_if(elite.world_overlays_after.begin(),elite.world_overlays_after.end(),
      [](const auto &p){return p && p->detail.depth && p->bump.depth;});
    require(glow!=elite.world_overlays_after.end(),"missing authored Elite volume layer");
    const auto original_glow=**glow;
    require(original_glow.forward_program.words[54]==0x8420 && original_glow.forward_program.words[59]==0,
      "Elite volume original addressing changed");
    auto sniper=material(cache,"objects\\weapons\\rifle\\sniper_rifle\\shaders\\sniper_rifle_screen");
    const auto scope=std::find_if(sniper.world_overlays_after.begin(),sniper.world_overlays_after.end(),
      [](const auto &p){return p && p->scope_texture;});
    require(scope!=sniper.world_overlays_after.end(),"missing authored sniper scope layer");
    const auto original_scope=**scope;
    Mesh mesh;mesh.vertices.resize(3);
    for(unsigned mip=0;mip<6;++mip) {
      auto observer=original_glow;identity(observer,original_scope);observer.scope_texture=false;
      // Observe t1 directly, keeping Elite's actual volume images and mode
      // word. This isolates image upload/filtering from its plasma combiner.
      observer.forward_program=scope_copy_frame({}).program;
      observer.forward_program.words[8]=9;observer.forward_program.words[54]=0x8420;
      observer.forward_blend=0;observer.forward_raster.color_write=0x10101;
      observer.texture_states[1].max_mip=mip;observer.texture_states[1].mip_filter=1;
      observer.texture_states[1].address_u=observer.texture_states[1].address_v=observer.texture_states[1].address_w=1;
      observer.texture_states[1].mag_filter=observer.texture_states[1].min_filter=2;
      mesh.materials.push_back(std::move(observer));
    }
    for(unsigned view=0;view<4;++view) {
      auto backdrop=original_scope;identity(backdrop,original_scope);backdrop.scope_texture=false;
      backdrop.forward_program=scope_copy_frame({}).program;backdrop.forward_blend=0;
      backdrop.diffuse={64,64,11,std::vector<std::byte>(64*64*4),{}};
      for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x) {
        const auto at=(y*64+x)*4;
        backdrop.diffuse.pixels[at]=std::byte(8+view*12+x/4);
        backdrop.diffuse.pixels[at+1]=std::byte(6+(3-view)*10+y/4);
        backdrop.diffuse.pixels[at+2]=std::byte(7+view*6+(x+y)/8);
        backdrop.diffuse.pixels[at+3]=std::byte{255};
      }
      mesh.materials.push_back(std::move(backdrop));
    }
    auto consumer=original_scope;identity(consumer,original_scope);
    // A white mask makes the original t0.rgb*t1.rgb consumer observable.
    // ONE/ONE blending is retained: expected output is scene + scope copy.
    consumer.detail=white();consumer.forward_raster.color_write=0x10101;
    require(consumer.forward_program.words==original_scope.forward_program.words && consumer.forward_blend==3,
      "scope consumer fixture changed original combiner/blend");
    const unsigned consumer_index=mesh.materials.size();mesh.materials.push_back(std::move(consumer));
    if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(std::string("SDL special texture init: ")+SDL_GetError());
    auto *window=SDL_CreateWindow("Original special texture integration",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    if(!window)throw std::runtime_error(std::string("SDL special texture window: ")+SDL_GetError());
    std::size_t volume_checks=0,scope_checks=0;
    {
      unsigned layer_count=0;vkEnumerateInstanceLayerProperties(&layer_count,nullptr);
      std::vector<VkLayerProperties> layers(layer_count);vkEnumerateInstanceLayerProperties(&layer_count,layers.data());
      const bool validation=std::any_of(layers.begin(),layers.end(),[](const auto &layer) {
        return std::string(layer.layerName)=="VK_LAYER_KHRONOS_validation";
      });
      if(!validation) {
        SDL_DestroyWindow(window);SDL_Quit();
        std::cout<<"special texture GPU integration requires the Vulkan validation layer\n";return 77;
      }
      Camera camera;camera.field_of_view=90;
      const auto material_views=[&](unsigned state,std::array<std::vector<Vertex>,4> &vertices) {
        std::array<ModelView,4> views;
        for(unsigned i=0;i<4;++i) {
          auto &view=views[i];view.camera=camera;view.camera.yaw=float(state)*.02f;
          view.left=float(i%2)*320;view.top=float(i/2)*240;
          view.width=320;view.height=240;view.full_surface=true;
          vertices[i]=quad(state==1?5u:0u,4.f/3.f);view.vertices=vertices[i];
        }
        return views;
      };
      const auto image_frame=[&](unsigned view,unsigned state) {
        auto image=mesh.materials[6+view].diffuse;
        for(std::size_t at=0;at<image.pixels.size();at+=4)for(unsigned c=0;c<3;++c)
          image.pixels[at+c]=std::byte((std::to_integer<unsigned>(image.pixels[at+c])+23*state*(view+1))%256);
        return image;
      };
      const auto image_views=[&](unsigned state,std::array<std::vector<Vertex>,4> &vertices) {
        auto views=material_views(state,vertices);
        for(unsigned i=0;i<4;++i) {
          vertices[i]=quad(6+i,4.f/3.f);views[i].vertices=vertices[i];
        }
        return views;
      };
      // Independent fresh uploads establish the pixel reference before the
      // retained renderer exists, avoiding concurrent Vulkan device lifetimes.
      const auto original_materials=mesh.materials;
      const auto apply_material_state=[&](Mesh &target,unsigned state) {
        for(unsigned m=0;m<6;++m) {
          auto &material=target.materials[m];
          material.program_constants[1][0][0]=1.f+float(state)*.125f;
          material.forward_program=original_materials[m].forward_program;
          if(state) {
            for(unsigned word=10;word<26;++word)
              material.forward_program.words[word]=(material.forward_program.words[word]&0xff000000u)|
                  ((material.forward_program.words[word]&0x00fefefeu)>>1);
            for(unsigned word:{43u,44u})material.forward_program.words[word]^=0x00402010u;
          }
          // A live control-word change must use the generic program, instead
          // of retaining the pipeline's original specialized stage count.
          if(state==2)material.forward_program.words[53]&=~255u;
        }
      };
      SDL_setenv("HALO2_REFERENCE_GENERIC_COMBINERS","1",1);
      SDL_setenv("HALO2_REFERENCE_SERIAL_GEOMETRY","1",1);
      SDL_setenv("HALO2_REFERENCE_UNSHARED_VERTICES","1",1);
      SDL_setenv("HALO2_REFERENCE_UNCACHED_VIEW_BINDINGS","1",1);
      std::array<Image,3> material_references,image_references;
      for(unsigned state=0;state<3;++state) {
        auto reference_mesh=mesh;
        apply_material_state(reference_mesh,state);
        for(unsigned i=0;i<4;++i)reference_mesh.materials[6+i].diffuse=image_frame(i,state);
        std::array<std::vector<Vertex>,4> vertices;
        const auto views=material_views(state,vertices);
        const auto path=directory/("material-fresh-"+std::to_string(state)+".ppm");
        Renderer fresh(window,reference_mesh,validation);fresh.scene_clear_color({});
        require(fresh.draw(camera,path,{},false,{},{},{},views),"fresh view material frame unavailable");
        material_references[state]=read(path);
        const auto dynamic_views=image_views(state,vertices);
        const auto dynamic_path=directory/("image-fresh-"+std::to_string(state)+".ppm");
        require(fresh.draw(camera,dynamic_path,{},false,{},{},{},dynamic_views),"fresh dynamic image frame unavailable");
        image_references[state]=read(dynamic_path);
      }
      SDL_setenv("HALO2_REFERENCE_GENERIC_COMBINERS","",1);
      SDL_setenv("HALO2_REFERENCE_SERIAL_GEOMETRY","",1);
      SDL_setenv("HALO2_REFERENCE_UNSHARED_VERTICES","",1);
      SDL_setenv("HALO2_REFERENCE_UNCACHED_VIEW_BINDINGS","",1);
      Renderer renderer(window,mesh,validation);renderer.scene_clear_color({});
      const auto capture=[&](const std::string &name,std::span<const ModelView> views) {
        const auto path=directory/(name+".ppm");bool drawn=false;
        for(unsigned attempt=0;attempt<4 && !drawn;++attempt)drawn=renderer.draw(camera,path,{},false,{}, {},{},views);
        require(drawn,"special texture framebuffer not rendered");return read(path);
      };
      for(const auto extent:std::array{std::array{640,480},std::array{1280,720}}) {
        SDL_SetWindowSize(window,extent[0],extent[1]);SDL_PumpEvents();renderer.resized();
        const auto suffix=std::to_string(extent[0])+"x"+std::to_string(extent[1]);
        for(unsigned mip=0;mip<6;++mip) {
          auto vertices=quad(mip,1);ModelView view;view.camera=camera;view.left=0;view.top=0;
          view.width=64.f*640/extent[0];view.height=64.f*480/extent[1];view.vertices=vertices;view.full_surface=true;
          const auto image=capture("volume-mip"+std::to_string(mip)+"-"+suffix,{&view,1});
          for(unsigned y=8;y<56;y+=3)for(unsigned x=8;x<56;x+=3)for(unsigned c=0;c<3;++c) {
            // The original stencil vertex program adds c11's 17/32-pixel
            // position bias, then the shared raster path truncates to1/16.
            // The controlled quad therefore starts at(.5,.5), so a Vulkan
            // pixel center x+.5 interpolates the authored UV at x/64.
            const float expected=volume_linear(original_glow.detail,mip,float(x)/64,float(y)/64,c);
            const auto got=image.pixels[(std::size_t(y)*image.width+x)*3+c];
            require(std::abs(float(got)-expected)<=2.1f,"Elite volume linear sampling or mip upload differs at mip"+
              std::to_string(mip)+" pixel"+std::to_string(x)+","+std::to_string(y)+" got"+std::to_string(got)+" expected"+std::to_string(expected));
            ++volume_checks;
          }
        }
        for(unsigned players:{1u,4u}) {
          std::array<ModelView,4> views;std::array<std::vector<Vertex>,4> world,foreground;
          for(unsigned i=0;i<players;++i) {
            auto &v=views[i];v.camera=camera;v.left=players==1?0:float(i%2)*320;
            v.top=players==1?0:float(i/2)*240;v.width=players==1?640:320;v.height=players==1?480:240;v.full_surface=true;
            const float aspect=(v.width*extent[0]/640)/(v.height*extent[1]/480);
            world[i]=quad(6+i,aspect,2);
            // External27 outside first-person must retain its authored
            // per-material fallback. Here that controlled fallback is zero:
            // adding the same consumer in world geometry must do nothing.
            const auto world_scope=quad(consumer_index,aspect,2);
            world[i].insert(world[i].end(),world_scope.begin(),world_scope.end());
            foreground[i]=quad(consumer_index,aspect);v.vertices=world[i];
          }
          const auto baseline=capture("scope-baseline"+std::to_string(players)+"-"+suffix,{views.data(),players});
          for(unsigned i=0;i<players;++i) {
            auto &v=views[i];const RenderViewport viewport{v.left*extent[0]/640,v.top*extent[1]/480,
              v.width*extent[0]/640,v.height*extent[1]/480};
            const Vec2 size{.35f+.05f*i,.3f+.04f*i};
            v.scope=scope_copy_frame(scope_source_rectangle(viewport,size,scope_sample_center(viewport),
              {float(extent[0]),float(extent[1])}));v.foreground=foreground[i];
          }
          const auto image=capture("scope-views"+std::to_string(players)+"-"+suffix,{views.data(),players});
          for(unsigned i=0;i<players;++i) {
            const auto &v=views[i];const auto r=v.scope->source_rectangle;
            Image copy{64,64,std::vector<unsigned char>(64*64*3)};
            for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x)for(unsigned c=0;c<3;++c)
              copy.pixels[(y*64+x)*3+c]=unsigned(std::nearbyint(linear(baseline,
                r[0]+(r[2]-r[0])*(x+.5f)/64,r[1]+(r[3]-r[1])*(y+.5f)/64,c)));
            const unsigned left=unsigned(v.left*extent[0]/640),top=unsigned(v.top*extent[1]/480),
              width=unsigned(v.width*extent[0]/640),height=unsigned(v.height*extent[1]/480);
            for(unsigned y=8;y<height-8;y+=13)for(unsigned x=8;x<width-8;x+=13)for(unsigned c=0;c<3;++c) {
              const auto at=(std::size_t(top+y)*image.width+left+x)*3+c;
              const float expected=std::min(255.f,float(baseline.pixels[at])+linear(copy,float(x)/width,float(y)/height,c));
              require(std::abs(float(image.pixels[at])-expected)<=2.1f,"scope copy/resize/view isolation differs at view"+
                std::to_string(i)+" pixel"+std::to_string(x)+","+std::to_string(y)+" got"+std::to_string(image.pixels[at])+" expected"+std::to_string(expected));
              ++scope_checks;
            }
          }
          // A following scope-free frame must not retain copied FP pixels.
          for(unsigned i=0;i<players;++i)views[i].scope.reset(),views[i].foreground={};
          const auto restored=capture("scope-restored"+std::to_string(players)+"-"+suffix,{views.data(),players});
          require(restored.pixels==baseline.pixels,"inactive scope leaks pixels into the following world frame");
        }
      }
      SDL_SetWindowSize(window,640,480);SDL_PumpEvents();renderer.resized();
      std::vector<unsigned char> previous_pixels;
      for(unsigned state=0;state<3;++state) {
        apply_material_state(mesh,state);
        const std::array<unsigned,1> active{state==1?5u:0u};
        renderer.update_material_constants(mesh.materials,active);
        std::array<std::vector<Vertex>,4> vertices;
        const auto views=material_views(state,vertices);
        const auto retained=capture("material-retained-"+std::to_string(state),views);
        require(retained.pixels==material_references[state].pixels,
            "retained per-view material differs from a fresh upload after visibility/constants/camera change");
        if(state)require(retained.pixels!=previous_pixels,"material change control did not alter visible pixels");
        previous_pixels=retained.pixels;
      }
      previous_pixels.clear();
      for(unsigned state=0;state<3;++state) {
        // Update one view, then four existing image pages. Repeated updates
        // select the latest bytes and must not retain the caller's pixels.
        for(unsigned i=0;i<(state?4u:1u);++i) {
          auto image=image_frame(i,state+1);renderer.update_image(6+i,image);
          image=image_frame(i,state);renderer.update_image(6+i,image);
          std::fill(image.pixels.begin(),image.pixels.end(),std::byte{});
        }
        std::array<std::vector<Vertex>,4> vertices;
        const auto views=image_views(state,vertices);
        const auto retained=capture("image-retained-"+std::to_string(state),views);
        require(retained.pixels==image_references[state].pixels,
            "dynamic image update differs from independently initialized textures");
        if(state)require(retained.pixels!=previous_pixels,"dynamic image control did not alter visible pixels");
        previous_pixels=retained.pixels;
      }
    }
    SDL_DestroyWindow(window);SDL_Quit();
    std::cout<<volume_checks<<" authored Elite volume upload/filter/mip checks; "<<scope_checks
      <<" original scope consumer copy/resize/isolation checks passed\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}
}
