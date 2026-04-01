import asyncio

import uvloop

async def main():
    async def worker():
        reader, writer = await asyncio.open_connection("dstat.st", 80)
        request = (
            b"GET / HTTP/1.1\r\n"
            b"Host: dstat.st\r\n"
            b"Connection: keep-alive\r\n"
            b"\r\n"
        )
        while True:
            writer.write(request)
            await writer.drain()
    await asyncio.gather(*(worker() for _ in range(400)))

uvloop.run(main())
