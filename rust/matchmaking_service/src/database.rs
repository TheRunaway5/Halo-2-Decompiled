//! SQLite IO for the shared Rust plans. No SQL or native handles enter from HTTP.
use halo2_online_protocol::{
    matchmaking_model::{Error, Result},
    matchmaking_owners::Owners,
};
use rusqlite::{
    Connection, TransactionBehavior, params_from_iter,
    types::{Value as SqlValue, ValueRef},
};
use serde_json::{Value, json};

pub fn failure(error: rusqlite::Error) -> Error {
    eprintln!(
        "Rust service SQLite failure: {:?}",
        error.sqlite_error_code()
    );
    Error(500, "Database operation failed")
}
fn parameter(v: &Value) -> Result<SqlValue> {
    Ok(match v {
        Value::Null => SqlValue::Null,
        Value::Bool(b) => SqlValue::Integer(i64::from(*b)),
        Value::Number(n) => {
            if let Some(n) = n.as_i64() {
                SqlValue::Integer(n)
            } else {
                SqlValue::Real(n.as_f64().ok_or(Error(503, "Invalid SQL number"))?)
            }
        }
        Value::String(s) => SqlValue::Text(s.clone()),
        Value::Object(map) if map.len() == 1 && map.contains_key("blob") => {
            let hex = map["blob"].as_str().ok_or(Error(503, "Invalid SQL blob"))?;
            SqlValue::Blob(halo2_online_protocol::matchmaking_model::hex(
                &json!(hex),
                hex.len() / 2,
            )?)
        }
        _ => return Err(Error(503, "Invalid SQL parameter")),
    })
}
pub fn sql(db: &Connection, event: &Value) -> Result<Value> {
    let mut rows = Vec::new();
    let mut changes = Vec::new();
    let statements = event["statements"]
        .as_array()
        .ok_or(Error(503, "Rust SQL plan required"))?;
    for item in statements {
        let sql = item["sql"]
            .as_str()
            .ok_or(Error(503, "Rust SQL text required"))?;
        let params = item["params"]
            .as_array()
            .ok_or(Error(503, "Rust SQL parameters required"))?
            .iter()
            .map(parameter)
            .collect::<Result<Vec<_>>>()?;
        let mut statement = db.prepare(sql).map_err(failure)?;
        if event["mode"] == "query" {
            let width = statement.column_count();
            if width > 32 {
                return Err(Error(503, "SQL result width exceeds bound"));
            }
            let mut cursor = statement.query(params_from_iter(params)).map_err(failure)?;
            while let Some(row) = cursor.next().map_err(failure)? {
                let mut cells = Vec::with_capacity(width);
                for index in 0..width {
                    cells.push(match row.get_ref(index).map_err(failure)? {
                        ValueRef::Null => Value::Null,
                        ValueRef::Integer(v) => json!(v),
                        ValueRef::Real(v) => serde_json::Number::from_f64(v)
                            .map(Value::Number)
                            .ok_or(Error(503, "Nonfinite SQL result"))?,
                        ValueRef::Text(v) => json!(
                            std::str::from_utf8(v).map_err(|_| Error(503, "Invalid SQL UTF8"))?
                        ),
                        ValueRef::Blob(v) => {
                            json!({"blob":v.iter().map(|v|format!("{v:02x}")).collect::<String>()})
                        }
                    });
                }
                rows.push(Value::Array(cells));
                if rows.len() == 129 {
                    break;
                }
            }
            changes.push(-1);
        } else {
            if event["mode"] != "batch" {
                return Err(Error(503, "Unknown Rust SQL mode"));
            }
            let changed = statement
                .execute(params_from_iter(params))
                .map_err(failure)? as i64;
            if item["changes"]
                .as_i64()
                .is_some_and(|expected| expected != changed)
            {
                return Err(Error(409, "Database write changed"));
            }
            changes.push(changed);
        }
    }
    Ok(json!({"op":"reply","frame":event["frame"],"rows":rows,"changes":changes}))
}

