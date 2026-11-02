//! Typed CXX IO for the Rust-owned native queue controller.
use crate::ffi;
use halo2_online_protocol::{matchmaking_client as core, matchmaking_readiness};

pub struct MatchmakingClient(core::Client);

pub fn matchmaking_client_runtime_compatibility() -> Result<String, Box<dyn std::error::Error>> {
    Ok(core::runtime_compatibility(crate::eos_config::eos_runtime_options()?.transport).into())
}

pub fn new_matchmaking_client(c: &ffi::MatchmakingClientConfig) -> Box<MatchmakingClient> {
    Box::new(MatchmakingClient(core::Client::new(core::Config {
        input_valid: c.input_valid,
        endpoint: c.endpoint.clone(),
        url: core::UrlParts {
            parsed: c.url_parsed,
            normalized: c.url_normalized.clone(),
            scheme: c.url_scheme.clone(),
            host: c.url_host.clone(),
            user: c.url_user.clone(),
            password: c.url_password.clone(),
            options: c.url_options.clone(),
            path: c.url_path.clone(),
        },
        maps: c.maps.clone(),
        compatibility: c.compatibility.clone(),
        language: c.language,
        poll_ms: c.poll_ms,
        loopback: c.loopback,
        login_present: c.login_present,
        terminal_recovery: c.terminal_recovery,
        force_relay: c.force_relay,
    })))
}

pub fn matchmaking_client_configured(c: &MatchmakingClient) -> bool {
    c.0.configured()
}
pub fn matchmaking_client_current(c: &MatchmakingClient, version: u64) -> bool {
    c.0.current(version)
}
pub fn matchmaking_client_active(c: &MatchmakingClient) -> bool {
    c.0.active()
}

fn appearance(v: &[u8]) -> Result<Option<[u8; 9]>, &'static str> {
    if v.is_empty() {
        Ok(None)
    } else {
        v.try_into()
            .map(Some)
            .map_err(|_| "Invalid native appearance extent")
    }
}

pub fn matchmaking_client_command(
    c: &MatchmakingClient,
    command: &ffi::MatchmakingClientCommand,
) -> Result<u64, &'static str> {
    if !command.input_valid {
        return Ok(c.0.command(
            if matches!(command.kind, ffi::MatchmakingClientCommandKind::Refresh) {
                core::Command::InvalidProfile
            } else {
                core::Command::Invalid
            },
        ));
    }
    let command = match command.kind {
        ffi::MatchmakingClientCommandKind::Refresh => core::Command::Refresh(core::Profile {
            name: command.name.clone(),
            appearance: appearance(&command.appearance)?,
            locals: command
                .locals
                .iter()
                .map(|v| {
                    Ok(core::LocalPlayer {
                        name: v.name.clone(),
                        signed_in: v.signed_in,
                        appearance: appearance(&v.appearance)?,
                    })
                })
                .collect::<Result<_, &'static str>>()?,
        }),
        ffi::MatchmakingClientCommandKind::Join => core::Command::Join(command.playlist.clone()),
        ffi::MatchmakingClientCommandKind::Cancel => core::Command::Cancel,
        ffi::MatchmakingClientCommandKind::SignOut => core::Command::SignOut,
        _ => return Err("Invalid native matchmaking command"),
    };
    Ok(c.0.command(command))
}

fn player(v: &core::Player) -> ffi::MatchmakingClientPlayer {
    ffi::MatchmakingClientPlayer {
        name: v.name.clone(),
        team: v.team,
        is_self: v.is_self,
        level_present: v.level.is_some(),
        level: v.level.unwrap_or(0),
        appearance: v.appearance.map_or_else(Vec::new, |v| v.to_vec()),
    }
}

