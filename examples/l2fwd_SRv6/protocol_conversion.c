#include <stdio.h>string_num.h
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <net/ethernet.h>
#include <rte_pdump.h>
#include <rte_udp.h>
#include <rte_tcp.h>
#include <rte_hash.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/*timer*/

#include <rte_malloc.h>
#include <rte_timer.h>

#include "test_pdump.h"
#include ""
#include "my_hash.h"

static volatile bool force_quit;

/* MAC updating enabled by default */
// static int mac_updating = 1;
#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 50 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 1024 //128
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* ethernet addresses of ports */
//端口的以太网地址
static struct rte_ether_addr l2fwd_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
//启用端口的mask
static uint32_t l2fwd_enabled_port_mask = 0;

/* list of enabled ports */
//启用端口列表
static uint32_t l2fwd_dst_ports[RTE_MAX_ETHPORTS];

struct port_pair_params {
#define NUM_PORTS	2
	uint16_t port[NUM_PORTS];
} __rte_cache_aligned;

static struct port_pair_params port_pair_params_array[RTE_MAX_ETHPORTS / 2];
static struct port_pair_params *port_pair_params;
static uint16_t nb_port_pair_params;
pool_clone

static unsigned int l2fwd_rx_queue_per_lcore = 1;

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
struct lcore_queue_conf
{
    unsigned n_rx_port;
    unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

static const struct rte_eth_conf port_conf = {
    .rxmode = {
        .max_rx_pkt_len = 1600,
        .split_hdr_size = 0,
        .offloads = (DEV_RX_OFFLOAD_JUMBO_FRAME),
        //  .header_split   = 0, /**< Header Split disabled */  //报头分离
        //  .hw_ip_checksum = 0, /**< IP checksum offload disabled */ //IP校验和卸载
        //  .hw_vlan_filter = 0, /**< VLAN filtering disabled */  //vlan过滤
        // .jumbo_frame    = 0, /**< Jumbo Frame Support disabled */ //巨星帧的支持
        // .hw_strip_crc   = 0, /**< CRC stripped by hardware */  //使用硬件清除CRC
    },
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    },
};

struct rte_mempool *l2fwd_pktmbuf_pool = NULL;
struct rte_mempool *pool_clone = NULL;
struct rte_mempool *pool_arp = NULL;
/*
struct rte_ring *ring_clone;
struct rte_ring *ring_res;
struct rte_ring *ring_private;
uint32_t ring_clone_size;
uint32_t ring_res_size;
uint32_t ring_private_size;*/
unsigned int port[6][RTE_MAX_ETHPORTS];
/*udp socket*/
struct sockaddr_in addr;
// static unsigned long process_byte=0;
// const char    *optarg;
FILE *output_file;
unsigned int drop[60] = {0};
/* Per-port statistics struct */
struct l2fwd_port_statistics
{
    uint64_t tx;
    uint64_t rx;
    uint64_t dropped;
} __rte_cache_aligned;
struct l2fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];

/*fake packet header*/
struct psd
{
    uint32_t src;
    uint32_t dst;
    uint8_t mbz;
    uint8_t p;
    uint16_t len;
} pheader;

struct l2fwd_resources
{
    // volatile uint8_t force_quit;
    // uint8_t mac_updating;
    uint8_t event_mode;
    uint8_t process_type;
    unsigned long process_byte;
    unsigned host_num;
    unsigned route_num;
} __rte_cache_aligned;

static const char short_options[] =
    "p:" /* portmask */
    "q:" /* number of queues */
    "T:" /* timer period */
    "b:";

#define MAX_TIMER_PERIOD 86400 /* 1 day max */
#define CMD_LINE_OPT_MAC_UPDATING "mac-updating"
#define CMD_LINE_OPT_NO_MAC_UPDATING "no-mac-updating"

enum
{
   route_receive = 1,
   route_cut,
   route_inquiry = 12,
};

unsigned int Port_num;

/* Hash parameters. */
#ifdef RTE_ARCH_64
/* default to 4 million hash entries (approx) */
#define L3FWD_HASH_ENTRIES		(1024*1024*4)
#else
/* 32-bit has less address-space for hugepage memory, limit to 1M entries */
#define L3FWD_HASH_ENTRIES		(1024*1024*1)
#endif
#if defined(RTE_ARCH_X86) || defined(__ARM_FEATURE_CRC32)
#define EM_HASH_CRC 1
#endif

#ifdef EM_HASH_CRC
#include <rte_hash_crc.h> 
#define DEFAULT_HASH_FUNC       rte_hash_crc
#else
#include <rte_jhash.h>
#define DEFAULT_HASH_FUNC       rte_jhash
#endif

struct hash_route_array
{
    u_int8_t ip[16];
};
struct hash_route_array hash_route;

static u_int32_t ipv4_l3fwd_out_if[L3FWD_HASH_ENTRIES] __rte_cache_aligned;

/*hash down*/
struct hash_route_array hash_down_route;

static u_int32_t ipv4_l3fwd_down_out_if[L3FWD_HASH_ENTRIES] __rte_cache_aligned;

/*ip_route message*/
FILE *route_outfile;

struct ipv4_l3fwd_route_mess
{
    uint32_t ip;
    uint32_t next_ip;

};

#define MAX_Rnum 100
static struct ipv4_l3fwd_route_mess ipv4_l3fwd_up_route_mess_array[MAX_Rnum] = {RTE_IPV4(172, 0, 0, 0), RTE_IPV4(192, 168, 10, 101)};

static struct ipv4_l3fwd_route_mess ipv4_l3fwd_down_route_mess_array[MAX_Rnum] = {RTE_IPV4(55, 4, 68, 68), RTE_IPV4(1, 10, 168, 192)};

struct Route_num
{
    u_int32_t UP_Route_Num;
    u_int32_t Down_Route_Num;
    u_int32_t Total_Route_Num;
};
static struct Route_num Route_Num;

struct Route_del_num
{
    u_int32_t UP_Num;
    u_int32_t Down_Num
};

struct Route_del_num Route_del_Num;
u_int32_t R_Route_del_up[100]={0},R_Route_del_down[100]={0};
u_int32_t Route_down_nip;

/*antennae ip map*/
struct rte_hash *Antennae_htable;
struct hash_IP_Antennae
{
    u_int32_t number;
};
struct hash_IP_Antennae hash_IP_Antennae_map;

static u_int32_t IP_Antennae_out_if[L3FWD_HASH_ENTRIES] __rte_cache_aligned;

struct IP_Antennae_Map
{
    u_int32_t number;
    u_int32_t ip;
};
#define Antennae_Num 1
struct IP_Antennae_Map IP_Antennae_Map_array[Antennae_Num] = {
    RTE_IPV4(0, 0, 0, 1), RTE_IPV4(192, 168, 10, 101)
};

/*arp answer*/
#define ARP_ENTRY_STATUS_DYNAMIC 0
#define ARP_ENTRY_STATUS_STATIC 1

#if ARP_ENTRY_STATUS_DYNAMIC
u_int8_t gDefaultArpMac[6]={0x00,0x00,0x00,0x00,0x00,0x00};
u_int8_t Arp_Eth_Mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};
u_int8_t My_mac1[6]={0x30,0x24,0xa9,0x8e,0x87,0x7b};
u_int8_t My_mac2[6]={0x30,0x24,0xa9,0x8e,0x87,0x7a};
#define ENABLE_ARP_REPLY 	1
#define ENABLE_TIMER 1
u_int get_dst_portid(uint32_t dip);
static void
arp_request_timer_cb(void *arg,uint32_t dip);

struct arp_entry //入口
{ 
    uint32_t ip;
    uint8_t hwaddr[RTE_ETHER_ADDR_LEN];
    uint8_t status;
    unsigned int portid;

    // 多条记录用双向链表
    struct arp_entry *next;
    struct arp_entry *prev;
};

struct arp_table //arp表
{
    struct arp_entry *entries;
    int count;
};
#endif

/*static arp array*/

#if ARP_ENTRY_STATUS_STATIC

#define IPV4_L3FWD_NUM_ARP 3
struct ipv4_l3fwd_arp
{
    uint32_t host_ip;
    struct rte_ether_addr addr;
    uint32_t portid;
};
static struct ipv4_l3fwd_arp ipv4_l3fwd_arp_array[] = {
    //{RTE_IPV4(192, 168, 10, 101), {0x30,0x24,0xa9,0x8e,0x87,0x7b}, 0},
    //{RTE_IPV4(192, 168, 10, 101), {0xcc,0xb0,0xda,0xe2,0x0a,0x6a}, 0},//ceshiyi:cc:b0:da:e2:01:6a
    {RTE_IPV4(192, 168, 10, 101), {0x30,0x24,0xa9,0x8e,0x87,0x7b}, 0},//101
    {RTE_IPV4(192, 168, 10, 66), {0x34,0x29,0x8f,0x75,0xab,0x82}, 1},//66
    //{RTE_IPV4(192, 168, 10, 66), {0xe4, 0x3d, 0x1a, 0x1d, 0X1d, 0x76}, 1},
    {RTE_IPV4(192, 168, 10, 102), {0x30,0x24,0xa9,0x8e,0x87,0x7a}, 0},
};
/*arp hash*/
struct rte_hash *Arp_htable;
/**Waiting for ndp*/
/* struct hash_route_array arp_array; */
struct rte_ether_addr Addr_Arp_out_if[L3FWD_HASH_ENTRIES] __rte_cache_aligned;
static uint32_t Arp_dst_ports[NUM_PORTS];
#endif

u_int32_t SELF_IP[2]={RTE_IPV4(192, 168, 10, 50),RTE_IPV4(192, 168, 10, 60)};
#define BROA_IP htonl((uint32_t)0xffffffff)


/*secret message*/
struct secret_message
{
    u_int8_t encry_strategy;//加密策略
    u_int8_t offset;//偏移
    u_int8_t pass_parameter[16];//密码参数
};

struct RouteArray_message
{
    int32_t Mess_src;
    int32_t Mess_dst;
    u_int32_t Pack_num;
    u_int32_t Order_type;
    u_int16_t Data_length;
};

struct UD_R_mess
{
    u_int32_t number;
    u_int32_t timeH;
    u_int32_t timeL;
    u_int32_t dip;
    u_int32_t nnum;
    u_int32_t nip;
    u_int8_t metric;
}; 

//route receive
struct R_receive_mess_total
{
    u_int32_t ID;
    u_int32_t U_R_num;
    u_int32_t D_R_num;
};

//route delete
struct R_delete_mess_total
{
    u_int32_t ID;
    u_int32_t R_num;
};

struct R_De_mess
{
    u_int32_t dip;
    u_int32_t dnum;
    u_int8_t metric;
    u_int64_t time_effect;
    u_int32_t nip;
    u_int64_t time_implement;
};

//route change answer
struct Route_change_answer_total
{
    uint32_t ID;
    uint8_t result;
    uint32_t Error_Num;
};

