#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/source_native_publication.hpp"
#include <thread>
#include <exception>

namespace halo2 {
namespace {
void require(bool value,const char *why){if(!value)throw FormatError(why);}
}
struct MccSoundVariationProvider::NativeBindingData {
    const std::thread::id executor=std::this_thread::get_id();
    const MccSoundVariationProvider provider;
    const std::shared_ptr<const Cache> cache;
    const MccSoundTimingReadLease lease;
    const std::weak_ptr<SourceNativePublication::State> native;
    NativeBindingData(MccSoundVariationProvider provider,std::shared_ptr<const Cache> cache,
        const std::shared_ptr<SourceNativePublication::State> &native)
        :provider(std::move(provider)),cache(std::move(cache)),lease(this->provider.timing_lease()),native(native){}
};
struct MccSoundVariationProvider::NativeHeldData {
    const std::thread::id executor=std::this_thread::get_id();
    const std::shared_ptr<const NativeBindingData> binding;
    const std::shared_ptr<SourceNativePublication::State> native;
    MccSoundTimingHeldRead timing;
    static std::shared_ptr<SourceNativePublication::State> admit(
        const std::shared_ptr<const NativeBindingData> &binding) {
        require(binding && binding->executor==std::this_thread::get_id(),
            "Native metadata hold requires its creator target binding");
        auto state=binding->native.lock();
        SourceNativePublication::require_provider(state,binding->provider,binding->cache);
        return state;
    }
    explicit NativeHeldData(std::shared_ptr<const NativeBindingData> binding)
        :binding(std::move(binding)),native(admit(this->binding)),
         timing(hold_mcc_sound_timing_read_lease(this->binding->lease)){}
    ~NativeHeldData(){if(executor!=std::this_thread::get_id())std::terminate();}
    // Reverse member destruction releases timing, native, then metadata/Cache.
};
MccSoundVariationProvider::NativeBinding::NativeBinding(std::shared_ptr<const NativeBindingData> data)
    :data_(std::move(data)){}
bool MccSoundVariationProvider::NativeBinding::current()const noexcept {
    if(!data_ || data_->executor!=std::this_thread::get_id())return false;
    return SourceNativePublication::retained_current(data_->native.lock());
}
MccSoundVariationProvider::NativeBinding MccSoundVariationProvider::native_binding(
    const SourceNativePublication &native,std::shared_ptr<const Cache> cache)const {
    // Complete provider/native pairing admission before allocating a
    // transferable value; the value retains no live native exclusion.
    SourceNativePublication::require_provider(native.state_,*this,cache);
    return NativeBinding(std::make_shared<const NativeBindingData>(*this,std::move(cache),native.state_));
}
MccSoundVariationProvider::HeldNativeBinding::HeldNativeBinding(std::unique_ptr<NativeHeldData> data)
    :data_(std::move(data)){}
MccSoundVariationProvider::HeldNativeBinding::~HeldNativeBinding(){
    if(data_ && data_->executor!=std::this_thread::get_id())std::terminate();
}
bool MccSoundVariationProvider::HeldNativeBinding::matches(const NativeBinding &binding)const noexcept {
    if(!data_ || data_->executor!=std::this_thread::get_id() || !binding.data_ || data_->binding!=binding.data_)return false;
    return SourceNativePublication::retained_current(data_->native) &&
        data_->timing.matches(data_->binding->lease);
}
MccSoundVariationProvider::HeldNativeBinding MccSoundVariationProvider::hold_native_binding(const NativeBinding &binding) {
    return HeldNativeBinding(std::make_unique<NativeHeldData>(binding.data_));
}
MccSoundVariationProvider MccSoundVariationProvider::binding_provider(const NativeBinding &binding) {
    require(binding.current(),"Native metadata replacement requires its current target binding");
    binding.data_->provider.require_current(*binding.data_->cache);
    return binding.data_->provider;
}
} // namespace halo2
