"""Bind the exact host-only birth-color collector and synchronous call borrow."""
import hashlib

INCLUDE='#include "halo2/native_weapon_effect_birth_color_observer.hpp"\n'
ORIGINAL='''                const SourcePhysicsContactRecordObserver observed=[&](const auto &store) {
                    if(o)o(store);
                    const auto pc=store.instruction;
                    const bool ca=pc==0x17b0a3u || pc==0x17e484u || pc==0x17e4d2u || pc==0x17e58eu;
                    const bool cb=pc==0x17b0a9u || pc==0x17e48du || pc==0x17e4dbu || pc==0x17e594u;
                    if((!ca && !cb) || store.written.size()!=4)return;
                    const auto color=Bytes(store.written).u32(0);
                    (ca?event.presentation.colors.a:event.presentation.colors.b)=color;
                    event.presentation.colors_observed=true;
                };'''
PIN='e42c6b0913f84c5677a72c9d1ea6d2e250fb0e58f2b464157aaf1391d776cc69'
REPLACEMENT='''                NativeWeaponEffectBirthColorCollector birth_colors{event.presentation,o};
                const auto&observed=birth_colors.observer();'''
CALL='''                    CaptureLifetime lifetime{*this};
                    required(m,{c.target,c.target,false},a,observed);'''
BORROWED='''                    CaptureLifetime lifetime{*this};
                    auto readonly_stores=birth_colors.borrow();
                    required(m,{c.target,c.target,false},a,observed);'''


def install_birth_color_collector(source):
    if hashlib.sha256(ORIGINAL.encode()).hexdigest()!=PIN:raise ValueError('Original birth-color collector bytes changed')
    if ORIGINAL in source:
        if source.count(ORIGINAL)!=1 or REPLACEMENT in source:raise ValueError('Ambiguous original birth-color collector')
        source=source.replace(ORIGINAL,REPLACEMENT,1)
    elif source.count(REPLACEMENT)!=1:raise ValueError('Birth-color collector owner drift')
    if CALL in source:
        if source.count(CALL)!=1 or BORROWED in source:raise ValueError('Ambiguous birth-color actual call scope')
        source=source.replace(CALL,BORROWED,1)
    elif source.count(BORROWED)!=1:raise ValueError('Birth-color synchronous borrow scope drift')
    if INCLUDE not in source:source=INCLUDE+source
    return source
