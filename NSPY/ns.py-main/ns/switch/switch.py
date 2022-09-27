""" 
Implements a packet switch with FIFO or WFQ/DRR/Virtual Clock bounded buffers for outgoing ports.
"""
from collections.abc import Callable

from ns.port.port import Port
from ns.demux.fib_demux import FIBDemux
from ns.scheduler.wfq import WFQServer
from ns.scheduler.drr import DRRServer
from ns.scheduler.virtual_clock import VirtualClockServer
from ns.scheduler.sp import SPServer


class SimplePacketSwitch:
    """ Implements a packet switch with a FIFO bounded buffer on each of the outgoing ports.

        Parameters
        ----------
        env: simpy.Environment
            the simulation environment.
        nports: int
            the total number of ports on this switch.
        port_rate: float
            the bit rate of the port.
        buffer_size: int
            the size of an outgoing port' bounded buffer, in packets.
        element_id: str
            The (optional) element ID of this component.
        debug: bool
            If True, prints more verbose debug information.
    """
    def __init__(self,
                 env,
                 nports: int,
                 port_rate: float,
                 buffer_size: int,
                 element_id: str = "",
                 debug: bool = False) -> None:
        self.env = env
        self.ports = []
        for port in range(nports):
            self.ports.append(
                Port(env,
                     rate=port_rate,
                     qlimit=buffer_size,
                     limit_bytes=False,
                     element_id=f"{element_id}_{port}",
                     debug=debug))
        self.demux = FIBDemux(fib=None, outs=self.ports, default=None)

    def put(self, packet):
        """ Sends a packet to this element. """
        self.demux.put(packet)


class FairPacketSwitch:
    """ Implements a fair packet switch with a choice of a WFQ, DRR, or Virtual Clock
        scheduler, as well as bounded buffers, on each of the outgoing ports.

        Parameters
        ----------
        env: simpy.Environment
            the simulation environment.
        nports: int
            the total number of ports on this switch.
        port_rate: float
            the bit rate of each outgoing port.
        buffer_size: int
            the size of an outgoing port' bounded buffer, in packets.
        weights: list or dict
            This can be either a list or a dictionary. If it is a list, it uses the flow_id ---
            or class_id, if class-based fair queueing is activated using the `flow_classes' parameter
            below --- as its index to look for the flow (or class)'s corresponding weight (or
            priority for Static Priority scheduling). If it is a dictionary, it contains
            (flow_id or class_id -> weight) pairs for each possible flow_id or class_id.
        flow_classes: function
            This is a function that matches flow_id's to class_ids, used to implement a class-based
            scheduling discipline. The default is an identity lambda function, which is equivalent
            to flow-based scheduling.
        server: str (possible values: 'WFQ', 'DRR', 'SP', or 'VirtualClock')
            The type of the scheduling discipline used for each outgoing port.
        element_id: str
            The (optional) element ID of this component.
        debug: bool
            If True, prints more verbose debug information.
    """
    def __init__(self,
                 env,
                 nports: int,
                 port_rate: float,
                 buffer_size: int,
                 weights,
                 server: str,
                 flow_classes: Callable = lambda x: x,
                 element_id: int = -1,
                 debug: bool = False,
                 layer: str = "aggregation",
                 ab_fgroup=None,
                 culprit_switches=None,
                 culprit_flows=None,
                 sketches=None,
                 sketch1=None,
                 sketch2=None,
                 sketch3=None,
                 loop_pair=None,
                 culprit_typ=None,
                 n_flows=None,
                 algo=None,
                 culprit_time=None) -> None:
        self.env = env
        self.ports = []
        self.egress_ports = []
        self.element_id = element_id
        self.cnt = 0
        self.layer = layer
        self.ab_fgroup = ab_fgroup
        self.culprit_switches = culprit_switches
        self.culprit_flows = culprit_flows
        self.sketch1 = sketch1
        self.sketch2 = sketch2
        self.sketch3 = sketch3
        self.loop_pair = loop_pair
        self.culprit_typ=culprit_typ
        # for port in range(nports):
        for port in range(2*nports):
            # prate=0
            prate=port_rate
            if port>=nports:
                prate=1000
            egress_port = Port(env,
                               rate=prate,
                            #    rate=0,
                               qlimit=buffer_size,
                               limit_bytes=False,
                               zero_downstream_buffer=True,
                               element_id=f"{element_id}_{port}",
                               port_id=port,
                               nod_id=element_id,
                            #    debug=1,
                               debug=debug,
                               layer=layer,
                               sketches=sketches,
                               sketch1=self.sketch1,
                               sketch2=self.sketch2,
                               sketch3=self.sketch3,
                               ab_fgroup=self.ab_fgroup,
                               n_flows=n_flows,
                               algo=algo,
                               culprit_time=culprit_time)
                               
            
            scheduler = None
            if server == 'WFQ':
                scheduler = WFQServer(env,
                                      rate=prate,
                                    #   rate=port_rate,
                                      weights=weights,
                                      flow_classes=flow_classes,
                                      zero_buffer=True,
                                      debug=debug,
                                      nod_id=element_id,
                                      layer=layer,
                                      sketches=sketches,
                                      sketch1=self.sketch1,
                                      sketch2=self.sketch2,
                                      sketch3=self.sketch3,
                                      ab_fgroup=self.ab_fgroup,
                                      algo=algo)
            elif server == 'DRR':
                scheduler = DRRServer(env,
                                      rate=prate,
                                    #   rate=port_rate,
                                      weights=weights,
                                      flow_classes=flow_classes,
                                      zero_buffer=True,
                                      nod_id=element_id,
                                      debug=debug)
            elif server == 'VirtualClock':
                scheduler = VirtualClockServer(env,
                                               rate=port_rate,
                                               vticks=weights,
                                               flow_classes=flow_classes,
                                               zero_buffer=True,
                                               debug=debug)
            elif server == 'SP':
                scheduler = SPServer(env,
                                     rate=port_rate,
                                     priorities=weights,
                                     flow_classes=flow_classes,
                                     zero_buffer=True,
                                     debug=debug)
            else:
                raise ValueError(
                    "Scheduler type must be 'WFQ', 'DRR', 'SP', or 'VirtualClock'."
                )

            egress_port.out = scheduler

            self.egress_ports.append(egress_port)
            self.ports.append(scheduler)

        self.demux = FIBDemux(env=env,fib=None, outs=self.egress_ports, default=None,nod_id=element_id,layer=layer,
                               sketches=sketches,
                               sketch1=self.sketch1,
                               sketch2=self.sketch2,
                               sketch3=self.sketch3,
                               culprit_switches=culprit_switches,
                               culprit_flows=culprit_flows,
                               loop_pair=loop_pair,
                               k=nports,
                               culprit_typ=culprit_typ,
                               culprit_time=culprit_time)
    
    def subnet(self,flow_id):
        return flow_id>>8

    def put(self, packet):
        """ Sends a packet to this element. """
        self.demux.put(packet)     

        
            


