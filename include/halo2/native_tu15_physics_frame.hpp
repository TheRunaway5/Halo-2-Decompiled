#pragma once
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <functional>

namespace halo2 {
struct NativeTu15PhysicsWorld { std::uint32_t identity{}; };
struct NativeTu15PhysicsBody { std::uint32_t identity{}; };
struct NativeTu15PhysicsManager { std::uint32_t identity{}; };
struct NativeTu15PhysicsObject { std::uint32_t datum{}; };
struct NativeTu15PhysicsProperties { std::uint32_t identity{}; };
struct NativeTu15PhysicsComponent { std::uint32_t identity{}; };
struct NativeTu15PhysicsDeferredBodies { std::uint32_t identity{}; };
struct NativeTu15PhysicsFrameFp {
  std::uint16_t control_word=0x027f,status{};
  std::uint32_t mxcsr=0x1f80;
};
struct NativeTu15PhysicsStep { std::uint32_t seconds_bits{},inverse_seconds_bits{}; };
enum class NativeTu15PhysicsIslandSelection : std::uint32_t { Active=0,Inactive=1,Both=2 };
struct NativeTu15PhysicsBodyIterator {
  NativeTu15PhysicsWorld world;
  NativeTu15PhysicsIslandSelection selection=NativeTu15PhysicsIslandSelection::Active;
  std::int32_t island=-1,body=-1;
  bool inactive{};
};
enum class NativeTu15PhysicsReleasePhase { Frame,Drain };

// Typed access to retained physics storage. Identities are data references,
// not instruction addresses. No register file, flags interpreter or code image
// belongs to this algorithm. A future host layout can implement this interface
// without preserving the original byte layout.
class NativeTu15PhysicsFrameStorage {
public:
  virtual ~NativeTu15PhysicsFrameStorage()=default;
  virtual NativeTu15PhysicsWorld world()const=0;
  virtual NativeTu15PhysicsManager manager()const=0;
  virtual std::uint32_t seconds_bits()const=0;
  virtual std::uint32_t one_bits()const=0;
  virtual std::uint32_t debug_arena()const=0;
  virtual void clear_allocation_failure()=0;
  virtual void stepping(NativeTu15PhysicsManager,bool)=0;
  virtual std::uint16_t iteration_depth()const=0;
  virtual void iteration_depth(std::uint16_t,bool entering)=0;
  virtual std::int32_t island_count(NativeTu15PhysicsWorld,bool inactive)const=0;
  virtual std::int32_t body_count(const NativeTu15PhysicsBodyIterator&)const=0;
  virtual NativeTu15PhysicsBody body(const NativeTu15PhysicsBodyIterator&)const=0;
  virtual std::int32_t property_count(NativeTu15PhysicsBody)const=0;
  virtual NativeTu15PhysicsProperties properties(NativeTu15PhysicsBody)const=0;
  virtual std::uint32_t property_key(NativeTu15PhysicsProperties,std::uint32_t)const=0;
  virtual std::uint32_t property_value(NativeTu15PhysicsProperties,std::uint32_t)const=0;
  virtual NativeTu15PhysicsComponent component(std::uint16_t)const=0;
  virtual bool component_publishes(NativeTu15PhysicsComponent)const=0;
  virtual NativeTu15PhysicsObject component_object(NativeTu15PhysicsComponent)const=0;
  virtual bool active(NativeTu15PhysicsBody)const=0;
  virtual std::uint16_t references(NativeTu15PhysicsBody)const=0;
  virtual void references(NativeTu15PhysicsBody,std::uint16_t,NativeTu15PhysicsReleasePhase,bool protected_reset)=0;
  virtual bool protected_reference(NativeTu15PhysicsBody)const=0;
  virtual std::int32_t deferred_cursor(NativeTu15PhysicsManager)const=0;
  virtual std::int32_t deferred_count(NativeTu15PhysicsManager)const=0;
  virtual NativeTu15PhysicsDeferredBodies deferred_bodies(NativeTu15PhysicsManager)const=0;
  virtual NativeTu15PhysicsBody deferred_body(NativeTu15PhysicsDeferredBodies,std::int32_t)const=0;
  virtual void deferred_cursor(NativeTu15PhysicsManager,std::int32_t,NativeTu15PhysicsReleasePhase,bool reset)=0;
  virtual void clear_deferred_count(NativeTu15PhysicsManager)=0;
};

// Residual modules are explicit dependencies during migration. None supplies
// a guessed success value. An omitted owner throws at its actual call phase,
// preserving preceding native stores. These callbacks do not certify that
// their implementations have themselves been converted to native kernels.
struct NativeTu15PhysicsFrameServices {
  std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> resize_debug_arena;
  std::function<void(NativeTu15PhysicsFrameFp&)> reset_debug_arena;
  std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> publish_debug_arena;
  std::function<void(NativeTu15PhysicsFrameFp&)> drain_commands;
  std::function<void(NativeTu15PhysicsWorld,const NativeTu15PhysicsStep&,NativeTu15PhysicsFrameFp&)> step_world;
  std::function<void(NativeTu15PhysicsFrameFp&)> post_step;
  std::function<void(NativeTu15PhysicsFrameFp&)> update_contacts;
  std::function<void(NativeTu15PhysicsObject,NativeTu15PhysicsFrameFp&)> publish_object;
  std::function<void(NativeTu15PhysicsBody,bool,NativeTu15PhysicsFrameFp&)> destroy_body;
};
bool native_tu15_next_physics_body(NativeTu15PhysicsFrameStorage&,NativeTu15PhysicsBodyIterator&);
void native_tu15_publish_physics_body(NativeTu15PhysicsFrameStorage&,NativeTu15PhysicsBody,
  NativeTu15PhysicsFrameFp&,const NativeTu15PhysicsFrameServices&);
void native_tu15_retire_physics_references(NativeTu15PhysicsFrameStorage&,NativeTu15PhysicsManager,
  NativeTu15PhysicsFrameFp&,const NativeTu15PhysicsFrameServices&);
// Actual TU1.5 1CC9CC..1CCACA tail, also used by the complete frame below.
void native_tu15_finish_physics_frame(NativeTu15PhysicsFrameStorage&,
  NativeTu15PhysicsFrameFp&,const NativeTu15PhysicsFrameServices&);
// Proper decompilation of English TU1.5 1CC8D0. Does not execute its lifted root.
void native_tu15_step_physics_frame(NativeTu15PhysicsFrameStorage&,
  NativeTu15PhysicsFrameFp&,const NativeTu15PhysicsFrameServices&);

// Byte-layout binding for the existing native retained bank. All executable
// policy remains in the typed algorithm; source PCs label optional write
// diagnostics only. This binding never decodes or executes Xbox instructions.
class NativeTu15PhysicsFrameMemory final:public NativeTu15PhysicsFrameStorage {
  SourcePhysicsContactMemory &memory_;
  SourcePhysicsContactRecordObserver observer_;
  std::uint32_t read(std::uint32_t,unsigned=4)const;
  void write(std::uint32_t,std::uint32_t,unsigned,std::uint32_t);
  std::uint32_t island(const NativeTu15PhysicsBodyIterator&)const;
public:
  explicit NativeTu15PhysicsFrameMemory(SourcePhysicsContactMemory&,SourcePhysicsContactRecordObserver={});
  NativeTu15PhysicsWorld world()const override;
  NativeTu15PhysicsManager manager()const override;
  std::uint32_t seconds_bits()const override;
  std::uint32_t one_bits()const override;
  std::uint32_t debug_arena()const override;
  void clear_allocation_failure()override;
  void stepping(NativeTu15PhysicsManager,bool)override;
  std::uint16_t iteration_depth()const override;
  void iteration_depth(std::uint16_t,bool)override;
  std::int32_t island_count(NativeTu15PhysicsWorld,bool)const override;
  std::int32_t body_count(const NativeTu15PhysicsBodyIterator&)const override;
  NativeTu15PhysicsBody body(const NativeTu15PhysicsBodyIterator&)const override;
  std::int32_t property_count(NativeTu15PhysicsBody)const override;
  NativeTu15PhysicsProperties properties(NativeTu15PhysicsBody)const override;
  std::uint32_t property_key(NativeTu15PhysicsProperties,std::uint32_t)const override;
  std::uint32_t property_value(NativeTu15PhysicsProperties,std::uint32_t)const override;
  NativeTu15PhysicsComponent component(std::uint16_t)const override;
  bool component_publishes(NativeTu15PhysicsComponent)const override;
  NativeTu15PhysicsObject component_object(NativeTu15PhysicsComponent)const override;
  bool active(NativeTu15PhysicsBody)const override;
  std::uint16_t references(NativeTu15PhysicsBody)const override;
  void references(NativeTu15PhysicsBody,std::uint16_t,NativeTu15PhysicsReleasePhase,bool)override;
  bool protected_reference(NativeTu15PhysicsBody)const override;
  std::int32_t deferred_cursor(NativeTu15PhysicsManager)const override;
  std::int32_t deferred_count(NativeTu15PhysicsManager)const override;
  NativeTu15PhysicsDeferredBodies deferred_bodies(NativeTu15PhysicsManager)const override;
  NativeTu15PhysicsBody deferred_body(NativeTu15PhysicsDeferredBodies,std::int32_t)const override;
  void deferred_cursor(NativeTu15PhysicsManager,std::int32_t,NativeTu15PhysicsReleasePhase,bool)override;
  void clear_deferred_count(NativeTu15PhysicsManager)override;
};
} // namespace halo2
