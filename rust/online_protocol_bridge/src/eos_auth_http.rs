//! Room-authorized EOS credential HTTP I/O. SDK/session state never enters this owner.
use crate::ffi;
use curl::easy::{Easy, List};
use serde::Deserialize;
use std::sync::atomic::{AtomicBool, Ordering};
use std::time::Duration;

const MAX_BODY: usize = 16_384;
const MAX_TOKEN: usize = 8_192;

pub struct EosAuthHttp {
    room_api: String,
    bearer: String,
    cancelled: AtomicBool,
}

fn valid_port(value: &str) -> bool {
    !value.is_empty()
        && value.bytes().all(|c| c.is_ascii_digit())
        && value.parse::<u16>().is_ok_and(|port| port != 0)
}

fn valid_room_api(value: &str) -> bool {
    if value.len() > 2_048
        || !value.bytes().all(|c| c.is_ascii_graphic())
        || value.bytes().any(|c| b"@?#\\%".contains(&c))
    {
        return false;
    }
    let Some((scheme, rest)) = value.split_once("://") else {
        return false;
    };
    if scheme != "https" && scheme != "http" {
        return false;
    }
    let Some((authority, path)) = rest.split_once('/') else {
        return false;
    };
    let Some(room) = path.strip_prefix("v1/matches/") else {
        return false;
    };
    if room.is_empty()
        || room.len() > 128
        || !room
            .bytes()
            .all(|c| c.is_ascii_alphanumeric() || b"_-".contains(&c))
    {
        return false;
    }
    let host = if let Some(ipv6) = authority.strip_prefix('[') {
        let Some((host, suffix)) = ipv6.split_once(']') else {
            return false;
        };
        if host.parse::<std::net::Ipv6Addr>().is_err()
            || (!suffix.is_empty() && !suffix.strip_prefix(':').is_some_and(valid_port))
        {
            return false;
        }
        host
    } else {
        let (host, port) = authority
            .split_once(':')
            .map_or((authority, None), |(h, p)| (h, Some(p)));
        if host.is_empty()
            || !host
                .bytes()
                .all(|c| c.is_ascii_alphanumeric() || b".-".contains(&c))
            || port.is_some_and(|port| !valid_port(port))
        {
            return false;
        }
        host
    };
    scheme == "https" || host == "127.0.0.1" || host == "::1"
}

pub fn new_eos_auth_http(room_api: &str, bearer: &str) -> Result<Box<EosAuthHttp>, String> {
    if !valid_room_api(room_api)
        || bearer.is_empty()
        || bearer.len() > MAX_TOKEN
        || !bearer.bytes().all(|c| c.is_ascii_graphic())
    {
        return Err("Invalid EOS room authentication endpoint or credential.".into());
    }
    Ok(Box::new(EosAuthHttp {
        room_api: room_api.into(),
        bearer: bearer.into(),
        cancelled: AtomicBool::new(false),
    }))
}

pub fn eos_auth_http_cancel(client: &EosAuthHttp) {
    client.cancelled.store(true, Ordering::Release);
}

