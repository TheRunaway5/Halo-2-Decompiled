#pragma once
#include "halo2/vista_widget_registration.hpp"
namespace halo2 {
struct VistaWidgetScreenDisposalServices {
 std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
 std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
 std::function<std::uint32_t(std::uint32_t,VistaWidgetIdentity)> screen_query; //Fresh virtual84, then80 target.
 std::function<void(std::uint32_t,VistaWidgetIdentity,VistaWidgetIdentity)> manager_remove; //Actual virtual20 target,this,screen.
 std::function<VistaWidgetIdentity(VistaWidgetIdentity)> root; //Complete611BA1, separately ported.
 std::function<void(std::uint32_t,VistaWidgetIdentity)> cleanup; //Screen virtual8 when+9FA!=0.
 std::function<void(std::uint32_t,VistaWidgetIdentity,std::uint32_t)> destructor; //Fresh virtual0, argument0.
 std::function<void(VistaWidgetIdentity)> release; //Actual60D2EA allocator owner.
};
//Complete609957. ECX=index, EDX=kind, original32-bit arithmetic/literal bank.
//No guessed controller/index bounds or native pointer narrowing are supplied.
VistaWidgetIdentity vista_widget_manager_mapping(std::uint32_t index,std::uint32_t kind);
//Complete637D9B. Genuine root/destructor/allocator services remain mandatory.
//All writes/reloads retain source order, including callback mutation/reentry.
void vista_widget_manager_unlink(VistaWidgetIdentity manager,VistaWidgetIdentity screen,
 const VistaWidgetScreenDisposalServices&);
//Complete60B9AC..60BAAA. NULL/invalid mappings reach the explicit read provider;
//no manager success or whole-root reset is synthesized.
void vista_widget_screen_dispose(VistaWidgetIdentity screen,
 const VistaWidgetScreenDisposalServices&);
} //namespace halo2
