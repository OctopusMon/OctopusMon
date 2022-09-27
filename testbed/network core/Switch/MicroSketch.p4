/*  base : 
    core switch measurement logic
    */
#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

//*p4 ----- 16*//
//register num 
#define MAX_BUCKET_REGISTER_ENTRIES 4096
#define MAX_BUCKET_TAG_ENTRIES 1024
#define MAX_CELL_BUCKET_ENTRIES 4096
#define INDEX_HIGHEST 11 //index_length - 1
#define CELL_INDEX_HIGHEST 11 //cell_index_length - 1
//used for test
#define BASE_NUM 1
#define MAX_RECORD_NUM 2000
#define MAX_TIME_INTERVAL 262143 //2^18 max value of dequeue_timedelta
#define MIN_TIME_INTERVAL 0
//Threshold for splitting bucket into cell
#define THRESHOLD 0

const bit<16> TYPE_IPV4 = 0x0800;
const bit<16> TYPE_ARP = 0x0806;
const bit<8> TYPE_TCP = 0x06;//tcp = 6
const bit<8> TYPE_UDP = 0x11;//udp = 17

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;	
typedef bit<32> ip4Addr_t;	
typedef bit<12> index;
typedef bit<10> index_hash_part;
typedef bit<12> cell_index;
typedef bit<10> cell_hash_part;

//ethernet header
header ethernet_t {  
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;	
}

//ipv4 header
header ipv4_t {   
    bit<4>    version; 
    bit<4>    ihl;      
    bit<8>    diffserv; 
    bit<16>   totalLen;
    bit<16>   identification; 
    bit<3>    flags;   
    bit<13>   fragOffset;  
    bit<8>    ttl;   
    bit<8>    protocol;  
    bit<16>   hdrChecksum; 
    ip4Addr_t srcAddr; 
    ip4Addr_t dstAddr;	
}

//arp header
header arp_h {
    bit<16> ar_hrd;
    bit<16> ar_pro;
    bit<8>  ar_hln;
    bit<8>  ar_pln;
    bit<16> ar_op;
    bit<48> src_mac;
    bit<32> src_ip;
    bit<48> dst_mac;
    bit<32> dst_ip;
}

//tcp header
header tcp_t{  
    bit<16> srcPort;  
    bit<16> dstPort;  
    bit<32> seqNo;    
    bit<32> ackNo;    
    bit<4>  dataOffset; 
    bit<4>  res;  
    bit<1>  cwr;  
    bit<1>  ece;  
    bit<1>  urg;  
    bit<1>  ack; 
    bit<1>  psh;  
    bit<1>  rst;  
    bit<1>  syn;  
    bit<1>  fin;  
    bit<16> window; 
    bit<16> checksum;  
    bit<16> urgentPtr; 
}

//udp header
header udp_t{
    bit<16> srcPort; 	
    bit<16> dstPort;	
    bit<16> length_;	
    bit<16> checksum;
}

//empty header
struct empty_header_t {}

struct headers {            
    ethernet_t  ethernet;
    ipv4_t      ipv4;
    arp_h       arp;
    tcp_t       tcp;
    udp_t       udp;
}

struct metadata {
    //bucket register index
    index bucket1_index;
    index bucket2_index;
    //bucket register index's offset part
    bit<2> offset1;
    bit<2> offset2;
    //bit<16> record_index;   //used for test
    bit<1> bucket1_compare_dst;
    bit<1> bucket1_compare_src;
    bit<1> bucket2_compare_dst;
    bit<1> bucket2_compare_src;
    bit<1> divide_flag1;
    bit<1> divide_flag2;
    bit<2> level2_offset1;
    cell_index cell_index_part1;
    cell_index cell_index_part2;
}

//empty MyIngressParser
parser MyIngressParser(
    packet_in packet,
    out headers hdr,
    out metadata meta,
    out ingress_intrinsic_metadata_t ig_intr_md
                ) {
    //parse 
    //extract information of packet
    state start {
        packet.extract(ig_intr_md);
        //port metadata related
        packet.advance(PORT_METADATA_SIZE);
        transition accept;
    }
}

