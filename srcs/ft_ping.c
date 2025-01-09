#include "ft_ping.h"

static void ft_ping(char **argv);
static char *resolve_dns(char *hostname, struct sockaddr_in *addr_con);
static char *reverse_dns(char *ip);
static void send_ping(char *ip, char *dns, char *hostname, struct sockaddr_in *addr, int sockfd);
static unsigned short calculate_checksum(void *b, int len);

static bool stop = false;

int main(int ac, char **av)
{
    if (ac < 2)
    {
        printf("Error command must have at least 1 argument\n");
    }
    (void)av;
    // parse args
    // ping
    ft_ping(av);
    return 0;
}

static void sighandler()
{
    stop = true;
}

static void ft_ping(char **argv)
{

    int sockfd;
    char *ip_addr, *reverse_hostname;
    struct sockaddr_in addr_con;

    ip_addr = resolve_dns(argv[1], &addr_con);
    if (ip_addr == NULL)
    {
        return;
    }

    reverse_hostname = reverse_dns(ip_addr);
    if (reverse_hostname == NULL)
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

    send_ping(ip_addr, reverse_hostname, argv[1], &addr_con, sockfd);
}

static char *resolve_dns(char *hostname, struct sockaddr_in *addr_con)
{
    char *ip = malloc(NI_MAXHOST * sizeof(char));
    struct hostent *host = gethostbyname(hostname);
    if (host == NULL)
    {
        printf("ping: %s: Name or service not known\n", hostname);
        return NULL;
    }
    strcpy(ip, inet_ntoa(*(struct in_addr *)host->h_addr));

    // printf("Hostname found %s\n", host->h_name);
    // printf("IP %s\n", ip);

    addr_con->sin_addr.s_addr = *(long *)host->h_name;
    addr_con->sin_family = host->h_addrtype;
    addr_con->sin_port = htons(0);
    return ip;
}

static char *reverse_dns(char *ip)
{
    struct sockaddr_in tmp;
    tmp.sin_addr.s_addr = inet_addr(ip);
    tmp.sin_family = AF_INET;

    socklen_t len = sizeof(struct sockaddr_in);
    char *reversedns = malloc(NI_MAXHOST * sizeof(char));
    if (getnameinfo((struct sockaddr *)&tmp, len, reversedns, NI_MAXHOST, NULL, 0, NI_NAMEREQD))
    {
        printf("Couldn't resolve hostname\n");
        return NULL;
    }
    return reversedns;
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

static void send_ping(char *ip, char *dns, char *hostname,
                      struct sockaddr_in *s_addr, int sockfd)
{

    struct sockaddr_in r_addr;

    struct timeval time_start, time_end, total_time;
    struct timeval t_out;
    struct packet_t packet;
    socklen_t addr_len = 0;
    int ttl = 64, i = 0, count = 1, send_error = 0;
    char receive_buffer[128];

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
    
    gettimeofday(&total_time, NULL);
    printf("PING %s (%s) 56(84) bytes of data.\n", hostname, ip);
    while (!stop)
    {

        bzero(&packet, sizeof(packet));
        bzero(&r_addr, sizeof(r_addr));
        packet.header.type = ICMP_ECHO;
        packet.header.un.echo.id = getpid();

        for (i = 0; i < (int)sizeof(packet.msg); i++) 
        {
            packet.msg[i] = i + '0';
        }
        packet.msg[i] = 0;
        packet.header.un.echo.sequence = count++;
        packet.header.checksum = calculate_checksum(&packet, sizeof(packet));
        gettimeofday(&time_start, NULL);

        int send_len = sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)s_addr, sizeof(*s_addr));
        printf("Send len %d\n",send_len);
        if (send_len <= 0)
        {
            send_error = 1;
            printf("Fail to send packet\n");
        }
        addr_len = sizeof(r_addr);
        int recv_len = recvfrom(sockfd, receive_buffer, sizeof(receive_buffer), 0, (struct sockaddr *)&r_addr, &addr_len);
        printf("Receive len %d\n",recv_len);
        if (recv_len <= 0) {
            perror("Fail to receive packet\n");
            printf("r_buf %s, \n", receive_buffer);
        } else {
            gettimeofday(&time_end, NULL);

            if (!send_error) {
                struct icmphdr *receiver_header = (struct icmphdr *)receive_buffer;
                long double timing = ((double)time_end.tv_usec - (double)time_start.tv_usec) / 1000;
                if (receiver_header->type == 0 && receiver_header->code == 0) {
                    printf("64 bytes from %s (%s): icmp_seq=%d ttl=%d time=%.1Lf ms\n",dns,ip, count, ttl, timing);
                }
            }

        }
        usleep(1000 * 1000);



    }
}