struct Route_change_answer_mess
{
    uint32_t number;
    uint8_t Error_num;
};

//route inquire
struct R_inquire_mess_begin
{
    u_int32_t ID;
    uint8_t type;
};

//route inquire answer
struct R_Inq_answer_mess_total
{ 
    u_int32_t ID;
    uint8_t type; 
    uint16_t num;
};

struct R_Inq_answer_mess_route_total
{
    u_int32_t U_num;
    u_int32_t D_num;
};

struct UD_R_mess R_Inq_answer_route_total[MAX_Rnum];

/*UDPcheck_sum*/
#define UDP_CHECK_ON 0
#define UDP_CHECK_OFF 1
/*定时器*/
#define TIMER_RESOLUTION_CYCLES 120000000000ULL // 10ms * 1000 = 10s * 6

/*10->ascll*/
#define KB2ASC(x) (uint8_t)(0x30 + x)
/*ascll->10*/
#define ASC2KB(x) (uint8_t)(x % 0x30)

/* A tsc-based timer responsible for triggering statistics printout */
static uint64_t timer_period = 10; /* default period is 10 seconds */
static u_int64_t timer_onesec = 1;
static u_int64_t timer_4sec = 4;
static int mac_updating = 1;
int sockfd;
unsigned int back_count;
static void init_ring(void);
static void capture_loop(struct rte_mbuf *pkts_free, unsigned portid, struct l2fwd_resources *rsrc);
// static void output_loop();
unsigned short inline checksum(unsigned short *buffer, unsigned short size);
uint16_t udpHeader(struct rte_udp_hdr *udp, int size, struct psd *pheader);
uint16_t tcpHeader(struct rte_tcp_hdr *ip, int size, struct psd *pheader);
uint16_t ipHeader(struct rte_ipv4_hdr *ip);
static __rte_always_inline struct l2fwd_resources *l2fwd_get_rsrc(void);
/* Print out statistics on packets dropped */
//打印的属性跳过。。
static void
print_stats(void)
{
    uint64_t total_packets_dropped, total_packets_tx, total_packets_rx;
    unsigned portid;

    total_packets_dropped = 0;
    total_packets_tx = 0;
    total_packets_rx = 0;

    const char clr[] = {27, '[', '2', 'J', '\0'};
    const char topLeft[] = {27, '[', '1', ';', '1', 'H', '\0'};

    /* Clear screen and move to top left */
    printf("%s%s", clr, topLeft);

    printf("\nPort statistics ====================================");

    for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
    {
        /* skip disabled ports */
        if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
            continue;
        printf("\nStatistics for port %u ------------------------------"
               "\nPackets sent: %24" PRIu64
               "\nPackets received: %20" PRIu64
               "\nPackets dropped: %21" PRIu64,
               portid,
               port_statistics[portid].tx,
               port_statistics[portid].rx,
               port_statistics[portid].dropped);

        total_packets_dropped += port_statistics[portid].dropped;
        total_packets_tx += port_statistics[portid].tx;
        total_packets_rx += port_statistics[portid].rx;
    }
    printf("\nAggregate statistics ==============================="
           "\nTotal packets sent: %18" PRIu64
           "\nTotal packets received: %14" PRIu64
           "\nTotal packets dropped: %15" PRIu64,
           total_packets_tx,
           total_packets_rx,
           total_packets_dropped);
    printf("\n====================================================\n");
}

static void
l2fwd_mac_updating(struct rte_mbuf *m, unsigned dest_portid)
{
    struct rte_ether_hdr *eth;
    void *tmp;

    eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
    /* 02:00:00:00:00:xx */
    tmp = &eth->d_addr.addr_bytes[0];
    *((uint64_t *)tmp) = 0x000000000002 + ((uint64_t)dest_portid << 40);

    /* src addr */
    rte_ether_addr_copy(&l2fwd_ports_eth_addr[dest_portid], &eth->s_addr);
}

#define L3FWD_HASH_ENTRIES  1024*1024*4

struct rte_hash *htable;
struct rte_hash *Down_htable;

static struct rte_hash*
htable_create(const char *name)
{
    struct rte_hash *hash_table;
    struct rte_hash_parameters hash_params;
    (void)memset(&hash_params, sizeof(struct rte_hash_parameters), 0);
    
    hash_params.entries = L3FWD_HASH_ENTRIES;
    hash_params.key_len = sizeof(struct hash_route_array);
    hash_params.hash_func = rte_hash_crc;
    hash_params.hash_func_init_val = 0;
    
    hash_params.name = (char *)malloc(sizeof(char) * 30);
    memset(hash_params.name, 0, 30);
    strcpy(hash_params.name,name);
    hash_params.socket_id = rte_lcore_to_socket_id(rte_get_master_lcore());
    hash_params.extra_flag = 0;
    //hash_params.key_mode = RTE_HASH_KEY_MODE_DUP;
    
    /* Find if the hash table was created before */
    hash_table = rte_hash_find_existing(hash_params.name);
    if (hash_table != NULL) {
        printf("hash_table[%s] exist!\n", hash_params.name);
        return hash_table;
    } else { 
        hash_table = rte_hash_create(&hash_params);
        if (!hash_table) {
            return NULL;
        }
  }
}


unsigned comparison_route(struct rte_mbuf *m, unsigned portid, struct l2fwd_resources *rsrc)
{
    struct rte_ipv4_hdr *ipv4_hdr;
    ipv4_hdr = rte_pktmbuf_mtod_offset(m, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr)); 

    uint8_t *d_iph = m->buf_addr + m->data_off;
    struct rte_udp_hdr *udph = &d_iph[34];
    struct rte_ether_hdr *eth_hdr = &d_iph[0];

    if(portid == 0 && ipv4_hdr->next_proto_id == IPPROTO_UDP && udph->dst_port == htons(28891))
    {
        #if ARP_ENTRY_STATUS_STATIC
        arp_array.ip = Route_down_nip;
        int ret = rte_hash_lookup(Arp_htable, (void *)&arp_array);
        if (ret < 0) {return 22;}
        else{
            rte_ether_addr_copy(&Addr_Arp_out_if[ret], &eth_hdr->d_addr);
            ipv4_hdr->hdr_checksum = ipHeader(ipv4_hdr);
            return ipv4_l3fwd_arp_array[ret].portid;
        }
        #endif
        #if ARP_ENTRY_STATUS_DYNAMIC
        rte_memcpy(eth_hdr->d_addr.addr_bytes, get_dst_macaddr(Route_down_nip), RTE_ETHER_ADDR_LEN);
        #endif
    }
    else if(ipv4_hdr->dst_addr == (htonl)(BROA_IP)) return l2fwd_dst_ports[portid];
    else
    {
        #if ARP_ENTRY_STATUS_STATIC
        arp_array.ip = ipv4_hdr->dst_addr;
        int ret = rte_hash_lookup(Arp_htable, (void *)&arp_array);
        if (ret < 0) {return 22;}
        else{
            rte_ether_addr_copy(&Addr_Arp_out_if[ret], &eth_hdr->d_addr);
            ipv4_hdr->hdr_checksum = ipHeader(ipv4_hdr);
            return ipv4_l3fwd_arp_array[ret].portid;
        }
        #endif
        #if ARP_ENTRY_STATUS_DYNAMIC
        if(get_dst_macaddr(iph->dst_addr) != NULL) rte_memcpy(eth_hdr->d_addr.addr_bytes, get_dst_macaddr(iph->dst_addr), RTE_ETHER_ADDR_LEN);
        else{
            if(iph->dst_addr == htonl(0xc0a80a65)) rte_memcpy(eth_hdr->d_addr.addr_bytes, My_mac1, RTE_ETHER_ADDR_LEN);
            if(iph->dst_addr == htonl(0xc0a80a66)) rte_memcpy(eth_hdr->d_addr.addr_bytes, My_mac2, RTE_ETHER_ADDR_LEN);
        }
        #endif
    }
    
}

//初始化ip包头
void
initialize_eth_header(struct rte_ether_hdr *eth_hdr,
		struct rte_ether_addr *src_mac,
		struct rte_ether_addr *dst_mac, uint16_t ether_type,
		uint8_t vlan_enabled, uint16_t van_id)
{
	rte_ether_addr_copy(dst_mac, &eth_hdr->d_addr);
	rte_ether_addr_copy(src_mac, &eth_hdr->s_addr);

	if (vlan_enabled) {
		struct rte_vlan_hdr *vhdr = (struct rte_vlan_hdr *)(
			(uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));

		eth_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_VLAN);

		vhdr->eth_proto =  rte_cpu_to_be_16(ether_type);
		vhdr->vlan_tci = van_id;
	} else {
		eth_hdr->ether_type = rte_cpu_to_be_16(ether_type);
	}
}

/*动态ARP模块*/
#if ARP_ENTRY_STATUS_DYNAMIC


static int ng_encode_arp_pkt(uint8_t *msg, u_int16_t opcode, uint8_t *dst_mac, uint32_t sip, uint32_t dip,u_int portid) {

	// 1 ethhdr
	struct rte_ether_hdr *eth = (struct rte_ether_hdr *)msg;
	rte_memcpy(eth->s_addr.addr_bytes, (l2fwd_ports_eth_addr[portid].addr_bytes), RTE_ETHER_ADDR_LEN);
	if(opcode == RTE_ARP_OP_REPLY) rte_memcpy(eth->d_addr.addr_bytes, dst_mac, RTE_ETHER_ADDR_LEN);
    else if(opcode == RTE_ARP_OP_REQUEST) rte_memcpy(eth->d_addr.addr_bytes, Arp_Eth_Mac, RTE_ETHER_ADDR_LEN);
	eth->ether_type = htons(RTE_ETHER_TYPE_ARP);

	// 2 arp 
	struct rte_arp_hdr *arp = (struct rte_arp_hdr *)(eth + 1);
	arp->arp_hardware = htons(1);
	arp->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
	arp->arp_hlen = RTE_ETHER_ADDR_LEN;
	arp->arp_plen = sizeof(uint32_t);
	arp->arp_opcode = htons(opcode);

	rte_memcpy(arp->arp_data.arp_sha.addr_bytes, (l2fwd_ports_eth_addr[portid].addr_bytes), RTE_ETHER_ADDR_LEN);
	rte_memcpy( arp->arp_data.arp_tha.addr_bytes, dst_mac, RTE_ETHER_ADDR_LEN);

	arp->arp_data.arp_sip = sip;
	arp->arp_data.arp_tip = dip;
	
	return 0;

}

static struct rte_mbuf *ng_send_arp(struct rte_mempool *mbuf_pool, uint16_t opcode,uint8_t *dst_mac, uint32_t sip, uint32_t dip,u_int portid) {

