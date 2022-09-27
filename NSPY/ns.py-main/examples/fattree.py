from functools import partial
from random import expovariate, sample

import Sketches
import numpy as np
import simpy

from ns.packet.dist_generator import DistPacketGenerator
from ns.packet.sink import PacketSink
from ns.switch.switch import SimplePacketSwitch
from ns.switch.switch import FairPacketSwitch
from ns.switch.switch import CheckSwitch
from ns.topos.fattree import build as build_fattree
from ns.topos.utils import generate_fib, generate_flows

env = simpy.Environment()

n_flows = 100
k = 4
pir = 100000
buffer_size = 1000
mean_pkt_size = 100.0
ab_fgroup = set()
sketches = Sketches.Sketches()

ft = build_fattree(k)

hosts = set()
for n in ft.nodes():
    if ft.nodes[n]['type'] == 'host':
        hosts.add(n)

all_flows = generate_flows(ft, hosts, n_flows)
size_dist = partial(expovariate, 1.0 / mean_pkt_size)

# for fid in all_flows:
for flow_id, flow in all_flows.items():
    arr_dist = partial(expovariate, 1 + np.random.rand())

    pg = DistPacketGenerator(env,
                             f"Flow_{flow.fid}",
                             arr_dist,
                             size_dist,
                             flow_id=flow.fid)
    ps = PacketSink(env)

    all_flows[flow_id].pkt_gen = pg
    all_flows[flow_id].pkt_sink = ps

ft = generate_fib(ft, all_flows)

n_classes_per_port = 4
weights = {c: 1 for c in range(n_classes_per_port)}


def flow_to_classes(f_id, n_id=0, fib=None):
    return (f_id + n_id + fib[f_id]) % n_classes_per_port

cnt_edge=0
cnt_core=0
for node_id in ft.nodes():
    node = ft.nodes[node_id]
    # node['device'] = SimplePacketSwitch(env, k, pir, buffer_size, element_id=f"{node_id}")
    flow_classes = partial(flow_to_classes,
                           n_id=node_id,
                           fib=node['flow_to_port'])
    if node['layer']=='edge':
        sketch1 = sketches.insketch[cnt_edge]
        sketch2 = sketches.outsketch[cnt_edge]
        cnt_edge+=1
        node['device'] = FairPacketSwitch(env,k,pir,buffer_size,weights,'DRR',flow_classes,
                                        element_id=f"{node_id}",
                                        layer=node['layer'],
                                        ab_fgroup=ab_fgroup,
                                        sketches=sketches,
                                        sketch1=sketch1,
                                        sketch2=sketch2)
    elif node['layer']=='aggregation' or node['layer']=='core':
        sketch1 = sketches.coresketch[cnt_core]
        cnt_core+=1
        node['device'] = FairPacketSwitch(env,k,pir,buffer_size,weights,'DRR',flow_classes,
                                        element_id=f"{node_id}",
                                        layer=node['layer'],
                                        ab_fgroup=ab_fgroup,
                                        sketches=sketches,
                                        sketch1=sketch1)
    elif node['layer']=='leaf':
        node['device'] = FairPacketSwitch(env,k,pir,buffer_size,weights,'DRR',flow_classes,
                                        element_id=f"{node_id}",
                                        layer=node['layer'],
                                        ab_fgroup=ab_fgroup,
                                        sketches=sketches)

    node['device'].demux.fib = node['flow_to_port']

for n in ft.nodes():
    node = ft.nodes[n]
    for port_number, next_hop in node['port_to_nexthop'].items():
        node['device'].ports[port_number].out = ft.nodes[next_hop]['device']

for flow_id, flow in all_flows.items():
    flow.pkt_gen.out = ft.nodes[flow.src]['device']
    ft.nodes[flow.dst]['device'].demux.ends[flow.fid] = flow.pkt_sink

check=CheckSwitch(env,ft,ab_fgroup,sketches,all_flows,15)

env.run(until=100)

# for flow_id in all_flows.keys():
#     print(flow_id,all_flows[flow_id].fid,all_flows[flow_id].src,all_flows[flow_id].dst)

# for flow_id in sample(all_flows.keys(), 5):
    # print(f"Flow {flow_id}")
    # print("waits",all_flows[flow_id].pkt_sink.waits)
    # print("arrivals",all_flows/[flow_id].pkt_sink.arrivals)
    # print("perhop_times",all_flows[flow_id].pkt_sink.perhop_times)

# for flow_id in range(40,44):
#     flow = all_flows[flow_id]
#     length = len(flow.path)
#     # swi = ft.nodes[flow.src]['device']
#     # nxt_hop = ft.nodes[flow.path[2]]['flow_to_nexthop'][flow.fid]
#     last_freq = ft.nodes[flow.path[length-3]]['device'].sketch1.Query(flow.fid)
#     # while ft.nodes[nxt_hop]['layer']!='edge':
#     #     freqs.append( ft.nodes[nxt_hop]['device'].sketch1.query(flow.fid) )
#     #     cnt+=1

#     for i in range(2,length-2):
#         freq = ft.nodes[flow.path[i]]['device'].sketch1.Query(flow.fid)
#         if freq/(last_freq+0.01) < 2:
#             print("flow",flow_id,flow.path[i])