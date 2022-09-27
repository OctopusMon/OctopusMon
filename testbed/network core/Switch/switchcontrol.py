import json
import time
import os
import sys
import glob
import signal
import argparse
import socket
import struct
from tracemalloc import start

bucket_num = 4096
bucket_tag_num = 1024
cell_num = 65536

s = socket.socket()
#target server ip and port
server_ip = "172.16.6.141"
server_port = 50001
s.connect((server_ip,server_port))

total_time = 0
p4 = bfrt.guanyu_cell.pipe
bucket1 = p4.MyEgress.bucket1
bucket1_srciptag = p4.MyEgress.bucket1_srciptag
bucket1_dstiptag = p4.MyEgress.bucket1_dstiptag
bucket2 = p4.MyEgress.bucket2
bucket2_srciptag = p4.MyEgress.bucket2_srciptag
bucket2_dstiptag = p4.MyEgress.bucket2_dstiptag
cell_bucket1_part1 = p4.MyEgress.cell_bucket1_part1
cell_bucket1_part2 = p4.MyEgress.cell_bucket1_part2
bucket1_to_cell_part1 = p4.MyEgress.bucket1_to_cell_part1
bucket1_to_cell_part2 = p4.MyEgress.bucket1_to_cell_part2
bucket2_to_cell_part1 = p4.MyEgress.bucket2_to_cell_part1
bucket2_to_cell_part2 = p4.MyEgress.bucket2_to_cell_part2

#loop times
for loop in range(50):
    #start time : The time Dump operation start
    start_time = time.time()
    divide_threshold = 0
    data = {}
    #dump data from switch data plane
    bucket1_result = bucket1.dump(json=True, from_hw=True)
    bucket2_result = bucket2.dump(json=True, from_hw=True)
    bucket1_srciptag_result = bucket1_srciptag.dump(json=True, from_hw=True)
    bucket1_dstiptag_result = bucket1_dstiptag.dump(json=True, from_hw=True)
    bucket2_srciptag_result = bucket2_srciptag.dump(json=True, from_hw=True)
    bucket2_dstiptag_result = bucket2_dstiptag.dump(json=True, from_hw=True)
    cell_bucket1_part1_result = cell_bucket1_part1.dump(json=True, from_hw=True)
    cell_bucket1_part2_result = cell_bucket1_part2.dump(json=True, from_hw=True)
    bucket1_data = json.loads(bucket1_result)
    bucket2_data = json.loads(bucket2_result)
    bucket1_srciptag_data = json.loads(bucket1_srciptag_result)
    bucket1_dstiptag_data = json.loads(bucket1_dstiptag_result)
    bucket2_srciptag_data = json.loads(bucket2_srciptag_result)
    bucket2_dstiptag_data = json.loads(bucket2_dstiptag_result)
    cell_bucket1_part1_data = json.loads(cell_bucket1_part1_result)
    cell_bucket1_part2_data = json.loads(cell_bucket1_part2_result)
    #if needed , also dump mapping relation from dataplane. This results in higher latency and more bandwidth consumption.
    #bucket1_to_cell_part1_result = bucket1_to_cell_part1.dump(json=True, from_hw=True)
    #bucket1_to_cell_part2_result = bucket1_to_cell_part2.dump(json=True, from_hw=True)
    #bucket2_to_cell_part1_result = bucket2_to_cell_part1.dump(json=True, from_hw=True)
    #bucket2_to_cell_part2_result = bucket2_to_cell_part2.dump(json=True, from_hw=True)
    bucket1_dict = []
    bucket2_dict = []
    bucket1_dstiptag_dict = []
    bucket1_srciptag_dict = []
    bucket2_dstiptag_dict = []
    bucket2_srciptag_dict = []
    cell_bucket1_part1_dict = []
    cell_bucket1_part2_dict = []
    #The original register data is large and contains a lot of useless data resources, which are simply filtered and organized into the form of index-value
    for i in range (128):
        bucket1_sip_temp = {"index":i}
        bucket2_sip_temp = {"index":i}
        bucket1_dip_temp = {"index":i}
        bucket2_dip_temp = {"index":i}
        bucket1_sip_temp["src"] = bucket1_srciptag_data[i]["data"]["MyEgress.bucket1_srciptag.f1"][3]
        bucket2_sip_temp["src"] = bucket2_srciptag_data[i]["data"]["MyEgress.bucket2_srciptag.f1"][3]
        bucket1_dip_temp["dst"] = bucket1_dstiptag_data[i]["data"]["MyEgress.bucket1_dstiptag.f1"][3]
        bucket2_dip_temp["dst"] = bucket2_dstiptag_data[i]["data"]["MyEgress.bucket2_dstiptag.f1"][3]
        bucket1_srciptag_dict.append(bucket1_sip_temp)
        bucket2_srciptag_dict.append(bucket2_sip_temp)
        bucket1_dstiptag_dict.append(bucket1_dip_temp)
        bucket2_dstiptag_dict.append(bucket2_dip_temp)
        for j in range(4):
            bucket1_temp = {"index":i*4+j}
            bucket2_temp = {"index":i*4+j}
            bucket1_temp["value"] = bucket1_data[i*4 + j]["data"]["MyEgress.bucket1.f1"][3]
            bucket2_temp["value"] = bucket2_data[i*4 + j]["data"]["MyEgress.bucket2.f1"][3]
            bucket1_dict.append(bucket1_temp)
            bucket2_dict.append(bucket2_temp)
            cell_part1_temp = {"index":i*4 + j}
            cell_part2_temp = {"index":i*4 + j}
            cell_part1_temp["value"] = cell_bucket1_part1_data[i*4 + j]["data"]["MyEgress.cell_bucket1_part1.f1"][3]
            cell_part2_temp["value"] = cell_bucket1_part2_data[i*4 + j]["data"]["MyEgress.cell_bucket1_part2.f1"][3]
            cell_bucket1_part1_dict.append(cell_part1_temp)
            cell_bucket1_part2_dict.append(cell_part2_temp)
    print("data load over\n")
    #change str to json
    data["bucket1"] = bucket1_dict
    data["bucket2"] = bucket2_dict
    data["bucket1_srcip"] = bucket1_srciptag_dict
    data["bucket1_dstip"] = bucket1_dstiptag_dict
    data["bucket2_srcip"] = bucket2_srciptag_dict
    data["bucket2_dstip"] = bucket2_dstiptag_dict
    data["cell_bucket1_part1"] = cell_bucket1_part1_dict
    data["cell_bucket1_part2"] = cell_bucket1_part2_dict
    data_to_send = json.dumps(data)
    #middle time : The time at which data processing is completed
    middle_time = time.time()
    len_send_data = len(data_to_send)
    f = struct.pack("i", len_send_data)
    s.send(f)
    #send reshaped data to center servers
    s.sendall(data_to_send.encode("utf-8"))
    #if needed, keep a copy of the data locally
    '''with open("flow_dqueue_time_data.json","w") as f:
            f.seek(0)
            f.truncate()
            json.dump(data_to_send,f)
            print("loading file over...")'''
    #end time : The time all the data has been sent
    end_time = time.time()
    #print(middle_time - start_time)
    #print(end_time - start_time)
    #print("send_time:"+str(end_time-middle_time))
    #total_time += end_time - middle_time
    #sleep time : 
    time.sleep(5 - end_time + start_time)

    #print(total_time)
