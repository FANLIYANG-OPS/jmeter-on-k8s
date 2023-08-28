FROM  library/nginx:1.24.0
COPY up /opt
COPY start.sh /opt
RUN chmod +x /opt/up
RUN chmod +x /opt/start.sh
WORKDIR /opt
ENTRYPOINT [ "sh" , "-c" , "/opt/start.sh" ]

