#include "halo2/hud.hpp"
#include "halo2/render_view.hpp"
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char* message) {
  if(!value)throw std::runtime_error(message);
}
}
int main(int argc,char**argv) {try {
  using namespace halo2;
  const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
  std::ifstream file;
  if(!probe) {file.open("tests/fixtures/retail_source_view_rectangle.tsv");
    require(bool(file),"missing original viewport rectangle fixture");}
  unsigned count=0,even_count=0,old_post_mismatches=0,old_bank_mismatches=0;
  const auto check=[&](unsigned width,unsigned height,bool wide,unsigned players,unsigned view,
                       const std::array<int,4>& edges,const std::array<std::uint32_t,16>& expected) {
    const auto r=hud_viewport(players,view,wide).outer;
    const RenderViewport layout{r.left,r.top,r.right-r.left,r.bottom-r.top};
    const auto physical=full_surface_viewport(layout,width,height);
    const std::array<int,4> actual{int(physical.top),int(physical.left),
      int(physical.top+physical.height),int(physical.left+physical.width)};
    const auto bank=std::bit_cast<std::array<std::uint32_t,16>>(viewport_constants(physical));
    require(actual==edges,"physical rectangle differs from frozen signed-short camera edges");
    require(bank==expected,"physical viewport constants differ from untouched12FA0 bank");
    const float sx=float(width)/640.f,sy=float(height)/480.f;
    const RenderViewport old{r.left*sx,r.top*sy,(r.right-r.left)*sx,(r.bottom-r.top)*sy};
    const bool size_diff=int(old.width)!=int(physical.width) || int(old.height)!=int(physical.height);
    const bool bank_diff=std::bit_cast<std::array<std::uint32_t,16>>(viewport_constants(old))!=expected;
    if(width%2==0 && height%2==0) {
      require(!size_diff && !bank_diff,"standard even-resolution bank changed");++even_count;
    } else {old_post_mismatches+=size_diff;old_bank_mismatches+=bank_diff;}
    // All submitted rectangles must contain exactly their bank's pixel count.
    require(physical.left>=0 && physical.top>=0 && physical.left+physical.width<=width &&
            physical.top+physical.height<=height,"submitted rectangle escapes its actual target");
    ++count;
  };
  if(probe) {
    for(const auto size:std::array{std::array{640u,480u},std::array{1280u,720u},
        std::array{1920u,1080u},std::array{3840u,2160u},std::array{1281u,721u},
        std::array{1921u,1081u},std::array{1100u,651u},std::array{853u,479u}})
      for(bool wide:{false,true})for(unsigned players=1;players<=4;++players)
        for(unsigned view=0;view<players;++view) {
          const auto r=hud_viewport(players,view,wide).outer;
          const auto p=full_surface_viewport({r.left,r.top,r.right-r.left,r.bottom-r.top},size[0],size[1]);
          std::cout<<size[0]<<' '<<size[1]<<' '<<wide<<' '<<players<<' '<<view<<' '
            <<int(p.top)<<' '<<int(p.left)<<' '<<int(p.top+p.height)<<' '<<int(p.left+p.width);
          for(const auto word:std::bit_cast<std::array<std::uint32_t,16>>(viewport_constants(p)))
            std::cout<<' '<<word;
          std::cout<<'\n';
        }
    return 0;
  }
  unsigned width,height,wide,players,view;
  while(file>>width>>height>>wide>>players>>view) {
    require(wide<=1,"invalid rectangle widescreen input");
    std::array<int,4> edges;for(auto& edge:edges)file>>edge;
    std::array<std::uint32_t,16> expected;for(auto& word:expected)file>>word;
    require(bool(file),"truncated original rectangle bank");
    check(width,height,bool(wide),players,view,edges,expected);
  }
  require(file.eof() && count==160 && even_count==80,"incomplete original rectangle corpus");
  require(old_post_mismatches==44 && old_bank_mismatches==69,
          "odd-resolution controls no longer exercise original defects");
  for(const auto bad:std::array{RenderViewport{0,0,0,480},RenderViewport{0,0,640,0},
      RenderViewport{32768,0,1,1},RenderViewport{NAN,0,1,1}}) {
    bool rejected=false;try{(void)full_surface_viewport(bad,640,480);}
    catch(const std::invalid_argument&){rejected=true;}
    require(rejected,"invalid signed-short viewport input was admitted");
  }
  require(viewport_constants({0,0,640,480,.5f,true,false,2})==
    std::array<ShaderVector,4>{{{64,0,0,64.5f},{0,-64,0,64.5f},
      {64,-64,16777215,0},{64.5f,64.5f,0,0}}},"fixed mode2 viewport changed");
  std::cout<<count<<" untouched12FA0 viewport banks exact; "<<even_count
    <<" standard layouts unchanged; 44 size and 69 fractional-bank defect controls passed\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