	const unsigned total_length = sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr);

	struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
	if (!mbuf) {
		rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc\n");
	}

	mbuf->pkt_len = total_length;
	mbuf->data_len = total_length;

	uint8_t *pkt_data = rte_pktmbuf_mtod(mbuf, uint8_t *);
	ng_encode_arp_pkt(pkt_data, opcode, dst_mac, sip, dip,portid);

	return mbuf;
}

#define LL_ADD(item, list) do {		\
	item->prev = NULL;				\
	item->next = list;				\
	if (list != NULL) list->prev = item; \
	list = item;					\
} while(0)

#define LL_REMOVE(item, list) do {		\
	if (item->prev != NULL) item->prev->next = item->next;	\
	if (item->next != NULL) item->next->prev = item->prev;	\
	if (list == item) list = item->next;	\
	item->prev = item->next = NULL;			\
} while(0)

// 定义单例模式
static struct arp_table *arpt = NULL;
static struct arp_table *arp_table_instance(void)
{

    if (arpt == NULL)
    {

        arpt = rte_malloc("arp table", sizeof(struct arp_table), 0); //dpdk的malloc，这里0的意思是对齐
        if (arpt == NULL)
        {
            rte_exit(EXIT_FAILURE, "rte_malloc arp table failed\n");
        }
        memset(arpt, 0, sizeof(struct arp_table)); //每次malloc之后都要memeset
    }

    return arpt;
}

// 匹配目的mac地址，匹配上了就不添加，没有匹配就可以添加到arp表中
static uint8_t *get_dst_macaddr(uint32_t dip)
{
    struct arp_entry *iter;
    struct arp_table *table = arp_table_instance();

    for (iter = table->entries; iter != NULL; iter = iter->next)
    {
        if (dip == iter->ip)
        {
            return iter->hwaddr;
        }
    }

    return NULL;
}

// 匹配目的端口
u_int get_dst_portid(uint32_t dip)
{
    struct arp_entry *iter;
    struct arp_table *table = arp_table_instance();

    for (iter = table->entries; iter != NULL; iter = iter->next)
    {
        if (dip == iter->ip)
        {
            return iter->portid;
        }
    }

    return 22;
}
#endif

static void
print_ethaddr(const char *name, const struct rte_ether_addr *eth_addr)
{
	char buf[RTE_ETHER_ADDR_FMT_SIZE];
	rte_ether_format_addr(buf, RTE_ETHER_ADDR_FMT_SIZE, eth_addr);
	printf("%s%s", name, buf);
}

#if ENABLE_DEBUG
	struct arp_entry *iter;
	for (iter = table->entries; iter != NULL; iter = iter->next)
	{

		struct in_addr addr;
		addr.s_addr = iter->ip;

		print_ethaddr("arp table --> mac: ", (struct rte_ether_addr *)iter->hwaddr);

		printf(" ip: %s \n", inet_ntoa(addr));
	}
#endif

//Route receive
void 
R_receive(struct rte_mbuf *m,u_int16_t Data_length,unsigned portid)
{  
    uint8_t *d_iph = m->buf_addr + m->data_off;
    struct R_receive_mess_total *R_rmess = &d_iph[60];
    struct in_addr in;
    struct in_addr in_next;
    char abuf[INET6_ADDRSTRLEN];
    char abuf_next[INET6_ADDRSTRLEN];
    struct UD_R_mess U_R_mess[ntohl(R_rmess->U_R_num)];
    struct UD_R_mess D_R_mess[ntohl(R_rmess->D_R_num)];
    struct rte_ipv4_hdr *iph = &d_iph[14];
    struct rte_udp_hdr *udp = &d_iph[34];

    struct rte_ether_hdr *eth;
    eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

    uint32_t ture_up_route_num=0,ture_down_route_num=0,total_error_num=0;

    Route_Num.UP_Route_Num = Route_Num.UP_Route_Num + ntohl(R_rmess->U_R_num);
    Route_Num.Down_Route_Num = Route_Num.Down_Route_Num + ntohl(R_rmess->D_R_num);
    Route_Num.Total_Route_Num = Route_Num.Total_Route_Num + (ntohl(R_rmess->U_R_num)+ntohl(R_rmess->D_R_num));

    for(int i=0;i<ntohl(R_rmess->U_R_num);i++)
    {
        struct UD_R_mess *UD_R_mess = &d_iph[72+i*25];
        U_R_mess[i].number = UD_R_mess->number;
        U_R_mess[i].timeH = UD_R_mess->timeH;
        U_R_mess[i].timeL = UD_R_mess->timeL;
        U_R_mess[i].dip = UD_R_mess->dip;
        U_R_mess[i].nip = UD_R_mess->nip;
        U_R_mess[i].nnum = UD_R_mess->nnum;
        U_R_mess[i].metric = UD_R_mess->metric;    
        ture_up_route_num++;
        R_Inq_answer_route_total[i].number = UD_R_mess->number;
        R_Inq_answer_route_total[i].timeH = UD_R_mess->timeH;
        R_Inq_answer_route_total[i].timeL = UD_R_mess->timeL;
        R_Inq_answer_route_total[i].dip = UD_R_mess->dip;
        R_Inq_answer_route_total[i].nip = UD_R_mess->nip;
        R_Inq_answer_route_total[i].nnum = UD_R_mess->nnum;
        R_Inq_answer_route_total[i].metric = UD_R_mess->metric;    
    }

    for(int i=0;i<ntohl(R_rmess->D_R_num);i++)
    {
        struct UD_R_mess *UD_R_mess = &d_iph[72+(i+ntohl(R_rmess->U_R_num))*25];
        D_R_mess[i].number = UD_R_mess->number;
        D_R_mess[i].timeH = UD_R_mess->timeH;
        D_R_mess[i].timeL = UD_R_mess->timeL;
        D_R_mess[i].dip = UD_R_mess->dip;
        D_R_mess[i].nip = UD_R_mess->nip;  
        D_R_mess[i].nnum = UD_R_mess->nnum;
        D_R_mess[i].metric = UD_R_mess->metric;
        ture_down_route_num++;
        R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].number = UD_R_mess->number;
        R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].timeH = UD_R_mess->timeH;
        R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].timeL = UD_R_mess->timeL;
        R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].dip = UD_R_mess->dip;
        R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].nip = UD_R_mess->nip;
        R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].nnum = UD_R_mess->nnum;
        R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].metric = UD_R_mess->metric;
    }

    //up route recieve
    for(int i=0;i<ntohl(R_rmess->U_R_num);i++)
    {
        ipv4_l3fwd_up_route_mess_array[i].ip = U_R_mess[i].dip;
        hash_IP_Antennae_map.number = U_R_mess[i].nip;
        int ret = rte_hash_lookup(Antennae_htable, (void *)&hash_IP_Antennae_map);
        if (ret < 0) {
            printf("Antennae_hash_lookup key failed!\n");
        } else {
            printf("Antennae_hash_lookup key success! ret:%d\n", ret);
        }
        ipv4_l3fwd_up_route_mess_array[i].next_ip = IP_Antennae_out_if[ret];;
        in.s_addr = ipv4_l3fwd_up_route_mess_array[i].ip;
        in_next.s_addr = ipv4_l3fwd_up_route_mess_array[i].next_ip;
		printf("LPM: Adding route dst-ip:%s  // next-ip:%s\n",
		       inet_ntop(AF_INET, &in, abuf, sizeof(abuf)),
               inet_ntop(AF_INET, &in_next, abuf_next, sizeof(abuf_next)));
        hash_route.ip = ipv4_l3fwd_up_route_mess_array[i].ip;
        ret = rte_hash_add_key(htable, (void *)&hash_route);
        if (ret < 0) {
            printf("rte_hash_add_key %d failed!\n", i);
        } else {
            printf("rte_hash_add_key %d success! idx:%d\n", i, ret);
        }
        ipv4_l3fwd_out_if[ret] = ipv4_l3fwd_up_route_mess_array[i].next_ip;
    }
    int size =rte_hash_count(htable);
    printf("up htable size=%d\n", size);

    //down route recieve
    for(int i=0;i<ntohl(R_rmess->D_R_num);i++)
    {
        ipv4_l3fwd_down_route_mess_array[i].ip = D_R_mess[i].dip;
        ipv4_l3fwd_down_route_mess_array[i].next_ip = D_R_mess[i].nip;

        in.s_addr = ipv4_l3fwd_down_route_mess_array[i].ip;
        in_next.s_addr = ipv4_l3fwd_down_route_mess_array[i].next_ip;
		printf("LPM: Adding down route dst-ip:%s  // next-ip:%s\n",
		       inet_ntop(AF_INET, &in, abuf, sizeof(abuf)),
               inet_ntop(AF_INET, &in_next, abuf_next, sizeof(abuf_next)));
        hash_down_route.ip = ipv4_l3fwd_down_route_mess_array[i].ip;
        int ret = rte_hash_add_key(Down_htable, (void *)&hash_down_route);
        if (ret < 0) {
            printf("rte_hash_down_add_key %d failed!\n", i);
        } else {
            printf("rte_hash_down_add_key %d success! idx:%d\n", i, ret);
        }
        ipv4_l3fwd_down_out_if[ret] = ipv4_l3fwd_down_route_mess_array[i].next_ip;
    }
    size =rte_hash_count(Down_htable);
    printf("down htable size=%d\n", size);

    total_error_num = ntohl(R_rmess->U_R_num)+ntohl(R_rmess->D_R_num)-ture_down_route_num-ture_up_route_num;
    struct RouteArray_message *Route_mess = &d_iph[42];
    rte_be16_t differ_num;
    if(total_error_num == 0){
        struct Route_change_answer_total *Route_change_answer_total = &d_iph[60]; 
        differ_num = (ntohs)(iph->total_length) - 55;
        m->data_len = m->data_len - differ_num;
        m->buf_len = m->buf_len - differ_num;
        m->pkt_len = m->pkt_len - differ_num;
        d_iph[57] = 0x03;
        u_int32_t ip_addr = iph->dst_addr;
        iph->dst_addr = iph->src_addr;
        iph->src_addr = ip_addr;
        /*u_int8_t ip_mac[6];
        for(int j=0;j<6;j++){
            ip_mac[j] = d_iph[j];
            d_iph[j] = d_iph[j+6];
        }*/
        rte_ether_addr_copy(&l2fwd_ports_eth_addr[portid], &eth->s_addr);

        iph->total_length = htons((ntohs)(iph->total_length) - differ_num);

        int32_t mess_src = Route_mess->Mess_src;
        Route_mess->Mess_src=Route_mess->Mess_dst;
        Route_mess->Mess_dst=mess_src;
        Route_mess->Data_length = htons((ntohs)(iph->total_length) - 28 - 18);

        Route_change_answer_total->ID = R_rmess->ID;
        Route_change_answer_total->result = 0x00;
        Route_change_answer_total->Error_Num = (htonl)(total_error_num);
        d_iph[67] = 0x00;

        rte_be16_t src_port;
        src_port = udp->dst_port;
        udp->dst_port = udp->src_port;
        udp->src_port = src_port;

        //iph->hdr_checksum = ipHeader(iph);
        udp->dgram_len = htons(9+18);
        pheader.src = iph->src_addr;
        pheader.dst = iph->dst_addr;
        pheader.mbz = 0;
        pheader.len = udp->dgram_len;
        pheader.p = 17;
        udp->dgram_cksum = udpHeader(udp, (ntohs)(udp->dgram_len), &pheader);
        #if UDP_CHECK_ON
        pheader.src = iph->src_addr;
        pheader.dst = iph->dst_addr;
        pheader.mbz = 0;
        pheader.len = udp->dgram_len;
        pheader.p = 17;
        udp->dgram_cksum = udpHeader(udp, (ntohs)(udp->dgram_len), &pheader);
        #endif
    }
    else{
        uint32_t error_route_num[total_error_num];
    }
}

