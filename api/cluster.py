import kopf
from utils import base_utils as util
from api.crd import JmeterCluster
from logging import Logger


class ClusterMutex:

    def __init__(self, cluster: JmeterCluster, logger: Logger):
        self.cluster = cluster
        self.log = logger

    def __enter__(self, *args):
        mutex_key = f"{self.cluster.namespace}/{self.cluster.name}-mutex"
        mutex_value = f"{self.cluster.name}"
        owner = util.get_ephemeral_state.test_set(
            mutex_key, mutex_value, self.log)
        if owner:
            raise kopf.TemporaryError(
                f"{self.cluster.name} busy. lock_owner={owner}", delay=10)

    def __exit__(self, *args):
        mutex_key = f"{self.cluster.namespace}/{self.cluster.name}-mutex"
        util.get_ephemeral_state.set(
            mutex_key, None, self.log)
