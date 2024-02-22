sudo ip address add 10.11.50.112/16 dev eth0 broadcast +
sudo ip link set dev eth0 up
sudo ip route add default via 10.11.0.1

