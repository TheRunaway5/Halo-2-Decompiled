"""Observe complete live source projectile generations in production HUD tests."""


def install_live_projectile_observation(text):
    declaration = ('      std::vector<bool> pending_projectiles;\n'
                   '      std::unique_ptr<SourcePhysicsEntryObserverBinding> shot_observer;\n')
    replacement = ('      std::set<std::uint32_t> authored_projectile_definitions,\n'
                   '          initial_source_projectiles,observed_source_projectiles;\n')
    if text.count(declaration) != 1:
        raise ValueError('Original projectile observer declaration changed')
    text = text.replace(declaration, replacement, 1)
    start = '        std::set<std::uint32_t> projectiles;\n'
    end = '        options.audio_observer='
    if text.count(start) != 1 or text.count(end) != 1:
        raise ValueError('Original projectile observer boundary changed')
    first = text.index(start); last = text.index(end, first)
    prior = text[first:last]
    if ('if(entry!=0xb9b70u)return;' not in prior or
            '"Turret audio shot is not a full live authored projectile generation"' not in prior):
        raise ValueError('Original projectile construction observer changed')
    setup = '''        const auto &weapon=content.weapons.at(actual.definition);
        authored_projectile_definitions.insert(turret_projectile);
        if(weapon.projectile!=null_datum)authored_projectile_definitions.insert(weapon.projectile);
        for(const auto &barrel:weapon.barrel_views)if(barrel && barrel->projectile!=null_datum)
          authored_projectile_definitions.insert(barrel->projectile);
        for(const auto &object:world.source_objects->live_objects())
          if(object.type==5 && authored_projectile_definitions.contains(object.definition))
            initial_source_projectiles.insert(object.object);
'''
    text = text[:first] + setup + text[last:]
    anchor = '''      options.view_state_observer=[&](unsigned view,const MultiplayerState &frame,const Camera &) {
        if(view)return;
'''
    observe = '''        // Complete source snapshots and the live full-generation pool are
        // available in ordinary production builds. Observe their intersection;
        // private constructor-entry instrumentation is not required here.
        if(turret_audio) {
          const auto &memory=world.source_objects->memory();
          const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
          const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
          for(const auto &projectile:frame.weapon_projectiles) {
            if(!authored_projectile_definitions.contains(projectile.definition) ||
                initial_source_projectiles.contains(projectile.datum))continue;
            const auto header=source_raw_datum_lookup(word(0x4ecbd0u),projectile.datum,storage);
            check(header && memory.view(header+3u,1)[0]==std::byte{5} &&
                word(word(header+8u))==projectile.definition,
                "Rendered turret projectile is not a full live authored source generation");
            if(observed_source_projectiles.insert(projectile.datum).second)++constructed_shots;
          }
        }
'''
    if text.count(anchor) != 1:
        raise ValueError('Native frame observer boundary changed')
    text = text.replace(anchor, anchor + observe, 1)
    anchor = '          row["turret_audio"]["constructed_shots"]=constructed_shots;\n'
    if text.count(anchor) != 1:
        raise ValueError('Native projectile report boundary changed')
    return text.replace(anchor, anchor +
        '          row["turret_audio"]["observed_live_source_projectiles"]=observed_source_projectiles;\n'
        '          row["turret_audio"]["projectile_observation"]="Completed native frame and full live source datum ownership; short-lived unobserved generations are not counted";\n', 1)
