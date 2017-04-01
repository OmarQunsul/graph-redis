make
sudo make install
sudo /etc/init.d/redis_6379 stop
sudo killall redis-server
sudo rm /var/run/redis_6379.pid
#sudo /etc/init.d/redis_6379 start
gdb -ex run redis-server
