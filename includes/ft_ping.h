#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <argp.h>

#pragma pack(push, 1)
typedef struct packet_t {
    struct icmphdr header;
    char msg[48];
} packet_s;
#pragma pack(pop)

#define TTL_VALUE 64