#include "ft_ping.h"

static void ft_ping(char **argv);
static char *resolve_dns(char *hostname, struct sockaddr_in *addr_con);
static char *reverse_dns(char *ip);
static void send_ping(char *ip, char *dns, struct sockaddr_in *addr, int sockfd);

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

static void sighandler() {
    stop = true;
}

static void ft_ping(char **argv)
{

    int sockfd;
    char *ip_addr, *reverse_hostname;
    struct sockaddr_in addr_con;

    ip_addr = resolve_dns(argv[1], &addr_con);
    if (ip_addr == NULL) {
        return ;
    }

    reverse_hostname = reverse_dns(ip_addr);
    if (reverse_hostname == NULL) {
        return ;
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

static void send_ping(char *ip, char *dns, char *hostname, struct sockaddr_in *addr, int sockfd) {

}