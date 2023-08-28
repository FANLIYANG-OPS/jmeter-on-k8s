from logging import Logger

import kopf
from api.crd import JmeterCluster
from utils.k8s_utils import api_apps
from consts.config import DEFAULT_CONFIG_SUFFIX, DEFAULT_DATA_SUFFIX
import yaml


def update_share_storage(cluster: JmeterCluster, storage: dict, logger: Logger):
    deploy = cluster.get_stateful_set()
    if deploy:
        patch = {
            "spec": {
                "volumeClaimTemplates": storage
            }
        }
        logger.info(f"update storage with:{patch}")
        api_apps.patch_namespaced_stateful_set(
            deploy.metadata.name, deploy.metadata.namespace, body=patch)


def update_size(cluster: JmeterCluster, size: int, logger: Logger):
    deploy = cluster.get_stateful_set()
    if deploy:
        patch = {
            "spec": {
                "replicas": size
            }
        }
        api_apps.patch_namespaced_stateful_set(
            deploy.metadata.name, deploy.metadata.namespace, body=patch)
    else:
        logger.info("create stateful_set with replicas={size}")
        deploy = prepare_stateful_set(cluster)
        kopf.adopt(deploy)
        api_apps.create_namespaced_stateful_set(
            namespace=cluster.namespace, body=deploy)


def prepare_stateful_set(cluster: JmeterCluster) -> dict:
    spec = cluster.parsed_spec
    tmpl = f"""
                apiVersion: apps/v1
                kind: StatefulSet
                metadata:
                  labels:
                    app: {spec.name}
                    app.kubernetes.io/created-by: jmeter
                    app.kubernetes.io/instance: {spec.name}
                    app.kubernetes.io/managed-by: kustomize
                    app.kubernetes.io/name: jmeter
                    app.kubernetes.io/part-of: jmeter
                    app.kubernetes.io/type: sts
                  name: {spec.name}
                  namespace: {spec.namespace}
                spec:
                  podManagementPolicy: Parallel
                  replicas: {spec.instances}
                  selector:
                    matchLabels:
                      app: {spec.name}
                      app.kubernetes.io/created-by: jmeter
                      app.kubernetes.io/instance: {spec.name}
                      app.kubernetes.io/managed-by: kustomize
                      app.kubernetes.io/name: jmeter
                      app.kubernetes.io/part-of: jmeter
                      app.kubernetes.io/type: sts
                  strategy:
                    rollingUpdate:
                      maxSurge: 25%
                      maxUnavailable: 25%
                    type: RollingUpdate
                  template:
                    metadata:
                      labels:
                        app: {spec.name}
                        app.kubernetes.io/created-by: jmeter
                        app.kubernetes.io/instance: {spec.name}
                        app.kubernetes.io/managed-by: kustomize
                        app.kubernetes.io/name: jmeter
                        app.kubernetes.io/part-of: jmeter
                        app.kubernetes.io/type: sts
                    spec:
                      containers:
                      - command:
                        - /bin/sh
                        - -c
                        - sleep 1000000
                        image: library/jmeter:5.4.1
                        imagePullPolicy: Always
                        name: jmeter-cluster
                        env:
                        - name: CLUSTER_NAME
                          value: {spec.name}
                        resources:
                          limits:
                            cpu: "2"
                            memory: 6G
                          requests:
                            cpu: "2"
                            memory: 6G
                        livenessProbe:
                          failureThreshold: 3
                          initialDelaySeconds: 20
                          periodSeconds: 5
                          timeoutSeconds: 5
                          exec:
                            command:
                            - ls
                            - /opt/report
                        volumeMounts:
                        - name: share-config
                          mountPath: /opt/config
                        - name: share-data
                          mountPath: /opt/report
                      dnsPolicy: ClusterFirst
                      restartPolicy: Always
                      schedulerName: default-scheduler    
                      volumes:
                      - name: share-config
                        persistentVolumeClaim:
                          claimName: {spec.name}-{DEFAULT_CONFIG_SUFFIX}
                      - name: share-data
                        persistentVolumeClaim:
                          claimName: {spec.name}-{DEFAULT_DATA_SUFFIX}          
    """
    stateful_set = yaml.safe_load(tmpl)
    return stateful_set
