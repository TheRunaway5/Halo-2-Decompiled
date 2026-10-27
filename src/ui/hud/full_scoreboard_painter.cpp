#include "halo2/full_scoreboard_painter.hpp"
#include "halo2/frontend_text_style.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
float word(std::uint32_t bits) { return std::bit_cast<float>(bits); }
std::int16_t short_word(int value) {
  return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value));
}
bool finite(const std::array<float, 4> &color) {
  return std::all_of(color.begin(), color.end(), [](float v) { return std::isfinite(v); });
}
void quad(UiDraw &draw, float x, float y, float width, float height,
          unsigned material, UiColor color, float u0=0, float v0=0,
          float u1=1, float v1=1, bool admitted_glyph=false) {
  if (width < 0 || height < 0 || (!admitted_glyph && (width==0 || height==0))) return;
  const Vertex a{{x,y,color.alpha},{color.red,color.green,color.blue},{u0,v0},material},
      b{{x+width,y,color.alpha},{color.red,color.green,color.blue},{u1,v0},material},
      c{{x+width,y+height,color.alpha},{color.red,color.green,color.blue},{u1,v1},material},
      d{{x,y+height,color.alpha},{color.red,color.green,color.blue},{u0,v1},material};
  draw.vertices.insert(draw.vertices.end(),{a,b,d,b,c,d});
}
UiColor color(const std::array<float,4> &c) { return {c[0],c[1],c[2],c[3]}; }
std::array<float,4> text_color(std::array<float,4> c, float maximum) {
  // Complete141750/141640: preserve the source branch and float order.
  if (unsigned(c[1]>maximum) & unsigned(c[2]>maximum) & unsigned(c[3]>maximum)) {
    float minimum=c[2]>c[3] ? c[3] : c[2];
    minimum=c[1]>minimum ? minimum : c[1];
    if (maximum>minimum) minimum=maximum;
    else if (minimum>1.f) minimum=1.f;
    const float scale=maximum/minimum;
    c[1]*=scale; c[2]*=scale; c[3]*=scale;
  }
  for (auto &component:c) component=std::clamp(component,0.f,1.f);
  return c;
}
std::uint32_t pixel(const std::array<float,4> &c, float scale) {
  std::uint32_t result=0;
  //141F50 multiplies in x87 and truncates, without an intervening float store.
  for (float component:c)
    result=(result<<8)|std::uint32_t(double(component)*double(scale));
  return result;
}
bool unavailable_control(std::u16string_view text) {
  return std::any_of(text.begin(),text.end(),[](char16_t c) {
    return c==u'|' || c==u'\t' || (c>=0xe405 && c<=0xe407);
  });
}
float measure(std::u16string_view chars,const RetailFont &font,float origin,float right) {
  float width=0; unsigned previous=0,admitted=0,characters=0;
  for (const auto c:chars) {
    if (c==u'\n') continue; //142080 ignores LF; CR is the line separator.
    if (const auto it=font.glyphs.find(c);it!=font.glyphs.end()) {
      //141A20/1421E0 pass iterator.previous in EDI and current in EBX
      // to124F60.142080's initial two-kind state keeps previous=0
      // for the first two ordinary characters of each line.
      const auto k=font.kerning.find({characters==1?0u:previous,c});
      const float kern=k==font.kerning.end() ? 0.f : float(k->second);
      const float endpoint=origin+float(it->second.advance)+float(it->second.bearing_x)+kern+width;
      //141A20 with wrap flag0 scans the whole line but does not accumulate an
      // overflowing glyph's width, except for the first admitted glyph. The
      // drawing pass still traverses all those glyphs and clips their ink.
      if (endpoint<right || !admitted) {
        width=float(it->second.advance)+float(it->second.bearing_x)+kern+width;
        ++admitted;
      }
    }
    previous=c;++characters;
  }
  return width;
}
void text(UiDraw &draw,const FullScoreboardWidget &w,const RetailFont &font,
          unsigned material,const FullScoreboardPaintConstants &constants,
          const FullScoreboardPaintFrame &frame) {
  if (!w.valid || w.text.empty()) return;
  const auto &bounds=w.text_rectangle;
  const float left=std::max(int(bounds.left),int(frame.clip.left)),
      right=std::min(int(bounds.right),int(frame.clip.right)),
      top=std::max(int(bounds.top),int(frame.clip.top)),
      bottom=std::min(int(bounds.bottom),int(frame.clip.bottom));
  if (right<=left || bottom<=top) return;
  const auto normalized=text_color(w.foreground,word(constants.bright_rgb_threshold));
  const auto foreground=pixel(normalized,word(constants.pixel_scale));
  std::array<float,4> shadow;
  for (unsigned j=0;j<4;++j) shadow[j]=word(constants.shadow[j]);
  shadow[0]=std::min(shadow[0],normalized[0]);
  const auto shadow_pixel=frame.shadow_enabled ? pixel(shadow,word(constants.pixel_scale)) : 0;
  const auto visible=std::u16string_view(w.text).substr(0,w.text.find(u'\0'));
  std::size_t start=0;
  unsigned line=0;
  for (;;) {
    const auto end=visible.find(u'\r',start);
    const auto length=(end==std::u16string::npos ? visible.size() : end)-start;
    const auto chars=visible.substr(start,length);
    const float indent=line ? frame.next_line_indent : frame.first_line_indent;
    const float line_left=float(bounds.left)+indent;
    float x=float(frame.font_horizontal_offset)+line_left;
    // Widget byte+8 maps to source alignment2 (CENTER), despite the frozen
    // layout API's historical right_justified member name. No menu flag map.
    if (w.right_justified)
      x=((float(bounds.right)-line_left)-measure(chars,font,x,float(bounds.right)))*.5f+line_left;
    const float baseline=float((font.ascent+font.descent+font.leading)*int(line)+font.ascent)+float(bounds.top);
    if (baseline>=float(bounds.bottom)) break; //141BC0 strict baseline gate
    unsigned previous=0,characters=0;
    for (const auto c:chars) {
      if (c==u'\n') continue;
      const auto it=font.glyphs.find(c);
      if (it==font.glyphs.end()) { previous=c;++characters;continue; }
      const auto &g=it->second;
      const auto k=font.kerning.find({characters==1?0u:previous,c});
      const float kern=k==font.kerning.end() ? 0.f : float(k->second);
      const float gx=x+float(g.bearing_x)+kern,gy=baseline-float(g.bearing_y);
      const float l=std::max(left,gx),r=std::min(right,gx+float(g.width)),
          a=std::max(top,gy),b=std::min(bottom,gy+float(g.height));
      if (g.width && g.height && r>l && b>a) {
        // Unit-scale1F470 truncates the clipped dimensions and bitmap crop
        // offsets before1F380. The resulting atlas packing is caller-owned.
        const float width=float(int(r-l)),height=float(int(b-a));
        const auto uv=retail_font_sample_uvs(font,c,int(l-gx),int(a-gy),int(width),int(height));
        //1F470 shifts the already-clipped shadow without reclipping.
        if (shadow_pixel&0xff000000u)
          quad(draw,l+1.f,a+1.f,width,height,material,frontend_text_pixel_color(shadow_pixel),uv[0],uv[1],uv[2],uv[3],true);
        quad(draw,l,a,width,height,material,frontend_text_pixel_color(
            frontend_text_texture_color(c) ? foreground|0xffffffu : foreground),uv[0],uv[1],uv[2],uv[3],true);
      }
      x=float(g.advance)+x+float(g.bearing_x)+kern;
      previous=c;++characters;
    }
    if (end==std::u16string::npos) break;
    start=end+1; ++line;
  }
}
} // namespace