void arrDelete(u_int32_t* arr,int index,int len){
    for (int i = 0; i < len-1; i++) {
        if (i < index) {
            arr[i] = arr[i];
        }else{
            arr[i] = arr[i+1];
        }
    }
    arr[len-1] = '\0';
}

//Route delete
void 
R_delete(struct rte_mbuf *m,u_int16_t Data_length,unsigned portid)
{
    uint8_t *d_iph = m->buf_addr + m->data_off;
    struct R_delete_mess_total *R_dmess_total = &d_iph[60];
    struct in_addr in;
    char abuf[INET6_ADDRSTRLEN];
    struct rte_ipv4_hdr *iph = &d_iph[14];
    struct rte_udp_hdr *udp = &d_iph[34];
    struct rte_ether_hdr *eth;
    eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

    uint32_t total_error_num=0;
    Route_del_Num.Down_Num=0;Route_del_Num.UP_Num=0;
    
    for(int i=0;i<(ntohl)(R_dmess_total->R_num);i++)
    {
        struct R_De_mess *R_dmess = &d_iph[69+i*30];
        uint8_t direction = d_iph[68+i*30];
        if(direction == 0x00){
            hash_route.ip = R_dmess->dip;
            in.s_addr = R_dmess->dip;
		printf("LPM:delete up route dst-ip:%s\n",
		       inet_ntop(AF_INET, &in, abuf, sizeof(abuf)));
            int ret = rte_hash_del_key(htable, (void *)&hash_route);
            if (ret < 0) {
                printf("rte_hash_del key failed!\n");
                total_error_num++;
            } else {
                R_Route_del_up[ret] = 1;
                Route_del_Num.UP_Num++;
                printf("rte_hash_del key success! ret:%d\n", ret);
            }
            int size =rte_hash_count(htable);
            printf("htable size=%d\n", size);
        }
        else if(direction == 0x01){
            hash_down_route.ip = R_dmess->dip;
            in.s_addr = R_dmess->dip;
		printf("LPM: delete down route dst-ip:%s\n",
		       inet_ntop(AF_INET, &in, abuf, sizeof(abuf)));
            int ret = rte_hash_del_key(Down_htable, (void *)&hash_down_route);
            if (ret < 0) {
                printf("rte_hash_down_del key failed!\n");
                total_error_num++;
            } else {
                R_Route_del_down[ret] = 1;
                Route_del_Num.Down_Num++;
                printf("rte_hash_down_del key success! ret:%d\n", ret);
            }
            int size =rte_hash_count(Down_htable);
            printf("Down_htable size=%d\n", size);
        }
    }
    rte_be16_t differ_num;
    struct RouteArray_message *Route_mess = &d_iph[42];
    if(total_error_num == 0){
        struct Route_change_answer_total *Route_change_answer_total = &d_iph[60]; 
        differ_num = (ntohs)(iph->total_length) - 55;
        m->data_len = m->data_len - differ_num;
        m->buf_len = m->buf_len - differ_num;
        m->pkt_len = m->pkt_len - differ_num;
        d_iph[57] = 0x03;
        u_int32_t ip_addr = iph->dst_addr;
        iph->dst_addr = iph->src_addr;
        iph->src_addr = ip_addr;
        //for(int j=0;j<6;j++) d_iph[j] = d_iph[j+6];

        rte_ether_addr_copy(&l2fwd_ports_eth_addr[portid], &eth->s_addr);

        iph->total_length = htons((ntohs)(iph->total_length) - differ_num);

        int32_t mess_src = Route_mess->Mess_src;
        Route_mess->Mess_src=Route_mess->Mess_dst;
        Route_mess->Mess_dst=mess_src;
        Route_mess->Data_length = htons((ntohs)(iph->total_length) - 28 - 18);

        Route_change_answer_total->ID = R_dmess_total->ID;
        Route_change_answer_total->result = 0x00;
        Route_change_answer_total->Error_Num = (htonl)(total_error_num);

        rte_be16_t src_port;
        src_port = udp->dst_port;
        udp->dst_port = udp->src_port;
        udp->src_port = src_port;

       // iph->hdr_checksum = ipHeader(iph);
        udp->dgram_len = htons(9+18);
        pheader.src = iph->src_addr;
        pheader.dst = iph->dst_addr;
        pheader.mbz = 0;
        pheader.len = udp->dgram_len;
        pheader.p = 17;
        udp->dgram_cksum = udpHeader(udp, (ntohs)(udp->dgram_len), &pheader);
        #if UDP_CHECK_ON
        pheader.src = iph->src_addr;
        pheader.dst = iph->dst_addr;
        pheader.mbz = 0;
        pheader.len = udp->dgram_len;
        pheader.p = 17;
        udp->dgram_cksum = udpHeader(udp, (ntohs)(udp->dgram_len), &pheader);
        #endif
    }
    else{
        uint32_t error_route_num[total_error_num];
    }
}

//route inquire
#define Route_inquire_answer 1
#define GREdrainage_inquire_answer 2
#define GRE_inquire_answer 3

void
R_Route_inquire_answer(struct rte_mbuf *m,u_int16_t Data_length,unsigned portid)
{
    uint8_t *d_iph = m->buf_addr + m->data_off;
    struct rte_ipv4_hdr *iph = &d_iph[14];
    struct rte_udp_hdr *udp = &d_iph[34];
    struct R_Inq_answer_mess_total *R_Inq_answer_mess_total = &d_iph[60];
    struct R_Inq_answer_mess_route_total *R_Inq_answer_mess_route_total = &d_iph[67];
    u_int32_t RRoute_num = Route_Num.Total_Route_Num-(Route_del_Num.Down_Num+Route_del_Num.UP_Num);
    int del_num_up=0,del_num_down=0;
    struct RouteArray_message *Route_mess = &d_iph[42];
    struct rte_ether_hdr *eth;
    eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

    m->data_len = m->data_len + 10 + 25*RRoute_num;
    m->buf_len = m->buf_len + 10 + 25*RRoute_num;
    m->pkt_len = m->pkt_len + 10 + 25*RRoute_num;
    
    d_iph[57] = 0x0d;
    u_int32_t ip_addr = iph->dst_addr;
    iph->dst_addr = iph->src_addr;
    iph->src_addr = ip_addr;
    //for(int j=0;j<6;j++) d_iph[j] = d_iph[j+6];
    rte_ether_addr_copy(&l2fwd_ports_eth_addr[portid], &eth->s_addr); 
    Data_length = 15+25*RRoute_num;
    iph->total_length = htons(Data_length+18+28);

    int32_t mess_src = Route_mess->Mess_src;
    Route_mess->Mess_src=Route_mess->Mess_dst;
    Route_mess->Mess_dst=mess_src;
    Route_mess->Data_length = htons((ntohs)(iph->total_length) - 28 - 18);

    uint16_t total_num = RRoute_num;
    R_Inq_answer_mess_total->num = total_num;
    R_Inq_answer_mess_route_total->U_num = (htonl)(Route_Num.UP_Route_Num-Route_del_Num.UP_Num);
    R_Inq_answer_mess_route_total->D_num = (htonl)(Route_Num.Down_Route_Num-Route_del_Num.Down_Num);

    for(int i=0;i<Route_Num.UP_Route_Num;i++)
    {
        if(R_Route_del_up[i]==1) del_num_up++;
        else{
        struct UD_R_mess *R_Inq_answer_mess_route = &d_iph[75+(i-del_num_up)*25];
        R_Inq_answer_mess_route->number = R_Inq_answer_route_total[i].number;
        R_Inq_answer_mess_route->timeH = R_Inq_answer_route_total[i].timeH;
        R_Inq_answer_mess_route->timeL = R_Inq_answer_route_total[i].timeL;
        R_Inq_answer_mess_route->dip = R_Inq_answer_route_total[i].dip;
        R_Inq_answer_mess_route->nip = R_Inq_answer_route_total[i].nip;
        R_Inq_answer_mess_route->nnum = R_Inq_answer_route_total[i].nnum;
        R_Inq_answer_mess_route->metric = R_Inq_answer_route_total[i].metric;
        }
    }
    for(int i=0;i<Route_Num.Down_Route_Num;i++)
    {
        if(R_Route_del_down[i]==1) del_num_down++;
        else{
        struct UD_R_mess *R_Inq_answer_mess_route = &d_iph[75+(i-del_num_down+(Route_Num.UP_Route_Num-Route_del_Num.UP_Num))*25];
        R_Inq_answer_mess_route->number = R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].number;
        R_Inq_answer_mess_route->timeH = R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].timeH;
        R_Inq_answer_mess_route->timeL = R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].timeL;
        R_Inq_answer_mess_route->dip = R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].dip;
        R_Inq_answer_mess_route->nip = R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].nip;
        R_Inq_answer_mess_route->nnum = R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].nnum;
        R_Inq_answer_mess_route->metric = R_Inq_answer_route_total[i+Route_Num.UP_Route_Num].metric;
        }
    }

    rte_be16_t src_port;
    src_port = udp->dst_port;
    udp->dst_port = udp->src_port;
    udp->src_port = src_port;

    //iph->hdr_checksum = ipHeader(iph);

    udp->dgram_len = htons(Data_length+18);
    pheader.src = iph->src_addr;
    pheader.dst = iph->dst_addr;
    pheader.mbz = 0;
    pheader.len = udp->dgram_len;
    pheader.p = 17;
    udp->dgram_cksum = udpHeader(udp, (ntohs)(udp->dgram_len), &pheader);
    #if UDP_CHECK_ON
    pheader.src = iph->src_addr;
    pheader.dst = iph->dst_addr;
    pheader.mbz = 0;
    pheader.len = udp->dgram_len;
    pheader.p = 17;
    udp->dgram_cksum = udpHeader(udp, (ntohs)(udp->dgram_len), &pheader);
    #endif
}

