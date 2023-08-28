import logging
from logging import Logger, INFO, WARNING, basicConfig
from kopf import Body
from api.cluster import ClusterMutex
from consts.enums import ClusterStatus, GROUP, VERSION, JMETER_PLURAL
from consts.config import DEFAULT_CONFIG_SUFFIX, DEFAULT_DATA_SUFFIX
from utils import api_utils, base_utils
from api.crd import JmeterCluster
from consts.config import log_config_banner
from typing import Optional, Any
import kopf
from utils.k8s_utils import ApiException as api_exception, api_apps, api_core
from api import statefulset_api as sts, deploy_api as deploy, config as cm_api, service_api
from api import persistent_volume_claim as pvc_api
# import time


@kopf.on.startup()
async def on_startup(settings: kopf.OperatorSettings, logger: Logger, *args, **_):
    settings.posting.level = logging.WARNING
    settings.posting.enabled = True
    settings.watching.connect_timeout = 1 * 60
    settings.watching.server_timeout = 10 * 60
    base_utils.log_banner(__file__, logger)
    log_config_banner(logger)
    kopf.configure(verbose=True)
    basicConfig(
        level=WARNING,
        format='%(asctime)s - [%(levelname)s] [%(name)s] %(message)s',
        datefmt="%Y-%m-%dT%H:%M:%S"
    )
    pass


@kopf.on.cleanup()
async def on_shutdown(logger: Logger, *args, **kwargs):
    logger.info(f"jmeter operator will be close...")
    pass


@kopf.timer(GROUP, VERSION, JMETER_PLURAL, interval=60, initial_delay=30)
async def on_timer(name: str, namespace: Optional[str], body: Body, logger: Logger, **kwargs):
    logger.info(f"jmeter cluster {namespace}/{name} will be check")
    cluster = JmeterCluster(body)
    run_num = cluster.cluser_pod_running_num()
    logger.info(f"jmeter cluster {namespace}/{name} running num : {run_num}")
    if run_num == cluster.instances:
        cluster.set_status({
            "cluster": {
                "status": ClusterStatus.ONLINE.value,
                "onlineInstances": cluster.instances,
                "lastProbeTime": base_utils.iso_time()
            },
        })
    else:
        cluster.set_status({
            "cluster": {
                "status": ClusterStatus.PENDING.value,
                "onlineInstances": cluster.cluser_pod_running_num(),
                "lastProbeTime": base_utils.iso_time()
            },
        })


