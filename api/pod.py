from typing import Optional, overload, cast
from kopf import Body
from api.base import K8sInterfaceObject
from api.crd import JmeterCluster
from kubernetes import client
import json
import typing
from logging import Logger
from utils.k8s_utils import api_core, api_client as api_exception


class JmeterPod(K8sInterfaceObject):
    logger: Optional[Logger] = None

    def __init__(self, pod: client.V1Pod):
        super().__init__()
        self.pod: client.V1Pod = pod

    @overload
    @classmethod
    def from_json(cls, pod: str) -> 'JmeterPod':
        ...

    @overload
    @classmethod
    def from_json(cls, pod: Body) -> 'JmeterPod':
        ...

    @classmethod
    def from_json(cls, pod) -> 'JmeterPod':
        class Wrapper:
            def __init__(self, data):
                self.data = json.dumps(data)

        if not isinstance(pod, str):
            pod = eval(str(pod))
        return JmeterPod(cast(client.V1Pod, api_core.api_client.deserialize(Wrapper(pod), client.V1Pod)))

    def __str__(self) -> str:
        return self.name

    def __repr__(self) -> str:
        return f"<JmeterPod {self.name}>"

    @classmethod
    def read(cls, name: str, ns: str) -> 'JmeterPod':
        return JmeterPod(cast(client.V1Pod, api_core.read_namespaced_pod(name, ns)))

    @property
    def metadata(self) -> client.V1ObjectMeta:
        return cast(client.V1ObjectMeta, self.pod.metadata)

    @property
    def status(self) -> client.V1PodStatus:
        return cast(client.V1PodStatus, self.pod.status)

    @property
    def phase(self) -> str:
        return cast(str, self.status.phase)

    @property
    def cluster_name(self) -> str:
        return self.name.rpartition("-")[0]

    @property
    def deleting(self) -> bool:
        return self.metadata.deletion_timestamp is not None

    def self_ref(self, field: Optional[str] = None) -> dict:
        ref = {
            "apiVersion": self.pod.api_version,
            "kind": self.pod.kind,
            "name": self.name,
            "namespace": self.namespace,
            "resourceVersion": self.metadata.resource_version,
            "uid": self.metadata.uid
        }
        if field:
            ref["fieldPath"] = field
        return ref

    @property
    def name(self) -> str:
        return cast(str, self.metadata.name)

    @property
    def namespace(self) -> str:
        return cast(str, self.metadata.namespace)

    def get_cluster(self, logger: Logger) -> typing.Optional[JmeterCluster]:
        try:
            return JmeterCluster.read(self.cluster_name, self.namespace)
        except api_exception as e:
            logger.error(f"Could not get cluster {self.namespace}/{self.cluster_name}: {e}")
            if e.status == 404:
                return None
            raise
