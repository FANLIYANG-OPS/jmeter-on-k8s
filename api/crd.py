import datetime
from typing import Optional, cast
from kopf import Body
from logging import Logger
from api.base import K8sInterfaceObject
from consts.enums import API_VERSION, JMETER_KIND, MAX_CLUSTER_NAME_LEN, GROUP, VERSION, JMETER_PLURAL, Cluster
from consts.config import DEFAULT_CONFIG_SUFFIX, DEFAULT_DATA_SUFFIX
from utils.api_utils import ApiSpecError, do_get_str, do_get_dict
from utils.k8s_utils import ApiException as api_err, api_custom_obj, client as api_client, api_apps, api_core
from utils.base_utils import merge_patch_object, iso_time
from api.pod_api import send_script_to_pod


class JmeterClusterSpec:
    instances: int = 1
    configStorage = None
    dataStorage = None
    command = None

    def __init__(self, namespace: str, name: str, spec: dict) -> None:
        self.namespace = namespace
        self.name = name
        self.load(spec)

    def load(self, spec: dict) -> None:
        if "configStorage" in spec:
            self.configStorage = do_get_dict(spec, "configStorage", "spec")
        if "dataStorage" in spec:
            self.dataStorage = do_get_dict(spec, "dataStorage", "spec")
        if "command" in spec:
            self.command = do_get_str(spec, "command", "spec")

    def validate(self, logger: Logger) -> None:
        if len(self.name) > MAX_CLUSTER_NAME_LEN:
            logger.error(
                f"Cluster name {self.name} is longer than {MAX_CLUSTER_NAME_LEN}")
            raise ApiSpecError(
                f"cluster name {self.name} is tool long , Must be < {MAX_CLUSTER_NAME_LEN}")
        if not self.instances:
            logger.error(
                f"spec.instances must be set and > 0 . Got {self.instances!r}")
            raise ApiSpecError(
                f"spec.instances must be set and > 0 . Got {self.instances!r} ")


