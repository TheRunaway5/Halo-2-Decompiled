#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <xmmintrin.h>

namespace halo2::native_raw_x87_detail {
// Private ownership seam for raw native arithmetic. State's logical stack is
// dense; holes, MMX, unmasked exceptions and overflowing arithmetic need another
// owner. Callers must use explicit native asm, with no compiler-owned x87
// temporaries spanning this scope. FIP/FDP/opcode are outside State's ABI.
struct Image {
    std::array<std::byte,108> x87{};
    std::uint32_t mxcsr{};
    std::uint16_t word(unsigned offset) const noexcept {
        std::uint16_t value{};std::memcpy(&value,x87.data()+offset,2);return value;
    }
    void word(unsigned offset,std::uint16_t value) noexcept {std::memcpy(x87.data()+offset,&value,2);}
    unsigned top() const noexcept {return (word(4)>>11u)&7u;}
    unsigned tag(unsigned logical) const noexcept {return (word(8)>>(((top()+logical)&7u)*2u))&3u;}
};
inline Image capture() noexcept {
    Image image;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnsave %0; frstor %0":"=m"(image.x87)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    image.mxcsr=_mm_getcsr();
#endif
    return image;
}
inline void restore(const Image&image) noexcept {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // Discard a hostile provider's pending exception before restoring ownership.
    asm volatile("fninit; frstor %0"::"m"(image.x87):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    _mm_setcsr(image.mxcsr);
#endif
}
inline unsigned value_tag(const std::byte*value) noexcept {
    std::uint64_t significand{};std::uint16_t exponent{};
    std::memcpy(&significand,value,8);std::memcpy(&exponent,value+8,2);exponent&=0x7fffu;
    if(!exponent&&!significand)return 1;
    if(!exponent||exponent==0x7fffu||!(significand>>63u))return 2;
    return 0;
}
inline bool controls_supported(const Image&image) noexcept {
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    return (image.word(0)&0x3fu)==0x3fu&&(image.word(4)&0x8080u)==0&&
        (image.mxcsr&0x1f80u)==0x1f80u&&(image.mxcsr&0xffff0000u)==0;
#endif
}
inline bool dense(const Image&image,unsigned&count) noexcept {
    count=0;while(count<8&&image.tag(count)!=3)++count;
    for(unsigned n=count;n<8;++n)if(image.tag(n)!=3)return false;
    for(unsigned n=0;n<count;++n)if(image.tag(n)!=value_tag(image.x87.data()+28+n*10))return false;
    return controls_supported(image);
}
inline bool equivalent(const Image&a,const Image&b) noexcept {
    // Instruction/data pointers describe our native host instructions, not the
    // virtual source PC. Ignore these and architecturally unused payloads.
    if(a.word(0)!=b.word(0)||a.word(4)!=b.word(4)||a.word(8)!=b.word(8)||a.mxcsr!=b.mxcsr)return false;
    for(unsigned n=0;n<8;++n)if(a.tag(n)!=3&&std::memcmp(a.x87.data()+28+n*10,b.x87.data()+28+n*10,10))return false;
    return true;
}
template<class State>bool supported(const State&state) noexcept {
    if(state.fp.count>8||state.fp.top>7)return false;
    if constexpr(requires{state.active_mmx;})if(state.active_mmx)return false;
    auto image=capture();image.word(4,state.fp.word());return controls_supported(image);
}
template<class State>class Scope {
    State&state_;
    Image host_;
    bool live_{true};
    std::uint64_t generation_{};
    static void require(bool condition,const char*why) {if(!condition)throw std::runtime_error(why);}
    Image imported() const {
        require(supported(state_),"Raw x87 continuation is outside dense masked ownership");
        auto image=capture();image.word(4,state_.fp.word());
        std::uint16_t tags=0xffffu;
        for(unsigned n=0;n<state_.fp.count;++n){
            std::memcpy(image.x87.data()+28+n*10,&state_.fp.lanes[n],10);
            const auto shift=((state_.fp.top+n)&7u)*2u;
            tags=std::uint16_t((tags&~(3u<<shift))|(value_tag(image.x87.data()+28+n*10)<<shift));
        }
        image.word(8,tags);return image;
    }
    void publish(const Image&image) noexcept {
        unsigned count{};const bool valid=dense(image,count);
        // Every arithmetic step belongs to the admitted, bounded numerical
        // body. Invalid topology is a programming/domain error, never silently
        // normalized into State's dense ABI.
        if(!valid)std::terminate();
        state_.fp.count=count;state_.fp.top=image.top();state_.fp.status=image.word(4);
        for(unsigned n=0;n<count;++n)std::memcpy(&state_.fp.lanes[n],image.x87.data()+28+n*10,10);
    }
    void park(const Image&image) noexcept {
        publish(image);auto empty=image;empty.word(8,0xffffu);empty.word(4,std::uint16_t(image.word(4)&~0x3800u));restore(empty);
    }
    void return_host(const Image&source) noexcept {
        auto returned=host_;returned.word(0,source.word(0));
        returned.word(4,std::uint16_t((source.word(4)&~0x3800u)|(host_.word(4)&0x3800u)));
        returned.mxcsr=source.mxcsr;restore(returned);
    }
    void adopt_child() {
        Image image;
        try {image=imported();}
        catch(...) {
            // An unsupported actual child outcome cannot be retried after its
            // side effects. Preserve its retained State for the outer owner,
            // release our host registers, and report the domain failure.
            live_=false;return_host(capture());throw;
        }
        restore(image);publish(image);++generation_;
    }
public:
    // Restoring entry controls here would lose newly raised source exceptions
    // when State.save subsequently samples the host. Keep source CW/SW/MXCSR,
    // restoring only the caller's register file, tags and coherent host TOP.
    explicit Scope(State&state):state_(state),host_(capture()) {
        require(supported(state_),"Raw x87 input is outside dense masked ownership");
        restore(imported());
    }
    Scope(const Scope&)=delete;Scope&operator=(const Scope&)=delete;
    ~Scope() noexcept {
        if(!live_)return;
        const auto source=capture();publish(source);
        return_host(source);
    }
    void publish() noexcept {publish(capture());}
    void require_capacity(unsigned extra) const {
        unsigned count{};require(dense(capture(),count)&&extra<=8-count,"Raw x87 numerical step exceeds dense capacity");
    }
    template<class Callback>void io(Callback&&callback) {
        static_assert(std::is_void_v<std::invoke_result_t<Callback>>,"FP callbacks return through ordinary captured storage");
        const auto source=capture();park(source);
        try {std::forward<Callback>(callback)();}
        catch(...) {restore(source);publish(source);throw;}
        restore(source);publish(source);
    }
    template<class Callback>void child(Callback&&callback) {
        static_assert(std::is_void_v<std::invoke_result_t<Callback>>,"FP children return through their retained State ABI");
        const auto source=capture();park(source);
        try {std::forward<Callback>(callback)();}
        catch(...) {
            const auto original=std::current_exception();
            try {adopt_child();}
            catch(...) {
                // Unsupported throwing children keep their actual retained
                // outcome and original exception. adopt_child released our
                // host registers; native resume is explicitly unavailable.
                std::rethrow_exception(original);
            }
            std::rethrow_exception(original);
        }
        adopt_child();
    }
    class SingleSpill {
        friend class Scope;
        const Scope*owner_{};std::uint64_t generation_{};Image before_,after_;
        SingleSpill(const Scope*owner,std::uint64_t generation,const Image&before,const Image&after,std::uint32_t bits)
            :owner_(owner),generation_(generation),before_(before),after_(after),bits(bits){}
    public:
        const std::uint32_t bits;
        SingleSpill(const SingleSpill&)=default;
    };
    // A virtual spill has three distinct ownership phases:
    //   auto pending=scope.stage_single(pop);
    //   scope.io([&]{ provider.write(pending.bits); });
    //   scope.commit(pending);
    //   scope.io([&]{ completed_observer(); });
    // Conversion remains speculative until provider success. An observer failure
    // after commit retains both the completed store and its post-conversion FP.
    SingleSpill stage_single(bool pop) {
        const auto before=capture();unsigned count{};
        require(dense(before,count)&&count,"Raw x87 spill requires a live ST0");
        std::uint32_t bits{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(pop)asm volatile("fstps %0":"=m"(bits)::"memory","st");
        else asm volatile("fsts %0":"=m"(bits)::"memory");
#endif
        const auto after=capture();restore(before);publish(before);
        return SingleSpill(this,generation_,before,after,bits);
    }
    void commit(const SingleSpill&spill) {
        require(spill.owner_==this&&spill.generation_==generation_&&equivalent(capture(),spill.before_),"Stale or foreign raw x87 spill token");
        restore(spill.after_);publish(spill.after_);++generation_;
    }
};
}