fn response(request: &ffi::EosAuthRequest) -> ffi::EosAuthResponse {
    ffi::EosAuthResponse {
        kind: request.kind,
        ticket: request.ticket,
        status: 0,
        ok: false,
        token: String::new(),
        user: String::new(),
        expires_in: 0,
    }
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct TokenReply {
    token: String,
    expires_in: u32,
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct ProofReply {
    eos_product_user_id: String,
}

fn parse_reply(kind: u8, body: &[u8], result: &mut ffi::EosAuthResponse) {
    if body.len() > MAX_BODY
        || body
            .iter()
            .find(|byte| !matches!(byte, b' ' | b'\t' | b'\r' | b'\n'))
            != Some(&b'{')
    {
        return;
    }
    if kind == 1 {
        let Ok(value) = serde_json::from_slice::<TokenReply>(body) else {
            return;
        };
        if value.token.is_empty()
            || value.token.len() > MAX_TOKEN
            || !(1..=120).contains(&value.expires_in)
        {
            return;
        }
        result.token = value.token;
        result.expires_in = value.expires_in;
    } else {
        let Ok(value) = serde_json::from_slice::<ProofReply>(body) else {
            return;
        };
        if value.eos_product_user_id.len() > 64 {
            return;
        }
        result.user = value.eos_product_user_id;
    }
    result.ok = true;
}

fn perform(
    client: &EosAuthHttp,
    request: &ffi::EosAuthRequest,
) -> Result<(u32, Vec<u8>), curl::Error> {
    let mut easy = Easy::new();
    let suffix = if request.kind == 1 {
        "/eos-token"
    } else {
        "/eos-proof"
    };
    let mut headers = List::new();
    headers.append(&format!("Authorization: Bearer {}", client.bearer))?;
    headers.append("Content-Type: application/json")?;
    let body = if request.kind == 1 {
        b"{}".to_vec()
    } else {
        serde_json::json!({"token": request.token})
            .to_string()
            .into_bytes()
    };
    // The admitted endpoint permits only HTTP(S), with HTTP limited to literal
    // loopback. Redirects cannot introduce another endpoint or protocol.
    easy.url(&format!("{}{suffix}", client.room_api))?;
    easy.http_headers(headers)?;
    easy.post(true)?;
    easy.post_fields_copy(&body)?;
    easy.follow_location(false)?;
    easy.proxy("")?;
    easy.signal(false)?;
    easy.connect_timeout(Duration::from_secs(2))?;
    easy.timeout(Duration::from_secs(if request.kind == 1 { 3 } else { 25 }))?;
    easy.ssl_verify_peer(true)?;
    easy.ssl_verify_host(true)?;
    easy.accept_encoding("identity")?;
    easy.http_content_decoding(false)?;
    easy.progress(true)?;
    let mut received = Vec::new();
    {
        let mut transfer = easy.transfer();
        transfer.write_function(|bytes| {
            if client.cancelled.load(Ordering::Acquire) || bytes.len() > MAX_BODY - received.len() {
                return Ok(0);
            }
            received.extend_from_slice(bytes);
            Ok(bytes.len())
        })?;
        transfer.progress_function(|_, _, _, _| !client.cancelled.load(Ordering::Acquire))?;
        transfer.perform()?;
    }
    Ok((easy.response_code()?, received))
}

pub fn eos_auth_http_request(
    client: &EosAuthHttp,
    request: &ffi::EosAuthRequest,
) -> ffi::EosAuthResponse {
    let mut result = response(request);
    if request.kind != 1 && request.kind != 2
        || request.kind == 2 && (request.token.is_empty() || request.token.len() > MAX_TOKEN)
    {
        result.status = 400;
        return result;
    }
    if client.cancelled.load(Ordering::Acquire) {
        return result;
    }
    // Network/parser diagnostics may contain private values. Return only the
    // factual HTTP status and bounded typed fields to the existing Rust policy.
    let Ok((status, body)) = perform(client, request) else {
        return result;
    };
    if client.cancelled.load(Ordering::Acquire) {
        return result;
    }
    result.status = u16::try_from(status)
        .ok()
        .filter(|status| *status <= 999)
        .unwrap_or(0);
    if status == 200 {
        parse_reply(request.kind, &body, &mut result);
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::{Read, Write};
    use std::net::{TcpListener, TcpStream};
    use std::sync::{Arc, mpsc};
    use std::thread;
    use std::time::Instant;

    fn listener() -> TcpListener {
        let listener = TcpListener::bind("127.0.0.1:0").unwrap();
        listener.set_nonblocking(true).unwrap();
        listener
    }

    fn accept(listener: &TcpListener) -> TcpStream {
        let deadline = Instant::now() + Duration::from_secs(3);
        loop {
            match listener.accept() {
                Ok((stream, _)) => {
                    stream.set_nonblocking(false).unwrap();
                    stream
                        .set_read_timeout(Some(Duration::from_secs(2)))
                        .unwrap();
                    stream
                        .set_write_timeout(Some(Duration::from_secs(2)))
                        .unwrap();
                    return stream;
                }
                Err(error) if error.kind() == std::io::ErrorKind::WouldBlock => {
                    assert!(Instant::now() < deadline, "loopback request did not arrive");
                    thread::sleep(Duration::from_millis(1));
                }
                Err(_) => panic!("loopback accept failed"),
            }
        }
    }

    fn read_request(stream: &mut TcpStream) -> Vec<u8> {
        let mut request = Vec::new();
        loop {
            let mut buffer = [0; 1024];
            let count = stream.read(&mut buffer).unwrap();
            assert!(count != 0 && request.len() + count <= MAX_BODY);
            request.extend_from_slice(&buffer[..count]);
            if let Some(end) = request.windows(4).position(|w| w == b"\r\n\r\n") {
                let headers = std::str::from_utf8(&request[..end]).unwrap();
                let length = headers
                    .lines()
                    .find_map(|line| {
                        let (key, value) = line.split_once(':')?;
                        key.eq_ignore_ascii_case("Content-Length")
                            .then(|| value.trim().parse::<usize>().unwrap())
                    })
                    .unwrap();
                if request.len() == end + 4 + length {
                    return request;
                }
            }
        }
    }

    fn endpoint(listener: &TcpListener) -> String {
        format!(
            "http://{}/v1/matches/fixture-room",
            listener.local_addr().unwrap()
        )
    }

    fn reply(stream: &mut TcpStream, status: &str, body: &[u8], extra: &str) {
        let header = format!(
            "HTTP/1.1 {status}\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n{extra}\r\n",
            body.len()
        );
        stream.write_all(header.as_bytes()).unwrap();
        // Oversized-body controls intentionally allow the client to close early.
        let _ = stream.write_all(body);
    }

    fn request(kind: u8) -> ffi::EosAuthRequest {
        ffi::EosAuthRequest {
            kind,
            ticket: u64::MAX,
            token: "proof".into(),
            user: "local".into(),
        }
    }

    #[test]
    fn endpoint_and_header_admission_excludes_credential_redirection() {
        for endpoint in [
            "https://master.example/v1/matches/room_1",
            "http://127.0.0.1:1234/v1/matches/room",
            "http://[::1]:1234/v1/matches/room",
        ] {
            assert!(new_eos_auth_http(endpoint, "private-room-bearer").is_ok());
        }
        for endpoint in [
            "http://master.example/v1/matches/room",
            "http://localhost/v1/matches/room",
            "file:///v1/matches/room",
            "https://user@master.example/v1/matches/room",
            "https://master.example/v1/matches/room?next=other",
            "https://master.example/v1/matches/room/other",
            "https://master.example:0/v1/matches/room",
            "https://master.example/v1/matches/..",
            "https://master.example/v1/matches/%2e",
        ] {
            assert!(new_eos_auth_http(endpoint, "bearer").is_err());
        }
        for bearer in ["", "value\r\nHeader: injected", "value with space"] {
            assert!(new_eos_auth_http("https://master.example/v1/matches/room", bearer).is_err());
        }
    }

    #[test]
    fn typed_json_preserves_bounds_and_rejects_duplicates_unknown_nested_and_trailing() {
        for body in [
            br#"{"token":"grant","expires_in":1}"#.as_slice(),
            br#"{"token":"grant","expires_in":120}"#,
            b" \t\r\n{\"token\":\"grant\",\"expires_in\":120}\n ",
        ] {
            let mut value = response(&request(1));
            parse_reply(1, body, &mut value);
            assert!(value.ok && value.token == "grant");
        }
        for body in [
            r#"["grant",120]"#,
            r#"{"token":"a","token":"b","expires_in":1}"#,
            r#"{"token":"a","expires_in":1,"expires_in":2}"#,
            r#"{"token":"a","expires_in":1,"extra":0}"#,
            r#"{"token":{"nested":true},"expires_in":1}"#,
            r#"{"token":"a","expires_in":0}"#,
            r#"{"token":"a","expires_in":121}"#,
            r#"{"token":"a","expires_in":1.0}"#,
            r#"{"token":"a","expires_in":-1}"#,
            r#"{"token":"a","expires_in":1}{}"#,
        ] {
            let mut value = response(&request(1));
            parse_reply(1, body.as_bytes(), &mut value);
            assert!(!value.ok && value.token.is_empty());
        }
        let mut value = response(&request(2));
        parse_reply(
            2,
            b"\r\n\t {\"eos_product_user_id\":\"actual\"} \n",
            &mut value,
        );
        assert!(value.ok && value.user == "actual");
        for body in [
            r#"["uid"]"#.into(),
            r#"{"eos_product_user_id":"a","eos_product_user_id":"b"}"#.into(),
            serde_json::json!({"eos_product_user_id":"x".repeat(65)}).to_string(),
        ] {
            let mut value = response(&request(2));
            parse_reply(2, body.as_bytes(), &mut value);
            assert!(!value.ok && value.user.is_empty());
        }
    }

    #[test]
    fn cancellation_is_latched_and_never_contacts_http_or_changes_request_identity() {
        let client = new_eos_auth_http("https://unused.invalid/v1/matches/room", "bearer").unwrap();
        eos_auth_http_cancel(&client);
        for kind in [1, 2] {
            let value = eos_auth_http_request(&client, &request(kind));
            assert_eq!(
                (value.kind, value.ticket, value.status, value.ok),
                (kind, u64::MAX, 0, false)
            );
            assert!(value.token.is_empty() && value.user.is_empty());
        }
    }

    #[test]
    fn real_proof_post_carries_only_room_bearer_and_proof_then_decodes_ack() {
        let listener = listener();
        let client = new_eos_auth_http(&endpoint(&listener), "fixture-room-bearer").unwrap();
        let server = thread::spawn(move || {
            let mut stream = accept(&listener);
            let request = read_request(&mut stream);
            let end = request.windows(4).position(|w| w == b"\r\n\r\n").unwrap();
            let headers = std::str::from_utf8(&request[..end]).unwrap();
            assert!(headers.starts_with("POST /v1/matches/fixture-room/eos-proof HTTP/1.1\r\n"));
            assert!(
                headers
                    .lines()
                    .any(|line| line == "Authorization: Bearer fixture-room-bearer")
            );
            let body: serde_json::Value = serde_json::from_slice(&request[end + 4..]).unwrap();
            assert!(body == serde_json::json!({"token":"fixture-sdk-proof"}));
            reply(
                &mut stream,
                "200 OK",
                br#"{"eos_product_user_id":"0123456789abcdef0123456789abcdef"}"#,
                "",
            );
        });
        let mut message = request(2);
        message.token = "fixture-sdk-proof".into();
        let result = eos_auth_http_request(&client, &message);
        server.join().unwrap();
        assert_eq!(
            (result.kind, result.ticket, result.status),
            (2, u64::MAX, 200)
        );
        assert!(
            result.ok
                && result.user == "0123456789abcdef0123456789abcdef"
                && result.token.is_empty()
        );
    }

    #[test]
    fn real_redirect_never_forwards_private_header_or_proof() {
        let listener = listener();
        let other = self::listener();
        let location = format!("Location: {}/eos-proof\r\n", endpoint(&other));
        let client = new_eos_auth_http(&endpoint(&listener), "fixture-room-bearer").unwrap();
        let server = thread::spawn(move || {
            let mut stream = accept(&listener);
            read_request(&mut stream);
            reply(&mut stream, "302 Found", b"", &location);
        });
        let result = eos_auth_http_request(&client, &request(2));
        server.join().unwrap();
        assert!(
            result.status == 302 && !result.ok && result.token.is_empty() && result.user.is_empty()
        );
        assert!(
            matches!(other.accept(), Err(error) if error.kind() == std::io::ErrorKind::WouldBlock)
        );
    }

    #[test]
    fn real_typed_reply_refuses_duplicate_unknown_deep_and_oversized_bodies() {
        let bodies = [
            br#"{"eos_product_user_id":"a","eos_product_user_id":"b"}"#.to_vec(),
            br#"{"eos_product_user_id":"a","extra":true}"#.to_vec(),
            br#"{"eos_product_user_id":[[[[[[[[[["a"]]]]]]]]]]}"#.to_vec(),
            serde_json::json!({"eos_product_user_id":"x".repeat(MAX_BODY)})
                .to_string()
                .into_bytes(),
        ];
        for body in bodies {
            let listener = listener();
            let client = new_eos_auth_http(&endpoint(&listener), "fixture-room-bearer").unwrap();
            let server = thread::spawn(move || {
                let mut stream = accept(&listener);
                read_request(&mut stream);
                reply(&mut stream, "200 OK", &body, "");
            });
            let result = eos_auth_http_request(&client, &request(2));
            server.join().unwrap();
            assert!(!result.ok && result.token.is_empty() && result.user.is_empty());
        }
    }

    #[test]
    fn actual_pending_proof_read_cancels_before_two_seconds_without_sdk_or_session_lock() {
        let listener = listener();
        let client =
            Arc::new(*new_eos_auth_http(&endpoint(&listener), "fixture-room-bearer").unwrap());
        let (pending_tx, pending_rx) = mpsc::channel();
        let (release_tx, release_rx) = mpsc::channel::<()>();
        let server = thread::spawn(move || {
            let mut stream = accept(&listener);
            read_request(&mut stream);
            stream
                .write_all(b"HTTP/1.1 200 OK\r\nContent-Length: 100\r\nConnection: close\r\n\r\n")
                .unwrap();
            pending_tx.send(()).unwrap();
            let _ = release_rx.recv();
        });
        let worker_client = Arc::clone(&client);
        let (completed_tx, completed_rx) = mpsc::channel();
        let worker = thread::spawn(move || {
            completed_tx
                .send(eos_auth_http_request(&worker_client, &request(2)))
                .unwrap();
        });
        pending_rx.recv_timeout(Duration::from_secs(2)).unwrap();
        let started = Instant::now();
        eos_auth_http_cancel(&client);
        let result = completed_rx
            .recv_timeout(Duration::from_millis(1_900))
            .unwrap();
        assert!(started.elapsed() < Duration::from_secs(2));
        assert!(
            result.status == 0 && !result.ok && result.token.is_empty() && result.user.is_empty()
        );
        drop(release_tx);
        server.join().unwrap();
        worker.join().unwrap();
    }
}