pub fn matchmaking_client_snapshot(c: &MatchmakingClient) -> ffi::MatchmakingClientSnapshot {
    let s = c.0.snapshot();
    let game = s.game.unwrap_or_default();
    let gathering_present = s.gathering.is_some();
    let gathering = s.gathering.unwrap_or_default();
    ffi::MatchmakingClientSnapshot {
        status: match s.status {
            core::Status::Unconfigured => ffi::MatchmakingClientStatus::Unconfigured,
            core::Status::Idle => ffi::MatchmakingClientStatus::Idle,
            core::Status::Connecting => ffi::MatchmakingClientStatus::Connecting,
            core::Status::Ready => ffi::MatchmakingClientStatus::Ready,
            core::Status::Joining => ffi::MatchmakingClientStatus::Joining,
            core::Status::Searching => ffi::MatchmakingClientStatus::Searching,
            core::Status::CheckingConnections => ffi::MatchmakingClientStatus::CheckingConnections,
            core::Status::AwaitingTransport => ffi::MatchmakingClientStatus::AwaitingTransport,
            core::Status::Loading => ffi::MatchmakingClientStatus::Loading,
            core::Status::Failed => ffi::MatchmakingClientStatus::Failed,
        },
        playlists: s
            .playlists
            .into_iter()
            .map(|p| ffi::MatchmakingClientPlaylist {
                id: p.id,
                revision: p.revision,
                name: p.name,
                description: p.description,
                ranked: p.ranked,
                quickmatch: p.quickmatch,
                eligible: p.eligible,
                minimum: p.minimum,
                maximum: p.maximum,
                level_present: p.level.is_some(),
                level: p.level.unwrap_or(0),
                clan_match: p.clan_match,
            })
            .collect(),
        playlist_id: s.playlist_id,
        message: s.message,
        queued_players: s.queued_players,
        revision: s.revision,
        start_supported: s.start_supported,
        start_present: s.start_ms.is_some(),
        start_ms: s.start_ms.unwrap_or(0),
        failure_present: s.failure_state.is_some(),
        failure_state: s.failure_state.unwrap_or(0),
        game_present: !game.players.is_empty(),
        map: game.map,
        variant: game.variant,
        game_id: game.game_id,
        settings: game.settings.map_or_else(Vec::new, |v| v.to_vec()),
        players: game.players.iter().map(player).collect(),
        gathering_present,
        gathering_players: gathering.players.iter().map(player).collect(),
        minimum: gathering.minimum,
        maximum: gathering.maximum,
        wait_present: gathering.wait_ms.is_some(),
        wait_ms: gathering.wait_ms.unwrap_or(0),
    }
}