void
R_inquiry(struct rte_mbuf *m,u_int16_t Data_length,unsigned portid)
{
    uint8_t *d_iph = m->buf_addr + m->data_off;
    struct R_inquire_mess_begin *R_imess_total = &d_iph[60]; 
    switch (R_imess_total->type)
    {
    case Route_inquire_answer:
        printf("-------into route inquire answer!-------\n");
        R_Route_inquire_answer(m,Data_length,portid);
        break;
    default:
        break;
    }
}

//route inquire answer

//处理收到的数据包
static void
l2fwd_simple_forward(struct rte_mbuf *m, unsigned portid, struct l2fwd_resources *rsrc)
{
    unsigned dst_port = 0;
    int sent;
    struct rte_eth_dev_tx_buffer *buffer;
    /*struct rte_ipv4_hdr *ipv4_hdr;
    ipv4_hdr = rte_pktmbuf_mtod_offset(m, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));

    dst_port = comparison_route(m,portid,rsrc);*/
    dst_port = comparison_route(m,portid,rsrc);
    if (dst_port != 22)
    {
        //获取目的端口的 tx 缓存
        buffer = tx_buffer[dst_port];
        //发送数据包，到目的端口的tx缓存
        sent = rte_eth_tx_buffer(dst_port, 0, buffer, m);

        if (sent)
            port_statistics[dst_port].tx += sent; //如果发包成功，发包计数+1
    }
    else{
    #if ARP_ENTRY_STATUS_DYNAMIC
    if(((ipv4_hdr->dst_addr) & ((htonl)((u_int32_t)0xffffff00))) == htonl((u_int32_t)0xc0a80a00)){
        arp_request_timer_cb(pool_arp,ipv4_hdr->dst_addr);
        }
    #endif
    rte_pktmbuf_free(m);
    }
    
    //更新MAC地址
    if (unlikely(mac_updating))
        l2fwd_mac_updating(m, dst_port);
        
    //rte_pktmbuf_free(m);
}

#if ARP_ENTRY_STATUS_DYNAMIC

static void
arp_request_timer_cb(void *arg,uint32_t dip)
{

	struct rte_mempool *mbuf_pool = (struct rte_mempool *)arg;

    #if 0
        struct rte_mbuf *arpbuf = ng_send_arp(mbuf_pool, RTE_ARP_OP_REQUEST, ahdr->arp_data.arp_sha.addr_bytes, 
            ahdr->arp_data.arp_tip, ahdr->arp_data.arp_sip);

        rte_eth_tx_burst(gDpdkPortId, 0, &arpbuf, 1);
        rte_pktmbuf_free(arpbuf);

    #endif
    
	for (uint16_t i = 0; i <= 1; i++)
	{

		uint32_t dstip = (htonl)(SELF_IP[i]);

		struct in_addr addr;
		addr.s_addr = dstip;
		//printf("arp ---> src: %s \n", inet_ntoa(addr));

		struct rte_mbuf *arpbuf = NULL;
		uint8_t *dstmac = get_dst_macaddr(dstip);
		if (dstmac == NULL)
		{
            //arpbuf = ng_send_arp(mbuf_pool, RTE_ARP_OP_REQUEST, l2fwd_ports_eth_addr[i].addr_bytes, (htonl)(SELF_IP[i]), dstip,i);
			arpbuf = ng_send_arp(mbuf_pool, RTE_ARP_OP_REQUEST, gDefaultArpMac, dstip, dip, i);
		}
		else
		{
			arpbuf = ng_send_arp(mbuf_pool, RTE_ARP_OP_REQUEST, dstmac, dstip, dip,i);
		}

		rte_eth_tx_burst(i, 0, &arpbuf, 1);
		rte_pktmbuf_free(arpbuf);
	}
}
#endif

#if ARP_ENTRY_STATUS_STATIC
void Arp_Hash_Add()
{
    for(int i=0;i<IPV4_L3FWD_NUM_ARP;i++)
    {
        arp_array.ip = (htonl)(ipv4_l3fwd_arp_array[i].host_ip);
        int ret = rte_hash_add_key(Arp_htable, (void *)&arp_array);
        if (ret < 0) {
            printf("Arp_hash_add_key %d failed!\n", i);
        } else {
            printf("Arp_hash_add_key %d success! idx:%d\n", i, ret);
        }
        rte_ether_addr_copy(&ipv4_l3fwd_arp_array[i].addr, &Addr_Arp_out_if[ret]);
    }
    int size =rte_hash_count(Arp_htable);
    printf("htable size=%d\n", size);
}
#endif

/* main processing loop */
static void
l2fwd_main_loop(struct l2fwd_resources *rsrc)
{
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
    struct rte_mbuf *m;
    int sent;
    unsigned lcore_id;
    uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc,arp_timer;
    unsigned i, j, portid, nb_rx;
    struct lcore_queue_conf *qconf;
    const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S *
                               BURST_TX_DRAIN_US;
    struct rte_eth_dev_tx_buffer *buffer;

    prev_tsc = 0;
    timer_tsc = 0;
    arp_timer = 0;

    //获取当前核心的ID
    lcore_id = rte_lcore_id();
    //获取当前核心的配置
    qconf = &lcore_queue_conf[lcore_id];

    //如果 rx_port的个数为0，则 记录日志。（其实是出错了）
    if (qconf->n_rx_port == 0)
    {
        RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n", lcore_id);
        return;
    }

    //记录日志，
    RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

    //遍历所有的port
    for (i = 0; i < qconf->n_rx_port; i++)
    {
        //获取到portID
        portid = qconf->rx_port_list[i];
        //记录日志
        RTE_LOG(INFO, L2FWD, " -- lcoreid=%u portid=%u\n", lcore_id,
                portid);
    }
    htable = htable_create("htable_up");
    Down_htable = htable_create("htable_down");
    Antennae_htable = htable_create("htable_antennae");
    Arp_htable = htable_create("htable_arp");

    for(int i=0;i<Antennae_Num;i++)
    {
    hash_IP_Antennae_map.number = (htonl)(IP_Antennae_Map_array[i].number);
        int ret = rte_hash_add_key(Antennae_htable, (void *)&hash_IP_Antennae_map);
        if (ret < 0) {
            printf("Antennae_hash_add_key %d failed!\n", i);
        } else {
            printf("Antennae_hash_add_key %d success! idx:%d\n", i, ret);
        }
        IP_Antennae_out_if[ret] = (htonl)(IP_Antennae_Map_array[i].ip);
    }
    int size =rte_hash_count(Antennae_htable);
    printf("htable size=%d\n", size);
    #if ARP_ENTRY_STATUS_STATIC
    Arp_Hash_Add();
    #endif
    hash_route.ip = (htonl)(ipv4_l3fwd_up_route_mess_array[0].ip);
    int ret = rte_hash_add_key(htable, (void *)&hash_route);
        if (ret < 0) {
            printf("rte_hash_add_key %d failed!\n", ret);
        } else {
            printf("rte_hash_add_key %d success!\n", ret);
        }
    ipv4_l3fwd_out_if[ret] = (htonl)(ipv4_l3fwd_up_route_mess_array[0].next_ip); 
    #if ARP_ENTRY_STATUS_DYNAMIC
    arp_request_timer_cb(pool_arp,htonl(0xc0a80a42));
    #endif

    //如果没超时（最开始设置的那个二层转发的运行时间）
    while (!force_quit)
    {
        //获取时间戳
        cur_tsc = rte_rdtsc();

        /*
         * TX burst queue drain
         */
        //对比时间戳
        diff_tsc = cur_tsc - prev_tsc;
        if (unlikely(diff_tsc > drain_tsc))
        {

            for (i = 0; i < qconf->n_rx_port; i++)
            {
                //获得portid和buffer
                portid = l2fwd_dst_ports[qconf->rx_port_list[i]];
                buffer = tx_buffer[portid];

                //把buffer中的数据发送到portid对应的port
                sent = rte_eth_tx_buffer_flush(portid, 0, buffer);
                if (sent)
                    port_statistics[portid].tx += sent; //如果发包成功，发包计数+1
            }

            /* if timer is enabled */
            //如果计时器开启
            if (timer_period > 0)
            {
                
                /* advance the timer */
                //调整计时器
                timer_tsc += diff_tsc;

                #if ENABLE_TIMER
                arp_timer += diff_tsc;
                if (arp_timer > TIMER_RESOLUTION_CYCLES)
                {
                    rte_timer_manage();
                    arp_timer = 0;
                    arp_request_timer_cb(pool_arp,htonl(0xc0a80a42));
                }

                #endif

                /* if timer has reached its timeout */
                //如果计时器超时
                if (unlikely(timer_tsc >= timer_period))
                {
                    /* do this only on master core */
                    //主线程打印一些属性，仅有主线程会执行print_stats
                    // if (lcore_id == rte_get_master_lcore()) {
                    if (lcore_id == rte_get_main_lcore())
                    {
                        print_stats(); //打印属性
                        /* reset the timer */
                        timer_tsc = 0; //设置计时器为0
                    }
                }
            }

            prev_tsc = cur_tsc;
        }

        /*
         * Read packet from RX queues   //收包模块
         */
        Port_num = qconf->n_rx_port;
        for (i = 0; i < qconf->n_rx_port; i++)
        {
            //获取portID
            portid = qconf->rx_port_list[i];
            //从portID对应的Port收到nb_rx个包
            nb_rx = rte_eth_rx_burst((uint8_t)portid, 0,
                                     pkts_burst, MAX_PKT_BURST);

            port_statistics[portid].rx += nb_rx; //收包计数+=nb_rx

            for (j = 0; j < nb_rx; j++)
            { //遍历收到的包
                m = pkts_burst[j];
                rte_prefetch0(rte_pktmbuf_mtod(m, void *)); //预取到pktmbuf
                #if ARP_ENTRY_STATUS_DYNAMIC
                    struct rte_ether_hdr *ehdr;
                    ehdr = rte_pktmbuf_mtod(m,
						struct rte_ether_hdr *);
                    //struct rte_ether_hdr *ehdr = rte_pktmbuf_mtod_offset(m, struct rte_ether_hdr *, sizeof(struct rte_ether_hdr));
                    if (ehdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP))
                    {
                        // 如果是arp的协议，需要转成arp结构（以太头后）
                        struct rte_arp_hdr *ahdr = (struct rte_arp_hdr *)(
                        (char *)ehdr + sizeof(struct rte_ether_hdr));
                        //struct rte_arp_hdr *ahdr = rte_pktmbuf_mtod_offset(m, struct rte_arp_hdr *, sizeof(struct rte_arp_hdr));
                        
                        // 打印接收到的数据相关信息
                        struct in_addr addr;
                        addr.s_addr = ahdr->arp_data.arp_tip;
                        //printf("arp ---> src: %s ", inet_ntoa(addr));

                        addr.s_addr = (htonl)(SELF_IP[portid]);
                        //printf("local: %s \n", inet_ntoa(addr));
                        
                        if (ahdr->arp_data.arp_tip == (htonl)(SELF_IP[portid]))
                        {
                            if (ahdr->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REQUEST)) // rte_cpu_to_be_16 转换大小端
	                        {
                                // 接收数据response的响应
                                struct rte_mbuf *arpbuf = ng_send_arp(pool_arp, RTE_ARP_OP_REPLY, ahdr->arp_data.arp_sha.addr_bytes,
                                                                    ahdr->arp_data.arp_tip, ahdr->arp_data.arp_sip,portid); // response里的源ip就是request里的目的ip

                                rte_eth_tx_burst(portid, 0, &arpbuf, 1); // 将数据send出去
                                rte_pktmbuf_free(arpbuf);

                                //rte_pktmbuf_free(m);
                                //arp_request_timer_cb(pool_arp,ahdr->arp_data.arp_sip);
                            }
                            else if (ahdr->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REPLY))
                            {
                                //printf("arp --> reply\n");

                                struct arp_table *table = arp_table_instance();

                                uint8_t *hwaddr = get_dst_macaddr(ahdr->arp_data.arp_sip);
                                if (NULL == hwaddr)
                                {
                                    struct arp_entry *entry = rte_malloc("arp_entry", sizeof(struct arp_entry), 0);
                                    if (entry)
                                    {
                                        memset(entry, 0, sizeof(struct arp_entry));

                                        entry->ip = ahdr->arp_data.arp_sip;
                                        rte_memcpy(entry->hwaddr, ahdr->arp_data.arp_sha.addr_bytes, RTE_ETHER_ADDR_LEN);
                                        entry->status = 0;
                                        entry->portid = portid;

                                        LL_ADD(entry, table->entries);
                                        table->count++;
                                        print_ethaddr("arp:",&ahdr->arp_data.arp_sha);
                                    }
                                }
                            }
                        }

                        continue;
                    }
                #endif

                capture_loop(m, portid, rsrc);
                l2fwd_simple_forward(m, portid, rsrc); //处理收到的包
            } 
        }
        //read_cmd();
        /*while(!kbhit())
        {
            printf("You pressed '%c'!/n", getchar());
        }*/
    }
}

