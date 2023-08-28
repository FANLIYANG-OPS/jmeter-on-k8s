#!/bin/bash
go build main.go
mv main up
docker build -f Dockerfile -t 192.168.252.252:5566/library/nginx:1.24.0-up .
docker push 192.168.252.252:5566/library/nginx:1.24.0-up
rm -rf up
