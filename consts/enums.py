import enum

GROUP = "jmeter.zs.com"
VERSION = "v1"
API_VERSION = GROUP + "/" + VERSION
JMETER_KIND = "Jmeter"
JMETER_PLURAL = "jmeters"
MAX_CLUSTER_NAME_LEN = 28


class Cluster(enum.Enum):
    CREATE_TIME = "createTime"


class ClusterStatus(enum.Enum):
    # 集群在线
    ONLINE = "ONLINE"
    # 开始调度
    PENDING = "PENDING"

