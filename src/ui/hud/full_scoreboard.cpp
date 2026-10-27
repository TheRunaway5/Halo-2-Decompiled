#include "halo2/full_scoreboard.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
float word(std::uint32_t b) { return std::bit_cast<float>(b); }
float add(float a, float b) { volatile float x = a + b; return x; }
float sub(float a, float b) { volatile float x = a - b; return x; }
float mul(float a, float b) { volatile float x = a * b; return x; }
float div(float a, float b) { volatile float x = a / b; return x; }
float clamp_source(float x, float one) { // COMISS unordered follows source JBE
  if (0.f > x) return 0.f;
  if (x > one) return one;
  return x;
}
std::int16_t short_word(std::int32_t x) {
  return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(x));
}
std::int32_t cvtt(float x) {
  if (!std::isfinite(x) || x < -2147483648.f || x >= 2147483648.f)
    return std::numeric_limits<std::int32_t>::min();
  return static_cast<std::int32_t>(x);
}
bool exhausted(const FullScoreboardPlayer& p, const FullScoreboardOptions& o) {
  return o.life_limit > 0 && p.current_unit == 0xffffffffu && p.lives_remaining == 0;
}
template<class Stats>
int compare_statistics(const Stats& a, const Stats& b, const FullScoreboardOptions& o) {
  if (!o.game_over) return int(b.score) - int(a.score);
  int d = int(b.wins) - int(a.wins);
  if (d) return d;
  d = int(b.total_score) - int(a.total_score);
  if (d || !o.extended_round_comparison) return d;
  d = int(b.stat8) - int(a.stat8);
  if (d) return d;
  d = int(a.stat_a) - int(b.stat_a);
  if (d) return d;
  return int(b.stat10) - int(a.stat10);
}
int compare_team(unsigned a, unsigned b, const FullScoreboardOptions& o,
                 std::span<const FullScoreboardTeam> teams, bool secondary) {
  // Original248C90 selectors10,12..18. Selector11 (team exhaustion) is absent
  // from all three descriptor lists used by the roster builder.
  if (a==b) return 0;
  int d=int(teams[b].playing)-int(teams[a].playing);
  if (d) return d;
  d=compare_statistics(teams[a],teams[b],o);
  if (d) return d;
  return secondary ? int(a)-int(b) : 0;
}
int compare(const FullScoreboardPlayer& a, const FullScoreboardPlayer& b,
            const FullScoreboardOptions& o, std::span<const FullScoreboardTeam> teams,
            bool secondary) {
  // Selector0 explicitly returns0 in team mode, including suppressed players.
  int d=o.team_mode ? 0 : int(!(b.flags & 2))-int(!(a.flags & 2));
  if (d) return d;
  d=int(!exhausted(b,o))-int(!exhausted(a,o));
  if (d) return d;
  d=int(b.membership!=-1)-int(a.membership!=-1);
  if (d) return d;
  if (o.team_mode && a.membership!=-1 && b.membership!=-1) {
    d=compare_team(unsigned(a.membership),unsigned(b.membership),o,teams,secondary);
    if (d) return d;
  }
  return compare_statistics(a,b,o);
}
template<class T, class Compare>
void original_sort(std::vector<T>& p, int lo, int hi, const Compare& compare) {
  // Actual140910/140A60 quicksort plus140BC0/140C20 selection sort. Equality
  // does not use native stable_sort or a fabricated slot/name tie breaker.
  if (hi <= lo) return;
  if (hi - lo + 1 <= 8) {
    for (int last = hi; last > lo; --last) {
      int best = lo;
      for (int i = lo + 1; i <= last; ++i)
        if (compare(p[i], p[best]) > 0) best = i;
      std::swap(p[best], p[last]);
    }
    return;
  }
  std::swap(p[lo], p[lo + (hi - lo + 1) / 2]);
  int front = lo, back = hi + 1;
  for (;;) {
    do { ++front; } while (front <= hi && compare(p[front], p[lo]) <= 0);
    do { --back; } while (back > lo && compare(p[lo], p[back]) <= 0);
    if (back < front) break;
    std::swap(p[back], p[front]);
  }
  std::swap(p[lo], p[back]);
  // Original processes the shorter segment first and saves the longer one.
  if ((back - lo - 1) >= (hi - front)) {
    original_sort(p, front, hi, compare);
    original_sort(p, lo, back - 1, compare);
  } else {
    original_sort(p, lo, back - 1, compare);
    original_sort(p, front, hi, compare);
  }
}
std::u16string number(std::int32_t n) {
  const auto s = std::to_string(n);
  return std::u16string(s.begin(), s.end());
}
std::u16string score_text(std::int32_t n, int mode) {
  if (mode != 3 && mode != 4 && mode != 8) return number(n);
  const int minutes = n / 60, seconds = n - minutes * 60;
  auto s = minutes ? number(minutes) : std::u16string(u" "); // original437544 literal
  s += u":";
  if (seconds <= 9) s += u"0";
  s += number(seconds);
  return s;
}
FullScoreboardWidget widget(int x, int y, int width, int capacity, bool right,
    std::u16string_view text, std::array<float, 4> bg, std::array<float, 4> fg) {
  FullScoreboardWidget w;
  w.valid = true; w.right_justified = right;
  w.rectangle = {short_word(y), short_word(x), short_word(y + 19), short_word(x + width - 1)};
  w.text_rectangle = {short_word(y - 1), short_word(x + 2), short_word(y + 19), short_word(x + width - 3)};
  w.background = bg; w.foreground = fg;
  w.text = text.substr(0, std::min(text.find(u'\0'), static_cast<std::size_t>(capacity - 1)));
  return w;
}
float row_alpha(float a, int k, int n, const FullScoreboardConstants& c) {
  return clamp_source(mul(sub(a, mul(div(float(k), float(n)), word(c.stagger_fraction))),
                          word(c.stagger_scale)), word(c.one));
}
int cap_integer(float value, FullScoreboardRounding r) {
  switch (r) {
  case FullScoreboardRounding::Down: return int(std::floor(value));
  case FullScoreboardRounding::Up: return int(std::ceil(value));
  case FullScoreboardRounding::TowardZero: return int(value);
  case FullScoreboardRounding::NearestEven: {
    const float f = std::floor(value), rem = value - f;
    return int(f) + (rem > .5f || (rem == .5f && (int(f) & 1)));
  }
  }
  return 0;
}
} // namespace

