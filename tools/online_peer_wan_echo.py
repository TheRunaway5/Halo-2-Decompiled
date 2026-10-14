#!/usr/bin/env python3
"""Isolated WAN endpoint. SSH carries setup only; payloads use peer UDP."""
import asyncio
import json
import sys
from aiortc import RTCPeerConnection, RTCConfiguration, RTCIceServer, RTCSessionDescription
from aiortc.sdp import candidate_from_sdp


async def main():
    pc = RTCPeerConnection(RTCConfiguration(iceServers=[RTCIceServer('stun:master.halotoforever.com:3478')]))
    channel = pc.createDataChannel('halo2-native-datagrams-v1', negotiated=True, id=0,
                                   ordered=False, maxRetransmits=0)
    received = set()
    @channel.on('message')
    def message(data):
        assert isinstance(data, bytes) and len(data) == 1400 and data[0] < 50
        assert data == bytes([data[0]]) + bytes((i ^ data[0]) & 255 for i in range(1, 1400))
        received.add(data[0])
        channel.send(data)
    reader = asyncio.StreamReader()
    protocol = asyncio.StreamReaderProtocol(reader)
    await asyncio.get_running_loop().connect_read_pipe(lambda: protocol, sys.stdin)
    try:
        async with asyncio.timeout(45):
            while line := await reader.readline():
                command = json.loads(line)
                if command.get('stop'):
                    break
                s = command['signal']
                if s['type'] == 'offer':
                    await pc.setRemoteDescription(RTCSessionDescription(s['payload'], 'offer'))
                    await pc.setLocalDescription(await pc.createAnswer())
                    print(json.dumps(dict(signal=dict(peer=0, type='answer', payload=pc.localDescription.sdp, mid=''))), flush=True)
                elif s['type'] == 'candidate':
                    candidate = candidate_from_sdp(s['payload'].removeprefix('candidate:'))
                    assert candidate.protocol.lower() == 'udp' and candidate.type != 'relay'
                    candidate.sdpMid = s['mid']
                    await pc.addIceCandidate(candidate)
        print(json.dumps(dict(done=True, received=len(received))), flush=True)
        assert len(received) >= 45
    finally:
        await pc.close()


if __name__ == '__main__':
    asyncio.run(main())
