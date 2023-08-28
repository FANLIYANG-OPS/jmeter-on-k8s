import datetime
import json
import threading
import time
import os
from logging import Logger

from consts import config


class EphemeralState:
    def __init__(self):
        self.data = {}
        self.lock = threading.Lock()

    def get(self, mutex_key: str):
        with self.lock:
            return self.data.get(mutex_key)

    def set(self, mutex_key: str, val: str, logger: Logger) -> None:
        logger.info(f"add mutex to cluster : key = {mutex_key}, val = {val}")
        with self.lock:
            self.data[mutex_key] = val

    def test_set(self, mutex_key: str, val: str, logger: Logger) -> None:
        with self.lock:
            old = self.data.get(mutex_key)
            if old is None:
                logger.info(
                    f"cluster mutex not exist , add it : key = {mutex_key}, val = {val}")
                self.data[mutex_key] = val
        return old


get_ephemeral_state = EphemeralState()


def log_banner(path: str, logger) -> None:
    import pkg_resources

    kopf_version = pkg_resources.get_distribution('kopf').version
    ts = datetime.datetime.fromtimestamp(os.stat(path).st_mtime).isoformat()

    path = os.path.basename(path)
    logger.info(
        f"Jmeter Operator/{path}={config.OPERATOR_VERSION} timestamp={ts} kopf={kopf_version} uid={os.getuid()}")


def iso_time() -> str:
    return datetime.datetime.utcnow().replace(microsecond=0).isoformat() + "Z"


def dict_to_json_string(d: dict) -> str:
    return json.dumps(d, indent=4)


def merge_patch_object(base: dict, patch: dict, prefix: str = "", key: str = "") -> None:
    """

    :rtype: object
    """
    assert not key, "not implemented"  # TODO support key

    if type(base) != type(patch):
        raise ValueError(f"Invalid type in patch at {prefix}")
    if type(base) != dict:
        raise ValueError(f"Invalid type in base at {prefix}")

    def get_named_object(l, name):
        for o in l:
            assert type(o) == dict, f"{prefix}: {name} = {o}"
            if o["name"] == name:
                return o
        return None

    for k, v in patch.items():
        ov = base.get(k)
        if ov is not None:
            if type(ov) == dict:
                if type(v) != dict:
                    # TODO
                    raise ValueError(f"Invalid type in {prefix}")
                else:
                    merge_patch_object(ov, v, prefix + "." + k)
            elif type(ov) == list:
                if type(v) != list:
                    # TODO
                    raise ValueError(f"Invalid type in {prefix}")
                else:
                    if not ov:
                        base[k] = v
                    else:
                        if type(v[0]) != dict:
                            base[k] = v
                        else:
                            for i, elem in enumerate(v):
                                if type(elem) != dict:
                                    raise ValueError(
                                        f"Invalid type in {prefix}")
                                name = elem.get("name")
                                if not name:
                                    raise ValueError(
                                        "Object in list must have name")
                                o = get_named_object(ov, name)
                                if o:
                                    merge_patch_object(
                                        o, elem, prefix + "." + k + "[" + str(i) + "]")
                                else:
                                    ov.append(elem)

            elif type(ov) not in (dict, list) and type(v) in (dict, list):
                raise ValueError(f"Invalid type in {prefix}")
            else:
                base[k] = v
        else:
            base[k] = v