std::u16string full_scoreboard_score_text(std::int32_t score,std::int32_t mode) {
  return score_text(score,mode);
}

FullScoreboardRanks full_scoreboard_ranks(std::span<const FullScoreboardPlayer> players,
    const FullScoreboardOptions& o,std::span<const FullScoreboardTeam> teams) {
  FullScoreboardRanks result;result.players.fill(0xff);result.teams.fill(0xff);
  if(players.size()>16 || (o.team_mode && teams.size()!=8))
    throw std::invalid_argument("Incomplete original ranking roster");
  std::vector<const FullScoreboardPlayer*> sorted;
  std::uint32_t occupied{};
  for(const auto &p:players) {
    const unsigned slot=p.datum&0xffff;
    if(p.datum==0xffffffffu || slot>=16 || !(p.datum>>16) || (occupied&(1u<<slot)) ||
        (o.team_mode && (p.membership < -1 || p.membership>7)))
      throw std::invalid_argument("Invalid original ranking participant");
    occupied|=1u<<slot;sorted.push_back(&p);
  }
  std::sort(sorted.begin(),sorted.end(),[](auto a,auto b){return (a->datum&0xffff)<(b->datum&0xffff);});
  original_sort(sorted,0,int(sorted.size())-1,[&](auto a,auto b){return compare(*a,*b,o,teams,true);});
  for(const auto *p:sorted) {
    unsigned better{},tied{};
    for(const auto *q:sorted)if(q!=p) {
      const int d=compare(*p,*q,o,teams,false);
      if(d>0)++better;else if(!d)++tied;else break;
    }
    result.players[p->datum&0xffff]=std::uint8_t(2*better+(tied!=0));
  }
  if(o.team_mode) {
    std::vector<unsigned> sorted_teams;
    for(unsigned i=0;i<8;++i)if(teams[i].retained)sorted_teams.push_back(i);
    original_sort(sorted_teams,0,int(sorted_teams.size())-1,[&](auto a,auto b){return compare_team(a,b,o,teams,true);});
    for(const auto i:sorted_teams) {
      unsigned better{},tied{};
      for(const auto j:sorted_teams)if(j!=i) {
        const int d=compare_team(i,j,o,teams,false);
        if(d>0)++better;else if(!d)++tied;else break;
      }
      result.teams[i]=std::uint8_t(2*better+(tied!=0));
    }
  }
  return result;
}

