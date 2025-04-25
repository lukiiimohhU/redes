// servidor_tres_en_raya.cpp (simplificado)
#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

using namespace std;
#define ERROR(func) throw runtime_error(string(func) + ": " + strerror(errno))

struct Cliente {
    string id;
    sockaddr_storage addr;
    socklen_t addrlen;
    char simbolo;
};

char tablero[3][3];

void reiniciar_tablero() {
    for (auto& fila : tablero)
        fill(begin(fila), end(fila), ' ');
}

string mostrar_tablero() {
    string s;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            s += tablero[i][j];
            if (j < 2) s += "|";
        }
        if (i < 2) s += "\n-+-+-\n";
    }
    return s;
}

int configura_servidor(const char *puerto) {
    struct addrinfo conf {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM
    }, *resultado;
    if (getaddrinfo(NULL, puerto, &conf, &resultado)) ERROR("getaddrinfo()");
    int unsocket = socket(resultado->ai_family, resultado->ai_socktype, resultado->ai_protocol);
    if (unsocket == -1) ERROR("socket()");
    if (bind(unsocket, resultado->ai_addr, resultado->ai_addrlen)) ERROR("bind()");
    freeaddrinfo(resultado);
    return unsocket;
}

void enviar_a_todos(int sock, const string& msg, const vector<Cliente>& clientes) {
    for (auto& c : clientes) {
        sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&c.addr, c.addrlen);
    }
}

void procesa_mensaje(int sock, char* buf, ssize_t bytes,
                      sockaddr_storage& dir, socklen_t dirlen,
                      vector<Cliente>& clientes, int& turno) {
    if (bytes < 4) return;
    string cmd(buf, buf+4);
    string datos(buf+4, buf+bytes);

    if (cmd == "HELO") {
        if (clientes.size() >= 2) return;
        char simbolo = clientes.empty() ? 'X' : 'O';
        clientes.push_back({datos, dir, dirlen, simbolo});
        string bienvenida = "Bienvenido " + datos + ", tu símbolo es "; bienvenida += simbolo;
        sendto(sock, bienvenida.c_str(), bienvenida.size(), 0, (sockaddr*)&dir, dirlen);
        if (clientes.size() == 2) {
            reiniciar_tablero();
            enviar_a_todos(sock, "¡Comienza la partida!\n" + mostrar_tablero(), clientes);
        }
    } else if (cmd == "MOV_" && clientes.size() == 2) {
        int fila = datos[0] - '0';
        int col = datos[2] - '0';
        Cliente& jugador = clientes[turno];
        if (tablero[fila][col] == ' ') {
            tablero[fila][col] = jugador.simbolo;
            turno = 1 - turno;
            string estado = mostrar_tablero();
            enviar_a_todos(sock, estado + "\nTurno de: " + clientes[turno].id, clientes);
        }
    }
}

int main() {
    const char* PUERTO = "32768";
    int sock = configura_servidor(PUERTO);
    vector<Cliente> clientes;
    int turno = 0;
    char* buf = new char[IP_MAXPACKET];
    sockaddr_storage dir;
    socklen_t dirlen;

    while (true) {
        dirlen = sizeof(dir);
        ssize_t bytes = recvfrom(sock, buf, IP_MAXPACKET, 0, (sockaddr*)&dir, &dirlen);
        if (bytes == -1) ERROR("recvfrom");
        procesa_mensaje(sock, buf, bytes, dir, dirlen, clientes, turno);
    }

    delete[] buf;
    close(sock);
    return 0;
}
