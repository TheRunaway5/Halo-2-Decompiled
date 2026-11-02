#pragma once
#include "halo2/native_tu15_contact_pressure.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include <stdexcept>
namespace halo2 {
// Sole currently qualified production services: authenticated constant type
// getters. Selected candidate/deletion services remain explicitly unbound.
inline NativeTu15ContactPressureServices native_tu15_contact_pressure_native_services(
    NativeTu15ContactPressureStats* activity=nullptr){
    NativeTu15ContactPressureServices services;
    services.classify_shape=[activity](auto,auto owner,auto&){
        if(const auto kind=native_tu15_activation_type(owner))return *kind;
        if(activity)++activity->unbound_services;
        throw std::runtime_error("Native contact pressure requires qualified shape classification owner");
    };
    return services;
}
}
