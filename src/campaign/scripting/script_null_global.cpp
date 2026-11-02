#include "halo2/script_null_global.hpp"
#include "halo2/kernels.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
ScriptValue source_null_campaign_global(unsigned index,const std::optional<ScriptValue> &value) {
    const unsigned type=index==0x3e3?7:index==0xd3?6:index==0x3f3?5:0;
    if(!type || (value && (value->type<5 || value->type>8)))
        throw FormatError("unsupported external campaign global "+std::to_string(index));
    return {std::uint16_t(type),value?scalar_cast_bits(value->bits,value->type,type):0,{}};
}
}
