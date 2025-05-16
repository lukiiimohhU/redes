1. Asignar IPs:
     $ sudo nano /etc/netplan/01-netcfg.yaml
     network:
       version: 2
       ethernets:
         enp4s0:
           addresses:
             - 10.0.1.1/24
     $ sudo netplan apply
# Lo mismo para la otra y comprobar conexión con ping

### Comprobar versión de DHCP
1. En Máquina A (servidor):
   $ sudo apt update
   $ sudo apt install isc-dhcp-server
   $ apt-cache policy isc-dhcp-server  # Anota versión
2. En Máquina B (cliente):
   $ sudo apt update
   $ sudo apt install isc-dhcp-client
   $ apt-cache policy isc-dhcp-client  # Anota versión

### Configurar servidor DHCP
1. $ sudo nano /etc/dhcp/dhcpd.conf
   ddns-update-style none;
   subnet 10.0.1.0 netmask 255.255.255.0 {
       range 10.0.1.11 10.0.1.100;
       option subnet-mask 255.255.255.0;
       option routers 10.0.1.1;
       option domain-name-servers 8.8.8.8;
       option broadcast-address 10.0.1.255;
       default-lease-time 86400;
       max-lease-time 86400;
   }
2. Configura interfaz:
   $ sudo nano /etc/default/isc-dhcp-server
   INTERFACESv4="enp4s0"
3. Inicia servidor:
   $ sudo systemctl start isc-dhcp-server
   $ sudo systemctl status isc-dhcp-server

### Configurar cliente DHCP
1. $ sudo nano /etc/netplan/01-netcfg.yaml
   network:
     version: 2
     ethernets:
       enp4s0:
         dhcp4: yes
   $ sudo netplan apply
2. Desactiva/activa interfaz:
   $ sudo ip link set eth0 down
   $ sudo ip link set eth0 up
   $ sudo dhclient eth0
3. Verifica IP:
   ip addr show eth0

### Comprobar archivo de préstamos
1. En servidor:
   $ cat /var/lib/dhcp/dhcpd.leases

### Asignar IP fija al cliente
1. En cliente:
   $ ip link show eth0  # Ejemplo: 00:00:ff:32:fa:23
2. En Servidor:
   $ sudo nano /etc/dhcp/dhcpd.conf
   Añadir dentro de subnet:
   host pc1 {
       hardware ethernet 00:00:ff:32:fa:23;
       fixed-address 10.0.1.2;
       option host-name "pc1";
   }
3. Reinicia servidor:
   $ sudo systemctl restart isc-dhcp-server

### Liberar y renovar concesión
1. En Cliente, libera:
   $ sudo dhclient -r eth0
2. Solicita nueva configuración:
   $ sudo dhclient eth0
   - Paquetes: DHCPDISCOVER, DHCPOFFER, DHCPREQUEST, DHCPACK
