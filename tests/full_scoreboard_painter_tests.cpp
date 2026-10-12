#include "halo2/full_scoreboard_painter.hpp"
#include "halo2/frontend_text_style.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <bit>
#include <cmath>
#include <cstring>
using namespace halo2;
using J=nlohmann::json;
unsigned checks=0; float source_alpha;
std::optional<float> source_pulse(FullScoreboardClock,float,float,float,float,uint16_t){return source_alpha;}
void check(bool b,const std::string &s){++checks;if(!b)throw std::runtime_error(s);}
std::vector<unsigned char> bytes(const std::string &s){std::vector<unsigned char>b;for(size_t i=0;i<s.size();i+=2)b.push_back(std::stoul(s.substr(i,2),nullptr,16));return b;}
template<class T>T read(const std::vector<unsigned char>&b,size_t o){T v;std::memcpy(&v,b.data()+o,sizeof v);return v;}
FullScoreboardRect rect(const J&j){return {j[0].get<int16_t>(),j[1].get<int16_t>(),j[2].get<int16_t>(),j[3].get<int16_t>()};}
FullScoreboardWidget widget(const J&j){FullScoreboardWidget w;w.valid=j["valid"].get<int>()!=0;w.right_justified=j["right_justified"].get<int>()!=0;w.rectangle=rect(j["rectangle"]);w.text_rectangle=rect(j["text_rectangle"]);auto b=bytes(j["text_utf16_hex"]);for(size_t i=0;i+1<b.size();i+=2){auto c=read<char16_t>(b,i);if(!c)break;w.text+=c;}for(int i=0;i<4;++i){w.foreground[i]=std::bit_cast<float>(uint32_t(std::stoul(j["text_color_bits"][i].get<std::string>(),nullptr,16)));w.background[i]=std::bit_cast<float>(uint32_t(std::stoul(j["background_bits"][i].get<std::string>(),nullptr,16)));}return w;}
void vista_sampling(const J &fixture,const RetailFont &original,const char *path,
    const FullScoreboardPaintConstants &constants,const FullScoreboardPaintFrame &frame) {
  auto sampled=original;
  check(apply_retail_font_sampling(sampled,load_retail_font(path))>0,"Vista scoreboard font has admitted artwork");
  FullScoreboardPainter old_painter(original,5,6),vista_painter(sampled,5,6);
  unsigned admitted=0,clipped=0;
  for(const auto &entry:fixture["text_cases"]) {
    FullScoreboardLayout layout;layout.status=FullScoreboardStatus::Ready;layout.title=widget(entry["widget"]);
    const auto old=old_painter.draw(layout,constants,frame),hd=vista_painter.draw(layout,constants,frame);
    check(old.draw.vertices.size()==hd.draw.vertices.size(),"Vista scoreboard preserves glyph count");
    for(size_t n=0;n<old.draw.vertices.size();n+=6) {
      const auto &a=old.draw.vertices[n],&b=old.draw.vertices[n+4];
      const int x=std::lround(a.uv.x*original.atlas.width),y=std::lround(a.uv.y*original.atlas.height);
      const int width=std::lround(b.uv.x*original.atlas.width)-x,height=std::lround(b.uv.y*original.atlas.height)-y;
      for(const auto code:layout.title.text) {
        const auto found=original.glyphs.find(code);if(found==original.glyphs.end())continue;
        const auto &g=found->second;
        if(x<int(g.x)||y<int(g.y)||x+width>int(g.x+g.width)||y+height>int(g.y+g.height))continue;
        const auto uv=retail_font_sample_uvs(sampled,code,x-g.x,y-g.y,width,height);
        check(std::abs(hd.draw.vertices[n].uv.x-uv[0])<1e-6f &&
              std::abs(hd.draw.vertices[n].uv.y-uv[1])<1e-6f &&
              std::abs(hd.draw.vertices[n+4].uv.x-uv[2])<1e-6f &&
              std::abs(hd.draw.vertices[n+4].uv.y-uv[3])<1e-6f,
              "Scoreboard still samples Xbox glyph pixels instead of the admitted Vista artwork");
        admitted+=sampled.sampled_glyphs.contains(code);
        clipped+=width<int(g.width)||height<int(g.height);break;
      }
      for(size_t k=0;k<6;++k)
        check(std::memcmp(&old.draw.vertices[n+k].position,&hd.draw.vertices[n+k].position,sizeof(Vec3))==0 &&
              std::memcmp(&old.draw.vertices[n+k].color,&hd.draw.vertices[n+k].color,sizeof(Vec3))==0,
              "Vista scoreboard changes current layout or source colors");
    }
  }
  check(admitted>20 && clipped>0,"Vista scoreboard regression covers artwork and clipped glyphs");
  std::cout<<admitted<<" Vista scoreboard glyphs, including "<<clipped<<" clipped quads, preserve current layout\n";
}
int main(int argc,char**argv){try{if(argc!=3 && argc!=4)return 2;if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;J j;std::ifstream(argv[1])>>j;const auto f=load_retail_font(argv[2]);FullScoreboardPaintConstants c;auto word=[&](const char*s){return read<uint32_t>(bytes(j["style_words"][s]),0);};c.bright_rgb_threshold=word("0046a438");c.pixel_scale=word("0046a148");c.pulse_phase=word("0046a2ec");c.pulse_one=word("0046a0d0");c.pulse_half=word("0046a0cc");auto b=bytes(j["style_words"]["0044aaf8"]);for(int i=0;i<4;++i)c.shadow[i]=read<uint32_t>(b,i*4);FullScoreboardPaintFrame frame;frame.clip={0,0,480,640};frame.shadow_enabled=true;frame.x87_control_word=0x27f;FullScoreboardPainter painter(f,5,6);
for(const auto &p:j["pulse_cases"]){auto clock=bytes(p["clock12_hex"]);frame.clock={read<int16_t>(clock,2),read<float>(clock,4),read<int32_t>(clock,8)};auto rowraw=bytes(p["row1c8_hex"]);FullScoreboardLayout l;l.status=FullScoreboardStatus::Ready;FullScoreboardRow r;r.current_player=true;r.widgets[2].valid=true;r.widgets[2].rectangle={read<int16_t>(rowraw,0x90),read<int16_t>(rowraw,0x92),read<int16_t>(rowraw,0x94),read<int16_t>(rowraw,0x96)};for(int i=0;i<4;++i)r.widgets[2].foreground[i]=read<float>(rowraw,0xb4+4*i);r.widgets[3].valid=rowraw[0x138];r.widgets[4].valid=rowraw[0x180];r.widgets[5].valid=rowraw[0x1c0];l.rows.push_back(r);source_alpha=read<float>(bytes(p["rectangles"][0]["color16_hex"]),0);auto host=full_scoreboard_host_pulse(frame.clock,std::bit_cast<float>(c.pulse_phase),std::bit_cast<float>(c.pulse_one),std::bit_cast<float>(c.pulse_half),r.widgets[2].foreground[0],frame.x87_control_word);check(host.has_value(),"host math provider available");if(std::bit_cast<uint32_t>(*host)!=std::bit_cast<uint32_t>(source_alpha))std::cout<<"math-provider-boundary tick="<<frame.clock.tick<<" rate="<<frame.clock.rate<<" source="<<std::hex<<std::bit_cast<uint32_t>(source_alpha)<<" host="<<std::bit_cast<uint32_t>(*host)<<std::dec<<"\n";auto out=painter.draw(l,c,frame,source_pulse);check(out.draw.vertices.size()==30,"pulse count");for(int k=0;k<4;++k){auto rectraw=bytes(p["rectangles"][k]["rectangle8_hex"]),colorraw=bytes(p["rectangles"][k]["color16_hex"]);auto &v=out.draw.vertices;size_t o=6+k*6;check(v[o].position.x==read<int16_t>(rectraw,2),"pulse left");check(v[o].position.y==read<int16_t>(rectraw,0),"pulse top");check(v[o+4].position.x==read<int16_t>(rectraw,6),"pulse right");check(v[o+4].position.y==read<int16_t>(rectraw,4),"pulse bottom");check(std::bit_cast<uint32_t>(v[o].position.z)==read<uint32_t>(colorraw,0),"pulse alpha tick"+std::to_string(frame.clock.tick));check(std::bit_cast<uint32_t>(v[o].color.x)==read<uint32_t>(colorraw,4),"pulse RGB");}}
for(const auto &p:j["text_cases"]){FullScoreboardLayout l;l.status=FullScoreboardStatus::Ready;l.title=widget(p["widget"]);auto out=painter.draw(l,c,frame);check(out.status==FullScoreboardPaintStatus::Ready,"text status");const auto &quads=p["quads80_hex"];if(out.draw.vertices.size()!=quads.size()*6){std::cerr<<p["label"]<<" native="<<out.draw.vertices.size()/6<<" source="<<quads.size()<<"\n";for(size_t n=0;n<out.draw.vertices.size();n+=6)std::cerr<<"N "<<out.draw.vertices[n].position.x<<","<<out.draw.vertices[n].position.y<<".."<<out.draw.vertices[n+4].position.x<<","<<out.draw.vertices[n+4].position.y<<"\n";for(auto q:quads){auto b=bytes(q);std::cerr<<"S "<<read<float>(b,0)<<","<<read<float>(b,4)<<".."<<read<float>(b,40)<<","<<read<float>(b,44)<<"\n";}}check(out.draw.vertices.size()==quads.size()*6,"glyph count"+p["label"].get<std::string>());for(size_t k=0;k<quads.size();++k){auto q=bytes(quads[k]);auto &v=out.draw.vertices;for(auto [native,orig]:{std::pair{0,0},std::pair{1,1},std::pair{4,2},std::pair{2,3}}){const auto &x=v[k*6+native];check(x.position.x==read<float>(q,orig*20),"glyph x");check(x.position.y==read<float>(q,orig*20+4),"glyph y");const auto rgba=read<uint32_t>(q,orig*20+16);const auto expected=frontend_text_pixel_color(rgba);check(x.position.z==expected.alpha&&x.color.x==expected.red&&x.color.y==expected.green&&x.color.z==expected.blue,"glyph packed color");}check(std::isfinite(v[k*6].uv.x)&&v[k*6].uv.x>=0&&v[k*6].uv.x<=1,"actual atlas U");check(std::isfinite(v[k*6].uv.y)&&v[k*6].uv.y>=0&&v[k*6].uv.y<=1,"actual atlas V");}}
const auto &bg=j["background_fragment"];const auto &original=bg["source_layout"];FullScoreboardLayout composed;composed.status=FullScoreboardStatus::Ready;composed.title=widget(original["title"]);std::array<std::string,4> headerNames{"place","name","wins","score"};for(int i=0;i<4;++i)composed.headers[i]=widget(original["headers"][headerNames[i]]);std::array<std::string,6> names{"place","icon","name","wins","score","connection"};for(const auto &p:original["players"]){FullScoreboardRow r;r.current_player=p["current_player"].get<int>()!=0;for(int i=0;i<6;++i)r.widgets[i]=widget(p["widgets"][names[i]]);r.icon_kind=read<int16_t>(bytes(p["widgets"]["icon"]["raw_hex"]),0x38);composed.rows.push_back(r);}auto clockraw=bytes(bg["clock12_hex"]);frame.clock={read<int16_t>(clockraw,2),read<float>(clockraw,4),read<int32_t>(clockraw,8)};source_alpha=read<float>(bytes(bg["rectangles"][9]["color16_hex"]),0);auto painted=painter.draw(composed,c,frame,source_pulse);check(painted.status==FullScoreboardPaintStatus::Partial&&painted.emblems_unavailable&&!painted.connection_quality_unavailable,"declared emblem boundary");const size_t backgrounds=bg["rectangles"].size();for(size_t k=0;k<backgrounds;++k){auto rb=bytes(bg["rectangles"][k]["rectangle8_hex"]),cb=bytes(bg["rectangles"][k]["color16_hex"]);const auto &a=painted.draw.vertices[k*6],&b=painted.draw.vertices[k*6+4];check(a.material==6,"background before text");check(a.position.x==read<int16_t>(rb,2)&&a.position.y==read<int16_t>(rb,0)&&b.position.x==read<int16_t>(rb,6)&&b.position.y==read<int16_t>(rb,4),"ordered actual row rectangles");check(std::bit_cast<uint32_t>(a.position.z)==read<uint32_t>(cb,0)&&std::bit_cast<uint32_t>(a.color.x)==read<uint32_t>(cb,4)&&std::bit_cast<uint32_t>(a.color.y)==read<uint32_t>(cb,8)&&std::bit_cast<uint32_t>(a.color.z)==read<uint32_t>(cb,12),"ordered background raw ARGB");}size_t cursor=backgrounds*6;for(int row=0;row<4;++row)for(const auto &name:std::array<std::string,3>{"place","name","score"}){auto label=std::to_string(row)+"_"+name;const J *found=nullptr;for(const auto &p:j["text_cases"])if(p["label"]==label)found=&p;check(found!=nullptr,"actual text case available");for(const auto &q:(*found)["quads80_hex"]){const auto raw=bytes(q);const auto &a=painted.draw.vertices[cursor];check(a.material==5&&a.position.x==read<float>(raw,0)&&a.position.y==read<float>(raw,4),"actual place/name/score draw order");cursor+=6;}}check(cursor==painted.draw.vertices.size(),"complete supported source draw order");
if(argc==4)vista_sampling(j,f,argv[3],c,frame);
FullScoreboardLayout invalid;check(painter.draw(invalid,c,frame).status==FullScoreboardPaintStatus::InvalidInput,"invalid layout");FullScoreboardLayout l;l.status=FullScoreboardStatus::Ready;l.title=widget(j["text_cases"][0]["widget"]);l.title.text=u"|r";check(painter.draw(l,c,frame).text_controls_unavailable==1,"unavailable controls");l.title.text=u"abc";frame.clip={0,1,0,2};check(painter.draw(l,c,frame).status==FullScoreboardPaintStatus::InvalidInput,"invalid clip");frame.clip={0,0,480,640};auto bad=c;bad.shadow[0]=0x7fc00000;check(painter.draw(l,bad,frame).status==FullScoreboardPaintStatus::InvalidInput,"nonfinite shadow");check(!full_scoreboard_host_pulse({0,0,0},1,1,1,1,0x27f),"zero rate");check(!full_scoreboard_host_pulse({30,0,0},1,1,1,1,0),"unmasked host FPU");std::cout<<checks<<" checks passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
