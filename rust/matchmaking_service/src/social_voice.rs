//! Typed socket lifetimes around the existing Rust social voice owner.
use super::*;
use std::{
    collections::BTreeMap,
    sync::{
        Arc,
        atomic::{AtomicU16, Ordering},
    },
};
use tokio::sync::Notify;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum VoiceKind {
    Clan,
    Party,
}
pub struct VoiceGrant {
    kind: VoiceKind,
    header: String,
    auth: Value,
}

#[cfg(test)]
mod tests {
    use super::*;
    use rusqlite::hooks::{AuthAction, AuthContext, Authorization, TransactionOperation};
    use std::{path::PathBuf, sync::atomic::AtomicUsize};
    fn uuid() -> String {
        let mut bytes = [0; 16];
        getrandom::getrandom(&mut bytes).unwrap();
        bytes[6] = (bytes[6] & 15) | 64;
        bytes[8] = (bytes[8] & 63) | 128;
        let text = bytes.iter().map(|b| format!("{b:02x}")).collect::<String>();
        format!(
            "{}-{}-{}-{}-{}",
            &text[..8],
            &text[8..12],
            &text[12..16],
            &text[16..20],
            &text[20..]
        )
    }
    struct Fixture {
        runtime: Option<Runtime>,
        root: PathBuf,
        headers: [String; 2],
    }
    impl Fixture {
        fn new() -> Self {
            let root = PathBuf::from(
                std::env::var_os("HALO2_TEST_SCRATCH")
                    .expect("Run service tests through tools/run_test.py"),
            )
            .join(format!("voice-runtime-{}", uuid()));
            std::fs::create_dir(&root).unwrap();
            let mut rt = Runtime::open(
                &root.join("registry.sqlite3"),
                true,
                Some(include_bytes!(
                    "../../../multiplayer/master_server/examples/matchmaking-online.json"
                )),
            )
            .unwrap();
            let headers = std::array::from_fn(|i| {
                let identity = Identity {
                    provider: "discord".into(),
                    subject: format!("{}", 111111111111111111u64 + i as u64),
                    username: format!("voice_{i}"),
                    steam_digest: None,
                };
                let reply = rt
                    .login(identity, json!({"screen_name":format!("Voice {i}")}))
                    .unwrap();
                format!("Bearer {}", reply["token"].as_str().unwrap())
            });
            let command = |action, extra: Value| {
                let mut body = json!({"action":action,"request_id":uuid()});
                body.as_object_mut()
                    .unwrap()
                    .extend(extra.as_object().unwrap().clone());
                body
            };
            rt.clan(
                &headers[0],
                Some(command("create", json!({"name":"Voice Test"}))),
                json!([]),
            )
            .unwrap();
            rt.clan(
                &headers[0],
                Some(command("invite", json!({"username":"voice_1"}))),
                json!([]),
            )
            .unwrap();
            let invite =
                rt.clan(&headers[1], None, json!([])).unwrap()["invitations"][0]["id"].clone();
            rt.clan(
                &headers[1],
                Some(command("accept", json!({"target":invite}))),
                json!([]),
            )
            .unwrap();
            Self {
                runtime: Some(rt),
                root,
                headers,
            }
        }
        fn rt(&mut self) -> &mut Runtime {
            self.runtime.as_mut().unwrap()
        }
        fn bind(&mut self, i: usize) -> VoiceBinding {
            let header = self.headers[i].clone();
            let grant = self.rt().voice_authorize(VoiceKind::Clan, header).unwrap();
            self.rt().voice_attach(grant).unwrap()
        }
        fn deny_commit(&mut self) -> Arc<AtomicUsize> {
            let count = Arc::new(AtomicUsize::new(0));
            let observed = count.clone();
            self.rt()
                .db
                .authorizer(Some(move |ctx: AuthContext<'_>| {
                    // The pinned rusqlite0.41 decoder maps COMMIT to Unknown.
                    if matches!(
                        ctx.action,
                        AuthAction::Transaction {
                            operation: TransactionOperation::Unknown
                        }
                    ) {
                        observed.fetch_add(1, Ordering::SeqCst);
                        Authorization::Deny
                    } else {
                        Authorization::Allow
                    }
                }))
                .unwrap();
            count
        }
        fn allow_commit(&mut self) {
            self.rt()
                .db
                .authorizer(None::<fn(AuthContext<'_>) -> Authorization>)
                .unwrap();
        }
    }
    impl Drop for Fixture {
        fn drop(&mut self) {
            self.runtime.take();
            let _ = std::fs::remove_dir_all(&self.root);
        }
    }
    #[test]
    fn actual_failed_commits_cannot_publish_bindings_frames_or_consume_delivery() {
        let mut f = Fixture::new();
        let header = f.headers[0].clone();
        let grant = f.rt().voice_authorize(VoiceKind::Clan, header).unwrap();
        let denied = f.deny_commit();
        assert!(f.rt().voice_attach(grant).is_err());
        assert_eq!(denied.load(Ordering::SeqCst), 1);
        f.allow_commit();
        assert!(f.rt().voices.links.is_empty());
        let a = f.bind(0);
        let b = f.bind(1);
        let packet = vec![1, 0, 0, 0, 1, 0x78, 0];
        let denied = f.deny_commit();
        assert!(f.rt().voice_frame(a.clone(), true, packet.clone()).is_err());
        assert_eq!(denied.load(Ordering::SeqCst), 1);
        f.allow_commit();
        assert!(f.rt().voice_poll(b.clone()).unwrap()["packet"].is_null());
        f.rt().voice_frame(a, true, packet.clone()).unwrap();
        let denied = f.deny_commit();
        assert!(f.rt().voice_poll(b.clone()).is_err());
        assert_eq!(denied.load(Ordering::SeqCst), 1);
        f.allow_commit();
        let got = f.rt().voice_poll(b.clone()).unwrap();
        let bytes = halo2_online_protocol::matchmaking_model::hex(
            &got["packet"],
            got["packet"].as_str().unwrap().len() / 2,
        )
        .unwrap();
        assert_eq!(&bytes[17..], &packet[1..]);
        assert!(f.rt().voice_poll(b).unwrap()["packet"].is_null());
    }
    #[test]
    fn queued_audio_rechecks_actual_sql_blocks_and_old_cleanup_preserves_replacement() {
        let mut f = Fixture::new();
        let a = f.bind(0);
        let b = f.bind(1);
        f.rt()
            .voice_frame(a.clone(), true, vec![1, 0, 0, 0, 1, 0x78, 0])
            .unwrap();
        f.rt().db.execute("INSERT INTO voice_blocks SELECT player,'111111111111111111' FROM player_identities WHERE provider='discord' AND subject='111111111111111112'",[]).unwrap();
        assert!(f.rt().voice_poll(b.clone()).unwrap()["packet"].is_null());
        f.rt().db.execute("DELETE FROM voice_blocks", []).unwrap();
        let newer = f.bind(0);
        f.rt().voice_detach(a).unwrap();
        f.rt()
            .voice_frame(newer, true, vec![1, 0, 0, 0, 1, 0x78, 0])
            .unwrap();
        assert!(f.rt().voice_poll(b).unwrap()["packet"].is_string());
    }
}
#[derive(Clone)]
pub struct VoiceBinding {
    kind: VoiceKind,
    peer: String,
    pub wake: Arc<Notify>,
    closed: Arc<AtomicU16>,
}
impl VoiceBinding {
    pub fn closed(&self) -> bool {
        self.closed.load(Ordering::Acquire) != 0
    }
    pub fn retire(&self) {
        let _ = self
            .closed
            .compare_exchange(0, 1000, Ordering::AcqRel, Ordering::Acquire);
        self.wake.notify_one();
    }
    pub fn close_code(&self) -> u16 {
        self.closed.load(Ordering::Acquire)
    }
}
pub(super) struct Voices {
    clan: u64,
    party: u64,
    links: BTreeMap<(VoiceKind, String), VoiceBinding>,
}
impl Voices {
    pub(super) fn new(owners: &mut Owners, accounts: u64, queue: Option<u64>) -> Result<Self> {
        let mut open = |kind| {
            owners.open(
                &json!({"social_voice":{"kind":kind,"account_source":accounts,"source":queue}}),
            )
        };
        Ok(Self {
            clan: open("clan")?,
            party: open("party")?,
            links: BTreeMap::new(),
        })
    }
    fn source(&self, kind: VoiceKind) -> u64 {
        match kind {
            VoiceKind::Clan => self.clan,
            VoiceKind::Party => self.party,
        }
    }
}
impl Runtime {
    fn voice_config(
        &self,
        kind: VoiceKind,
        mode: &str,
        auth: Option<&Value>,
        body: Value,
    ) -> Result<Value> {
        let clock = self.clock()?;
        let mut setup = self.config(
            "voice",
            mode,
            auth.and_then(|a| a["player"].as_str()),
            auth.and_then(|a| a["key"].as_str()),
            body,
            Value::Null,
            &clock,
        );
        let row = &mut setup["party_directory"];
        row["voice_source"] = json!(self.voices.source(kind));
        row["voice_now"] = json!(clock.monotonic);
        row["voice_account_now"] = json!(clock.monotonic);
        Ok(setup)
    }
    pub(super) fn voice_schema(&mut self) -> Result<()> {
        for kind in [VoiceKind::Clan, VoiceKind::Party] {
            let setup = self.voice_config(kind, "relay_schema", None, Value::Null)?;
            self.plan(setup)?;
        }
        Ok(())
    }
    pub fn voice_preferences(
        &mut self,
        identity: Identity,
        body: Value,
        deadline: Instant,
    ) -> Result<Value> {
        let mut setup = self.config(
            "voice",
            "update",
            None,
            None,
            body,
            Value::Null,
            &self.clock()?,
        );
        setup["party_directory"]["verified_owner"] =
            json!({"provider":identity.provider,"subject":identity.subject});
        self.plan_reply_checked(setup, None, None, Some((identity, deadline)))
    }
    pub fn voice_authorize(&mut self, kind: VoiceKind, header: String) -> Result<VoiceGrant> {
        let auth = self.session(&header)?;
        let setup = self.voice_config(kind, "preflight", Some(&auth), Value::Null)?;
        self.plan(setup)?;
        Ok(VoiceGrant { kind, header, auth })
    }
    pub fn voice_attach(&mut self, grant: VoiceGrant) -> Result<VoiceBinding> {
        let auth = self.session(&grant.header)?;
        if auth != grant.auth {
            return Err(Error(401, "ONLINE voice capability changed"));
        }
        let setup = self.voice_config(grant.kind, "attach", Some(&auth), Value::Null)?;
        let reply = self.plan(setup)?;
        let peer = reply["peer"]
            .as_str()
            .ok_or(Error(503, "Native voice binding missing"))?
            .to_owned();
        if let Some(old) = reply["replaced"].as_str() {
            if let Some(binding) = self.voices.links.get(&(grant.kind, old.to_owned())) {
                binding.wake.notify_one();
            }
        }
        let binding = VoiceBinding {
            kind: grant.kind,
            peer,
            wake: Arc::new(Notify::new()),
            closed: Arc::new(AtomicU16::new(0)),
        };
        self.voices
            .links
            .insert((binding.kind, binding.peer.clone()), binding.clone());
        Ok(binding)
    }
    fn voice_call(&mut self, binding: &VoiceBinding, mode: &str, mut body: Value) -> Result<Value> {
        body["peer"] = json!(binding.peer);
        let setup = self.voice_config(binding.kind, mode, None, body)?;
        let result = self.plan(setup)?;
        if let Some(wake) = result["wake"].as_array() {
            for peer in wake {
                if let Some(peer) = peer.as_str() {
                    if let Some(listener) = self.voices.links.get(&(binding.kind, peer.to_owned()))
                    {
                        listener.wake.notify_one();
                    }
                }
            }
        }
        Ok(result)
    }
    pub fn voice_frame(
        &mut self,
        binding: VoiceBinding,
        binary: bool,
        packet: Vec<u8>,
    ) -> Result<Value> {
        let packet = packet
            .iter()
            .map(|b| format!("{b:02x}"))
            .collect::<String>();
        self.voice_call(&binding, "frame", json!({"binary":binary,"packet":packet}))
    }
    pub fn voice_poll(&mut self, binding: VoiceBinding) -> Result<Value> {
        self.voice_call(&binding, "poll", json!({}))
    }
    pub fn voice_detach(&mut self, binding: VoiceBinding) -> Result<()> {
        binding.retire();
        self.voice_call(&binding, "detach", json!({}))?;
        self.voices.links.remove(&(binding.kind, binding.peer));
        Ok(())
    }
    pub(super) fn voice_maintenance(&mut self) -> Result<()> {
        let retired = self
            .voices
            .links
            .values()
            .filter(|b| b.closed())
            .cloned()
            .collect::<Vec<_>>();
        for binding in retired {
            self.voice_detach(binding)?;
        }
        Ok(())
    }
    pub fn voice_shutdown(&mut self) {
        for binding in self.voices.links.values() {
            binding.closed.store(1001, Ordering::Release);
            binding.wake.notify_one();
        }
    }
    pub fn voice_connections(&self) -> usize {
        self.voices.links.len()
    }
}
