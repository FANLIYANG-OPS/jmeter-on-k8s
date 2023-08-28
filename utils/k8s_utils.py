from kubernetes import client, config
from kubernetes.client.rest import ApiException

try:
    # outside k8s
    config.load_kube_config()
except config.config_exception.ConfigException:
    try:
        # inside a k8s pod
        config.load_incluster_config()
    except config.config_exception.ConfigException:
        raise Exception(
            "Could not configure kubernetes python client")

api_client: client.ApiClient = client.ApiClient()
api_core: client.CoreV1Api = client.CoreV1Api()
api_custom_obj: client.CustomObjectsApi = client.CustomObjectsApi()
api_apps: client.AppsV1Api = client.AppsV1Api()


def k8s_version() -> str:
    api_instance = client.VersionApi(api_client)
    api_response = api_instance.get_code()
    return f"{api_response.major}.{api_response.minor}"
