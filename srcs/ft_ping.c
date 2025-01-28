#include "ft_ping.h"

static void ft_ping(char *argv);
static char *resolve_dns(char *hostname, struct sockaddr_in *addr_con);
// static char *reverse_dns(char *ip, char *arg);
static void send_ping(char *ip, char *hostname, struct sockaddr_in *addr, int sockfd);
static unsigned short calculate_checksum(void *b, int len);
// static bool is_ip_address(char *addr);
static error_t parse_opt(int key, char *arg, struct argp_state *state);

static bool stop = false;

const char *argp_program_bug_address =
  "<hdiot.student@42lyon.fr>";

static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Produce verbose output", 0},
    {"help", '?', 0, 0, "Display ping options", 0},
    {0}};

struct arguments
{
    char *args[2]; /* arg1 & arg2 */
    int verbose, help;
};

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
        if (state->arg_num == 2) {
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

    ft_ping(arguments.args[0]);
    return 0;
}

static void sighandler()
{
    stop = true;
}

static void ft_ping(char *argv)
{

    int sockfd;
    char *ip_addr;
    struct sockaddr_in addr_con;

    ip_addr = resolve_dns(argv, &addr_con);
    if (ip_addr == NULL)
    {
        return;
    }

    // reverse_hostname = reverse_dns(ip_addr, argv);
    // if (reverse_hostname == NULL)
    // {
    //     return;
    // }

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        printf("Fail to create socket fd %d\n", sockfd);
        return;
    }

    signal(SIGINT, sighandler);

    send_ping(ip_addr, argv, &addr_con, sockfd);
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

// static bool is_ip_address(char *addr)
// {
//     int len = strlen(addr);
//     for (int i = 0; i < len; i++)
//     {
//         if ((addr[i] <= '0' || addr[i] >= '9') && addr[i] != '.')
//         {
//             return false;
//         }
//     }
//     return true;
// }

// static char *reverse_dns(char *ip, char *arg)
// {
//     struct sockaddr_in tmp;
//     tmp.sin_addr.s_addr = inet_addr(ip);
//     tmp.sin_family = AF_INET;

//     socklen_t len = sizeof(struct sockaddr_in);
//     char *reversedns = malloc(NI_MAXHOST * sizeof(char));
//     if (is_ip_address(arg) == true)
//     {
//         strncpy(reversedns, arg, strlen(arg));
//     }
//     else
//     {
//         if (getnameinfo((struct sockaddr *)&tmp, len, reversedns, NI_MAXHOST, NULL, 0, NI_NAMEREQD))
//         {
//             // printf("Couldn't resolve hostname\n");
//             return reversedns;
//         }
//     }
//     return reversedns;
// }

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
                      struct sockaddr_in *s_addr, int sockfd)
{

    struct sockaddr_in r_addr;

    struct timeval time_start, time_end = {0};
    struct timeval t_out;
    struct packet_t packet;
    socklen_t addr_len = 0;
    int ttl = 64, i = 0, count = 0, send_error = 0;
    char receive_buffer[128];
    int send_count = 0, receive_count = 0, error_count = 0;
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

    printf("PING %s (%s) 56 data bytes\n", hostname, ip);
    while (!stop)
    {
        bzero(&packet, sizeof(packet));
        bzero(&r_addr, sizeof(r_addr));
        bzero(receive_buffer, sizeof(receive_buffer));
        packet.header.type = ICMP_ECHO;
        packet.header.un.echo.id = htons(getpid());
        packet.header.code = 0;

        for (i = 0; i < (int)sizeof(packet.msg); i++)
        {
            packet.msg[i] = i + '0';
        }
        packet.msg[i] = 0;
        packet.header.un.echo.sequence = __bswap_16(count++);
        packet.header.checksum = calculate_checksum(&packet, sizeof(packet));

        gettimeofday(&time_start, NULL);
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

        int recv_len = recvfrom(sockfd, receive_buffer, sizeof(receive_buffer), 0, (struct sockaddr *)s_addr, &addr_len);
        gettimeofday(&time_end, NULL);
        long double timing = ((double)time_end.tv_usec - (double)time_start.tv_usec) / 1000;
        if (timing < 0)
        {
            timing += 1000;
        }
        if (recv_len > 0)
        {
            receive_count++;
            struct iphdr *ip_header = (struct iphdr *)receive_buffer;
            struct icmphdr *receiver_header = (struct icmphdr *)(receive_buffer + (ip_header->ihl * 4));

            if (!send_error)
            {
                if (receiver_header->type == 0 && receiver_header->code == 0)
                {
                    if (timing < min || min == 0)
                    {
                        min = timing;
                    }
                    if (timing > max || max == 0)
                    {
                        max = timing;
                    }
                    avg += timing;
                    if (count > 1)
                    {
                        long double calcul_mdev = last_timing - timing;
                        if (calcul_mdev < 0)
                        {
                            calcul_mdev *= -1;
                        }
                        mdev += calcul_mdev;
                    }
                    last_timing = timing;
                    printf("64 bytes from %s: icmp_seq=%d ttl=%d time=%.1Lf ms\n", ip, __bswap_16(receiver_header->un.echo.sequence), ip_header->ttl, timing);
                }
                else
                {
                    error_count++;
                    printf("Receive code %d type %d\n", receiver_header->code, receiver_header->type);
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
    printf("%d packets transmitted, %d received,", send_count, receive_count);
    if (error_count > 0)
    {
        printf("+ %derrors, ", error_count);
    }
    printf(" %d%% packet loss\n", 100 - (receive_count / send_count * 100));
    if (receive_count > 0)
    {
        mdev = mdev / receive_count;
        avg = avg / receive_count;
        printf("round-trip min/avg/max/stddev = %.3Lf/%.3Lf/%.3Lf/%.3Lf ms\n", min, avg, max, mdev);
    }
}