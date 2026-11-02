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
mod peer_mesh_quality;
mod peer_source_admissions;
use peer_source_admissions::*;
mod peer_authority_transfer;
use peer_authority_transfer::*;
mod peer_snapshot_candidate;
use peer_snapshot_candidate::*;
use peer_mesh_quality::{PeerMeshQuality,new_peer_mesh_quality,peer_mesh_quality_context,
    peer_mesh_quality_poll,peer_mesh_quality_receive,peer_mesh_quality_matrix,
    peer_mesh_quality_local,peer_mesh_quality_candidates,peer_mesh_quality_is_packet};
mod lan_auth;
mod netcode;
mod matchmaking_readiness;
mod matchmaking_client;
mod matchmaking_native_qos;
mod matchmaking_native_session;
use matchmaking_native_session::*;
mod matchmaking_native_peers;
use matchmaking_native_peers::*;
mod matchmaking_native_publication;
mod matchmaking_native_exchange;
mod matchmaking_native_receive;
mod matchmaking_native_access;
use matchmaking_native_access::*;
mod matchmaking_native_migration;
use matchmaking_native_migration::*;
mod matchmaking_native_api;
use matchmaking_native_api::*;
use matchmaking_native_receive::native_receive_dispatch;
use matchmaking_native_exchange::{native_exchange_session,native_exchange_source_publish,
    native_exchange_source_visible,native_exchange_authority_publish,native_exchange_capability,
    native_exchange_common,native_exchange_common_view,native_exchange_candidates};
use matchmaking_native_publication::{native_publication_session,native_publication_abort,
    native_publication_progress,native_publication_fence,native_publication_prepare,
    native_publication_propose,native_publication_propose_automatic,native_publication_finish};
mod matchmaking_native_preparation;
use matchmaking_native_preparation::*;
use matchmaking_native_qos::{MatchmakingQos, new_matchmaking_qos, matchmaking_qos_run};
use matchmaking_client::{MatchmakingClient, new_matchmaking_client, matchmaking_client_configured,
    matchmaking_client_current, matchmaking_client_active, matchmaking_client_command, matchmaking_client_snapshot,
    matchmaking_client_poll, matchmaking_client_shutdown, matchmaking_client_reject_content,
    matchmaking_client_speaker_binding, matchmaking_client_take_launch, matchmaking_client_runtime_compatibility};
