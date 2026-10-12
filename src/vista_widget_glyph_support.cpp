#include "halo2/vista_widget_glyph_support.hpp"
#include "halo2/cache.hpp"
#include <utility>
namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a){if(!f)throw FormatError("Missing consumed Vista glyph support service");return f(std::forward<A>(a)...);}
struct Owner {
 const VistaGlyphSupportServices& s;std::uint32_t id{};std::span<std::byte> bank{};
 std::uint32_t read(std::uint32_t at,std::size_t n=4)const{if(id&&at>=id&&std::uint64_t(at)+n<=std::uint64_t(id)+bank.size()){std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(bank[at-id+i])<<(8*i);return v;}return call(s.read,at,n);}
 void write(std::uint32_t at,std::uint32_t v,std::size_t n=4)const{if(id&&at>=id&&std::uint64_t(at)+n<=std::uint64_t(id)+bank.size()){for(std::size_t i=0;i<n;++i)bank[at-id+i]=std::byte(v>>(8*i));}else call(s.write,at,v,n);}
};
std::uint32_t copy(std::uint32_t dst,std::uint32_t capacity,std::uint32_t input,const Owner& o){
 auto error=[&](std::uint32_t value){const auto at=call(o.s.errno_address);o.write(at,value);call(o.s.invalid_parameter,std::array<std::uint32_t,5>{});return value;};
 if(!dst||!capacity)return error(22);
 if(!input){o.write(dst,0,2);return error(22);}
 auto output=dst,source=input,remaining=capacity;
 for(;;){const auto unit=o.read(source,2);o.write(output,unit,2);output+=2;source+=2;if(!unit)break;if(!--remaining)break;}
 if(!remaining){o.write(dst,0,2);return error(34);}
 return 0;
}
} //namespace
std::uint8_t vista_glyph_color_classify(std::uint32_t c){
 if(c-0xe112u<=0x19u)return 0;
 if(c-0xe000u>0x3ffu)return 0;
 if(c<=0xe008u){if(c<=0xe002u||c==0xe004u||c==0xe008u)return 0;return 1;}
 const auto index=c-0xe106u;
 if(index>0x2bu)return 1;
 //Source498CB0, consumed through actual498CA0 target indices.
 constexpr std::array<std::uint8_t,44> table={0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0};
 return table[index];
}
std::uint32_t vista_glyph_copy_wide(std::uint32_t destination,std::uint32_t capacity,
 std::uint32_t input,const VistaGlyphSupportServices& s){return copy(destination,capacity,input,Owner{s});}
void vista_glyph_ellipsis(std::uint32_t destination,std::uint32_t id,
 std::span<std::byte,0x400> scratch,const VistaGlyphSupportServices& s){
 if(!id||std::uint64_t(id)+scratch.size()>0x100000000ull)throw FormatError("Invalid Vista ellipsis source bank");
 const Owner o{s,id,scratch};o.write(id,0,2);const auto context=call(s.context);
 if(context){const auto list=o.read(context+0x11c);call(s.localized,list,0x08000708u,id);}
 copy(destination,4,id,o);
}
} //namespace halo2
