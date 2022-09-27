import json
import re
import sys
import binascii
import socket
import threading
import time
import struct


table_size = 1024
print_flag = 1
server_ip = "10.0.0.1"
server_port = 10000

elastic_regs_0 = [bfrt.elastic_coco.pipe.Ingress.freq_vote_01, bfrt.elastic_coco.pipe.Ingress.freq_vote_02, bfrt.elastic_coco.pipe.Ingress.freq_vote_03, bfrt.elastic_coco.pipe.Egress.freq_vote_04]
elastic_id_strs_0 = ["Ingress.freq_vote_01.lo","Ingress.freq_vote_02.lo","Ingress.freq_vote_03.lo","Egress.freq_vote_04.lo"]
elastic_counter_strs_0 = ["Ingress.freq_vote_01.hi","Ingress.freq_vote_02.hi","Ingress.freq_vote_03.hi","Egress.freq_vote_04.hi"]
elastic_regs_1 = [bfrt.elastic_coco.pipe.Ingress.freq_vote_11, bfrt.elastic_coco.pipe.Ingress.freq_vote_12, bfrt.elastic_coco.pipe.Ingress.freq_vote_13, bfrt.elastic_coco.pipe.Egress.freq_vote_14]
elastic_id_strs_1 = ["Ingress.freq_vote_11.lo","Ingress.freq_vote_12.lo","Ingress.freq_vote_13.lo","Egress.freq_vote_14.lo"]
elastic_counter_strs_1 = ["Ingress.freq_vote_11.hi","Ingress.freq_vote_12.hi","Ingress.freq_vote_13.hi","Egress.freq_vote_14.hi"]

### main begin
s = socket.socket()
s.connect((server_ip, server_port))
if print_flag == 1:
    print("main begin")
    start_time = time.clock()
if print_flag == 1:
    print("start time: %f"%(start_time))

### get choose flag
flag_table = bfrt.elastic_coco.pipe.Ingress.sketch_choose_flag
flag_text = flag_table.dump(json=True, from_hw=True)
flag_data = json.loads(flag_text)
choose_flag = flag_data[0]['data']['Ingress.sketch_choose_flag.f1'][0]
get_time = time.clock()
if print_flag == 1:
    print("choose flag: %d"%(choose_flag))
    print("get flag time: %f" % (get_time-start_time))

### change choose flag
if choose_flag == 0:
    flag_table.mod(0,1)
else:
    flag_table.mod(0,0)
if print_flag == 1:
    mod_time = time.clock()
    print("mode flag time: %f" % (mod_time-get_time))

### collect data 
res_in = {}
res_out = {}

### elastic data 
for i in range(4):
    if choose_flag == 0:
        text = elastic_regs_0[i].dump(json=True, from_hw=True)
        table = json.loads(text)
        id_str = elastic_id_strs_0[i]
        counter_str = elastic_counter_strs_0[i]
    else :
        text = elastic_regs_1[i].dump(json=True, from_hw=True)
        table = json.loads(text)
        id_str = elastic_id_strs_1[i]
        counter_str = elastic_counter_strs_1[i]
    
    for j in range(table_size):
        id = table[j]['data'][id_str][0]
        counter = table[j]['data'][counter_str][0]
        if id != 0 and counter != 0:
            if id not in res_in:
                res_in[id] = counter
            else:
                res_in[id] += counter

    for j in range(table_size, table_size+table_size):
        id = table[j]['data'][id_str][0]
        counter = table[j]['data'][counter_str][0]
        if id != 0 and counter != 0:
            if id not in res_in:
                res_out[id] = counter
            else:
                res_out[id] += counter

if print_flag == 1:
    elastic_time = time.clock()
    print("dump elastic time: %f" % (elastic_time-mod_time))

### coco data
if choose_flag == 0:
    counter_table = bfrt.elastic_coco.pipe.Egress.coco_counter_01
    id_table = bfrt.elastic_coco.pipe.Egress.coco_id_01
    counter_string = "Egress.coco_counter_01.f1"
    id_string = "Egress.coco_id_01.f1"
else:
    counter_table = bfrt.elastic_coco.pipe.Egress.coco_counter_11
    id_table = bfrt.elastic_coco.pipe.Egress.coco_id_11
    counter_string = "Egress.coco_counter_11.f1"
    id_string = "Egress.coco_id_11.f1"

res = {}
counter_text = counter_table.dump(json=True, from_hw=True)
counters = json.loads(counter_text)
id_text = id_table.dump(json=True, from_hw=True)
ids = json.loads(id_text)

for i in range(table_size):
    id = ids[i]['data'][id_string][0]
    counter = counters[i]['data'][counter_string][0]
    if id != 0 and counter != 0:
        if id not in res:
            res_in[id] = counter
        else:
            res_in[id] += counter

for i in range(table_size,table_size+table_size):
    id = ids[i]['data'][id_string][0]
    counter = counters[i]['data'][counter_string][0]
    if id != 0 and counter != 0:
        if id not in res:
            res_out[id] = counter
        else:
            res_out[id] += counter
if print_flag == 1:
    coco_time = time.clock()
    print("dump coco time: %f" % (coco_time-elastic_time))

### data / 32
for id in res_in:
    res_in[id] /= 32
for id in res_out:
    res_out[id] /= 32

###send data
data = {"in":res_in,"out":res_out}
data = json.dumps(data)
if print_flag == 1:
    print(data)
    dump_end_time = time.clock()
    print("total dump spending time: %f" % (dump_end_time-start_time))

len_send_data = len(data)
f = struct.pack("i", len_send_data)
s.send(f)
s.sendall(data.encode("utf-8"))

### receive data
d = s.recv(struct.calcsize("i"))
len_recv_data = struct.unpack("i", d)
num = len_recv_data[0]//1024
recv_data = ""
for i in range(num):
    recv_data += s.recv(1024).decode("utf-8")
recv_data += s.recv(len_recv_data[0]%1024).decode('utf-8')

if print_flag == 1:
    tcp_end_time = time.clock()
    print("total tcp send time: %f" % (tcp_end_time-dump_end_time))

### set INT
ids = json.dumps(recv_data)
int_table = bfrt.elastic_coco.pipe.Ingress.mark_INT_table
int_table.clear()
for id in ids:
    int_table.add_with_mark_INT(id)
if print_flag == 1:
    end_time = time.clock()
    print("total spending time: %f" % (end_time-start_time))

s.close()