static __rte_always_inline struct l2fwd_resources *
l2fwd_get_rsrc(void)
{
    static const char name[RTE_MEMZONE_NAMESIZE] = "rsrc";
    const struct rte_memzone *mz;

    mz = rte_memzone_lookup(name);
    if (mz != NULL)
        return mz->addr;

    mz = rte_memzone_reserve(name, sizeof(struct l2fwd_resources), 0, 0);
    if (mz != NULL)
    {
        struct l2fwd_resources *rsrc = mz->addr;

        memset(rsrc, 0, sizeof(struct l2fwd_resources));
        // rsrc->mac_updating = true;
        rsrc->host_num = 2;
        rsrc->route_num = 0;
        // rsrc->rx_queue_per_lcore = 1;
        // rsrc->sched_type = RTE_SCHED_TYPE_ATOMIC;
        // rsrc->timer_period = 10 * rte_get_timer_hz();

        return mz->addr;
    }
}
//__attribute__((unused))表示可能没有这个参数，编译器不会报错
static int
l2fwd_launch_one_lcore(__attribute__((unused)) void *dummy)
{
    struct l2fwd_resources *rsrc = dummy;
    l2fwd_main_loop(rsrc); //二层转发主要循环
    return 0;
}

/* display usage */
static void
l2fwd_usage(const char *prgname)
{
    printf("%s [EAL options] -- -p PORTMASK [-q NQ]\n"
           "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
           "  -q NQ: number of queue (=ports) per lcore (default is 1)\n"
           "  -T PERIOD: statistics will be refreshed each PERIOD seconds (0 to disable, 10 default, 86400 maximum)\n"
           "  --[no-]mac-updating: Enable or disable MAC addresses updating (enabled by default)\n"
           "      When enabled:\n"
           "       - The source MAC address is replaced by the TX port MAC address\n"
           "       - The destination MAC address is replaced by 02:00:00:00:00:TX_PORT_ID\n",
           prgname);
}

static int
l2fwd_parse_portmask(const char *portmask)
{
    char *end = NULL;
    unsigned long pm;

    /* parse hexadecimal string */
    //解析十六进制字符串，将十六进制字符串转换为unsigned long
    pm = strtoul(portmask, &end, 16);
    if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
        return -1;

    if (pm == 0)
        return -1;

    return pm;
}

static unsigned int
l2fwd_parse_nqueue(const char *q_arg)
{
    char *end = NULL;
    unsigned long n;

    /* parse hexadecimal string */
    n = strtoul(q_arg, &end, 10);
    if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
        return 0;
    if (n == 0)
        return 0;
    if (n >= MAX_RX_QUEUE_PER_LCORE)
        return 0;

    return n;
}

static int
l2fwd_parse_timer_period(const char *q_arg)
{
    char *end = NULL;
    int n;

    /* parse number string */
    n = strtol(q_arg, &end, 10);
    if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
        return -1;
    if (n >= MAX_TIMER_PERIOD)
        return -1;

    return n;
}

static int
l2fwd_parse_process_byte(const char *q_arg)
{
    char *end = NULL;
    unsigned long n;

    /* parse number string */
    n = strtol(q_arg, &end, 10);
    if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
        return -1;

    return n;
}


//解析命令行参数
/* Parse the argument given in the command line of the application */
static int
l2fwd_parse_args(int argc, char **argv, struct l2fwd_resources *rsrc)
{
    int opt, ret, timer_secs;
    char **argvopt;
    int option_index;
    char *prgname = argv[0];
    struct option lgopts[] = {
        {CMD_LINE_OPT_MAC_UPDATING, no_argument, &mac_updating, 1},
        {CMD_LINE_OPT_NO_MAC_UPDATING, no_argument, &mac_updating, 0},
        /*{ CMD_LINE_OPT_PROCESS_BYTE, required_argument, NULL,
                            CMD_LINE_OPT_PROCESS_BYTE_NUM}, */
        {NULL, 0, 0, 0}
        /*static struct option lgopts[] = {
            { "mac-updating", no_argument, &mac_updating, 1},
            { "no-mac-updating", no_argument, &mac_updating, 0},
            { "procecss-num", no_argument, &process_type, 1},
            {NULL, 0, 0, 0}*/
    };

    argvopt = argv; //复制argv指针

    //解析命令行，getopt_long()可以解析命令行参数
    while ((opt = getopt_long(argc, argvopt, short_options,
                              lgopts, &option_index)) != EOF)
    {

        //命令行有三个参数， p:portmask，n:nqueue，T：timer period
        switch (opt)
        {
        /* portmask */
        // port的个数，以十六进制表示的，如0x0f表示15
        case 'p':
            // l2fwd_parse_portmask()函数为自定义函数
            //把十六进制的mask字符串转换成unsigned long。
            //端口使能情况
            l2fwd_enabled_port_mask = l2fwd_parse_portmask(optarg);
            if (l2fwd_enabled_port_mask == 0)
            { // mask为0表示出错
                printf("invalid portmask\n");
                l2fwd_usage(prgname); //打印用户选项，类似于 --help
                return -1;            //参数传递错误，会返回-1，其结果是退出程序
            }
            break;

        /* nqueue */
        //队列的个数，同样也是用十六进制字符串表示的。
        case 'q':
            //每个核心配置几个 rx 队列。
            l2fwd_rx_queue_per_lcore = l2fwd_parse_nqueue(optarg);
            if (l2fwd_rx_queue_per_lcore == 0)
            { // lcore 0表示出错
                printf("invalid queue number\n");
                l2fwd_usage(prgname);
                return -1; //返回-1，其结果是退出程序。
            }
            break;

            /* timer period */ //定时器周期

        case 'T':
            //配置定时器周期，单位为秒
            //即l2fwd运行多少秒，使用十六进制表示的。
            timer_secs = l2fwd_parse_timer_period(optarg);
            if (timer_secs < 0)
            {
                printf("invalid timer period\n");
                l2fwd_usage(prgname);
                return -1;
            }
            // timer_period表示二层转发测试时间，默认为10秒，可通过-T来调节时间
            timer_period = timer_secs;
            break;

        case 'b':
            rsrc->process_byte = l2fwd_parse_process_byte(optarg);
            break;
        /*case CMD_LINE_OPT_PROCESS_BYTE_NUM:
            l2fwd_event_parse_type(optarg, rsrc);
            break;*/
        //--打头的选项不处理
        case 0:
            break;

        default:
            //参数传递错误，打印--help
            l2fwd_usage(prgname);
            return -1;
        }
    }

    if (optind >= 0)
        argv[optind - 1] = prgname;

    ret = optind - 1;
    optind = 0; /* reset getopt lib */
    return ret;
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void
check_all_ports_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90  /* 9s (90 * 100ms) in total */
    uint8_t portid, count, all_ports_up, print_flag = 0;
    struct rte_eth_link link;

    printf("\nChecking link status");
    fflush(stdout);
    for (count = 0; count <= MAX_CHECK_TIME; count++)
    {
        if (force_quit)
            return;
        all_ports_up = 1;
        for (portid = 0; portid < port_num; portid++)
        {
            if (force_quit)
                return;
            if ((port_mask & (1 << portid)) == 0)
                continue;
            memset(&link, 0, sizeof(link));
            rte_eth_link_get_nowait(portid, &link);
            /* print link status if flag set */
            if (print_flag == 1)
            {
                if (link.link_status)
                    printf("Port %d Link Up - speed %u "
                           "Mbps - %s\n",
                           (uint8_t)portid,
                           (unsigned)link.link_speed,
                           (link.link_duplex == ETH_LINK_FULL_DUPLEX) ? ("full-duplex") : ("half-duplex\n"));
                else
                    printf("Port %d Link Down\n",
                           (uint8_t)portid);
                continue;
            }
            /* clear all_ports_up flag if any link down */
            if (link.link_status == ETH_LINK_DOWN)
            {
                all_ports_up = 0;
                break;
            }
        }
        /* after finally printing all link status, get out */
        if (print_flag == 1)
            break;

        if (all_ports_up == 0)
        {
            printf(".");
            fflush(stdout);
            rte_delay_ms(CHECK_INTERVAL);
        }

        /* set the print_flag if all ports up or timeout */
        if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1))
        {
            print_flag = 1;
            printf("done\n");
        }
    }
}

static void
signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        printf("\n\nSignal %d received, preparing to exit...\n",
               signum);
        force_quit = true; //强制退出按钮为真
    }
}

int test_pdump_init(void)
{
    int ret = 0;

    ret = rte_pdump_init();
    if (ret < 0)
    {
        printf("rte_pdump_init failed\n");
        return -1;
    }
    /*ret = test_ring_setup(&ring_server, &portid);
    if (ret < 0) {
        printf("test_ring_setup failed\n");
        return -1;
    }*/
    printf("pdump_init success\n");
}

