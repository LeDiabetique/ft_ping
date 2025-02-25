#include "ft_ping.h"

struct arguments
{
    char *args[2];
    int verbose, help;
};

static void ft_ping(char *argv, struct arguments arguments);
static char *resolve_dns(char *hostname, struct sockaddr_in *addr_con);
static void send_ping(char *ip, char *hostname, struct sockaddr_in *addr, int sockfd, int verbose);
static unsigned short calculate_checksum(void *b, int len);
static error_t parse_opt(int key, char *arg, struct argp_state *state);
static void icmp_error_handler(struct iphdr *ip_header, struct icmphdr *receiver_header, int verbose);

static bool stop = false;

const char *argp_program_bug_address =
    "<hdiot.student@42lyon.fr>";

static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Produce verbose output", 0},
    {"help", '?', 0, 0, "Display ping options", 0},
    {0}};

static char args_doc[] = "HOST ...";
static char doc[] = "Send ICMP ECHO_REQUEST packets to network hosts.";

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, NULL};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{

    struct arguments *arguments = state->input;
    switch (key)
    {
    case ARGP_KEY_ARG:
        if (state->arg_num >= 2)
        {
            argp_usage(state);
        }
        arguments->args[state->arg_num] = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < 1)
        {
            argp_usage(state);
        }
        break;
    case 'v':
        arguments->verbose = 1;
        if (state->arg_num == 2)
        {
            argp_usage(state);
        }
        break;
    case '?':
        arguments->help = 1;
        argp_help(&argp, stdout, ARGP_HELP_STD_HELP, "ft_ping.c");
        exit(0);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

int main(int ac, char **av)
{
    struct arguments arguments;
    arguments.verbose = 0;

    argp_parse(&argp, ac, av, 0, 0, &arguments);

    ft_ping(arguments.args[0], arguments);
    return 0;
}

static void sighandler()
{
    stop = true;
}

static void ft_ping(char *argv, struct arguments arguments)
{

    int sockfd;
    char *ip_addr;
    struct sockaddr_in addr_con;

    ip_addr = resolve_dns(argv, &addr_con);
    if (ip_addr == NULL)
    {
        return;
    }

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        printf("Fail to create socket fd %d\n", sockfd);
        return;
    }

    signal(SIGINT, sighandler);

    send_ping(ip_addr, argv, &addr_con, sockfd, arguments.verbose);
}

static char *resolve_dns(char *hostname, struct sockaddr_in *addr_con)
{
    char *ip = malloc(NI_MAXHOST * sizeof(char));
    struct hostent *host = gethostbyname(hostname);
    if (host == NULL)
    {
        printf("ping: unknown host\n");
        return NULL;
    }
    if (inet_pton(AF_INET, inet_ntoa(*(struct in_addr *)host->h_addr), &(addr_con->sin_addr.s_addr)) <= 0)
    {
        return NULL;
    }
    strcpy(ip, inet_ntoa(*(struct in_addr *)host->h_addr));

    addr_con->sin_family = host->h_addrtype;
    addr_con->sin_port = htons(0);
    return ip;
}

static unsigned short calculate_checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

static void send_ping(char *ip, char *hostname,
                      struct sockaddr_in *s_addr, int sockfd, int verbose)
{

    struct sockaddr_in *r_addr;

    struct timeval time_start, time_end = {0};
    struct timeval t_out;
    struct packet_t packet;
    socklen_t addr_len = 0;
    int ttl = TTL_VALUE, i = 0, count = 0, send_error = 0;
    char receive_buffer[128];
    int send_count = 0, receive_count = 0;
    long double min = 0, max = 0, avg = 0, mdev = 0, last_timing;
    t_out.tv_sec = 1;
    t_out.tv_usec = 0;

