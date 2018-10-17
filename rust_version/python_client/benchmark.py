import asyncio
import aioredis
import fses
from time import time


loop = asyncio.get_event_loop()
nr_itt = int(2 * 1000)


async def send_redis(nr_itt, loop):
    pool = await aioredis.create_pool('redis://localhost', minsize=30, maxsize=30, loop=loop)

    futures = []
    for i in range(0,nr_itt):
        op = pool.execute('set', 'K{}'.format(i), 'Value Number {} !'.format(i))
        futures.append(op)
        if len(futures) > 29:
            await asyncio.gather(*futures)
            futures = []
    await asyncio.gather(*futures)

    futures = []
    for i in range(0,nr_itt):
        op = pool.execute('get', 'K{}'.format(i))
        futures.append(op)
        if len(futures) > 29:
            await asyncio.gather(*futures)
            futures = []
    await asyncio.gather(*futures)

    pool.close()
    await pool.wait_closed()


async def send_5s8s(nr_itt, loop):
    client = fses.Client(no_connections=3)
    await client.async_init()

    futures = []
    for i in range(0,nr_itt):
        op = client.add(str(i), str(i))
        futures.append(op)
        if len(futures) > 2:
            await asyncio.gather(*futures)
            futures = []
    await asyncio.gather(*futures)


    futures = []
    for i in range(0,nr_itt):
        op = client.get(str(i))
        futures.append(op)
        if len(futures) > 2:
            await asyncio.gather(*futures)
            futures = []
    await asyncio.gather(*futures)

    client.close()


a = time()
loop.run_until_complete(send_redis(nr_itt, loop))
print('Benchmarked average R+W time on redis: {} nanoseconds'.format( round(((time() - a)/nr_itt)*pow(10,9)) ))

a = time()
loop.run_until_complete(send_5s8s(nr_itt, loop))
print('Benchmarked average R+W time on 5s8s: {} nanoseconds'.format( round(((time() - a)/nr_itt)*pow(10,9)) ))

loop.close()
