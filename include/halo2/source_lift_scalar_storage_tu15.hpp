#pragma once
#include "halo2/source_lift_scalar.hpp"
namespace halo2 {
enum class SourceLiftScalarEpsilonUse { Exponent,Mass,Step,Ratio };
// Semantic storage phases of1DA530. Accessors remain lazy: source stores can
// alias arguments, and an observer can change a later operand. A host layout
// need not retain original addresses or registers to implement this contract.
class SourceLiftScalarStorageTu15 {
public:
  virtual ~SourceLiftScalarStorageTu15()=default;
  virtual void begin_local_scratch()=0;
  virtual float tested_exponent()const=0;
  virtual float epsilon(SourceLiftScalarEpsilonUse)const=0;
  virtual float target_for_power()const=0;
  virtual void power_base(float,bool negated)=0;
  virtual float power_base()const=0;
  virtual float exponent()const=0;
  virtual float acceleration()const=0;
  virtual void power_acceleration(float)=0;
  virtual float power_acceleration()const=0;
  virtual bool scale_by_mass()const=0;
  virtual float mass()const=0;
  virtual float seconds()=0;
  virtual float braking_seconds()const=0;
  virtual void step(float)=0;
  virtual float target_for_distance()const=0;
  virtual float position()const=0;
  virtual float velocity_limit()const=0;
  virtual float velocity()const=0;
  virtual void distance(float)=0;
  virtual bool force_acceleration()const=0;
  virtual float one()const=0;
  virtual float half()const=0;
  virtual float braking_step()const=0;
  virtual float braking_distance()const=0;
  virtual void braking_result(float)=0;
  virtual float braking_result()const=0;
  virtual bool allow_reverse()const=0;
  virtual float reverse_minimum()const=0;
  virtual float reverse_maximum()const=0;
  virtual void prepare_ratio_destination()=0;
  virtual void ratio(float)=0;
  virtual void finish_local_scratch(bool minimum_clamped)=0;
};
using SourceLiftAccelerationLoad=std::function<float()>;
using SourceLiftPowerLazyTu15=std::function<float(float,float,const SourceLiftAccelerationLoad&)>;
SourceLiftScalarOutcome source_lift_scalar_storage_tu15(SourceLiftScalarStorageTu15&,
    SourceLiftScalarEnvironment,const SourceLiftPowerLazyTu15&);
}
