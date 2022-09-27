from random import randint, sample
import networkx as nx
from ns.flow.flow import Flow

def read_topo(fname):
    ftype = ".graphml"
    if fname.endswith(ftype):
        return nx.read_graphml(fname)
    else:
        print(f"{fname} is not GraphML")

def cal_gp(src,k):

    mu = k*k*5//4
    pod = (src-mu)//(k*k//4)
    lala = k*k//4 + k*pod + k//2 + (src-mu-pod*k*k//4)//(k//2)
    return lala


class PathGenerator:
    def __init__(self,G,hosts,k):
        self.topo=G
        self.hosts=hosts
        self.pathDict = {}
        # tmp=0
        # for src in hosts:
        #     for dst in hosts:
        #         tmp+=1
        #         print("tmp",tmp)
        #         gp1 = cal_gp(src,int(k))
        #         gp2 = cal_gp(dst,int(k))
        #         if gp1==gp2:
        #             continue
        #         self.pathDict[(src,dst)]=list(nx.all_shortest_paths(G, src, dst))
                # self.pathDict[(src,dst)]=list(nx.all_simple_paths(G, src, dst, cutoff=nx.diameter(G)))

    def generate_flows(self, nflows,k=8):
        all_flows = dict()
        # print("dfa")
        for flow_id in range(nflows):
            # print("flow",flow_id)
            while True:
                # print("murmur1")
                src, dst = sample(self.hosts, 2)
                # print("murmur2")
                gp1 = cal_gp(src,int(k))
                gp2 = cal_gp(dst,int(k))
                if gp1==gp2:
                    # print("again")
                    continue

                # print("murmur3")
                p1 = randint(0,65535)
                p2 = randint(0,65535)
                ptc = randint(0,255)
                fid = (gp1<<92)+(src<<80)+(0<<72)+(gp2<<60)+(dst<<48)+(0<<40)+(p1<<24)+(p2<<8)+ptc
                # fid = (gp1<<88)+(src<<80)+(src<<72)+(gp2<<56)+(dst<<48)+(dst<<40)+(p1<<24)+(p2<<8)+ptc
                all_flows[flow_id] = Flow(fid, src, dst)
                # print("murmur4")
                # all_flows[flow_id].path = sample(
                #     list(nx.all_simple_paths(G, src, dst, cutoff=nx.diameter(G))),
                #     1)[0]
                all_flows[flow_id].path = sample(
                    list(nx.all_shortest_paths(self.topo, src, dst)),
                    1)[0]
                # print(all_flows[flow_id].path)
                # all_flows[flow_id].path = sample(
                #     self.pathDict[(src,dst)],
                #     1)[0]
                # print("murmur5")                
                break
            
        return all_flows


def generate_fib(G, all_flows):
    for n in G.nodes():
        node = G.nodes[n]

        node['port_to_nexthop'] = dict()
        node['nexthop_to_port'] = dict()

        for port, nh in enumerate(nx.neighbors(G, n)):
            node['nexthop_to_port'][nh] = port
            node['port_to_nexthop'][port] = nh

        node['flow_to_port'] = dict()
        node['flow_to_nexthop'] = dict()

    for f in all_flows:
        flow = all_flows[f]
        path = list(zip(flow.path, flow.path[1:]))
        for seg in path:
            a, z = seg
            G.nodes[a]['flow_to_port'][
                flow.fid] = G.nodes[a]['nexthop_to_port'][z]
            G.nodes[a]['flow_to_nexthop'][flow.fid] = z

    return G
