use crate::{ffi, matchmaking_frontend_party_flow::FrontendPartyFlow};
use halo2_online_protocol::{
    matchmaking_frontend_account_flow::Entry, matchmaking_frontend_signin as core,
};
pub(crate) fn encoded_state(s: core::State) -> ffi::SigninFlowState {
    ffi::SigninFlowState {
        keys: s
            .keys
            .into_iter()
            .map(|(name, key)| ffi::SigninFlowKey { name, key })
            .collect(),
        bound: s.bound,
        suppressed: s.suppressed,
        awaiting: s.awaiting,
        pending_screen: s.pending_screen,
        pending_context: s.pending_context,
    }
}
fn frame(f: ffi::SigninFlowFrame) -> core::Frame {
    core::Frame {
        screen: f.screen,
        context: f.context,
        selected: f.selected,
        instance: f.instance,
        pane_clock: f.pane_clock,
        description: f.description,
        entries: f
            .entries
            .into_iter()
            .map(|e| Entry {
                label: e.label,
                destination: e.destination,
                value: e.value,
            })
            .collect(),
    }
}
struct Io<'a>(&'a ffi::NativeFrontendSigninIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.signin_flow_facts().map_err(|e| e.to_string())?;
        Ok(core::Facts {
            login: f.login,
            account: f.account,
            queue: f.queue,
            voice: f.voice,
            social: f.social,
            directory: f.directory,
            mode: f.mode,
            signed_in: f.signed_in,
            keyboard: f.keyboard,
            frames: f.frames.into_iter().map(frame).collect(),
        })
    }
    fn random(&self) -> core::Result<Vec<u8>> {
        self.0.signin_flow_random().map_err(|e| e.to_string())
    }
    fn steam(&self) -> core::Result<bool> {
        self.0.signin_flow_steam().map_err(|e| e.to_string())
    }
    fn authorized(&self) -> core::Result<bool> {
        self.0.signin_flow_authorized().map_err(|e| e.to_string())
    }
    fn snapshot(&self) -> core::Result<core::Snapshot> {
        let s = self.0.signin_flow_snapshot().map_err(|e| e.to_string())?;
        Ok(core::Snapshot {
            status: s.status,
            identity: s.identity,
        })
    }
    fn identity(&self) -> core::Result<bool> {
        self.0.signin_flow_identity().map_err(|e| e.to_string())
    }
    fn profile_name(&self) -> core::Result<Vec<u8>> {
        self.0.signin_flow_profile_name().map_err(|e| e.to_string())
    }
    fn build(&self, s: u32, c: u32) -> core::Result<core::Frame> {
        self.0
            .signin_flow_build(s, c)
            .map(frame)
            .map_err(|e| e.to_string())
    }
    fn effect(&self, e: core::Effect) -> core::Result<()> {
        use core::Effect::*;
        let mut out = ffi::SigninFlowEffect {
            kind: 0,
            index: 0,
            a: 0,
            b: 0,
            instance: 0,
            pane_clock: 0,
            text: vec![],
        };
        out.kind = match e {
            Save => 0,
            Bind(t) => {
                out.text = t;
                1
            }
            ResetPrior => 2,
            SignOutAccount => 3,
            ClearAccount => 4,
            SignOutQueue => 5,
            StopSearch => 6,
            LeaveVoice => 7,
            DisconnectSocial => 8,
            UpdateCouch => 9,
            UpdateFriends => 10,
            UpdateAccount => 11,
            UpdatePrivacy => 12,
            UpdateLeave => 13,
            UpdatePostgame => 14,
            Restore => 15,
            Start => 16,
            Cancel => 17,
            SignOutLogin => 18,
            ConnectAccount => 19,
            ClearCouch => 20,
            CancelDirectory => 21,
            ClearDirectory => 22,
            ClearNavigation => 23,
            QuickOff => 24,
            ResetProgress => 25,
            EnterLobby => 26,
            Open(s, c) => {
                out.a = s;
                out.b = c;
                27
            }
            Pop => 28,
            Unavailable(t) => {
                out.text = t;
                29
            }
            KeyboardProfile => 30,
            KeyboardTag => 31,
            CommitBuilt {
                index,
                instance,
                pane_clock,
                selected,
            } => {
                out.index = index;
                out.instance = instance;
                out.pane_clock = pane_clock;
                out.a = selected;
                32
            }
            DiscardBuilt => 33,
            Revision => 34,
            CancelQueue => 35,
        };
        self.0.signin_flow_effect(&out).map_err(|e| e.to_string())
    }
}
pub fn frontend_signin_state(c: &FrontendPartyFlow) -> ffi::SigninFlowState {
    encoded_state(c.controller().signin_state())
}
pub fn frontend_signin_load_key(c: &FrontendPartyFlow, name: &[u8], key: &[u8]) {
    core::load_key(c.controller(), name.to_vec(), key.to_vec());
}
pub fn frontend_signin_take_key(c: &FrontendPartyFlow, name: &[u8]) -> ffi::SigninFlowResult {
    let k = core::take_key(c.controller(), name);
    ffi::SigninFlowResult {
        handled: false,
        screen: 0,
        context: 0,
        present: k.is_some(),
        text: k.unwrap_or_default(),
    }
}
pub fn frontend_signin_flow(
    c: &FrontendPartyFlow,
    action: u32,
    arg: &ffi::SigninFlowArgument,
    io: &ffi::NativeFrontendSigninIo,
) -> core::Result<ffi::SigninFlowResult> {
    let io = Io(io);
    let mut out = ffi::SigninFlowResult {
        handled: false,
        screen: 0,
        context: 0,
        present: false,
        text: vec![],
    };
    match action {
        0 => out.text = core::key(c.controller(), &io, &arg.text)?,
        1 => core::bind(c.controller(), &io)?,
        2 => core::update(c.controller(), &io)?,
        3 => {
            let (s, context) = core::prepare_open(c.controller(), &io, arg.screen, arg.context)?;
            out.screen = s;
            out.context = context;
        }
        4 => out.handled = core::accept(c.controller(), &io)?,
        5 => core::back(c.controller(), &io)?,
        6 => core::return_online(c.controller(), &io, &arg.text)?,
        7 => core::return_lobby(c.controller(), &io, &arg.text)?,
        8 => core::restore_key(c.controller(), &io, arg.flag.then(|| arg.text.clone()))?,
        _ => return Err("Invalid native sign-in flow action".into()),
    }
    Ok(out)
}