FullScoreboardLayout full_scoreboard_layout(std::span<const FullScoreboardPlayer> players,
    std::uint32_t current, const FullScoreboardOptions& o,
    FullScoreboardRect outer, FullScoreboardRect inner, float scroll, float alpha,
    const FullScoreboardLabels& labels, const FullScoreboardConstants& c,
    std::span<const FullScoreboardTeam> teams) {
  FullScoreboardLayout out;
  if (!o.engine_present || players.size() > 16 || (o.team_mode && teams.size()!=8)) return out;
  std::vector<const FullScoreboardPlayer*> sorted;
  const FullScoreboardPlayer* local = nullptr;
  std::uint32_t occupied = 0;
  for (const auto& p : players) {
    const unsigned slot = p.datum & 65535;
    if (p.datum == 0xffffffffu || slot >= 16 || !(p.datum >> 16) ||
        (occupied & (1u << slot)) ||
        (o.team_mode && (p.membership < -1 || p.membership > 7))) return out;
    occupied |= 1u << slot;
    if (p.datum == current) local = &p;
    sorted.push_back(&p);
  }
  if (!local || local->local_view < 0 || local->local_view >= 4) return out;
  const auto ranks=full_scoreboard_ranks(players,o,teams);
  // Source occupied bitmap enumeration precedes its own non-stable sort.
  std::sort(sorted.begin(), sorted.end(), [](auto a, auto b) { return (a->datum & 65535) < (b->datum & 65535); });
  original_sort(sorted, 0, int(sorted.size()) - 1,
      [&](auto a, auto b) { return compare(*a,*b,o,teams,true); });
  std::vector<unsigned> sorted_teams;
  if (o.team_mode) {
    for (unsigned i=0;i<8;++i) if (teams[i].retained) sorted_teams.push_back(i);
    original_sort(sorted_teams,0,int(sorted_teams.size())-1,
        [&](unsigned a,unsigned b) { return compare_team(a,b,o,teams,true); });
  }
  out.status = FullScoreboardStatus::Ready;
  out.columns = {o.game_over || o.score_limit != 1, o.round_limit != 0,
                 o.round_limit != 0 && !o.team_mode};
  out.input_alpha = alpha;
  const int content_height = int(sorted.size()+sorted_teams.size())*20+44+
                             (sorted_teams.empty()?0:21);
  const int inner_height = short_word(int(inner.bottom) - inner.top);
  if (content_height < inner_height) scroll = 0;
  else if (0.f > scroll) scroll = 0;
  else if (scroll > float(content_height - inner_height)) scroll = float(content_height - inner_height);
  out.scroll = scroll;
  const int width = short_word(int(inner.right) - inner.left);
  const int x = short_word(width / 2 - outer.left + inner.left) - 134;
  const auto dy = static_cast<std::uint32_t>(inner.top) - static_cast<std::uint32_t>(outer.top)
                - static_cast<std::uint32_t>(cvtt(scroll));
  int y = short_word(std::bit_cast<std::int32_t>(dy));
  const int n = int(sorted.size()+sorted_teams.size())+2;
  const float one = word(c.one);
  const float title_alpha = row_alpha(alpha, 0, n, c);
  out.title = widget(x, y, 268, 80, false, labels.title,
                     {0,0,0,0}, {title_alpha,one,one,one});
  y = short_word(y + 24);
  const float ha = row_alpha(alpha, 1, n, c), hr = word(c.header_rgb);
  const std::array<float,4> hb{ha,hr,hr,hr}, hf{ha,one,one,one};
  const int wins_width = out.columns[1] ? 50 : 0, score_width = out.columns[0] ? 54 : 0;
  const int header_name_width = 236 - wins_width - score_width;
  out.headers[0] = widget(x,y,25,20,false,{},hb,hf);
  out.headers[1] = widget(x+25,y,header_name_width,20,false,labels.name,hb,hf);
  if (wins_width) out.headers[2] = widget(x+25+header_name_width,y,wins_width,20,true,labels.wins,hb,hf);
  if (score_width) out.headers[3] = widget(x+25+header_name_width+wins_width,y,score_width,20,true,labels.score,hb,hf);
  std::array<std::optional<std::uint8_t>,8> team_ranks;
  for (unsigned i=0;i<sorted_teams.size();++i) {
    const unsigned index=sorted_teams[i];const auto& team=teams[index];
    const auto rank=ranks.teams[index];team_ranks[index]=rank;
    const float a=row_alpha(alpha,int(i)+2,n,c);
    const std::array<float,4> bg{mul(a,word(c.background_alpha)),team.color[0],team.color[1],team.color[2]};
    std::array<float,4> fg{a,0,0,0};
    for (unsigned j=0;j<3;++j) {
      fg[j+1]=add(mul(team.color[j],word(c.text_scale)),word(c.text_bias));
      if (!team.playing) fg[j+1]=mul(fg[j+1],word(c.dim_scale));
    }
    y=short_word(y+20);
    FullScoreboardTeamRow row;row.team=std::uint8_t(index);row.encoded_rank=rank;
    row.widgets[0]=widget(x,y,25,6,true,number(rank/2+1),bg,fg);
    row.widgets[1]=widget(x+25,y,header_name_width,20,false,team.display_name,bg,fg);
    if (wins_width) row.widgets[2]=widget(x+25+header_name_width,y,50,6,true,number(team.wins),bg,fg);
    if (score_width) row.widgets[3]=widget(x+25+header_name_width+wins_width,y,54,6,true,
        score_text(o.game_over?team.total_score:team.score,o.display_mode),bg,fg);
    out.team_rows.push_back(std::move(row));
  }
  if (!sorted_teams.empty()) y=short_word(y+21);
  for (unsigned i = 0; i < sorted.size(); ++i) {
    const auto& p = *sorted[i];
    y = short_word(y + 20);
    const bool hide = p.membership == -1;
    const bool dim = (p.flags & 2) || !(p.flags & 1) || exhausted(p,o) || hide;
    const bool dead_icon = !hide && p.current_unit == 0xffffffffu && !(p.flags & 2);
    const bool hide_connection = (p.flags & 2) != 0;
    const float a = row_alpha(alpha,int(i)+int(sorted_teams.size())+2,n,c);
    const std::array<float,4> bg{mul(a,word(c.background_alpha)),p.color[0],p.color[1],p.color[2]};
    std::array<float,4> fg{a,0,0,0};
    for (unsigned j=0;j<3;++j) {
      fg[j+1] = add(mul(p.color[j],word(c.text_scale)),word(c.text_bias));
      if (dim) fg[j+1] = mul(fg[j+1],word(c.dim_scale));
    }
    FullScoreboardRow row;
    row.datum=p.datum;row.encoded_rank=ranks.players[p.datum&0xffff];row.current_player=p.datum==current;
    row.icon_kind = dead_icon && o.engine_active ? 1 : dim ? 2 : 0;
    //162E70 consumes column byte2 for player wins;162B70 consumes byte1
    // for team wins. Team mode clears byte2 even when round_limit is nonzero.
    const int player_wins_width=out.columns[2]?50:0;
    const int name_width=216-score_width-player_wins_width;
    if (!hide) {
      const auto rank=o.team_mode && team_ranks[unsigned(p.membership)] ?
          *team_ranks[unsigned(p.membership)] : row.encoded_rank;
      row.widgets[0]=widget(x,y,25,6,true,number(rank/2+1),bg,fg);
      row.widgets[1]=widget(x+25,y,20,6,false,{},bg,fg);
    }
    row.widgets[2]=widget(x+45,y,name_width,20,false,p.display_name,bg,fg);
    if (player_wins_width && !hide) row.widgets[3]=widget(x+45+name_width,y,50,6,true,number(p.wins),bg,fg);
    if (score_width && !hide) row.widgets[4]=widget(x+45+name_width+player_wins_width,y,54,6,true,score_text(o.game_over?p.total_score:p.score,o.display_mode),bg,fg);
    if (!hide_connection && p.connection_quality) {
      row.widgets[5]=widget(x+45+name_width+player_wins_width+score_width,y,7,2,true,{},bg,fg);
      row.widgets[5].text.assign(1,std::bit_cast<std::uint16_t>(*p.connection_quality));
    }
    out.rows.push_back(std::move(row));
  }
  return out;
}

