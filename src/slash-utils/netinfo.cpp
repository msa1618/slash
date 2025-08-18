#include "../abstractions/iofuncs.h"
#include "../abstractions/definitions.h"


#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pwd.h>

class NetInfo {
  public:
  NetInfo() {}

  std::pair<std::string, std::string> get_ip_and_netmask() {
    std::string ipv4;
    std::string mask;

    std::string ipv6;
    std::string v6_mask;

    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN];
    char netmask[INET_ADDRSTRLEN];

    if(getifaddrs(&ifaddr) == -1) {
      perror("netinfo");
    }

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
      if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
        void* addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
        void* msk = &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr;
        inet_ntop(AF_INET, addr, ip, sizeof(ip));
        inet_ntop(AF_INET, msk, netmask, sizeof(netmask));

        if(std::string(ip) != "127.0.0.1") {
          ipv4 = std::string(ip);
          mask = std::string(netmask);
        }
      }
    }

    return std::pair<std::string, std::string>(ipv4, mask);
  }

  int exec(std::vector<std::string> args) {
    uid_t uid = getuid();
    struct passwd* ps = getpwuid(uid);

    std::string ipv4_addr = get_ip_and_netmask().first;
    std::string netmask = get_ip_and_netmask().second;
    std::string ssh_cs = std::string(ps->pw_name) + "@" + ipv4_addr.c_str();

    io::print("Network Information\n");
    io::print("---------------------\n");

    io::print("IP Address: ");
    io::print(blue);
    io::print(ipv4_addr);
    io::print(reset);
    io::print("\n");

    io::print("Netmask: ");
    io::print(blue);
    io::print(netmask);
    io::print(reset);
    io::print("\n");

    io::print("SSH Connection String: ");
    io::print(blue);
    io::print(ssh_cs);
    io::print(reset);
    io::print("\n");

    return 0;
  }
};

int main(int argc, char* argv[]) {
  NetInfo ni;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  ni.exec(args);
  return 0;
}