class JmeterCluster(K8sInterfaceObject):

    def __init__(self, cluster: Body) -> None:
        super().__init__()
        self.obj: Body = cluster
        self._parsed_spec: Optional[JmeterClusterSpec] = None

    def self_ref(self, field_path: Optional[str] = None) -> dict:
        ref = {
            "apiVersion": API_VERSION,
            "kind": JMETER_KIND,
            "name": self.name,
            "namespace": self.namespace,
            "resourceVersion": self.metadata["resourceVersion"],
            "uid": self.uid
        }
        if field_path:
            ref["fieldPath"] = field_path
        return ref

    @classmethod
    def _get(cls, ns: str, name: str) -> Body:
        try:
            res = cast(Body, api_custom_obj.get_namespaced_custom_object(
                GROUP, VERSION, ns, JMETER_PLURAL, name))
        except api_err as e:
            raise e
        return res

    @classmethod
    def _patch_status(cls, ns: str, name: str, patch: dict) -> Body:
        return cast(Body,
                    api_custom_obj.patch_namespaced_custom_object_status(GROUP, VERSION, ns, JMETER_PLURAL, name,
                                                                         body=patch))

    @classmethod
    def read(cls, name: str, namespace: str) -> 'JmeterCluster':
        return JmeterCluster(cls._get(namespace, name))

    @property
    def instances(self) -> int:
        return cast(int, self.spec["instances"])

    @property
    def spec(self) -> dict:
        return self.obj["spec"]

    @property
    def metadata(self) -> dict:
        return self.obj["metadata"]

    @property
    def name(self) -> str:
        return self.metadata["name"]

    @property
    def namespace(self) -> str:
        return self.metadata["namespace"]

    @property
    def uid(self) -> str:
        return self.metadata["uid"]

    @property
    def status(self) -> dict:
        if "status" in self.obj:
            return self.obj["status"]
        return {}

    @property
    def ready(self) -> bool:
        return cast(bool, self.get_create_time())

    def parse_spec(self) -> None:
        self._parsed_spec = JmeterClusterSpec(
            self.namespace, self.name, self.spec)

    @property
    def parsed_spec(self) -> JmeterClusterSpec:
        if not self._parsed_spec:
            self.parse_spec()
            assert self._parsed_spec
        return self._parsed_spec

    def get_stateful_set(self) -> Optional[api_client.V1StatefulSet]:
        try:
            return cast(api_client.V1StatefulSet, api_apps.read_namespaced_stateful_set(
                self.name, self.namespace))
        except api_err as e:
            if e.status == 404:
                return None

    def cluser_pod_running_num(self) -> Optional[int]:
        pods = self.get_jmeter_pods()
        if pods is not None:
            num = 0
            for pod in pods.items:
                if pod.status.phase == "Running":
                    num += 1
            return num
        return 0

    def get_deployments(self) -> Optional[api_client.V1Deployment]:
        try:
            return cast(api_client.V1Deployment,
                        api_apps.read_namespaced_deployment(f"{self.name}-nginx", self.namespace))
        except api_err as e:
            if e.status == 404:
                return None

    def get_service(self) -> Optional[api_client.V1Service]:
        try:
            return cast(api_client.V1Service, api_core.read_namespaced_service(f"{self.name}-svc", self.namespace))
        except api_err as e:
            if e.status == 404:
                return None

    def get_configs(self) -> Optional[api_client.V1ConfigMap]:
        try:
            return cast(api_client.V1ConfigMap,
                        api_core.read_namespaced_config_map(f"{self.name}-config", self.namespace))
        except api_err as e:
            if e.status == 404:
                return None

    def delete_storage_by_name(self, param):
        try:
            api_core.delete_namespaced_persistent_volume_claim(
                param, self.namespace, async_req=True)
        except api_err as e:
            self.error(action="DeleteCluster",
                       reason="DeleteResourceFailed", message=f"{e}")

    def get_share_storage(self, name) -> Optional[api_client.V1PersistentVolumeClaim]:
        try:
            return cast(api_client.V1PersistentVolumeClaim, api_core.read_namespaced_persistent_volume_claim(
                name, self.namespace))
        except api_err as e:
            if e.status == 404:
                return None

    def get_share_config_storage(self) -> Optional[api_client.V1PersistentVolumeClaim]:
        return self.get_share_storage(f"{self.name}-{DEFAULT_CONFIG_SUFFIX}")

    def get_share_data_storage(self) -> Optional[api_client.V1PersistentVolumeClaim]:
        return self.get_share_storage(f"{self.name}-{DEFAULT_DATA_SUFFIX}")

    # def is_job_running(self, shell: str, logger: Logger) -> None:
    #     send_script_to_pod("ps | grep start.sh | awk {'print $1'}", logger)

    def send_script_to_cluster(self, shell: str, logger: Logger) -> None:
        pods = self.get_jmeter_pods()
        if pods is not None:
            tasks = []
            for pod in pods.items:
                name = pod.metadata.name
                self.set_status({
                    "cluster": {
                        f"{self.namespace}-{name}": True,
                        "lastProbeTime": iso_time()
                    }
                })
                tasks.append(send_script_to_pod(
                    self.namespace, name, shell, logger))
            for task in tasks:
                task.join()

    def get_jmeter_pods(self) -> Optional[api_client.V1PodList]:
        label = f"app.kubernetes.io/instance={self.name}"
        try:
            return cast(api_client.V1PodList, api_core.list_namespaced_pod(namespace=self.namespace,
                                                                           label_selector=label))
        except api_err as e:
            if e.status == 404:
                return None

    def set_status(self, status: dict) -> None:
        obj = cast(dict, self._get(self.namespace, self.name))
        if "status" not in obj:
            obj["status"] = status
        else:
            merge_patch_object(obj["status"], status)
        self.obj = self._patch_status(self.namespace, self.name, obj)

    def log_cluster_info(self, logger: Logger) -> None:
        logger.info(f"Jmeter Cluster {self.namespace}/{self.name}\n")
        logger.info(f"Jmeter instance:{self.parsed_spec.instances}\n")

    def get_create_time(self) -> Optional[datetime.datetime]:
        data_time = self._get_status_field(Cluster.CREATE_TIME.value)
        if data_time:
            return datetime.datetime.fromisoformat(data_time.rstrip("Z"))
        return None

    def _get_status_field(self, field: str):
        return cast(str, self.status.get(field))
