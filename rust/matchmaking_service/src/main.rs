//! Rust HTTP/SQLite runtime for the progressive production API migration.
mod admission;
mod authentication;
mod database;
mod eos_identity;
mod friend_cloud;
mod http;
mod runtime;
mod steam_friends_provider;
mod steam_login;
use halo2_online_protocol::matchmaking_model::{Error, Result};
use std::{net::SocketAddr, path::PathBuf, sync::Arc};

struct Config {
    state: PathBuf,
    listen: SocketAddr,
    client_id: String,
    steam: bool,
    proxy: bool,
    allow_http: bool,
    catalog: Option<PathBuf>,
    public_url: String,
    eos_audience: String,
    eos_worker: Option<PathBuf>,
    steam_key: String,
    friend_cloud: Option<friend_cloud::Config>,
}
fn config() -> Result<Config> {
    let mut result = Config {
        state: PathBuf::from(".local/master-rust"),
        listen: "127.0.0.1:8080".parse().unwrap(),
        client_id: "1558522073272426519".into(),
        steam: false,
        proxy: false,
        allow_http: false,
        catalog: None,
        public_url: std::env::var("H2_PUBLIC_URL")
            .or_else(|_| std::env::var("H2_MASTER_PUBLIC_URL"))
            .unwrap_or_default(),
        eos_audience: std::env::var("H2_EOS_OPENID_AUDIENCE").unwrap_or_default(),
        eos_worker: std::env::var_os("H2_EOS_WORKER")
            .filter(|s| !s.is_empty())
            .map(PathBuf::from),
        steam_key: steam_key()?,
        friend_cloud: None,
    };
    let mut args = std::env::args().skip(1);
    while let Some(key) = args.next() {
        match key.as_str() {
            "--state" => {
                result.state = PathBuf::from(args.next().ok_or(Error(400, "State path required"))?)
            }
            "--listen" => {
                result.listen = args
                    .next()
                    .ok_or(Error(400, "Listen address required"))?
                    .parse()
                    .map_err(|_| Error(400, "Invalid listen address"))?
            }
            "--discord-client-id" => {
                result.client_id = args
                    .next()
                    .ok_or(Error(400, "Discord application required"))?
            }
            "--steam-enabled" => result.steam = true,
            "--public-url" => {
                result.public_url = args
                    .next()
                    .ok_or(Error(400, "Public issuer URL required"))?
            }
            "--eos-openid-audience" => {
                result.eos_audience = args.next().ok_or(Error(400, "EOS audience required"))?
            }
            "--eos-worker" => {
                result.eos_worker = Some(PathBuf::from(
                    args.next().ok_or(Error(400, "EOS worker path required"))?,
                ))
            }
            "--matchmaking-catalog" => {
                result.catalog = Some(PathBuf::from(
                    args.next()
                        .ok_or(Error(400, "Matchmaking catalog required"))?,
                ));
            }
            "--behind-tls-proxy" => result.proxy = true,
            "--allow-http" => result.allow_http = true,
            _ => return Err(Error(400, "Unknown service option")),
        }
    }
    if !result.allow_http && !(result.proxy && result.listen.ip().is_loopback()) {
        return Err(Error(
            400,
            "Configure an isolated TLS proxy or explicit development HTTP",
        ));
    }
    if !(17..=20).contains(&result.client_id.len())
        || !result.client_id.bytes().all(|b| b.is_ascii_digit())
    {
        return Err(Error(400, "Invalid Discord application ID"));
    }
    result.friend_cloud = friend_cloud::Config::from_environment(result.eos_worker.clone())?;
    Ok(result)
}
fn open(config: &Config) -> Result<runtime::Runtime> {
    let new = !config.state.exists();
    if new {
        std::fs::create_dir_all(&config.state)
            .map_err(|_| Error(500, "Cannot create service state"))?;
    }
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        if new {
            std::fs::set_permissions(&config.state, std::fs::Permissions::from_mode(0o700))
                .map_err(|_| Error(500, "Cannot protect service state"))?;
        }
        if std::fs::metadata(&config.state)
            .map_err(|_| Error(500, "Cannot inspect service state"))?
            .permissions()
            .mode()
            & 0o077
            != 0
        {
            return Err(Error(500, "Service state directory must be private"));
        }
    }
    let database = config.state.join("registry.sqlite3");
    let catalog = config
        .catalog
        .as_ref()
        .map(|path| {
            use std::io::Read;
            let mut bytes = Vec::new();
            std::fs::File::open(path)
                .map_err(|_| Error(500, "Cannot open matchmaking catalog"))?
                .take(1024 * 1024 + 1)
                .read_to_end(&mut bytes)
                .map_err(|_| Error(500, "Cannot read matchmaking catalog"))?;
            if bytes.len() > 1024 * 1024 {
                return Err(Error(400, "Matchmaking catalog exceeds bounds"));
            }
            Ok(bytes)
        })
        .transpose()?;
    let runtime = runtime::Runtime::open(&database, config.steam, catalog.as_deref())?;
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        std::fs::set_permissions(database, std::fs::Permissions::from_mode(0o600))
            .map_err(|_| Error(500, "Cannot protect service database"))?;
    }
    Ok(runtime)
}
fn main() {
    let result = (|| -> Result<()> {
        let config = config()?;
        let runtime = open(&config)?;
        let eos = eos_identity::Identity::open(
            &config.state,
            &config.public_url,
            &config.eos_audience,
            config.eos_worker.clone(),
            config.allow_http,
        )?
        .map(Arc::new);
        if config.friend_cloud.is_some() && eos.is_none() {
            return Err(Error(
                503,
                "Private friend storage requires an EOS signing identity",
            ));
        }
        let executor = tokio::runtime::Builder::new_multi_thread()
            .worker_threads(2)
            .max_blocking_threads(4)
            .enable_all()
            .build()
            .map_err(|_| Error(500, "Cannot create service executor"))?;
        executor.block_on(async move {
            let listener=tokio::net::TcpListener::bind(config.listen).await.map_err(|_|Error(500,"Cannot bind service listener"))?;
            let address=listener.local_addr().map_err(|_|Error(500,"Cannot read service listener"))?;
            let mut runtime=runtime;
            let issuer=if config.public_url.is_empty()&&config.allow_http&&address.ip().is_loopback(){format!("http://{address}")}else{config.public_url.clone()};
            runtime.configure_steam(&issuer,config.allow_http)?;
            runtime.configure_friend_cloud(config.friend_cloud.is_some());
            let app=Arc::new(http::App::new(runtime,config.client_id,config.proxy,eos.clone(),config.steam_key));
            println!("LISTENING {address}");
            use std::io::Write;
            std::io::stdout().flush().map_err(|_|Error(500,"Cannot publish listener address"))?;
            let maintenance=tokio::spawn(http::maintain(app.clone()));
            let (cloud_closing,cloud_signal)=tokio::sync::watch::channel(false);
            let cloud=config.friend_cloud.map(|config|tokio::spawn(friend_cloud::run(app.clone(),config,cloud_signal)));
            let result=tokio::select! {
                result=http::serve(listener,app.clone())=>result.map_err(|_|Error(500,"Service listener failed")),
                result=shutdown()=>result.map_err(|_|Error(503,"Service shutdown signal unavailable")),
            };
            maintenance.abort();
            let _=maintenance.await;
            http::close_voice(app).await;
            cloud_closing.send_replace(true);
            if let Some(cloud)=cloud { let _=cloud.await; }
            if let Some(eos)=eos { eos.verifier.close().await; }
            result
        })
    })();
    if let Err(error) = result {
        eprintln!("Rust matchmaking service: {} ({})", error.1, error.0);
        std::process::exit(1);
    }
}
fn steam_key() -> Result<String> {
    use std::io::Read;
    let text = if let Some(path) = std::env::var_os("H2_STEAM_KEY_FILE").filter(|s| !s.is_empty()) {
        let file =
            std::fs::File::open(path).map_err(|_| Error(500, "Cannot open Steam Web API key"))?;
        let mut text = String::new();
        file.take(513)
            .read_to_string(&mut text)
            .map_err(|_| Error(500, "Cannot read Steam Web API key"))?;
        if text.len() > 512 {
            return Err(Error(400, "Invalid Steam Web API key configuration"));
        }
        text
    } else {
        std::env::var("H2_STEAM_KEY")
            .or_else(|_| std::env::var("STEAM_KEY"))
            .unwrap_or_default()
    };
    let text = text.trim();
    if !text.is_empty() && (text.len() != 32 || !text.bytes().all(|c| c.is_ascii_hexdigit())) {
        return Err(Error(400, "Invalid Steam Web API key configuration"));
    }
    Ok(text.into())
}
async fn shutdown() -> std::io::Result<()> {
    #[cfg(unix)]
    {
        let mut terminate =
            tokio::signal::unix::signal(tokio::signal::unix::SignalKind::terminate())?;
        tokio::select! { result=tokio::signal::ctrl_c()=>result,_=terminate.recv()=>Ok(()) }
    }
    #[cfg(not(unix))]
    {
        tokio::signal::ctrl_c().await
    }
}
