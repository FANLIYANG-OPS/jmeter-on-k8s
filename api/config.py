import yaml
from api.crd import JmeterCluster


def prepare_config_map(cluster: JmeterCluster):
    conf = f"""
    user  nginx;
    worker_processes  auto;
    error_log  /var/log/nginx/error.log notice;
    pid  /var/run/nginx.pid;
    events {{
        worker_connections  1024;
    }}
    http {{
        server {{
            listen 80;
            server_name localhost;
            location / {{
                root /opt/report/{cluster.name}/;
                autoindex on;
                autoindex_exact_size off;
                autoindex_localtime on;
            }}
        }}
    }}
    """
    temp = f"""
        apiVersion: v1
        data:
          nginx.conf: ""
        kind: ConfigMap
        metadata:
          name: {cluster.name}-config
          namespace: {cluster.namespace}
        """
    config_map = yaml.safe_load(temp)
    config_map["data"]["nginx.conf"] = conf
    return config_map
