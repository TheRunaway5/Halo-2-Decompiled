use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_lobby_query as core;
fn edit(e: ffi::LobbyQueryEdit) -> core::Edit {
    core::Edit {
        valid: e.valid,
        team: e.team,
    }
}
fn edits(e: ffi::LobbyQueryEdits) -> [core::Edit; 4] {
    [edit(e.v0), edit(e.v1), edit(e.v2), edit(e.v3)]
}
fn member(m: ffi::LobbyQueryMember) -> core::Member {
    core::Member {
        handle: m.handle,
        name: m.name,
        is_self: m.is_self,
        leader: m.leader,
        appearance: m.appearance,
    }
}
fn map(m: ffi::LobbyQueryMap) -> core::Map {
    core::Map {
        scenario: m.scenario,
        campaign_id: m.campaign_id,
        counts: m.counts,
    }
}
fn out_edit(e: core::Edit) -> ffi::LobbyQueryEdit {
    ffi::LobbyQueryEdit {
        valid: e.valid,
        team: e.team,
    }
}
fn out_edits(e: [core::Edit; 4]) -> ffi::LobbyQueryEdits {
    ffi::LobbyQueryEdits {
        v0: out_edit(e[0]),
        v1: out_edit(e[1]),
        v2: out_edit(e[2]),
        v3: out_edit(e[3]),
    }
}
struct Io<'a>(&'a ffi::NativeFrontendLobbyQueryIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.lobby_query_facts().map_err(|e| e.to_string())?;
        Ok(core::Facts {
            mode: f.mode,
            party: f.party,
            leader: f.leader,
            members: f.members.into_iter().map(member).collect(),
            requester: f.requester,
            privacy: f.privacy.present.then_some(f.privacy.value),
            voice_party: f.voice_party,
            joined: f.joined,
            transmitting: f.transmitting,
            microphone_muted: f.microphone_muted,
            talking: f.talking.into_iter().map(|p| p.value).collect(),
            muted: f.muted.into_iter().map(|p| p.value).collect(),
            engine: f.engine,
            difficulty: f.difficulty,
            level: f.level,
            map_index: f.map_index,
            protocol: f.protocol,
            profile: f.profile,
            team: f.team,
            edit: edit(f.edit),
            edits: edits(f.edits),
            local_indices: [
                f.local_indices.v0,
                f.local_indices.v1,
                f.local_indices.v2,
                f.local_indices.v3,
            ],
            countdown: f.countdown.present.then_some(f.countdown.value),
            system_host: f.system_host.present.then_some(f.system_host.value),
            system_machines: f.system_machines,
            system_players: f
                .system_players
                .into_iter()
                .map(|p| core::SystemPlayer {
                    name: p.name,
                    team: p.team,
                    session_index: p.session_index,
                })
                .collect(),
            campaign_maps: f.campaign_maps.into_iter().map(map).collect(),
            multiplayer_maps: f.multiplayer_maps.into_iter().map(map).collect(),
        })
    }
    fn start_error(&self) -> core::Result<Vec<u8>> {
        self.0.lobby_query_start_error().map_err(|e| e.to_string())
    }
    fn seconds(&self) -> core::Result<i32> {
        self.0.lobby_query_seconds().map_err(|e| e.to_string())
    }
    fn members(&self) -> core::Result<Vec<core::Member>> {
        self.0
            .lobby_query_members()
            .map(|v| v.into_iter().map(member).collect())
            .map_err(|e| e.to_string())
    }
    fn locals(&self) -> core::Result<Vec<i32>> {
        self.0.lobby_query_locals().map_err(|e| e.to_string())
    }
    fn has_stats(&self) -> core::Result<bool> {
        self.0.lobby_query_has_stats().map_err(|e| e.to_string())
    }
    fn prior_count(&self) -> core::Result<i32> {
        self.0.lobby_query_prior_count().map_err(|e| e.to_string())
    }
    fn campaign_file(&self, n: u32) -> core::Result<Vec<u8>> {
        self.0
            .lobby_query_campaign_file(n)
            .map_err(|e| e.to_string())
    }
    fn profile_option(&self, s: u32, p: u32) -> core::Result<i32> {
        self.0
            .lobby_query_profile_option(s, p)
            .map_err(|e| e.to_string())
    }
    fn variant(&self) -> core::Result<()> {
        self.0.lobby_query_variant().map_err(|e| e.to_string())
    }
    fn defusal(&self) -> core::Result<bool> {
        self.0.lobby_query_defusal().map_err(|e| e.to_string())
    }
    fn value(&self, p: u32) -> core::Result<i32> {
        self.0.lobby_query_value(p).map_err(|e| e.to_string())
    }
    fn appearance(&self) -> core::Result<()> {
        self.0.lobby_query_appearance().map_err(|e| e.to_string())
    }
    fn custom(&self) -> core::Result<bool> {
        self.0.lobby_query_custom().map_err(|e| e.to_string())
    }
    fn capacity(&self, p: u32) -> core::Result<i32> {
        self.0.lobby_query_capacity(p).map_err(|e| e.to_string())
    }
    fn quick(&self, e: u32, values: &[(u32, i32)], s: u32, separator: u32) -> core::Result<()> {
        let values: Vec<_> = values
            .iter()
            .map(|&(parameter, value)| ffi::LobbyQueryValue { parameter, value })
            .collect();
        self.0
            .lobby_query_quick(e, &values, s, separator)
            .map_err(|e| e.to_string())
    }
}
pub fn frontend_lobby_query(
    io: &ffi::NativeFrontendLobbyQueryIo,
) -> core::Result<ffi::LobbyQueryView> {
    let result = core::query(&Io(io))?;
    let present = result.is_some();
    let v = result.unwrap_or_default();
    let i = v.input;
    let input = ffi::LobbyViewInput {
        session: i.session,
        previous: i.previous,
        host: i.host,
        team_mode: i.team_mode,
        local_team_selection: i.local_team_selection,
        countdown: i.countdown,
        start_permission: i.start_permission,
        update_guard: i.update_guard,
        status: i.status,
        delayed_player: i.delayed_player,
        countdown_cancelled: i.countdown_cancelled,
        loading_map: i.loading_map,
        editing_options: i.editing_options,
        team_count: i.team_count,
        player_count: i.player_count,
        max_players: i.max_players,
        overflow_status: i.overflow_status,
        privacy: i.privacy,
        prior_stats: i.prior_stats,
    };
    Ok(ffi::LobbyQueryView {
        present,
        input,
        protocol: v.protocol,
        machines: v.machines,
        engine: v.engine,
        difficulty: v.difficulty,
        map_kind: v.map.map_or(0, |m| if m.campaign { 1 } else { 2 }),
        map_index: v.map.map_or(0, |m| m.index as u64),
        profile: v.profile,
        team: v.team,
        edit: out_edit(v.edit),
        edits: out_edits(v.edits),
        local_indices: ffi::LobbyQueryIndices {
            v0: v.local_indices[0],
            v1: v.local_indices[1],
            v2: v.local_indices[2],
            v3: v.local_indices[3],
        },
        team_count: v.team_count,
        allow_none: v.allow_none,
        quick: v.quick,
        names_present: v.names.is_some(),
        names: v
            .names
            .unwrap_or_default()
            .into_iter()
            .map(|value| ffi::PartyBytesData { value })
            .collect(),
        speakers: v.speakers,
        rows: v
            .rows
            .into_iter()
            .map(|r| ffi::LobbyQueryRow {
                system: r.system,
                source: r.source as u64,
                session_index: r.session_index,
                name: r.name,
                appearance: r.appearance,
            })
            .collect(),
    })
}
