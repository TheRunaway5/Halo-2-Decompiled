//! One bounded verifier stream. Cancelling a waiter never retires its owner;
//! cancelling an owner retires/reaps its exact child before another exchange.
use super::*;
use halo2_online_protocol::security_policy::EosAdmissions;
use std::{process::Stdio, sync::Mutex, time::Duration};
use tokio::{
    io::{AsyncBufReadExt, AsyncWriteExt, BufReader},
    process::{Child, ChildStdin, ChildStdout, Command},
    sync::{Mutex as AsyncMutex, oneshot, watch},
};

pub struct Verified {
    pub(super) account: String,
    pub(super) puid: String,
}
impl Verified {
    pub fn account(&self) -> &str {
        &self.account
    }
    pub fn puid(&self) -> &str {
        &self.puid
    }
}
pub struct Verifier {
    path: Option<PathBuf>,
    admissions: Arc<Mutex<EosAdmissions>>,
    worker: Arc<AsyncMutex<Option<Worker>>>,
    closing: watch::Sender<bool>,
}
struct Lease {
    admissions: Arc<Mutex<EosAdmissions>>,
    principal: [u64; 4],
}
impl Drop for Lease {
    fn drop(&mut self) {
        if let Ok(mut gate) = self.admissions.lock() {
            gate.release(self.principal);
        }
    }
}
struct Worker {
    child: Child,
    input: Option<ChildStdin>,
    output: BufReader<ChildStdout>,
    retired: bool,
}
fn failure(_: impl std::fmt::Debug) -> Error {
    Error(503, "EOS verification unavailable")
}
impl Worker {
    fn start(path: &Path) -> Result<Self> {
        let mut child = Command::new(path)
            .env_remove("H2_EOS_WORKER_MODE")
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::null())
            .kill_on_drop(true)
            .spawn()
            .map_err(failure)?;
        let input = child.stdin.take().ok_or_else(|| failure(()))?;
        let output = child.stdout.take().ok_or_else(|| failure(()))?;
        Ok(Self {
            child,
            input: Some(input),
            output: BufReader::new(output),
            retired: false,
        })
    }
    async fn exchange(&mut self, puid: &str, token: &str) -> Result<String> {
        let mut bytes = json!({"op":"verify","user":puid,"token":token})
            .to_string()
            .into_bytes();
        bytes.push(b'\n');
        let input = self.input.as_mut().ok_or_else(|| failure(()))?;
        input.write_all(&bytes).await.map_err(failure)?;
        input.flush().await.map_err(failure)?;
        let mut remaining = 262144usize;
        for _ in 0..64 {
            let mut line = Vec::new();
            loop {
                let data = self.output.fill_buf().await.map_err(failure)?;
                if data.is_empty() {
                    return Err(failure(()));
                }
                let take = data
                    .iter()
                    .position(|b| *b == b'\n')
                    .map_or(data.len(), |i| i + 1);
                if take > remaining {
                    return Err(failure(()));
                }
                remaining -= take;
                line.extend_from_slice(&data[..take]);
                self.output.consume(take);
                if line.last() == Some(&b'\n') {
                    break;
                }
            }
            if !line.starts_with(b"H2EOS ") {
                continue;
            }
            let reply = matchmaking_json::parse(&line[6..], 262144).map_err(failure)?;
            match reply["ok"].as_bool() {
                Some(false) => return Err(Error(403, "EOS identity proof rejected")),
                Some(true) => {
                    m::fields(&reply, &["ok", "value"]).map_err(failure)?;
                    m::fields(&reply["value"], &["account"]).map_err(failure)?;
                    return Ok(m::text(&reply["value"]["account"], 64)
                        .map_err(failure)?
                        .to_owned());
                }
                None => return Err(failure(())),
            }
        }
        Err(failure(()))
    }
}
async fn retire(slot: &mut Option<Worker>) {
    if let Some(worker) = slot {
        worker.retired = true;
        worker.input.take();
        let _ = worker.child.start_kill();
        if tokio::time::timeout(Duration::from_secs(2), worker.child.wait())
            .await
            .is_ok_and(|r| r.is_ok())
        {
            slot.take();
        }
    }
}
impl Verifier {
    pub fn new(path: Option<PathBuf>) -> Result<Self> {
        if path
            .as_ref()
            .is_some_and(|p| !p.is_absolute() || !p.is_file())
        {
            return Err(Error(
                400,
                "EOS worker requires an absolute executable path",
            ));
        }
        Ok(Self {
            path,
            admissions: Arc::new(Mutex::new(EosAdmissions::default())),
            worker: Arc::new(AsyncMutex::new(None)),
            closing: watch::channel(false).0,
        })
    }
    pub fn configured(&self) -> bool {
        self.path.is_some()
    }
    pub async fn verify(&self, puid: String, token: String, principal: &str) -> Result<Verified> {
        let mut closing = self.closing.subscribe();
        if *closing.borrow() {
            return Err(failure(()));
        }
        let path = self
            .path
            .as_ref()
            .ok_or(Error(503, "EOS verification is not configured"))?
            .clone();
        let digest = Sha256::digest(principal.as_bytes());
        let principal = std::array::from_fn(|i| {
            u64::from_le_bytes(digest[i * 8..i * 8 + 8].try_into().unwrap())
        });
        let status = self.admissions.lock().map_err(failure)?.admit(principal);
        if status != 0 {
            return Err(Error(status, "EOS verification busy for player; retry"));
        }
        let lease = Lease {
            admissions: self.admissions.clone(),
            principal,
        };
        let mut slot =
            tokio::time::timeout(Duration::from_secs(2), self.worker.clone().lock_owned())
                .await
                .map_err(|_| Error(503, "EOS verification busy; retry"))?;
        let (mut answer, receive) = oneshot::channel();
        tokio::spawn(async move {
            let _lease = lease;
            let already_closing = *closing.borrow();
            let exchange = async {
                if already_closing {
                    return Err(failure(()));
                }
                if slot.as_ref().is_some_and(|w| w.retired) {
                    retire(&mut slot).await;
                }
                if slot.as_ref().is_some_and(|w| w.retired) {
                    return Err(failure(()));
                }
                if slot
                    .as_mut()
                    .is_some_and(|w| w.child.try_wait().map_or(true, |s| s.is_some()))
                {
                    retire(&mut slot).await;
                }
                if slot.is_none() {
                    *slot = Some(Worker::start(&path)?);
                }
                slot.as_mut().unwrap().exchange(&puid, &token).await
            };
            let result = tokio::select! {
                biased;
                _=answer.closed()=>Err(failure(())),
                _=closing.changed()=>Err(failure(())),
                result=tokio::time::timeout(Duration::from_secs(20),exchange)=>result.unwrap_or_else(|_|Err(failure(()))),
            };
            // Even an SDK rejection retires this stream: a late callback or
            // malformed provider reply must never become another user's proof.
            if result.is_err() {
                retire(&mut slot).await;
            }
            let _ = answer.send(result.map(|account| Verified { account, puid }));
        });
        receive.await.map_err(failure)?
    }
    pub async fn close(&self) {
        self.closing.send_replace(true);
        let mut slot = self.worker.lock().await;
        retire(&mut slot).await;
    }
}

