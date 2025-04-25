#include <iostream>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

using namespace std;
#define ERROR(funcion) throw runtime_error(string(funcion) + ": " + strerror(errno))

struct Cliente {
    string id;
    sockaddr_storage addr;
    socklen_t addrlen;
};

int configura_servidor(const char *puerto) {
    struct addrinfo conf {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM
    }, *resultado;
    if(getaddrinfo(NULL, puerto, &conf, &resultado)) ERROR("getaddrinfo()");
    int unsocket = -1;
    while(unsocket = socket(resultado->ai_family,
                           resultado->ai_socktype, resultado->ai_protocol)) {
        if(unsocket != -1) break;
        cout << "socket() - " + string(strerror(errno)) << endl;
        resultado = resultado->ai_next;
    }
    if(bind(unsocket, resultado->ai_addr, resultado->ai_addrlen))
        ERROR("bind()");
    freeaddrinfo(resultado);
    return unsocket;
}

void procesa_mensaje(int sock, char* buf, ssize_t bytes,
                      sockaddr_storage& dir, socklen_t dirlen,
                      vector<Cliente>& clientes) {
    if (bytes < 4) return;  // mensaje demasiado corto
    string cmd(buf, buf+4);
    string datos(buf+4, buf+bytes);

    //mostrar desde donde viene la conexión
    char host[NI_MAXHOST]; //serv[NI_MAXSERV];
    getnameinfo((sockaddr*)&dir, dirlen,
                host, sizeof(host), nullptr, 0, //serv, sizeof(serv)
                NI_NUMERICHOST); // | NI_NUMERICSERV
    cout << "[" << host << "] "
         << cmd << " " << datos << endl;

    if (cmd == "HELO") {
        //meter el cliente al vector
        Cliente c{datos, dir, dirlen};
        clientes.push_back(c);

        string msg = "* " + c.id + " se ha unido al chat";
        for (auto& cli : clientes) {
            sendto(sock, msg.c_str(), msg.size(), 0,
                   (sockaddr*)&cli.addr, cli.addrlen);
        }
    }
    else if (cmd == "SEND") {
        //buscar los remitentes
        string remitente;
        for (auto& cli : clientes) {
            if (memcmp(&cli.addr, &dir, dirlen)==0) {
                remitente = cli.id;
                break;
            }
        }
        if (remitente.empty()) return;
        //hacemos el mensaje y lo enviamos
        string msg = "[" + remitente + "] -> " + datos;
        for (auto& cli : clientes) {
            sendto(sock, msg.c_str(), msg.size(), 0,
                   (sockaddr*)&cli.addr, cli.addrlen);
        }
    }
    else if (cmd == "QUIT") {
        auto it = remove_if(clientes.begin(), clientes.end(), [&](const Cliente& cli){
            return memcmp(&cli.addr, &dir, dirlen)==0;
        });
        if (it != clientes.end()) {
            string id = it->id;
            clientes.erase(it, clientes.end());
            // salida
            string msg = "* " + id + " ha abandonado el chat";
            for (auto& cli : clientes) {
                sendto(sock, msg.c_str(), msg.size(), 0,
                       (sockaddr*)&cli.addr, cli.addrlen);
            }
        }
    }
}

int main() {
    const char* PUERTO = "32768";
    int sock = configura_servidor(PUERTO);
    vector<Cliente> clientes;

    char* buf = new char[IP_MAXPACKET];
    sockaddr_storage dir;
    socklen_t dirlen;

    while (true) {
        dirlen = sizeof(dir);
        ssize_t bytes = recvfrom(sock, buf, IP_MAXPACKET, 0,
                                 (sockaddr*)&dir, &dirlen);
        if (bytes == -1) ERROR("recvfrom");
        procesa_mensaje(sock, buf, bytes, dir, dirlen, clientes);
    }

    delete[] buf;
    close(sock);
    return 0;
}