struct Io<'a>(&'a ffi::NativeMatchmakingIo);
impl core::Io for Io<'_> {
    fn now_ms(&self) -> u64 {
        self.0.client_now_ms()
    }
    fn cancelled(&self, version: u64) -> bool {
        self.0.client_cancelled(version)
    }
    fn identity(&self, version: u64) -> core::Result<core::Identity> {
        let v = self
            .0
            .client_identity(version)
            .map_err(|_| core::Error::Unavailable)?;
        Ok(core::Identity {
            primary: v.primary_present.then_some(v.primary),
            locals: v
                .locals
                .into_iter()
                .map(|v| core::Grant {
                    authorization: v.present.then_some(v.authorization),
                    steam: v.steam,
                })
                .collect(),
        })
    }
    fn request(&self, r: &core::Request) -> core::Result<core::Response> {
        let v = self
            .0
            .client_request(&ffi::MatchmakingClientRequest {
                version: r.version,
                method: r.method.clone(),
                url: r.url.clone(),
                body: r.body.clone(),
                bearer: r.bearer.clone(),
                cleanup: r.cleanup,
            })
            .map_err(|_| core::Error::Unavailable)?;
        Ok(core::Response {
            status: v.status,
            body: v.body,
        })
    }
    fn connection(&self, version: u64) -> core::Result<core::Connection> {
        let v = self
            .0
            .client_connection(version)
            .map_err(|_| core::Error::Unavailable)?;
        Ok(core::Connection {
            phase: match v.phase {
                ffi::MatchmakingClientPhase::Missing => core::ConnectionPhase::Missing,
                ffi::MatchmakingClientPhase::Connecting => core::ConnectionPhase::Connecting,
                ffi::MatchmakingClientPhase::Loading => core::ConnectionPhase::Loading,
                ffi::MatchmakingClientPhase::Failed => core::ConnectionPhase::Failed,
                ffi::MatchmakingClientPhase::Other => core::ConnectionPhase::Other,
                _ => return Err(core::Error::Invalid),
            },
            error: v.error,
        })
    }
    fn install(&self, version: u64, v: &core::Admission) -> core::Result<bool> {
        let eos_present = v.eos.is_some();
        let eos = v.eos.clone().unwrap_or_default();
        self.0
            .client_install(
                version,
                &ffi::MatchmakingClientAdmission {
                    endpoint: v.endpoint.clone(),
                    room: v.room.clone(),
                    bearer: v.bearer.clone(),
                    current: v.current,
                    seats: v
                        .seats
                        .iter()
                        .map(|s| ffi::MatchmakingClientSeat {
                            slot: s.slot,
                            name: s.name.clone(),
                            team: s.team,
                            identity: s.identity,
                            peer: s.peer,
                            guest: s.guest,
                            level_present: s.level.is_some(),
                            level: s.level.unwrap_or(0),
                        })
                        .collect(),
                    loopback: v.loopback,
                    team_present: v.team_mode.is_some(),
                    team_mode: v.team_mode.unwrap_or(false),
                    direct: v.direct,
                    stun: v.stun.clone(),
                    terminal_recovery: v.terminal_recovery,
                    eos_present,
                    eos_socket: eos.socket,
                    eos_peers: eos
                        .peers
                        .into_iter()
                        .map(|(slot, key)| ffi::EosPeer {
                            slot: slot as u8,
                            key,
                        })
                        .collect(),
                    eos_account_login: eos.account_login,
                    eos_playlist: eos.playlist,
                    eos_revision: eos.revision,
                    eos_map: eos.map,
                    eos_variant: eos.variant,
                    eos_force_relay: eos.force_relay,
                },
            )
            .map_err(|_| core::Error::Unavailable)
    }
    fn disconnect(&self, version: u64) {
        self.0.client_disconnect(version);
    }
    fn qos(
        &self,
        version: u64,
        endpoint: &str,
        bearer: &str,
        loopback: bool,
    ) -> core::Result<bool> {
        self.0
            .client_qos(version, endpoint, bearer, loopback)
            .map_err(|_| core::Error::Unavailable)
    }
}

pub fn matchmaking_client_poll(c: &MatchmakingClient, io: &ffi::NativeMatchmakingIo) {
    c.0.poll(&Io(io));
}
pub fn matchmaking_client_shutdown(c: &MatchmakingClient, io: &ffi::NativeMatchmakingIo) {
    c.0.shutdown(&Io(io));
}
pub fn matchmaking_client_reject_content(c: &MatchmakingClient) -> bool {
    c.0.reject_content()
}
pub fn matchmaking_client_speaker_binding(
    c: &MatchmakingClient,
    row: u32,
    name: &str,
    team: i32,
    slot: u32,
    peer: u32,
) -> bool {
    c.0.speaker_binding(row, name, team, slot, peer)
}
pub fn matchmaking_client_take_launch(
    c: &MatchmakingClient,
    o: &ffi::MatchmakingLaunchObservation,
    seats: &[ffi::MatchmakingSeat],
) -> bool {
    c.0.take_launch(
        matchmaking_readiness::NativeLaunchObservation {
            session_present: o.session_present,
            transferred: false,
            loading: o.loading,
            start_supported: false,
            start_ms: None,
            team_mode: o.team_mode_present.then_some(o.team_mode),
            current_peer: o.current_peer,
        },
        &seats
            .iter()
            .map(|s| matchmaking_readiness::Seat {
                occupied: s.occupied,
                peer: s.peer,
                team: s.team,
            })
            .collect::<Vec<_>>(),
    )
}