#[cfg(all(test, unix))]
mod tests {
    use super::*;
    struct Fixture {
        root: PathBuf,
        path: PathBuf,
    }
    impl Fixture {
        fn new() -> Self {
            use std::os::unix::fs::PermissionsExt;
            let mut random = [0u8; 8];
            getrandom::getrandom(&mut random).unwrap();
            let root = PathBuf::from(
                std::env::var_os("HALO2_TEST_SCRATCH").expect("run_test.py storage required"),
            )
            .join(format!("eos-verifier-{}", u64::from_le_bytes(random)));
            std::fs::create_dir(&root).unwrap();
            let path = root.join("worker.py");
            let script = r#"#!/usr/bin/env python3
import json,os,sys,time
from pathlib import Path
root=Path(@ROOT@)
for line in sys.stdin:
    request=json.loads(line)
    (root/'pid').write_text(str(os.getpid()))
    with (root/'calls').open('a') as out:out.write(request['token']+'\n')
    if request['token']=='first':
        (root/'ready').write_text('ready')
        while not (root/'release').exists():time.sleep(.005)
    print('H2EOS '+json.dumps({'ok':True,'value':{'account':request['token']}}),flush=True)
"#
            .replace("@ROOT@", &json!(root.to_str().unwrap()).to_string());
            std::fs::write(&path, script).unwrap();
            std::fs::set_permissions(&path, std::fs::Permissions::from_mode(0o700)).unwrap();
            Self { root, path }
        }
        async fn ready(&self) {
            let deadline = tokio::time::Instant::now() + Duration::from_secs(3);
            while !self.root.join("ready").exists() {
                assert!(
                    tokio::time::Instant::now() < deadline,
                    "Owned worker did not receive its request"
                );
                tokio::time::sleep(Duration::from_millis(5)).await;
            }
        }
        fn pid(&self) -> String {
            std::fs::read_to_string(self.root.join("pid")).unwrap()
        }
    }
    impl Drop for Fixture {
        fn drop(&mut self) {
            let _ = std::fs::remove_dir_all(&self.root);
        }
    }
    async fn pending(v: &Verifier, count: usize) {
        let deadline = tokio::time::Instant::now() + Duration::from_secs(3);
        while v.admissions.lock().unwrap().pending() != count {
            assert!(
                tokio::time::Instant::now() < deadline,
                "Verifier admission did not settle"
            );
            tokio::time::sleep(Duration::from_millis(5)).await;
        }
    }
    #[tokio::test]
    async fn cancelled_waiter_preserves_actual_owner_stream_and_reply() {
        let fixture = Fixture::new();
        let verifier = Arc::new(Verifier::new(Some(fixture.path.clone())).unwrap());
        let v = verifier.clone();
        let owner =
            tokio::spawn(async move { v.verify("a".repeat(32), "first".into(), "owner").await });
        fixture.ready().await;
        let first_pid = fixture.pid();
        let v = verifier.clone();
        let waiting =
            tokio::spawn(async move { v.verify("b".repeat(32), "queued".into(), "waiter").await });
        pending(&verifier, 2).await;
        waiting.abort();
        assert!(matches!(waiting.await,Err(e) if e.is_cancelled()));
        pending(&verifier, 1).await;
        assert_eq!(fixture.pid(), first_pid);
        assert_eq!(
            std::fs::read_to_string(fixture.root.join("calls")).unwrap(),
            "first\n"
        );
        std::fs::write(fixture.root.join("release"), b"ready").unwrap();
        assert_eq!(owner.await.unwrap().unwrap().account(), "first");
        assert_eq!(
            verifier
                .verify("b".repeat(32), "second".into(), "waiter")
                .await
                .unwrap()
                .account(),
            "second"
        );
        assert_eq!(fixture.pid(), first_pid);
        pending(&verifier, 0).await;
        verifier.close().await;
    }
    #[tokio::test]
    async fn cancelled_owner_and_shutdown_reap_the_exact_child_before_reuse() {
        let fixture = Fixture::new();
        let verifier = Arc::new(Verifier::new(Some(fixture.path.clone())).unwrap());
        let v = verifier.clone();
        let owner =
            tokio::spawn(async move { v.verify("a".repeat(32), "first".into(), "owner").await });
        fixture.ready().await;
        let first_pid = fixture.pid();
        owner.abort();
        assert!(matches!(owner.await,Err(e) if e.is_cancelled()));
        assert_eq!(
            verifier
                .verify("b".repeat(32), "second".into(), "waiter")
                .await
                .unwrap()
                .account(),
            "second"
        );
        assert_ne!(fixture.pid(), first_pid);
        #[cfg(target_os = "linux")]
        assert!(!Path::new("/proc").join(first_pid).exists());
        std::fs::remove_file(fixture.root.join("ready")).unwrap();
        let v = verifier.clone();
        let active =
            tokio::spawn(async move { v.verify("a".repeat(32), "first".into(), "owner").await });
        fixture.ready().await;
        let second_pid = fixture.pid();
        verifier.close().await;
        assert_eq!(active.await.unwrap().err().unwrap().0, 503);
        pending(&verifier, 0).await;
        #[cfg(target_os = "linux")]
        assert!(!Path::new("/proc").join(second_pid).exists());
        assert_eq!(
            verifier
                .verify("b".repeat(32), "third".into(), "waiter")
                .await
                .err()
                .unwrap()
                .0,
            503
        );
    }
}
