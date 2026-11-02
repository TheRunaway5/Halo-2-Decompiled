//! Owned private friend-file worker IO; shared Rust owns claims and revisions.
use crate::{database, http::App};
use halo2_online_protocol::{
    matchmaking_coordinator,
    matchmaking_model::{Error, Result},
};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    fs::{self, File},
    io::Read,
    path::{Path, PathBuf},
    process::Stdio,
    sync::Arc,
    time::Duration,
};
use tokio::{
    io::{AsyncBufReadExt, AsyncWriteExt, BufReader},
    process::Command,
    sync::watch,
};

const KEYS: [&str; 7] = [
    "H2_EOS_PRODUCT_ID",
    "H2_EOS_SANDBOX_ID",
    "H2_EOS_DEPLOYMENT_ID",
    "H2_EOS_FRIEND_CLIENT_ID",
    "H2_EOS_FRIEND_CLIENT_SECRET",
    "H2_EOS_FRIEND_CACHE_DIR",
    "H2_EOS_FRIEND_ENCRYPTION_KEY",
];
fn failure(_: impl std::fmt::Debug) -> Error {
    Error(503, "Private friend storage unavailable")
}

pub struct Config {
    worker: PathBuf,
    path: PathBuf,
    cache_override: Option<PathBuf>,
}
impl Config {
    pub fn from_environment(worker: Option<PathBuf>) -> Result<Option<Self>> {
        if std::env::var("H2_EOS_FRIEND_STORAGE").as_deref() != Ok("1") {
            return Ok(None);
        }
        let value = Self {
            worker: worker.ok_or_else(|| failure(()))?,
            path: std::env::var_os("H2_EOS_FRIEND_STORAGE_CONFIG")
                .filter(|v| !v.is_empty())
                .map(PathBuf::from)
                .ok_or_else(|| failure(()))?,
            cache_override: std::env::var_os("H2_EOS_FRIEND_CACHE_DIR").map(PathBuf::from),
        };
        if !value.worker.is_absolute() || !value.worker.is_file() {
            return Err(failure(()));
        }
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            if fs::metadata(&value.worker)
                .map_err(failure)?
                .permissions()
                .mode()
                & 0o111
                == 0
            {
                return Err(failure(()));
            }
        }
        value.settings()?;
        Ok(Some(value))
    }
    fn settings(&self) -> Result<PathBuf> {
        let values = private_config(&self.path)?;
        let cache = self
            .cache_override
            .clone()
            .unwrap_or_else(|| PathBuf::from(&values["H2_EOS_FRIEND_CACHE_DIR"]));
        if !cache.is_absolute() {
            return Err(failure(()));
        }
        private_directory(&cache)?;
        Ok(cache)
    }
}
fn private_config(path: &Path) -> Result<BTreeMap<String, String>> {
    if !path.is_absolute() {
        return Err(failure(()));
    }
    #[cfg(unix)]
    let file = {
        use rustix::fs::{Mode, OFlags, open};
        File::from(
            open(
                path,
                OFlags::RDONLY | OFlags::CLOEXEC | OFlags::NOFOLLOW | OFlags::NONBLOCK,
                Mode::empty(),
            )
            .map_err(failure)?,
        )
    };
    #[cfg(not(unix))]
    let file: File = {
        let _ = path;
        return Err(failure(()));
    };
    let info = file.metadata().map_err(failure)?;
    if !info.is_file() || info.len() > 16384 {
        return Err(failure(()));
    }
    #[cfg(unix)]
    {
        use std::os::unix::fs::{MetadataExt, PermissionsExt};
        if ![0, rustix::process::geteuid().as_raw()].contains(&info.uid())
            || info.permissions().mode() & 0o077 != 0
        {
            return Err(failure(()));
        }
    }
    let mut raw = String::new();
    file.take(16385).read_to_string(&mut raw).map_err(failure)?;
    if raw.len() > 16384 {
        return Err(failure(()));
    }
    let mut values = BTreeMap::new();
    for line in raw
        .lines()
        .map(str::trim)
        .filter(|s| !s.is_empty() && !s.starts_with('#'))
    {
        let (key, value) = line.split_once('=').ok_or_else(|| failure(()))?;
        if !KEYS.contains(&key)
            || value.is_empty()
            || value.len() > 4096
            || !value.bytes().all(|b| (32..127).contains(&b))
            || values.insert(key.to_owned(), value.to_owned()).is_some()
        {
            return Err(failure(()));
        }
    }
    if values.len() != KEYS.len() {
        return Err(failure(()));
    }
    for key in &KEYS[..3] {
        let s = &values[*key];
        if s.len() != 32
            || !s
                .bytes()
                .all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b))
        {
            return Err(failure(()));
        }
    }
    let s = &values[KEYS[6]];
    if s.len() != 64
        || !s.bytes().all(|b| b.is_ascii_hexdigit())
        || !Path::new(&values[KEYS[5]]).is_absolute()
    {
        return Err(failure(()));
    }
    Ok(values)
}
fn private_directory(path: &Path) -> Result<()> {
    if !path.is_absolute() {
        return Err(failure(()));
    }
    let mut builder = fs::DirBuilder::new();
    builder.recursive(true);
    #[cfg(unix)]
    {
        use std::os::unix::fs::DirBuilderExt;
        builder.mode(0o700);
    }
    builder.create(path).map_err(failure)?;
    let info = fs::symlink_metadata(path).map_err(failure)?;
    if !info.is_dir() {
        return Err(failure(()));
    }
    #[cfg(unix)]
    {
        use std::os::unix::fs::{MetadataExt, PermissionsExt};
        if info.uid() != rustix::process::geteuid().as_raw()
            || info.permissions().mode() & 0o077 != 0
        {
            return Err(failure(()));
        }
    }
    Ok(())
}
struct Cache(PathBuf);
impl Cache {
    fn new(parent: &Path) -> Result<Self> {
        for _ in 0..16 {
            let mut random = [0; 16];
            getrandom::getrandom(&mut random).map_err(failure)?;
            let name = format!(
                "sync-{}",
                random
                    .iter()
                    .map(|b| format!("{b:02x}"))
                    .collect::<String>()
            );
            let path = parent.join(name);
            let mut builder = fs::DirBuilder::new();
            #[cfg(unix)]
            {
                use std::os::unix::fs::DirBuilderExt;
                builder.mode(0o700);
            }
            match builder.create(&path) {
                Ok(()) => return Ok(Self(path)),
                Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => continue,
                Err(e) => return Err(failure(e)),
            }
        }
        Err(failure(()))
    }
}
impl Drop for Cache {
    fn drop(&mut self) {
        let _ = fs::remove_dir_all(&self.0);
    }
}

