FROM library/ubuntu:22.04
RUN apt-get update && apt-get install -y python3.10 wget
WORKDIR /opt
RUN wget https://bootstrap.pypa.io/get-pip.py
RUN python3.10 get-pip.py
RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
COPY requirements.txt /opt
RUN pip install -r requirements.txt
RUN mkdir -p /opt/api && mkdir -p /opt/deploy && mkdir -p /opt/consts && mkdir -p /opt/utils
COPY start.py /opt
COPY api /opt/api
COPY consts /opt/consts
COPY utils /opt/utils
ENTRYPOINT ["kopf","run","start.py"]

