//! Account client ABI and native IO only. The safe core owns every decision.
use crate::ffi;
use halo2_online_protocol::{matchmaking_account_client as core, matchmaking_origin::UrlParts};
pub struct AccountClient(core::Client);
fn protect<T>(f: impl FnOnce() -> core::Result<T>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(f))
        .map_err(|_| "Invalid ONLINE response".to_owned())?
        .map_err(|e| e.to_string())
}
pub fn new_account_client(
    c: &ffi::MatchmakingClientConfig,
    steam: bool,
    parties: bool,
    profile_only: bool,
) -> Box<AccountClient> {
    Box::new(AccountClient(core::Client::new(core::Config {
        input_valid: c.input_valid,
        endpoint: c.endpoint.clone(),
        url: UrlParts {
            parsed: c.url_parsed,
            normalized: c.url_normalized.clone(),
            scheme: c.url_scheme.clone(),
            host: c.url_host.clone(),
            user: c.url_user.clone(),
            password: c.url_password.clone(),
            options: c.url_options.clone(),
            path: c.url_path.clone(),
        },
        loopback: c.loopback,
        login_present: c.login_present,
        steam,
        poll_ms: c.poll_ms,
        parties,
        profile_only,
    })))
}
struct Io<'a>(&'a ffi::NativeAccountClientIo);
fn io<T>(v: Result<T, cxx::Exception>) -> core::Result<T> {
    v.map_err(|e| core::Error::Transport(e.to_string()))
}
impl core::Io for Io<'_> {
    fn lock(&self) -> core::Result<()> {
        io(self.0.account_lock())
    }
    fn unlock(&self) -> core::Result<()> {
        io(self.0.account_unlock())
    }
    fn now(&self) -> i64 {
        self.0.account_now_ns()
    }
    fn identity(&self) -> core::Result<Option<core::Identity>> {
        let id = io(self.0.account_identity())?;
        Ok(id.present.then_some(core::Identity {
            bearer: id.bearer,
            subject: id.subject,
        }))
    }
    fn request(
        &self,
        method: &str,
        url: &str,
        body: &str,
        bearer: &str,
        cleanup: bool,
    ) -> core::Result<core::Reply> {
        let response = io(self.0.account_request(&ffi::MatchmakingClientRequest {
            version: 0,
            method: method.into(),
            url: url.into(),
            body: body.into(),
            bearer: bearer.into(),
            cleanup,
        }))?;
        Ok(core::Reply {
            status: response.status,
            body: response.body,
        })
    }
    fn cancelled(&self) -> bool {
        self.0.account_cancelled()
    }
    fn reset_cancellation(&self) -> core::Result<()> {
        io(self.0.account_reset_cancellation())
    }
    fn cancel(&self) {
        self.0.account_cancel()
    }
    fn wake(&self) {
        self.0.account_wake()
    }
    fn entropy(&self) -> core::Result<[u8; 16]> {
        io(self.0.account_entropy())?
            .try_into()
            .map_err(|_| core::Error::Invalid)
    }
    fn eos_profiles(&self) -> core::Result<bool> {
        io(self.0.account_eos_profiles())
    }
    fn eos_identity(&self, token: &str) -> core::Result<(String, String)> {
        let v = io(self.0.account_eos_identity(token))?;
        Ok((v.user, v.token))
    }
}
pub fn account_client_configured(c: &AccountClient) -> bool {
    c.0.configured()
}
pub fn account_client_active(c: &AccountClient) -> bool {
    c.0.active()
}
pub fn account_client_pending(c: &AccountClient) -> bool {
    c.0.pending()
}
pub fn account_client_cadence_ms(c: &AccountClient) -> i64 {
    c.0.cadence_ms()
}
pub fn account_client_endpoint(c: &AccountClient) -> String {
    c.0.endpoint().into()
}
/// Trusted presentation projection of already validated Rust-owned state.
/// This contains no authorization, private subject, link or join secret.
pub fn account_client_snapshot(c: &AccountClient) -> String {
    c.0.snapshot().to_string()
}
pub fn account_client_connect(c: &AccountClient, name: &str, v: &ffi::NativeAccountClientIo) {
    c.0.connect(name.into(), &Io(v))
}
pub fn account_client_sign_out(c: &AccountClient, v: &ffi::NativeAccountClientIo) {
    c.0.sign_out(&Io(v))
}
pub fn account_client_profile(
    c: &AccountClient,
    bytes: &[u8],
    tag: &str,
    v: &ffi::NativeAccountClientIo,
) -> Result<(), String> {
    protect(|| {
        c.0.profile(
            bytes.try_into().map_err(|_| core::Error::Invalid)?,
            tag,
            &Io(v),
        );
        Ok(())
    })
}
pub fn account_client_clan_command(
    c: &AccountClient,
    action: &str,
    value: &str,
    role: u32,
    confirmed: bool,
    v: &ffi::NativeAccountClientIo,
) -> Result<bool, String> {
    protect(|| c.0.clan_command(action, value, role, confirmed, &Io(v)))
}
pub fn account_client_party_command(
    c: &AccountClient,
    p: &ffi::AccountClientPartyCommand,
    activity: u32,
    v: &ffi::NativeAccountClientIo,
) -> Result<u64, String> {
    protect(|| {
        c.0.party_command(
            core::PartyCommand {
                action: p.action.clone(),
                value: p.value.clone(),
                playlist: p.playlist.clone(),
                revision: p.revision.clone(),
                version: p.version,
                privacy: p.privacy_present.then_some(p.privacy),
                privacy_revision: p.privacy_revision,
                privacy_context: p.privacy_context.clone(),
                join_context: p.join_context.clone(),
            },
            activity,
            &Io(v),
        )
    })
}
pub fn account_client_friend_command(
    c: &AccountClient,
    action: &str,
    value: &str,
    v: &ffi::NativeAccountClientIo,
) -> Result<bool, String> {
    protect(|| c.0.friend_command(action, value, &Io(v)))
}
pub fn account_client_refresh_steam(c: &AccountClient, v: &ffi::NativeAccountClientIo) {
    c.0.refresh_steam(&Io(v))
}
pub fn account_client_poll(
    c: &AccountClient,
    v: &ffi::NativeAccountClientIo,
) -> Result<(), String> {
    protect(|| c.0.poll(&Io(v)))
}
pub fn account_client_shutdown(c: &AccountClient, v: &ffi::NativeAccountClientIo) {
    c.0.shutdown(&Io(v))
}
fn link(value: Option<core::Link>) -> ffi::AccountClientLink {
    let present = value.is_some();
    let v = value.unwrap_or_default();
    ffi::AccountClientLink {
        present,
        party_id: v.party_id,
        join_secret: v.join_secret,
        version: v.version,
        current_size: v.current_size,
        max_size: v.max_size,
        expires_ns: v.expires,
    }
}
pub fn account_client_voice(
    c: &AccountClient,
    party: bool,
    v: &ffi::NativeAccountClientIo,
) -> Result<ffi::AccountClientVoice, String> {
    protect(|| {
        let value = c.0.voice(party, &Io(v))?;
        let present = value.is_some();
        let a = value.unwrap_or_default();
        Ok(ffi::AccountClientVoice {
            present,
            origin: a.origin,
            bearer: a.bearer,
            channel: a.channel,
            player: a.player,
            loopback: a.loopback,
            party: a.party,
        })
    })
}
pub fn account_client_discord_operation(
    c: &AccountClient,
    serial: u64,
    v: &ffi::NativeAccountClientIo,
) -> Result<ffi::AccountClientOperation, String> {
    protect(|| {
        let a = c.0.discord_operation(serial, &Io(v))?;
        Ok(ffi::AccountClientOperation {
            serial: a.serial,
            pending: a.pending,
            successful: a.successful,
            link: link(a.link),
        })
    })
}
pub fn account_client_discord_link(
    c: &AccountClient,
    v: &ffi::NativeAccountClientIo,
) -> Result<ffi::AccountClientLink, String> {
    protect(|| c.0.discord_link(&Io(v)).map(link))
}
pub fn account_party_follower(leader: bool, members: usize) -> bool {
    core::postgame_follower(leader, members)
}
pub fn account_party_leader_left_postgame(
    leader: bool,
    members: &[ffi::AccountClientPostgameMember],
) -> bool {
    core::postgame_leader_left(
        leader,
        members.len(),
        members.iter().map(|m| {
            (
                m.is_self,
                m.leader,
                m.postgame_present.then_some(m.postgame),
            )
        }),
    )
}
