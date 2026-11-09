#pragma once

#include "halo2/physics_contact_record.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace halo2 {

// Bind the native CPU bitmap owner to one admitted effect-lighting call.
// D563B is a pointer read, not a texture allocation or a color result.
class NativeD5610BitmapRowBindingTu15 {
public:
    using Project=std::function<std::uint32_t(std::uint32_t bitmap,
        std::uint32_t root,std::uint32_t raw)>;

    NativeD5610BitmapRowBindingTu15(const SourcePhysicsContactMemory &memory,Project project)
        :memory_(memory),project_(std::move(project)),previous_(active_) {
        if(!project_)throw std::invalid_argument("D5610 requires its native CPU bitmap provider");
        active_=this;
    }
    ~NativeD5610BitmapRowBindingTu15(){active_=previous_;}
    NativeD5610BitmapRowBindingTu15(const NativeD5610BitmapRowBindingTu15&)=delete;
    NativeD5610BitmapRowBindingTu15& operator=(const NativeD5610BitmapRowBindingTu15&)=delete;

    static std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t bitmap,
        std::uint32_t root,std::uint32_t raw,std::uint32_t callee_sp) {
        for(auto binding=active_;binding;binding=binding->previous_) {
            if(&binding->memory_!=&memory)continue;
            if(!root || !raw || bitmap==0xffffffffu || callee_sp>0xffffff78u)
                throw std::runtime_error("D5610 lost its original full bitmap input");
            // D5610 subtracts 0x7C and pushes EBX and ESI before D563B.
            std::array<std::byte,4> bytes{};
            memory.read(callee_sp+0x84u,bytes);
            std::uint32_t returned{};
            for(unsigned i=0;i<bytes.size();++i)
                returned|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);
            if(returned!=0xd45aau && returned!=0xd4ec4u)
                throw std::runtime_error("D5610 bitmap projection requires its authenticated D4350 call");
            const auto row=binding->project_(bitmap,root,raw);
            if(!row || row==raw)
                throw std::runtime_error("D5610 bitmap projection did not supply a separate CPU row");
            return row;
        }
        // No conversion is bound for an original-layout cache or an isolated
        // source comparison. Preserve the exact operand already read.
        return raw;
    }
private:
    const SourcePhysicsContactMemory &memory_;
    Project project_;
    const NativeD5610BitmapRowBindingTu15 *previous_;
    static inline thread_local const NativeD5610BitmapRowBindingTu15 *active_{};
};

}
