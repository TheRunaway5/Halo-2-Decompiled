//! The sole CXX schema for the ONLINE parser bridge.
//! Application parsing stays in the unsafe-forbidden protocol crate.

use halo2_online_protocol::native_requests::{NativeRequest, decode_request};
use halo2_online_protocol::{DecodeError, ParsedVoice, decode_voice};
mod eos_transport;
mod eos_config;
mod steam_credentials;
use steam_credentials::{steam_saved_login, steam_save_login, steam_forget_login};
use eos_config::eos_runtime_options;
use eos_transport::{EosTransport, new_eos_transport, eos_tick, eos_signal, eos_signals, eos_ready, eos_failed, eos_send, eos_receive, eos_relayed, eos_account_identity};

fn eos_process_shutdown() -> bool {
    ffi::sdk_process_shutdown()
}

// CXX generates the ABI shims. No handwritten pointer conversion or unsafe
// operation belongs here. The exception to deny(unsafe_code) is this schema.
#[allow(unsafe_code)]
#[cxx::bridge(namespace = "halo2::online_protocol_bridge")]
mod ffi {
    struct EosRuntimeOptions { transport: bool, profiles: bool }
    struct EosConfig {
        product: String,
        sandbox: String,
        deployment: String,
        client: String,
        secret: String,
        cache: String,
    }
    struct EosSession {
        enabled: bool,
        playlist: String,
        revision: String,
        map: String,
        variant: String,
    }
    struct EosInviteInfo {
        session: String,
        owner: String,
        playlist: String,
        revision: String,
        map: String,
        variant: String,
    }
    struct EosEvent { kind: u8, code: i32, user: String, socket: String, data: Vec<u8> }
    struct EosPeer { slot: u8, key: String }
    struct EosSignal { peer: u8, user: String }
    struct EosPacket { present: bool, peer: u8, data: Vec<u8> }
    struct EosIdentity { user: String, token: String }
    unsafe extern "C++" {
        include!("src/eos_sdk.hpp");
        type RawEos;
        fn eos_sdk_available() -> bool;
        fn sdk_process_shutdown() -> bool;
        fn new_raw_eos(config: &EosConfig) -> Result<UniquePtr<RawEos>>;
        fn sdk_device(self: Pin<&mut RawEos>) -> Result<()>;
        fn sdk_login(self: Pin<&mut RawEos>) -> Result<()>;
        fn sdk_external_login(self: Pin<&mut RawEos>, token: &str) -> Result<()>;
        fn sdk_id_token(self: &RawEos) -> Result<String>;
        fn sdk_create_user(self: Pin<&mut RawEos>) -> Result<()>;
        fn sdk_socket(self: Pin<&mut RawEos>, name: &str, force_relay: bool) -> Result<()>;
        fn sdk_poll(self: Pin<&mut RawEos>) -> Result<Vec<EosEvent>>;
        fn sdk_accept(self: Pin<&mut RawEos>, user: &str) -> bool;
        fn sdk_send(self: Pin<&mut RawEos>, user: &str, bytes: &[u8], delayed: bool) -> bool;
        fn sdk_session_create(self: Pin<&mut RawEos>, session: &EosSession, capacity: u32) -> Result<()>;
        fn sdk_session_invite(self: Pin<&mut RawEos>, user: &str) -> Result<()>;
        fn sdk_session_query(self: Pin<&mut RawEos>) -> Result<()>;
        fn sdk_session_register(self: Pin<&mut RawEos>, user: &str) -> Result<()>;
        fn sdk_session_info(self: &RawEos, invite: &str) -> Result<EosInviteInfo>;
        fn sdk_session_join(self: Pin<&mut RawEos>, invite: &str) -> Result<()>;
    }
    extern "Rust" {
        fn steam_saved_login(origin: &str, credential_namespace: &str, profile: &str) -> Result<String>;
        fn steam_save_login(origin: &str, credential_namespace: &str, profile: &str, value: &str) -> Result<()>;
        fn steam_forget_login(origin: &str, credential_namespace: &str, profile: &str) -> Result<()>;
        fn eos_runtime_options() -> Result<EosRuntimeOptions>;
        fn eos_process_shutdown() -> bool;
        type EosTransport;
        fn eos_account_identity(token: &str) -> Result<EosIdentity>;
        fn new_eos_transport(self_slot: u8, peers: Vec<EosPeer>, socket: &str, force_relay: bool, session: EosSession) -> Result<Box<EosTransport>>;
        fn eos_tick(transport: &mut EosTransport) -> Result<()>;
        fn eos_signal(transport: &mut EosTransport, peer: u8, user: &str) -> Result<()>;
        fn eos_signals(transport: &mut EosTransport) -> Vec<EosSignal>;
        fn eos_ready(transport: &EosTransport, peer: u8) -> bool;
        fn eos_failed(transport: &EosTransport, peer: u8) -> bool;
        fn eos_relayed(transport: &EosTransport, peer: u8) -> bool;
        fn eos_send(transport: &mut EosTransport, peer: u8, bytes: &[u8]) -> bool;
        fn eos_receive(transport: &mut EosTransport) -> EosPacket;
    }
    struct ArchiveLimits {
        encoded_bytes: u64,
        decoded_bytes: u64,
        allocated_bytes: u64,
        container_elements: u64,
        fixed_native_bytes: u64,
    }
    struct ArchiveNode {
        valid: bool,
        tag: u8,
        bits: u64,
        count: u64,
        next: usize,
    }
    struct ArchiveStats {
        valid: bool,
        raw_bytes: u64,
        nodes: usize,
        metadata_bytes: u64,
        deepest: usize,
        working_limit: u64,
        working_reserved: u64,
        working_peak: u64,
        native_claimed: u64,
    }
    enum DecodeStatus {
        Ok,
        TooShort,
        TooLarge,
        WrongMagic,
        WrongProtocol,
        WrongKind,
        InvalidSubtype,
        InvalidLength,
        InvalidSource,
        InvalidBoolean,
        InvalidGain,
        InvalidCount,
        InvalidControl,
        InvalidIntent,
        InternalFailure,
    }