mod matchmaking_account_client;
use matchmaking_account_client::*;
mod matchmaking_native_results;
use matchmaking_native_results::*;
mod matchmaking_frontend_lobby;
use matchmaking_frontend_lobby::*;
mod matchmaking_frontend_search;
use matchmaking_frontend_search::*;
mod matchmaking_frontend_progress;
use matchmaking_frontend_progress::*;
mod matchmaking_frontend_presentation;
use matchmaking_frontend_presentation::*;
mod matchmaking_frontend_party;
use matchmaking_frontend_party::*;
mod matchmaking_frontend_party_flow;
use matchmaking_frontend_party_flow::*;
mod matchmaking_frontend_party_view;
use matchmaking_frontend_party_view::*;
mod matchmaking_frontend_lobby_query;
use matchmaking_frontend_lobby_query::*;
mod matchmaking_frontend_account_flow;
use matchmaking_frontend_account_flow::*;
mod matchmaking_frontend_friends;
use matchmaking_frontend_friends::*;
mod matchmaking_frontend_lobby_view;
use matchmaking_frontend_lobby_view::*;
use matchmaking_readiness::{matchmaking_connected_teams, matchmaking_eligibility,
    matchmaking_error_dialog, matchmaking_error_clear, matchmaking_native_launch_ready};
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
use halo2_online_protocol::matchmaking_steam_request::{account_http_response_limit, account_http_timeout_ms};
use halo2_online_protocol::matchmaking_variant::valid as matchmaking_variant_valid;
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
    struct NativeSessionConfig {
        origin: MatchmakingQosConfig, room:String, self_peer:u32, seats:Vec<NativeSessionSeat>,
        team_present:bool, team_mode:bool, protocol:u16, extension_floor:u32,
        direct:bool, continuity:bool, peer_migration:bool, snapshot_candidates:bool,
        survivor_selection:bool, terminal_recovery:bool, eos:bool, eos_peers:Vec<u32>, now:u64,
    }
    struct NativeSessionSeat {slot:u32,peer:u32,guest:bool,team:i32,name:String,identity:[u8;8]}
    struct NativeSessionSeal {present:bool,epoch:u32,tick:u64,digest:String}
    struct NativeSessionView {
        session:[u8;32],phase:u8,host:u8,error:String,epoch:u32,members:u16,continuity_host:u8,selected:NativeSessionSeal,
        voice_members:u16,voice_blocked:u16,departed:u16,recorded_present:bool,recorded:bool,
        presence_present:bool,presence:u16,fingerprint_present:bool,fingerprint:u64,
        audio_generation:u64,peer_started:bool,peer_fenced:bool,selection_started:bool,
        common_phase:bool,native_control_phase:bool,peer_handoff_phase:bool,sync_audio:bool,
        quality_present:bool,quality:HostAgreementContext,
    }
    struct NativeSessionResources {authority_present:bool,authority:HostAgreementContext,quality_present:bool,quality:HostAgreementContext,agreement_present:bool,agreement:HostAgreementContext}
    struct NativeSessionSignal {peer:u32,kind:String,payload:String,mid:String}
    struct NativeSessionEffects {
        initialize:bool,context:HostAgreementContext,common:bool,snapshot:bool,agreement:bool,selection:bool,
        quality_present:bool,quality:HostAgreementContext,observe_host:bool,audio_fence:bool,audio_stop:bool,
        signal_present:bool,signal:NativeSessionSignal,reply:String,
    }
    struct NativeSessionParameter {id:u32,value:i32}
    struct NativeSessionPacket {peer:u32,bytes:Vec<u8>,epoch:u32,generation:u64}
    struct NativeSessionAudio {source:u32,sequence:u32,radio:bool,left:f32,right:f32,opus:Vec<u8>,signature:Vec<u8>,generation:u64}
    struct NativeApiAudioBinding {existing:bool,same:bool}
    struct NativeSessionVoice {peer:u32,preferences:bool,enabled:bool,muted:u16,audio:NativeSessionAudio}
    struct NativeSessionFlush {loaded_present:bool,loaded:String,completion_present:bool,completion:String,confirmation_present:bool,confirmation:String,commands:Vec<String>,packets:Vec<NativeSessionPacket>}
    struct NativeSessionLink {peer:u32,measured:bool,rtt:u32,upload:u32,loss:u16}
    struct NativeSessionDirect {now:u64,authenticated:bool,ready:u16,failed:u16,links:Vec<NativeSessionLink>,authority_present:bool,authority:HostAgreementContext,admitted_present:bool,admitted:HostAgreementContext}
    struct NativeSessionDirectEffects {terminal_present:bool,terminal:HostAgreementContext,failed:bool,effects:NativeSessionEffects}
    struct NativeSessionFence {context:HostAgreementContext,fenced:bool,decision:HostAgreementDecision,selection_started:bool,selection_failed:bool}
    struct NativeMigrationBinding {present:bool,context:NativePublicationContext}
    struct NativeMigrationSelection {present:bool,complete:bool,failed:bool}
    struct NativeMigrationCandidate {present:bool,peer:u32}
    struct NativeSessionCandidate {
        agreement_present:bool,context:HostAgreementContext,agreement:HostAgreementState,
        quality_present:bool,quality:HostAgreementContext,ranked:Vec<u32>,
        selection_supported:bool,selection_complete:bool,selection_failed:bool,
    }
    enum NativeSessionSendKind {Text,Binary,Pong,Close}
    enum NativePeerPoll {Authority,Admissions,Common,Selection,Snapshot,Quality,Agreement}
    struct NativePeerResources {
        authority_present:bool,authority:HostAgreementContext,
        common_present:bool,common:HostAgreementContext,
        selection_present:bool,selection:HostAgreementContext,
        snapshot_present:bool,snapshot:HostAgreementContext,features:[u32;16],
    }
    struct NativePeerPlan {
        common_supported:bool,selection_supported:bool,snapshot_supported:bool,
        native_control:bool,common:bool,handoff:bool,failed:bool,setup:bool,
    }
    struct NativePeerSample {now:u64,authenticated:bool,setup:bool,links:Vec<NativeSessionLink>}
    struct NativePeerDatagram {present:bool,peer:u32,epoch:u32,bytes:Vec<u8>}
    struct NativePublicationContext {session:[u8;32],epoch:u32,membership_revision:u64,peers:u16,host:u32}
    struct NativePublicationDecision {present:bool,previous:NativePublicationContext,next:NativePublicationContext,round:u64,proposer:u32,host:u32,cut:MigrationPreparationCut,voters:u16}
    struct NativePublicationSession {self_peer:u32,handoff:bool,selection:bool,snapshots:bool,commit_allowed:bool}
    struct NativePublicationResources {agreement_present:bool,agreement_context_present:bool,agreement_context:NativePublicationContext,authority_present:bool,authority:NativePublicationContext,source_present:bool,history:u32}
    struct NativePublicationSelection {present:bool,started:bool,supported:bool,complete:bool,failed:bool,survivors:u16,allows_ready:bool}
    struct NativePublicationAgreement {present:bool,state:HostAgreementState}
    struct NativePublicationCommon {abort:bool,pending:bool,context:NativePublicationContext,cut:MigrationPreparationCut}
    struct NativePublicationAdmission {present:bool,context:NativePublicationContext,players:Vec<SourceAdmissionsPlayer>,continuity_present:bool,continuity:Vec<SourceAdmissionsPlayer>}
    struct NativePublicationAudio {present:bool,ready:bool}
    struct NativePublicationSnapshot {present:bool,pending:bool,retired:u16}
    struct NativePublicationSuccessor {multiple:bool,selection:bool,snapshot:bool,selection_feature:bool,snapshot_feature:bool,limits:AuthorityTransferLimits}
    struct NativePreparationTicket {present:bool,context:NativePublicationContext,self_peer:u32,id:u64}
    enum NativePreparationKind {Prepare,Retire}
    enum NativePreparationOutcome {Inactive,Preparing,Retiring,Published,Retryable,Terminal,Refused}
    struct NativePreparationStep {outcome:NativePreparationOutcome,kind:NativePreparationKind,ticket:NativePreparationTicket}
    struct NativePreparationSelection {present:bool,context:NativePublicationContext,started:bool,failed:bool}
    struct NativePreparationAuthorization {supported:bool,admitted_present:bool,admitted:NativePublicationContext}
    struct NativeSessionPresence {present:bool,members:u16}
    struct NativeSessionContinuity {present:bool,epoch:u32,host:u32,members:u16,selected:NativeSessionSeal}
    struct NativePostgamePreferences {enabled:bool,muted:u16,blocked:u16}
    struct NativeReceiveSession {common:bool,native_control:bool,handoff:bool,direct:bool,epochs:bool}
    enum NativeReceiveTarget {Selection,Snapshot,Common,Agreement,Quality,Admissions,Authority}
    struct NativeExchangeSession {self_peer:u32,host:u32,direct:bool,running:bool,common_phase:bool,candidate_phase:bool,terminal_recovery:bool}
    struct NativeExchangeCommon {present:bool,supported:bool,context:NativePublicationContext}
    enum NativeExchangeCommonAction {Offer,ConfirmNative,Associate,Apply,Release,Reject,Abort,AcknowledgeAbort}
    enum NativeExchangeCapability {Authority,Replay,Pipeline,HostContinuation,AdmittedCamera,SourceConfirmed}
    struct MatchmakingQosConfig {
        input_valid: bool, origin: String, bearer: String, loopback: bool,
        url_parsed: bool, url_normalized: String, url_scheme: String, url_host: String,
        url_user: String, url_password: String, url_options: String, url_path: String,
    }
    struct MatchmakingQosOpen { url: String, bearer: String, connect_ms: u32, timeout_ms: u32 }
    struct MatchmakingQosFrame {
        again: bool, text: bool, binary: bool, ping: bool, pong: bool, close: bool, continued: bool,
        bytes_left: i64, data: Vec<u8>,
    }
    struct MatchmakingQosSent { again: bool, count: usize }
    enum MatchmakingQosSendKind { Text, Pong }
    enum MatchmakingClientStatus { Unconfigured, Idle, Connecting, Ready, Joining, Searching, CheckingConnections, AwaitingTransport, Loading, Failed }
    enum MatchmakingClientCommandKind { Refresh, Join, Cancel, SignOut }
    enum MatchmakingClientPhase { Missing, Connecting, Loading, Failed, Other }
    struct MatchmakingClientConfig {
        input_valid: bool,
        endpoint: String, url_parsed: bool, url_normalized: String, url_scheme: String, url_host: String,
        url_user: String, url_password: String, url_options: String, url_path: String,
        maps: Vec<String>, compatibility: String, language: u32, poll_ms: i64, loopback: bool,
        login_present: bool, terminal_recovery: bool, force_relay: bool,
    }
    struct MatchmakingClientLocal { name: String, signed_in: bool, appearance: Vec<u8> }
    struct MatchmakingClientCommand { kind: MatchmakingClientCommandKind, input_valid: bool, name: String, playlist: String, appearance: Vec<u8>, locals: Vec<MatchmakingClientLocal> }
    struct MatchmakingClientPlaylist {
        id: String, revision: String, name: String, description: String, ranked: bool, quickmatch: bool,
        eligible: bool, minimum: u32, maximum: u32, level_present: bool, level: u32, clan_match: bool,
    }
    struct MatchmakingClientPlayer { name: String, team: u32, is_self: bool, level_present: bool, level: u32, appearance: Vec<u8> }
    struct MatchmakingClientSnapshot {
        status: MatchmakingClientStatus, playlists: Vec<MatchmakingClientPlaylist>, playlist_id: String, message: String,
        queued_players: u32, revision: u64, start_supported: bool, start_present: bool, start_ms: u32,
        failure_present: bool, failure_state: i32, game_present: bool, map: String, variant: String, game_id: String,
        settings: Vec<u8>, players: Vec<MatchmakingClientPlayer>, gathering_present: bool,
        gathering_players: Vec<MatchmakingClientPlayer>, minimum: u32, maximum: u32, wait_present: bool, wait_ms: u32,
    }
    struct MatchmakingClientGrant { present: bool, authorization: String, steam: bool }
    struct MatchmakingClientIdentity { primary_present: bool, primary: String, locals: Vec<MatchmakingClientGrant> }
    struct MatchmakingClientRequest { version: u64, method: String, url: String, body: String, bearer: String, cleanup: bool }
    struct MatchmakingClientResponse { status: i64, body: String }
    struct AccountClientIdentity {present:bool,bearer:String,subject:String}
    struct AccountClientPartyCommand {action:String,value:String,playlist:String,revision:String,version:u64,privacy_present:bool,privacy:u32,privacy_revision:u64,privacy_context:String,join_context:String}
    struct AccountClientVoice {present:bool,origin:String,bearer:String,channel:String,player:String,loopback:bool,party:bool}
    struct AccountClientLink {present:bool,party_id:String,join_secret:String,version:u64,current_size:u32,max_size:u32,expires_ns:i64}
    struct AccountClientOperation {serial:u64,pending:bool,successful:bool,link:AccountClientLink}
    struct AccountClientPostgameMember {is_self:bool,leader:bool,postgame_present:bool,postgame:bool}
    struct FrontendLobbyMember {handle:String,ready_playlist:String}
    struct SearchMember {handle:String,is_self:bool,ready:String}
    struct SearchFrame {screen:u32,context:u32,party_screen:bool,first_present:bool,first_destination:i32}
    struct SearchContext {mode:i32,clock:u32,couch_revision:u64,frames:Vec<SearchFrame>,party_present:bool,leader:bool,members:Vec<SearchMember>,queue:bool,account:bool}
    struct SearchPlaylist {id:String,revision:String,eligible:bool,quickmatch:bool,clan_match:bool}
    struct SearchSnapshot {status:u32,playlists:Vec<SearchPlaylist>,matched:bool,gathering:bool,revision:u64,message_empty:bool,countdown_supported:bool,countdown_present:bool,countdown_ms:u32,failure_present:bool,failure:i32}
    struct SearchAccount {present:bool,leader:bool,members:Vec<SearchMember>,busy:bool,message:Vec<u8>}
    enum SearchUiKind {ClearSnapshot,NoService,Failure,ClearFailure,ModeOnline,PopLobbyOrOnline,PopLobby,OpenLobby,EnterLobby,OpenSearch,Dialog,Revision,RepopulateLobby,PopTop,Sound,Render,Unavailable,PlaylistError}
    struct SearchUi {kind:SearchUiKind,number:u32,text:Vec<u8>}
    struct SearchVariant {present:bool,teams:bool}
    struct SearchTake {present:bool,teams_present:bool,teams:bool}
    struct SearchCounts {profiles:u32,slots:u32}
    struct SearchFailure {format:bool,text:Vec<u8>}
    struct SearchName {bytes:Vec<u8>}
    struct ProgressKey {team:i32,value:i32}
    struct ProgressCountdown {seconds:i32,ready:bool}
    struct ProgressTitle {xbox:u32,pc:u32}
    struct PartyBytesData {value:Vec<u8>}
    struct PartyAppearanceData {bytes:[u8;9]}
    struct PartyAuthData {present:bool,authorized:bool}
    struct PartySeatData {controller:u32,name:Vec<u8>,appearance:[u8;9]}
    struct PartyLocalData {name:Vec<u8>,guest:bool,appearance_present:bool,appearance:[u8;9]}
    struct PartyMemberData {handle:Vec<u8>,name:Vec<u8>,ready:Vec<u8>,is_self:bool,leader:bool,online:bool,locals:Vec<PartyLocalData>,controller:i32,guest:bool,appearance_present:bool,appearance:[u8;9],postgame_present:bool,postgame:bool}
    struct PartyVoiceData {party:bool,joined:bool,transmitting:bool,muted:Vec<PartyBytesData>,talking:Vec<PartyBytesData>}
    struct PartyMembershipData {handle:Vec<u8>,local_count:u64}
    struct PartyNotificationData {authenticated:bool,busy:bool,party:bool,members:Vec<PartyMembershipData>,invitations:Vec<PartyBytesData>,message_empty:bool}
    struct PartyInviteData {id:Vec<u8>,name:Vec<u8>}
    struct PartyNoticeFactsData {authenticated:bool,signed_in:Vec<u8>,invitations:Vec<PartyInviteData>,dismissed:Vec<PartyBytesData>,busy:bool,party:bool,launch:bool,status:u32}
    struct PartyNoticeData {present:bool,id:Vec<u8>,name:Vec<u8>,busy:bool,can_accept:bool}
    struct PartyFlowEntry {destination:i32,value:i32}
    struct PartyFlowFrame {screen:u32,context:u32,instance:u64,selected:u32,entries:Vec<PartyFlowEntry>}
    struct PartyFlowPrivacy {mode:u32,revision:u64,context:Vec<u8>}
    struct PartyFlowInvite {id:Vec<u8>,name:Vec<u8>,version:u64}
    struct PartyFlowFriend {id:Vec<u8>,state:Vec<u8>}
    struct PartyFlowFacts {mode:i32,frames:Vec<PartyFlowFrame>,account_client:bool,queue_client:bool,voice_client:bool,authenticated:bool,busy:bool,party:bool,leader:bool,version:u64,privacy_present:bool,privacy:PartyFlowPrivacy,outgoing:Vec<PartyBytesData>,invitations:Vec<PartyFlowInvite>,friends:Vec<PartyFlowFriend>,clan:bool,clan_members:Vec<PartyBytesData>,signed_in:Vec<u8>,launch:bool,status:u32,party_message:Vec<u8>,command_serial:u64,successful_present:bool,successful:bool,voice_party:bool,voice_joined:bool,voice_connecting:bool,microphone_muted:bool,muted:Vec<PartyBytesData>}
    struct PartyFlowCommand {action:Vec<u8>,value:Vec<u8>,playlist:Vec<u8>,revision:Vec<u8>,version:u64,privacy_present:bool,privacy:u32,privacy_revision:u64,privacy_context:Vec<u8>,join_context:Vec<u8>}
    struct PartyFlowState {friend_target:Vec<u8>,activity_target:Vec<u8>,friend_join_serial:u64,account_name:Vec<u8>,voice_party:bool,voice_wanted:bool,target:Vec<u8>,kind:u32,version:u64,tab:u32,dismissed:Vec<PartyBytesData>,pending:bool,frame_instance:u64,command_serial:u64,revision:u64,mode:u32,context:Vec<u8>}
    struct FriendsFlowFriend {id:Vec<u8>,name:Vec<u8>,tag:Vec<u8>,state:Vec<u8>,presence:Vec<u8>,joinable:LobbyViewBool,context:Vec<u8>}
    struct FriendsFlowAccount {authenticated:bool,busy:bool,party:bool,leader:bool,available:bool,tag:Vec<u8>,message:Vec<u8>,friends:Vec<FriendsFlowFriend>,search_present:bool,search:FriendsFlowFriend}
    struct FriendsFlowSocialFriend {handle:Vec<u8>,name:Vec<u8>,kind:i32,presence:i32}
    struct FriendsFlowInvite {handle:Vec<u8>,name:Vec<u8>,status:Vec<u8>}
    struct FriendsFlowSocial {available:bool,enabled:bool,connecting:bool,ready:bool,busy:bool,message:Vec<u8>,revision:u64,friends:Vec<FriendsFlowSocialFriend>,invitations:Vec<FriendsFlowInvite>}
    struct FriendsFlowFacts {account_client:bool,social_client:bool,login_client:bool,account:FriendsFlowAccount,social:FriendsFlowSocial,frame:AccountFlowFrame,frames:Vec<AccountFlowFrame>,launch:bool,status:u32}
    struct FriendsFlowEffect {kind:u32,index:u64,a:u32,b:u32,text:Vec<u8>,entry:AccountFlowEntry,entries:Vec<AccountFlowEntry>}
    struct FriendsFlowResult {handled:bool,text:Vec<u8>}
    struct PartyFlowUi {kind:u32,a:u32,b:u32,flag:bool,text:Vec<u8>}
    struct PresentationPlaylist {id:Vec<u8>,name:Vec<u8>,description:Vec<u8>,eligible:bool,ranked:bool,clan:bool,minimum:u32,maximum:u32,level_present:bool,level:u32}
    struct PresentationFacts {screen:u32,selected_row:u32,selected_id:Vec<u8>,playlists:Vec<PresentationPlaylist>,status:u32,ui:i32,pane:u32,title:i32,countdown:i32,pc_progress:bool,
        matched_present:bool,matched:u64,gathering_present:bool,gathering:u64,minimum:u32,queued:u32,variant:Vec<u8>,map:Vec<u8>,message:Vec<u8>,party:bool,leader:bool,party_message:Vec<u8>}
    struct ProgressMember {key:ProgressKey,level_present:bool,level:u32,is_self:bool,local_controller:i32,handle:String}
    struct ProgressFacts {pane:u32,ui:i32,status:u32,ranked:bool,teams:bool,party_present:bool,match_present:bool,gathering_present:bool,
        party:Vec<ProgressMember>,local:Vec<ProgressMember>,matched:Vec<ProgressMember>,gathering:Vec<ProgressMember>,maximum:u32,
        viewer_level_present:bool,viewer_level:u32,party_voice:bool,voice_joined:bool,microphone_muted:bool,transmitting:bool,talking:Vec<String>}
    struct ProgressRow {present:bool,kind:u8,source:u8,index:u32,rank:i32,speaker:i32}
    struct ProgressTip {present:bool,index:u32,seed:u32}
    struct SearchChoice {kind:u32,index:u32}
    struct SearchState {active:bool,quick:bool,consent:bool,selected:String,name:Vec<u8>,couch_revision:u64,lobby_pending:bool,create_submitted:bool,countdown:i32,have_fun:bool,pane:u32,title:i32,pane_clock:u32,party:Vec<String>}
    struct FrontendLobbyPlaylist {id:String,revision:String,eligible:bool}
    struct FrontendLobbyFacts {mode:i32,screens:Vec<u32>,account_client:bool,authenticated:bool,busy:bool,party_message_present:bool,
        queue_client:bool,selected_playlist:String,playlists:Vec<FrontendLobbyPlaylist>,party_present:bool,leader:bool,
        countdown_supported:bool,countdown_present:bool,serial:u64,seconds_present:bool,seconds:i32,complete:bool,members:Vec<FrontendLobbyMember>}
    struct FrontendLobbyObservation {revision:bool,beep:bool,departure:bool,search:bool}
    struct FrontendLobbyInputEffects {handled:bool,sound:bool,bounded_sound:bool,error:String,confirm_leave:bool,friends:bool,refresh_account:bool,begin_leave:bool}
    struct FrontendLobbyView {matchmaking:bool,seconds:i32,stage:i32,consumed:u64,counted:Vec<String>,leave_pending:bool}
    struct NativeResultPlayer {connected:bool,membership:i32,total_score:i32,kills:i32,deaths:i32,assists:i32,suicides:i32}
    struct NativeResultState {finished:bool,results_finalized:bool,game_over_countdown:i32,team_mode:bool,retained_teams:u8,
        players:Vec<NativeResultPlayer>,team_scores:Vec<i32>,accuracy_present:bool,accuracy:Vec<u16>}
    struct NativeResultAdmission {slot:u32,team:i32,identity:Vec<u8>}
    struct NativeResultLivePlayer {connected:bool,datum:u32,profile_present:bool,membership:i32}
    struct NativeResultRecording {statistics_present:bool,enabled:u8,frozen:u8}
    struct NativeResultRanks {players:Vec<u8>,teams:Vec<u8>}
    struct NativeResultPublicationPlayer {occupied:u8,identity:Vec<u8>,configuration:Vec<u8>,score:i32,placement:i32}
    struct NativeResultPublicationTeam {occupied:u8,score:i32,placement:i32}
    struct NativeResultPublication {variant:Vec<u8>,players:Vec<NativeResultPublicationPlayer>,teams:Vec<NativeResultPublicationTeam>,current:u8,finalized:u8,engine_present:bool}

    struct MatchmakingClientConnection { phase: MatchmakingClientPhase, error: String }
    struct MatchmakingClientSeat { slot: u32, name: String, team: u32, identity: [u8;8], peer: u32, guest: bool, level_present: bool, level: u32 }
    struct MatchmakingClientAdmission {
        endpoint: String, room: String, bearer: String, current: u32, seats: Vec<MatchmakingClientSeat>,
        loopback: bool, team_present: bool, team_mode: bool, direct: bool, stun: Vec<String>, terminal_recovery: bool,
        eos_present: bool, eos_socket: String, eos_peers: Vec<EosPeer>, eos_account_login: bool,
        eos_playlist: String, eos_revision: String, eos_map: String, eos_variant: String, eos_force_relay: bool,
    }
    struct MatchmakingSeat { occupied: bool, peer: u32, team: i32 }
    struct MatchmakingEligibility { ready: bool, defer: bool }
    struct MatchmakingErrorDialog { present: bool, id: u32 }
    struct MatchmakingLaunchObservation {
        session_present: bool, transferred: bool, loading: bool,
        start_supported: bool, start_present: bool, start_ms: u32,
        team_mode_present: bool, team_mode: bool, current_peer: u32,
    }
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
    struct MeshQualityContext {session:[u8;32],epoch:u32,membership_revision:u64,peers:u16,host:u32}
    struct SourceAdmissionsContext {session:[u8;32],epoch:u32,membership_revision:u64,peers:u16,host:u32}
    struct SourceAdmissionsSeat {slot:u32,peer:u32,guest:bool,account:[u8;8]}
    struct SourceAdmissionsPlayer {slot:u32,player:u32,identity:[u8;12]}
    struct SourceAdmissionsAccepted {present:bool,context:SourceAdmissionsContext,digest:[u8;32],players:Vec<SourceAdmissionsPlayer>}
    struct SourceAdmissionsPacket {peer:u32,bytes:Vec<u8>}
    struct AuthorityTransferLimits {maximum_bytes:u64,bytes_per_second:u64,burst_bytes:u64,retry_ms:u64,offer_ms:u64,idle_ms:u64,lifetime_ms:u64,window_chunks:u32,features:u32}
    struct AuthorityTransferFeatures {context:SourceAdmissionsContext,features:[u32;16]}
    struct AuthorityTransferReceived {present:bool,peer:u32,context:SourceAdmissionsContext,seal:MigrationPreparationCut,bytes:Vec<u8>}
    struct AuthorityTransferReceipt {peer:u32,context:SourceAdmissionsContext,seal:MigrationPreparationCut}
    struct NativeSnapshotToken {present:bool,context:SourceAdmissionsContext,recipients:u16,nonce:u64}
    struct NativeSnapshotLimits {maximum_archive_bytes:u64,retry_ms:u64,idle_ms:u64,window_chunks:u32}
    struct NativeSnapshotArchive {peer:u32,bytes:UniquePtr<NativeSnapshotBuffer>}
    struct NativeSnapshotWitness {present:bool,owner_matches:bool,recipient:u32,token:NativeSnapshotToken,bytes_present:bool}
    struct NativeSnapshotState {context:SourceAdmissionsContext,recipients:u16,pending:NativeSnapshotToken,armed:u16,delivered:u16,captured:u16,retired:u16,rejected:u16,closing_present:bool,closing:u32,published:bool,delivery_outstanding:bool,stalled:bool,buffered_bytes:u64,maximum_buffered_bytes:u64,rejected_packets:u64}
    struct MeshQualityLink {rtt_ms:u32,upload_bytes_per_second:u32,loss_per_mille:u16,measured:bool}
    struct MeshQualityRow {present:bool,links:[MeshQualityLink;16],received_ms:u64}
    struct MeshQualityMatrix {rows:[MeshQualityRow;16]}
    struct MeshQualityHost {peer:u32,mean_rtt_ms:u32,minimum_upload_bytes_per_second:u32,worst_loss_per_mille:u16}
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
    struct LobbyViewBool {present:bool,value:bool}
    struct LobbyViewI32 {present:bool,value:i32}
    struct LobbyViewU32 {present:bool,value:u32}
    struct LobbyViewBytes {present:bool,value:Vec<u8>}
    struct LobbyViewSession {session_class:i32,live_subtype:i32,campaign_id:i32,variant:LobbyViewI32,settings_valid:bool}
    struct LobbyViewInput {
        session:i32,previous:i32,host:bool,team_mode:bool,local_team_selection:bool,
        countdown:bool,start_permission:bool,update_guard:bool,status:i32,delayed_player:i32,
        countdown_cancelled:bool,loading_map:bool,editing_options:bool,team_count:i32,
        player_count:i32,max_players:i32,overflow_status:u32,privacy:i32,prior_stats:i32,
    }
    struct LobbyViewPresentation {
        pane:i32,buttons_present:bool,button0:bool,button1:bool,leader_text:LobbyViewU32,
        leader_visible:LobbyViewBool,start_eligible:LobbyViewBool,status:LobbyViewU32,
        status_warning:bool,subtitle:u32,privacy_text:LobbyViewU32,privacy_visible:LobbyViewBool,
        privacy:LobbyViewU32,prior_stats_text:LobbyViewU32,prior_stats_visible:LobbyViewBool,
    }
    struct LobbyViewHelp {text:LobbyViewU32,visible:bool,value:LobbyViewU32,string_id:u32}
    struct LobbyViewCategory {present:bool,rows:Vec<u32>}
    struct LobbyViewChoice {label:Vec<u8>,value:i32}
    struct LobbyViewParameter {present:bool,parameter:u32,title:Vec<u8>,choices:Vec<LobbyViewChoice>}
    struct PartyViewNamed {id:Vec<u8>,name:Vec<u8>}
    struct PartyViewFriend {name:Vec<u8>,state:Vec<u8>,presence:Vec<u8>}
    struct PartyViewClanMember {name:Vec<u8>,is_self:bool,presence:u32}
    struct PartyViewMember {handle:Vec<u8>,name:Vec<u8>,is_self:bool,leader:bool,controller:i32}
    struct PartyViewFacts {
        screen:u32,context:u32,description:Vec<u8>,party:bool,leader:bool,countdown_supported:bool,
        members:Vec<PartyViewMember>,outgoing:Vec<PartyViewNamed>,invitations:Vec<PartyViewNamed>,
        friends:Vec<PartyViewFriend>,clan:bool,clan_role:u32,clan_members:Vec<PartyViewClanMember>,
        friends_message:Vec<u8>,party_message:Vec<u8>,account_message:Vec<u8>,voice_client:bool,
        voice_party:bool,voice_joined:bool,voice_connecting:bool,microphone_muted:bool,muted:Vec<PartyBytesData>,
    }
    struct LobbyQueryMap {scenario:Vec<u8>,campaign_id:i32,counts:Vec<u8>}
    struct LobbyQueryMember {handle:Vec<u8>,name:Vec<u8>,is_self:bool,leader:bool,appearance:bool}
    struct LobbyQuerySystemPlayer {name:Vec<u8>,team:i32,session_index:i32}
    struct LobbyQueryEdit {valid:bool,team:i16}
    struct LobbyQueryEdits {v0:LobbyQueryEdit,v1:LobbyQueryEdit,v2:LobbyQueryEdit,v3:LobbyQueryEdit}
    struct LobbyQueryIndices {v0:i32,v1:i32,v2:i32,v3:i32}
    struct LobbyQueryFacts {
        mode:i32,party:bool,leader:bool,members:Vec<LobbyQueryMember>,requester:Vec<u8>,privacy:LobbyViewU32,
        voice_party:bool,joined:bool,transmitting:bool,microphone_muted:bool,talking:Vec<PartyBytesData>,muted:Vec<PartyBytesData>,
        engine:u32,difficulty:u32,level:u32,map_index:u32,protocol:i32,profile:bool,team:i32,
        edit:LobbyQueryEdit,edits:LobbyQueryEdits,local_indices:LobbyQueryIndices,countdown:LobbyViewI32,system_host:LobbyViewBool,
        system_machines:u32,system_players:Vec<LobbyQuerySystemPlayer>,campaign_maps:Vec<LobbyQueryMap>,multiplayer_maps:Vec<LobbyQueryMap>,
    }
    struct LobbyQueryValue {parameter:u32,value:i32}
    struct LobbyQueryRow {system:bool,source:u64,session_index:i32,name:Vec<u8>,appearance:bool}
    struct LobbyQueryView {
        present:bool,input:LobbyViewInput,protocol:i32,machines:i32,engine:u32,difficulty:u32,
        map_kind:u32,map_index:u64,profile:bool,team:i32,edit:LobbyQueryEdit,edits:LobbyQueryEdits,local_indices:LobbyQueryIndices,
        team_count:u32,allow_none:bool,quick:bool,names_present:bool,names:Vec<PartyBytesData>,speakers:Vec<u32>,rows:Vec<LobbyQueryRow>,
    }
    struct AccountFlowAccount {revision:u64,notification:PartyNotificationData,friends:Vec<PartyBytesData>,clan:bool,clan_members:Vec<PartyBytesData>,friend_search:bool,command_serial:u64,successful:LobbyViewBool,party_message:Vec<u8>}
    struct AccountFlowEntry {label:Vec<u8>,destination:i32,value:i32}
    struct AccountFlowFrame {screen:u32,context:u32,selected:u32,pcr:bool,entries:Vec<AccountFlowEntry>}
    struct AccountFlowFacts {account_client:bool,login_client:bool,voice_client:bool,queue_client:bool,signed_in:Vec<u8>,tags:Vec<PartyViewNamed>,account:AccountFlowAccount,voice_revision:u64,frames:Vec<AccountFlowFrame>,launch:bool,mode:i32,sounds:u64}
    struct AccountFlowEffect {kind:u32,index:u64,a:u32,b:u32,text:Vec<u8>}
    struct PartyViewLayout {original:bool,buttons:u64}
    struct PartyViewSnapshot {present:bool,input:LobbyViewInput,protocol:i32,machines:i32}
    struct PartyViewEntry {label:Vec<u8>,destination:i32,value:i32}
    struct PartyViewEffect {kind:u32,text:Vec<u8>,entry:PartyViewEntry,entries:Vec<PartyViewEntry>}
    unsafe extern "C++" {
        include!("src/matchmaking_frontend_lobby_query.hpp");
        type NativeFrontendLobbyQueryIo;
        fn lobby_query_facts(self:&NativeFrontendLobbyQueryIo)->Result<LobbyQueryFacts>;
        fn lobby_query_start_error(self:&NativeFrontendLobbyQueryIo)->Result<Vec<u8>>;
        fn lobby_query_seconds(self:&NativeFrontendLobbyQueryIo)->Result<i32>;
        fn lobby_query_members(self:&NativeFrontendLobbyQueryIo)->Result<Vec<LobbyQueryMember>>;
        fn lobby_query_locals(self:&NativeFrontendLobbyQueryIo)->Result<Vec<i32>>;
        fn lobby_query_has_stats(self:&NativeFrontendLobbyQueryIo)->Result<bool>;
        fn lobby_query_prior_count(self:&NativeFrontendLobbyQueryIo)->Result<i32>;
        fn lobby_query_campaign_file(self:&NativeFrontendLobbyQueryIo,level:u32)->Result<Vec<u8>>;
        fn lobby_query_profile_option(self:&NativeFrontendLobbyQueryIo,screen:u32,parameter:u32)->Result<i32>;
        fn lobby_query_variant(self:&NativeFrontendLobbyQueryIo)->Result<()>;
        fn lobby_query_defusal(self:&NativeFrontendLobbyQueryIo)->Result<bool>;
        fn lobby_query_value(self:&NativeFrontendLobbyQueryIo,parameter:u32)->Result<i32>;
        fn lobby_query_appearance(self:&NativeFrontendLobbyQueryIo)->Result<()>;
        fn lobby_query_custom(self:&NativeFrontendLobbyQueryIo)->Result<bool>;
        fn lobby_query_capacity(self:&NativeFrontendLobbyQueryIo,parameter:u32)->Result<i32>;
        fn lobby_query_quick(self:&NativeFrontendLobbyQueryIo,engine:u32,values:&[LobbyQueryValue],screen:u32,separator:u32)->Result<()>;
    }
    unsafe extern "C++" {
        include!("src/matchmaking_frontend_account_flow.hpp");
        type NativeFrontendAccountFlowIo;
        fn account_flow_facts(self:&NativeFrontendAccountFlowIo)->Result<AccountFlowFacts>;
        fn account_flow_identity(self:&NativeFrontendAccountFlowIo)->Result<LobbyViewBytes>;
        fn account_flow_profile(self:&NativeFrontendAccountFlowIo,name:&[u8],tag:&[u8])->Result<()>;
        fn account_flow_connect(self:&NativeFrontendAccountFlowIo,name:&[u8])->Result<()>;
        fn account_flow_sign_out(self:&NativeFrontendAccountFlowIo)->Result<()>;
        fn account_flow_snapshot(self:&NativeFrontendAccountFlowIo)->Result<AccountFlowAccount>;
        fn account_flow_stage_voice(self:&NativeFrontendAccountFlowIo,read:bool)->Result<u64>;
        fn account_flow_native_friends(self:&NativeFrontendAccountFlowIo)->Result<bool>;
        fn account_flow_effect(self:&NativeFrontendAccountFlowIo,effect:&AccountFlowEffect)->Result<()>;
    }
    unsafe extern "C++" {
        include!("src/matchmaking_frontend_friends.hpp");
        type NativeFrontendFriendsIo;
        fn friends_flow_facts(self:&NativeFrontendFriendsIo)->Result<FriendsFlowFacts>;
        fn friends_flow_steam(self:&NativeFrontendFriendsIo)->Result<bool>;
        fn friends_flow_identity(self:&NativeFrontendFriendsIo)->Result<bool>;
        fn friends_flow_pane(self:&NativeFrontendFriendsIo)->Result<()>;
        fn friends_flow_localized(self:&NativeFrontendFriendsIo,index:u32)->Result<Vec<u8>>;
        fn friends_flow_profile_name(self:&NativeFrontendFriendsIo)->Result<Vec<u8>>;
        fn friends_flow_social_snapshot(self:&NativeFrontendFriendsIo)->Result<FriendsFlowSocial>;
        fn friends_flow_friend_command(self:&NativeFrontendFriendsIo,action:&[u8],target:&[u8])->Result<bool>;
        fn friends_flow_party_command(self:&NativeFrontendFriendsIo,command:&PartyFlowCommand)->Result<bool>;
        fn friends_flow_wrapped_party_command(self:&NativeFrontendFriendsIo,command:&PartyFlowCommand)->Result<()>;
        fn friends_flow_serial(self:&NativeFrontendFriendsIo)->Result<u64>;
        fn friends_flow_update_account(self:&NativeFrontendFriendsIo)->Result<()>;
        fn friends_flow_social_command(self:&NativeFrontendFriendsIo,kind:u32,target:&[u8],action:&[u8])->Result<bool>;
        fn friends_flow_effect(self:&NativeFrontendFriendsIo,effect:&FriendsFlowEffect)->Result<()>;
    }
    unsafe extern "C++" {
        include!("src/matchmaking_native_preparation.hpp");
        type NativePreparationIo;
        fn preparation_driver_present(self:&NativePreparationIo)->bool;
        fn preparation_owner_matches(self:&NativePreparationIo)->bool;
        fn preparation_observe(self:&NativePreparationIo)->Result<()>;
        fn preparation_step(self:&NativePreparationIo)->Result<MigrationDriverAction>;
        fn preparation_reserve(self:&NativePreparationIo,ticket:&NativePreparationTicket)->Result<bool>;
        fn preparation_retained(self:&NativePreparationIo,ticket:&NativePreparationTicket,lease:u64)->Result<bool>;
        fn preparation_retired(self:&NativePreparationIo,ticket:&NativePreparationTicket)->Result<MigrationDriverRetirement>;
        fn preparation_admit(self:&NativePreparationIo,ticket:&NativePreparationTicket)->Result<MigrationDriverFreezeKind>;
        fn preparation_admit_external(self:&NativePreparationIo)->Result<MigrationDriverFreezeKind>;
        fn preparation_selection(self:&NativePreparationIo)->Result<NativePreparationSelection>;
        fn preparation_publish(self:&NativePreparationIo)->Result<bool>;
        fn preparation_invalidate(self:&NativePreparationIo,cut:&MigrationPreparationCut)->Result<bool>;
        fn preparation_abort(self:&NativePreparationIo)->Result<()>;
        fn preparation_progress(self:&NativePreparationIo)->Result<()>;
        fn preparation_begin(self:&NativePreparationIo)->Result<()>;
        fn preparation_terminal(self:&NativePreparationIo)->Result<()>;
        fn preparation_retirement_pending(self:&NativePreparationIo)->bool;
        include!("src/matchmaking_native_publication.hpp");
        include!("src/matchmaking_native_exchange.hpp");
        include!("src/matchmaking_native_access.hpp");
        include!("src/matchmaking_native_migration.hpp");
        include!("src/matchmaking_native_api.hpp");
        include!("src/matchmaking_account_client.hpp");
        include!("src/matchmaking_native_results.hpp");
        include!("src/matchmaking_frontend_search.hpp");
        include!("src/matchmaking_frontend_presentation.hpp");
        include!("src/matchmaking_frontend_party.hpp");
        include!("src/matchmaking_frontend_party_flow.hpp");
        include!("src/matchmaking_frontend_party_view.hpp");
        type NativeFrontendPartyViewIo;
        fn party_view_facts(self:&NativeFrontendPartyViewIo)->Result<PartyViewFacts>;
        fn party_view_layout(self:&NativeFrontendPartyViewIo,screen:u32,pane:u32,direct:bool,theme:bool)->Result<PartyViewLayout>;
        fn party_view_read(self:&NativeFrontendPartyViewIo,original:bool,kind:u32,index:u32)->Result<Vec<u8>>;
        fn party_view_members(self:&NativeFrontendPartyViewIo)->Result<Vec<PartyViewMember>>;
        fn party_view_seconds(self:&NativeFrontendPartyViewIo)->Result<i32>;
        fn party_view_matchmaking(self:&NativeFrontendPartyViewIo)->Result<bool>;
        fn party_view_selected(self:&NativeFrontendPartyViewIo)->Result<Vec<u8>>;
        fn party_view_snapshot(self:&NativeFrontendPartyViewIo)->Result<PartyViewSnapshot>;
        fn party_view_has_stats(self:&NativeFrontendPartyViewIo)->Result<bool>;
        fn party_view_start_error(self:&NativeFrontendPartyViewIo)->Result<Vec<u8>>;
        fn party_view_expand(self:&NativeFrontendPartyViewIo,text:&[u8])->Result<Vec<u8>>;
        fn party_view_effect(self:&NativeFrontendPartyViewIo,effect:&PartyViewEffect)->Result<()>;
        fn party_view_entries_empty(self:&NativeFrontendPartyViewIo)->Result<bool>;

        include!("src/matchmaking_frontend_lobby_view.hpp");
        type NativeFrontendLobbyViewIo;
        fn lobby_view_category(self:&NativeFrontendLobbyViewIo,id:u32)->Result<LobbyViewCategory>;
        fn lobby_view_parameter(self:&NativeFrontendLobbyViewIo,tag:u32)->Result<LobbyViewParameter>;
        fn lobby_view_value(self:&NativeFrontendLobbyViewIo,parameter:u32)->Result<LobbyViewI32>;
        fn lobby_view_default(self:&NativeFrontendLobbyViewIo,engine:u32,parameter:u32)->Result<LobbyViewI32>;
        type NativeFrontendPartyFlowIo;
        fn party_flow_facts(self:&NativeFrontendPartyFlowIo)->Result<PartyFlowFacts>;
        fn party_flow_members(self:&NativeFrontendPartyFlowIo)->Result<Vec<PartyMemberData>>;
        fn party_flow_command(self:&NativeFrontendPartyFlowIo,command:&PartyFlowCommand)->Result<bool>;
        fn party_flow_serial(self:&NativeFrontendPartyFlowIo)->Result<u64>;
        fn party_flow_update(self:&NativeFrontendPartyFlowIo)->Result<()>;
        fn party_flow_cancel(self:&NativeFrontendPartyFlowIo)->Result<()>;
        fn party_flow_lobby(self:&NativeFrontendPartyFlowIo,destination:i32)->Result<bool>;
        fn party_flow_ui(self:&NativeFrontendPartyFlowIo,effect:&PartyFlowUi)->Result<()>;
        type NativeFrontendPartyIo;
        fn party_primary_appearance(self:&NativeFrontendPartyIo)->Result<PartyAppearanceData>;
        fn party_local_players(self:&NativeFrontendPartyIo)->Result<Vec<PartySeatData>>;
        fn party_authorization(self:&NativeFrontendPartyIo,controller:u32)->Result<PartyAuthData>;

        type NativeFrontendTextIo;
        fn text_original_text(self:&NativeFrontendTextIo,screen:u32,pane:u32,index:u32)->Result<Vec<u8>>;
        fn text_original_string(self:&NativeFrontendTextIo,screen:u32,id:u32)->Result<Vec<u8>>;
        fn text_pc_string(self:&NativeFrontendTextIo,id:u32)->Result<Vec<u8>>;
        fn text_pc_authored(self:&NativeFrontendTextIo,pane:u32)->Result<Vec<u8>>;
        fn text_pc_match_line(self:&NativeFrontendTextIo)->Result<Vec<u8>>;
        fn text_map_name(self:&NativeFrontendTextIo)->Result<Vec<u8>>;
        fn text_quick_option(self:&NativeFrontendTextIo,index:u32)->Result<Vec<u8>>;
        fn text_party_count(self:&NativeFrontendTextIo)->Result<u64>;
        fn text_expand(self:&NativeFrontendTextIo,value:&[u8])->Result<Vec<u8>>;
        type NativeFrontendSearchIo;
        fn search_context(self:&NativeFrontendSearchIo)->Result<SearchContext>;
        fn search_view(self:&NativeFrontendSearchIo)->Result<SearchSnapshot>;
        fn search_identity(self:&NativeFrontendSearchIo)->Result<bool>;
        fn search_profile(self:&NativeFrontendSearchIo)->Result<Vec<u8>>;
        fn search_local_count(self:&NativeFrontendSearchIo)->Result<u32>;
        fn search_pull_after(self:&NativeFrontendSearchIo,revision:u64)->Result<bool>;
        fn search_pull(self:&NativeFrontendSearchIo)->Result<()>;
        fn search_account(self:&NativeFrontendSearchIo)->Result<SearchAccount>;
        fn search_command(self:&NativeFrontendSearchIo,action:&str,playlist:&str,revision:&str)->Result<bool>;
        fn search_sign_out(self:&NativeFrontendSearchIo)->Result<()>;
        fn search_refresh(self:&NativeFrontendSearchIo,name:&[u8])->Result<()>;
        fn search_cancel(self:&NativeFrontendSearchIo)->Result<()>;
        fn search_join(self:&NativeFrontendSearchIo,playlist:&str)->Result<()>;
        fn search_lobby_countdown(self:&NativeFrontendSearchIo)->Result<bool>;
        fn search_launch(self:&NativeFrontendSearchIo)->Result<bool>;
        fn search_has_launch(self:&NativeFrontendSearchIo)->Result<bool>;
        fn search_ui(self:&NativeFrontendSearchIo,effect:&SearchUi)->Result<()>;
        fn search_variant(self:&NativeFrontendSearchIo)->Result<SearchVariant>;
        fn search_map(self:&NativeFrontendSearchIo)->Result<Vec<u8>>;
        fn search_prepare_map(self:&NativeFrontendSearchIo,map:&[u8])->Result<()>;
        fn search_self_present(self:&NativeFrontendSearchIo)->Result<bool>;
        fn search_take(self:&NativeFrontendSearchIo)->Result<SearchTake>;
        fn search_local_counts(self:&NativeFrontendSearchIo)->Result<SearchCounts>;
        fn search_set_local_count(self:&NativeFrontendSearchIo,count:u32)->Result<()>;
        fn search_admitted_name(self:&NativeFrontendSearchIo,index:u32)->Result<Vec<u8>>;
        fn search_profile_name(self:&NativeFrontendSearchIo,index:u32)->Result<Vec<u8>>;
        fn search_append_profile(self:&NativeFrontendSearchIo,index:u32,teams:bool)->Result<()>;
        fn search_validate_map(self:&NativeFrontendSearchIo)->Result<bool>;
        fn search_reject_content(self:&NativeFrontendSearchIo)->Result<()>;
        fn search_publish(self:&NativeFrontendSearchIo)->Result<()>;
        fn search_discard(self:&NativeFrontendSearchIo)->Result<()>;
        fn search_failure(self:&NativeFrontendSearchIo)->Result<SearchFailure>;
        type NativeResultIo;
        fn result_state(self:&NativeResultIo)->Result<NativeResultState>;
        fn result_roster(self:&NativeResultIo)->Result<Vec<NativeResultAdmission>>;
        fn result_round_flags(self:&NativeResultIo)->Result<u32>;
        fn result_variant(self:&NativeResultIo)->Result<Vec<u8>>;
        fn result_recording(self:&NativeResultIo)->Result<NativeResultRecording>;
        fn result_ranks(self:&NativeResultIo,extended:bool,count:u32)->Result<NativeResultRanks>;
        fn result_changing_teams(self:&NativeResultIo)->Result<bool>;
        fn result_live_player(self:&NativeResultIo,slot:u32)->Result<NativeResultLivePlayer>;
        fn result_configuration(self:&NativeResultIo,slot:u32)->Result<Vec<u8>>;
        fn result_name(self:&NativeResultIo,index:u32)->Result<Vec<u8>>;
        fn result_team_score(self:&NativeResultIo,team:u32)->Result<i32>;
        fn result_accumulated_score(self:&NativeResultIo,datum:u32)->Result<i16>;
        fn result_write(self:&NativeResultIo,input:&NativeResultPublication)->Result<()>;

        type NativeAccountClientIo;
        fn account_lock(self:&NativeAccountClientIo)->Result<()>;
        fn account_unlock(self:&NativeAccountClientIo)->Result<()>;
        fn account_now_ns(self:&NativeAccountClientIo)->i64;
        fn account_identity(self:&NativeAccountClientIo)->Result<AccountClientIdentity>;
        fn account_request(self:&NativeAccountClientIo,request:&MatchmakingClientRequest)->Result<MatchmakingClientResponse>;
        fn account_cancelled(self:&NativeAccountClientIo)->bool;
        fn account_reset_cancellation(self:&NativeAccountClientIo)->Result<()>;
        fn account_cancel(self:&NativeAccountClientIo);
        fn account_wake(self:&NativeAccountClientIo);
        fn account_entropy(self:&NativeAccountClientIo)->Result<Vec<u8>>;
        fn account_eos_profiles(self:&NativeAccountClientIo)->Result<bool>;
        fn account_eos_identity(self:&NativeAccountClientIo,token:&str)->Result<EosIdentity>;
        type NativeSessionApiIo;
        fn api_lock(self:&NativeSessionApiIo)->Result<()>;
        fn api_unlock(self:&NativeSessionApiIo)->Result<()>;
        fn api_fence(self:&NativeSessionApiIo)->Result<()>;
        fn api_send(self:&NativeSessionApiIo,peer:u32,bytes:&[u8])->Result<()>;
        fn api_receive(self:&NativeSessionApiIo)->Result<Vec<NativeSessionPacket>>;
        fn api_receive_peer(self:&NativeSessionApiIo,peer:u32)->Result<()>;
        fn api_receive_bytes(self:&NativeSessionApiIo,bytes:&[u8])->Result<()>;
        fn api_audio_present(self:&NativeSessionApiIo)->bool;
        fn api_audio_resume_allowed(self:&NativeSessionApiIo)->bool;
        fn api_audio_resume(self:&NativeSessionApiIo)->Result<()>;
        fn api_audio_ready(self:&NativeSessionApiIo)->Result<bool>;
        fn api_audio_generation(self:&NativeSessionApiIo)->u64;
        fn api_audio_fence(self:&NativeSessionApiIo)->Result<u64>;
        fn api_apply_audio_fence(self:&NativeSessionApiIo,generation:u64)->Result<()>;
        fn api_voice(self:&NativeSessionApiIo,target:u32,ready:bool)->Result<()>;
        fn api_preferences(self:&NativeSessionApiIo,target:u32,enabled:bool,muted:u16)->Result<()>;
        fn api_collect_voices(self:&NativeSessionApiIo)->Result<()>;
        fn api_audio_binding(self:&NativeSessionApiIo)->NativeApiAudioBinding;
        fn api_install_audio(self:&NativeSessionApiIo)->Result<()>;
        fn api_bind_audio(self:&NativeSessionApiIo)->Result<()>;
        fn api_close_presence(self:&NativeSessionApiIo)->Result<()>;
        fn api_pin_audio(self:&NativeSessionApiIo)->Result<bool>;
        fn api_close_audio(self:&NativeSessionApiIo)->Result<()>;
        fn api_unpin_audio(self:&NativeSessionApiIo)->Result<()>;
        fn api_stop_worker(self:&NativeSessionApiIo)->Result<()>;
        fn api_worker_joinable(self:&NativeSessionApiIo)->bool;
        fn api_join_worker(self:&NativeSessionApiIo)->Result<()>;
        type NativeMigrationIo;
        fn migration_binding(self:&NativeMigrationIo)->Result<NativeMigrationBinding>;
        fn migration_agreement_present(self:&NativeMigrationIo)->bool;
        fn migration_now_ms(self:&NativeMigrationIo)->u64;
        fn migration_agreement(self:&NativeMigrationIo,now:u64)->Result<HostAgreementState>;
        fn migration_handoff(self:&NativeMigrationIo)->bool;
        fn migration_selection_started(self:&NativeMigrationIo)->bool;
        fn migration_quality(self:&NativeMigrationIo)->Result<NativeMigrationBinding>;
        fn migration_ranked(self:&NativeMigrationIo,now:u64)->Result<Vec<u32>>;
        fn migration_selection_supported(self:&NativeMigrationIo)->Result<bool>;
        fn migration_selection(self:&NativeMigrationIo)->Result<NativeMigrationSelection>;
        fn migration_apply_fence(self:&NativeMigrationIo,value:&NativeSessionFence)->Result<()>;
        fn migration_begin(self:&NativeMigrationIo,context:&HostAgreementContext)->Result<()>;
        fn migration_pick(self:&NativeMigrationIo,value:&NativeSessionCandidate)->Result<u32>;
        type NativeSnapshotViewIo;
        fn snapshot_view_common(self:&NativeSnapshotViewIo)->bool;
        fn snapshot_view_supported(self:&NativeSnapshotViewIo)->Result<bool>;
        fn snapshot_view_select(self:&NativeSnapshotViewIo)->Result<()>;
        type NativePostgameIo;
        fn postgame_pin_audio(self:&NativePostgameIo)->Result<bool>;
        fn postgame_phase(self:&NativePostgameIo)->Result<u32>;
        fn postgame_enable(self:&NativePostgameIo,enabled:bool)->Result<()>;
        fn postgame_focused(self:&NativePostgameIo)->bool;
        fn postgame_focus(self:&NativePostgameIo,focused:bool)->Result<()>;
        fn postgame_radio(self:&NativePostgameIo,pressed:bool,active:bool)->Result<()>;
        fn postgame_blocks(self:&NativePostgameIo)->Result<u16>;
        fn postgame_set_blocks(self:&NativePostgameIo,blocked:u16)->Result<()>;
        fn postgame_preferences(self:&NativePostgameIo)->Result<NativePostgamePreferences>;
        fn postgame_outgoing(self:&NativePostgameIo)->Result<Vec<NativeSessionAudio>>;
        fn postgame_lock(self:&NativePostgameIo)->Result<()>;
        fn postgame_unlock(self:&NativePostgameIo)->Result<()>;
        fn postgame_fence(self:&NativePostgameIo)->Result<()>;
        fn postgame_exchange(self:&NativePostgameIo,prefs:&NativePostgamePreferences,frames:Vec<NativeSessionAudio>)->Result<Vec<NativeSessionAudio>>;
        fn postgame_deliver(self:&NativePostgameIo,frames:Vec<NativeSessionAudio>)->Result<()>;
        include!("src/matchmaking_native_receive.hpp");
        type NativeReceiveIo;
        fn receive_lock(self:&NativeReceiveIo)->Result<()>;
        fn receive_unlock(self:&NativeReceiveIo)->Result<()>;
        fn receive_session(self:&NativeReceiveIo)->Result<NativeReceiveSession>;
        fn receive_supported(self:&NativeReceiveIo,target:NativeReceiveTarget)->Result<bool>;
        fn receive_pin(self:&NativeReceiveIo,target:NativeReceiveTarget)->Result<()>;
        fn receive_present(self:&NativeReceiveIo,target:NativeReceiveTarget)->Result<bool>;
        fn receive_epoch(self:&NativeReceiveIo,target:NativeReceiveTarget)->Result<u32>;
        fn receive_binding_present(self:&NativeReceiveIo)->Result<bool>;
        fn receive_binding_epoch(self:&NativeReceiveIo)->Result<u32>;
        fn receive_history_len(self:&NativeReceiveIo)->Result<u64>;
        fn receive_history_epoch(self:&NativeReceiveIo,index:u64)->Result<u32>;
        fn receive_history_packet(self:&NativeReceiveIo,index:u64,source:u32,packet:&[u8])->Result<()>;
        fn receive_packet(self:&NativeReceiveIo,target:NativeReceiveTarget,source:u32,packet:&[u8])->Result<()>;
        fn receive_abort(self:&NativeReceiveIo)->Result<()>;
        fn receive_progress(self:&NativeReceiveIo)->Result<()>;
        fn receive_fence(self:&NativeReceiveIo)->Result<()>;
        fn receive_general(self:&NativeReceiveIo,source:u32,epoch:u32,packet:&[u8])->Result<bool>;
        fn receive_observe_host(self:&NativeReceiveIo)->Result<()>;
        fn receive_refresh(self:&NativeReceiveIo)->Result<()>;

        include!("src/peer_snapshot_candidate.hpp");
        type NativeSnapshotBuffer;
        fn new_snapshot_buffer(size:u64)->Result<UniquePtr<NativeSnapshotBuffer>>;
        fn snapshot_buffer_bytes(self:&NativeSnapshotBuffer)->&[u8];
        fn snapshot_buffer_write(self:Pin<&mut NativeSnapshotBuffer>,offset:u64,data:&[u8])->Result<()>;
        type NativeSnapshotLeaseIo;
        fn snapshot_witness(self:&NativeSnapshotLeaseIo)->NativeSnapshotWitness;
        fn snapshot_issue(self:&NativeSnapshotLeaseIo,token:&NativeSnapshotToken,bytes:UniquePtr<NativeSnapshotBuffer>);
        fn snapshot_destroy(self:&NativeSnapshotLeaseIo);
        type NativeExchangeIo;
        fn exchange_session(self:&NativeExchangeIo)->NativeExchangeSession;
        fn exchange_now_ms(self:&NativeExchangeIo)->u64;
        fn exchange_direct_ready(self:&NativeExchangeIo)->u16;
        fn exchange_source_present(self:&NativeExchangeIo)->bool;
        fn exchange_continuity(self:&NativeExchangeIo)->Result<NativePublicationAdmission>;
        fn exchange_publish_source(self:&NativeExchangeIo)->Result<bool>;
        fn exchange_admission(self:&NativeExchangeIo)->Result<NativePublicationAdmission>;
        fn exchange_source_confirmed(self:&NativeExchangeIo,peer:u32)->Result<bool>;
        fn exchange_authority_present(self:&NativeExchangeIo)->bool;
        fn exchange_publish_authority(self:&NativeExchangeIo,ahead:bool,now:u64)->Result<MigrationPreparationCut>;
        fn exchange_authority_supported(self:&NativeExchangeIo,peer:u32,features:u32)->Result<bool>;
        fn exchange_common(self:&NativeExchangeIo)->Result<NativeExchangeCommon>;
        fn exchange_common_perform(self:&NativeExchangeIo,action:NativeExchangeCommonAction)->Result<bool>;
        fn exchange_common_view(self:&NativeExchangeIo)->Result<()>;
        fn exchange_selection_abort(self:&NativeExchangeIo)->Result<()>;
        fn exchange_selection_progress(self:&NativeExchangeIo)->Result<()>;
        fn exchange_quality(self:&NativeExchangeIo)->Result<NativeExchangeCommon>;
        fn exchange_candidates(self:&NativeExchangeIo,now:u64)->Result<()>;
        type NativePublicationIo;
        fn publication_now_ms(self:&NativePublicationIo)->u64;
        fn publication_session(self:&NativePublicationIo,decision:&NativePublicationDecision)->Result<NativePublicationSession>;
        fn publication_resources(self:&NativePublicationIo)->Result<NativePublicationResources>;
        fn publication_selection(self:&NativePublicationIo,cut:&MigrationPreparationCut)->Result<NativePublicationSelection>;
        fn publication_agreement(self:&NativePublicationIo,now:u64)->Result<NativePublicationAgreement>;
        fn publication_common(self:&NativePublicationIo)->Result<NativePublicationCommon>;
        fn publication_admission(self:&NativePublicationIo)->Result<NativePublicationAdmission>;
        fn publication_audio(self:&NativePublicationIo)->Result<NativePublicationAudio>;
        fn publication_snapshot(self:&NativePublicationIo)->Result<NativePublicationSnapshot>;
        fn publication_blocks(self:&NativePublicationIo)->bool;
        fn publication_invalidate(self:&NativePublicationIo,cut:&MigrationPreparationCut)->Result<()>;
        fn publication_observe_ready(self:&NativePublicationIo,peer:u8,cut:&MigrationPreparationCut)->Result<bool>;
        fn publication_fence(self:&NativePublicationIo,failed:bool)->Result<()>;
        fn publication_ready(self:&NativePublicationIo)->Result<bool>;
        fn publication_propose(self:&NativePublicationIo,peer:u32,now:u64)->Result<bool>;
        fn publication_candidate(self:&NativePublicationIo,now:u64)->Result<u32>;
        fn publication_allocate(self:&NativePublicationIo,plan:&NativePublicationSuccessor)->Result<()>;
        fn publication_has_commit(self:&NativePublicationIo)->bool;
        fn publication_commit(self:&NativePublicationIo)->Result<bool>;
        fn publication_publish(self:&NativePublicationIo)->Result<()>;
        include!("src/matchmaking_native_peers.hpp");
        type NativeMatchPeersIo;
        fn peers_cancelled(self:&NativeMatchPeersIo)->bool;
        fn peers_wait(self:&NativeMatchPeersIo,milliseconds:u32);
        fn peers_ready(self:&NativeMatchPeersIo,peer:u8)->bool;
        fn peers_failed(self:&NativeMatchPeersIo,peer:u8)->bool;
        fn peers_signals(self:&NativeMatchPeersIo)->Result<Vec<NativeSessionSignal>>;
        fn peers_signal(self:&NativeMatchPeersIo,signal:&NativeSessionSignal)->Result<()>;
        fn peers_sample(self:&NativeMatchPeersIo,ready:u16)->Result<NativePeerSample>;
        fn peers_receive(self:&NativeMatchPeersIo)->Result<NativePeerDatagram>;
        fn peers_datagram(self:&NativeMatchPeersIo,peer:u32,epoch:u32,bytes:&[u8])->Result<()>;
        fn peers_rejected(self:&NativeMatchPeersIo);
        fn peers_drain(self:&NativeMatchPeersIo)->Result<Vec<NativeSessionPacket>>;
        fn peers_send_media(self:&NativeMatchPeersIo,packet:&NativeSessionPacket,bytes:&[u8])->Result<()>;
        fn peers_send(self:&NativeMatchPeersIo,peer:u32,bytes:&[u8])->Result<()>;
        fn peers_poll(self:&NativeMatchPeersIo,kind:NativePeerPoll,budget:u32,ready:u16,index:u32)->Result<Vec<NativePeerDatagram>>;
        fn peers_drive(self:&NativeMatchPeersIo)->Result<()>;
        fn peers_agreement_generations(self:&NativeMatchPeersIo)->u32;
        fn peers_finish(self:&NativeMatchPeersIo,observation:&NativeSessionDirect)->Result<bool>;
        fn peers_failure(self:&NativeMatchPeersIo);
        include!("src/matchmaking_native_session.hpp");
        type NativeMatchSessionIo;
        fn session_now_ms(self:&NativeMatchSessionIo)->u64;
        fn session_cancelled(self:&NativeMatchSessionIo)->bool;
        fn session_wait(self:&NativeMatchSessionIo,milliseconds:u32);
        fn session_open(self:&NativeMatchSessionIo,options:&MatchmakingQosOpen)->Result<()>;
        fn session_receive(self:&NativeMatchSessionIo)->Result<MatchmakingQosFrame>;
        fn session_send(self:&NativeMatchSessionIo,kind:NativeSessionSendKind,data:&[u8])->Result<MatchmakingQosSent>;
        fn session_close(self:&NativeMatchSessionIo);
        fn session_text(self:&NativeMatchSessionIo,data:&[u8])->Result<String>;
        fn session_datagram(self:&NativeMatchSessionIo,peer:u8,epoch:u32,data:&[u8])->Result<()>;
        fn session_pump(self:&NativeMatchSessionIo);
        fn session_flush(self:&NativeMatchSessionIo)->Result<NativeSessionFlush>;
        fn session_packet_current(self:&NativeMatchSessionIo,packet:&NativeSessionPacket)->bool;
        fn session_failure(self:&NativeMatchSessionIo,terminal:bool,message:&str)->bool;
        include!("src/matchmaking_native_qos.hpp");
        type NativeMatchmakingQosIo;
        fn qos_now_ms(self: &NativeMatchmakingQosIo) -> u64;
        fn qos_cancelled(self: &NativeMatchmakingQosIo) -> bool;
        fn qos_wait(self: &NativeMatchmakingQosIo, milliseconds: u32);
        fn qos_open(self: &NativeMatchmakingQosIo, options: &MatchmakingQosOpen) -> Result<()>;
        fn qos_receive(self: &NativeMatchmakingQosIo) -> Result<MatchmakingQosFrame>;
        fn qos_send(self: &NativeMatchmakingQosIo, kind: MatchmakingQosSendKind, data: &[u8]) -> Result<MatchmakingQosSent>;
        fn qos_close(self: &NativeMatchmakingQosIo);
        include!("src/matchmaking_client.hpp");
        type NativeMatchmakingIo;
        fn client_now_ms(self: &NativeMatchmakingIo) -> u64;
        fn client_cancelled(self: &NativeMatchmakingIo, version: u64) -> bool;
        fn client_identity(self: &NativeMatchmakingIo, version: u64) -> Result<MatchmakingClientIdentity>;
        fn client_request(self: &NativeMatchmakingIo, request: &MatchmakingClientRequest) -> Result<MatchmakingClientResponse>;
        fn client_connection(self: &NativeMatchmakingIo, version: u64) -> Result<MatchmakingClientConnection>;
        fn client_install(self: &NativeMatchmakingIo, version: u64, admission: &MatchmakingClientAdmission) -> Result<bool>;
        fn client_disconnect(self: &NativeMatchmakingIo, version: u64);
        fn client_qos(self: &NativeMatchmakingIo, version: u64, endpoint: &str, bearer: &str, loopback: bool) -> Result<bool>;
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
        type PeerMeshQuality;
        fn new_peer_mesh_quality(self_peer:u32,context:&MeshQualityContext)->Result<Box<PeerMeshQuality>>;
        fn peer_mesh_quality_context(mesh:&PeerMeshQuality)->Result<MeshQualityContext>;
        fn peer_mesh_quality_poll(mesh:&mut PeerMeshQuality,ready:u16,now:u64,budget:u32)->Result<Vec<HostAgreementPacket>>;
        fn peer_mesh_quality_receive(mesh:&mut PeerMeshQuality,peer:u32,bytes:&[u8],now:u64)->Result<bool>;
        fn peer_mesh_quality_matrix(mesh:&PeerMeshQuality,now:u64)->Result<MeshQualityMatrix>;
        fn peer_mesh_quality_local(mesh:&PeerMeshQuality,now:u64)->Result<MeshQualityRow>;
        fn peer_mesh_quality_candidates(mesh:&PeerMeshQuality,now:u64,migration:bool)->Result<Vec<MeshQualityHost>>;
        fn peer_mesh_quality_is_packet(bytes:&[u8])->bool;
        type PeerSourceAdmissions;
        fn new_peer_source_admissions(self_peer:u32,protocol:u16,context:&SourceAdmissionsContext,roster:&[SourceAdmissionsSeat])->Result<Box<PeerSourceAdmissions>>;
        fn peer_source_admissions_resume(self_peer:u32,protocol:u16,next:&SourceAdmissionsContext,roster:&[SourceAdmissionsSeat],previous:&SourceAdmissionsAccepted)->Result<Box<PeerSourceAdmissions>>;
        fn peer_source_admissions_context(state:&PeerSourceAdmissions)->Result<SourceAdmissionsContext>;
        fn peer_source_admissions_replace(state:&mut PeerSourceAdmissions,expected:&SourceAdmissionsContext,next:&SourceAdmissionsContext)->Result<bool>;
        fn peer_source_admissions_publish(state:&mut PeerSourceAdmissions,context:&SourceAdmissionsContext,players:&[SourceAdmissionsPlayer])->Result<bool>;
        fn peer_source_admissions_accepted(state:&PeerSourceAdmissions)->Result<SourceAdmissionsAccepted>;
        fn peer_source_admissions_confirmed(state:&PeerSourceAdmissions,peer:u32)->Result<bool>;
        fn peer_source_admissions_poll(state:&mut PeerSourceAdmissions,now:u64,budget:u32)->Result<Vec<SourceAdmissionsPacket>>;
        fn peer_source_admissions_receive(state:&mut PeerSourceAdmissions,peer:u32,bytes:&[u8])->Result<bool>;
        fn peer_source_admissions_is_packet(bytes:&[u8])->bool;
        type PeerAuthorityTransfer;
        fn new_peer_authority_transfer(self_peer:u32,protocol:u16,context:&SourceAdmissionsContext,limits:&AuthorityTransferLimits)->Result<Box<PeerAuthorityTransfer>>;
        fn peer_authority_transfer_context(state:&PeerAuthorityTransfer)->Result<SourceAdmissionsContext>;
        fn peer_authority_transfer_replace(state:&PeerAuthorityTransfer,expected:&SourceAdmissionsContext,next:&SourceAdmissionsContext)->Result<bool>;
        fn peer_authority_transfer_supported(state:&PeerAuthorityTransfer,peer:u32)->Result<bool>;
        fn peer_authority_transfer_feature_supported(state:&PeerAuthorityTransfer,peer:u32,feature:u32)->Result<bool>;
        fn peer_authority_transfer_features(state:&PeerAuthorityTransfer)->Result<AuthorityTransferFeatures>;
        fn peer_authority_transfer_publish(state:&PeerAuthorityTransfer,context:&SourceAdmissionsContext,tick:u64,bytes:&[u8],targets:&[u32],now:u64)->Result<MigrationPreparationCut>;
        fn peer_authority_transfer_ahead(state:&PeerAuthorityTransfer,context:&SourceAdmissionsContext,previous:&MigrationPreparationCut,tick:u64,bytes:&[u8],peer:u32,now:u64)->Result<MigrationPreparationCut>;
        fn peer_authority_transfer_delivered(state:&PeerAuthorityTransfer,context:&SourceAdmissionsContext,seal:&MigrationPreparationCut,peer:u32)->Result<bool>;
        fn peer_authority_transfer_failed(state:&PeerAuthorityTransfer,context:&SourceAdmissionsContext,seal:&MigrationPreparationCut,peer:u32,now:u64)->Result<bool>;
        fn peer_authority_transfer_preparing(state:&PeerAuthorityTransfer,context:&SourceAdmissionsContext,seal:&MigrationPreparationCut,peer:u32,now:u64)->Result<bool>;
        fn peer_authority_transfer_poll(state:&PeerAuthorityTransfer,now:u64,budget:u32)->Result<Vec<SourceAdmissionsPacket>>;
        fn peer_authority_transfer_receive(state:&PeerAuthorityTransfer,peer:u32,bytes:&[u8],now:u64)->Result<bool>;
        fn peer_authority_transfer_completed(state:&PeerAuthorityTransfer)->Result<AuthorityTransferReceived>;
        fn peer_authority_transfer_acknowledge(state:&PeerAuthorityTransfer,context:&SourceAdmissionsContext,seal:&MigrationPreparationCut)->Result<bool>;
        fn peer_authority_transfer_reject(state:&PeerAuthorityTransfer,context:&SourceAdmissionsContext,seal:&MigrationPreparationCut)->Result<bool>;
        fn peer_authority_transfer_validated(state:&PeerAuthorityTransfer)->Result<Vec<AuthorityTransferReceipt>>;
        fn peer_authority_transfer_is_packet(bytes:&[u8])->bool;
        type PeerSnapshotCandidate;
        fn new_peer_snapshot_candidate(self_peer:u32,protocol:u16,context:&SourceAdmissionsContext,limits:&NativeSnapshotLimits)->Result<Box<PeerSnapshotCandidate>>;
        fn peer_snapshot_self(state:&PeerSnapshotCandidate)->u32;
        fn peer_snapshot_begin(state:&PeerSnapshotCandidate,now:u64)->Result<NativeSnapshotToken>;
        fn peer_snapshot_arm(state:&PeerSnapshotCandidate,token:&NativeSnapshotToken)->Result<bool>;
        fn peer_snapshot_publish(state:&PeerSnapshotCandidate,token:&NativeSnapshotToken,archives:Vec<NativeSnapshotArchive>,now:u64)->Result<bool>;
        fn peer_snapshot_take(state:&PeerSnapshotCandidate,io:&NativeSnapshotLeaseIo)->Result<bool>;
        fn peer_snapshot_capture(state:&PeerSnapshotCandidate,io:&NativeSnapshotLeaseIo)->Result<bool>;
        fn peer_snapshot_close(state:&PeerSnapshotCandidate,token:&NativeSnapshotToken,mode:u32)->Result<bool>;
        fn peer_snapshot_reject(state:&PeerSnapshotCandidate,token:&NativeSnapshotToken)->Result<bool>;
        fn peer_snapshot_can_retire(state:&PeerSnapshotCandidate,token:&NativeSnapshotToken,io:&NativeSnapshotLeaseIo)->Result<bool>;
        fn peer_snapshot_retire(state:&PeerSnapshotCandidate,token:&NativeSnapshotToken,io:&NativeSnapshotLeaseIo)->Result<bool>;
        fn peer_snapshot_state(state:&PeerSnapshotCandidate)->Result<NativeSnapshotState>;
        fn peer_snapshot_poll(state:&PeerSnapshotCandidate,now:u64,budget:u32)->Result<Vec<SourceAdmissionsPacket>>;
        fn peer_snapshot_receive(state:&PeerSnapshotCandidate,peer:u32,bytes:&[u8],now:u64)->Result<bool>;
        fn peer_snapshot_is_packet(bytes:&[u8])->bool;
        type NativeSession;
        type AccountClient;
        fn new_account_client(config:&MatchmakingClientConfig,steam:bool,parties:bool,profile_only:bool)->Box<AccountClient>;
        fn account_client_configured(client:&AccountClient)->bool;
        fn account_client_active(client:&AccountClient)->bool;
        fn account_client_pending(client:&AccountClient)->bool;
        fn account_client_cadence_ms(client:&AccountClient)->i64;
        fn account_client_endpoint(client:&AccountClient)->String;
        fn account_client_snapshot(client:&AccountClient)->String;
        fn account_client_connect(client:&AccountClient,name:&str,io:&NativeAccountClientIo);
        fn account_client_sign_out(client:&AccountClient,io:&NativeAccountClientIo);
        fn account_client_profile(client:&AccountClient,bytes:&[u8],tag:&str,io:&NativeAccountClientIo)->Result<()>;
        fn account_client_clan_command(client:&AccountClient,action:&str,value:&str,role:u32,confirmed:bool,io:&NativeAccountClientIo)->Result<bool>;
        fn account_client_party_command(client:&AccountClient,command:&AccountClientPartyCommand,activity:u32,io:&NativeAccountClientIo)->Result<u64>;
        fn account_client_friend_command(client:&AccountClient,action:&str,value:&str,io:&NativeAccountClientIo)->Result<bool>;
        fn account_client_refresh_steam(client:&AccountClient,io:&NativeAccountClientIo);
        fn account_client_poll(client:&AccountClient,io:&NativeAccountClientIo)->Result<()>;
        fn native_result_publish(io:&NativeResultIo)->Result<()>;
        fn native_result_validate(io:&NativeResultIo,raw:&[u8])->Result<()>;
        type FrontendLobby;
        type FrontendSearch;
        fn frontend_party_members(present:bool,party:&[PartyMemberData],io:&NativeFrontendPartyIo)->Result<Vec<PartyMemberData>>;
        fn frontend_party_column(party:&[PartyMemberData],voice:&PartyVoiceData,screen:u32,row:u32,column:u32)->Vec<u8>;
        fn frontend_party_sounds(before:&PartyNotificationData,after:&PartyNotificationData)->Vec<u32>;
        fn frontend_party_notice(facts:&PartyNoticeFactsData)->PartyNoticeData;
        fn lobby_view_session(input:&LobbyViewSession)->i32;
        fn lobby_view_pane(session:i32,previous:i32)->i32;
        fn lobby_view_protocol(session:i32)->u32;
        fn lobby_view_presentation(input:&LobbyViewInput)->LobbyViewPresentation;
        fn lobby_view_variant_type(engine:u32)->LobbyViewI32;
        fn lobby_view_game_icon(kind:i32)->i32;
        fn lobby_view_quick_category(kind:i32)->LobbyViewU32;
        fn lobby_view_difficulty(value:i16)->u32;
        fn lobby_view_help(pane:i32,protocol:i32,machines:i32,privacy:i32,session:i32)->LobbyViewHelp;
        fn lobby_view_player_count(previous:i32,a:i32,b:i32)->i32;
        fn lobby_view_player_count_string(count:i32)->LobbyViewU32;
        fn lobby_view_default_value(engine:u32,parameter:u32,io:&NativeFrontendLobbyViewIo)->Result<LobbyViewI32>;
        fn lobby_view_quick_options(engine:u32,separator:&[u8],count:usize,io:&NativeFrontendLobbyViewIo)->Result<Vec<LobbyViewBytes>>;
        type FrontendPartyFlow;
        fn frontend_party_view_populate(controller:&FrontendPartyFlow,io:&NativeFrontendPartyViewIo)->Result<()>;
        fn frontend_party_view_text(index:u32,io:&NativeFrontendPartyViewIo)->Result<Vec<u8>>;
        fn frontend_lobby_query(io:&NativeFrontendLobbyQueryIo)->Result<LobbyQueryView>;
        fn frontend_account_flow(controller:&FrontendPartyFlow,continuation:bool,io:&NativeFrontendAccountFlowIo)->Result<()>;
        fn frontend_friends_flow(controller:&FrontendPartyFlow,action:u32,a:u32,b:u32,io:&NativeFrontendFriendsIo)->Result<FriendsFlowResult>;
        fn frontend_friends_screen(screen:u32,context:u32)->bool;

        fn new_frontend_party_flow()->Box<FrontendPartyFlow>;
        fn clone_frontend_party_flow(controller:&FrontendPartyFlow)->Box<FrontendPartyFlow>;
        fn frontend_party_flow_state(controller:&FrontendPartyFlow)->PartyFlowState;
        fn frontend_party_flow_clear_dismissed(controller:&FrontendPartyFlow);
        fn frontend_party_flow_join_serial(controller:&FrontendPartyFlow,value:u64);
        fn frontend_party_flow_account_name(controller:&FrontendPartyFlow,value:&[u8]);
        fn frontend_party_flow_voice_party(controller:&FrontendPartyFlow,value:bool);
        fn frontend_party_flow_voice_wanted(controller:&FrontendPartyFlow,value:bool);
        fn frontend_party_flow_match_voice(controller:&FrontendPartyFlow,value:bool,joined:bool)->bool;
        fn frontend_party_flow_prune(controller:&FrontendPartyFlow,current:&[PartyBytesData]);
        fn frontend_party_flow_run(controller:&FrontendPartyFlow,action:u32,value:i32,io:&NativeFrontendPartyFlowIo)->Result<bool>;
        fn frontend_party_flow_command(controller:&FrontendPartyFlow,command:&PartyFlowCommand,io:&NativeFrontendPartyFlowIo)->Result<()>;
        fn frontend_party_flow_screen(mode:i32,screen:u32,context:u32)->bool;
        fn frontend_party_flow_picker(screen:u32,context:u32)->bool;
        fn frontend_presentation_text(f:&PresentationFacts,original:bool,pane:u32,index:u32,io:&NativeFrontendTextIo)->Result<Vec<u8>>;
        fn progress_countdown(elapsed:u32)->ProgressCountdown;
        fn progress_published(present:bool,remaining:u32)->ProgressCountdown;
        fn progress_pane(ui:i32,countdown:i32)->u32;
        fn progress_pc_pane(ui:i32,countdown:i32)->u32;
        fn progress_sound(previous:u32,current:u32)->i32;
        fn progress_title(ui:i32)->u32;
        fn progress_titles(ui:i32)->ProgressTitle;
        fn progress_cancel(pane:u32,elapsed:u32)->bool;
        fn progress_y(pane:u32)->bool;
        fn progress_leave(party:usize,local:usize)->u32;
        fn progress_team_colors(countdown:i32,teams:bool)->bool;
        fn progress_uses_party(pane:u32,ranked:bool)->bool;
        fn progress_order(players:&[ProgressKey],ranked:bool,teams:bool)->Vec<u32>;
        fn progress_pane_order(players:&[ProgressKey],pane:u32,ranked:bool,teams:bool)->Vec<u32>;
        fn progress_rows(pane:u32,ranked:bool,party:u32,game:u32,maximum_present:bool,maximum:u32)->Vec<u8>;
        fn progress_category(kind:i32)->i32;
        fn progress_icon(kind:i32)->u32;
        fn progress_needed(available:bool,count:i32,required:i32)->u8;
        type ProgressTipClock;
        fn new_progress_tip_clock(last:u32,initialized:bool)->Box<ProgressTipClock>;
        fn clone_progress_tip_clock(clock:&ProgressTipClock)->Box<ProgressTipClock>;
        fn progress_tip_update(clock:&mut ProgressTipClock,now:u32)->bool;
        fn progress_tip_index(seed:u32,count:u32)->ProgressTip;
        fn progress_lobby_press(current:i32)->i32;
        fn progress_lobby_due(current:i32,elapsed:u32)->bool;
        fn progress_lobby_beeps(previous:i32,current:i32)->bool;
        fn progress_project(f:&ProgressFacts)->Vec<ProgressRow>;
        fn progress_query(f:&ProgressFacts,screen:i32,entries:&[i32],row:u32)->ProgressRow;
        fn new_frontend_search()->Box<FrontendSearch>;
        fn clone_frontend_search(c:&FrontendSearch)->Box<FrontendSearch>;
        fn frontend_search_scalar(c:&FrontendSearch,field:u32)->Result<i64>;
        fn frontend_search_selected(c:&FrontendSearch)->String;
        fn frontend_search_stop(c:&FrontendSearch);
        fn frontend_search_quick(c:&FrontendSearch,value:bool);
        fn frontend_search_left(c:&FrontendSearch);
        fn frontend_search_reset(c:&FrontendSearch,clock:u32);
        fn frontend_search_run(c:&FrontendSearch,operation:u32,index:u32,io:&NativeFrontendSearchIo)->Result<bool>;
        fn frontend_search_ui(c:&FrontendSearch,view:SearchSnapshot)->Result<i32>;
        fn frontend_search_pane(c:&FrontendSearch,view:SearchSnapshot)->Result<u32>;
        fn frontend_search_state(c:&FrontendSearch)->SearchState;
        fn frontend_search_variant(name:&[u8],present:bool,raw:&[u8],content:&[SearchName],bundled:&[SearchName])->SearchChoice;
        type FrontendLobbyPlan;
        fn new_frontend_lobby()->Box<FrontendLobby>;
        fn clone_frontend_lobby(l:&FrontendLobby)->Box<FrontendLobby>;
        fn frontend_lobby_seconds(l:&FrontendLobby)->i32;
        fn frontend_lobby_matchmaking(l:&FrontendLobby)->bool;
        fn frontend_lobby_leave_pending(l:&FrontendLobby)->bool;
        fn frontend_lobby_cancel(l:&mut FrontendLobby);
        fn frontend_lobby_exit(l:&mut FrontendLobby);
        fn frontend_lobby_enter(l:&mut FrontendLobby,f:&FrontendLobbyFacts);
        fn frontend_lobby_observe(l:&mut FrontendLobby,f:&FrontendLobbyFacts)->FrontendLobbyObservation;
        fn frontend_lobby_start_error(l:&FrontendLobby,f:&FrontendLobbyFacts)->String;
        fn frontend_lobby_plan(l:&FrontendLobby,f:&FrontendLobbyFacts,event:u32,destination:i32)->Result<Box<FrontendLobbyPlan>>;
        fn frontend_lobby_plan_command(p:&FrontendLobbyPlan)->u8;
        fn frontend_lobby_plan_playlist(p:&FrontendLobbyPlan)->String;
        fn frontend_lobby_plan_revision(p:&FrontendLobbyPlan)->String;
        fn frontend_lobby_finish_input(l:&mut FrontendLobby,p:Box<FrontendLobbyPlan>,accepted:bool)->FrontendLobbyInputEffects;
        fn frontend_lobby_observe_leave(l:&mut FrontendLobby,f:&FrontendLobbyFacts)->bool;
        fn frontend_lobby_left(l:&mut FrontendLobby);
        fn frontend_lobby_view(l:&FrontendLobby)->FrontendLobbyView;
        fn account_client_shutdown(client:&AccountClient,io:&NativeAccountClientIo);
        fn account_client_voice(client:&AccountClient,party:bool,io:&NativeAccountClientIo)->Result<AccountClientVoice>;
        fn account_client_discord_operation(client:&AccountClient,serial:u64,io:&NativeAccountClientIo)->Result<AccountClientOperation>;
        fn account_client_discord_link(client:&AccountClient,io:&NativeAccountClientIo)->Result<AccountClientLink>;
        fn account_party_follower(leader:bool,members:usize)->bool;
        fn account_party_leader_left_postgame(leader:bool,members:&[AccountClientPostgameMember])->bool;
        type NativeSessionWorker;
        fn new_native_session(config:&NativeSessionConfig)->Result<Box<NativeSession>>;
        fn native_session_view(state:&NativeSession)->NativeSessionView;
        fn native_session_worker(state:&NativeSession)->Box<NativeSessionWorker>;
        fn native_session_room_api(state:&NativeSession)->String;
        fn native_session_run(worker:&NativeSessionWorker,io:&NativeMatchSessionIo);
        fn native_session_text(state:&mut NativeSession,data:&[u8],resources:&NativeSessionResources)->Result<NativeSessionEffects>;
        fn native_session_failure(state:&mut NativeSession,terminal:bool,message:&str)->NativeSessionDirectEffects;
        fn native_session_fail(state:&mut NativeSession,message:&str)->NativeSessionEffects;
        fn native_session_fence_audio(state:&mut NativeSession,generation:u64);
        fn native_session_bind_audio(state:&mut NativeSession);
        fn native_session_loaded(state:&mut NativeSession,fingerprint:u64,settings:&[u8],parameters:&[NativeSessionParameter])->Result<()>;
        fn native_session_send(state:&mut NativeSession,peer:u32,data:&[u8]);
        fn native_session_receive(state:&mut NativeSession)->Vec<NativeSessionPacket>;
        fn native_session_datagram(state:&mut NativeSession,peer:u32,epoch:u32,data:&[u8])->Result<bool>;
        fn native_session_outgoing(state:&mut NativeSession)->Vec<NativeSessionPacket>;
        fn native_session_packet_current(state:&NativeSession,packet:&NativeSessionPacket)->bool;
        fn native_session_flush(state:&mut NativeSession)->NativeSessionFlush;
        fn native_session_voice(state:&mut NativeSession,target:u32,frame:&NativeSessionAudio,ready:bool);
        fn native_session_preferences(state:&mut NativeSession,target:u32,enabled:bool,muted:u16);
        fn native_session_voices(state:&mut NativeSession)->Vec<NativeSessionVoice>;
        fn native_session_voice_enabled(state:&NativeSession,slot:u32)->bool;
        fn native_session_postgame_voice(state:&mut NativeSession,now:u64,enabled:bool,muted:u16,blocked:u16,frames:Vec<NativeSessionAudio>,ready:bool)->Result<Vec<NativeSessionAudio>>;
        fn native_session_complete(state:&mut NativeSession,report:&[u8])->Result<()>;
        fn native_session_confirm(state:&mut NativeSession)->Result<()>;
        fn native_session_postgame(state:&NativeSession)->Vec<u8>;
        fn native_session_close_postgame(state:&mut NativeSession);
        fn native_session_world(state:&mut NativeSession,kind:u8,seal:&NativeSessionSeal)->Result<()>;
        fn native_session_seals(state:&mut NativeSession)->Vec<NativeSessionSeal>;
        fn native_session_signal(state:&mut NativeSession,signal:&NativeSessionSignal)->Result<()>;
        fn native_session_direct(state:&mut NativeSession,observation:&NativeSessionDirect)->Result<NativeSessionDirectEffects>;
        fn native_session_fence(state:&mut NativeSession,observation:&NativeSessionFence)->Result<NativeSessionEffects>;
        fn native_session_candidate(state:&NativeSession,observation:&NativeSessionCandidate)->Result<u32>;
        type NativePeerWorker;
        fn native_session_peer_worker(state:&NativeSession)->Result<Box<NativePeerWorker>>;
        fn native_session_epoch_packets(state:&NativeSession)->bool;
        fn native_peer_run(worker:&mut NativePeerWorker,io:&NativeMatchPeersIo);
        fn native_session_peer_plan(state:&NativeSession,observed:&NativePeerResources)->NativePeerPlan;
        fn native_peer_poll_allowed(plan:&NativePeerPlan,kind:NativePeerPoll)->bool;
        fn native_publication_session(state:&NativeSession,decision:&NativePublicationDecision)->NativePublicationSession;
        fn native_session_authority_limits(state:&NativeSession)->AuthorityTransferLimits;
        fn native_session_local_slots(state:&NativeSession)->Vec<u32>;
        fn native_session_physical_peers(state:&NativeSession)->Vec<u32>;
        fn native_session_presence(state:&NativeSession)->NativeSessionPresence;
        fn native_session_continuity_view(state:&NativeSession)->NativeSessionContinuity;
        fn native_session_control_postgame(direct_present:bool)->bool;
        fn native_session_snapshot_view(io:&NativeSnapshotViewIo)->Result<()>;
        fn native_session_pump_postgame(io:&NativePostgameIo)->Result<()>;
        fn native_migration_fence(io:&NativeMigrationIo,force:bool,failed:bool)->Result<()>;
        fn native_migration_begin(io:&NativeMigrationIo)->Result<()>;
        fn native_migration_candidate(io:&NativeMigrationIo,now:u64)->Result<NativeMigrationCandidate>;
        fn native_api_send(io:&NativeSessionApiIo,peer:u32,bytes:&[u8])->Result<()>;
        fn native_api_receive(io:&NativeSessionApiIo)->Result<bool>;
        fn native_api_voice(io:&NativeSessionApiIo,target:u32)->Result<()>;
        fn native_api_preferences(io:&NativeSessionApiIo,target:u32,enabled:bool,muted:u16)->Result<()>;
        fn native_api_voices(io:&NativeSessionApiIo)->Result<()>;
        fn native_api_retain_audio(io:&NativeSessionApiIo)->Result<bool>;
        fn native_api_close(io:&NativeSessionApiIo)->Result<()>;
        fn native_api_sync_audio(io:&NativeSessionApiIo)->Result<()>;
        fn native_api_fence_audio(io:&NativeSessionApiIo)->Result<()>;
        fn native_receive_dispatch(io:&NativeReceiveIo,source:u32,packet:&[u8],epoch:u32)->Result<()>;
        fn native_exchange_session(state:&NativeSession)->NativeExchangeSession;
        fn native_exchange_source_publish(io:&NativeExchangeIo,players:&[SourceAdmissionsPlayer])->Result<bool>;
        fn native_exchange_source_visible(io:&NativeExchangeIo)->Result<bool>;
        fn native_exchange_authority_publish(io:&NativeExchangeIo,ahead:bool)->Result<MigrationPreparationCut>;
        fn native_exchange_capability(io:&NativeExchangeIo,kind:NativeExchangeCapability,peer:u32)->Result<bool>;
        fn native_exchange_common(io:&NativeExchangeIo,context:&NativePublicationContext,action:NativeExchangeCommonAction)->Result<bool>;
        fn native_exchange_common_view(io:&NativeExchangeIo)->Result<()>;
        fn native_exchange_candidates(io:&NativeExchangeIo)->Result<()>;
        fn native_publication_abort(io:&NativePublicationIo)->Result<()>;
        fn native_publication_progress(io:&NativePublicationIo)->Result<()>;
        fn native_publication_fence(io:&NativePublicationIo)->Result<()>;
        fn native_publication_prepare(io:&NativePublicationIo,context:&NativePublicationContext,cut:&MigrationPreparationCut)->Result<bool>;
        fn native_publication_propose(io:&NativePublicationIo,peer:u32)->Result<bool>;
        fn native_publication_propose_automatic(io:&NativePublicationIo)->Result<bool>;
        fn native_publication_finish(io:&NativePublicationIo,decision:&NativePublicationDecision)->Result<bool>;
        fn native_preparation_authorize(state:&NativeSession,resources:&NativePublicationResources,admission:&NativePublicationAdmission,selection_supported:bool)->Result<NativePreparationAuthorization>;
        fn native_preparation_request(io:&NativePreparationIo)->Result<NativePreparationStep>;
        fn native_preparation_complete(io:&NativePreparationIo,kind:NativePreparationKind,ticket:&NativePreparationTicket,inventory_present:bool,lease:u64)->Result<NativePreparationOutcome>;
        fn native_preparation_freeze(io:&NativePreparationIo,context:&NativePublicationContext)->Result<bool>;
        fn native_preparation_invalidate(io:&NativePreparationIo,context:&NativePublicationContext,cut:&MigrationPreparationCut)->Result<bool>;
        fn native_preparation_retiring(io:&NativePreparationIo)->bool;
        fn native_session_selection(state:&mut NativeSession,context:&HostAgreementContext)->Result<()>;
        fn native_session_commit_allowed(state:&NativeSession,decision:&HostAgreementDecision)->bool;
        fn native_session_committed(state:&mut NativeSession,decision:&HostAgreementDecision)->Result<()>;
        type MatchmakingQos;
        fn new_matchmaking_qos(config: &MatchmakingQosConfig) -> Result<Box<MatchmakingQos>>;
        fn matchmaking_qos_run(qos: &MatchmakingQos, io: &NativeMatchmakingQosIo);
        type MatchmakingClient;
        fn matchmaking_client_runtime_compatibility() -> Result<String>;
        fn new_matchmaking_client(config: &MatchmakingClientConfig) -> Box<MatchmakingClient>;
        fn matchmaking_client_configured(client: &MatchmakingClient) -> bool;
        fn matchmaking_client_current(client: &MatchmakingClient, version: u64) -> bool;
        fn matchmaking_client_active(client: &MatchmakingClient) -> bool;
        fn matchmaking_client_command(client: &MatchmakingClient, command: &MatchmakingClientCommand) -> Result<u64>;
        fn matchmaking_client_snapshot(client: &MatchmakingClient) -> MatchmakingClientSnapshot;
        fn matchmaking_client_poll(client: &MatchmakingClient, io: &NativeMatchmakingIo);
        fn matchmaking_client_shutdown(client: &MatchmakingClient, io: &NativeMatchmakingIo);
        fn matchmaking_client_reject_content(client: &MatchmakingClient) -> bool;
        fn matchmaking_client_speaker_binding(client: &MatchmakingClient, row: u32, name: &str, team: i32, slot: u32, peer: u32) -> bool;
        fn matchmaking_client_take_launch(client: &MatchmakingClient, observation: &MatchmakingLaunchObservation, seats: &[MatchmakingSeat]) -> bool;
        fn matchmaking_connected_teams(peers: &[u32], current: u32, seats: &[MatchmakingSeat]) -> bool;
        fn matchmaking_variant_valid(settings: &[u8]) -> bool;
        fn matchmaking_eligibility(require_teams: bool, connected: bool, check_original_policy: bool,
            live_logon: bool, local_bit31: bool, disable_teams: bool, disable_local_policy: bool) -> MatchmakingEligibility;
        fn matchmaking_error_dialog(state: i32, local_cancel: bool) -> MatchmakingErrorDialog;
        fn matchmaking_error_clear(local_host: bool, state: i32, shown: u32, now: u32) -> bool;
        fn matchmaking_native_launch_ready(observation: &MatchmakingLaunchObservation, seats: &[MatchmakingSeat]) -> bool;
        type LanEndpoint;
        type LanSession;
        fn new_lan_endpoint() -> Result<Box<LanEndpoint>>;
        fn new_lan_session() -> Box<LanSession>;
        fn lan_public(endpoint: &LanEndpoint) -> [u8;32];
        fn lan_bind(endpoint: &LanEndpoint, session: &mut LanSession, peer: &[u8;32], host: bool, context: &[u8;24]) -> Result<()>;
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
        fn account_http_response_limit(method: &str, url: &str) -> usize;
        fn account_http_timeout_ms(method: &str, url: &str, body: &str) -> u32;
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
        working_bytes: u64,
    }
    struct ArchiveNode {
        valid: bool,
        tag: u8,
        bits: u64,
        count: u64,
        next: usize,
    }
    struct ArchiveRange {
        valid: bool,
        offset: usize,
        bytes: usize,
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
    struct NativeMotion {
        present: bool,
        grounded: bool,
        crouching: bool,
        baseline_age: u8,
        position: [i32; 3],
        velocity: [i16; 3],
    }
    struct NativeRedundantControl {
        sequence: u32,
        view: u8,
        forward: f32,
        right: f32,
        yaw: f32,
        pitch: f32,
        flags: u16,
        zoom: i16,
    }
    struct NativeRequestFrame {
        team_view: u8,
        team: u8,
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
        has_view: bool,
        view_tick: u16,
        view_fraction: u8,
        motion: Vec<NativeMotion>,
        redundant: Vec<NativeRedundantControl>,
    }

    extern "Rust" {
        type Archive;
        fn parse_authority_archive(packet: &[u8], limits: ArchiveLimits) -> Result<Box<Archive>>;
        fn parse_authority_archive_raw(packet: &[u8], hash:[u8;32], limits: ArchiveLimits) -> Result<Box<Archive>>;
        fn archive_node(self: &Archive, index: usize) -> ArchiveNode;
        fn archive_name(self: &Archive, index: usize) -> &[u8];
        fn archive_data(self: &Archive, index: usize) -> &[u8];
        fn archive_raw(self: &Archive) -> &[u8];
        fn archive_hash(self: &Archive) -> [u8; 32];
        fn archive_len(self: &Archive) -> usize;
        fn archive_claim(self: &Archive, count: u64, extent: u64, elements: bool) -> bool;
        fn archive_stats(self: &Archive) -> ArchiveStats;
        type ArchiveIndex;
        fn index_authority_archive_raw(packet: &[u8], hash: [u8; 32], limits: ArchiveLimits) -> Result<Box<ArchiveIndex>>;
        fn archive_index_node(self: &ArchiveIndex, index: usize) -> ArchiveNode;
        fn archive_index_name(self: &ArchiveIndex, index: usize) -> ArchiveRange;
        fn archive_index_data(self: &ArchiveIndex, index: usize) -> ArchiveRange;
        fn archive_index_hash(self: &ArchiveIndex) -> [u8; 32];
        fn archive_index_len(self: &ArchiveIndex) -> usize;
        fn archive_index_claim(self: &ArchiveIndex, count: u64, extent: u64, elements: bool) -> bool;
        fn archive_index_stats(self: &ArchiveIndex) -> ArchiveStats;
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
            working_bytes: limits.working_bytes,
        };
        halo2_online_protocol::authority_archive::decode(packet, limits)
            .map(|inner| Box::new(Archive { inner }))
            .map_err(|e| e.to_string())
    })
    .unwrap_or_else(|_| Err("Authority archive parser failed".to_owned()))
}

