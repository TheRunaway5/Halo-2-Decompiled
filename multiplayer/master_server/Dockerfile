FROM rust:1.96.0-slim AS security
WORKDIR /build
COPY rust/online_protocol /build/rust/online_protocol
COPY rust/online_security_ffi /build/rust/online_security_ffi
RUN cargo build --locked --release --manifest-path rust/online_security_ffi/Cargo.toml

FROM python:3.13-slim
ENV PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1 H2_MASTER_STATE=/state H2_MASTER_BIND=0.0.0.0
WORKDIR /app
COPY multiplayer/master_server/requirements.txt /app/requirements.txt
RUN pip install --no-cache-dir -r requirements.txt \
    && groupadd --gid 10001 directory \
    && useradd --uid 10001 --gid 10001 --no-create-home directory \
    && mkdir -p /state/a /state/b && chown -R 10001:10001 /state
COPY multiplayer/master_server/*.py multiplayer/master_server/retail_gathering.json /app/multiplayer/master_server/
COPY multiplayer/master_server/data/retail-launch-2004-rank.json multiplayer/master_server/data/title-update-1.5-rank.json /app/multiplayer/master_server/data/
COPY --from=security /build/rust/online_security_ffi/target/release/libhalo2_online_security_ffi.so /app/multiplayer/master_server/
USER 10001:10001
EXPOSE 8080
HEALTHCHECK --interval=15s --timeout=4s --start-period=5s --retries=3 \
  CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8080/v1/health',timeout=3).read()"
CMD ["python", "-m", "multiplayer.master_server"]