# class CheckSwitch:
#     def __init__(self,
#                  env,
#                  ft,
#                  ab_fgroup,
#                  sketches,
#                  all_flows,
#                  duration=10) -> None:
#         self.env = env
#         self.ft = ft
#         self.duration = duration
#         self.ab_fgroup = ab_fgroup
#         self.sketches = sketches
#         self.all_flows = all_flows
#         self.action = env.process(self.run())

#     def convert(self,x):
#         res = x.srcIP_dstIP()
#         res = (res<<16)+x.srcPort()
#         res = (res<<16)+x.dstPort()
#         res = (res<<8)+x.proto()
#         return res

#     def converts(self,xs):
#         ress = []
#         for x in xs:
#             res = self.convert(x)
#             ress.append(res)
#         return ress

#     def run(self):
#         while True:
#             yield self.env.timeout(self.duration)
#             abnormal_flows = self.converts(self.sketches.ab_fs)
#             # for flow_id in range(0,100):
#             #     flow = self.all_flows[flow_id]
#             #     if not self.convert(flow.fid) in abnormal_flows:
#             #         continue

#             #     length = len(flow.path)
#             #     # last_freq = self.ft.nodes[flow.path[length-3]]['device'].sketch1.Query(flow.fid)
#             #     first_freq = self.ft.nodes[flow.path[2]]['device'].sketch1.query(flow.fid)
#             #     print("flow",flow.fid,"path",flow.path)
#             #     print("first",flow.path[2],"freq",first_freq)
#             #     # print("last",flow.path[length-3],"freq",last_freq)
#             #     for i in range(2,length-2):
#             #         freq = self.ft.nodes[flow.path[i]]['device'].sketch1.query(flow.fid)
#             #         print(flow.path[i],freq)
#             #         if first_freq-freq > 2:
#             #             print("flow",flow_id,"ab-switch",flow.path[i])
#             #             break

#             ab_fg_list = self.sketches.ab_fg()
#             # self.ab_fgroup = set(ab_fg_list)
#             # print("lala",self.ab_fgroup)
#             # self.sketches.Clear()

            
#             # for node_id in self.ft.nodes():
#                 # node = self.ft.nodes[node_id]
#                 # if node['layer'] == 'edge':
#                 #     node['device'].cnt+=1
#                 # node['device'].sketch.clear()
                    