//empty MyIngress
control MyIngress(
    inout headers hdr,
    inout metadata meta,
    in ingress_intrinsic_metadata_t ig_intr_md,
    in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md
    ) {
    //forward to egress port
    Register<bit<16>,bit<32>>(MAX_RECORD_NUM,0) ingress_pkt_counter;
	//bucket1: record
	RegisterAction<bit<16>,bit<32>,bit<16>>(ingress_pkt_counter) ingress_pkt_counter_read = {
        void apply(inout bit<16> input_value,out bit<16> output_value){
            output_value = input_value;
            input_value = input_value + 1;
        }
    };
    apply {
        //forward to appropriate egress device_port
        ig_intr_tm_md.ucast_egress_port = 9w384;
        //ingress_pkt_counter_read.execute(32w0); //used for test
        //don't bypass egress
        ig_intr_tm_md.bypass_egress = 1w0;
    }
}

//empty MyIngress deparser
control MyIngressDeparser(
        packet_out pkt,
        inout headers hdr,
        in metadata meta,
        in ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md) {

    apply {
        //didn't modify header,only emit totally
        pkt.emit(hdr);
    }
}

parser MyEgressParser(
        packet_in packet,
        out headers hdr,
        out metadata meta,
        out egress_intrinsic_metadata_t eg_intr_md) {
    //mainly logic is in egress 
    //extract egress standard metadata information of packet
    state start {
        packet.extract(eg_intr_md);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet); 
        transition select (hdr.ethernet.etherType) { 
            TYPE_IPV4: parse_ipv4;  
            TYPE_ARP: parse_arp;  
            default: accept;   
        }
    }
    
   state parse_ipv4 {
        packet.extract(hdr.ipv4);   
        transition select (hdr.ipv4.protocol) { 
            TYPE_TCP: parse_tcp;   
            TYPE_UDP: parse_udp;
            default: accept; 
		}
    }

    state parse_arp {
        packet.extract(hdr.arp);   
    }
    state parse_tcp {    
        packet.extract(hdr.tcp);  
        transition accept;
    }
    state parse_udp {
        packet.extract(hdr.udp);
        transition accept;
    }
}

control MyEgressDeparser(
        packet_out pkt,
        inout headers hdr,
        in metadata eg_md,
        in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
    apply {
        pkt.emit(hdr);
    }
}

