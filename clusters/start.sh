#!/bin/bash

password=$1

echo "clean up the cluster files..."
echo $password | killall redis-server
sleep 5
rm -f nodes-*.conf
rm -f dump*
rm -f log*

../src/redis-server 7000/redis.conf --maxmemory 15mb
../src/redis-server 7001/redis.conf --maxmemory 15mb
../src/redis-server 7002/redis.conf --maxmemory 15mb
../src/redis-server 7003/redis.conf --maxmemory 15mb
../src/redis-server 7004/redis.conf --maxmemory 15mb
../src/redis-server 7005/redis.conf --maxmemory 15mb

../src/redis-trib.rb create --replicas 1 127.0.0.1:7000 127.0.0.1:7001 127.0.0.1:7002 127.0.0.1:7003 127.0.0.1:7004 127.0.0.1:7005

