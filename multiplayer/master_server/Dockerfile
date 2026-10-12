FROM python:3.13-slim
ENV PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1 H2_MASTER_STATE=/state H2_MASTER_BIND=0.0.0.0
WORKDIR /app
COPY requirements.txt /app/requirements.txt
RUN pip install --no-cache-dir -r requirements.txt \
    && groupadd --gid 10001 directory \
    && useradd --uid 10001 --gid 10001 --no-create-home directory \
    && mkdir -p /state/a /state/b && chown -R 10001:10001 /state
COPY . /app/multiplayer/master_server
USER 10001:10001
EXPOSE 8080
HEALTHCHECK --interval=15s --timeout=4s --start-period=5s --retries=3 \
  CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8080/v1/health',timeout=3).read()"
CMD ["python", "-m", "multiplayer.master_server"]
