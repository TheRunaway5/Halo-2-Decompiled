//! The sole CXX schema for ONLINE parsing and peer coordination.
//! Protocol policy stays in the unsafe-forbidden protocol crate.

use halo2_online_protocol::native_requests::{NativeRequest, decode_request};
use halo2_online_protocol::{DecodeError, ParsedVoice, decode_voice};
mod eos_transport;
mod eos_sdk_ipc;
mod eos_sdk_proxy;
mod eos_auth_http;
mod eos_config;
mod steam_credentials;
mod host_agreement;
mod lan_auth;
mod netcode;
use netcode::{
    SnapshotReceiver, SnapshotSender, new_snapshot_receiver, new_snapshot_sender, snapshot_receiver_ack,
    snapshot_receiver_buffered, snapshot_receiver_commit, snapshot_receiver_push, snapshot_receiver_reject,
    snapshot_receiver_reset,
    snapshot_receiver_stats, snapshot_sender_ack, snapshot_sender_encode, snapshot_sender_stats,
};
use lan_auth::{LanEndpoint, LanSession, new_lan_endpoint, new_lan_session, lan_public, lan_bind, lan_seal, lan_open, lan_is_packet};
use halo2_online_protocol::security_policy::{steam_revocation_accepted, network_replay_call};
mod common_cut;
mod survivor_selection;
mod migration_driver;
use migration_driver::{MigrationDriver, new_migration_driver, migration_driver_step,
    migration_driver_retained, migration_driver_admit_freeze, migration_driver_admit_external_freeze,
    migration_driver_reserve_retirement, migration_driver_retired, migration_driver_blocks_publication,
    migration_driver_retirement_pending};
use survivor_selection::{
    SurvivorSelection, new_survivor_selection, survivor_selection_publish_inventory,
    survivor_selection_invalidate, survivor_selection_allows_ready,
    survivor_selection_observe_ready, survivor_selection_poll, survivor_selection_receive,
    survivor_selection_state, survivor_selection_is_packet,
    survivor_inventory_admission, survivor_retained_inventory,
};
use common_cut::{
    CommonCutAgreement, new_common_cut_agreement, common_cut_context,
    common_cut_offer, common_cut_confirm_native, common_cut_associate,
    common_cut_apply, common_cut_release, common_cut_reject, common_cut_abort,
    common_cut_acknowledge_abort, common_cut_poll, common_cut_receive,
    common_cut_state, common_cut_is_packet,
};
use host_agreement::{
    HostAgreement, new_host_agreement, host_agreement_observe_host,
    host_agreement_publish_ready, host_agreement_propose, host_agreement_drive,
    host_agreement_poll, host_agreement_receive, host_agreement_state,
    host_agreement_is_packet,
};
use steam_credentials::{steam_saved_login, steam_save_login, steam_forget_login};
use eos_config::eos_runtime_options;
use eos_auth_http::{EosAuthHttp, new_eos_auth_http, eos_auth_http_request, eos_auth_http_cancel};
use eos_transport::{EosTransport, new_eos_transport, new_eos_account_transport, eos_auth_request, eos_auth_complete, eos_auth_bound, eos_tick, eos_signal, eos_signals, eos_ready, eos_failed, eos_send, eos_receive, eos_relayed, eos_account_identity};

fn eos_process_shutdown() -> bool {
    eos_sdk_proxy::process_shutdown()
}

fn eos_sdk_register_host() -> Result<(), Box<dyn std::error::Error>> {
    eos_sdk_proxy::register_adjacent_worker()
}

