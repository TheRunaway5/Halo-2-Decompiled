//! Authoritative native room/session policy. Native resources are typed IO.
//!
//! This is the replacement service protocol, not recovered original Live wire.
//! Native preparation/ownership observations never manufacture a played world.
use crate::{
    ParsedVoice, decode_voice,
    host_agreement::{Context, Decision},
    matchmaking_json,
    matchmaking_origin::{UrlParts, pinned_origin},
    matchmaking_variant,
};
use ed25519_dalek::{Signature, Signer, SigningKey, VerifyingKey};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::VecDeque;

pub use crate::migration_driver::Phase;
pub const REPORT_BYTES: usize = 0xdc78;
pub const MESSAGE_BYTES: usize = 119_000;
const PACKETS: usize = 512;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
pub type Result<T> = std::result::Result<T, Error>;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("ONLINE match connection failed or ended. Search again.")
    }
}
impl std::error::Error for Error {}
fn need(ok: bool) -> Result<()> {
    if ok { Ok(()) } else { Err(Error) }
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Options {
    pub direct: bool,
    pub continuity: bool,
    pub peer_migration: bool,
    pub snapshot_candidates: bool,
    pub survivor_selection: bool,
    pub terminal_recovery: bool,
    pub eos: bool,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Seat {
    pub slot: u8,
    pub owner: u8,
    pub guest: bool,
    pub team: i32,
    pub name: String,
    pub identity: [u8; 8],
}
pub struct Config {
    pub input_valid: bool,
    pub origin: String,
    pub url: UrlParts,
    pub room: String,
    pub bearer: String,
    pub loopback: bool,
    pub self_peer: u8,
    pub seats: Vec<Seat>,
    pub team_mode: Option<bool>,
    pub protocol: u16,
    pub native_extension_floor: u32,
    pub options: Options,
    pub eos_peers: Vec<u8>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Seal {
    pub epoch: u32,
    pub tick: u64,
    pub digest: String,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Continuity {
    pub epoch: u32,
    pub host: u8,
    pub members: u16,
    pub selected: Option<Seal>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Packet {
    pub peer: u8,
    pub bytes: Vec<u8>,
    pub epoch: u32,
    pub audio_generation: u64,
}
#[derive(Clone, Debug, PartialEq)]
pub struct Audio {
    pub source: u8,
    pub sequence: u32,
    pub radio: bool,
    pub left: f32,
    pub right: f32,
    pub opus: Vec<u8>,
    pub signature: Option<[u8; 64]>,
    pub generation: u64,
}
#[derive(Clone, Debug, PartialEq)]
pub struct Voice {
    pub peer: u8,
    pub preferences: bool,
    pub enabled: bool,
    pub muted: u16,
    pub audio: Audio,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Signal {
    pub peer: u8,
    pub kind: String,
    pub payload: String,
    pub mid: String,
}
#[derive(Clone, Copy, Debug)]
pub struct Link {
    pub peer: u8,
    pub measured: bool,
    pub rtt: u32,
    pub upload: u32,
    pub loss: u16,
}
pub struct DirectObservation {
    pub now: u64,
    pub authenticated: bool,
    pub ready: u16,
    pub failed: u16,
    pub links: Vec<Link>,
    pub authority: Option<Context>,
    pub admitted: Option<Context>,
}
#[derive(Default)]
pub struct DirectEffects {
    pub terminal: Option<Context>,
    pub failed: bool,
    pub effects: Effects,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Resources {
    pub authority: Option<Context>,
    pub quality: Option<Context>,
    pub agreement: Option<Context>,
}
#[derive(Clone, Copy, Debug, Default)]
pub struct PeerResources {
    pub authority: Option<Context>,
    pub common: Option<Context>,
    pub selection: Option<Context>,
    pub snapshot: Option<Context>,
    pub features: [u32; 16],
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct PeerPlan {
    pub common_supported: bool,
    pub selection_supported: bool,
    pub snapshot_supported: bool,
    pub native_control: bool,
    pub common: bool,
    pub handoff: bool,
    pub failed: bool,
    pub setup: bool,
}
impl PeerPlan {
    pub fn allows(&self, kind: crate::matchmaking_native_peers::Poll) -> bool {
        use crate::matchmaking_native_peers::Poll;
        match kind {
            Poll::Authority | Poll::Admissions => self.native_control,
            Poll::Common => self.common && self.common_supported,
            Poll::Selection => self.common && self.selection_supported,
            Poll::Snapshot => self.common && self.snapshot_supported,
            Poll::Quality => true,
            Poll::Agreement => self.handoff,
        }
    }
}
pub struct CandidateObservation {
    pub agreement: Option<(Context, crate::host_agreement::State)>,
    pub quality: Option<Context>,
    pub ranked: Vec<u8>,
    pub selection_supported: bool,
    pub selection_complete: bool,
    pub selection_failed: bool,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Initialize {
    pub context: Context,
    pub common: bool,
    pub snapshot: bool,
    pub agreement: bool,
    pub selection: bool,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Effects {
    pub initialize: Option<Initialize>,
    pub quality: Option<Context>,
    pub observe_host: bool,
    pub audio_fence: bool,
    pub audio_stop: bool,
    pub signal: Option<Signal>,
    pub reply: Option<String>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct View {
    pub session: [u8; 32],
    pub phase: Phase,
    pub host: u8,
    pub error: String,
    pub continuity: Continuity,
    pub voice_members: u16,
    pub voice_blocked: u16,
    pub departed: u16,
    pub result_recorded: Option<bool>,
    pub presence: Option<u16>,
    pub native_fingerprint: Option<u64>,
    pub audio_generation: u64,
    pub peer_started: bool,
    pub peer_fenced: bool,
    pub selection_started: bool,
    pub common_phase: bool,
    pub native_control_phase: bool,
    pub peer_handoff_phase: bool,
    pub sync_audio: bool,
}

struct VoiceKeys {
    seed: [u8; 32],
    keys: Vec<[u8; 32]>,
    signing: SigningKey,
}
pub struct State {
    config: Config,
    endpoint: String,
    socket_url: String,
    authority_session: [u8; 32],
    authority_context: Option<Context>,
    phase: Phase,
    host: u8,
    error: String,
    continuity: Continuity,
    gameplay_admitted: bool,
    peer_started: bool,
    peer_fenced: bool,
    selection_started: bool,
    departed: u16,
    direct_failed: u16,
    direct_opened: u16,
    measurement_started: Option<u64>,
    direct_announced: bool,
    loaded: String,
    native_fingerprint: Option<u64>,
    completion: String,
    confirmation: String,
    report: Vec<u8>,
    recorded: Option<bool>,
    presence_observed: bool,
    presence_closed: bool,
    voice_members: u16,
    voice_blocked: u16,
    voice_enabled: [bool; 16],
    voice_muted: [u16; 16],
    voice_sequences: [u32; 16],
    voice_credits: [f64; 16],
    voice_credit_at: u64,
    voice_preferences_at: u64,
    keys: Option<VoiceKeys>,
    audio_generation: u64,
    audio_present: bool,
    incoming: VecDeque<Packet>,
    outgoing: VecDeque<Packet>,
    voices: VecDeque<Voice>,
    commands: VecDeque<String>,
    seals: VecDeque<Seal>,
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}
fn unhex(text: &str, length: usize) -> Result<Vec<u8>> {
    need(
        text.len() == length * 2
            && text
                .bytes()
                .all(|v| v.is_ascii_digit() || (b'a'..=b'f').contains(&v)),
    )?;
    (0..length)
        .map(|i| u8::from_str_radix(&text[2 * i..2 * i + 2], 16).map_err(|_| Error))
        .collect()
}
fn number(v: &Value, max: u64) -> Result<u64> {
    let n = v.as_u64().ok_or(Error)?;
    need(n <= max)?;
    Ok(n)
}
fn digest(bytes: &[u8]) -> String {
    hex(&Sha256::digest(bytes))
}
fn seal(v: &Value) -> Result<Seal> {
    let o = v.as_object().ok_or(Error)?;
    need(o.len() == 3)?;
    let epoch = number(&v["epoch"], u32::MAX as u64)? as u32;
    let tick = number(&v["tick"], u64::MAX)?;
    let value = v["digest"].as_str().ok_or(Error)?;
    unhex(value, 32)?;
    Ok(Seal {
        epoch,
        tick,
        digest: value.into(),
    })
}
pub fn packet_role(local_host: bool, source_host: bool, kind: u8) -> bool {
    kind == 6
        || (kind == 18 && (local_host || source_host))
        || (local_host && matches!(kind, 1 | 2 | 4 | 19))
        || (source_host && matches!(kind, 3 | 5 | 14 | 20))
}

impl State {
    pub fn new(config: Config, now: u64) -> Result<Self> {
        let options = config.options;
        need(config.input_valid && config.protocol != 0 && (2..=16).contains(&config.seats.len()))?;
        let endpoint =
            pinned_origin(&config.origin, &config.url, config.loopback).map_err(|_| Error)?;
        need(
            config.room.len() == 36
                && config
                    .room
                    .bytes()
                    .all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b) || b == b'-'),
        )?;
        need(
            (32..=128).contains(&config.bearer.len())
                && config
                    .bearer
                    .bytes()
                    .all(|b| b.is_ascii_alphanumeric() || b == b'-' || b == b'_'),
        )?;
        need(!options.eos || options.direct)?;
        need(!options.peer_migration || (options.direct && !options.continuity))?;
        need(!options.snapshot_candidates || (options.direct && options.peer_migration))?;
        need(!options.survivor_selection || options.peer_migration)?;
        need(
            !options.terminal_recovery
                || (options.direct
                    && !options.continuity
                    && !options.peer_migration
                    && !options.snapshot_candidates
                    && !options.survivor_selection),
        )?;
        let n = config.seats.len();
        let this = config.self_peer as usize;
        need(
            this < n && !config.seats[this].guest && config.seats[this].owner == config.self_peer,
        )?;
        for (i, p) in config.seats.iter().enumerate() {
            need(p.slot as usize == i && (p.owner as usize) < n)?;
            let owner = &config.seats[p.owner as usize];
            need(owner.owner == p.owner && !owner.guest)?;
            need(!config.team_mode.unwrap_or(true) || p.team == owner.team)?;
            need(config.seats.iter().filter(|q| q.owner == p.owner).count() <= 4)?;
        }
        if options.eos {
            let mut peers = config.eos_peers.clone();
            peers.sort_unstable();
            let physical = config
                .seats
                .iter()
                .filter(|p| p.owner == p.slot && p.slot != config.self_peer)
                .map(|p| p.slot)
                .collect::<Vec<_>>();
            need(peers == physical)?;
        } else {
            need(config.eos_peers.is_empty())?;
        }
        let (_, tail) = endpoint.split_once(':').ok_or(Error)?;
        let scheme = if config.url.scheme == "https" {
            "wss"
        } else {
            "ws"
        };
        let socket_url = format!(
            "{scheme}:{tail}/v1/matches/{}{}{}",
            config.room,
            if options.direct { "/signal" } else { "/relay" },
            if options.peer_migration {
                "?completion=1&native_handoff=1&departure_game_over=1"
            } else {
                "?completion=1&departure_game_over=1"
            }
        );
        let authority_session = Sha256::digest(config.room.as_bytes()).into();
        let members = ((1u32 << n) - 1) as u16;
        Ok(Self {
            config,
            endpoint,
            socket_url,
            authority_session,
            authority_context: None,
            phase: Phase::Connecting,
            host: 16,
            error: String::new(),
            continuity: Continuity {
                epoch: 0,
                host: 16,
                members,
                selected: None,
            },
            gameplay_admitted: false,
            peer_started: false,
            peer_fenced: false,
            selection_started: false,
            departed: 0,
            direct_failed: 0,
            direct_opened: 0,
            measurement_started: None,
            direct_announced: false,
            loaded: String::new(),
            native_fingerprint: None,
            completion: String::new(),
            confirmation: String::new(),
            report: Vec::new(),
            recorded: None,
            presence_observed: false,
            presence_closed: false,
            voice_members: members,
            voice_blocked: 0,
            voice_enabled: [false; 16],
            voice_muted: [0; 16],
            voice_sequences: [0; 16],
            voice_credits: [10.; 16],
            voice_credit_at: now,
            voice_preferences_at: 0,
            keys: None,
            audio_generation: 0,
            audio_present: false,
            incoming: VecDeque::new(),
            outgoing: VecDeque::new(),
            voices: VecDeque::new(),
            commands: VecDeque::new(),
            seals: VecDeque::new(),
        })
    }
    pub fn socket_url(&self) -> &str {
        &self.socket_url
    }
    pub fn room_api(&self) -> String {
        format!("{}/v1/matches/{}", self.endpoint, self.config.room)
    }
    pub fn bearer(&self) -> &str {
        &self.config.bearer
    }
    pub fn options(&self) -> Options {
        self.config.options
    }
    pub fn self_peer(&self) -> u8 {
        self.config.self_peer
    }
    pub fn authority_limits(&self) -> crate::peer_authority_transfer::Limits {
        let o = self.config.options;
        crate::matchmaking_native_access::transfer_limits(
            o.snapshot_candidates,
            o.survivor_selection,
            o.terminal_recovery,
        )
    }
    pub fn local_slots(&self) -> Vec<u8> {
        self.config
            .seats
            .iter()
            .filter(|s| s.owner == self.config.self_peer)
            .map(|s| s.slot)
            .collect()
    }
    pub fn physical_peers(&self) -> Vec<u8> {
        self.config
            .seats
            .iter()
            .filter(|s| s.owner == s.slot)
            .map(|s| s.slot)
            .collect()
    }
    pub fn presence(&self) -> Option<u16> {
        (!self.presence_closed
            && self.presence_observed
            && matches!(self.phase, Phase::Reviewing | Phase::Postgame))
        .then_some(self.voice_members)
    }
    pub fn continuity_view(&self) -> Option<Continuity> {
        self.epoch_packets().then(|| self.continuity.clone())
    }
    pub fn seats(&self) -> &[Seat] {
        &self.config.seats
    }
    pub fn epoch_packets(&self) -> bool {
        self.config.options.continuity || self.config.options.peer_migration
    }
    pub fn physical(&self, members: u16) -> u16 {
        self.config
            .seats
            .iter()
            .filter(|p| p.owner == p.slot && members & (1u16 << p.slot) != 0)
            .fold(0, |m, p| m | (1u16 << p.slot))
    }
    fn mask(&self) -> u16 {
        ((1u32 << self.config.seats.len()) - 1) as u16
    }
    fn owner(&self, slot: u8) -> Result<u8> {
        self.config
            .seats
            .get(slot as usize)
            .map(|p| p.owner)
            .ok_or(Error)
    }
    fn physical_host(&self, host: u8) -> Result<()> {
        need(self.owner(host)? == host && !self.config.seats[host as usize].guest)
    }
    fn member_group(&self, members: u16) -> Result<()> {
        need(members & !self.mask() == 0)?;
        for p in &self.config.seats {
            need((members & (1u16 << p.slot) != 0) == (members & (1u16 << p.owner) != 0))?;
        }
        Ok(())
    }
    fn peer_handoff_phase(&self) -> bool {
        self.peer_started
            && matches!(
                self.phase,
                Phase::Running | Phase::AwaitingWorld | Phase::Migrating | Phase::Rebinding
            )
    }
    pub fn peer_config(&self) -> Result<crate::matchmaking_native_peers::Config> {
        need(self.config.options.direct)?;
        Ok(crate::matchmaking_native_peers::Config {
            self_peer: self.config.self_peer,
            physical: self.physical(self.mask()),
            epochs: self.epoch_packets(),
        })
    }
    /// Observes the actual fixed physical context and advertised capabilities.
    /// Transport liveness never shrinks this barrier or creates native Ready.
    pub fn peer_plan(&self, o: PeerResources) -> PeerPlan {
        use crate::matchmaking_native_peers::{
            COMMON_FEATURE, SELECTION_FEATURE, SNAPSHOT_FEATURE,
        };
        let supported = |resource: Option<Context>, feature: u32| {
            let Some(context) = resource else {
                return false;
            };
            o.authority == Some(context)
                && (0..16).all(|peer| {
                    peer == self.config.self_peer
                        || context.peers & (1u16 << peer) == 0
                        || o.features[peer as usize] & feature == feature
                })
        };
        let common_supported = self.config.options.direct && supported(o.common, COMMON_FEATURE);
        let selection_supported =
            self.config.options.survivor_selection && supported(o.selection, SELECTION_FEATURE);
        let snapshot_supported = self.config.options.snapshot_candidates
            && common_supported
            && supported(o.snapshot, SNAPSHOT_FEATURE);
        PeerPlan {
            common_supported,
            selection_supported,
            snapshot_supported,
            native_control: matches!(
                self.phase,
                Phase::Loading | Phase::Running | Phase::Migrating | Phase::Rebinding
            ) || (self.selection_started && self.phase == Phase::AwaitingWorld),
            common: matches!(
                self.phase,
                Phase::Loading
                    | Phase::Running
                    | Phase::AwaitingWorld
                    | Phase::Migrating
                    | Phase::Rebinding
            ),
            handoff: self.peer_handoff_phase(),
            failed: self.phase == Phase::Failed,
            setup: matches!(self.phase, Phase::Connecting | Phase::Loading),
        }
    }
    pub fn view(&self) -> View {
        let peer = self.peer_plan(PeerResources::default());
        View {
            session: self.authority_session,
            phase: self.phase,
            host: self.host,
            error: self.error.clone(),
            continuity: self.continuity.clone(),
            voice_members: self.voice_members,
            voice_blocked: self.voice_blocked,
            departed: self.departed | self.direct_failed,
            result_recorded: self.recorded,
            presence: self.presence(),
            native_fingerprint: self.native_fingerprint,
            audio_generation: self.audio_generation,
            peer_started: self.peer_started,
            peer_fenced: self.peer_fenced,
            selection_started: self.selection_started,
            common_phase: peer.common,
            native_control_phase: peer.native_control,
            peer_handoff_phase: peer.handoff,
            sync_audio: !self.peer_fenced
                && matches!(
                    self.phase,
                    Phase::Running | Phase::Reviewing | Phase::Postgame
                ),
        }
    }
    fn clear_media(&mut self) {
        self.incoming.clear();
        self.outgoing.clear();
        self.voices.clear();
    }
    pub fn fence_audio(&mut self, generation: u64) {
        self.audio_generation = generation;
        self.voices.clear();
        self.outgoing.retain(|p| p.bytes.get(6) != Some(&6));
    }
    pub fn bind_audio(&mut self) {
        self.audio_present = true;
    }
    pub fn control_config(&self) -> crate::matchmaking_native_control::Config {
        crate::matchmaking_native_control::Config {
            url: self.socket_url.clone(),
            bearer: self.config.bearer.clone(),
            direct: self.config.options.direct,
            continuity: self.config.options.continuity,
        }
    }
    pub fn initial_quality(&self) -> Option<Context> {
        let peers = self.physical(self.mask());
        (self.config.options.direct && peers.count_ones() > 1).then_some(Context {
            session: self.authority_session,
            epoch: 0,
            membership_revision: 0,
            peers,
            host: 16,
        })
    }
    /// Chooses only among the actual agreement's Ready values and the actual
    /// same-generation reciprocal quality order. It creates no native Ready.
    pub fn migration_candidate(&self, o: CandidateObservation) -> Option<u8> {
        if !self.peer_handoff_phase() || o.ranked.len() > 16 {
            return None;
        }
        let (context, state) = o.agreement?;
        if self.authority_context != Some(context) || o.quality != Some(context) {
            return None;
        }
        if context.host >= 16 || state.electorate != context.peers & !(1u16 << context.host) {
            return None;
        }
        if self.config.options.survivor_selection
            && (!o.selection_supported || !o.selection_complete || o.selection_failed)
        {
            return None;
        }
        if (!state.suspected && !state.fenced)
            || state.decision.is_some()
            || state.electorate & (1u16 << self.config.self_peer) == 0
        {
            return None;
        }
        if state.electorate.count_ones() == 1 {
            return state.ready[self.config.self_peer as usize].map(|_| self.config.self_peer);
        }
        o.ranked.into_iter().find(|p| {
            *p < 16 && state.electorate & (1u16 << p) != 0 && state.ready[*p as usize].is_some()
        })
    }
    pub fn control_flush(&mut self) -> crate::matchmaking_native_control::Flush {
        let loaded =
            (self.phase == Phase::Loading && !self.loaded.is_empty()).then(|| self.loaded.clone());
        let completion = self.completion_text().map(str::to_owned);
        let confirmation = self.confirmation_text().map(str::to_owned);
        let commands = self.take_commands();
        let packets = if self.config.options.direct {
            Vec::new()
        } else {
            self.take_outgoing()
        };
        crate::matchmaking_native_control::Flush {
            loaded,
            completion,
            confirmation,
            commands,
            packets,
        }
    }
    pub fn fail(&mut self, message: &str) -> Effects {
        if self.phase != Phase::Postgame {
            self.phase = Phase::Failed;
        }
        self.error = message.into();
        self.voice_enabled.fill(false);
        Effects {
            audio_fence: true,
            audio_stop: true,
            ..Effects::default()
        }
    }
    /// Reconnection policy is derived from actual direct/session state, not IO.
    pub fn connection_failure(&mut self, terminal: bool, message: &str) -> (bool, Effects) {
        let o = self.config.options;
        if terminal {
            if o.terminal_recovery && o.direct && self.phase == Phase::Running {
                self.error = "Match admission ended; waiting for actual direct-peer status.".into();
                return (true, Effects::default());
            }
            if o.direct && o.peer_migration && self.peer_started && self.phase != Phase::Failed {
                self.error="Master admission ended; direct peers retain authority until the native handoff completes.".into();
                return (true, Effects::default());
            }
        } else if o.direct
            && (matches!(
                self.phase,
                Phase::Running | Phase::Reviewing | Phase::Postgame
            ) || (o.peer_migration && self.peer_started && self.phase != Phase::Failed))
        {
            self.error="Master connection unavailable; the player-hosted match continues. Results will retry.".into();
            return (true, Effects::default());
        }
        (false, self.fail(message))
    }
    pub fn text(&mut self, bytes: &[u8], resources: Resources) -> Result<Effects> {
        let v = matchmaking_json::parse(bytes, MESSAGE_BYTES).map_err(|_| Error)?;
        let object = v.as_object().ok_or(Error)?;
        if object.contains_key("departed_players") {
            need(object.len() == 1 && self.gameplay_admitted)?;
            let mask = number(&v["departed_players"], u16::MAX as u64)? as u16;
            self.member_group(mask)?;
            need(mask & (1u16 << self.config.self_peer) == 0)?;
            self.departed |= mask;
            return Ok(Effects::default());
        }
        if object.contains_key("voice_auth") {
            need(object.len() == 1)?;
            let a = v["voice_auth"].as_object().ok_or(Error)?;
            need(a.len() == 3 && v["voice_auth"]["version"].as_u64() == Some(1))?;
            let seed: [u8; 32] = unhex(v["voice_auth"]["seed"].as_str().ok_or(Error)?, 32)?
                .try_into()
                .map_err(|_| Error)?;
            let values = v["voice_auth"]["keys"].as_array().ok_or(Error)?;
            need(values.len() == self.config.seats.len())?;
            let keys = values
                .iter()
                .map(|v| {
                    unhex(v.as_str().ok_or(Error)?, 32)?
                        .try_into()
                        .map_err(|_| Error)
                })
                .collect::<Result<Vec<[u8; 32]>>>()?;
            for p in &self.config.seats {
                need(keys[p.slot as usize] == keys[p.owner as usize])?;
            }
            if let Some(previous) = &self.keys {
                need(previous.seed == seed && previous.keys == keys)?;
            } else {
                let signing = SigningKey::from_bytes(&seed);
                need(signing.verifying_key().to_bytes() == keys[self.config.self_peer as usize])?;
                self.keys = Some(VoiceKeys {
                    seed,
                    keys,
                    signing,
                });
            }
            return Ok(Effects::default());
        }
        if object.contains_key("peer_signal") {
            need(self.config.options.direct && object.len() == 1)?;
            let s = v["peer_signal"].as_object().ok_or(Error)?;
            need(s.len() == 4)?;
            return Ok(Effects {
                signal: Some(Signal {
                    peer: number(&v["peer_signal"]["peer"], u8::MAX as u64)? as u8,
                    kind: v["peer_signal"]["type"].as_str().ok_or(Error)?.into(),
                    payload: v["peer_signal"]["payload"].as_str().ok_or(Error)?.into(),
                    mid: v["peer_signal"]["mid"].as_str().ok_or(Error)?.into(),
                }),
                ..Effects::default()
            });
        }
        if object.contains_key("world_seal") {
            need(self.config.options.continuity && object.len() == 1)?;
            let s = seal(&v["world_seal"])?;
            need(s.epoch <= self.continuity.epoch)?;
            if self.seals.len() == 64 {
                self.seals.pop_front();
            }
            self.seals.push_back(s);
            return Ok(Effects::default());
        }
        if object.contains_key("postgame") {
            need(
                object.len() == 1
                    && (self.phase == Phase::Running
                        || (self.config.options.direct
                            && matches!(self.phase, Phase::Reviewing | Phase::Postgame))),
            )?;
            let bytes = unhex(v["postgame"].as_str().ok_or(Error)?, REPORT_BYTES)?;
            need(self.report.is_empty() || self.report == bytes)?;
            self.report = bytes;
            self.phase = Phase::Reviewing;
            return Ok(Effects::default());
        }
        if object.contains_key("voice_blocked") {
            need(object.len() == 1)?;
            let mask = number(&v["voice_blocked"], u16::MAX as u64)? as u16;
            need(mask & !self.mask() == 0 && mask & (1u16 << self.config.self_peer) == 0)?;
            self.voice_blocked = mask;
            return Ok(Effects::default());
        }
        if object.contains_key("voice_host") {
            need(
                object.len() == 2
                    && (self.phase == Phase::Postgame
                        || ((self.config.options.continuity || self.config.options.direct)
                            && self.phase == Phase::Reviewing)),
            )?;
            let host = number(&v["voice_host"], 15)? as u8;
            self.physical_host(host)?;
            let members = number(&v["voice_members"], u16::MAX as u64)? as u16;
            self.member_group(members)?;
            need(members & (1u16 << host) != 0 && members & (1u16 << self.config.self_peer) != 0)?;
            let fence = host != self.host || members != self.voice_members;
            self.host = host;
            self.voice_members = members;
            self.presence_observed = true;
            for i in 0..16 {
                if members & (1u16 << i) == 0 {
                    self.voice_enabled[i] = false;
                }
            }
            return Ok(Effects {
                audio_fence: fence,
                ..Effects::default()
            });
        }
        if object.contains_key("completed") {
            need(
                object.len() == 1 + usize::from(object.contains_key("recorded"))
                    && self.phase == Phase::Reviewing
                    && !self.confirmation.is_empty(),
            )?;
            need(v["completed"].as_str() == Some(digest(&self.report).as_str()))?;
            let recorded = if object.contains_key("recorded") {
                Some(v["recorded"].as_bool().ok_or(Error)?)
            } else {
                None
            };
            self.recorded = recorded;
            self.phase = Phase::Postgame;
            return Ok(Effects::default());
        }
        if object.contains_key("ping") {
            need(object.len() == 1)?;
            let ping = v["ping"].as_str().ok_or(Error)?;
            need(ping.len() == 16)?;
            return Ok(Effects {
                reply: Some(json!({"pong":ping}).to_string()),
                ..Effects::default()
            });
        }
        self.phase_control(&v, resources)
    }
    fn phase_control(&mut self, v: &Value, resources: Resources) -> Result<Effects> {
        need(
            v.as_object().ok_or(Error)?.len() == if self.config.options.continuity { 3 } else { 2 },
        )?;
        let name = v["phase"].as_str().ok_or(Error)?;
        let phase = match name {
            "connecting" => Phase::Connecting,
            "loading" => Phase::Loading,
            "running" => Phase::Running,
            "awaiting_world" if self.config.options.continuity => Phase::AwaitingWorld,
            "migrating" if self.config.options.continuity => Phase::Migrating,
            "rebinding" if self.config.options.continuity => Phase::Rebinding,
            _ => return Err(Error),
        };
        if self.phase == Phase::Failed || (self.config.options.peer_migration && self.peer_started)
        {
            return Ok(Effects::default());
        }
        let previous_host = self.host;
        let previous_epoch = self.continuity.epoch;
        let host = if phase == Phase::Connecting {
            self.host
        } else if phase == Phase::AwaitingWorld {
            need(v["host"].is_null())?;
            16
        } else {
            let h = number(&v["host"], 15)? as u8;
            self.physical_host(h)?;
            h
        };
        let mut continuity = self.continuity.clone();
        if self.config.options.continuity {
            if matches!(phase, Phase::Connecting | Phase::Loading) {
                need(v["continuity"].is_null())?;
            } else {
                let c = v["continuity"].as_object().ok_or(Error)?;
                let epoch = number(&v["continuity"]["epoch"], u32::MAX as u64)? as u32;
                let members = number(&v["continuity"]["members"], u16::MAX as u64)? as u16;
                self.member_group(members)?;
                need(
                    epoch >= continuity.epoch
                        && members & (1u16 << self.config.self_peer) != 0
                        && (host == 16 || members & (1u16 << host) != 0),
                )?;
                need(
                    v["continuity"]["phase"].as_str() == Some(name)
                        && v["continuity"]["host"] == v["host"],
                )?;
                let selected = if matches!(phase, Phase::Migrating | Phase::Rebinding) {
                    need(c.len() == 6)?;
                    Some(seal(
                        &json!({"epoch":epoch,"tick":v["continuity"]["tick"],"digest":v["continuity"]["digest"]}),
                    )?)
                } else {
                    need(c.len() == 4)?;
                    None
                };
                continuity = Continuity {
                    epoch,
                    host,
                    members,
                    selected,
                };
            }
        }
        let mut effects = Effects::default();
        let o = self.config.options;
        if o.direct
            && resources.authority.is_none()
            && matches!(phase, Phase::Loading | Phase::Running)
            && host < 16
        {
            let physical = self.physical(self.mask());
            let context = Context {
                session: self.authority_session,
                epoch: continuity.epoch,
                membership_revision: 0,
                peers: physical,
                host,
            };
            effects.initialize = Some(Initialize {
                context,
                common: physical.count_ones() > 1 && !o.terminal_recovery,
                snapshot: o.snapshot_candidates && physical.count_ones() > 1,
                agreement: o.peer_migration && physical.count_ones() > 1,
                selection: o.survivor_selection
                    && physical.count_ones() > 1
                    && self.config.self_peer != host,
            });
        }
        if o.direct && host < 16 && phase != Phase::Connecting {
            let physical = self.physical(continuity.members);
            let mut context = Context {
                session: self.authority_session,
                epoch: continuity.epoch,
                membership_revision: continuity.members as u64,
                peers: physical,
                host,
            };
            let agreement = resources.agreement.or(effects
                .initialize
                .filter(|i| i.agreement)
                .map(|i| i.context));
            if o.peer_migration {
                if let Some(old) = agreement {
                    need(
                        old.session == context.session
                            && old.epoch == context.epoch
                            && old.peers == context.peers
                            && old.host == context.host,
                    )?;
                    context = old;
                }
            }
            if physical.count_ones() > 1 && resources.quality != Some(context) {
                effects.quality = Some(context);
            }
        }
        self.host = host;
        if let Some(init) = effects.initialize {
            self.authority_context = Some(init.context);
        }
        self.phase = phase;
        self.continuity = continuity;
        if o.continuity {
            self.voice_members = self.continuity.members;
            for i in 0..16 {
                if self.voice_members & (1u16 << i) == 0 {
                    self.voice_enabled[i] = false;
                }
            }
            if previous_host != host || previous_epoch != self.continuity.epoch {
                self.clear_media();
            }
        }
        self.gameplay_admitted |= phase == Phase::Running;
        if o.peer_migration && phase == Phase::Running {
            self.peer_started = true;
            self.continuity.host = host;
            effects.observe_host = true;
        }
        Ok(effects)
    }
    pub fn loaded(
        &mut self,
        fingerprint: u64,
        settings: &[u8],
        parameters: &[(u32, i32)],
    ) -> Result<()> {
        need(matchmaking_variant::valid(settings))?;
        let mut bytes = fingerprint.to_le_bytes().to_vec();
        bytes.extend_from_slice(settings);
        let mut parameters = parameters
            .iter()
            .copied()
            .filter(|(id, _)| *id >= self.config.native_extension_floor)
            .collect::<Vec<_>>();
        parameters.sort_unstable_by_key(|(id, _)| *id);
        need(parameters.windows(2).all(|p| p[0].0 != p[1].0))?;
        for (id, value) in parameters {
            bytes.extend_from_slice(&id.to_le_bytes());
            bytes.extend_from_slice(&value.to_le_bytes());
        }
        self.loaded = format!("{}:{:07x}", &digest(&bytes)[..16], self.config.protocol);
        self.native_fingerprint = Some(fingerprint);
        Ok(())
    }
    pub fn loaded_text(&self) -> &str {
        &self.loaded
    }
    pub fn complete(&mut self, report: &[u8]) -> Result<()> {
        need(
            report.len() == REPORT_BYTES
                && self.host == self.config.self_peer
                && self.phase == Phase::Running,
        )?;
        if self.completion.is_empty() {
            self.completion = json!({"postgame":hex(report)}).to_string();
        }
        Ok(())
    }
    pub fn confirm(&mut self) -> Result<()> {
        need(self.phase == Phase::Reviewing && self.report.len() == REPORT_BYTES)?;
        if self.confirmation.is_empty() {
            self.confirmation = json!({"confirm_result":digest(&self.report)}).to_string();
        }
        Ok(())
    }
    pub fn postgame(&self) -> &[u8] {
        &self.report
    }
    pub fn close_postgame(&mut self) {
        self.presence_closed = true;
        self.presence_observed = false;
    }
    /// Complete native caller: derives the current epoch or exact selected seal
    /// from this owner instead of accepting C++ continuity/phase decisions.
    pub fn queue_world_request(&mut self, kind: u8, value: Seal) -> Result<()> {
        let value = match kind {
            0 => {
                need(self.phase == Phase::Running && self.host == self.config.self_peer)?;
                Seal {
                    epoch: self.continuity.epoch,
                    ..value
                }
            }
            1 => {
                need(
                    (self.phase == Phase::Running && value.epoch == self.continuity.epoch)
                        || (self.phase == Phase::AwaitingWorld
                            && u64::from(value.epoch) + 1 == u64::from(self.continuity.epoch)),
                )?;
                value
            }
            2 | 3 => {
                need(if kind == 2 {
                    self.phase == Phase::Migrating && self.config.self_peer == self.host
                } else {
                    self.phase == Phase::Rebinding && self.config.self_peer != self.host
                })?;
                let selected = self.continuity.selected.as_ref().ok_or(Error)?;
                need(selected.tick == value.tick && selected.digest == value.digest)?;
                selected.clone()
            }
            _ => return Err(Error),
        };
        self.queue_world(kind, value)
    }
    pub fn queue_world(&mut self, kind: u8, value: Seal) -> Result<()> {
        need(self.config.options.continuity && self.commands.len() < 64)?;
        unhex(&value.digest, 32)?;
        let name = match kind {
            0 => {
                need(
                    self.phase == Phase::Running
                        && self.host == self.config.self_peer
                        && value.epoch == self.continuity.epoch,
                )?;
                "world_seal"
            }
            1 => {
                need(
                    (self.phase == Phase::Running && value.epoch == self.continuity.epoch)
                        || (self.phase == Phase::AwaitingWorld
                            && u64::from(value.epoch) + 1 == u64::from(self.continuity.epoch)),
                )?;
                "world_ack"
            }
            2 | 3 => {
                need(self.continuity.selected.as_ref() == Some(&value))?;
                need(if kind == 2 {
                    self.phase == Phase::Migrating && self.host == self.config.self_peer
                } else {
                    self.phase == Phase::Rebinding && self.host != self.config.self_peer
                })?;
                if kind == 2 {
                    "migration_accept"
                } else {
                    "migration_rebind"
                }
            }
            _ => return Err(Error),
        };
        self.commands.push_back(
            json!({name:{"epoch":value.epoch,"tick":value.tick,"digest":value.digest}}).to_string(),
        );
        Ok(())
    }
    pub fn take_seals(&mut self) -> Vec<Seal> {
        self.seals.drain(..).collect()
    }
    pub fn signal(&mut self, signal: Signal) -> Result<()> {
        need(self.commands.len() < 1024)?;
        self.commands.push_back(json!({"peer_signal":{"peer":signal.peer,"type":signal.kind,"payload":signal.payload,"mid":signal.mid}}).to_string());
        Ok(())
    }
    pub fn send(&mut self, peer: u8, bytes: &[u8]) -> bool {
        if self.phase != Phase::Running
            || self.owner(peer).is_err()
            || bytes.len() > 1400
            || self.outgoing.len() >= PACKETS
            || (self.epoch_packets() && self.continuity.members & (1u16 << peer) == 0)
        {
            return false;
        }
        self.outgoing.push_back(Packet {
            peer,
            bytes: bytes.into(),
            epoch: self.continuity.epoch,
            audio_generation: 0,
        });
        true
    }
    pub fn receive(&mut self) -> Option<Packet> {
        self.incoming.pop_front()
    }
    pub fn take_voices(&mut self) -> Vec<Voice> {
        self.voices.drain(..).collect()
    }
    pub fn take_commands(&mut self) -> Vec<String> {
        self.commands.drain(..).collect()
    }
    pub fn take_outgoing(&mut self) -> Vec<Packet> {
        let members = if matches!(self.phase, Phase::Reviewing | Phase::Postgame) {
            self.voice_members
        } else {
            self.continuity.members
        };
        let generation = self.audio_generation;
        let epoch = self.continuity.epoch;
        let epoch_packets = self.epoch_packets();
        let fenced = self.peer_fenced;
        self.outgoing
            .drain(..)
            .filter(|p| {
                !fenced
                    && (!epoch_packets || (p.epoch == epoch && members & (1u16 << p.peer) != 0))
                    && (p.bytes.get(6) != Some(&6) || p.audio_generation == generation)
            })
            .collect()
    }
    pub fn packet_current(&self, p: &Packet) -> bool {
        let members = if matches!(self.phase, Phase::Reviewing | Phase::Postgame) {
            self.voice_members
        } else {
            self.continuity.members
        };
        !self.peer_fenced
            && p.peer < 16
            && (!self.epoch_packets()
                || (p.epoch == self.continuity.epoch && members & (1u16 << p.peer) != 0))
            && (p.bytes.get(6) != Some(&6) || p.audio_generation == self.audio_generation)
    }
    pub fn completion_text(&self) -> Option<&str> {
        if self.phase == Phase::Running && !self.completion.is_empty() {
            Some(&self.completion)
        } else {
            None
        }
    }
    pub fn confirmation_text(&self) -> Option<&str> {
        if self.phase == Phase::Reviewing && !self.confirmation.is_empty() {
            Some(&self.confirmation)
        } else {
            None
        }
    }
    fn voice_message(&self, body: &[u8], epoch: u32, target: u8) -> Vec<u8> {
        let mut bytes = b"H2VOICE2\0".to_vec();
        bytes.extend_from_slice(self.config.room.as_bytes());
        bytes.extend_from_slice(&epoch.to_le_bytes());
        bytes.push(target);
        bytes.extend_from_slice(body);
        bytes
    }
    pub fn datagram(&mut self, source: u8, bytes: &[u8], epoch: u32) -> Result<bool> {
        let frozen = self.selection_started && self.phase == Phase::AwaitingWorld;
        if !matches!(
            self.phase,
            Phase::Running | Phase::Reviewing | Phase::Postgame
        ) && !frozen
        {
            return Ok(false);
        }
        need(source != self.config.self_peer && self.owner(source)? == source)?;
        if self.epoch_packets()
            && (epoch != self.continuity.epoch || self.continuity.members & (1u16 << source) == 0)
        {
            return Ok(false);
        }
        need(
            (7..=1400).contains(&bytes.len())
                && bytes[..4] == *b"H2N2"
                && u16::from_le_bytes([bytes[4], bytes[5]]) == self.config.protocol,
        )?;
        let kind = bytes[6];
        need(packet_role(
            self.config.self_peer == self.host,
            source == self.host,
            kind,
        ))?;
        if kind != 6 {
            if self.phase == Phase::Running && self.incoming.len() < PACKETS {
                self.incoming.push_back(Packet {
                    peer: source,
                    bytes: bytes.into(),
                    epoch,
                    audio_generation: 0,
                });
            }
            return Ok(source == self.host && (self.phase == Phase::Running || frozen));
        }
        if frozen || self.voice_members & (1u16 << source) == 0 {
            return Ok(false);
        }
        let mut body = bytes;
        let mut signature = None;
        if bytes.get(7) == Some(&0) {
            need(bytes.len() > 64)?;
            body = &bytes[..bytes.len() - 64];
            let sig: [u8; 64] = bytes[bytes.len() - 64..].try_into().map_err(|_| Error)?;
            need(body.len() > 22 && body.len() <= 278)?;
            let speaker = *body.get(8).ok_or(Error)?;
            need(self.owner(speaker)? == source)?;
            let keys = self.keys.as_ref().ok_or(Error)?;
            let key = VerifyingKey::from_bytes(keys.keys.get(source as usize).ok_or(Error)?)
                .map_err(|_| Error)?;
            key.verify_strict(
                &self.voice_message(body, epoch, self.config.self_peer),
                &Signature::from_bytes(&sig),
            )
            .map_err(|_| Error)?;
            signature = Some(sig);
        }
        let decoded = decode_voice(body, self.config.protocol).map_err(|_| Error)?;
        let mut voice = Voice {
            peer: source,
            preferences: false,
            enabled: false,
            muted: 0,
            audio: Audio {
                source,
                sequence: 0,
                radio: false,
                left: 0.,
                right: 0.,
                opus: Vec::new(),
                signature,
                generation: self.audio_generation,
            },
        };
        match decoded {
            ParsedVoice::Preferences { enabled, muted } => {
                voice.preferences = true;
                voice.enabled = enabled;
                voice.muted = muted;
            }
            ParsedVoice::ForwardedPreferences { .. } => return Err(Error),
            ParsedVoice::Audio {
                source: speaker,
                sequence,
                radio,
                left,
                right,
                opus,
            } => {
                need(self.owner(speaker)? == source)?;
                if self.voice_members & (1u16 << speaker) == 0
                    || self.voice_blocked & (1u16 << speaker) != 0
                {
                    return Ok(false);
                }
                voice.audio.source = speaker;
                voice.audio.sequence = sequence;
                voice.audio.radio = radio;
                voice.audio.left = left;
                voice.audio.right = right;
                voice.audio.opus = opus;
            }
        }
        if self.voices.len() == 80 {
            self.voices.pop_front();
        }
        self.voices.push_back(voice);
        Ok(false)
    }
    pub fn voice(&mut self, target: u8, frame: &Audio, audio_ready: bool) -> bool {
        if frame.source >= 16
            || frame.opus.is_empty()
            || frame.opus.len() > 256
            || self.owner(frame.source).ok() != Some(self.config.self_peer)
            || target >= 16
            || self.voice_blocked & (1u16 << target) != 0
        {
            return false;
        }
        if self.audio_present && (!audio_ready || frame.generation != self.audio_generation) {
            return false;
        }
        if !matches!(
            self.phase,
            Phase::Running | Phase::Reviewing | Phase::Postgame
        ) || self.owner(target).is_err()
            || (self.epoch_packets() && self.voice_members & (1u16 << target) == 0)
            || self.outgoing.len() >= PACKETS
        {
            return false;
        }
        let Some(keys) = &self.keys else {
            return false;
        };
        let mut body = b"H2N2".to_vec();
        body.extend_from_slice(&self.config.protocol.to_le_bytes());
        body.extend_from_slice(&[6, 0, frame.source]);
        body.extend_from_slice(&frame.sequence.to_le_bytes());
        body.push(u8::from(frame.radio));
        body.extend_from_slice(&frame.left.to_bits().to_le_bytes());
        body.extend_from_slice(&frame.right.to_bits().to_le_bytes());
        body.extend_from_slice(&frame.opus);
        let signature = keys
            .signing
            .sign(&self.voice_message(&body, self.continuity.epoch, target))
            .to_bytes();
        body.extend_from_slice(&signature);
        self.outgoing.push_back(Packet {
            peer: target,
            bytes: body,
            epoch: self.continuity.epoch,
            audio_generation: self.audio_generation,
        });
        true
    }
    pub fn voice_preferences(&mut self, target: u8, enabled: bool, muted: u16) -> bool {
        if !matches!(
            self.phase,
            Phase::Running | Phase::Reviewing | Phase::Postgame
        ) || self.owner(target).is_err()
            || (self.epoch_packets() && self.voice_members & (1u16 << target) == 0)
            || self.outgoing.len() >= PACKETS
        {
            return false;
        }
        let mut bytes = b"H2N2".to_vec();
        bytes.extend_from_slice(&self.config.protocol.to_le_bytes());
        bytes.extend_from_slice(&[6, 1, u8::from(enabled)]);
        bytes.extend_from_slice(&muted.to_le_bytes());
        self.outgoing.push_back(Packet {
            peer: target,
            bytes,
            epoch: self.continuity.epoch,
            audio_generation: self.audio_generation,
        });
        true
    }
    pub fn direct_failures(&mut self, opened: u16, failed: u16) {
        if self.phase == Phase::Running {
            self.direct_failed = self
                .config
                .seats
                .iter()
                .filter(|p| {
                    p.owner != self.config.self_peer
                        && opened & (1u16 << p.owner) != 0
                        && failed & (1u16 << p.owner) != 0
                })
                .fold(0, |mask, p| mask | (1u16 << p.slot));
        }
    }
    pub fn direct_observation(&mut self, o: DirectObservation) -> Result<DirectEffects> {
        need(self.config.options.direct && o.links.len() <= 16)?;
        let physical = self.physical(self.mask());
        need((o.ready | o.failed) & !physical == 0)?;
        self.direct_opened |= o.ready;
        self.direct_failures(self.direct_opened, o.failed);
        if self.measurement_started.is_none() && o.authenticated {
            self.measurement_started = Some(o.now);
        }
        let mut measured = 0u16;
        let mut peers = 0u16;
        for link in &o.links {
            need(
                link.peer < 16
                    && peers & (1u16 << link.peer) == 0
                    && physical & (1u16 << link.peer) != 0,
            )?;
            peers |= 1u16 << link.peer;
            if link.measured {
                measured |= 1u16 << link.peer;
            }
        }
        if !self.direct_announced {
            if let Some(start) = self.measurement_started {
                if o.now.checked_sub(start).ok_or(Error)?
                    >= if self.config.options.eos {
                        45_000
                    } else {
                        10_000
                    }
                    || (measured | (1u16 << self.config.self_peer)) & physical == physical
                {
                    need(self.commands.len() < 1024)?;
                    let rows=self.config.seats.iter().filter(|p|p.owner==p.slot && p.slot!=self.config.self_peer).map(|p| {
                        let link=o.links.iter().find(|l|l.peer==p.slot&&l.measured);
                        json!({"peer":p.slot,"rtt_ms":link.map(|l|l.rtt),"upload_bytes_per_second":link.map(|l|l.upload),"loss_per_mille":link.map(|l|l.loss)})
                    }).collect::<Vec<_>>();
                    self.commands
                        .push_back(json!({"peer_ready":rows}).to_string());
                    self.direct_announced = true;
                }
            }
        }
        let mut result = DirectEffects::default();
        if !(self.config.options.peer_migration && self.peer_started)
            && self.host < 16
            && self.host != self.config.self_peer
            && !matches!(self.phase, Phase::Reviewing | Phase::Postgame)
            && o.failed & (1u16 << self.host) != 0
        {
            let mut observing = false;
            if self.config.options.terminal_recovery && self.phase == Phase::Running {
                if let (Some(context), Some(admitted)) = (o.authority, o.admitted) {
                    if Some(context) == self.authority_context
                        && admitted == context
                        && context.host == self.host
                    {
                        let retired = context.peers & o.failed & !(1u16 << self.config.self_peer);
                        if context.peers == retired | (1u16 << self.config.self_peer) {
                            result.terminal = Some(context);
                        } else {
                            observing = true;
                        }
                    }
                }
            }
            if observing {
                self.error =
                    "Player host was lost; observing the remaining direct connections.".into();
            } else {
                result.failed = true;
                result.effects = self.fail("Direct player connection was lost. Search again.");
            }
        }
        Ok(result)
    }
    pub fn native_fence(
        &mut self,
        context: Context,
        fenced: bool,
        decision: Option<Decision>,
        selection_started: bool,
        selection_failed: bool,
    ) -> Result<Effects> {
        need(self.config.options.peer_migration && self.authority_context == Some(context))?;
        if let Some(d) = decision {
            self.validate_decision(context, d)?;
        }
        if !self.peer_handoff_phase() {
            return Ok(Effects::default());
        }
        self.selection_started |= selection_started;
        if !fenced && !selection_started {
            return Ok(Effects::default());
        }
        let audio_fence = !self.peer_fenced;
        self.peer_fenced = true;
        if audio_fence {
            self.clear_media();
        }
        if selection_failed {
            self.phase = Phase::AwaitingWorld;
        } else if let Some(d) = decision {
            if d.next.peers & (1u16 << self.config.self_peer) == 0 {
                self.phase = Phase::Failed;
                self.error = "This player host was replaced by the surviving peer majority.".into();
            } else {
                self.phase = if self.config.self_peer == d.next.host {
                    Phase::Migrating
                } else {
                    Phase::Rebinding
                };
            }
        } else {
            self.phase = Phase::AwaitingWorld;
        }
        Ok(Effects {
            audio_fence,
            ..Effects::default()
        })
    }
    fn validate_decision(&self, context: Context, d: Decision) -> Result<()> {
        need(context.host < 16)?;
        let electorate = context.peers & !(1u16 << context.host);
        need(
            context.host < 16
                && d.previous == context
                && d.next.session == context.session
                && d.next.epoch == context.epoch.checked_add(1).ok_or(Error)?
                && d.next.membership_revision
                    == context.membership_revision.checked_add(1).ok_or(Error)?
                && d.next.peers == electorate
                && d.next.host == d.value.host
                && d.next.host < 16
                && electorate & (1u16 << d.next.host) != 0
                && d.ballot.round > 0
                && d.ballot.proposer < 16
                && electorate & (1u16 << d.ballot.proposer) != 0
                && d.voters == electorate
                && d.value.cut.id != 0
                && d.value.cut.bytes != 0
                && d.value.cut.bytes <= 64 * 1024 * 1024
                && d.value.cut.digest.iter().any(|b| *b != 0),
        )
    }
    pub fn commit_allowed(&self, decision: Decision) -> bool {
        self.authority_context
            .is_some_and(|c| self.validate_decision(c, decision).is_ok())
            && self.peer_handoff_phase()
            && decision.next.peers & (1u16 << self.config.self_peer) != 0
    }
    pub fn publication_session(
        &self,
        decision: Option<Decision>,
    ) -> crate::matchmaking_native_publication::Session {
        crate::matchmaking_native_publication::Session {
            self_peer: self.config.self_peer,
            handoff: self.peer_handoff_phase(),
            selection: self.config.options.survivor_selection,
            snapshots: self.config.options.snapshot_candidates,
            commit_allowed: decision.is_some_and(|d| self.commit_allowed(d)),
        }
    }
    /// Native facts are captured together under the actual Session lock.
    /// Consistent admission metadata grants no native lease or readiness.
    pub fn exchange_session(&self) -> crate::matchmaking_native_exchange::Session {
        crate::matchmaking_native_exchange::Session {
            self_peer: self.config.self_peer.into(),
            host: self.host.into(),
            direct: self.config.options.direct,
            running: self.phase == Phase::Running,
            common_phase: self.peer_plan(PeerResources::default()).common,
            candidate_phase: !matches!(self.phase, Phase::Connecting | Phase::Failed),
            terminal_recovery: self.config.options.terminal_recovery,
        }
    }
    pub fn preparation_authorization(
        &self,
        resources: crate::matchmaking_native_publication::Resources,
        admission: crate::matchmaking_native_publication::Admission,
        selection_supported: bool,
    ) -> crate::matchmaking_native_preparation::Authorization {
        crate::matchmaking_native_preparation::Authorization {
            supported: self.peer_started
                && resources.agreement_context.is_some()
                && resources.authority == resources.agreement_context
                && selection_supported,
            admitted: admission.accepted.as_ref().and_then(|(context, rows)| {
                admission
                    .continuity
                    .as_ref()
                    .is_none_or(|before| before == rows)
                    .then_some(*context)
            }),
        }
    }
    /// Called only after genuine native publication returned success under the
    /// unchanged outer generation lock; it never creates Ready or a lease.
    pub fn committed(&mut self, decision: Decision) -> Result<()> {
        need(self.commit_allowed(decision))?;
        self.authority_context = Some(decision.next);
        self.host = decision.next.host;
        let members = self
            .config
            .seats
            .iter()
            .filter(|p| decision.next.peers & (1u16 << p.owner) != 0)
            .fold(0, |m, p| m | (1u16 << p.slot));
        self.continuity = Continuity {
            epoch: decision.next.epoch,
            host: self.host,
            members,
            selected: None,
        };
        self.voice_members = members;
        for i in 0..16 {
            if members & (1u16 << i) == 0 {
                self.voice_enabled[i] = false;
            }
        }
        self.selection_started = false;
        self.peer_fenced = false;
        self.clear_media();
        self.phase = Phase::Running;
        Ok(())
    }
    pub fn selection_started(&mut self, context: Context) -> Result<()> {
        need(self.authority_context == Some(context) && self.config.options.survivor_selection)?;
        if self.peer_handoff_phase() {
            self.selection_started = true;
        }
        Ok(())
    }
    pub fn postgame_voice_enabled(&self, slot: u8) -> bool {
        self.owner(slot).is_ok_and(|owner| {
            self.voice_members & (1u16 << slot) != 0 && self.voice_enabled[owner as usize]
        })
    }
    pub fn postgame_voice(
        &mut self,
        now: u64,
        enabled: bool,
        muted: u16,
        blocked: u16,
        frames: Vec<Audio>,
        audio_ready: bool,
    ) -> Result<Vec<Audio>> {
        if !matches!(self.phase, Phase::Reviewing | Phase::Postgame) {
            return Ok(Vec::new());
        }
        let elapsed = now.checked_sub(self.voice_credit_at).ok_or(Error)? as f64 / 1000.;
        self.voice_credit_at = now;
        for credit in &mut self.voice_credits {
            *credit = 10f64.min(*credit + 50. * elapsed);
        }
        let local = self.config.self_peer;
        self.voice_enabled[local as usize] = enabled;
        self.voice_muted[local as usize] = muted;
        if now.checked_sub(self.voice_preferences_at).ok_or(Error)? > 200 {
            self.voice_preferences_at = now;
            for target in 0..16 {
                if target != local && self.physical(self.mask()) & (1u16 << target) != 0 {
                    self.voice_preferences(target, enabled, muted | blocked);
                }
            }
        }
        let mut received = Vec::new();
        for packet in self.take_voices() {
            if packet.peer == local {
                continue;
            }
            if packet.preferences {
                self.voice_enabled[packet.peer as usize] = packet.enabled;
                self.voice_muted[packet.peer as usize] = packet.muted;
            } else {
                received.push(packet.audio);
            }
        }
        for mut frame in frames {
            frame.source = local;
            let source = local as usize;
            if self.voice_members & (1u16 << local) == 0
                || frame.sequence <= self.voice_sequences[source]
                || self.voice_credits[source] < 1.
            {
                continue;
            }
            self.voice_credits[source] -= 1.;
            self.voice_sequences[source] = frame.sequence;
            for target in 0..16 {
                if target != local
                    && self.physical(self.voice_members) & (1u16 << target) != 0
                    && self.voice_enabled[source]
                    && self.voice_enabled[target as usize]
                    && self.voice_muted[source] & (1u16 << target) == 0
                    && self.voice_muted[target as usize] & (1u16 << local) == 0
                {
                    frame.left = 1.;
                    frame.right = 1.;
                    frame.radio = false;
                    self.voice(target, &frame, audio_ready);
                }
            }
        }
        Ok(received)
    }
}
