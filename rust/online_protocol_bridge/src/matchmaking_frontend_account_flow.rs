use crate::ffi;
use crate::matchmaking_frontend_party_flow::FrontendPartyFlow;
use halo2_online_protocol::matchmaking_frontend_account_flow as core;
fn account(f: ffi::AccountFlowAccount) -> core::Account {
    core::Account {
        revision: f.revision,
        notification: crate::matchmaking_frontend_party::notification(&f.notification),
        friends: f.friends.into_iter().map(|p| p.value).collect(),
        clan: f
            .clan
            .then(|| f.clan_members.into_iter().map(|p| p.value).collect()),
        friend_search: f.friend_search,
        command_serial: f.command_serial,
        successful: f.successful.present.then_some(f.successful.value),
        party_message: f.party_message,
    }
}
struct Io<'a>(&'a ffi::NativeFrontendAccountFlowIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.account_flow_facts().map_err(|e| e.to_string())?;
        Ok(core::Facts {
            account_client: f.account_client,
            login_client: f.login_client,
            voice_client: f.voice_client,
            queue_client: f.queue_client,
            signed_in: f.signed_in,
            tags: f.tags.into_iter().map(|p| (p.id, p.name)).collect(),
            account: account(f.account),
            voice_revision: f.voice_revision,
            frames: f
                .frames
                .into_iter()
                .map(|p| core::Frame {
                    screen: p.screen,
                    context: p.context,
                    selected: p.selected,
                    pcr: p.pcr,
                    entries: p
                        .entries
                        .into_iter()
                        .map(|e| core::Entry {
                            label: e.label,
                            destination: e.destination,
                            value: e.value,
                        })
                        .collect(),
                })
                .collect(),
            launch: f.launch,
            mode: f.mode,
            sounds: f.sounds,
        })
    }
    fn identity(&self) -> core::Result<Option<Vec<u8>>> {
        let i = self.0.account_flow_identity().map_err(|e| e.to_string())?;
        Ok(i.present.then_some(i.value))
    }
    fn profile(&self, n: &[u8], t: &[u8]) -> core::Result<()> {
        self.0.account_flow_profile(n, t).map_err(|e| e.to_string())
    }
    fn connect(&self, n: &[u8]) -> core::Result<()> {
        self.0.account_flow_connect(n).map_err(|e| e.to_string())
    }
    fn sign_out(&self) -> core::Result<()> {
        self.0.account_flow_sign_out().map_err(|e| e.to_string())
    }
    fn snapshot(&self) -> core::Result<core::Account> {
        self.0
            .account_flow_snapshot()
            .map(account)
            .map_err(|e| e.to_string())
    }
    fn stage_voice(&self, read: bool) -> core::Result<u64> {
        self.0
            .account_flow_stage_voice(read)
            .map_err(|e| e.to_string())
    }
    fn native_friends(&self) -> core::Result<bool> {
        self.0
            .account_flow_native_friends()
            .map_err(|e| e.to_string())
    }
    fn effect(&self, e: core::Effect) -> core::Result<()> {
        use core::Effect::*;
        let mut f = ffi::AccountFlowEffect {
            kind: 0,
            index: 0,
            a: 0,
            b: 0,
            text: vec![],
        };
        f.kind = match e {
            Sound(a) => {
                f.a = a;
                0
            }
            CommitAccount => 1,
            CommitVoice => 2,
            CancelQueue => 3,
            StopSearch => 4,
            ReturnOnline => 5,
            Open(a, b) => {
                f.a = a;
                f.b = b;
                6
            }
            VoiceLeave => 7,
            VoiceDrop => 8,
            VoiceCreateParty => 9,
            VoiceJoin => 10,
            VoiceRefresh => 11,
            EraseFrom(i) => {
                f.index = i as u64;
                12
            }
            ClearEntries(i) => {
                f.index = i as u64;
                13
            }
            Populate(i, k) => {
                f.index = i as u64;
                f.a = k;
                14
            }
            Select(i, k) => {
                f.index = i as u64;
                f.a = k;
                15
            }
            Revision => 16,
            Unavailable(text) => {
                f.text = text;
                17
            }
        };
        self.0.account_flow_effect(&f).map_err(|e| e.to_string())
    }
}
pub fn frontend_account_flow(
    controller: &FrontendPartyFlow,
    continuation: bool,
    io: &ffi::NativeFrontendAccountFlowIo,
) -> core::Result<()> {
    if continuation {
        core::friend_join(controller.controller(), &Io(io))
    } else {
        core::update(controller.controller(), &Io(io))
    }
}
