"""Real native probe socket: strict parsing, echo-only contract and recovery."""
import asyncio
import json
from pathlib import Path
import sys
from aiohttp import web, WSMsgType
from aiohttp.test_utils import TestServer


async def main():
    connections, completed = [], []
    errors = []
    async def connect(request):
        socket = web.WebSocketResponse()
        try:
            assert request.headers.get("Authorization") == "Bearer "+"s"*43
            await socket.prepare(request)
            connections.append(socket)
            number = len(connections)
            if number == 1:
                await socket.send_str('{"probe":"'+"a"*32+'","probe":"'+"b"*32+'"}')
            elif number == 2:
                await socket.send_json({"probe":"c"*32,"rank":127})
            else:
                await socket.send_json({"ping":"d"*32})
                reply = await asyncio.wait_for(socket.receive(),1)
                assert reply.type == WSMsgType.TEXT and json.loads(reply.data)=={"pong":"d"*32}
                await socket.send_json({"probe":"e"*32})
                reply = await asyncio.wait_for(socket.receive(),1)
                assert reply.type == WSMsgType.TEXT and json.loads(reply.data)=={"echo":"e"*32}
                completed.append(number)
                if number == 3:
                    await socket.send_bytes(b"invalid binary search data")
                else:
                    await socket.close()
            if number <= 3:
                async with asyncio.timeout(1):
                    async for message in socket:
                        assert message.type not in (WSMsgType.TEXT,WSMsgType.BINARY), "Native echoed malformed control"
            return socket
        except Exception as error:
            errors.append(type(error).__name__)
            await socket.close()
            return socket
    app = web.Application()
    app.router.add_get("/v1/matchmaking/qos",connect)
    async with TestServer(app) as server:
        process = await asyncio.create_subprocess_exec(str(Path(sys.argv[1]).resolve()),str(server.make_url("/")).rstrip("/"),
            stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.PIPE)
        try:
            out,err = await asyncio.wait_for(process.communicate(),9)
            print(out.decode(),end="")
            print(err.decode(),end="",file=sys.stderr)
            assert process.returncode == 0
            assert not errors,errors
            assert completed[:2] == [3,4],(len(connections),completed)
            print("PASS native rejects duplicate/extra/binary controls, returns challenges only, reconnects after failure")
        finally:
            if process.returncode is None:
                process.kill()
                await process.wait()
            await asyncio.gather(*(s.close() for s in connections))


if __name__ == "__main__":
    asyncio.run(main())
