use crate::{ffi, matchmaking_frontend_party_flow::FrontendPartyFlow};
use halo2_online_protocol::{
    matchmaking_frontend_account_flow::Entry, matchmaking_frontend_couch as core,
};
pub(crate) fn encoded_state(s: core::State) -> ffi::CouchFlowState {
    ffi::CouchFlowState {
        subjects: s
            .subjects
            .into_iter()
            .map(|value| ffi::PartyBytesData { value })
            .collect(),
        options_present: s.options.is_some(),
        options: s.options.unwrap_or(0),
        joining_present: s.joining.is_some(),
        joining: s.joining.unwrap_or(0),
        joining_online: s.joining_online,
        host_profile: s.host_profile,
        revision: s.revision,
    }
}
fn entry(e: ffi::AccountFlowEntry) -> Entry {
    Entry {
        label: e.label,
        destination: e.destination,
        value: e.value,
    }
}
fn encoded_entry(e: Entry) -> ffi::AccountFlowEntry {
    ffi::AccountFlowEntry {
        label: e.label,
        destination: e.destination,
        value: e.value,
    }
}
fn frame(f: ffi::CouchFlowFrame) -> core::Frame {
    core::Frame {
        screen: f.screen,
        context: f.context,
        selected: f.selected,
        instance: f.instance,
        description: f.description,
        entries: f.entries.into_iter().map(entry).collect(),
    }
}
fn login(l: core::Login) -> (u32, u32) {
    match l {
        core::Login::Primary => (0, 0),
        core::Login::Local(i) => (1, i),
        core::Login::Held => (2, 0),
    }
}
struct Io<'a>(&'a ffi::NativeFrontendCouchIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.couch_flow_facts().map_err(|e| e.to_string())?;
        Ok(core::Facts {
            status: f.status,
            mode: f.mode,
            primary_login: f.primary_login,
            account_client: f.account_client,
            authenticated: f.authenticated,
            factory: f.factory,
            local_logins: f.local_logins,
            local_accounts: f.local_accounts,
            profiles: f.profiles.into_iter().map(|p| p.value).collect(),
            inputs: f.inputs,
            profile: f.profile,
            signed_in: f.signed_in,
            system_link_host: f.system_link_host,
            countdown: f.countdown_present.then_some(f.countdown),
            frame: f.frame_present.then(|| frame(f.frame)),
            frames: f.frames.into_iter().map(frame).collect(),
        })
    }
    fn hold_login(&self, i: u32) -> core::Result<bool> {
        self.0.couch_flow_hold(i).map_err(|e| e.to_string())
    }
    fn release_login(&self) -> core::Result<()> {
        self.0.couch_flow_release().map_err(|e| e.to_string())
    }
    fn snapshot(&self, l: core::Login) -> core::Result<core::Snapshot> {
        let (k, i) = login(l);
        let s = self
            .0
            .couch_flow_snapshot(k, i)
            .map_err(|e| e.to_string())?;
        Ok(core::Snapshot {
            status: s.status,
            identity: s.identity_present.then_some(s.identity),
        })
    }
    fn authorized(&self, l: core::Login) -> core::Result<bool> {
        let (k, i) = login(l);
        self.0
            .couch_flow_authorized(k, i)
            .map_err(|e| e.to_string())
    }
    fn steam(&self, l: core::Login) -> core::Result<bool> {
        let (k, i) = login(l);
        self.0.couch_flow_steam(k, i).map_err(|e| e.to_string())
    }
    fn cancel(&self, l: core::Login) -> core::Result<()> {
        let (k, i) = login(l);
        self.0.couch_flow_cancel(k, i).map_err(|e| e.to_string())
    }
    fn identity(&self) -> core::Result<bool> {
        self.0.couch_flow_identity().map_err(|e| e.to_string())
    }
    fn description(&self) -> core::Result<Vec<u8>> {
        self.0.couch_flow_description().map_err(|e| e.to_string())
    }
    fn build(&self) -> core::Result<core::Frame> {
        self.0
            .couch_flow_build()
            .map(frame)
            .map_err(|e| e.to_string())
    }
    fn players(&self) -> core::Result<Vec<u32>> {
        self.0.couch_flow_players().map_err(|e| e.to_string())
    }
    fn effect(&self, e: core::Effect) -> core::Result<()> {
        use core::Effect::*;
        let mut out = ffi::CouchFlowEffect {
            kind: 0,
            index: 0,
            value: 0,
            a: 0,
            b: 0,
            instance: 0,
            text: vec![],
            entry: ffi::AccountFlowEntry {
                label: vec![],
                destination: 0,
                value: 0,
            },
            entries: vec![],
        };
        out.kind = match e {
            Title(t) => {
                out.text = t;
                0
            }
            Description(t) => {
                out.text = t;
                1
            }
            Replace(rows) => {
                out.entries = rows.into_iter().map(encoded_entry).collect();
                2
            }
            Append(e) => {
                out.entry = encoded_entry(e);
                3
            }
            Open(a, b) => {
                out.a = a;
                out.b = b;
                4
            }
            Back => 5,
            Pop => 6,
            Revision => 7,
            Unavailable(t) => {
                out.text = t;
                8
            }
            LocalProfile(i, t) => {
                out.index = i;
                out.text = t;
                9
            }
            LocalInput(i, v) => {
                out.index = i;
                out.value = v;
                10
            }
            ResetLocalTeam(i) => {
                out.index = i;
                11
            }
            Profile(i) => {
                out.index = i;
                12
            }
            SignOutAccount(i) => {
                out.index = i;
                13
            }
            ResetAccount(i) => {
                out.index = i;
                14
            }
            ClearAccounts => 15,
            ResetLogin(i) => {
                out.index = i;
                16
            }
            CreateLogin(i) => {
                out.index = i;
                17
            }
            BindLogin(i) => {
                out.index = i;
                18
            }
            StartLogin(i) => {
                out.index = i;
                19
            }
            CreateAccount(i) => {
                out.index = i;
                20
            }
            ProfileAccount(i) => {
                out.index = i;
                21
            }
            ConnectAccount(i) => {
                out.index = i;
                22
            }
            DescriptionIndex(i, t) => {
                out.index = i;
                out.text = t;
                23
            }
            CommitBuilt(instance, selected) => {
                out.instance = instance;
                out.a = selected;
                24
            }
            DiscardBuilt => 25,
            EmitLocal(i) => {
                out.index = i;
                26
            }
            ProfileFromCurrent(i) => {
                out.index = i;
                27
            }
        };
        self.0.couch_flow_effect(&out).map_err(|e| e.to_string())
    }
}
fn result() -> ffi::CouchFlowResult {
    ffi::CouchFlowResult {
        handled: true,
        present: false,
        index: 0,
        text: vec![],
    }
}
pub fn frontend_couch_state(c: &FrontendPartyFlow) -> ffi::CouchFlowState {
    encoded_state(c.controller().couch_state())
}
pub fn frontend_couch_changed(c: &FrontendPartyFlow) {
    c.controller().couch_changed();
}
pub fn frontend_couch_selected_controller(c: &FrontendPartyFlow) -> ffi::CouchFlowResult {
    let mut out = result();
    let value = core::selected_controller(c.controller());
    out.present = value.is_some();
    out.index = value.unwrap_or(0);
    out
}
pub fn frontend_couch_guest_entry(c: &FrontendPartyFlow, screen: u32) -> bool {
    core::guest_entry(c.controller(), screen)
}
pub fn frontend_couch_screen(s: u32, c: u32) -> bool {
    core::screen(s, c)
}
pub fn frontend_couch_flow(
    c: &FrontendPartyFlow,
    action: u32,
    arg: &ffi::CouchFlowArgument,
    io: &ffi::NativeFrontendCouchIo,
) -> core::Result<ffi::CouchFlowResult> {
    let flow = c.controller();
    let io = Io(io);
    let mut out = result();
    match action {
        0 => out.handled = core::mutable(&io)?,
        1 => core::open(flow, &io, arg.a)?,
        2 => core::populate(flow, &io)?,
        3 => out.text = core::text(flow, &io, arg.a)?,
        4 => out.handled = core::accept(flow, &io)?,
        5 => core::update(flow, &io)?,
        6 => core::clear(flow, &io)?,
        7 => core::local_players(&io)?,
        8 => {
            let value = core::controller_slot(flow, &io, arg.signed_value, arg.flag)?;
            out.present = value.is_some();
            out.index = value.unwrap_or(0);
        }
        9 => core::finish(flow, &io)?,
        10 => core::leave(flow, &io, arg.a)?,
        11 => out.handled = core::back(flow, &io)?,
        12 => out.handled = core::back_join(flow, &io)?,
        13 => out.handled = core::identity(&io)?,
        14 => out.handled = core::profile_entry(flow, &io, arg.a, &arg.text)?,
        _ => return Err("Invalid native couch flow action".into()),
    }
    Ok(out)
}
