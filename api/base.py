from typing import Optional

from utils.k8s_utils import api_core

import datetime

g_component = None
g_host = None


class K8sInterfaceObject:
    def __init__(self) -> None:
        pass

    @property
    def name(self) -> str:
        raise NotImplemented()

    @property
    def namespace(self) -> str:
        raise NotImplemented()

    def self_ref(self, field: Optional[str] = None) -> dict:
        raise NotImplemented()

    def info(self, *, action: str, reason: str, message: str,
             field: Optional[str] = None) -> None:
        post_event(self.namespace, self.self_ref(field), t="Normal",
                   action=action, reason=reason, message=message)

    def warn(self, *, action: str, reason: str, message: str,
             field: Optional[str] = None) -> None:
        post_event(self.namespace, self.self_ref(field), t="Warning",
                   action=action, reason=reason, message=message)

    def error(self, *, action: str, reason: str, message: str,
              field: Optional[str] = None) -> None:
        post_event(self.namespace, self.self_ref(field), t="Error",
                   action=action, reason=reason, message=message)


def post_event(namespace: str, object_ref: dict, t: str, action: str,
               reason: str, message: str) -> None:
    if len(message) > 1024:
        message = message[:1024]

    body = {
        'action': action,

        'eventTime': datetime.datetime.now().isoformat() + "Z",
        'involvedObject': object_ref,
        'message': message,
        'metadata': {
            'namespace': namespace,
            'generateName': 'jmeter-operator-evt-',
        },

        # This should be a short, machine understandable string that gives the
        # reason for the transition into the object's current status.
        'reason': reason,

        'reportingComponent': f'jmeter.zs.com/jmeter-operator-{g_component}',
        'reportingInstance': f'{g_host}',

        'source': {
            'component': g_component,
            'host': g_host
        },

        'type': t
    }
    api_core.create_namespaced_event(namespace, body)
