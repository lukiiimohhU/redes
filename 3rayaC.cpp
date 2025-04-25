// cliente_tres_en_raya.cpp
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <csignal>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

using namespace std;
#define ERROR(func) throw runtime_error(string(func) + ": " + strerror(errno))

static int sockfd;
static struct sockaddr_storage servidor_addr;
static socklen_t servidor_len;
static atomic<bool> running{true};

void receptor() {
    char buffer[IP_MAXPACKET];
    while (running) {
        ssize_t bytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                 (struct sockaddr *)&servidor_addr,
                                 &servidor_len);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            cout << buffer << endl;
        }
    }
}

void manejador_sigint(int) {
    running = false;
    close(sockfd);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <IP_servidor> <usuario>" << endl;
        return 1;
    }

    const char *ip_servidor = argv[1];
    string usuario = argv[2];
    const uint16_t PUERTO = 32768;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) ERROR("socket()");

    struct sockaddr_in *dest = (struct sockaddr_in *)&servidor_addr;
    memset(dest, 0, sizeof(*dest));
    dest->sin_family = AF_INET;
    dest->sin_port = htons(PUERTO);
    if (!inet_aton(ip_servidor, &dest->sin_addr)) {
        cerr << "IP inválida: " << ip_servidor << endl;
        return 1;
    }
    servidor_len = sizeof(*dest);

    signal(SIGINT, manejador_sigint);

    string helo_cmd = string("HELO") + usuario;
    sendto(sockfd, helo_cmd.c_str(), helo_cmd.size(), 0,
           (struct sockaddr *)&servidor_addr, servidor_len);

    jthread hilo_receptor(receptor);

    cout << "Esperando al otro jugador...\n";

    while (running) {
        string input;
        cout << "Ingresa tu jugada (fila,columna): ";
        getline(cin, input);
        if (input.size() == 3 && input[1] == ',' && isdigit(input[0]) && isdigit(input[2])) {
            string mov_cmd = string("MOV_") + input;
            sendto(sockfd, mov_cmd.c_str(), mov_cmd.size(), 0,
                   (struct sockaddr *)&servidor_addr, servidor_len);
        } else {
            cout << "Formato inválido. Usa fila,columna (ej: 0,1)\n";
        }
    }

    return 0;
}
