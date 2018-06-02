import socket
import sys
import pickle


def itb(i): return str(i).encode('ascii')#"{0:031b}".format(i).encode('ascii')
sep = '|'.encode('ascii')
insert = 'i'.encode('ascii')
get = 'g'.encode('ascii')

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client.connect(("127.0.0.1", 5282))
for x in range(0, 20):
    message = pickle.dumps("〇")
    key = pickle.dumps(x)
    payload = insert + itb(len(key)) + sep + itb(len(message)) + sep + key + message
    print("Sending data")
    client.send(payload)

for x in range(0, 20):
    key = pickle.dumps(x + 54353)
    payload = get + itb(len(key)) + sep + key
    client.send(payload)
    print("Expecting data")
    resp = client.recv(4096)
    print(resp)
