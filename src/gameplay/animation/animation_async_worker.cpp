#include "halo2/animation_async_worker.hpp"
#include <bit>
#include <stdexcept>
#include <unordered_set>
namespace halo2 { namespace {
std::uint32_t load(const SourceAnimationResourceMemory &m,std::uint32_t at,unsigned n=4){
  if(!m.read)throw std::runtime_error("Original async raw storage owner unavailable");
  if(std::uint64_t(at)+n>0x100000000ull)throw std::out_of_range("Async raw span wraps");
  const auto b=m.read(at,n);if(b.size()<n)throw std::out_of_range("Async raw span truncated");
  std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(i*8);return v;
}
void put(const SourceAnimationResourceMemory &m,std::uint32_t at,std::uint32_t v,
         std::uint32_t pc,const SourceAnimationAsyncObserver &o,unsigned n=4){
  const auto before=load(m,at,n);if(!m.write)throw std::runtime_error("Mutable async raw storage owner unavailable");
  std::array<std::byte,4>b;for(unsigned i=0;i<n;++i)b[i]=std::byte((v>>(i*8))&255);m.write(at,std::span(b).first(n));
  if(o)o({pc,at,before,load(m,at,n),n});
}
void node(const SourceAnimationAsyncWorkerServices &s,std::uint32_t p){
  const auto base=s.bindings.queue.nodes;
  if(p<base||std::uint64_t(p)>=std::uint64_t(base)+150*64||(p-base)%64)
    throw std::out_of_range("Async node outside original150x64 owned bank");
  (void)load(s.memory,p+60);
}
void chain(const SourceAnimationAsyncWorkerServices&s,std::uint32_t head){
  std::unordered_set<std::uint32_t> seen;
  for(auto p=head;p;p=load(s.memory,p+60)){
    node(s,p);if(!seen.insert(p).second)throw std::invalid_argument("Malformed async raw list cycle");
  }
}
void wait(const SourceAnimationAsyncWorkerServices&s,std::uint32_t slot){
  const auto h=load(s.memory,slot);if(!s.wait)throw std::runtime_error("Actual async wait owner unavailable");s.wait(h,0xffffffffu);
}
void release(const SourceAnimationAsyncWorkerServices&s,std::uint32_t slot){
  const auto h=load(s.memory,slot);if(!s.release_mutex)throw std::runtime_error("Actual async mutex release owner unavailable");s.release_mutex(h);
}
void signal(const SourceAnimationAsyncWorkerServices&s){
  const auto h=load(s.memory,s.bindings.queue.semaphore_slot);
  if(!s.release_semaphore)throw std::runtime_error("Actual async semaphore release owner unavailable");s.release_semaphore(h,1);
}
bool before(const SourceAnimationAsyncWorkerServices&s,std::uint32_t incoming,std::uint32_t other,
            std::uint32_t &file,std::uint32_t &offset,bool &valid){
  const auto &m=s.memory;const auto read=s.bindings.read_callback;
  const auto ip=std::bit_cast<std::int32_t>(load(m,incoming));
  const auto op=std::bit_cast<std::int32_t>(load(m,other));
  const auto ic=load(m,incoming+52),oc=load(m,other+52);bool result=false;
  if(ip>op)result=true;
  else if(ip==op&&ic==read){
    if(oc!=read)result=true;
    else if(valid){
      const auto a=load(m,incoming+8)-file,b=load(m,other+8)-file;
      result=a<b||(a==b&&load(m,incoming+20)-offset<load(m,other+20)-offset);
    }
  }
  if(oc==read){file=load(m,other+8);offset=load(m,other+20);valid=true;}return result;
}
} // namespace
std::uint32_t lease_source_animation_async_node(const SourceAnimationAsyncWorkerServices&s,const SourceAnimationAsyncObserver&o){
  const auto &b=s.bindings.queue;
  for(;;){
    wait(s,b.free_mutex_slot);const auto p=load(s.memory,b.free_head);
    if(p){node(s,p);put(s.memory,b.free_head,load(s.memory,p+60),0x121fee,o);put(s.memory,p+60,0,0x121ff4,o);}
    release(s,b.free_mutex_slot);if(p)return p;
    if(!s.yield)throw std::runtime_error("Actual empty async free-list yield owner unavailable");s.yield();
  }
}
void insert_source_animation_async_node(std::uint32_t p,const SourceAnimationAsyncWorkerServices&s,const SourceAnimationAsyncObserver&o){
  node(s,p);const auto &m=s.memory;const auto head=load(m,s.bindings.queue.work_head);chain(s,head);
  for(auto q=head;q;q=load(m,q+60))if(q==p)throw std::invalid_argument("Async insert node is already queued");
  if(!head){put(m,s.bindings.queue.work_head,p,0x122040,o);put(m,p+60,0,0x122045,o);return;}
  if(std::bit_cast<std::int32_t>(load(m,p))>std::bit_cast<std::int32_t>(load(m,head))){
    put(m,p+60,head,0x12205a,o);put(m,s.bindings.queue.work_head,p,0x12205d,o);return;
  }
  auto previous=head;std::uint32_t file=0,offset=0;bool valid=false;
  while(const auto q=load(m,previous+60)){
    if(before(s,p,q,file,offset,valid)){
      put(m,p+60,load(m,previous+60),0x1220cc,o);put(m,previous+60,p,0x1220d0,o);return;
    }previous=q;
  }
  put(m,previous+60,p,0x1220b1,o);put(m,p+60,0,0x1220b5,o);
}
std::uint32_t publish_source_animation_async_node(std::uint32_t p,const SourceAnimationAsyncWorkerServices&s,const SourceAnimationAsyncObserver&o){
  const auto &m=s.memory;const auto &b=s.bindings;wait(s,b.queue.work_mutex_slot);insert_source_animation_async_node(p,s,o);
  const auto index=(p-b.queue.nodes)/64;put(m,p+4,index,0x122101,o);
  put(m,p+4,(load(m,b.counter)<<8)|index,0x12210f,o);put(m,b.counter,load(m,b.counter)+1,0x12211f,o);
  const auto id=load(m,p+4);release(s,b.queue.work_mutex_slot);signal(s);return id;
}
void unlink_source_animation_async_node(std::uint32_t p,const SourceAnimationAsyncWorkerServices&s,const SourceAnimationAsyncObserver&o){
  node(s,p);const auto&m=s.memory;const auto&b=s.bindings.queue;wait(s,b.work_mutex_slot);
  auto q=load(m,b.work_head);chain(s,q);
  if(q==p){put(m,b.work_head,load(m,p+60),0x122169,o);put(m,p+4,0xffffffffu,0x12216f,o);}
  else{
    while(q&&load(m,q+60)!=p)q=load(m,q+60);
    if(!q)throw std::invalid_argument("Async unlink node is absent from work list");
    put(m,q+60,load(m,p+60),0x122192,o);put(m,p+4,0xffffffffu,0x122195,o);
  }release(s,b.work_mutex_slot);
}
std::uint32_t submit_source_animation_async_read(const SourceBitmapReadRequest&r,
    const SourceBitmapFileBank&files,const SourceAnimationAsyncWorkerServices&s,const SourceAnimationAsyncObserver&o){
  const std::uint32_t tokens[]{files.current,files.mainmenu,files.shared,files.single_player_shared};
  SourceAsyncTask task;task.words[0]=tokens[r.location>>30];task.words[1]=r.buffer;
  task.words[2]=r.size;task.words[3]=r.location&0x3fffffffu;task.words[4]=r.bytes_output;
  put(s.memory,s.bindings.requests_added,load(s.memory,s.bindings.requests_added)+1,0x1a48bf,o);
  put(s.memory,r.done,0,0x1a48cd,o,1);const auto p=lease_source_animation_async_node(s,o);
  for(unsigned i=0;i<10;++i)put(s.memory,p+8+i*4,task.words[i],0x1a48e5,o);
  put(s.memory,p+56,r.done,0x1a48eb,o);put(s.memory,p,std::bit_cast<std::uint32_t>(r.priority),0x1a48ee,o);
  put(s.memory,p+52,s.bindings.read_callback,0x1a48f0,o);put(s.memory,p+48,r.category,0x1a48f7,o);
  return publish_source_animation_async_node(p,s,o);
}
[[noreturn]] void run_source_animation_async_worker(const SourceAnimationAsyncWorkerServices&s,const SourceAnimationAsyncObserver&o){
  const auto&m=s.memory;const auto&b=s.bindings.queue;
  for(;;){
    wait(s,b.semaphore_slot);wait(s,b.work_mutex_slot);const auto p=load(m,b.work_head);release(s,b.work_mutex_slot);
    put(m,s.bindings.current_node,p,0x122393,o);
    node(s,p);const auto callback=load(m,p+52);if(!s.callback)throw std::runtime_error("Actual async callback owner unavailable");
    const auto result=s.callback(callback,p+8);
    if(result){
      if(result==1){const auto done=load(m,p+56);if(done)put(m,done,1,0x1223ac,o,1);}
      unlink_source_animation_async_node(p,s,o);wait(s,b.free_mutex_slot);
      put(m,p+60,load(m,b.free_head),0x1223c8,o);put(m,b.free_head,p,0x1223d1,o);release(s,b.free_mutex_slot);
    }else signal(s);
  }
}
SourceAsyncReadExecution read_source_animation_async_file(std::uint32_t payload,std::uint32_t limit,
    const SourceAsyncOwnedBuffer&buffer,SourceAsyncOwnedFiles&files,const SourceAnimationResourceMemory&m,const SourceAnimationAsyncObserver&o,const SourceAnimationAsyncFileObserver&io){
  if(std::uint64_t(payload)+40>0x100000000ull)throw std::out_of_range("Async task payload wraps");
  SourceAsyncTask task;for(unsigned i=0;i<10;++i)task.words[i]=load(m,payload+i*4);
  const auto step=source_async_read_begin(task,limit);
  if(step.size&&std::uint64_t(step.buffer)<std::uint64_t(payload)+40&&
      std::uint64_t(step.buffer)+step.size>payload)
    throw std::invalid_argument("Owned-file adapter does not represent task/IO-buffer aliasing");
  auto out=files.read(task,limit,buffer);if(io)io(out);
  if(out.io.seek_low!=0xffffffffu){
    put(m,payload+20,task.words[5],0x1a483e,o);put(m,payload+12,task.words[3],0x1a4841,o);
    if(task.words[4])put(m,task.words[4],task.words[5],0x1a4846,o);
  }return out;
}
} // namespace halo2
