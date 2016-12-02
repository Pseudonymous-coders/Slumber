import socket
import json
import threading
import time
import random

TCP_IP = "127.0.0.1"
TCP_PORT = 3005
BUFFER_SIZE = 512

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

ready_write = False
continuer = True
continue_text = ""


def reader(client: socket.socket):
    global ready_write, continue_text, continuer
    while True:
        recv = client.recv(BUFFER_SIZE).decode('utf-8')
        print(recv)
        json_data = json.loads(recv)

        print("Got data: %s" % str(json_data))

        try:
            continuer = (continue_text == json_data["check"])
            continue
        except KeyError:
            pass

        try:
            ready_write = json_data["state"]
        except KeyError:
            pass


thread = threading.Thread(target=reader, args=(s,))
thread.setDaemon(True)
thread.start()

def rand():
    return round(random.uniform(0, 100))

count = 0
while True:
    if not ready_write:
        continue

    #if not continuer:
        #time.sleep(0.1)
        #continuer = True
        #continue
    to_send = {"response": "liveUpdate", "data": {"sleepScore": rand(), "accelX": rand(), "accelY": rand(), "accelZ": rand(), "temp": rand(), "hum": rand()}}
    continue_text = json.dumps(to_send)
    s.send(continue_text.encode('utf-8'))
    print("Sending %s" % str(to_send))
    time.sleep(5)
s.close()
