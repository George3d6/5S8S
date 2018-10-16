import asyncio
import aioredis
from time import time


async def send_redis(nr_itt, loop):
    pool = await aioredis.create_pool('redis://localhost',
        minsize=1, maxsize=1, loop=loop)

    futures = []
    for i in range(0,nr_itt):
        op = pool.execute('set', 'K{}'.format(i), 'Value Number {} !'.format(i))
        await op

    for i in range(0,nr_itt):
        op = pool.execute('get', 'K{}'.format(i))
        await op

    pool.close()
    await pool.wait_closed()


async def send_5s8s(nr_itt, loop):
    conn = asyncio.open_connection('localhost', 5282)
    reader, writer = await conn

    futures = []
    for i in range(0,nr_itt):
        writer.write('a{}K{}Value Number {} !\n'.format(str(1 + len(str(i))).zfill(5),i,i).encode('utf-8'))
        msg = await reader.read(50)

    for i in range(0,nr_itt):
        writer.write('gK{}\n'.format(i).encode('utf-8'))
        msg = await reader.read(255)


    writer.close()


loop = asyncio.get_event_loop()
a = time()
nr_itt = 50 * 1000
loop.run_until_complete(send_5s8s(nr_itt, loop))
print('Benchmark time on 5s8s: {}'.format(time() - a))
b = time()
loop.run_until_complete(send_redis(nr_itt, loop))
print('Benchmark time on redis: {}'.format(time() - b))
loop.close()