fn parse_authority_archive_raw(packet:&[u8],hash:[u8;32],limits:ffi::ArchiveLimits)->Result<Box<Archive>,String>{
    std::panic::catch_unwind(||{
        let limits=halo2_online_protocol::authority_archive::Limits{
            encoded_bytes:limits.encoded_bytes,decoded_bytes:limits.decoded_bytes,
            allocated_bytes:limits.allocated_bytes,container_elements:limits.container_elements,
            fixed_native_bytes:limits.fixed_native_bytes,working_bytes:limits.working_bytes,
        };
        halo2_online_protocol::authority_archive::decode_raw(packet,hash,limits)
            .map(|inner|Box::new(Archive{inner})).map_err(|e|e.to_string())
    }).unwrap_or_else(|_|Err("Authority archive parser failed".to_owned()))
}

struct ArchiveIndex {
    inner: halo2_online_protocol::authority_archive::ArchiveIndex,
}
impl ArchiveIndex {
    fn archive_index_node(&self, index: usize) -> ffi::ArchiveNode {
        self.inner.node(index).map_or(
            ffi::ArchiveNode { valid: false, tag: 0, bits: 0, count: 0, next: 0 },
            |n| ffi::ArchiveNode { valid: true, tag: n.tag, bits: n.bits, count: n.count, next: n.next },
        )
    }
    fn archive_index_name(&self, index: usize) -> ffi::ArchiveRange {
        self.inner.name_range(index).map_or(
            ffi::ArchiveRange { valid: false, offset: 0, bytes: 0 },
            |range| ffi::ArchiveRange { valid: true, offset: range.start, bytes: range.end - range.start },
        )
    }
    fn archive_index_data(&self, index: usize) -> ffi::ArchiveRange {
        self.inner.data_range(index).map_or(
            ffi::ArchiveRange { valid: false, offset: 0, bytes: 0 },
            |range| ffi::ArchiveRange { valid: true, offset: range.start, bytes: range.end - range.start },
        )
    }
    fn archive_index_hash(&self) -> [u8; 32] { self.inner.hash() }
    fn archive_index_len(&self) -> usize { self.inner.len() }
    fn archive_index_claim(&self, count: u64, extent: u64, elements: bool) -> bool {
        self.inner.claim_native(count, extent, elements)
    }
    fn archive_index_stats(&self) -> ffi::ArchiveStats {
        let Some(s) = self.inner.stats() else {
            return ffi::ArchiveStats { valid: false, raw_bytes: 0, nodes: 0, metadata_bytes: 0,
                deepest: 0, working_limit: 0, working_reserved: 0, working_peak: 0, native_claimed: 0 };
        };
        ffi::ArchiveStats { valid: true, raw_bytes: s.raw_bytes, nodes: s.nodes, metadata_bytes: s.metadata_bytes,
            deepest: s.deepest, working_limit: s.working_limit, working_reserved: s.working_reserved,
            working_peak: s.working_peak, native_claimed: s.native_claimed }
    }
}
fn index_authority_archive_raw(packet: &[u8], hash: [u8; 32], limits: ffi::ArchiveLimits) -> Result<Box<ArchiveIndex>, String> {
    std::panic::catch_unwind(|| {
        let limits = halo2_online_protocol::authority_archive::Limits {
            encoded_bytes: limits.encoded_bytes, decoded_bytes: limits.decoded_bytes,
            allocated_bytes: limits.allocated_bytes, container_elements: limits.container_elements,
            fixed_native_bytes: limits.fixed_native_bytes, working_bytes: limits.working_bytes,
        };
        halo2_online_protocol::authority_archive::index_raw(packet, hash, limits)
            .map(|inner| Box::new(ArchiveIndex { inner })).map_err(|e| e.to_string())
    }).unwrap_or_else(|_| Err("Authority archive parser failed".to_owned()))
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
        team_view: 0,
        team: 0,
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
        has_view: false,
        view_tick: 0,
        view_fraction: 0,
        motion: Vec::new(),
        redundant: Vec::new(),
    }
}

