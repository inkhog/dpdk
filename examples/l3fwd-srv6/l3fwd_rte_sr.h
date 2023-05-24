#ifndef _RTE_SR_H_
#define _RTE_SR_H_

/**
 * @file
 *
 * IP-related defines
 */

#include <stdint.h>

#ifdef RTE_EXEC_ENV_WINDOWS
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#endif

#include <rte_byteorder.h>
#include <rte_mbuf.h>
#include <rte_ip.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Segment Routing Header
 */
struct rte_srh {
	uint8_t  proto;		/**< Protocol, next header. */
	uint8_t  ext_len;	/**< Extend length of SRH.*/
	uint8_t  r_type;	/**< Routing type. */
	uint8_t  seg_left;	/**< Segments left.*/
	uint8_t  last_entry;	/**< Last entry.*/
	uint8_t  flags;	/**< Flags.*/
	uint16_t  tag;	/**Tag.*/
// struct seg_list *seg_list;
} __rte_packed;

enum segment_type{
	END,
#define SID_END END
	END_X,
#define SID_END_X END_X
	END_DT4,
#define SID_END_DT4 END_DT4
	END_DT6,
#define SID_END_DT6 END_DT6
	END_DX4,
#define SID_END_DX4 END_DX4
	END_DX6,
#define SID_END_DX6 END_DX6
	END_DX2,
#define SID_END_DX2 END_DX2
	END_DX2V,
	// 其他Segment Type
};

struct rte_seg{
	uint8_t ip[16];
};

uint8_t get_segment_type(struct rte_seg *seg_list){
	return seg_list->ip[0];
}

uint8_t get_segment_length(struct rte_seg *seg_list){
	return seg_list->ip[1];
}

//缺陷代码 test only
void srv6_parse_packet(struct rte_ipv6_hdr *ipv6_hdr) {
	struct rte_srh *srh_hdr;
	struct rte_seg *seg_list;

  // 获取Segment List的起始位置
  srh_hdr = (struct rte_srh *)(ipv6_hdr + 1);
  seg_list = (struct rte_seg *)(srh_hdr + 1);
  
  // 遍历Segment List
  for(uint8_t i = 0; i < srh_hdr->seg_left; i++) {
    uint8_t segment_type = get_segment_type(seg_list);
    uint8_t segment_length = get_segment_length(seg_list);
    uint8_t *segment_data = seg_list + 2;
    
    // 根据Segment Type执行相应的操作
    switch (segment_type) {
      case END:
        process_segment_type_x(segment_data, segment_length);
        break;
      case END_DT4:
        process_segment_type_y(segment_data, segment_length);
        break;
      // 其他Segment Type的处理逻辑
      default:
        // 处理未知的Segment Type
        break;
    }
    
    // 更新Segment List指针
    seg_list += segment_length;
  }
  
  // 最后一个Segment的处理逻辑
  uint8_t segment_type = get_segment_type(seg_list);
  uint8_t segment_length = get_segment_length(seg_list);
  uint8_t *segment_data = seg_list + 2;
  
  // 根据最后一个Segment Type执行相应的操作
  switch (segment_type) {
    case SEGMENT_TYPE_X:
      process_segment_type_x(segment_data, segment_length);
      break;
    case SEGMENT_TYPE_Y:
      process_segment_type_y(segment_data, segment_length);
      break;
    // 其他Segment Type的处理逻辑
    default:
      // 处理未知的Segment Type
      break;
  }
}


static inline void* create_rte_srh(struct rte_ipv6_hdr ipv6_hdr,struct rte_srh srh_ext,int seg_count,void *seg_list){
	void *ipv6_ful_hdr=malloc(sizeof(ipv6_hdr)+sizeof(srh_ext)+sizeof(uint8_t)*16*seg_count);//40+8+4*n Bytes
	memcpy(ipv6_ful_hdr,&ipv6_hdr,sizeof(struct rte_ipv6_hdr));//40 Bytes
	memcpy(ipv6_ful_hdr+sizeof(struct rte_ipv6_hdr),&srh_ext,sizeof(struct rte_srh));//8 Bytes
	memcpy(ipv6_ful_hdr+sizeof(struct rte_ipv6_hdr)+sizeof(struct rte_srh),seg_list,sizeof(uint8_t)*16*seg_count);
	return ipv6_ful_hdr;
};

enum {
	SRH,
	
};

static inline int rte_srv6_get_next_ext(const uint8_t *p, int proto, size_t *ext_len)
{
	int next_proto;

	switch (proto) {
	case IPPROTO_HOPOPTS://IPv6 Routing, SRv6 need this
	
	case IPPROTO_DCCP://IPv6 header

	case IPPROTO_AH:
		next_proto = *p++;
		*ext_len = (*p + 2) * sizeof(uint32_t);
		break;

	case IPPROTO_ROUTING:
	case IPPROTO_DSTOPTS:
		next_proto = *p++;
		*ext_len = (*p + 1) * sizeof(uint64_t);
		break;

	case IPPROTO_FRAGMENT://IPv6 ICMP
		next_proto = *p;
		*ext_len = RTE_IPV6_FRAG_HDR_SIZE;
		break;

	default:
		return -EINVAL;
	}

	return next_proto;
}





/** SRH Segment extension header. */
struct rte_srh_segment_ext {
	uint8_t next_header;	/**< Next header type */
	uint8_t reserved;	/**< Reserved */
	rte_be16_t frag_data;	/**< All fragmentation data */
	rte_be32_t id;		/**< Packet ID */
} __rte_packed;

/* IPv6 fragment extension header size */
#define RTE_SRV6_SEG_HDR_SIZE	sizeof(struct rte_srh_segment_ext)

/**
 * Parse next IPv6 header extension
 *
 * This function checks if proto number is an IPv6 extensions and parses its
 * data if so, providing information on next header and extension length.
 *
 * @param p
 *   Pointer to an extension raw data.
 * @param proto
 *   Protocol number extracted from the "next header" field from
 *   the IPv6 header or the previous extension.
 * @param ext_len
 *   Extension data length.
 * @return
 *   next protocol number if proto is an IPv6 extension, -EINVAL otherwise
 */
__rte_experimental
static inline int
rte_ipv6_get_next_ext(const uint8_t *p, int proto, size_t *ext_len)
{
	int next_proto;

	switch (proto) {
	case IPPROTO_AH:
		next_proto = *p++;
		*ext_len = (*p + 2) * sizeof(uint32_t);
		break;

	case IPPROTO_HOPOPTS:
	case IPPROTO_ROUTING:
	case IPPROTO_DSTOPTS:
		next_proto = *p++;
		*ext_len = (*p + 1) * sizeof(uint64_t);
		break;

	case IPPROTO_FRAGMENT:
		next_proto = *p;
		*ext_len = RTE_IPV6_FRAG_HDR_SIZE;
		break;

	default:
		return -EINVAL;
	}

	return next_proto;
}

#ifdef __cplusplus
}
#endif

#endif /* _RTE_SR_H_ */