    if (setsockopt(sockfd, SOL_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
    {
        printf("Fail to set ttl socket\n");
        return;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t_out, sizeof(t_out)) < 0)
    {
        printf("Fail to set socket timeout\n");
        return;
    }
    uint16_t id = htons(getpid());
    if (verbose == 1)
    {
        printf("PING %s (%s): %ld data bytes, id 0x%x = %u\n", hostname, ip, sizeof(packet)- sizeof(struct icmphdr), id, id);
    }
    else
    {
        printf("PING %s (%s): %ld data bytes\n", hostname, ip, sizeof(packet));
    }
    while (!stop)
    {
        bzero(&packet, sizeof(packet));
        bzero(&r_addr, sizeof(r_addr));
        bzero(receive_buffer, sizeof(receive_buffer));
        packet.header.type = ICMP_ECHO;
        packet.header.un.echo.id = id;
        packet.header.code = 0;

        for (i = 0; i < (int)sizeof(packet.msg); i++)
        {
            packet.msg[i] = i + '0';
        }
        packet.msg[i] = 0;
        packet.header.un.echo.sequence = __bswap_16(count++);
        gettimeofday(&time_start, NULL);
        uint32_t sec = htonl(time_start.tv_sec);
        uint32_t usec = htonl(time_start.tv_usec);
        
        memcpy(packet.timestamp, &sec, sizeof(sec));  // Copier la partie secondes
        memcpy(packet.timestamp + 4, &usec, sizeof(usec)); 
        packet.header.checksum = calculate_checksum(&packet, sizeof(packet));

        int send_len = sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)s_addr, sizeof(*s_addr));
        if (send_len <= 0)
        {
            send_error = 1;
        }
        else
        {
            send_count++;
        }
        addr_len = sizeof(r_addr);

        int recv_len = recvfrom(sockfd, receive_buffer, sizeof(receive_buffer), 0, (struct sockaddr *)r_addr, &addr_len);
        gettimeofday(&time_end, NULL);
        long double timing = ((double)time_end.tv_usec - (double)time_start.tv_usec) / 1000;
        if (timing < 0)
        {
            timing += 1000;
        }
        if (recv_len > 0)
        {
            struct iphdr *ip_header = (struct iphdr *)receive_buffer;
            struct iphdr *ip_info = (struct iphdr *)(receive_buffer + 28);
            struct icmphdr *receiver_header = (struct icmphdr *)(receive_buffer + (ip_header->ihl * 4));

            if (!send_error)
            {
                if (receiver_header->type == 0 && receiver_header->code == 0)
                {
                    receive_count++;
                    if (timing < min || min == 0)
                    {
                        min = timing;
                    }
                    if (timing > max || max == 0)
                    {
                        max = timing;
                    }
                    avg += timing;
                    if (receive_count > 1)
                    {
                        long double calcul_mdev = last_timing - timing;
                        if (calcul_mdev < 0)
                        {
                            calcul_mdev *= -1;
                        }
                        mdev += calcul_mdev;
                    }
                    last_timing = timing;
                    printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.3Lf ms\n", sizeof(receive_buffer) / 2, ip, __bswap_16(receiver_header->un.echo.sequence), ip_header->ttl, timing);
                }
                else
                {
                    icmp_error_handler(ip_info, receiver_header, verbose);
                    printf("ICMP: type %d, code %d, size %zu, id 0x%x, seq 0x%x\n", packet.header.type, packet.header.code, sizeof(packet), packet.header.un.echo.id, packet.header.un.echo.sequence);
                }
            }
        }
        long double sleep_timing = ((double)time_end.tv_usec - (double)time_start.tv_usec) / 1000;
        if (sleep_timing < 0)
        {
            sleep_timing += 1000;
        }
        long double sleep_time = 1000 - sleep_timing;
        usleep(sleep_time * 1000);
    }
    printf("--- %s ping statistics ---\n", hostname);
    printf("%d packets transmitted, %d packets received,", send_count, receive_count);
    printf(" %d%% packet loss\n", 100 - (receive_count / send_count * 100));
    if (receive_count > 0)
    {
        mdev = mdev / receive_count;
        avg = avg / receive_count;
        printf("round-trip min/avg/max/stddev = %.3Lf/%.3Lf/%.3Lf/%.3Lf ms\n", min, avg, max, mdev);
    }
}

