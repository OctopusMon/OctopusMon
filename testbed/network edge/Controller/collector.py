import socket
import time
import json
import struct

ip = "127.0.0.1"
port = 10005
threshold_size = 100

def sendString(string, client_socket):
    len_s = len(string)
    f = struct.pack("i", len_s)
    client_socket.send(f)
    client_socket.sendall(string.encode("utf-8"))
    return

def receiveString(client_socket):
    d = client_socket.recv(struct.calcsize("i"))
    len_s = struct.unpack("i", d)
    num = len_s[0]//1024
    data = ""
    for i in range(num):
        data += client_socket.recv(1024).decode("utf-8")
    data += client_socket.recv(len_s[0]%1024).decode('utf-8')
    return data


if __name__ == "__main__":
    s = socket.socket()
    s.bind((ip,port))
    s.listen(1)
    conn, addr = s.accept()
    while True:
        print("begin receive")
        data = receiveString(conn)
        data = json.loads(data)
        print("end receive")
        sendString("hello",conn)
        print("end send")
    conn.close()