#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct SearchContext;
struct SearchSnapshot;
struct SearchAccount;
struct SearchUi;
struct SearchVariant;
struct SearchTake;
struct SearchCounts;
struct SearchFailure;
// Actual native resources, cached presentation facts and UI effects only.
// Every Rust state guard ends before one of these callbacks is invoked.
class NativeFrontendSearchIo {
public:
 virtual ~NativeFrontendSearchIo()=default;
 virtual SearchContext search_context() const=0;
 virtual SearchSnapshot search_view() const=0;
 virtual bool search_identity() const=0;
 virtual rust::Vec<std::uint8_t> search_profile() const=0;
 virtual std::uint32_t search_local_count() const=0;
 virtual bool search_pull_after(std::uint64_t) const=0;
 virtual void search_pull() const=0;
 virtual SearchAccount search_account() const=0;
 virtual bool search_command(rust::Str,rust::Str,rust::Str) const=0;
 virtual void search_sign_out() const=0;
 virtual void search_refresh(rust::Slice<const std::uint8_t>) const=0;
 virtual void search_cancel() const=0;
 virtual void search_join(rust::Str) const=0;
 virtual bool search_lobby_countdown() const=0;
 virtual bool search_launch() const=0;
 virtual bool search_has_launch() const=0;
 virtual void search_ui(const SearchUi&) const=0;
 virtual SearchVariant search_variant() const=0;
 virtual rust::Vec<std::uint8_t> search_map() const=0;
 virtual void search_prepare_map(rust::Slice<const std::uint8_t>) const=0;
 virtual bool search_self_present() const=0;
 virtual SearchTake search_take() const=0;
 virtual SearchCounts search_local_counts() const=0;
 virtual void search_set_local_count(std::uint32_t) const=0;
 virtual rust::Vec<std::uint8_t> search_admitted_name(std::uint32_t) const=0;
 virtual rust::Vec<std::uint8_t> search_profile_name(std::uint32_t) const=0;
 virtual void search_append_profile(std::uint32_t,bool) const=0;
 virtual bool search_validate_map() const=0;
 virtual void search_reject_content() const=0;
 virtual void search_publish() const=0;
 virtual void search_discard() const=0;
 virtual SearchFailure search_failure() const=0;
};
}
