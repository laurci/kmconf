#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "init.h"

#define PACKET_TYPE_TEST 0x00
#define PACKET_READ_REQUEST 0x01
#define PACKET_READ_RESPONSE 0x02

struct packet_header {
    u8 type;
};

struct test_packet {
    u32 a;
    u32 b;
    u32 c;
};

struct read_request_packet {
    u32 req_id;
    u32 address;
};

struct read_response_packet {
    u32 req_id;
    u32 address;
    u32 length;
};

#endif
