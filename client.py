import pickle
import asyncio
from time import sleep


# sending help
sep = '|'.encode('ascii')
insert = 'i'.encode('ascii')
get = 'g'.encode('ascii')
rm = 'd'.encode('ascii')

def itb(i): return str(i).encode('ascii')

def compose_add(message, key):
    message = pickle.dumps(message) #.encode('utf-8')
    key = pickle.dumps(key)
    payload = insert + itb(len(key)) + sep + itb(len(message)) + sep + key + message
    return payload

def compose_get(key):
    key = pickle.dumps(key)
    payload = get + itb(len(key)) + sep + key
    return payload

def compose_rm(key):
    key = pickle.dumps(key)
    payload = rm + itb(len(key)) + sep + key
    return payload

# status from the server
deleted = 'd'
none = 'n'


class fiveSeightS:
    '''
    reader = None
    writer = None
    async def __init__(self):
        self.reader, self.writer = await asyncio.open_connection('127.0.0.1', 5282)
    '''

    async def add(self, key, val):
        _, writer = await asyncio.open_connection('127.0.0.1', 5282)
        print("Sending: {}-{}".format(key,val))
        writer.write(compose_add(key,val))
        writer.close()
        return 0

    async def get(self, key):
        reader, writer = await asyncio.open_connection('127.0.0.1', 5282)
        writer.write(compose_get(key))
        data = (await reader.read(800)).decode()
        writer.close()
        return data

    async def rm(self, key):
        reader, writer = await asyncio.open_connection('127.0.0.1', 5282)
        writer.write(compose_rm(key))
        data = (await reader.read(800)).decode()
        writer.close()
        return data

async def test():
    client = fiveSeightS()
    for x in range(1,50):
        await client.add(x,[x,x/2,x/4,x*2])
    await asyncio.sleep(3)
    for x in range(1,50):
        value = await client.get(x)
        print(value)
        #conf = await client.rm(x)
    return 0

loop = asyncio.get_event_loop()
loop.run_until_complete(test())
loop.close()

'''
async def test_send(loop):
    reader, writer = await asyncio.open_connection('127.0.0.1', 8888, loop=loop)
    for x in range(0, 800):
         writer.write(compose_add("compose_add"))






client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client.connect(("127.0.0.1", 5282))
for x in range(0, 0):
    message = pickle.dumps({'dk':'TEST〇FSFS'}) #.encode('utf-8')
    key = pickle.dumps(x)
    payload = insert + itb(len(key)) + sep + itb(len(message)) + sep + key + message
    client.sendall(payload)


print("Deleting !")
for x in range(0, 80):

    client.sendall(payload)

print("Receiving !")
msg_rcv = 0
for x in range(0, 80):
    key = pickle.dumps(x)
    payload = get + itb(len(key)) + sep + key
    client.sendall(payload)
    resp = client.recv(40)
    print(resp)
    msg_rcv += 1
    #print(resp)

print("Received: {0} messages".format(msg_rcv))
client.close()
print("Closed socket, exiting !")
'''