int check_Udpbroad(struct rte_ipv4_hdr *iphd)
{
    if (iphd->dst_addr == htonl(0xffffffff))
        return 1;
    else
        return 0;
}

#if UDP_CHECK_OFF
uint16_t udpHeader_nocheck(struct rte_udp_hdr *udp,u_int32_t differ,u_int32_t flag)
{
    uint32_t result = 0;
    result = ((~udp->dgram_cksum)<<16) & 0xffff0000;
    if(flag == 0) result = htonl((ntohl)(result)+(ntohl)(differ));
    else {result = htonl((ntohl)(result)-(ntohl)(differ));}
    result = result / 0x10000 + result % 0x10000;
    uint16_t sum = result % 0x10000;
    return ~sum;
}
#endif

static void
capture_loop(struct rte_mbuf *pkts_free, unsigned portid, struct l2fwd_resources *rsrc) // Linx
{
    unsigned int i;
    uint8_t *d_iph = pkts_free->buf_addr + pkts_free->data_off;
//    struct rte_udp_hdr *udp = &d_iph[34];
    struct rte_ipv6_hdr *iph = &d_iph[4];
    struct rte_ether_hdr *eth_hdr = &d_iph[0];
    Route_down_nip=0;

    /**If a endpoint node*/
    if(iph->proto == IPPROTO_HOPOPTS /* && udp->dst_port == htons(28891) */ && iph->dst_addr == (htonl)(SELF_IP[portid]))
    {
        //uint64_t Pbegin_time = rte_rdtsc();
        //rte_ether_addr_copy(&eth_hdr->s_addr, &eth_hdr->d_addr);
        rte_ether_addr_copy(&l2fwd_ports_eth_addr[portid], &eth_hdr->s_addr);
        #if UDP_CHECK_OFF 
        u_int32_t Udp_Differ=0,add_flag=0;
        struct hash_route_array *Udp_Ipaddr = &d_iph[30];
        u_int32_t Udp_Addr_Pre=Udp_Ipaddr->ip;;
        #endif

        if(portid == 0)
        {
            struct rte_ipv6_hdr *dst_iph = &d_iph[60];
            iph->dst_addr = dst_iph->dst_addr;
            hash_down_route.ip = dst_iph->dst_addr;
            int ret = rte_hash_lookup(Down_htable, (void *)&hash_down_route);
            if (ret < 0) {
                hash_down_route.ip = (dst_iph->dst_addr) & ((htonl)((u_int32_t)0xffffff00));
                ret = rte_hash_lookup(Down_htable, (void *)&hash_down_route);
                if (ret < 0) {
                printf("rte_hash_down_lookup key failed!\n");}
                else{
                    printf("rte_hash_down_lookup key success! ret:%d\n", ret);
                    Route_down_nip = ipv4_l3fwd_down_out_if[ret];
                }
            } else {
                printf("rte_hash_down_lookup key success! ret:%d\n", ret);
                Route_down_nip = ipv4_l3fwd_down_out_if[ret];
                #if UDP_CHECK_OFF
                u_int32_t Udp_Addr=Udp_Ipaddr->ip;
                if(Udp_Addr > Udp_Addr_Pre) Udp_Differ = Udp_Addr - Udp_Addr_Pre;
                else {Udp_Differ = Udp_Addr_Pre - Udp_Addr;
                add_flag = 1;}
                //Udp_Differ = (int32_t)(Udp_Addr - Udp_Addr_Pre);
                udp->dgram_cksum = udpHeader_nocheck(udp, Udp_Differ,add_flag);
                #endif
                #if UDP_CHECK_ON
                pheader.src = iph->src_addr;
                pheader.dst = iph->dst_addr;
                pheader.mbz = 0;
                pheader.len = udp->dgram_len;
                pheader.p = 17;
                udp->dgram_cksum = udpHeader(udp, (ntohs)(udp->dgram_len), &pheader);
                #endif
            }
        }

        else if(portid == 1)
        {
            struct rte_ipv4_hdr *dst_iph = &d_iph[60];
            
            //#endif
            //hash_route.ip = dst_iph->dst_addr;
            /* 查找路由 */
            /*int ret = rte_hash_lookup(htable, (void *)&hash_route);
            if (ret < 0) {*/
            hash_route.ip = dst_iph->dst_addr & ((htonl)((u_int32_t)0xff000000));
            int ret = rte_hash_lookup(htable, (void *)&hash_route);
            if (ret < 0) {
            printf("rte_hash_lookup key failed!\n");}
            else{
                printf("rte_hash_lookup key success! ret:%d\n", ret);
                iph->dst_addr = ipv4_l3fwd_out_if[ret];
                #if UDP_CHECK_OFF
                u_int32_t Udp_Addr=Udp_Ipaddr->ip;
                if(Udp_Addr > Udp_Addr_Pre) Udp_Differ = Udp_Addr - Udp_Addr_Pre;
                else {Udp_Differ = Udp_Addr_Pre - Udp_Addr;
                add_flag = 1;}
                udp->dgram_cksum = udpHeader_nocheck(udp, Udp_Differ,add_flag);
                #endif
                #if UDP_CHECK_ON
                pheader.src = iph->src_addr;
                pheader.dst = iph->dst_addr;
                pheader.mbz = 0;
                pheader.len = udp->dgram_len;
                pheader.p = 17;
                udp->dgram_cksum = udpHeader(udp, (ntohs)(udp->dgram_len), &pheader);
                #endif
            }
            /*} else {
                printf("rte_hash_lookup key success! ret:%d\n", ret);
                iph->dst_addr = ipv4_l3fwd_out_if[ret];
                #if ARP_ENTRY_STATUS_STATIC*/
                /*for (int i = 0; i < IPV4_L3FWD_NUM_ARP; i++)
                {
                    if(iph->dst_addr == (htonl)(ipv4_l3fwd_arp_array[i].host_ip))
                    {
                        rte_ether_addr_copy(&ipv4_l3fwd_arp_array[i].addr, &eth_hdr->d_addr);
                        printf("search sucess in %d!\n",i);
                    }
                }*/
               /*
                arp_array.ip = iph->dst_addr;
                ret = rte_hash_lookup(Arp_htable, (void *)&arp_array);
                if (ret < 0) {;}
                else{
                    rte_ether_addr_copy(&Addr_Arp_out_if[ret], &eth_hdr->d_addr);
                }
                #endif
                #if ARP_ENTRY_STATUS_DYNAMIC
                if(get_dst_macaddr(iph->dst_addr) != NULL) rte_memcpy(eth_hdr->d_addr.addr_bytes, get_dst_macaddr(iph->dst_addr), RTE_ETHER_ADDR_LEN);
                else{
                    if(iph->dst_addr == htonl(0xc0a80a65)) rte_memcpy(eth_hdr->d_addr.addr_bytes, My_mac1, RTE_ETHER_ADDR_LEN);
                    if(iph->dst_addr == htonl(0xc0a80a66)) rte_memcpy(eth_hdr->d_addr.addr_bytes, My_mac2, RTE_ETHER_ADDR_LEN);
                }
                #endif
            }    */
        }
        //uint64_t Pend_time = rte_rdtsc();
        //printf("total time use %.5fms\n",(float)(Pend_time-Pbegin_time)/rte_get_timer_hz()*1000);
    }
    /**If a transit node*/
    else if(iph->proto == IPPROTO_HOPOPTS/*  && udp->dst_port == htons(60000) */ && iph->dst_addr !== (htonl)(SELF_IP[portid]))
    {
        /* 如果是中转节点，仅处理ipv6头，进行下一跳的转发，修改src_dst */
        struct RouteArray_message *Route_mess = &d_iph[42];
        
        switch (ntohl(Route_mess->Order_type))
        {
        case route_receive:
		    printf("receive route pkt in port%d!\n",portid);
            R_receive(pkts_free, Route_mess->Data_length,portid);
            break;
        case route_cut:
            R_delete(pkts_free, Route_mess->Data_length,portid);
            break;
        case route_inquiry:
            R_inquiry(pkts_free, Route_mess->Data_length,portid);
            break;
        default:
            break;
        }
    }
}

/*
 *计算ip首部校验和
 */
uint16_t ipHeader(struct rte_ipv4_hdr *ip)
{
    uint16_t *tmp = ip;
    tmp[5] = 0;
    int i = 0;
    uint32_t result = 0;
    for (i = 0; i < 10; i++)
    {
        result += tmp[i];
    }
    if (result > 0x10000)
    {
        result = result / 0x10000 + result % 0x10000;
    }
    uint16_t sum = result % 0x10000;
    return ~sum;
}

uint16_t tcpHeader(struct rte_tcp_hdr *ip, int size, struct psd *pheader)
{
    int len = size + sizeof(struct psd);
    int flag = 0;
    if (len % 2 == 1)
    {
        len = len + 1;
        flag = 1;
    }
    uint8_t buf[len];
    bzero(buf, len);
    ip->cksum = 0;
    pheader->p = 6;
    memcpy(buf, pheader, sizeof(struct psd));
    memcpy(buf + sizeof(struct psd), ip, size);
    if (flag == 1)
    {
        buf[len - 1] = buf[len - 2];
        buf[len - 2] = 0;
    }
    return checksum((unsigned short *)buf, len / 2);
}

uint16_t udpHeader(struct rte_udp_hdr *udp, int size, struct psd *pheader)
{
    int len = size + sizeof(struct psd);
    // int flag = 0;
    if (len % 2 == 1)
    {
        len = len + 1;
        // flag = 1;
    }
    uint8_t buf[len];
    bzero(buf, len);
    udp->dgram_cksum = 0;
    pheader->p = 17;
    memcpy(buf, pheader, sizeof(struct psd));
    memcpy(buf + sizeof(struct psd), udp, size);
    /*if (flag == 1)
    {
        buf[len - 1] = buf[len - 2];
        buf[len - 2] = 0;
    }*/
    // for(i=0;i<24;i++) printf("-----------%d----------\n",buf[i]);
    return checksum((unsigned short *)buf, len / 2);
}
#if UDP_CHECK_ON
uint16_t udpHeader(struct rte_udp_hdr *udp, int size, struct psd *pheader)
{
    int len = size + sizeof(struct psd);
    // int flag = 0;
    if (len % 2 == 1)
    {
        len = len + 1;
        // flag = 1;
    }
    uint8_t buf[len];
    bzero(buf, len);
    udp->dgram_cksum = 0;
    pheader->p = 17;
    memcpy(buf, pheader, sizeof(struct psd));
    memcpy(buf + sizeof(struct psd), udp, size);
    /*if (flag == 1)
    {
        buf[len - 1] = buf[len - 2];
        buf[len - 2] = 0;
    }*/
    // for(i=0;i<24;i++) printf("-----------%d----------\n",buf[i]);
    return checksum((unsigned short *)buf, len / 2);
}
#endif