static void icmp_error_handler(struct iphdr *ip_header, struct icmphdr *receiver_header, int verbose)
{
    printf("%d bytes from _gateway (%s): ", 36, inet_ntoa(*(struct in_addr *)&ip_header->saddr));
    if (receiver_header->type == ICMP_TIME_EXCEEDED)
    {
        switch (receiver_header->code)
        {
        case ICMP_EXC_TTL:
            printf("Time to live exceeded\n");
            break;
        case ICMP_EXC_FRAGTIME:
            printf("Fragment Reassembly Time Exceeded\n");
            break;
        default:
            printf("Time to live exceeded\n");
            break;
        }
    }
    else if (receiver_header->type == ICMP_DEST_UNREACH)
    {
        switch (receiver_header->code)
        {
        case ICMP_NET_UNREACH:
            printf("Network unreachable\n");
            break;
        case ICMP_HOST_UNREACH:
            printf("Host unreachable\n");
            break;
        case ICMP_PROT_UNREACH:
            printf("Protocol unreachable\n");
            break;
        case ICMP_PORT_UNREACH:
            printf("Port unreachable\n");
            break;
        case ICMP_FRAG_NEEDED:
            printf("Fragmentation Needed and Don't Fragment was Set\n");
            break;
        case ICMP_SR_FAILED:
            printf("Source Route Failed\n");
            break;
        case ICMP_NET_UNKNOWN:
            printf("Destination Network Unknown\n");
            break;
        case ICMP_HOST_UNKNOWN:
            printf("Destination Host Unknown\n");
            break;
        case ICMP_NET_UNR_TOS:
            printf("Destination Network Unreachable for Type of Service\n");
            break;
        case ICMP_HOST_UNR_TOS:
            printf("Destination Host Unreachable for Type of Service\n");
            break;
        case ICMP_PKT_FILTERED:
            printf("Communication Administratively Prohibited\n");
            break;
        case ICMP_PREC_VIOLATION:
            printf("Host Precedence Violation\n");
            break;
        case ICMP_PREC_CUTOFF:
            printf("Precedence cutoff in effect\n");
            break;
        default:
            printf("Destination unreachable\n");
            break;
        }
    }
    else if (receiver_header->type == ICMP_REDIRECT)
    {
        switch (receiver_header->code)
        {
        case ICMP_REDIR_HOST:
            printf("Redirect for Destination Host\n");
            break;
        case ICMP_REDIR_HOSTTOS:
            printf("Redirect for Destination Host Based on Type-of-Service\n");
            break;
        default:
            printf("Redirect error\n");
            break;
        }
    }
    else if (receiver_header->type == ICMP_PARAMETERPROB)
    {
        switch (receiver_header->code)
        {
        case 0:
            printf("Pointer indicates the error\n");
            break;
        case 1:
            printf("Missing a Required Option\n");
            break;
        case 2:
            printf("Bad Length\n");
            break;
        default:
            printf("Parameter problem\n");
            break;
        }
    }
    else
    {
        printf("Unhandled Error\n");
    }
    if (verbose == 1)
    {
        printf("IP Hdr Dump:\n");
        size_t hdr_len = sizeof(*ip_header);
        for (size_t i = 0; i < hdr_len; i+=2)
        {
            if(i + 1 < hdr_len) {
                printf(" %02x%02x", ((unsigned char *)ip_header)[i], ((unsigned char *)ip_header)[i +1]);
            } else {
                printf(" %02x", ((unsigned char *)ip_header)[i]);
            }
        }
        printf("\n");
        printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data\n");
        char src_buf[64];
        char dest_buf[64];
        inet_ntop(AF_INET,&ip_header->saddr,src_buf,sizeof(src_buf));
        inet_ntop(AF_INET,&ip_header->daddr,dest_buf,sizeof(dest_buf));
        printf(" %x  %x  %02x %4x %x   %d %04x  %02x  %02x %x %s  %s\n", ip_header->version, ip_header->ihl,
               ip_header->tos, ip_header->tot_len, ip_header->id, (ip_header->frag_off >> 5),(ip_header->frag_off & 0xBF), ip_header->ttl,
               ip_header->protocol, ip_header->check, src_buf, dest_buf);
    }
}