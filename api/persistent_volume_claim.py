import yaml
from api.crd import JmeterCluster, JmeterClusterSpec
from utils.base_utils import merge_patch_object
from consts.config import DEFAULT_CONFIG_SUFFIX, DEFAULT_DATA_SUFFIX


def prepare_config_volume(cluster: JmeterCluster) -> dict:
    spec = cluster.parsed_spec
    return prepare_persistent_volume_claim(spec, DEFAULT_CONFIG_SUFFIX, spec.configStorage)


def prepare_data_volume(cluster: JmeterCluster) -> dict:
    spec = cluster.parsed_spec
    return prepare_persistent_volume_claim(spec, DEFAULT_DATA_SUFFIX, spec.dataStorage)


def prepare_persistent_volume_claim(spec: JmeterClusterSpec, t: str, volume) -> dict:
    tmpl = f"""
    apiVersion: v1
    kind: PersistentVolumeClaim
    metadata:
      labels:
        app.kubernetes.io/created-by: jmeter
        app.kubernetes.io/instance: {spec.name}
        app.kubernetes.io/managed-by: kustomize
        app.kubernetes.io/name: jmeter
        app.kubernetes.io/part-of: jmeter
      name: {spec.name}-{t}
      namespace: {spec.namespace}
    spec:
      accessModes:
        - ReadWriteMany
      resources:
        requests:
          storage: 10Gi
      storageClassName: csi-minio-s3
      volumeMode: Filesystem
    """
    persistent_volume_claim = yaml.safe_load(tmpl)
    if volume is not None:
        merge_patch_object(persistent_volume_claim["spec"],
                           volume, "spec")
    return persistent_volume_claim