/// Runs the bundled Rust SDK service on this process's main thread.
/// The dedicated worker dispatches here before creating any SDK owners.
pub fn eos_sdk_worker_main() -> i32 {
    eos_sdk_proxy::worker_main()
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
    struct EosPacketQueueInfo { result: i32, incoming_bytes: u64, incoming_packets: u64, outgoing_bytes: u64, outgoing_packets: u64 }
    struct EosIdentity { user: String, token: String }
    struct EosAuthRequest { kind: u8, ticket: u64, token: String, user: String }
    struct EosAuthResponse { kind: u8, ticket: u64, status: u16, ok: bool, token: String, user: String, expires_in: u32 }
    struct MigrationPreparationCut {
        present: bool,
        id: u64,
        tick: u64,
        bytes: u64,
        digest: [u8; 32],
    }
    struct PeerPollOrder { peers: [u8; 16] }
    struct SnapshotPacket { data: Vec<u8> }
    struct SnapshotEmission {
        emitted: bool,
        keyframe: bool,
        digest: [u8; 32],
        view_len: u32,
        packets: Vec<SnapshotPacket>,
    }
    struct SnapshotDelivery {
        complete: bool,
        failed: bool,
        sequence: u32,
        tick: u32,
        digest: [u8; 32],
        view_len: u32,
        body: Vec<u8>,
    }
    struct SnapshotAckFields { latest: u32, mask: u32, flags: u8 }
    // Per-tick metadata for one snapshot body. Viewers are local-player
    // positions (x, y, z triples); own are those players' unit objects.
    struct SnapshotFrameInfo {
        has_poses: bool,
        pose_begin: usize,
        pose_end: usize,
        tick: u32,
        sequence: u32,
        now_ms: u64,
        viewers: Vec<f32>,
        own: Vec<u32>,
        hot: Vec<u32>,
    }
    struct SnapshotSenderStats {
        messages: u64,
        wire_bytes: u64,
        packets: u64,
        keyframes: u64,
        resets: u64,
        over_budget: u64,
        full_refreshes: u64,
        root_updates: u64,
        forced_refreshes: u64,
        held_refreshes: u64,
        lost_messages: u64,
        budget_bps: u32,
        rtt_ms: u64,
        loss_percent: f32,
        retired: bool,
    }
    struct SnapshotReceiverStats { messages: u64, failures: u64, held_poses: u64, missing_poses: u64 }
    struct HostAgreementContext {
        session: [u8; 32],
        epoch: u32,
        membership_revision: u64,
        peers: u16,
        host: u8,
    }
    // Checked role metadata only. This is not an election certificate, Ready
    // value, ownership token or authorization to mutate/publish native state.
    struct ProspectiveHostRoleDescriptor {
        previous: HostAgreementContext,
        next: HostAgreementContext,
        cut: MigrationPreparationCut,
    }
    struct ProspectiveClientRoleDescriptor {
        previous: HostAgreementContext,
        cut: MigrationPreparationCut,
        self_peer: u8,
        next_epoch: u32,
        next_membership_revision: u64,
        survivors: u16,
    }
    struct HostAgreementBallot { round: u64, proposer: u8 }
    struct HostAgreementValue { host: u8, cut: MigrationPreparationCut }
    struct HostAgreementDecision {
        present: bool,
        previous: HostAgreementContext,
        next: HostAgreementContext,
        ballot: HostAgreementBallot,
        value: HostAgreementValue,
        voters: u16,
    }
    struct HostAgreementState {
        suspected: bool,
        fenced: bool,
        electorate: u16,
        silent: u16,
        majority: u32,
        ready: [MigrationPreparationCut; 16],
        decision: HostAgreementDecision,
        rejected: u64,
    }
    struct HostAgreementPacket { peer: u8, bytes: Vec<u8> }
    struct CommonCutValue {
        present: bool,
        context: HostAgreementContext,
        seal: MigrationPreparationCut,
        recipients: u16,
    }
    struct CommonCutState {
        context: HostAgreementContext,
        recipients: u16,
        committed: CommonCutValue,
        pending: CommonCutValue,
        associated: u16,
        applied: u16,
        released: u16,
        aborted: u16,
        native_all_validated: bool,
        commit: bool,
        release: bool,
        abort: bool,
        rejected: u64,
    }
    struct CommonCutPacket { peer: u8, bytes: Vec<u8> }
    struct SurvivorInventoryEntry { cut: MigrationPreparationCut, irreversible_common: bool }
    struct SurvivorInventory { lease: u64, cuts: [SurvivorInventoryEntry; 3] }
    enum SurvivorInventoryAdmission { Admit, Refuse, Invalidate }
    struct SurvivorInventoryPreparation {
        inventory: SurvivorInventory,
        admission: SurvivorInventoryAdmission,
    }
    struct SurvivorSelectionState {
        context: HostAgreementContext,
        survivors: u16,
        inventory_peers: u16,
        accepted: u16,
        selected: MigrationPreparationCut,
        transcript: [u8; 32],
        selection_complete: bool,
        failed: bool,
        ready_observed: u16,
        rejected: u64,
    }
    struct MigrationDriverObservation {
        context_present: bool, context: HostAgreementContext,
        admitted_present: bool, admitted: HostAgreementContext,
        self_peer: u8, phase: u8, supported: bool, selection_started: bool,
        agreement_present: bool, agreement: HostAgreementState,
        selection_present: bool, selection: SurvivorSelectionState,
    }
    struct MigrationDriverTicket { present: bool, context: HostAgreementContext, self_peer: u8, id: u64 }
    enum MigrationDriverActionKind { Wait, Prepare, Retire, Published, Terminal }
    enum MigrationDriverReason { None, Healthy, Unavailable, Stale, Provenance, Aborted, Published, Fenced, Exhausted, NativeFailure }
    struct MigrationDriverAction { kind: MigrationDriverActionKind, ticket: MigrationDriverTicket, reason: MigrationDriverReason }
    enum MigrationDriverFreezeKind { Publish, Retire, Refuse, Invalidate }
    struct MigrationDriverFreeze { kind: MigrationDriverFreezeKind, reason: MigrationDriverReason }
    enum MigrationDriverRetirement { Retryable, RetiredButFenced, Stale }
    unsafe extern "C++" {
        include!("src/eos_sdk.hpp");
        type SdkWorkerOutput;
        fn new_sdk_worker_output() -> Result<UniquePtr<SdkWorkerOutput>>;
        fn sdk_worker_write(self: Pin<&mut SdkWorkerOutput>, bytes: &[u8]) -> Result<()>;
        fn sdk_worker_flush(self: Pin<&mut SdkWorkerOutput>) -> Result<()>;
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
        fn sdk_send(self: Pin<&mut RawEos>, user: &str, bytes: &[u8], delayed: bool) -> i32;
        fn sdk_packet_queue_info(self: &RawEos) -> EosPacketQueueInfo;
        fn sdk_session_create(self: Pin<&mut RawEos>, session: &EosSession, bucket: &str, capacity: u32) -> Result<()>;
        fn sdk_session_invite(self: Pin<&mut RawEos>, user: &str) -> Result<()>;
        fn sdk_session_query(self: Pin<&mut RawEos>) -> Result<()>;
        fn sdk_session_register(self: Pin<&mut RawEos>, user: &str) -> Result<()>;
        fn sdk_session_info(self: &RawEos, invite: &str) -> Result<EosInviteInfo>;
        fn sdk_session_join(self: Pin<&mut RawEos>, invite: &str) -> Result<()>;
    }
    extern "Rust" {
        type LanEndpoint;
        type LanSession;
        fn new_lan_endpoint() -> Result<Box<LanEndpoint>>;
        fn new_lan_session() -> Box<LanSession>;
        fn lan_public(endpoint: &LanEndpoint) -> [u8;32];
        fn lan_bind(endpoint: &LanEndpoint, session: &mut LanSession, peer: &[u8;32]) -> Result<()>;
        fn lan_seal(endpoint: &LanEndpoint, session: &mut LanSession, host: bool, body: &[u8]) -> Result<Vec<u8>>;
        fn lan_open(endpoint: &LanEndpoint, session: &mut LanSession, host: bool, packet: &[u8]) -> Result<Vec<u8>>;
        fn lan_is_packet(packet: &[u8]) -> bool;
        type SnapshotSender;
        type SnapshotReceiver;
        fn new_snapshot_sender(chunk: usize, overhead: usize) -> Box<SnapshotSender>;
        fn snapshot_sender_encode(sender: &mut SnapshotSender, body: &[u8], floats: &[u32],
            anchor_offsets: &[u32], anchor_keys: &[u64], info: &SnapshotFrameInfo) -> Result<SnapshotEmission>;
        fn snapshot_sender_ack(sender: &mut SnapshotSender, latest: u32, mask: u32, flags: u8, now_ms: u64);
        fn snapshot_sender_stats(sender: &SnapshotSender) -> SnapshotSenderStats;
        fn new_snapshot_receiver(chunk: usize) -> Box<SnapshotReceiver>;
        fn snapshot_receiver_push(receiver: &mut SnapshotReceiver, packet: &[u8], now_ms: u64) -> SnapshotDelivery;
        fn snapshot_receiver_commit(receiver: &mut SnapshotReceiver, sequence: u32) -> bool;
        fn snapshot_receiver_reject(receiver: &mut SnapshotReceiver, sequence: u32);
        fn snapshot_receiver_ack(receiver: &SnapshotReceiver) -> SnapshotAckFields;
        fn snapshot_receiver_reset(receiver: &mut SnapshotReceiver);
        fn snapshot_receiver_stats(receiver: &SnapshotReceiver) -> SnapshotReceiverStats;
        fn snapshot_receiver_buffered(receiver: &SnapshotReceiver) -> usize;
        fn steam_revocation_accepted(status:u16, body:&str) -> bool;
        fn network_replay_call(kind:u8) -> bool;
        type MigrationDriver;
        fn new_migration_driver(self_peer:u8, context:&HostAgreementContext) -> Result<Box<MigrationDriver>>;
        fn migration_driver_step(driver:&mut MigrationDriver, observation:&MigrationDriverObservation) -> Result<MigrationDriverAction>;
        fn migration_driver_retained(driver:&mut MigrationDriver, ticket:&MigrationDriverTicket, lease:u64) -> Result<bool>;
        fn migration_driver_admit_freeze(driver:&mut MigrationDriver, observation:&MigrationDriverObservation,
            ticket:&MigrationDriverTicket, inventory:&SurvivorInventory, common_present:bool, common:&CommonCutState) -> Result<MigrationDriverFreeze>;
        fn migration_driver_admit_external_freeze(driver:&MigrationDriver, observation:&MigrationDriverObservation,
            inventory:&SurvivorInventory, common_present:bool, common:&CommonCutState) -> Result<MigrationDriverFreeze>;
        fn migration_driver_reserve_retirement(driver:&mut MigrationDriver, observation:&MigrationDriverObservation,
            ticket:&MigrationDriverTicket) -> Result<bool>;
        fn migration_driver_retired(driver:&mut MigrationDriver, observation:&MigrationDriverObservation,
            ticket:&MigrationDriverTicket) -> Result<MigrationDriverRetirement>;
        fn migration_driver_blocks_publication(driver:&MigrationDriver) -> bool;
        fn migration_driver_retirement_pending(driver:&MigrationDriver) -> bool;
        fn survivor_inventory_admission(context: &HostAgreementContext,
            inventory: &SurvivorInventory, common_present: bool,
            common: &CommonCutState) -> Result<SurvivorInventoryAdmission>;
        fn survivor_retained_inventory(context: &HostAgreementContext, lease: u64,
            retained: &[MigrationPreparationCut], common_present: bool,
            common: &CommonCutState) -> Result<SurvivorInventoryPreparation>;
        type SurvivorSelection;
        fn new_survivor_selection(self_peer: u8, context: &HostAgreementContext,
            protocol: u16) -> Result<Box<SurvivorSelection>>;
        fn survivor_selection_publish_inventory(owner: &mut SurvivorSelection,
            inventory: &SurvivorInventory) -> Result<bool>;
        fn survivor_selection_invalidate(owner: &mut SurvivorSelection,
            cut: &MigrationPreparationCut) -> Result<bool>;
        fn survivor_selection_allows_ready(owner: &SurvivorSelection, peer: u8,
            cut: &MigrationPreparationCut) -> Result<bool>;
        fn survivor_selection_observe_ready(owner: &mut SurvivorSelection, peer: u8,
            cut: &MigrationPreparationCut) -> Result<bool>;
        fn survivor_selection_poll(owner: &mut SurvivorSelection, now: u64,
            budget: u32) -> Result<Vec<CommonCutPacket>>;
        fn survivor_selection_receive(owner: &mut SurvivorSelection, peer: u8,
            packet: &[u8], now: u64) -> Result<bool>;
        fn survivor_selection_state(owner: &SurvivorSelection) -> Result<SurvivorSelectionState>;
        fn survivor_selection_is_packet(packet: &[u8]) -> bool;
        type CommonCutAgreement;
        fn new_common_cut_agreement(self_peer: u8, context: &HostAgreementContext,
            protocol: u16) -> Result<Box<CommonCutAgreement>>;
        fn common_cut_context(agreement: &CommonCutAgreement) -> Result<HostAgreementContext>;
        fn common_cut_offer(agreement: &mut CommonCutAgreement, cut: &CommonCutValue) -> Result<bool>;
        fn common_cut_confirm_native(agreement: &mut CommonCutAgreement, cut: &CommonCutValue) -> Result<bool>;
        fn common_cut_associate(agreement: &mut CommonCutAgreement, cut: &CommonCutValue) -> Result<bool>;
        fn common_cut_apply(agreement: &mut CommonCutAgreement, cut: &CommonCutValue) -> Result<bool>;
        fn common_cut_release(agreement: &mut CommonCutAgreement, cut: &CommonCutValue) -> Result<bool>;
        fn common_cut_reject(agreement: &mut CommonCutAgreement, cut: &CommonCutValue) -> Result<bool>;
        fn common_cut_abort(agreement: &mut CommonCutAgreement, cut: &CommonCutValue) -> Result<bool>;
        fn common_cut_acknowledge_abort(agreement: &mut CommonCutAgreement, cut: &CommonCutValue) -> Result<bool>;
        fn common_cut_poll(agreement: &mut CommonCutAgreement, now_ms: u64,
            packet_budget: u32) -> Result<Vec<CommonCutPacket>>;
        fn common_cut_receive(agreement: &mut CommonCutAgreement, authenticated_peer: u8,
            packet: &[u8], now_ms: u64) -> Result<bool>;
        fn common_cut_state(agreement: &CommonCutAgreement) -> Result<CommonCutState>;
        fn common_cut_is_packet(packet: &[u8]) -> bool;
        type HostAgreement;
        fn new_host_agreement(self_peer: u8, context: &HostAgreementContext,
            protocol: u16, last_host_ms: u64) -> Result<Box<HostAgreement>>;
        fn host_agreement_observe_host(agreement: &mut HostAgreement, now_ms: u64) -> Result<()>;
        fn host_agreement_publish_ready(agreement: &mut HostAgreement, peer: u8,
            context: &HostAgreementContext, cut: &MigrationPreparationCut) -> Result<bool>;
        fn host_agreement_propose(agreement: &mut HostAgreement, candidate: u8,
            now_ms: u64) -> Result<bool>;
        fn host_agreement_drive(agreement: &mut HostAgreement, candidate: u8,
            now_ms: u64) -> Result<bool>;
        fn host_agreement_poll(agreement: &mut HostAgreement, now_ms: u64,
            packet_budget: u32) -> Result<Vec<HostAgreementPacket>>;
        fn host_agreement_receive(agreement: &mut HostAgreement, authenticated_peer: u8,
            packet: &[u8], now_ms: u64) -> Result<bool>;
        fn host_agreement_state(agreement: &HostAgreement, now_ms: u64) -> Result<HostAgreementState>;
        fn host_agreement_is_packet(packet: &[u8]) -> bool;
        type PeerPollCursor;
        fn new_peer_poll_cursor() -> Box<PeerPollCursor>;
        fn peer_poll_order(cursor: &PeerPollCursor) -> PeerPollOrder;
        fn peer_record_served(cursor: &mut PeerPollCursor, peer: u8) -> bool;
        fn steam_saved_login(origin: &str, credential_namespace: &str, profile: &str) -> Result<String>;
        fn steam_save_login(origin: &str, credential_namespace: &str, profile: &str, value: &str) -> Result<()>;
        fn steam_forget_login(origin: &str, credential_namespace: &str, profile: &str) -> Result<()>;
        fn eos_runtime_options() -> Result<EosRuntimeOptions>;
        fn eos_process_shutdown() -> bool;
        fn eos_sdk_register_host() -> Result<()>;
        fn eos_sdk_worker_main() -> i32;
        type EosTransport;
        type EosAuthHttp;
        fn new_eos_auth_http(room_api: &str, bearer: &str) -> Result<Box<EosAuthHttp>>;
        fn eos_auth_http_request(client: &EosAuthHttp, request: &EosAuthRequest) -> EosAuthResponse;
        fn eos_auth_http_cancel(client: &EosAuthHttp);
        fn eos_account_identity(token: &str) -> Result<EosIdentity>;
        fn new_eos_transport(self_slot: u8, peers: Vec<EosPeer>, socket: &str, force_relay: bool, session: EosSession) -> Result<Box<EosTransport>>;
        fn new_eos_account_transport(self_slot: u8, peers: Vec<EosPeer>, socket: &str, force_relay: bool, session: EosSession) -> Result<Box<EosTransport>>;
        fn eos_auth_request(transport: &mut EosTransport) -> Result<EosAuthRequest>;
        fn eos_auth_complete(transport: &mut EosTransport, response: &EosAuthResponse) -> Result<bool>;
        fn eos_auth_bound(transport: &EosTransport) -> bool;
        fn eos_tick(transport: &mut EosTransport) -> Result<()>;
        fn eos_signal(transport: &mut EosTransport, peer: u8, user: &str) -> Result<()>;
        fn eos_signals(transport: &mut EosTransport) -> Vec<EosSignal>;
        fn eos_ready(transport: &EosTransport, peer: u8) -> bool;
        fn eos_failed(transport: &EosTransport, peer: u8) -> bool;
        fn eos_relayed(transport: &EosTransport, peer: u8) -> bool;
        fn eos_send(transport: &mut EosTransport, peer: u8, bytes: &[u8]) -> bool;
        fn eos_receive(transport: &mut EosTransport) -> EosPacket;
        fn all_survivors_prepared(
            previous_peers: u16,
            successor_peers: u16,
            old_host: u8,
            new_host: u8,
            selected: &MigrationPreparationCut,
            prepared: &[MigrationPreparationCut],
        ) -> bool;
        fn derive_prospective_host_role(
            previous: &HostAgreementContext,
            selected: &MigrationPreparationCut,
            self_peer: u8,
        ) -> Result<ProspectiveHostRoleDescriptor>;
        fn derive_prospective_client_role(previous: &HostAgreementContext,
            selected: &MigrationPreparationCut, self_peer: u8) -> Result<ProspectiveClientRoleDescriptor>;
        fn bind_prospective_client_role(role: &ProspectiveClientRoleDescriptor,
            previous: &HostAgreementContext, next: &HostAgreementContext,
            value: &HostAgreementValue) -> Result<HostAgreementContext>;
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
        ack_latest: u32,
        ack_mask: u32,
        ack_flags: u8,
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

struct PeerPollCursor {
    inner: halo2_online_protocol::peer_pacing::PeerPollCursor,
}

fn new_peer_poll_cursor() -> Box<PeerPollCursor> {
    Box::new(PeerPollCursor {
        inner: halo2_online_protocol::peer_pacing::PeerPollCursor::default(),
    })
}

fn peer_poll_order(cursor: &PeerPollCursor) -> ffi::PeerPollOrder {
    ffi::PeerPollOrder {
        peers: cursor.inner.order(),
    }
}

fn peer_record_served(cursor: &mut PeerPollCursor, peer: u8) -> bool {
    cursor.inner.record_served(peer)
}

fn preparation_cut(
    cut: &ffi::MigrationPreparationCut,
) -> Option<halo2_online_protocol::migration_preparation::PreparationCut> {
    cut.present.then_some(
        halo2_online_protocol::migration_preparation::PreparationCut {
            id: cut.id,
            tick: cut.tick,
            bytes: cut.bytes,
            digest: cut.digest,
        },
    )
}

fn derive_prospective_host_role(
    previous: &ffi::HostAgreementContext,
    selected: &ffi::MigrationPreparationCut,
    self_peer: u8,
) -> Result<ffi::ProspectiveHostRoleDescriptor, String> {
    use halo2_online_protocol::prospective_role::{self, Context};
    std::panic::catch_unwind(|| {
        let previous = Context {
            session: previous.session,
            epoch: previous.epoch,
            membership_revision: previous.membership_revision,
            peers: previous.peers,
            host: previous.host,
        };
        let selected = preparation_cut(selected)
            .ok_or_else(|| "Prospective-role selected cut is absent".to_owned())?;
        let descriptor = prospective_role::derive(previous, selected, self_peer)
            .map_err(|error| error.to_string())?;
        let context = |value: Context| ffi::HostAgreementContext {
            session: value.session,
            epoch: value.epoch,
            membership_revision: value.membership_revision,
            peers: value.peers,
            host: value.host,
        };
        Ok(ffi::ProspectiveHostRoleDescriptor {
            previous: context(descriptor.previous),
            next: context(descriptor.next),
            cut: ffi::MigrationPreparationCut {
                present: true,
                id: descriptor.cut.id,
                tick: descriptor.cut.tick,
                bytes: descriptor.cut.bytes,
                digest: descriptor.cut.digest,
            },
        })
    }).unwrap_or_else(|_| Err("Rust prospective-role metadata derivation failed".to_owned()))
}

fn prospective_context(
    value: &ffi::HostAgreementContext,
) -> halo2_online_protocol::prospective_role::Context {
    halo2_online_protocol::prospective_role::Context {
        session: value.session,
        epoch: value.epoch,
        membership_revision: value.membership_revision,
        peers: value.peers,
        host: value.host,
    }
}

fn prospective_context_to_ffi(
    value: halo2_online_protocol::prospective_role::Context,
) -> ffi::HostAgreementContext {
    ffi::HostAgreementContext {
        session: value.session,
        epoch: value.epoch,
        membership_revision: value.membership_revision,
        peers: value.peers,
        host: value.host,
    }
}

fn derive_prospective_client_role(
    previous: &ffi::HostAgreementContext,
    selected: &ffi::MigrationPreparationCut,
    self_peer: u8,
) -> Result<ffi::ProspectiveClientRoleDescriptor, String> {
    std::panic::catch_unwind(|| {
        let selected = preparation_cut(selected)
            .ok_or_else(|| "Prospective client cut is absent".to_owned())?;
        let role = halo2_online_protocol::prospective_role::derive_client(
            prospective_context(previous),
            selected,
            self_peer,
        )
        .map_err(|error| error.to_string())?;
        Ok(ffi::ProspectiveClientRoleDescriptor {
            previous: prospective_context_to_ffi(role.previous),
            cut: ffi::MigrationPreparationCut {
                present: true,
                id: role.cut.id,
                tick: role.cut.tick,
                bytes: role.cut.bytes,
                digest: role.cut.digest,
            },
            self_peer: role.self_peer,
            next_epoch: role.next_epoch,
            next_membership_revision: role.next_membership_revision,
            survivors: role.survivors,
        })
    })
    .unwrap_or_else(|_| Err("Rust prospective client derivation failed".to_owned()))
}

fn bind_prospective_client_role(
    role: &ffi::ProspectiveClientRoleDescriptor,
    previous: &ffi::HostAgreementContext,
    next: &ffi::HostAgreementContext,
    value: &ffi::HostAgreementValue,
) -> Result<ffi::HostAgreementContext, String> {
    use halo2_online_protocol::{host_agreement, prospective_role};
    std::panic::catch_unwind(|| {
        let role = prospective_role::ClientDescriptor {
            previous: prospective_context(&role.previous),
            cut: preparation_cut(&role.cut)
                .ok_or_else(|| "Prospective client cut is absent".to_owned())?,
            self_peer: role.self_peer,
            next_epoch: role.next_epoch,
            next_membership_revision: role.next_membership_revision,
            survivors: role.survivors,
        };
        let value = host_agreement::Value {
            host: value.host,
            cut: preparation_cut(&value.cut)
                .ok_or_else(|| "Certificate endpoint cut is absent".to_owned())?,
        };
        prospective_role::bind_client(
            role,
            prospective_context(previous),
            prospective_context(next),
            value,
        )
        .map(prospective_context_to_ffi)
        .map_err(|error| error.to_string())
    })
    .unwrap_or_else(|_| Err("Rust prospective client endpoint binding failed".to_owned()))
}
fn all_survivors_prepared(
    previous_peers: u16,
    successor_peers: u16,
    old_host: u8,
    new_host: u8,
    selected: &ffi::MigrationPreparationCut,
    prepared: &[ffi::MigrationPreparationCut],
) -> bool {
    use halo2_online_protocol::migration_preparation::{self, PHYSICAL_PEERS};
    if prepared.len() != PHYSICAL_PEERS {
        return false;
    }
    let Some(selected) = preparation_cut(selected) else {
        return false;
    };
    let mut cuts = [None; PHYSICAL_PEERS];
    for (target, value) in cuts.iter_mut().zip(prepared) {
        *target = preparation_cut(value);
    }
    migration_preparation::all_survivors_prepared(
        previous_peers,
        successor_peers,
        old_host,
        new_host,
        &selected,
        &cuts,
    )
    .is_ok()
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
        ack_latest: 0,
        ack_mask: 0,
        ack_flags: 0,
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
            ack,
        } => {
            frame.kind = 2;
            frame.nonce = nonce;
            frame.cookie = cookie;
            frame.sequence = sequence;
            frame.ack_latest = ack.latest;
            frame.ack_mask = ack.mask;
            frame.ack_flags = ack.flags;
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

#[cfg(test)]
mod prospective_role_tests {
    use super::*;

    fn context() -> ffi::HostAgreementContext {
        ffi::HostAgreementContext {
            session: [0x81; 32], epoch: u32::MAX - 1,
            membership_revision: u64::MAX - 1, peers: u16::MAX, host: 15,
        }
    }
    fn selected() -> ffi::MigrationPreparationCut {
        ffi::MigrationPreparationCut {
            present: true, id: u64::MAX, tick: 0x9abc_def0_1234_5678,
            bytes: 64 * 1024 * 1024, digest: [0xa5; 32],
        }
    }

    #[test]
    fn prospective_descriptor_preserves_full_width_typed_values_without_aliases() {
        let mut previous = context();let mut cut = selected();
        let value = derive_prospective_host_role(&previous, &cut, 0).unwrap();
        assert_eq!(value.previous.session, previous.session);
        assert_eq!(value.previous.epoch, previous.epoch);
        assert_eq!(value.previous.membership_revision, previous.membership_revision);
        assert_eq!(value.previous.peers, u16::MAX);
        assert_eq!(value.previous.host, 15);
        assert_eq!(value.next.session, previous.session);
        assert_eq!(value.next.epoch, u32::MAX);
        assert_eq!(value.next.membership_revision, u64::MAX);
        assert_eq!(value.next.peers, 0x7fff);
        assert_eq!(value.next.host, 0);
        assert!(value.cut.present);
        assert_eq!(value.cut.id, u64::MAX);
        assert_eq!(value.cut.tick, cut.tick);
        assert_eq!(value.cut.bytes, cut.bytes);
        assert_eq!(value.cut.digest, cut.digest);
        previous.session.fill(0);cut.digest.fill(0);
        assert_eq!(value.previous.session, [0x81; 32]);
        assert_eq!(value.cut.digest, [0xa5; 32]);
    }

    #[test]
    fn prospective_descriptor_rejects_absence_overflow_invalid_context_and_cut() {
        for peer in [15, 16, 255] {
            assert!(derive_prospective_host_role(&context(), &selected(), peer).is_err());
        }
        for field in 0..6 {
            let mut previous = context();
            match field {
                0 => previous.epoch = u32::MAX,
                1 => previous.membership_revision = u64::MAX,
                2 => previous.host = 255,
                3 => previous.session = [0; 32],
                4 => previous.peers = 1 << 15,
                _ => previous.peers = 0x7fff,
            }
            assert!(derive_prospective_host_role(&previous, &selected(), 0).is_err());
        }
        for field in 0..5 {
            let mut cut = selected();
            match field {
                0 => cut.present = false,
                1 => cut.id = 0,
                2 => cut.bytes = 0,
                3 => cut.bytes += 1,
                _ => cut.digest = [0; 32],
            }
            assert!(derive_prospective_host_role(&context(), &cut, 0).is_err());
        }
    }

    fn client_next() -> ffi::HostAgreementContext {
        derive_prospective_host_role(&context(), &selected(), 1)
            .unwrap()
            .next
    }
    fn client_value() -> ffi::HostAgreementValue {
        ffi::HostAgreementValue {
            host: 1,
            cut: selected(),
        }
    }

    #[test]
    fn role_neutral_client_metadata_preserves_full_width_and_binds_actual_endpoint_fields() {
        let mut previous = context();
        let mut cut = selected();
        let role = derive_prospective_client_role(&previous, &cut, 0).unwrap();
        assert_eq!(role.previous.session, previous.session);
        assert_eq!(role.previous.epoch, u32::MAX - 1);
        assert_eq!(role.previous.membership_revision, u64::MAX - 1);
        assert_eq!(role.previous.peers, u16::MAX);
        assert_eq!(role.previous.host, 15);
        assert!(role.cut.present);
        assert_eq!(
            (role.cut.id, role.cut.tick, role.cut.bytes, role.cut.digest),
            (cut.id, cut.tick, cut.bytes, cut.digest)
        );
        assert_eq!(
            (
                role.self_peer,
                role.next_epoch,
                role.next_membership_revision,
                role.survivors
            ),
            (0, u32::MAX, u64::MAX, 0x7fff)
        );
        let endpoint =
            bind_prospective_client_role(&role, &previous, &client_next(), &client_value())
                .unwrap();
        assert_eq!(endpoint.session, previous.session);
        assert_eq!(endpoint.epoch, u32::MAX);
        assert_eq!(endpoint.membership_revision, u64::MAX);
        assert_eq!((endpoint.peers, endpoint.host), (0x7fff, 1));
        previous.session.fill(0);
        cut.digest.fill(0);
        assert_eq!(role.previous.session, [0x81; 32]);
        assert_eq!(role.cut.digest, [0xa5; 32]);
    }

    #[test]
    fn client_metadata_rejects_absence_no_other_survivor_and_changed_late_fields() {
        for peer in [15, 16, 255] {
            assert!(derive_prospective_client_role(&context(), &selected(), peer).is_err());
        }
        let mut single = context();
        single.peers = 0x8001;
        assert!(derive_prospective_host_role(&single, &selected(), 0).is_ok());
        assert!(derive_prospective_client_role(&single, &selected(), 0).is_err());
        for field in 0..6 {
            let mut previous = context();
            let mut cut = selected();
            match field {
                0 => previous.epoch = u32::MAX,
                1 => previous.membership_revision = u64::MAX,
                2 => cut.present = false,
                3 => cut.bytes = 0,
                4 => cut.bytes += 1,
                _ => cut.digest = [0; 32],
            }
            assert!(derive_prospective_client_role(&previous, &cut, 0).is_err());
        }
        for field in 0..12 {
            let mut role = derive_prospective_client_role(&context(), &selected(), 0).unwrap();
            let mut previous = context();
            let mut next = client_next();
            let mut value = client_value();
            match field {
                0 => role.cut.present = false,
                1 => role.self_peer = 255,
                2 => role.next_epoch -= 1,
                3 => role.next_membership_revision -= 1,
                4 => role.survivors ^= 2,
                5 => value.cut.present = false,
                6 => next.host = 0,
                7 => next.peers = 1,
                8 => previous.session[0] ^= 1,
                9 => value.host = 15,
                10 => next.epoch -= 1,
                _ => next.membership_revision -= 1,
            }
            assert!(bind_prospective_client_role(&role, &previous, &next, &value).is_err());
        }
    }

}