unsigned short inline checksum(unsigned short *buffer, unsigned short size)
{
    int i = 0;
    uint32_t result = 0;
    for (i = 0; i < size; i++)
    {
        result += buffer[i];
    }
    result = result / 0x10000 + result % 0x10000;
    uint16_t sum = result % 0x10000;
    return ~sum;
}
/*
static void
init_ring(void)
{
    ring_clone_size = NB_MBUF;
    ring_clone = rte_ring_create(
        "ring_clone", ring_clone_size, rte_socket_id(),
        RING_F_SP_ENQ | RING_F_SC_DEQ);

    ring_res_size = NB_MBUF;
    ring_res = rte_ring_create(
        "ring_res", ring_res_size, rte_socket_id(),
        RING_F_SP_ENQ | RING_F_SC_DEQ);
    ring_private_size = NB_MBUF;
    ring_private = rte_ring_create(
        "ring_private", ring_private_size, rte_socket_id(),
        RING_F_SP_ENQ | RING_F_SC_DEQ);
}*/

int main(int argc, char **argv)
{
    struct lcore_queue_conf *qconf;
    struct l2fwd_resources *rsrc;

    int ret;                          //返回值
    uint8_t nb_ports;                 //总port个数
    uint8_t nb_ports_available;       //可用port个数
    uint8_t portid, last_port;        //当前portid，前一个portid。
    unsigned lcore_id, rx_lcore_id;   //核心id，rx_lcore_id
    unsigned nb_ports_in_mask = 0;
    unsigned int nb_mbufs;
    unsigned int nb_lcores = 0;

    /* init EAL */
    //初始化环境
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    argc -= ret;
    argv += ret;

    rsrc = l2fwd_get_rsrc();
    /* 初始化pdump */
    test_pdump_init();

    force_quit = false; //强制推出按钮为假
    //当捕获到SIGINT或SIGTERM信号时，强制推出按钮为真，详见signal_handler
    // signal_handler为自定义函数
    signal(SIGINT, signal_handler);  //信号捕获处理1
    signal(SIGTERM, signal_handler); //信号捕获处理2

    /* parse application arguments (after the EAL ones) */
    //解析传参（必须在eal_init之后），此为自定义函数，详见函数定义
    ret = l2fwd_parse_args(argc, argv, rsrc);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid L2FWD arguments\n");

    printf("MAC updating %s\n", mac_updating ? "enabled" : "disabled");
    // printf("----------------%d-----------\n",rsrc->process_type);

    /* convert to number of cycles */
    //获取定时器的时钟周期
    timer_period *= rte_get_timer_hz();
    nb_ports = rte_eth_dev_count_avail();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	if (port_pair_params != NULL) {
		if (check_port_pair_config() < 0)
			rte_exit(EXIT_FAILURE, "Invalid port pair config\n");
	}

	/* check port mask to possible port mask */
	if (l2fwd_enabled_port_mask & ~((1 << nb_ports) - 1))
		rte_exit(EXIT_FAILURE, "Invalid portmask; possible (0x%x)\n",
			(1 << nb_ports) - 1);

	/* Initialization of the driver. 8< */

	/* reset l2fwd_dst_ports */
	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
		l2fwd_dst_ports[portid] = 0;
	last_port = 0;

    for (portid = 0; portid < NUM_PORTS; portid++)
		Arp_dst_ports[portid] = 0;

	/* populate destination port details */
	if (port_pair_params != NULL) {
		uint16_t idx, p;

		for (idx = 0; idx < (nb_port_pair_params << 1); idx++) {
			p = idx & 1;
			portid = port_pair_params[idx >> 1].port[p];
			l2fwd_dst_ports[portid] =
				port_pair_params[idx >> 1].port[p ^ 1];
		}
	} else {
		RTE_ETH_FOREACH_DEV(portid) {
			/* skip ports that are not enabled */
			if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
				continue;

			if (nb_ports_in_mask % 2) {
				l2fwd_dst_ports[portid] = last_port;
				l2fwd_dst_ports[last_port] = portid;
			} else {
				last_port = portid;
			}

			nb_ports_in_mask++;
		}
		if (nb_ports_in_mask % 2) {
			printf("Notice: odd number of ports in portmask.\n");
			l2fwd_dst_ports[last_port] = last_port;
		}
	}
	/* >8 End of initialization of the driver. */

	rx_lcore_id = 0;
	qconf = NULL;

	/* Initialize the port/queue configuration of each logical core */
	RTE_ETH_FOREACH_DEV(portid) {
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;

		/* get the lcore_id for this port */
		while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
		       lcore_queue_conf[rx_lcore_id].n_rx_port ==
		       l2fwd_rx_queue_per_lcore) {
			rx_lcore_id++;
			if (rx_lcore_id >= RTE_MAX_LCORE)
				rte_exit(EXIT_FAILURE, "Not enough cores\n");
		}

		if (qconf != &lcore_queue_conf[rx_lcore_id]) {
			/* Assigned a new logical core in the loop above. */
			qconf = &lcore_queue_conf[rx_lcore_id];
			nb_lcores++;
		}

		qconf->rx_port_list[qconf->n_rx_port] = portid;
		qconf->n_rx_port++;
		printf("Lcore %u: RX port %u TX port %u\n", rx_lcore_id,
		       portid, l2fwd_dst_ports[portid]);
	}

    nb_mbufs = RTE_MAX(nb_ports * (nb_rxd + nb_txd + MAX_PKT_BURST +
		nb_lcores * MEMPOOL_CACHE_SIZE), 8192U);
    /* create the mbuf pool */
    //创建缓存池，用于存储数据包。
    l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", nb_mbufs,
                                                 MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                                                 rte_socket_id());
    if (l2fwd_pktmbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

    pool_clone = rte_pktmbuf_pool_create("pool_clone", nb_mbufs << 3,
                                         MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                                         rte_socket_id());
    if (pool_clone == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init clone pool\n");

    pool_arp = rte_pktmbuf_pool_create("pool_arp", nb_mbufs, 32,
		0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (pool_arp == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    //init_ring();

    /* Initialise each port */
	RTE_ETH_FOREACH_DEV(portid) {
		struct rte_eth_rxconf rxq_conf;
		struct rte_eth_txconf txq_conf;
		struct rte_eth_conf local_port_conf = port_conf;
		struct rte_eth_dev_info dev_info;

		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0) {
			printf("Skipping disabled port %u\n", portid);
			continue;
		}
		nb_ports_available++;

		/* init port */
		printf("Initializing port %u... ", portid);
		fflush(stdout);

		ret = rte_eth_dev_info_get(portid, &dev_info);
		if (ret != 0)
			rte_exit(EXIT_FAILURE,
				"Error during getting device (port %u) info: %s\n",
				portid, strerror(-ret));

		if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
			local_port_conf.txmode.offloads |=
				DEV_TX_OFFLOAD_MBUF_FAST_FREE;
		/* Configure the number of queues for a port. */
		ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
				  ret, portid);
		/* >8 End of configuration of the number of queues for a port. */

		ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,
						       &nb_txd);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot adjust number of descriptors: err=%d, port=%u\n",
				 ret, portid);

		ret = rte_eth_macaddr_get(portid,
					  &l2fwd_ports_eth_addr[portid]);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot get MAC address: err=%d, port=%u\n",
				 ret, portid);

		/* init one RX queue */
		fflush(stdout);
		rxq_conf = dev_info.default_rxconf;
		rxq_conf.offloads = local_port_conf.rxmode.offloads;
		/* RX queue setup. 8< */
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
					     rte_eth_dev_socket_id(portid),
					     &rxq_conf,
					     l2fwd_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				  ret, portid);
		/* >8 End of RX queue setup. */

		/* Init one TX queue on each port. 8< */
		fflush(stdout);
		txq_conf = dev_info.default_txconf;
		txq_conf.offloads = local_port_conf.txmode.offloads;
		ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
				rte_eth_dev_socket_id(portid),
				&txq_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
				ret, portid);
		/* >8 End of init one TX queue on each port. */

		/* Initialize TX buffers */
		tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
				RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
				rte_eth_dev_socket_id(portid));
		if (tx_buffer[portid] == NULL)
			rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
					portid);

		rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

		ret = rte_eth_tx_buffer_set_err_callback(tx_buffer[portid],
				rte_eth_tx_buffer_count_callback,
				&port_statistics[portid].dropped);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
			"Cannot set error callback for tx buffer on port %u\n",
				 portid);

		ret = rte_eth_dev_set_ptypes(portid, RTE_PTYPE_UNKNOWN, NULL,
					     0);
		if (ret < 0)
			printf("Port %u, Failed to disable Ptype parsing\n",
					portid);
        /* Start device */
        //启动设备
        ret = rte_eth_dev_start(portid);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
                     ret, (unsigned)portid);

        printf("done: \n");

        //设置网卡为混杂模式
        ret = rte_eth_promiscuous_enable(portid);
        if (ret != 0)
            rte_exit(EXIT_FAILURE,
                     "rte_eth_promiscuous_enable:err=%s, port=%u\n",
                     rte_strerror(-ret), portid);

        //打印port的MAC地址
        printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
               (unsigned)portid,
               l2fwd_ports_eth_addr[portid].addr_bytes[0],
               l2fwd_ports_eth_addr[portid].addr_bytes[1],
               l2fwd_ports_eth_addr[portid].addr_bytes[2],
               l2fwd_ports_eth_addr[portid].addr_bytes[3],
               l2fwd_ports_eth_addr[portid].addr_bytes[4],
               l2fwd_ports_eth_addr[portid].addr_bytes[5]);

        /* initialize port stats */
        //初始化port属性
        memset(&port_statistics, 0, sizeof(port_statistics));
    }

    //如果可以使能的网卡为0个，则报错。
    if (!nb_ports_available)
    {
        rte_exit(EXIT_FAILURE,
                 "All available ports are disabled. Please set portmask.\n");
    }

    //检查所有port的link属性
    check_all_ports_link_status(nb_ports, l2fwd_enabled_port_mask);

    output_file = fopen("./output", "w");
    rte_openlog_stream(output_file);
    ret = 0;
    /* launch per-lcore init on every lcore */
    //启动所有的lcore，回调l2fwd_launch_one_lcore函数，参数为NULL
    rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, rsrc, CALL_MASTER);
    //遍历所有lcore_id
    RTE_LCORE_FOREACH_SLAVE(lcore_id)
    {
        //等所有线程结束
        if (rte_eal_wait_lcore(lcore_id) < 0)
        {
            ret = -1;
            break;
        }
    }

    //停止并且关闭所有的port
   RTE_ETH_FOREACH_DEV(portid) {
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("Closing port %d...", portid);
		ret = rte_eth_dev_stop(portid);
		if (ret != 0)
			printf("rte_eth_dev_stop: err=%d, port=%d\n",
			       ret, portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
    // close(sockfd);
    printf("Bye...\n");

    return ret;
}