#[derive(Clone, Copy)]
pub struct Clock {
    pub monotonic: f64,
    pub wall: i64,
}
pub fn wall_now() -> Result<i64> {
    let wall = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map_err(|_| Error(503, "System clock unavailable"))?
        .as_secs();
    i64::try_from(wall).map_err(|_| Error(503, "System clock exceeds bounds"))
}
#[derive(Clone, Copy)]
pub struct Sources<'a> {
    pub accounts: u64,
    pub queue: Option<u64>,
    pub rooms: &'a crate::runtime::room_io::Observations,
}
/// Provisional native replies stay under the runtime's exclusive lock. A real
/// SQL COMMIT is the final fallible operation before publishing the result.
pub fn transaction<T>(
    owners: &mut Owners,
    db: &mut Connection,
    queue: Option<u64>,
    work: impl FnOnce(&mut Owners, &Connection) -> Result<T>,
) -> Result<T> {
    let tx = db
        .transaction_with_behavior(TransactionBehavior::Immediate)
        .map_err(failure)?;
    let execute = |owners: &mut Owners| {
        let result = work(owners, &tx)?;
        tx.commit().map_err(failure)?;
        Ok(result)
    };
    if let Some(queue) = queue {
        owners.queue_transaction(queue, execute)
    } else {
        execute(owners)
    }
}
pub fn drive(
    owners: &mut Owners,
    db: &Connection,
    setup: Value,
    clock: &Clock,
    sources: Sources,
) -> Result<(u64, Value)> {
    let directory = setup.get("party_directory");
    let id = owners.open(&setup)?;
    let mut metadata = serde_json::Map::new();
    if let Some(config) = directory {
        for (field, source) in [
            ("account_now", "account_source"),
            ("profile_now", "profile_source"),
            ("friend_now", "friend_source"),
            ("voice_now", "voice_source"),
            ("voice_account_now", "voice_source"),
            ("steam_now", "steam_source"),
        ] {
            if !config[source].is_null() {
                metadata.insert(field.to_owned(), json!(clock.monotonic));
            }
        }
        if !config["steam_source"].is_null() {
            metadata.insert(
                "steam_credential".into(),
                config["steam_credential"].clone(),
            );
        }
    }
    let send = |owners: &mut Owners, mut r: Value| {
        r.as_object_mut().unwrap().extend(metadata.clone());
        owners.request(id, &r)
    };
    let result = (|| {
        let mut event = send(owners, json!({"op":"begin"}))?;
        for _ in 0..4096 {
            match event["kind"].as_str() {
                Some("done") => {
                    if let Some(status) = event["rejection"]["status"].as_u64() {
                        return Err(Error(status as u16, "Request rejected"));
                    }
                    return Ok(event["value"].clone());
                }
                Some("sql") => event = send(owners, sql(db, &event)?)?,
                Some("provider") => {
                    let value = match event["method"].as_str() {
                        Some("rooms") => {
                            if let Some(source)=sources.queue {
                                owners.request(source,&json!({"op":"room_service_observations","now":sources.rooms.now(),"io_now":sources.rooms.now()}))?["matching"].clone()
                            } else {json!([])}
                        }
                        Some("room_connections") => owners.request(sources.accounts,&json!({"op":"presence","source":sources.queue,"now":sources.rooms.now(),"queue_now":sources.rooms.now(),"wall":wall_now()?,"rooms":sources.rooms.facts(),"view":"rooms"}))?,
                        Some("friend_presence") => owners.request(sources.accounts,&json!({"op":"presence","source":sources.queue,"now":sources.rooms.now(),"queue_now":sources.rooms.now(),"wall":wall_now()?,"rooms":sources.rooms.facts(),"view":"observed"}))?,
                        Some("friend_party_views") => {
                            let who = halo2_online_protocol::matchmaking_model::text(&event["args"]["player"],64)?;
                            let targets = event["args"]["targets"].as_array().ok_or(Error(503,"Native friend targets required"))?;
                            if targets.len()>250 {return Err(Error(503,"Too many native friend targets"));}
                            let mut result=Vec::with_capacity(targets.len());
                            for target in targets {
                                halo2_online_protocol::matchmaking_model::text(target,64)?;
                                let mut row=json!({});
                                for (field,mode) in [("joinable","friend_joinable"),("context","friend_context")] {
                                    let config=json!({"party_directory":{"family":"party","mode":mode,"source":sources.queue,"player":who,"target":target,"wall":clock.wall,"clock":clock.monotonic}});
                                    let (id,value)=drive(owners,db,config,clock,sources)?;
                                    owners.retire(id);
                                    row[field]=value;
                                }
                                result.push(row);
                            }
                            json!(result)
                        }
                        Some("presence_states") | Some("online") => {
                            let present=owners.request(sources.accounts,&json!({"op":"presence","source":sources.queue,"now":sources.rooms.now(),"queue_now":sources.rooms.now(),"wall":wall_now()?,"rooms":sources.rooms.facts(),"view":"observed"}))?;
                            let args = event["args"]
                                .as_array()
                                .ok_or(Error(503, "Native provider arguments required"))?;
                            json!(
                                args.iter()
                                    .map(|who| {
                                        let state = present
                                            .get(who.as_str().unwrap_or(""))
                                            .cloned()
                                            .unwrap_or(json!("offline"));
                                        if event["method"] == "online" {
                                            json!(state != "offline")
                                        } else {
                                            state
                                        }
                                    })
                                    .collect::<Vec<_>>()
                            )
                        }
                        Some("voice_allowed") => {
                            let pairs = event["args"].as_array().ok_or(Error(503,"Native voice pairs required"))?;
                            if pairs.len() > 256 { return Err(Error(503,"Too many voice relationship pairs")); }
                            let mut result = Vec::new();
                            for pair in pairs {
                                if !pair.as_array().is_some_and(|p|p.len()==2) { return Err(Error(503,"Native voice pair required")); }
                                let config=json!({"party_directory":{"family":"voice","mode":"allowed","source":sources.queue,"player":pair[0],"target":pair[1],"wall":clock.wall,"clock":clock.monotonic}});
                                let (id,value)=drive(owners,db,config,clock,sources)?;
                                owners.retire(id);
                                result.push(value);
                            }
                            json!(result)
                        }
                        _ => return Err(Error(503, "Required service capability unavailable")),
                    };
                    event = send(
                        owners,
                        json!({"op":"reply","frame":event["frame"],"value":value}),
                    )?;
                }
                _ => return Err(Error(503, "Unknown Rust service plan")),
            }
        }
        Err(Error(503, "Rust service plan exceeds bounds"))
    })();
    if result.is_err() {
        owners.retire(id);
    }
    result.map(|value| (id, value))
}
pub fn committed(owners: &mut Owners, id: u64, setup: &Value, clock: &Clock) -> Result<()> {
    committed_reply(owners, id, setup, clock).map(|_| ())
}
pub fn committed_reply(
    owners: &mut Owners,
    id: u64,
    setup: &Value,
    clock: &Clock,
) -> Result<Value> {
    if let Some(config) = setup.get("party_directory") {
        let mut r = json!({"op":"committed"});
        for (field, source) in [
            ("account_now", "account_source"),
            ("profile_now", "profile_source"),
            ("friend_now", "friend_source"),
            ("voice_now", "voice_source"),
            ("voice_account_now", "voice_source"),
            ("steam_now", "steam_source"),
        ] {
            if !config[source].is_null() {
                r[field] = json!(clock.monotonic);
            }
        }
        if !config["steam_source"].is_null() {
            r["steam_credential"] = config["steam_credential"].clone();
        }
        return owners.request(id, &r);
    }
    Ok(Value::Null)
}
