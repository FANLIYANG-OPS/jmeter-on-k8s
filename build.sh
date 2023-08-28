#!/bin/bash
docker build -f Dockerfile -t 192.168.252.252:5566/jmeter/operator:1.0.5 .
docker push 192.168.252.252:5566/jmeter/operator:1.0.5
kubectl set image deployment/jmeter-operator -n middleware-jmeter jmeter-operator=jmeter/operator:1.0.5
