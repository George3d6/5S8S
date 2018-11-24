import asyncio


class Client:
    pool = []
    def __init__(self, host='localhost', port=5282, no_connections=3):
        self.host = host
        self.port = port
        self.no_connections = no_connections

    async def async_init(self):
        for i in range(0, self.no_connections):
            reader, writer = await asyncio.open_connection(self.host, self.port)
            self.pool.append([reader,writer,False,i])



    async def add(self, key, value):
        while True:
            for conn in self.pool:
                if conn[2] == False:
                    conn[2] = True
                    conn[1].write('a{}{}{}{}'.format( str(len(key)).zfill(8),  str(len(value)).zfill(8), key, value ).encode('utf-8'))
                    status = await conn[0].read(254)
                    print(status)
                    conn[2] = False
                    return status
            await asyncio.sleep(0.01)

    async def get(self, key):
        while True:
            for conn in self.pool:
                if conn[2] == False:
                    conn[2] = True
                    conn[1].write('g{}{}'.format(str(len(key)).zfill(8), key).encode('utf-8'))
                    value = await conn[0].read(255)
                    conn[2] = False
                    return value
            await asyncio.sleep(0.1)

    def close(self):
        for conn in self.pool:
            conn[1].close()

    def __del__(self):
        self.close()
