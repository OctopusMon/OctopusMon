"""
Implements a port with an output buffer, given an output rate and a buffer size (in either bytes
or the number of packets). This implementation uses the simple tail-drop mechanism to drop packets.
"""
from random import randint
import simpy

class Port:
    """ Models an output port on a switch with a given rate and buffer size (in either bytes
        or the number of packets), using the simple tail-drop mechanism to drop packets.

        Parameters
        ----------
        env: simpy.Environment
            the simulation environment.
        rate: float
            the bit rate of the port (0 for unlimited).
        element_id: int
            the element id of this port.
        qlimit: integer (or None)
            a queue limit in bytes or packets (including the packet in service), beyond
            which all packets will be dropped.
        limit_bytes: bool
            if True, the queue limit will be based on bytes; if False, the queue limit
            will be based on packets.
        zero_downstream_buffer: bool
            if True, assume that the downstream element does not have any buffers,
            and backpressure is in effect so that all waiting packets queue up in this
            element's buffer.
        debug: bool
            If True, prints more verbose debug information.
    """
    def __init__(self,
                 env,
                 rate: float,
                 qlimit: int = None,
                 limit_bytes: bool = False,
                 zero_downstream_buffer: bool = False,
                 element_id: int = None,
                 nod_id: int = None,
                 port_id: int = None,
                 debug: bool = False,
                 layer: str = "aggregation",
                 sketches=None,
                 sketch1 = None,
                 sketch2 = None,
                 sketch3 = None,
                 ab_fgroup = None,
                 n_flows=None,
                 algo=None,
                 culprit_time=None):
        self.store = simpy.Store(env)
        self.rate = rate
        self.env = env
        self.out = None
        self.out1 = None
        self.packets_received = 0
        self.packets_dropped = 0
        self.qlimit = qlimit
        self.limit_bytes = limit_bytes
        self.byte_size = 0  # the current size of the queue in bytes
        self.element_id = element_id
        self.nod_id = nod_id
        self.layer = layer
        self.sketches = sketches
        self.sketch1 = sketch1
        self.sketch2 = sketch2
        self.sketch3 = sketch3
        self.ab_fgroup = ab_fgroup
        self.port_id = port_id
        self.cnt=0
        self.n_flows=n_flows
        self.last_arrive_time = dict()
        self.algo=algo
        self.culprit_time = culprit_time

        self.zero_downstream_buffer = zero_downstream_buffer
        if self.zero_downstream_buffer:
            self.downstream_store = simpy.Store(env)

        self.debug = debug
        self.busy = 0  # used to track if a packet is currently being sent
        self.busy_packet_size = 0

        self.action = env.process(self.run())

    def update(self, packet):
        """
        The packet has just been retrieved from this element's own buffer by a downstream
        node that has no buffers.
        """
        # There is nothing that needs to be done, just print a debug message
        # if self.debug:
        #     print(f"Retrieved Packet {packet.packet_id} from flow {packet.flow_id}.")


    def run(self):
        """The generator function used in simulations."""
        while True:
            if self.zero_downstream_buffer:
                packet = yield self.downstream_store.get()
            else:
                packet = yield self.store.get()

            self.busy = 1
            self.busy_packet_size = packet.size
            
            if self.rate > 0:
                # yield self.env.timeout(packet.size * 8.0 / self.rate)
                if self.rate<10001:
                    for key in self.culprit_time.keys():
                        if not (self.env.now>=key and self.env.now<self.culprit_time[key]):
                            continue
                        yield self.env.timeout(11000/100000)
                        # yield self.env.timeout(5500/self.n_flows)
                        break
                    # yield self.env.timeout(100/self.n_flows)
                    


            self.byte_size -= packet.size

            if self.zero_downstream_buffer:
                self.out.put(packet,
                             upstream_update=self.update,
                             upstream_store=self.store)
                
            else:
                self.out.put(packet)

            self.busy = 0
            self.busy_packet_size = 0

    def subnet(self,flow_id):
        return flow_id>>8

    def put(self, packet):
        """ Sends a packet to this element. """
        self.packets_received += 1
        byte_count = self.byte_size + packet.size

        if self.element_id is not None:
            packet.perhop_time[self.element_id] = self.env.now

        packet.arrive_time = self.env.now
        if packet.flow_id in self.last_arrive_time.keys():
            packet.arrive_interval = (self.env.now-self.last_arrive_time[packet.flow_id])
            self.last_arrive_time[packet.flow_id] = self.env.now
        else:
            packet.arrive_interval = 0
            self.last_arrive_time[packet.flow_id] = self.env.now


        if self.qlimit is None:
            self.byte_size = byte_count
            if self.zero_downstream_buffer:
                self.downstream_store.put(packet)

            return self.store.put(packet)

        if self.limit_bytes and byte_count >= self.qlimit:
            self.packets_dropped += 1
            if self.debug:
                print(
                    f"Packet dropped: flow id = {packet.flow_id} and packet id = {packet.packet_id}"
                )
        elif not self.limit_bytes and len(self.store.items) >= self.qlimit - 1:
            self.packets_dropped += 1
            if self.debug:
                print(
                    f"Packet dropped: flow id = {packet.flow_id}, packet id = {packet.packet_id}"
                )
        else:
            # If the packet has not been dropped, record the queue length at this port
            # if self.debug:
            #     print(
            #         f"Queue length at port: {len(self.store.items)} packets.")
            
            self.byte_size = byte_count

            if self.zero_downstream_buffer:
                self.downstream_store.put(packet)
########################################################################################################
            src_ip = packet.flow_id>>72
            dst_ip = packet.flow_id>>40 & 0xffffffff
            src_port = (packet.flow_id>>24) & 0xffff
            dst_port = (packet.flow_id>>8) & 0xffff
            _proto = packet.flow_id & 0xff
            
            if self.algo=="dleft":
                if self.layer == 'edge':
                    if self.out.out.layer != 'leaf':
                        self.sketch2.insert_pre(src_ip,dst_ip,src_port,dst_port,_proto,int((self.env.now-packet.time)*1000000)) #up
                    if self.out.out.layer == 'leaf':
                        if randint(1,100)==50:
                            self.sketch3.insert_pre(src_ip,dst_ip,src_port,dst_port,_proto,1000) #down
                        else:
                            self.sketch3.insert_pre(src_ip,dst_ip,src_port,dst_port,_proto,int((self.env.now-packet.time)*1000000)) #down

########################################################################################################
            return self.store.put(packet)
