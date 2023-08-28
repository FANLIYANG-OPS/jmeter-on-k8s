#!/bin/bash

# sh start.sh -j 2.jmx -t 2.jtl -e 202304201711

jmx_name="1.jmx"
jtl_name="1.jtl"
exec_time="202304201627"
is_delete_jtl=0

while getopts ":j:t:e:d:" opt ; do
    case $opt in
        "j")
            jmx_name=$OPTARG
            ;;
        "t")
            jtl_name=$OPTARG
            ;;
        "e")
            exec_time=$OPTARG
            ;;
        "d")
            is_delete_jtl=$OPTARG
            ;;
        ?)
            echo "error input"
            ;;
    esac
done

echo "jmx_name is ${jmx_name}"
echo "jtl_name is ${jtl_name}"
echo "exec_time is ${exec_time}"
echo "is_delete_jtl is ${is_delete_jtl}"

folder="/opt/report/${CLUSTER_NAME}/${exec_time}/${HOSTNAME}/report"
if [ ! -d ${folder} ]; then
    mkdir -p ${folder}
else
    echo "${folder} is exists"
fi

sh /opt/apache-jmeter-5.4.1/bin/jmeter -n -t /opt/config/${jmx_name} -l /opt/report/${CLUSTER_NAME}/${exec_time}/${HOSTNAME}/${jtl_name} -e -o /opt/report/${CLUSTER_NAME}/${exec_time}/${HOSTNAME}/report

if [ ${is_delete_jtl} -gt 0 ];then
  rm -rf /opt/report/${CLUSTER_NAME}/${exec_time}/${jtl_name}
fi



