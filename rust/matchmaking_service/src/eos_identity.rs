//! EOS-purpose issuer and bounded official-SDK worker IO. Unverified JWT
//! claims are routing hints; only a private SDK reply authorizes a binding.
use base64::{Engine as _, engine::general_purpose::URL_SAFE_NO_PAD};
use halo2_online_protocol::{
    matchmaking_json,
    matchmaking_model::{self as m, Error, Result},
};
use openssl::{
    hash::MessageDigest,
    pkey::{PKey, Private},
    rsa::{Padding, Rsa},
    sign::Signer,
};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::{
    fs::{self, OpenOptions},
    io::{Read, Write},
    path::{Path, PathBuf},
    sync::Arc,
};

#[path = "eos_verifier.rs"]
mod verifier;
pub use verifier::{Verified, Verifier};

pub struct Identity {
    key: PKey<Private>,
    kid: String,
    issuer: String,
    audience: String,
    pub verifier: Arc<Verifier>,
}
fn unavailable(_: impl std::fmt::Debug) -> Error {
    Error(503, "EOS signing identity unavailable")
}

impl Identity {
    pub fn open(
        state: &Path,
        issuer: &str,
        audience: &str,
        worker: Option<PathBuf>,
        allow_http: bool,
    ) -> Result<Option<Self>> {
        if audience.is_empty() {
            if worker.is_some() {
                return Err(Error(
                    400,
                    "EOS worker requires a configured signing identity",
                ));
            }
            return Ok(None);
        }
        let uri: axum::http::Uri = issuer
            .parse()
            .map_err(|_| Error(400, "Invalid public EOS issuer"))?;
        if allow_http
            || audience.len() > 128
            || !audience.bytes().all(|b| (33..127).contains(&b))
            || uri.scheme_str() != Some("https")
            || uri.host().is_none_or(str::is_empty)
            || uri.authority().is_some_and(|a| a.as_str().contains('@'))
            || uri.query().is_some()
            || issuer.contains('#')
            || issuer.len() > 2048
        {
            return Err(Error(
                400,
                "EOS OpenID requires a public HTTPS issuer and audience",
            ));
        }
        let path = state.join("eos-openid-key.pem");
        if !path.exists() {
            let rsa = Rsa::generate(2048).map_err(unavailable)?;
            let key = PKey::from_rsa(rsa).map_err(unavailable)?;
            let bytes = key.private_key_to_pem_pkcs8().map_err(unavailable)?;
            let mut options = OpenOptions::new();
            options.write(true).create_new(true);
            #[cfg(unix)]
            {
                use std::os::unix::fs::OpenOptionsExt;
                options.mode(0o600);
            }
            match options.open(&path) {
                Ok(mut file) => {
                    file.write_all(&bytes).map_err(unavailable)?;
                    file.sync_all().map_err(unavailable)?;
                }
                Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => (),
                Err(e) => return Err(unavailable(e)),
            }
        }
        let metadata = fs::symlink_metadata(&path).map_err(unavailable)?;
        if !metadata.is_file() || metadata.len() > 16384 {
            return Err(unavailable(()));
        }
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            if metadata.permissions().mode() & 0o077 != 0 {
                return Err(unavailable(()));
            }
        }
        let mut bytes = Vec::new();
        fs::File::open(&path)
            .map_err(unavailable)?
            .take(16385)
            .read_to_end(&mut bytes)
            .map_err(unavailable)?;
        if bytes.len() > 16384 {
            return Err(unavailable(()));
        }
        let key = PKey::private_key_from_pem(&bytes).map_err(unavailable)?;
        let rsa = key.rsa().map_err(unavailable)?;
        if !(2048..=8192).contains(&key.bits()) || !rsa.check_key().map_err(unavailable)? {
            return Err(unavailable(()));
        }
        let kid = format!(
            "{:x}",
            Sha256::digest(key.public_key_to_der().map_err(unavailable)?)
        )[..32]
            .to_owned();
        Ok(Some(Self {
            key,
            kid,
            issuer: issuer.trim_end_matches('/').into(),
            audience: audience.into(),
            verifier: Arc::new(Verifier::new(worker)?),
        }))
    }
    pub fn jwks(&self) -> Result<Value> {
        let rsa = self.key.rsa().map_err(unavailable)?;
        Ok(
            json!({"keys":[{"kty":"RSA","use":"sig","alg":"RS256","kid":self.kid,
            "n":URL_SAFE_NO_PAD.encode(rsa.n().to_vec()),"e":URL_SAFE_NO_PAD.encode(rsa.e().to_vec())}]}),
        )
    }
    pub fn token(&self, player: &str, name: &str, wall: i64) -> Result<Value> {
        if wall < 5 || wall > 9007199254740871 {
            return Err(Error(503, "EOS issuance clock unavailable"));
        }
        let encode = |value: Value| URL_SAFE_NO_PAD.encode(value.to_string().as_bytes());
        let text = format!(
            "{}.{}",
            encode(json!({"alg":"RS256","typ":"JWT","kid":self.kid})),
            encode(
                json!({"iss":self.issuer,"sub":player,"aud":self.audience,"name":name,"iat":wall,"nbf":wall-5,"exp":wall+120})
            )
        );
        let mut signer = Signer::new(MessageDigest::sha256(), &self.key).map_err(unavailable)?;
        signer
            .set_rsa_padding(Padding::PKCS1)
            .map_err(unavailable)?;
        let signature = signer
            .sign_oneshot_to_vec(text.as_bytes())
            .map_err(unavailable)?;
        Ok(json!({"token":format!("{text}.{}",URL_SAFE_NO_PAD.encode(signature)),"expires_in":120}))
    }
    pub fn health(&self) -> Value {
        json!({"issuer":self.issuer,"audience":self.audience,"kid":self.kid,"verification_configured":self.verifier.configured()})
    }
}
pub fn proof_input(body: &Value, room: bool) -> Result<(String, String)> {
    m::fields(body, if room { &["token"] } else { &["puid", "token"] })?;
    let token = m::text(&body["token"], 8192)?;
    m::check(
        token.bytes().all(|b| (33..127).contains(&b)),
        "Invalid EOS identity proof",
    )?;
    let puid = if room {
        proof_subject(token)?
    } else {
        m::hex(&body["puid"], 16)?;
        m::text(&body["puid"], 32)?.to_owned()
    };
    Ok((puid, token.to_owned()))
}
fn proof_subject(token: &str) -> Result<String> {
    let parts = token.split('.').collect::<Vec<_>>();
    m::check(
        parts.len() == 3 && parts.iter().all(|s| !s.is_empty()),
        "Invalid EOS identity proof",
    )?;
    let bytes = URL_SAFE_NO_PAD
        .decode(parts[1])
        .map_err(|_| Error(400, "Invalid EOS identity proof"))?;
    let value = matchmaking_json::parse(&bytes, 8192)?;
    m::hex(&value["sub"], 16)?;
    Ok(m::text(&value["sub"], 32)?.to_owned())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn unverified_room_subject_is_bounded_and_duplicate_safe() {
        let valid = format!(
            "a.{}.b",
            URL_SAFE_NO_PAD.encode(json!({"sub":"a".repeat(32)}).to_string())
        );
        assert_eq!(
            proof_input(&json!({"token":valid}), true).unwrap().0,
            "a".repeat(32)
        );
        for token in [
            "opaque".to_owned(),
            "a..b".to_owned(),
            "a.%%%.b".to_owned(),
            format!(
                "a.{}.b",
                URL_SAFE_NO_PAD.encode(format!(
                    "{{\"sub\":\"{}\",\"sub\":\"{}\"}}",
                    "a".repeat(32),
                    "b".repeat(32)
                ))
            ),
            format!("a.{}.b", URL_SAFE_NO_PAD.encode("{\"sub\":4}")),
            "x".repeat(8193),
        ] {
            assert!(proof_input(&json!({"token":token}), true).is_err());
        }
    }
}
