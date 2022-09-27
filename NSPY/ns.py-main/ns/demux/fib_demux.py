class FIBDemux:
    """
    The constructor takes a list of downstream elements for the
    corresponding output ports as its input.

    Parameters
    ----------
    fib: dict
        forwarding information base. Key: flow id, Value: output port
    outs: list
        list of downstream elements corresponding to the output ports
    ends: list
        list of downstream elements corresponding to the output ports
    default:
        default downstream element
    """
    def __init__(self,
                 env,
                 fib: dict = None,
                 outs: list = None,
                 ends: dict = None,
                 default=None,
                 nod_id=-1,
                 sketches = None,
                 sketch1 = None,
                 sketch2 = None,
                 sketch3 = None,
                 culprit_switches = None,
                 culprit_flows = None,
                 layer="aggregation",
                 loop_pair=None,
                 k=0,
                 culprit_typ=None,
                 culprit_time=None) -> None:
        self.env=env
        self.outs = outs
        self.default = default
        self.packets_received = 0
        self.fib = fib
        self.nexthop_to_port = None
        self.nod_id = nod_id
        self.loop_num=dict()
        self.layer = layer
        self.ab_fgroup = []
        self.sketches = sketches
        self.sketch1 = sketch1
        self.sketch2 = sketch2
        self.sketch3 = sketch3
        self.culprit_switches = culprit_switches
        self.culprit_flows = culprit_flows
        self.loop_pair = loop_pair
        self.k = k
        self.culprit_typ=culprit_typ
        self.culprit_time=culprit_time
        self.cul_key=list(culprit_time.keys())
        self.flag = 0
        self.cul_num = 0
        self.cul_sum = len(culprit_time)
        self.start_ = self.cul_key[self.cul_num]
        self.end_ = self.culprit_time[self.start_]
        if ends:
            self.ends = ends
        else:
            self.ends = dict()

    def subnet(self,flow_id):
        return flow_id>>8

    def put(self, packet):
        """ Sends a packet to this element. """
        self.packets_received += 1
        flow_id = packet.flow_id

        if flow_id in self.ends:
            self.ends[flow_id].put(packet)
        else:
            try:
                if self.culprit_typ==0 or self.culprit_typ==1:
                    # mark=0
                    # for key in self.culprit_time.keys():
                    #     if not (self.env.now>=key and self.env.now<self.culprit_time[key]):
                    #         continue
                    #     mark=1
                    #     break

                    self.flag = 0
                    if (self.env.now>=self.start_ and self.env.now<self.end_):
                        self.flag = 1
                    elif self.env.now >= self.end_:
                        if self.cul_num+1<self.cul_sum:
                            self.cul_num+=1
                            self.start_ = self.cul_key[self.cul_num]
                            self.end_ = self.culprit_time[self.start_]
                    
                    # if not 1:
                    if not self.flag:
                    # if not mark:
                        self.outs[self.fib[packet.flow_id]].put(packet)
                    else:
                        ######################################################black####################################################################
                        if self.culprit_typ==0:
                            # if not ((self.nod_id in self.culprit_switches) and (packet.flow_id>>80)==self.culprit_flows[self.nod_id] and ((packet.flow_id>>72)&1)):    
                            if not (((packet.flow_id>>80) in self.culprit_flows) and (self.nod_id in self.culprit_switches[packet.flow_id>>80]) and ((packet.flow_id>>72)&1)):    
                                self.outs[self.fib[packet.flow_id]].put(packet)
                        ######################################################black####################################################################
                        ######################################################loop####################################################################
                        elif self.culprit_typ==1:
                            # if not ((self.nod_id in self.culprit_switches) and (packet.flow_id>>80)==self.culprit_flows[self.nod_id] and ((packet.flow_id>>72)&1)):    
                            if not (((packet.flow_id>>80) in self.culprit_flows) and (self.nod_id in self.culprit_switches[packet.flow_id>>80]) and ((packet.flow_id>>72)&1)):    
                                self.outs[self.fib[packet.flow_id]].put(packet)
                            else:
                                # print(self.nod_id,"to",self.loop_pair[self.nod_id])
                                if packet.flow_id not in self.loop_num.keys():
                                    self.loop_num[packet.flow_id]=1
                                    self.outs[self.nexthop_to_port[self.loop_pair[self.nod_id]]].put(packet)

                                elif self.loop_num[packet.flow_id]<10:
                                    self.loop_num[packet.flow_id]+=1
                                    self.outs[self.nexthop_to_port[self.loop_pair[self.nod_id]]].put(packet)
                                else:
                                    self.loop_num[packet.flow_id]=0
                        ######################################################loop####################################################################
                    ######################################################jitter####################################################################
                elif self.culprit_typ==2 or self.culprit_typ==3:
                        # if not ((self.nod_id in self.culprit_switches) and (packet.flow_id>>80)==self.culprit_flows[self.nod_id] and ((packet.flow_id>>72)&1)):    
                    if not (((packet.flow_id>>80) in self.culprit_flows) and (self.nod_id in self.culprit_switches[packet.flow_id>>80]) and ((packet.flow_id>>72)&1)):    

                        self.outs[self.fib[packet.flow_id]].put(packet)
                    else:
                        self.outs[self.fib[packet.flow_id]+self.k].put(packet)

                    ######################################################jitter####################################################################
                    
            except (KeyError, IndexError, ValueError) as exc:
                print("FIB Demux Error: " + str(exc))
                if self.default:
                    self.default.put(packet)