std::optional<float> full_scoreboard_host_pulse(FullScoreboardClock clock,
    float phase,float one,float half,float alpha,std::uint16_t control_word) {
  if (clock.rate<=0 || (control_word&0x3f)!=0x3f ||
      !std::isfinite(phase) || !std::isfinite(one) || !std::isfinite(half) || !std::isfinite(alpha)) return {};
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
  float result;
  std::uint16_t previous;
  const std::int32_t rate=clock.rate;
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tfidivl %5\n\tfsin\n\tfadds %6\n\tfmuls %7\n\tfmuls %8\n\tfstps %0\n\tfldcw %1"
      : "=m"(result),"=&m"(previous)
      : "m"(control_word),"m"(clock.tick),"m"(phase),"m"(rate),"m"(one),"m"(half),"m"(alpha)
      : "st");
  return std::isfinite(result) ? std::optional<float>(result) : std::nullopt;
#else
  return {};
#endif
}

FullScoreboardPainter::FullScoreboardPainter(const RetailFont &font6,
    unsigned font_material,unsigned rectangle_material)
    : font6_(font6),font_material_(font_material),rectangle_material_(rectangle_material) {}

FullScoreboardPaintResult FullScoreboardPainter::draw(const FullScoreboardLayout &layout,
    const FullScoreboardPaintConstants &constants,const FullScoreboardPaintFrame &frame,
    FullScoreboardPulse pulse,const FullScoreboardEmblem &emblem) const {
  FullScoreboardPaintResult result;
  if (layout.status!=FullScoreboardStatus::Ready || layout.rows.size()>16 || layout.team_rows.size()>8 ||
      !font6_.atlas.width || !font6_.atlas.height || font6_.ascent<0 ||
      font6_.descent<0 || font6_.leading<0 || font6_.ascent>128 ||
      font6_.descent>128 || font6_.leading>128 || frame.clip.right<=frame.clip.left ||
      frame.clip.bottom<=frame.clip.top) return result;
  const float threshold=word(constants.bright_rgb_threshold),scale=word(constants.pixel_scale);
  if (!std::isfinite(threshold) || threshold<=0 || threshold>1 ||
      !std::isfinite(scale) || scale<0 || scale>255) return result;
  for (auto bits:constants.shadow) if (!std::isfinite(word(bits)) || word(bits)<0 || word(bits)>1) return result;
  auto valid=[](const FullScoreboardWidget &w) { return !w.valid || (finite(w.foreground)&&finite(w.background)&&w.text.size()<=2047); };
  if (!valid(layout.title) || !std::all_of(layout.headers.begin(),layout.headers.end(),valid)) return result;
  for (const auto &row:layout.team_rows) if (!std::all_of(row.widgets.begin(),row.widgets.end(),valid)) return result;
  for (const auto &row:layout.rows) if (!std::all_of(row.widgets.begin(),row.widgets.end(),valid)) return result;
  auto background=[&](const FullScoreboardWidget &w) {
    if (w.valid) quad(result.draw,float(w.rectangle.left),float(w.rectangle.top),
        float(int(w.rectangle.right)-int(w.rectangle.left)),float(int(w.rectangle.bottom)-int(w.rectangle.top)),
        rectangle_material_,color(w.background));
  };
  //163EA0 / Vista4749F7: emblems first, header/team/player backgrounds, then
  // title/header/team/player text. Only player rows have quality/pulse/emblems.
  result.emblems_unavailable=!emblem && !layout.rows.empty();
  for(const auto &row:layout.rows) {
    const auto &w=row.widgets[1];
    if(!w.valid || row.icon_kind==2)continue;
    const auto material=emblem?emblem(row):std::nullopt;
    if(!material){result.emblems_unavailable=true;continue;}
    quad(result.draw,float(w.rectangle.left),float(w.rectangle.top),
        float(int(w.rectangle.right)-w.rectangle.left),float(int(w.rectangle.bottom)-w.rectangle.top),
        *material,{1,1,1,1});
  }
  for (const auto &w:layout.headers) background(w);
  for (const auto &row:layout.team_rows) for (const auto &w:row.widgets) background(w);
  for (const auto &row:layout.rows) {
    if (row.icon_kind==2) background(row.widgets[1]);
    for (unsigned j:{0u,2u,3u,4u,5u}) background(row.widgets[j]);
    // Vista46BF66 (Xbox1663B0): six three-pixel strips, with a black inset
    // backing and red/yellow/green quality color. Quality is source-owned;
    // absent measurements do not invent a ping or show a fabricated bar.
    const auto &connection=row.widgets[5];
    if(connection.valid) {
      if(connection.text.empty()){result.connection_quality_unavailable=true;}
      else {
        const int quality=std::clamp(int(connection.text[0]),1,6);
        const auto &r=connection.rectangle;const float alpha=connection.foreground[0];
        quad(result.draw,r.left+1,r.bottom-18,r.right-r.left-2,17,rectangle_material_,{alpha,0,0,0});
        const UiColor ink{alpha,quality<4?1.f:0.f,quality>=2?1.f:0.f,0};
        for(int bar=0;bar<quality;++bar)
          quad(result.draw,r.left+1,r.bottom-3-bar*3,r.right-r.left-2,2,rectangle_material_,ink);
      }
    }
    if (row.current_player) {
      const auto &name=row.widgets[2];
      const auto alpha=pulse ? pulse(frame.clock,word(constants.pulse_phase),word(constants.pulse_one),
          word(constants.pulse_half),name.foreground[0],frame.x87_control_word) : std::nullopt;
      if (!alpha || !std::isfinite(*alpha)) { result.pulse_unavailable=true;continue; }
      const auto &r=name.rectangle;
      const auto l=short_word(int(r.left)-45),t=short_word(int(r.top)-1),b=short_word(int(r.bottom)+1),
          right=short_word(int(r.right)+(row.widgets[5].valid?7:0)+(row.widgets[4].valid?54:0)+(row.widgets[3].valid?50:0));
      auto foreground=name.foreground;foreground[0]=*alpha;
      const std::array<FullScoreboardRect,4> strips{{{t,l,r.top,right},
          {t,right,b,short_word(int(right)+1)},{r.bottom,l,b,right},{t,short_word(int(l)-1),b,l}}};
      for (const auto &s:strips) quad(result.draw,float(s.left),float(s.top),
          float(int(s.right)-int(s.left)),float(int(s.bottom)-int(s.top)),rectangle_material_,color(foreground));
    }
  }
  auto draw_text=[&](const FullScoreboardWidget &w) {
    if (!w.valid) return;
    if (unavailable_control(std::u16string_view(w.text).substr(0,w.text.find(u'\0')))) { ++result.text_controls_unavailable;return; }
    text(result.draw,w,font6_,font_material_,constants,frame);
  };
  draw_text(layout.title);
  for (const auto &w:layout.headers) draw_text(w);
  for (const auto &row:layout.team_rows) for (const auto &w:row.widgets) draw_text(w);
  for (const auto &row:layout.rows) for (unsigned j:{0u,2u,3u,4u}) draw_text(row.widgets[j]);
  result.status=(result.emblems_unavailable || result.connection_quality_unavailable ||
      result.pulse_unavailable || result.text_controls_unavailable) ? FullScoreboardPaintStatus::Partial : FullScoreboardPaintStatus::Ready;
  return result;
}
} // namespace halo2