async fn transfer(
    config: &Config,
    claim: &Value,
    token: &str,
    closing: &mut watch::Receiver<bool>,
) -> Result<&'static str> {
    if *closing.borrow() {
        return Ok("cancel");
    }
    let parent = config.settings()?;
    let cache = Cache::new(&parent)?;
    let mut data =
        json!({"op":"friend_store","user":claim["puid"],"token":token,"record":claim["payload"]})
            .to_string()
            .into_bytes();
    data.push(b'\n');
    if data.len() > 65536
        || token.is_empty()
        || token.len() > 8192
        || !token.bytes().all(|b| (33..127).contains(&b))
    {
        return Err(failure(()));
    }
    let mut command = Command::new(&config.worker);
    command.env_clear();
    for key in [
        "PATH",
        "LD_LIBRARY_PATH",
        "SSL_CERT_FILE",
        "SSL_CERT_DIR",
        "LANG",
        "LC_ALL",
        "TZ",
        "SYSTEMROOT",
        "WINDIR",
    ] {
        if let Some(value) = std::env::var_os(key) {
            command.env(key, value);
        }
    }
    command
        .env("H2_EOS_WORKER_MODE", "friends")
        .env("H2_EOS_FRIEND_STORAGE_CONFIG", &config.path);
    for key in [
        "H2_EOS_FRIEND_CACHE_DIR",
        "HOME",
        "TMPDIR",
        "TMP",
        "TEMP",
        "XDG_CACHE_HOME",
        "XDG_CONFIG_HOME",
        "XDG_DATA_HOME",
    ] {
        command.env(key, &cache.0);
    }
    let mut child = command
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::null())
        .kill_on_drop(true)
        .spawn()
        .map_err(failure)?;
    let exchange = async {
        let mut input = child.stdin.take().ok_or_else(|| failure(()))?;
        let mut output = BufReader::new(child.stdout.take().ok_or_else(|| failure(()))?);
        input.write_all(&data).await.map_err(failure)?;
        input.flush().await.map_err(failure)?;
        for _ in 0..64 {
            let mut line = Vec::new();
            loop {
                let bytes = output.fill_buf().await.map_err(failure)?;
                if bytes.is_empty() {
                    return Err(failure(()));
                }
                let take = bytes
                    .iter()
                    .position(|b| *b == b'\n')
                    .map_or(bytes.len(), |i| i + 1);
                if line.len() + take > 131072 {
                    return Err(failure(()));
                }
                line.extend_from_slice(&bytes[..take]);
                output.consume(take);
                if line.last() == Some(&b'\n') {
                    break;
                }
            }
            if !line.starts_with(b"H2EOS ") {
                continue;
            }
            let text = std::str::from_utf8(&line[6..]).map_err(failure)?;
            matchmaking_coordinator::value(
                &json!({"op":"friend_worker_reply","text":text,"record":claim["payload"]}),
            )?;
            return Ok("success");
        }
        Err(failure(()))
    };
    let result = tokio::select! {
        biased;
        _=closing.changed()=>Ok("cancel"),
        result=tokio::time::timeout(Duration::from_secs(130),exchange)=>result.unwrap_or_else(|_|Err(failure(()))),
    };
    let _ = child.start_kill();
    child.wait().await.map_err(failure)?;
    drop(cache);
    result
}
async fn next(app: Arc<App>) -> Result<Value> {
    tokio::task::spawn_blocking(move || app.runtime.lock().map_err(failure)?.friend_cloud_next())
        .await
        .map_err(failure)?
}
async fn finish(app: Arc<App>, claim: String, outcome: &'static str) -> Result<()> {
    tokio::task::spawn_blocking(move || {
        app.runtime
            .lock()
            .map_err(failure)?
            .friend_cloud_finish(&claim, outcome)
    })
    .await
    .map_err(failure)?
}
pub async fn run(app: Arc<App>, config: Config, mut closing: watch::Receiver<bool>) {
    let wake = match app.runtime.lock() {
        Ok(r) => r.friend_cloud_wake(),
        Err(_) => return,
    };
    loop {
        if *closing.borrow() {
            return;
        }
        let notified = wake.notified();
        let work = match next(app.clone()).await {
            Ok(claim) if !claim.is_null() => Some(claim),
            _ => None,
        };
        let Some(claim) = work else {
            tokio::select! {biased;_=closing.changed()=>return,_=notified=>{},_=tokio::time::sleep(Duration::from_secs(10))=>{}}
            continue;
        };
        let outcome = async {
            let issuer = app.eos.as_ref().ok_or_else(|| failure(()))?;
            let token = issuer.token(
                claim["player"].as_str().ok_or_else(|| failure(()))?,
                claim["name"].as_str().ok_or_else(|| failure(()))?,
                database::wall_now()?,
            )?;
            transfer(
                &config,
                &claim,
                token["token"].as_str().ok_or_else(|| failure(()))?,
                &mut closing,
            )
            .await
        }
        .await
        .unwrap_or("failure");
        let outcome = if *closing.borrow() { "cancel" } else { outcome };
        let Some(token) = claim["claim"].as_str() else {
            return;
        };
        loop {
            match finish(app.clone(), token.to_owned(), outcome).await {
                Ok(()) | Err(Error(409, _)) => break,
                Err(_) => {}
            }
            if *closing.borrow() {
                return;
            }
            tokio::select! {biased;_=closing.changed()=>return,_=tokio::time::sleep(Duration::from_secs(10))=>{}}
        }
    }
}
