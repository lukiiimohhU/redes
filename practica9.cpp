#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <iomanip>
using namespace std;

void muestraIP(struct addrinfo *direccion) {

    cout << "Direcciones IPv4" << endl;
    cout << setfill('=') << setw(25) << "" << setfill(' ') << endl;
    for (struct addrinfo *rp = direccion; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) { //Para las IPv4
            char strdireccion[NI_MAXHOST + 1];
            inet_ntop(rp->ai_family,
                      (void *)&((struct sockaddr_in *)rp->ai_addr)->sin_addr,
                      strdireccion, NI_MAXHOST);
            cout << strdireccion << endl;
        }
    }

    cout << "\nDirecciones IPv6" << endl;
    cout << setfill('=') << setw(25) << "" << setfill(' ') << endl;   
    for (struct addrinfo *rp = direccion; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET6) { //Para las IPv6
            char strdireccion[NI_MAXHOST + 1];
            inet_ntop(rp->ai_family,
                      (void *)&((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr,
                      strdireccion, NI_MAXHOST);
            cout << strdireccion << endl;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <host1> <host2> ..." << endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        cout << endl << "\nHost >" << argv[i] << "<" << endl << endl;

        struct addrinfo configuracion, *resultado;

        configuracion = {};
        configuracion.ai_family = AF_UNSPEC;
        configuracion.ai_socktype = SOCK_STREAM;

        getaddrinfo(argv[i], NULL, &configuracion, &resultado);

        muestraIP(resultado);
        freeaddrinfo(resultado);
    }

    return 0;
}
