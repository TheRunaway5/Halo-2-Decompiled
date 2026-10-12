#include "halo2/frontend.hpp"
#include <array>
#include <bit>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
std::uint32_t word(float value) {return std::bit_cast<std::uint32_t>(value);}
std::array<std::uint32_t,33> words(const Vertex &v) {
  return {word(v.position.x),word(v.position.y),word(v.position.z),
    word(v.color.x),word(v.color.y),word(v.color.z),word(v.uv.x),word(v.uv.y),
    v.material,word(v.lightmap_uv.x),word(v.lightmap_uv.y),
    word(v.normal.x),word(v.normal.y),word(v.normal.z),
    word(v.tangent.x),word(v.tangent.y),word(v.tangent.z),
    word(v.binormal.x),word(v.binormal.y),word(v.binormal.z),
    word(v.incident.x),word(v.incident.y),word(v.incident.z),v.lightmap,
    std::bit_cast<std::uint32_t>(v.bsp_cluster),word(v.prt_cluster),
    word(v.prt_weights[0]),word(v.prt_weights[1]),word(v.prt_weights[2]),
    word(v.prt_weights[3]),v.source_section,v.source_part,v.source_stipple_level};
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("maps directory and sampled-font directory required");
    const std::filesystem::path maps(argv[1]),sampled(argv[2]);
    if(!std::filesystem::exists(maps/"mainmenu.map") || !std::filesystem::exists(sampled/"conduit-12"))return 77;
    const Cache cache(maps/"mainmenu.map");
    const auto content=cache.load_frontend();
    Mesh stock_mesh,hd_mesh;
    stock_mesh.materials.push_back({"world-sentinel",Texture{1,1,11,std::vector<std::byte>(4,std::byte{127}),{}}});
    hd_mesh=stock_mesh;
    FrontendPainter stock(content,maps/"fonts",stock_mesh);
    FrontendPainter hd(content,maps/"fonts",hd_mesh,{}, {},sampled);
    require(stock_mesh.materials.size()==hd_mesh.materials.size(),"HD font source changed material indices");
    std::size_t atlases{},changed_uvs{},draws{},panes{};
    for(unsigned i=0;i<stock_mesh.materials.size();++i) {
      const auto &a=stock_mesh.materials[i],&b=hd_mesh.materials[i];
      require(a.name==b.name,"HD fonts changed material identity");
      if(a.name.starts_with("font/")) {
        atlases+=a.diffuse.pixels!=b.diffuse.pixels;
      } else {
        require(a.diffuse.width==b.diffuse.width && a.diffuse.height==b.diffuse.height &&
          a.diffuse.pixels==b.diffuse.pixels,"font sampler changed a world/UI bitmap");
      }
    }
    auto compare=[&](MenuState &state,int pane,unsigned time) {
      const auto a=stock.draw(state,float(time)/1000.f,pane,time);
      const auto b=hd.draw(state,float(time)/1000.f,pane,time);
      require(a.vertices.size()==b.vertices.size(),"HD fonts changed text count or wrapping");
      for(unsigned i=0;i<a.vertices.size();++i) {
        auto x=words(a.vertices[i]),y=words(b.vertices[i]);
        if(stock_mesh.materials.at(a.vertices[i].material).name.starts_with("font/")) {
          changed_uvs+=(x[6]!=y[6] || x[7]!=y[7]);
          x[6]=y[6];x[7]=y[7];
        }
        require(x==y,"HD fonts changed logical geometry, clipping, color or order");
      }
      require(a.scene_color_filter==b.scene_color_filter && a.backdrops.size()==b.backdrops.size() &&
        a.scenes.size()==b.scenes.size(),"HD fonts changed backdrop or model scheduling");
      for(unsigned i=0;i<a.scenes.size();++i)
        require(a.scenes[i].definition==b.scenes[i].definition &&
          a.scenes[i].before_vertex==b.scenes[i].before_vertex &&
          a.scenes[i].appearance==b.scenes[i].appearance,"HD fonts changed a 3D model definition");
      for(int y=-8;y<=488;y+=8)for(int x=-8;x<=648;x+=8) {
        require(stock.hit(float(x),float(y))==hd.hit(float(x),float(y)),"HD fonts moved a mouse target");
        require(stock.pointer_hit(float(x),float(y))==hd.pointer_hit(float(x),float(y)),"HD fonts moved a prompt target");
      }
      ++draws;
    };
    for(const auto &[id,screen]:content.screens) {
      MenuState state(content);if(id!=9)state.open(id);
      for(unsigned pane=0;pane<screen.panes.size();++pane) {compare(state,int(pane),5000);++panes;}
    }
    MenuState settings(content);settings.ui_clock(100);settings.open(19);
    for(unsigned time:{100u,150u,500u,2000u,6000u})compare(settings,-1,time);
    MenuState keyboard(content);keyboard.accept();keyboard.accept();keyboard.text("AV Na Typing 123");
    compare(keyboard,-1,5000);
    require(atlases>=5 && changed_uvs>1000,"actual larger font sampling was not exercised");
    std::cout<<"PASS "<<atlases<<" HD font atlases, "<<panes<<" panes, "<<draws
      <<" paired draws, "<<changed_uvs<<" sampled font vertices; logical geometry/hits unchanged\n";
    return 0;
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