    struct VoiceFrame {
        status: DecodeStatus,
        subtype: u8,
        source: u8,
        enabled: bool,
        muted: u16,
        sequence: u32,
        radio: bool,
        left: f32,
        right: f32,
        opus: Vec<u8>,
    }

    struct NativePlayer {
        team: i8,
        has_profile: bool,
        profile: Vec<u8>,
    }
    struct NativeIntent {
        present: bool,
        action: i16,
        index: i16,
        object: u32,
    }
    struct NativeControl {
        forward: f32,
        right: f32,
        yaw: f32,
        pitch: f32,
        flags: u16,
        zoom: i16,
        zoom_reset_serial: u32,
        pickup_milliseconds: u16,
        dual_wield_milliseconds: u16,
        completed: [u16; 2],
        presses: [u32; 10],
        vehicle: NativeIntent,
        completed_vehicle: NativeIntent,
    }
    struct NativeRequestFrame {
        status: DecodeStatus,
        kind: u8,
        nonce: u64,
        fingerprint: u64,
        cookie: u64,
        sequence: u32,
        players: Vec<NativePlayer>,
        controls: Vec<NativeControl>,
    }

    extern "Rust" {
        type Archive;
        fn parse_authority_archive(packet: &[u8], limits: ArchiveLimits) -> Result<Box<Archive>>;
        fn archive_node(self: &Archive, index: usize) -> ArchiveNode;
        fn archive_name(self: &Archive, index: usize) -> &[u8];
        fn archive_data(self: &Archive, index: usize) -> &[u8];
        fn archive_raw(self: &Archive) -> &[u8];
        fn archive_hash(self: &Archive) -> [u8; 32];
        fn archive_len(self: &Archive) -> usize;
        fn archive_claim(self: &Archive, count: u64, extent: u64, elements: bool) -> bool;
        fn archive_stats(self: &Archive) -> ArchiveStats;
        fn decode_voice_frame(packet: &[u8], expected_protocol: u16) -> VoiceFrame;
        fn decode_native_request_frame(packet: &[u8], expected_protocol: u16)
        -> NativeRequestFrame;
    }
}

