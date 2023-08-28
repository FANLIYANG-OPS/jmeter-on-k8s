import os

from utils.k8s_utils import k8s_version

OPERATOR_VERSION = "1.0.0"
SHELL_VERSION = "5.4.1"
DEFAULT_VERSION_TAG = "5.4.1"
DEFAULT_OPERATOR_VERSION_TAG = "1.0.0"
DEFAULT_IMAGE_REPOSITORY = os.getenv("JMETER_OPERATOR_DEFAULT_REPOSITORY", default="jmeter").rstrip('/')
DEFAULT_DATA_SUFFIX = "share-data"
DEFAULT_CONFIG_SUFFIX = "share-config"


def log_config_banner(logger) -> None:
    logger.info(f"KUBERNETES_VERSION ={k8s_version()}")
    logger.info(f"OPERATOR_VERSION   ={OPERATOR_VERSION}")
    logger.info(f"SHELL_VERSION      ={SHELL_VERSION}")
    logger.info(f"DEFAULT_VERSION_TAG={DEFAULT_VERSION_TAG}")
    logger.info(f"SIDECAR_VERSION_TAG={DEFAULT_OPERATOR_VERSION_TAG}")
    logger.info(f"DEFAULT_IMAGE_REPOSITORY   ={DEFAULT_IMAGE_REPOSITORY}")
