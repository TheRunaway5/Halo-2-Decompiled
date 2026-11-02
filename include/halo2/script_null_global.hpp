#pragma once
#include "halo2/scripts.hpp"
namespace halo2 {
// Authenticated null-backed declarations: the assignment expression returns
// its converted value, while every subsequent read refreshes to typed zero.
ScriptValue source_null_campaign_global(unsigned,const std::optional<ScriptValue> &);
}
