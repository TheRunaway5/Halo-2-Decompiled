#include "halo2/vista_pcr_results.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <vector>

namespace halo2 {
namespace {
using Bank=std::span<const std::byte>;
std::uint32_t read(Bank b,std::size_t at,unsigned n=4) {
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned char>(b[at+i]))<<(8*i);return v;
}
std::int32_t signed_byte(Bank b,std::size_t at) {return std::bit_cast<std::int8_t>(std::uint8_t(read(b,at,1)));}
std::int32_t signed_word(Bank b,std::size_t at) {return std::bit_cast<std::int16_t>(std::uint16_t(read(b,at,2)));}
void write(std::span<std::byte> b,std::size_t at,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));
}
void admit(Bank b) {if(b.size()<vista_pcr_source_size)throw FormatError("Truncated Vista PCR source bank");}
bool finalized(Bank b) {return b[0x10]!=std::byte{} && b[0x11]!=std::byte{};}
std::size_t player(unsigned p) {return 0x370+p*0x94;}
std::size_t team(unsigned t) {return 0xcb0+t*0x18;}
//48CB18 selection sort is used for partitions of at most8.48CE4C's pivot
// exchange and partition traversal must be retained: std::sort changes ties.
template<class Greater> void source_sort(std::span<std::uint32_t> rows,Greater greater) {
  if(rows.size()<2)return;
  std::vector<std::pair<int,int>> pending;
  int lo=0,hi=int(rows.size())-1;
  for(;;) {
    if(hi-lo+1<=8) {
      for(int last=hi;last>lo;--last) {
        int best=lo;
        for(int i=lo+1;i<=last;++i)if(greater(rows[i],rows[best]))best=i;
        std::swap(rows[last],rows[best]);
      }
    } else {
      std::swap(rows[lo],rows[lo+(hi-lo+1)/2]);
      int left=lo,right=hi+1;
      for(;;) {
        do{++left;}while(left<=hi && !greater(rows[left],rows[lo]));
        do{--right;}while(right>lo && !greater(rows[lo],rows[right]));
        if(right<left)break;
        std::swap(rows[left],rows[right]);
      }
      std::swap(rows[lo],rows[right]);
      if(right-lo-1>=hi-left) {
        if(lo+1<right)pending.emplace_back(lo,right-1);
        if(left<hi){lo=left;continue;}
      } else {
        if(left<hi)pending.emplace_back(left,hi);
        if(lo+1<right){hi=right-1;continue;}
      }
    }
    if(pending.empty())return;
    const auto next=pending.back();pending.pop_back();lo=next.first;hi=next.second;
  }
}
void copy_text(std::span<std::byte> out,std::size_t at,unsigned capacity,
    std::u16string_view text) {
  //44BD27 copies at most capacity-1, writes a terminator, and leaves the rest.
  unsigned n{};while(n+1<capacity && n<text.size() && text[n]){write(out,at+2*n,text[n],2);++n;}
  write(out,at+2*n,0,2);
}
std::u16string player_name(Bank b,unsigned p) {
  std::u16string s;
  // Source466FDC returns the retained configuration name at person+C.
  for(unsigned i=0;i<32;++i){const auto c=read(b,player(p)+0xc+2*i,2);if(!c)break;s.push_back(char16_t(c));}
  return s;
}
std::int32_t stat(Bank b,unsigned p,unsigned n) {return std::int32_t(read(b,0xe30+2*(p*0x1b5+n),2)&0x7fff);}
std::int32_t player_placement(Bank b,unsigned p) {
  if((read(b,0x68,1)&1)==0)return signed_byte(b,player(p)+0x90);
  const auto t=signed_byte(b,player(p)+0x88);
  if(t<0 || t>15 || b[team(unsigned(t))]==std::byte{})return -1;
  return signed_byte(b,team(unsigned(t))+1);
}
}
VistaPcrOrdering vista_pcr_order(Bank source) {
  admit(source);VistaPcrOrdering out;
  if(!finalized(source))return out;
  for(unsigned i=0;i<16;++i) {
    if(source[player(i)]!=std::byte{})out.players[out.player_count++]=i;
    if(source[team(i)]!=std::byte{})out.teams[out.team_count++]=i;
  }
  const auto compare=[&](bool teams,unsigned a,unsigned b) {
    const auto aa=teams?team(a)+1:player(a)+0x90;
    const auto bb=teams?team(b)+1:player(b)+0x90;
    const auto rank=signed_byte(source,aa)-signed_byte(source,bb);
    return rank?rank>0:signed_word(source,bb+2)-signed_word(source,aa+2)>0;
  };
  source_sort(std::span(out.players).first(out.player_count),[&](auto a,auto b){return compare(false,a,b);});
  source_sort(std::span(out.teams).first(out.team_count),[&](auto a,auto b){
    // Team score starts+2, one byte after its rank (player score is+92).
    const auto rank=signed_byte(source,team(a)+1)-signed_byte(source,team(b)+1);
    return rank?rank>0:signed_word(source,team(b)+2)-signed_word(source,team(a)+2)>0;
  });
  return out;
}
void vista_pcr_build(VistaPcrResults& out,Bank b,const VistaPcrServices& services) {
  if(!services.engine_present)return;
  admit(b);
  const auto order=vista_pcr_order(b);
  if((order.player_count || (services.team_play && order.team_count)) &&
      (!services.score || !services.placement || (services.team_play && order.team_count && !services.team_name)))
    throw FormatError("Missing Vista PCR locale owner");
  if(order.player_count && !services.team_play && !services.live_identity)
    throw FormatError("Missing Vista PCR live-player lookup owner");
  out.players={};out.teams={};out.player_count=order.player_count;out.team_count=order.team_count;
  out.variant=finalized(b)?read(b,0x64):0;
  if(services.team_play)for(unsigned r=0;r<order.team_count;++r) {
    const auto t=order.teams[r];auto& row=out.teams[r];
    copy_text(row,0,32,services.team_name(t));
    const auto score=t<8?signed_word(b,team(t)+2):0;
    copy_text(row,0x40,16,services.score(score));
    const auto place=t<8?signed_byte(b,team(t)+1):0;
    write(row,0x60,std::uint32_t(place),2);write(row,0x62,t,2);
    copy_text(row,0x64,16,services.placement(place));
  }
  for(unsigned r=0;r<order.player_count;++r) {
    const auto p=order.players[r];auto& row=out.players[r];
    for(unsigned j=0;j<order.player_count;++j)
      write(row,0x90+j*4,read(b,0x44d0+2*((p*16+order.players[j])*2),2)&0x7fff);
    bool spree{},multikill{};unsigned distinct{};std::uint32_t total{},mask{};
    for(int medal=31;medal>=0;--medal) {
      const auto n=read(b,0xe8a+2*(p*0x1b5+unsigned(medal)),2)&0x7fff;
      if(!n)continue;
      bool show=true;
      if(medal>=13 && medal<=17){show=!spree;spree=true;}
      else if(medal<=5){show=!multikill;multikill=true;}
      if(show){++distinct;mask|=std::uint32_t(1)<<medal;}
      total+=n;
    }
    write(row,0x7c,total);write(row,0x80,mask);
    if(!services.team_play && services.live_identity) {
      if(const auto identity=services.live_identity(p)) {
        if(!services.selected_identity)throw FormatError("Missing Vista PCR selected identity owner");
        if(*identity==services.selected_identity()) {
          if(!services.medal_achievement)throw FormatError("Missing Vista PCR medal achievement owner");
          services.medal_achievement(distinct);
        }
      }
    }
    copy_text(row,0,32,player_name(b,p));
    copy_text(row,0x40,16,services.score(signed_word(b,player(p)+0x92)));
    constexpr std::array<unsigned,4> indices{7,9,8,11};
    for(unsigned i=0;i<4;++i)write(row,0x60+i*4,std::uint32_t(stat(b,p,indices[i])));
    const auto place=player_placement(b,p);write(row,0x70,std::uint32_t(place),2);
    const auto actual_team=signed_byte(b,player(p)+0x88);
    write(row,0x72,services.team_play?std::uint32_t(actual_team):0xffffffffu,2);
    write(row,0x74,actual_team==-1,1);write(row,0x78,0xffffffffu);
    std::copy_n(b.begin()+std::ptrdiff_t(player(p)+0x4c),16,row.begin()+0xd0);
    copy_text(row,0xe0,16,services.placement(place));
    std::copy_n(b.begin()+std::ptrdiff_t(player(p)+2),8,row.begin()+0x100);
    unsigned first{},second{};
    switch(out.variant) {
      case 2: {const auto divisor=std::max(stat(b,p,9)+1,1);write(row,0x108,std::uint32_t(stat(b,p,13)/divisor));second=12;break;}
      case 1:first=16;second=15;break;case 3:first=26;second=25;break;
      case 4:first=28;second=27;break;case 7:first=38;second=39;break;
      case 8:first=43;second=44;break;case 9:first=21;second=20;break;
      default:break;
    }
    if(first)write(row,0x108,std::uint32_t(stat(b,p,first)));
    if(second)write(row,0x10c,std::uint32_t(stat(b,p,second)));
    for(unsigned weapon=5;weapon<39;++weapon)for(unsigned category=4;category<=6;++category) {
      const auto at=0x84+(category-4)*4;
      const auto n=read(b,0xecc+2*(p*0x1b5+category+weapon*8),2)&0x7fff;
      write(row,at,read(row,at)+n);
    }
  }
}
} // namespace halo2