control MyEgress(
        inout headers hdr,
        inout metadata meta,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    /*logic: 
    packet in -> hash flow key and get bucket1 index -> judge whether bucket1 is empty or not 
        empty -> fill bucket1 tag register and update bucket1 value     
        not empty -> judge whether bucket1's recorded flow keys match current packet's flow key or not
            match -> update bucket1 value and judge whether bucket1 value exceed threshold
                exceed -> hash bucket1 index and flow key to get cell part1/part2 index + record hash mapping relation + update cell register
            not match -> hash bucket2 index -> judge judge whether bucket2 is empty or not and repeat the previous logic
    */
    /* *******
     dleft alogirthm table/action and regsiter 
    ****** */
    Hash<index_hash_part>(HashAlgorithm_t.CRC16) hash_bucket1;
    Hash<index_hash_part>(HashAlgorithm_t.CRC32) hash_bucket2;
    //bucket index hash function
    //bucket1 index table
    action get_bucket1_index_hash_part() {
        meta.bucket1_index[INDEX_HIGHEST:2] = hash_bucket1.get(
			{hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol});
    }
    table bucket1_index_hash_part {
        actions  = {get_bucket1_index_hash_part;}
        const default_action = get_bucket1_index_hash_part();
        size = 1;
    }
    //bucket2 index table
    action get_bucket2_index_hash_part() {
        meta.bucket2_index[INDEX_HIGHEST:2] = hash_bucket2.get(
			{hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol});
    }
    table bucket2_index_hash_part {
        actions  = {get_bucket2_index_hash_part;}
        const default_action = get_bucket2_index_hash_part();
        size = 1;
    }
    //bucket 1 part 
    Register<bit<32>,bit<32>>(MAX_BUCKET_REGISTER_ENTRIES,0) bucket1;
	//bucket1: record
	RegisterAction<bit<32>,bit<32>,bit<1>>(bucket1) bucket1_read = {
        void apply(inout bit<32> input_value,out bit<1> output_value){
            input_value = input_value + 1;
            if(input_value >= THRESHOLD)
            {
                output_value = 1w1;
            }
            else
            {
                output_value = 1w0;
            }
        }
    };
    Register<bit<32>,bit<32>>(MAX_BUCKET_TAG_ENTRIES,0) bucket1_srciptag;
	//bucket1: record
	RegisterAction<bit<32>,bit<32>,bit<1>>(bucket1_srciptag) bucket1_srciptag_read = {
        void apply(inout bit<32> input_value,out bit<1> output_value){
            if(input_value == 32w0 || input_value == hdr.ipv4.srcAddr)
            {
                input_value = hdr.ipv4.srcAddr;
                output_value = 0;
            }
            else
            {
                output_value = 1;
            }
        }
    };
    Register<bit<32>,bit<32>>(MAX_BUCKET_TAG_ENTRIES,0) bucket1_dstiptag;
	//bucket1: record
	RegisterAction<bit<32>,bit<32>,bit<1>>(bucket1_dstiptag) bucket1_dstiptag_read = {
        void apply(inout bit<32> input_value,out bit<1> output_value){
            if(input_value == 32w0 || input_value == hdr.ipv4.dstAddr)
            {
                input_value = hdr.ipv4.dstAddr;
                output_value = 0;
            }
            else
            {
                output_value = 1;
            }
        }
    };
    //bucket 2 part
    Register<bit<32>,bit<32>>(MAX_BUCKET_REGISTER_ENTRIES,0) bucket2;
	//bucket1: record
	RegisterAction<bit<32>,bit<32>,bit<1>>(bucket2) bucket2_read = {
        void apply(inout bit<32> input_value,out bit<1> output_value){
            input_value = input_value + 1;
            if(input_value >= THRESHOLD)
            {
                output_value = 1w1;
            }
            else
            {
                output_value = 1w0;
            }
        }
    };
    Register<bit<32>,bit<32>>(MAX_BUCKET_TAG_ENTRIES,0) bucket2_srciptag;
	//bucket1: record
	RegisterAction<bit<32>,bit<32>,bit<1>>(bucket2_srciptag) bucket2_srciptag_read = {
        void apply(inout bit<32> input_value,out bit<1> output_value){
            if(input_value == 32w0 || input_value == hdr.ipv4.srcAddr)
            {
                input_value = hdr.ipv4.srcAddr;
                output_value = 0;
            }
            else
            {
                output_value = 1;
            }
        }
    };
    Register<bit<32>,bit<32>>(MAX_BUCKET_TAG_ENTRIES,0) bucket2_dstiptag;
	//bucket1: record
	RegisterAction<bit<32>,bit<32>,bit<1>>(bucket2_dstiptag) bucket2_dstiptag_read = {
        void apply(inout bit<32> input_value,out bit<1> output_value){
            if(input_value == 32w0 || input_value == hdr.ipv4.dstAddr)
            {
                input_value = hdr.ipv4.dstAddr;
                output_value = 0;
            }
            else
            {
                output_value = 1;
            }
        }
    };
    //used for test  TODO : delete
    Register<bit<16>,bit<32>>(BASE_NUM,0) pkt_counter;
	RegisterAction<bit<16>,bit<32>,bit<16>>(pkt_counter) pkt_counter_read = {
        void apply(inout bit<16> input_value,out bit<16> output_value){
            input_value = input_value + 1;
            output_value = 1;
        }
    };
    Register<bit<32>,bit<32>>(MAX_RECORD_NUM,0) dequeue_record;
	RegisterAction<bit<32>,bit<32>,bit<32>>(dequeue_record) dequeue_record_read = {
        void apply(inout bit<32> input_value,out bit<32> output_value){
            input_value = (bit<32>)eg_intr_md.deq_timedelta;
            output_value = 1;
        }
    };
    //delay offset table
    action set_offset(bit<2> offset_1,bit<2> offset_2)
    {
        meta.offset1 = offset_1;
        meta.offset2 = offset_2;
    }
    table range_match_level1 {
        key = {
            eg_intr_md.deq_timedelta : range;
        }
        actions = {
            set_offset;
        }
        const default_action = set_offset(3,3);
        size = 100;
    }
    //complete index
    action get_bucket1_index_offset_part()
    {
        meta.bucket1_index[1:0] = meta.offset1;
    }
    table complete_bucket1_index{
        actions = {get_bucket1_index_offset_part;}
        const default_action = get_bucket1_index_offset_part();
        size = 1;
    }
    action get_bucket2_index_offset_part()
    {
        meta.bucket2_index[1:0] = meta.offset2;
    }
    table complete_bucket2_index{
        actions = {get_bucket2_index_offset_part;}
        const default_action = get_bucket2_index_offset_part();
        size = 1;
    }
    //tag register 
    //bucket1 
    action read_and_compare_bucket1_dstip_tag() {
        meta.bucket1_compare_dst = bucket1_dstiptag_read.execute((bit<32>)meta.bucket1_index[INDEX_HIGHEST:2]);
    }
    table update_bucket1_dstip_tag_register {
        actions = {read_and_compare_bucket1_dstip_tag;}
        const default_action = read_and_compare_bucket1_dstip_tag();
        size = 1;
    }
    action read_and_compare_bucket1_srcip_tag() {
        meta.bucket1_compare_src = bucket1_srciptag_read.execute((bit<32>)meta.bucket1_index[INDEX_HIGHEST:2]);
    }
    table update_bucket1_srcip_tag_register {
        actions = {read_and_compare_bucket1_srcip_tag;}
        const default_action = read_and_compare_bucket1_srcip_tag();
        size = 1;
    }
    //bukcet2
    action read_and_compare_bucket2_dstip_tag() {
        meta.bucket2_compare_dst = bucket2_dstiptag_read.execute((bit<32>)meta.bucket2_index[INDEX_HIGHEST:2]);
    }
    table update_bucket2_dstip_tag_register {
        actions = {read_and_compare_bucket2_dstip_tag;}
        const default_action = read_and_compare_bucket2_dstip_tag();
        size = 1;
    }
    action read_and_compare_bucket2_srcip_tag() {
        meta.bucket2_compare_src = bucket2_srciptag_read.execute((bit<32>)meta.bucket2_index[INDEX_HIGHEST:2]);
    }
    table update_bucket2_srcip_tag_register {
        actions = {read_and_compare_bucket2_srcip_tag;}
        const default_action = read_and_compare_bucket2_srcip_tag();
        size = 1;
    }
    //bucket register update
    //bucket1 
    action write_register1() {
       meta.divide_flag1 =  bucket1_read.execute((bit<32>)meta.bucket1_index);
    }
    table update_bucket1{
        actions = {write_register1;}
        const default_action = write_register1();
        size = 1;
    }
    //bucket2
    action write_register2() {
        meta.divide_flag2 = bucket2_read.execute((bit<32>)meta.bucket2_index);
    }
    table update_bucket2{
        actions = {write_register2;}
        const default_action = write_register2();
        size = 1;
    }
    /* *******
     cell division alogrithm table/action and register
    *******/
    //cell hash function
    Hash<cell_hash_part>(HashAlgorithm_t.CRC16) b1_hash_cell1;
    Hash<cell_hash_part>(HashAlgorithm_t.CRC32) b1_hash_cell2;
    Hash<cell_hash_part>(HashAlgorithm_t.CRC16) b2_hash_cell1;
    Hash<cell_hash_part>(HashAlgorithm_t.CRC32) b2_hash_cell2;
    //Hash part : dleft in bucket1 , hash unit : bucket1_index
    action b1_hash_cell_index_part1() {
        meta.cell_index_part1[CELL_INDEX_HIGHEST:2] = b1_hash_cell1.get({
            hdr.ipv4.srcAddr,hdr.ipv4.dstAddr,meta.bucket1_index
        });
    }
    table b1_get_cell_hash_part1 {
        actions = {b1_hash_cell_index_part1;}
        const default_action = b1_hash_cell_index_part1();
        size = 10;
    }
    action b1_hash_cell_index_part2() {
        meta.cell_index_part2[CELL_INDEX_HIGHEST:2] = b1_hash_cell2.get({
            hdr.ipv4.srcAddr,hdr.ipv4.dstAddr,meta.bucket1_index
        });
    }
    table b1_get_cell_hash_part2 {
        actions = {b1_hash_cell_index_part2;}
        const default_action = b1_hash_cell_index_part2();
        size = 10;
    }
    //Hash part : dleft in bucket2 , hash unit : bucket2_index
    action b2_hash_cell_index_part1() {
        meta.cell_index_part1[CELL_INDEX_HIGHEST:2] = b2_hash_cell1.get({
            hdr.ipv4.srcAddr,hdr.ipv4.dstAddr,meta.bucket2_index
        });
    }
    table b2_get_cell_hash_part1 {
        actions = {b2_hash_cell_index_part1;}
        const default_action = b2_hash_cell_index_part1();
        size = 10;
    }
    action b2_hash_cell_index_part2() {
        meta.cell_index_part2[CELL_INDEX_HIGHEST:2] = b2_hash_cell2.get({
            hdr.ipv4.srcAddr,hdr.ipv4.dstAddr,meta.bucket2_index
        });
    }
    table b2_get_cell_hash_part2 {
        actions = {b2_hash_cell_index_part2;}
        const default_action = b2_hash_cell_index_part2();
        size = 10;
    }
    //complete cell index offset part
    //dleft in bucket1 
    action b1_offset_cell_index_part1() {
        meta.cell_index_part1[1:0] = meta.level2_offset1;
    }
    table b1_complete_cell_part1 {
        actions = {b1_offset_cell_index_part1;}
        const default_action = b1_offset_cell_index_part1();
        size = 10;
    }
    action b1_offset_cell_index_part2() {
        meta.cell_index_part2[1:0] = meta.level2_offset1;
    }
    table b1_complete_cell_part2 {
        actions = {b1_offset_cell_index_part2;}
        const default_action = b1_offset_cell_index_part2();
        size = 10;
    }
    //dleft in bucket2
    action b2_offset_cell_index_part1() {
        meta.cell_index_part1[1:0] = meta.level2_offset1;
    }
    table b2_complete_cell_part1 {
        actions = {b2_offset_cell_index_part1;}
        const default_action = b2_offset_cell_index_part1();
        size = 10;
    }
    action b2_offset_cell_index_part2() {
        meta.cell_index_part2[1:0] = meta.level2_offset1;
    }
    table b2_complete_cell_part2 {
        actions = {b2_offset_cell_index_part2;}
        const default_action = b2_offset_cell_index_part2();
        size = 10;
    }
    //cell register
    Register<bit<32>,bit<32>>(MAX_CELL_BUCKET_ENTRIES,0) cell_bucket1_part1;
	RegisterAction<bit<32>,bit<32>,bit<32>>(cell_bucket1_part1) cell_bucket1_part1_read = {
        void apply(inout bit<32> input_value,out bit<32> output_value){
            input_value = input_value + 1;
            output_value = 1;
        }
    };
    Register<bit<32>,bit<32>>(MAX_CELL_BUCKET_ENTRIES,0) cell_bucket1_part2;
	RegisterAction<bit<32>,bit<32>,bit<32>>(cell_bucket1_part2) cell_bucket1_part2_read = {
        void apply(inout bit<32> input_value,out bit<32> output_value){
            input_value = input_value + 1;
            output_value = 1;
        }
    };
    //import register : record func : bucket_index(with offset) -> cell_index(without offset)
    Register<bit<16>,bit<32>>(MAX_BUCKET_REGISTER_ENTRIES,0) bucket1_to_cell_part1;
	RegisterAction<bit<16>,bit<32>,bit<16>>(bucket1_to_cell_part1) bucket1_to_cell_part1_read = {
        void apply(inout bit<16> input_value,out bit<16> output_value){
            input_value = (bit<16>)meta.cell_index_part1[CELL_INDEX_HIGHEST:2];
        }
    };
    Register<bit<16>,bit<32>>(MAX_BUCKET_REGISTER_ENTRIES,0) bucket1_to_cell_part2;
	RegisterAction<bit<16>,bit<32>,bit<16>>(bucket1_to_cell_part2) bucket1_to_cell_part2_read = {
        void apply(inout bit<16> input_value,out bit<16> output_value){
            input_value = (bit<16>)meta.cell_index_part2[CELL_INDEX_HIGHEST:2];
        }
    };
    Register<bit<16>,bit<32>>(MAX_BUCKET_REGISTER_ENTRIES,0) bucket2_to_cell_part1;
	RegisterAction<bit<16>,bit<32>,bit<16>>(bucket2_to_cell_part1) bucket2_to_cell_part1_read = {
        void apply(inout bit<16> input_value,out bit<16> output_value){
            input_value = (bit<16>)meta.cell_index_part1[CELL_INDEX_HIGHEST:2];
        }
    };
    Register<bit<16>,bit<32>>(MAX_BUCKET_REGISTER_ENTRIES,0) bucket2_to_cell_part2;
	RegisterAction<bit<16>,bit<32>,bit<16>>(bucket2_to_cell_part2) bucket2_to_cell_part2_read = {
        void apply(inout bit<16> input_value,out bit<16> output_value){
            input_value = (bit<16>)meta.cell_index_part2[CELL_INDEX_HIGHEST:2];
        }
    };
    //cell division write register tables
    //dleft in bucket1 
    action b1_write_cell_part1() {
        cell_bucket1_part1_read.execute((bit<32>)meta.cell_index_part1);
    }
    table b1_update_cell_part1 {
        actions  = {b1_write_cell_part1;}
        const default_action = b1_write_cell_part1();
        size = 10;
    }
    action b1_write_cell_part2() {
        cell_bucket1_part2_read.execute((bit<32>)meta.cell_index_part2);
    }
    table b1_update_cell_part2 {
        actions  = {b1_write_cell_part2;}
        const default_action = b1_write_cell_part2();
        size = 10;
    }
    //dleft in bucket2
    action b2_write_cell_part1() {
        cell_bucket1_part1_read.execute((bit<32>)meta.cell_index_part1);
    }
    table b2_update_cell_part1 {
        actions  = {b2_write_cell_part1;}
        const default_action = b2_write_cell_part1();
        size = 10;
    }
    action b2_write_cell_part2() {
        cell_bucket1_part2_read.execute((bit<32>)meta.cell_index_part2);
    }
    table b2_update_cell_part2 {
        actions  = {b2_write_cell_part2;}
        const default_action = b2_write_cell_part2();
        size = 10;
    }
    //need record func 
    //bucket1_index -> cell_index
    action write_b1_to_cell_part1() {
        bucket1_to_cell_part1_read.execute((bit<32>)meta.bucket1_index);
    }
    table record_b1_to_cell_index_part1 {
        actions = {write_b1_to_cell_part1;}
        const default_action = write_b1_to_cell_part1();
        size = 10;
    }
    action write_b1_to_cell_part2() {
        bucket1_to_cell_part2_read.execute((bit<32>)meta.bucket1_index);
    }
    table record_b1_to_cell_index_part2 {
        actions = {write_b1_to_cell_part2;}
        const default_action = write_b1_to_cell_part2();
        size = 10;
    }
    //bucket2_index -> cell_index
    action write_b2_to_cell_part1() {
        bucket2_to_cell_part1_read.execute((bit<32>)meta.bucket2_index);
    }
    table record_b2_to_cell_index_part1 {
        actions = {write_b2_to_cell_part1;}
        const default_action = write_b2_to_cell_part1();
        size = 10;
    }
    action write_b2_to_cell_part2() {
        bucket2_to_cell_part2_read.execute((bit<32>)meta.bucket2_index);
    }
    table record_b2_to_cell_index_part2 {
        actions = {write_b2_to_cell_part2;}
        const default_action = write_b2_to_cell_part2();
        size = 10;
    }
    /* after dleft , also need a cell division alogrithm
    dleft : deq_timedelta ->               [] [] [] []
                                           /\
    cell division : deq_timedelta ->     [] [] [] []
    */
    action set_offset2(bit<2> value1)
    {
        meta.level2_offset1 = value1;
    }
    table range_match_level2 {
        key = {
            eg_intr_md.deq_timedelta : range;
        }
        actions = {
            set_offset2;
        }
        const default_action = set_offset2(3);
        size = 100;
    }
    apply {
        //has been marked by edge switches
        if(hdr.ipv4.flags[0:0] == 1w1)
        {
            //get offset
            range_match_level1.apply();
            range_match_level2.apply();
            pkt_counter_read.execute(32w0);
            //dequeue_record_read.execute((bit<32>)meta.record_index); //used for test
            //offset1_record_read.execute((bit<32>)meta.record_index); //used for test
            //change header
            /* 2 level dleft alogrithm part*/
            hdr.ethernet.srcAddr = (bit<48>)eg_intr_md.enq_tstamp;
            bucket1_index_hash_part.apply();
            complete_bucket1_index.apply();
            update_bucket1_dstip_tag_register.apply();
            update_bucket1_srcip_tag_register.apply();
            //if bucket is empty 
            if(meta.bucket1_compare_dst == 1w0 && meta.bucket1_compare_src == 1w0)
            {
                update_bucket1.apply();
                /* 1 level cell division part */
                //devide_flag decide whether do cell division or not
                if(meta.divide_flag1 == 1w1)
                {
                    b1_get_cell_hash_part1.apply();
                    b1_complete_cell_part1.apply();
                    b1_get_cell_hash_part2.apply();
                    b1_complete_cell_part2.apply();
                    record_b1_to_cell_index_part1.apply();
                    record_b1_to_cell_index_part2.apply();
                    b1_update_cell_part1.apply();
                    b1_update_cell_part2.apply();
                }
            }
            else
            {
                bucket2_index_hash_part.apply();
                complete_bucket2_index.apply();
                update_bucket2_dstip_tag_register.apply();
                update_bucket2_srcip_tag_register.apply();
                if(meta.bucket2_compare_dst == 1w0 && meta.bucket2_compare_src == 1w0)
                {
                    update_bucket2.apply();
                    /* 1 level cell division part */
                    //devide_flag decide whether do cell division or not
                    if(meta.divide_flag2 == 1w1)
                    {
                        b2_get_cell_hash_part1.apply();
                        b2_complete_cell_part1.apply();
                        b2_get_cell_hash_part2.apply();
                        b2_complete_cell_part2.apply();
                        record_b2_to_cell_index_part1.apply();
                        record_b2_to_cell_index_part2.apply();
                        b2_update_cell_part1.apply();
                        b2_update_cell_part2.apply();
                    }
                }
            }
        }
    }
}

Pipeline(MyIngressParser(),
         MyIngress(),
         MyIngressDeparser(),
         MyEgressParser(),
         MyEgress(),
         MyEgressDeparser()) pipe;

Switch(pipe) main;