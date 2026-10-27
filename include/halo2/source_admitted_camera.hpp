#pragma once
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/peer_source_admissions.hpp"
#include "halo2/source_original_camera_seed.hpp"
#include <thread>

namespace halo2 {
class MultiplayerMatch;
// A locally constructed original camera owner for one admitted physical PC.
// Numeric updates never install a received controller/observer bank or callback.
// This fresh owner requires the genuine tick0 source world. It does not repair
// missing camera history, certify a host cut or enable network replay.
class SourceAdmittedCamera {
public:
  SourceAdmittedCamera(MultiplayerMatch &,MultiplayerObjectRuntime &,const OriginalData &,
      const AcceptedSourcePlayers &,std::span<const OnlineAdmission>,unsigned physical_owner);
  std::optional<NativeDeathCameraView> update(const AuthorityTransferContext &,unsigned view,
      float seconds,bool cycle,std::optional<NativeDeathCameraView> *completed=nullptr);
  // Read-only native result, useful for byte comparison against the producer.
  // No incoming camera checkpoint can be installed through this interface.
  NativeVehicleCamera::Checkpoint checkpoint()const;
  bool death_view(unsigned view)const{return camera_.death_view(view);}
  unsigned owner()const noexcept{return owner_;}
  std::span<const SourcePlayerAdmission> views()const noexcept{return views_;}
private:
  friend class SourceAdmittedCameraBank;
  MultiplayerMatch &match_;
  MultiplayerObjectRuntime &world_;
  AuthorityTransferContext context_;
  unsigned owner_{};
  std::vector<SourcePlayerAdmission> views_;
  SourceOriginalCameraSeed seed_;
  NativeVehicleCamera camera_;
  std::uint16_t flags(const SourcePlayerAdmission &)const;
};
// This bank is installed from independently admitted source players on the
// native executor before any received numeric camera operation may execute.
// It owns one original camera instance for each physical couch; received values
// can select only an already admitted numeric owner/view and elapsed input.
class SourceAdmittedCameraBank {
public:
  SourceAdmittedCameraBank(MultiplayerMatch &,MultiplayerObjectRuntime &,const OriginalData &,
      const AcceptedSourcePlayers &,std::span<const OnlineAdmission>);
  void activate();
  bool active()const noexcept{return active_;}
  const AuthorityTransferContext &context()const noexcept{return context_;}
  MultiplayerObjectRuntime &world()const noexcept{return world_;}
  void validate(unsigned owner,unsigned view,float seconds)const;
  std::optional<NativeDeathCameraView> update(unsigned owner,unsigned view,float seconds,bool cycle,
      std::optional<NativeDeathCameraView> *completed=nullptr);
  bool death_view(unsigned owner,unsigned view)const;
  NativeVehicleCamera::Checkpoint checkpoint(unsigned owner)const;
  std::span<const SourcePlayerAdmission> views(unsigned owner)const;
  class Binding {
  public:
    explicit Binding(SourceAdmittedCameraBank &);
    ~Binding();
    Binding(const Binding &)=delete;
    Binding &operator=(const Binding &)=delete;
  private:
    SourceAdmittedCameraBank *bank_;
  };
  static SourceAdmittedCameraBank &bound(MultiplayerObjectRuntime &);
private:
  const std::thread::id executor_=std::this_thread::get_id();
  MultiplayerObjectRuntime &world_;
  AuthorityTransferContext context_;
  std::array<std::unique_ptr<SourceAdmittedCamera>,16> cameras_;
  bool active_{};
  void own()const;
};
}