@kopf.on.create(GROUP, VERSION, JMETER_PLURAL)
async def create_fn(name: str, namespace: Optional[str], body: Body, logger: Logger, **kwargs):
    logger.info(
        f"Initializing Jmeter Cluster with : {namespace}/{name} , will be create")
    cluster = JmeterCluster(body)
    try:
        # 将dict转化为spec
        cluster.parse_spec()
        # 检验spec参数是否正常
        cluster.parsed_spec.validate(logger)
    except api_utils.ApiSpecError as e:
        cluster.set_status({
            "cluster": {
                "status": ClusterStatus.PENDING.value,
                "onlineInstances": 0,
                "lastProbeTime": base_utils.iso_time()
            },
        })
        cluster.error(action="CreateCluster",
                      reason="InvalidArgument", message=str(e))
        raise kopf.TemporaryError(f"error in JmeterCluster spec:{e}")
    spec = cluster.parsed_spec

    def ignore_404(f) -> Any:
        try:
            return f()
        except api_exception as err:
            if err.status == 404:
                return None
            raise

    cluster.log_cluster_info(logger)
    if not cluster.ready:
        try:
            if not ignore_404(cluster.get_configs):
                print(
                    "-----------------------1.create cluster config map----------------------------------\n")
                config_map = cm_api.prepare_config_map(cluster)
                kopf.adopt(config_map)
                api_core.create_namespaced_config_map(
                    namespace=namespace, body=config_map)

            if not ignore_404(cluster.get_share_config_storage):
                if spec.configStorage is not None:
                    print(
                        "-------------------------2.create cluster minio share config storage---------------------\n")
                    pvc = pvc_api.prepare_config_volume(cluster)
                    kopf.adopt(pvc)
                    api_core.create_namespaced_persistent_volume_claim(
                        namespace=namespace, body=pvc, async_req=True)
            if not ignore_404(cluster.get_share_data_storage):
                if spec.dataStorage is not None:
                    print(
                        "-------------------------3.create cluster minio data config storage-------------------------\n")
                    pvc = pvc_api.prepare_data_volume(cluster)
                    kopf.adopt(pvc)
                    api_core.create_namespaced_persistent_volume_claim(
                        namespace=namespace, body=pvc, async_req=True)
            if not ignore_404(cluster.get_stateful_set):
                print(
                    "----------------------------4.Jmeter Cluster stateful_set----------------------------\n")
                if spec.instances > 0:
                    stateful_set = sts.prepare_stateful_set(
                        cluster)
                    logger.info(
                        f"the sts will be create , with {stateful_set}")
                    kopf.adopt(stateful_set)
                    api_apps.create_namespaced_stateful_set(
                        namespace=namespace, body=stateful_set)
            if not ignore_404(cluster.get_deployments):
                print(
                    "------------------------5.nginx Deployment--------------------------\n")
                deployment = deploy.prepare_deployment(cluster)
                logger.info(
                    f"the nginx deploy will be create , with {deployment}")
                kopf.adopt(deployment)
                api_apps.create_namespaced_deployment(
                    namespace=namespace, body=deployment)
            if not ignore_404(cluster.get_service):
                print(
                    "------------------------6.nginx service----------------------------\n")
                service = service_api.prepare_service(cluster)
                logger.info(
                    f"the nginx service will be create , with {service}")
                kopf.adopt(service)
                api_core.create_namespaced_service(
                    namespace=namespace, body=service)
        except Exception as exc:
            cluster.error(action="CreateCluster",
                          reason="CreateResourceFailed", message=f"{exc}")
            logger.error(f"init jmeter cluster failed : {exc}")

        cluster.info(action="CreateCluster", reason="ResourcesCreated",
                     message="Dependency resources created, switching status to PENDING")
        cluster.set_status({
            "cluster": {
                "status": ClusterStatus.PENDING.value,
                "onlineInstances": 0,
                "lastProbeTime": base_utils.iso_time()
            }})


@kopf.on.field(GROUP, VERSION, JMETER_PLURAL, field="spec.command")
def on_jmeter_cluster_field_command(old, new, body: Body, logger: Logger, **kwargs):
    cluster = JmeterCluster(body)
    if not cluster.ready:
        logger.debug(f"Ignore spec.shell change for unready cluster")
    if cluster.parsed_spec.command is not None:
        with ClusterMutex(cluster, logger):
            cluster.parsed_spec.validate(logger)
            logger.info(
                f"update jmeter stateful_set , send bash to jmeter cluster with: {old} to {new}")
            cluster.send_script_to_cluster(new, logger)


@kopf.on.field(GROUP, VERSION, JMETER_PLURAL, field="spec.instances")
def on_jmeter_cluster_field_instances(old, new, body: Body, logger: Logger, **kwargs):
    cluster = JmeterCluster(body)
    if not cluster.ready:
        logger.debug(f"Ignore spec.instances change for unready cluster")
    with ClusterMutex(cluster, logger):
        cluster.parsed_spec.validate(logger)
        logger.info(
            f"update jmeter cluster instances : {old} to {new}")
        sts.update_size(cluster, new, logger)
        cluster.set_status({
            "cluster": {
                "status": ClusterStatus.PENDING.value,
                "onlineInstances": cluster.instances,
                "lastProbeTime": base_utils.iso_time()
            }})


@kopf.on.delete(GROUP, VERSION, JMETER_PLURAL)
def on_jmeter_cluster_delete(name: str, namespace: str, body: Body, logger: Logger, **kwargs):
    cluster = JmeterCluster(body)
    logger.info(f"delete jmeter cluster {namespace}/{name}")
    if cluster.get_share_config_storage() is not None:
        cluster.delete_storage_by_name(
            f"{cluster.name}-{DEFAULT_CONFIG_SUFFIX}")
    if cluster.get_share_data_storage() is not None:
        cluster.delete_storage_by_name(f"{cluster.name}-{DEFAULT_DATA_SUFFIX}")