struct Archive {
    inner: halo2_online_protocol::authority_archive::Archive,
}
impl Archive {
    fn archive_node(&self, index: usize) -> ffi::ArchiveNode {
        self.inner.node(index).map_or(
            ffi::ArchiveNode {
                valid: false,
                tag: 0,
                bits: 0,
                count: 0,
                next: 0,
            },
            |n| ffi::ArchiveNode {
                valid: true,
                tag: n.tag,
                bits: n.bits,
                count: n.count,
                next: n.next,
            },
        )
    }
    fn archive_name(&self, index: usize) -> &[u8] {
        self.inner.name(index)
    }
    fn archive_data(&self, index: usize) -> &[u8] {
        self.inner.data(index)
    }
    fn archive_raw(&self) -> &[u8] {
        self.inner.raw()
    }
    fn archive_hash(&self) -> [u8; 32] {
        self.inner.hash()
    }
    fn archive_len(&self) -> usize {
        self.inner.len()
    }
    fn archive_claim(&self, count: u64, extent: u64, elements: bool) -> bool {
        self.inner.claim_native(count, extent, elements)
    }
    fn archive_stats(&self) -> ffi::ArchiveStats {
        let Some(s) = self.inner.stats() else {
            return ffi::ArchiveStats {
                valid: false,
                raw_bytes: 0,
                nodes: 0,
                metadata_bytes: 0,
                deepest: 0,
                working_limit: 0,
                working_reserved: 0,
                working_peak: 0,
                native_claimed: 0,
            };
        };
        ffi::ArchiveStats {
            valid: true,
            raw_bytes: s.raw_bytes,
            nodes: s.nodes,
            metadata_bytes: s.metadata_bytes,
            deepest: s.deepest,
            working_limit: s.working_limit,
            working_reserved: s.working_reserved,
            working_peak: s.working_peak,
            native_claimed: s.native_claimed,
        }
    }
}
fn parse_authority_archive(
    packet: &[u8],
    limits: ffi::ArchiveLimits,
) -> Result<Box<Archive>, String> {
    std::panic::catch_unwind(|| {
        let limits = halo2_online_protocol::authority_archive::Limits {
            encoded_bytes: limits.encoded_bytes,
            decoded_bytes: limits.decoded_bytes,
            allocated_bytes: limits.allocated_bytes,
            container_elements: limits.container_elements,
            fixed_native_bytes: limits.fixed_native_bytes,
        };
        halo2_online_protocol::authority_archive::decode(packet, limits)
            .map(|inner| Box::new(Archive { inner }))
            .map_err(|e| e.to_string())
    })
    .unwrap_or_else(|_| Err("Authority archive parser failed".to_owned()))
}

fn empty(status: ffi::DecodeStatus) -> ffi::VoiceFrame {
    ffi::VoiceFrame {
        status,
        subtype: 0,
        source: 0,
        enabled: false,
        muted: 0,
        sequence: 0,
        radio: false,
        left: 0.0,
        right: 0.0,
        opus: Vec::new(),
    }
}

fn parse(packet: &[u8], expected_protocol: u16) -> ffi::VoiceFrame {
    let mut frame = empty(ffi::DecodeStatus::Ok);
    match decode_voice(packet, expected_protocol) {
        Ok(ParsedVoice::Preferences { enabled, muted }) => {
            frame.subtype = 1;
            frame.enabled = enabled;
            frame.muted = muted;
        }
        Ok(ParsedVoice::ForwardedPreferences {
            source,
            enabled,
            muted,
        }) => {
            frame.subtype = 2;
            frame.source = source;
            frame.enabled = enabled;
            frame.muted = muted;
        }
        Ok(ParsedVoice::Audio {
            source,
            sequence,
            radio,
            left,
            right,
            opus,
        }) => {
            frame.source = source;
            frame.sequence = sequence;
            frame.radio = radio;
            frame.left = left;
            frame.right = right;
            frame.opus = opus;
        }
        Err(error) => {
            frame.status = status(error);
        }
    }
    frame
}

fn decode_voice_frame(packet: &[u8], expected_protocol: u16) -> ffi::VoiceFrame {
    // Malformed input returns a typed status. Unexpected Rust unwinding must
    // also end here, before the generated extern shim. OOM abort is separate.
    std::panic::catch_unwind(|| parse(packet, expected_protocol))
        .unwrap_or_else(|_| empty(ffi::DecodeStatus::InternalFailure))
}

