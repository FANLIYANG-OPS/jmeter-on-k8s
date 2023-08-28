import yaml

from api.crd import JmeterCluster


def prepare_service(cluster: JmeterCluster) -> dict:
    spec = cluster.parsed_spec
    temp = f"""
    apiVersion: v1
    kind: Service
    metadata:
      labels:
        app: {spec.name}-nginx
        app.kubernetes.io/created-by: jmeter
        app.kubernetes.io/managed-by: kustomize
        app.kubernetes.io/name: jmeter
        app.kubernetes.io/part-of: jmeter
      name: {spec.name}-svc
      namespace: {spec.namespace}
    spec:
      externalTrafficPolicy: Cluster
      ipFamilies:
      - IPv4
      ipFamilyPolicy: SingleStack
      ports:
      - name: tcp-80
        port: 80
        protocol: TCP
        targetPort: 80
      - name: tcp-8080
        port: 8080
        protocol: TCP
        targetPort: 8080
      selector:
        app.kubernetes.io/instance: {spec.name}-nginx
      sessionAffinity: None
      type: LoadBalancer
    """
    service = yaml.safe_load(temp)
    return service