fn native_motion(
    value: Option<halo2_online_protocol::native_requests::MotionReport>,
) -> ffi::NativeMotion {
    match value {
        Some(m) => ffi::NativeMotion {
            present: true,
            grounded: m.grounded,
            crouching: m.crouching,
            baseline_age: m.baseline_age,
            position: m.position,
            velocity: m.velocity,
        },
        None => ffi::NativeMotion {
            present: false,
            grounded: false,
            crouching: false,
            baseline_age: 0,
            position: [0; 3],
            velocity: [0; 3],
        },
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
            view,
            motion,
            redundant,
        } => {
            frame.kind = 2;
            frame.motion = motion.into_iter().map(native_motion).collect();
            frame.redundant = redundant
                .into_iter()
                .flat_map(|entry| {
                    let sequence = entry.sequence;
                    entry
                        .controls
                        .into_iter()
                        .enumerate()
                        .map(move |(view, c)| ffi::NativeRedundantControl {
                            sequence,
                            view: view as u8, // at most MAX_LOCAL_PLAYERS views
                            forward: c.forward,
                            right: c.right,
                            yaw: c.yaw,
                            pitch: c.pitch,
                            flags: c.flags,
                            zoom: c.zoom,
                        })
                })
                .collect();
            frame.nonce = nonce;
            frame.cookie = cookie;
            frame.sequence = sequence;
            frame.ack_latest = ack.latest;
            frame.ack_mask = ack.mask;
            frame.ack_flags = ack.flags;
            if let Some(view) = view {
                frame.has_view = true;
                frame.view_tick = view.tick;
                frame.view_fraction = view.fraction;
            }
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
        NativeRequest::TeamChange {nonce,cookie,sequence,view,team} => {
            frame.kind=19;frame.nonce=nonce;frame.cookie=cookie;frame.sequence=sequence;
            frame.team_view=view;frame.team=team;
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