bool full_scoreboard_update_held(FullScoreboardHeldState& state,
    const FullScoreboardHeldInput& in, FullScoreboardClock clock,
    FullScoreboardRounding rounding, const FullScoreboardConstants& c) {
  if (in.local_view == -1 || in.input_user == -1) return true;
  if (in.local_view < 0 || in.local_view >= 4 || in.input_user < 0 || in.input_user >= 4 || clock.rate <= 0) return false;
  const float cap = mul(float(clock.rate),word(c.counter_seconds));
  if (!std::isfinite(cap) || cap < 0 || cap > 32767) return false;
  const bool active = in.engine_present && in.lifecycle == 1 &&
                      (in.options_state == 4 || in.engine_status == 1);
  if (active) state.inactive_counter = 0;
  else state.inactive_counter = std::uint8_t(std::min(int(state.inactive_counter)+1,int(clock.rate)));
  const bool dead = in.current_unit == 0xffffffffu && in.death_countdown <= 1 &&
                    !(in.player_flags & 0x4000) && !in.exhausted &&
                    clock.tick > int(clock.rate)*3 && (in.player_flags & 1);
  const bool admit = ((in.processed_record_present && in.held) ||
                      int(state.inactive_counter) == clock.rate || dead) && !in.death_observer_excluded;
  auto& counter = state.counters[std::size_t(in.local_view)];
  if (!admit) { counter = counter ? std::uint8_t(counter-1) : 0; return true; }
  counter = std::uint8_t(std::min(int(counter)+1,cap_integer(cap,rounding)));
  if (in.processed_record_present) {
    const float axis = mul(float(-int(in.scroll_axis)),word(c.scroll_axis_scale));
    const float pixels = div(word(c.scroll_pixels),float(clock.rate));
    state.scroll[std::size_t(in.local_view)] = add(mul(axis,pixels),state.scroll[std::size_t(in.local_view)]);
  }
  return true;
}
float full_scoreboard_fade(std::uint8_t counter, FullScoreboardClock clock,
                           const FullScoreboardConstants& c) {
  return clamp_source(mul(mul(float(counter),clock.seconds_per_tick),word(c.fade_scale)),word(c.one));
}
bool full_scoreboard_normal_hud(bool engine, std::int16_t view,
    std::uint8_t counter, FullScoreboardClock clock, const FullScoreboardConstants& c) {
  if (!engine || view < 0 || view >= 4) return true;
  const float a=full_scoreboard_fade(counter,clock,c);
  // Source COMISS/JB treats unordered as below and retains the normal HUD.
  return a < word(c.one) || std::isnan(a) || 0.f > a;
}
float full_scoreboard_display_alpha(std::uint8_t counter, FullScoreboardClock clock,
    const FullScoreboardConstants& c, FullScoreboardPower power) {
  const float fade=full_scoreboard_fade(counter,clock,c);
  if (!(fade > 0)) return 0;
  if (!power) throw std::invalid_argument("full scoreboard needs a power provider");
  return power(fade,c.fade_exponent);
}
float full_scoreboard_host_power(float base,std::uint64_t exponent) {
  return static_cast<float>(std::pow(static_cast<long double>(base),
                                   static_cast<long double>(std::bit_cast<double>(exponent))));
}
} // namespace halo2
