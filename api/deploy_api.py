from api.crd import JmeterCluster
import yaml
from consts.config import DEFAULT_DATA_SUFFIX, DEFAULT_CONFIG_SUFFIX


def prepare_deployment(cluster: JmeterCluster) -> dict:
    spec = cluster.parsed_spec
    temp = f"""
    apiVersion: apps/v1
    kind: Deployment
    metadata:
      name: {spec.name}-nginx
      namespace: {spec.namespace}
      labels:
        app: {spec.name}
        app.kubernetes.io/created-by: jmeter
        app.kubernetes.io/instance: {spec.name}-nginx
        app.kubernetes.io/managed-by: kustomize
        app.kubernetes.io/name: jmeter
        app.kubernetes.io/part-of: jmeter
    spec:
      replicas: 1
      selector:
        matchLabels:
          app: {spec.name}
          app.kubernetes.io/created-by: jmeter
          app.kubernetes.io/instance: {spec.name}-nginx
          app.kubernetes.io/managed-by: kustomize
          app.kubernetes.io/name: jmeter
          app.kubernetes.io/part-of: jmeter
      template:
        metadata:
          labels:
            app: {spec.name}
            app.kubernetes.io/created-by: jmeter
            app.kubernetes.io/instance: {spec.name}-nginx
            app.kubernetes.io/managed-by: kustomize
            app.kubernetes.io/name: jmeter
            app.kubernetes.io/part-of: jmeter
        spec:
          volumes:
          - name: share-data
            persistentVolumeClaim:
              claimName: {spec.name}-{DEFAULT_DATA_SUFFIX}
          - name: share-config
            persistentVolumeClaim:
              claimName: {spec.name}-{DEFAULT_CONFIG_SUFFIX}
          - name: nginx-config
            configMap:
              defaultMode: 420
              items:
              - key: nginx.conf
                path: nginx.conf
              name: {spec.name}-config
          containers:
          - name: nginx
            image: library/nginx:1.24.0-up
            imagePullPolicy: Always
            ports:
            - containerPort: 80
              name: tcp-80
              protocol: TCP
            - containerPort: 8080
              name: tcp-8080
              protocol: TCP
            livenessProbe:
              failureThreshold: 3
              initialDelaySeconds: 20
              periodSeconds: 5
              timeoutSeconds: 5
              exec:
                command:
                - ls
                - /opt/report
            resources:
              limits:
                cpu: "2"
                memory: 4G
              requests:
                cpu: "1"
                memory: 2G
            ports:
            - containerPort: 80
            volumeMounts:
            - name: share-data
              mountPath: /opt/report
            - name: share-config
              mountPath: /opt/config
            - name: nginx-config
              mountPath: /etc/nginx/nginx.conf
              readOnly: true
              subPath: nginx.conf              
    """
    deployment = yaml.safe_load(temp)
    return deployment
