#include "halo2/vista_widget_screen_disposal.hpp"
#include "halo2/cache.hpp"
#include <utility>
namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a){if(!f)throw FormatError("Vista screen disposal reached an unbound genuine service");return f(std::forward<A>(a)...);}
std::uint32_t read(const VistaWidgetScreenDisposalServices&s,std::uint32_t at,std::size_t n=4){return call(s.read,at,n);}
void write(const VistaWidgetScreenDisposalServices&s,std::uint32_t at,std::uint32_t value,std::size_t n=4){call(s.write,at,value,n);}
}
VistaWidgetIdentity vista_widget_manager_mapping(std::uint32_t index,std::uint32_t kind){
 switch(kind){
 case 0:return {index==4?0xd71e78u:0u};
 case 1:return {index*60u+0xd71bf8u};
 case 2:return {index==4?0xd71ebcu:0u};
 case 3:return {index*60u+0xd71accu};
 case 4:return {index*68u+0xd71d24u};
 case 5:return {index*68u+0xd71978u};
 case 6:return {0xd7190cu};
 default:return {0};
 }
}
void vista_widget_manager_unlink(VistaWidgetIdentity manager,VistaWidgetIdentity screen,
 const VistaWidgetScreenDisposalServices&s){
 for(const auto offset:{8u,0xcu,0x30u})if(read(s,manager.value+offset)==screen.value)write(s,manager.value+offset,0);
 for(std::uint32_t offset=0x34;offset<=0x38;offset+=4){const VistaWidgetIdentity child{read(s,manager.value+offset)};if(child&&(child==screen||call(s.root,child)==screen))write(s,manager.value+offset,0);}
 if(read(s,screen.value+0x9fa,1)){const auto table=read(s,screen.value);const auto target=read(s,table+8);call(s.cleanup,target,screen);}
 const auto table=read(s,screen.value);const auto target=read(s,table);call(s.destructor,target,screen,0u);call(s.release,screen);
}
void vista_widget_screen_dispose(VistaWidgetIdentity screen,const VistaWidgetScreenDisposalServices&s){
 auto table=read(s,screen.value);auto target=read(s,table+0x84);const auto index=call(s.screen_query,target,screen);
 table=read(s,screen.value);target=read(s,table+0x80);const auto kind=call(s.screen_query,target,screen);
 const auto manager=vista_widget_manager_mapping(index,kind);table=read(s,manager.value);target=read(s,table+0x20);call(s.manager_remove,target,manager,screen);
 bool present=false;
 for(const auto at:{0xd71914u,0xd71918u,0xd71e80u,0xd71e84u,0xd71ec4u,0xd71ec8u})if(read(s,at)){present=true;break;}
 if(!present){
  auto first=0xd71984u,second=0xd71ad8u;
  while(first<0xd71ad8u){
   if(read(s,first-4)||read(s,first)||read(s,second-4)||read(s,second)||read(s,first+0x3a8)||read(s,first+0x3ac)||read(s,second+0x128)||read(s,second+0x12c)){present=true;break;}
   first+=0x44;second+=0x3c;
  }
  if(!present)write(s,0xd718e4,0,1);
 }
 for(std::uint32_t offset=0;offset<0x8c;offset+=4)if(read(s,0xd71ef8+offset)==screen.value)write(s,0xd71ef8+offset,0);
}
} //namespace halo2
