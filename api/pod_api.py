from logging import Logger
from threading import Thread
from typing import Optional
from utils.k8s_utils import api_core, api_client as api_exception
from kubernetes.stream import stream
import threading


def send_script_to_pod_async(ns: str, name: str, shell: str, logger: Logger) -> Optional[bool]:
    exec_command = [
        '/bin/sh',
        '-c',
        shell]
    try:
        resp = stream(
            api_core.connect_get_namespaced_pod_exec,
            name=name, namespace=ns, command=exec_command,
            stderr=True,
            stdin=False,
            stdout=True, tty=False,
            container="jmeter-cluster", _preload_content=True)
        logger.info(f"send script to pod , the resp value = {resp}")
        if resp is None:
            return False
        return True
    except api_exception as e:
        if e.status != 200:
            logger.error(
                f"send script to pod with {ns}/{name} error , err message with :{e}")


def send_script_to_pod(ns: str, name: str, shell: str, logger: Logger) -> Thread:
    t = threading.Thread(target=send_script_to_pod_async,
                         args=(ns, name, shell, logger))
    t.start()
    return t