fn status(error: DecodeError) -> ffi::DecodeStatus {
    match error {
        DecodeError::TooShort => ffi::DecodeStatus::TooShort,
        DecodeError::TooLarge => ffi::DecodeStatus::TooLarge,
        DecodeError::WrongMagic => ffi::DecodeStatus::WrongMagic,
        DecodeError::WrongProtocol => ffi::DecodeStatus::WrongProtocol,
        DecodeError::WrongKind => ffi::DecodeStatus::WrongKind,
        DecodeError::InvalidSubtype => ffi::DecodeStatus::InvalidSubtype,
        DecodeError::InvalidLength => ffi::DecodeStatus::InvalidLength,
        DecodeError::InvalidSource => ffi::DecodeStatus::InvalidSource,
        DecodeError::InvalidBoolean => ffi::DecodeStatus::InvalidBoolean,
        DecodeError::InvalidGain => ffi::DecodeStatus::InvalidGain,
        DecodeError::InvalidCount => ffi::DecodeStatus::InvalidCount,
        DecodeError::InvalidControl => ffi::DecodeStatus::InvalidControl,
        DecodeError::InvalidIntent => ffi::DecodeStatus::InvalidIntent,
    }
}

fn empty_request(status: ffi::DecodeStatus) -> ffi::NativeRequestFrame {
    ffi::NativeRequestFrame {
        status,
        kind: 0,
        nonce: 0,
        fingerprint: 0,
        cookie: 0,
        sequence: 0,
        players: Vec::new(),
        controls: Vec::new(),
    }
}

fn intent(
    value: Option<halo2_online_protocol::native_requests::VehicleIntent>,
) -> ffi::NativeIntent {
    value.map_or(
        ffi::NativeIntent {
            present: false,
            action: 0,
            index: 0,
            object: 0,
        },
        |v| ffi::NativeIntent {
            present: true,
            action: v.action,
            index: v.index,
            object: v.object,
        },
    )
}

fn parse_request(packet: &[u8], protocol: u16) -> ffi::NativeRequestFrame {
    let request = match decode_request(packet, protocol) {
        Ok(value) => value,
        Err(error) => return empty_request(status(error)),
    };
    let mut frame = empty_request(ffi::DecodeStatus::Ok);
    match request {
        NativeRequest::Hello {
            nonce,
            fingerprint,
            players,
        } => {
            frame.kind = 1;
            frame.nonce = nonce;
            frame.fingerprint = fingerprint;
            frame.players = players
                .into_iter()
                .map(|player| ffi::NativePlayer {
                    team: player.team,
                    has_profile: player.profile.is_some(),
                    profile: player.profile.map_or_else(Vec::new, |p| p.to_vec()),
                })
                .collect();
        }
        NativeRequest::Input {
            nonce,
            cookie,
            sequence,
            controls,
        } => {
            frame.kind = 2;
            frame.nonce = nonce;
            frame.cookie = cookie;
            frame.sequence = sequence;
            frame.controls = controls
                .into_iter()
                .map(|c| ffi::NativeControl {
                    forward: c.forward,
                    right: c.right,
                    yaw: c.yaw,
                    pitch: c.pitch,
                    flags: c.flags,
                    zoom: c.zoom,
                    zoom_reset_serial: c.zoom_reset_serial,
                    pickup_milliseconds: c.pickup_milliseconds,
                    dual_wield_milliseconds: c.dual_wield_milliseconds,
                    completed: c.completed,
                    presses: c.presses,
                    vehicle: intent(c.vehicle),
                    completed_vehicle: intent(c.completed_vehicle),
                })
                .collect();
        }
        NativeRequest::Leave { nonce, cookie } => {
            frame.kind = 4;
            frame.nonce = nonce;
            frame.cookie = cookie;
        }
        NativeRequest::SessionQuery { nonce } => {
            frame.kind = 6;
            frame.nonce = nonce;
        }
    }
    frame
}

fn decode_native_request_frame(packet: &[u8], expected_protocol: u16) -> ffi::NativeRequestFrame {
    std::panic::catch_unwind(|| parse_request(packet, expected_protocol))
        .unwrap_or_else(|_| empty_request(ffi::DecodeStatus::InternalFailure))
}
