

# jmeter-k8s
A performance testing tool based on kubernetes + jmeter.

# deploy

## 1. build jmeter operator images  and jmeter images

```
docker build -t jmeter-operator:latest -f Dockerfile .
cd deploy/images/
docker build -t jmeter:latest -f Dockerfile .
```


## 2. deploy jmeter operator and crd file

kubectl apply -f deploy/yaml/crd/crd.yaml

kubectl apply -f deploy/yaml/rbac/*.yaml

kubectl apply -f deploy/yaml/crd/deploy.yaml

## 3. deploy jmeter cluster



```
apiVersion: jmeter.zs.com/v1
kind: Jmeter
metadata:
  labels:
    app.kubernetes.io/name: jmeter
    app.kubernetes.io/instance: jmeter-sample
    app.kubernetes.io/part-of: jmeter
    app.kubernetes.io/managed-by: kustomize
    app.kubernetes.io/created-by: jmeter
  name: jmeter-sample-ceph
  namespace: middleware-jmeter
spec:
  instances: 3
  shareStorageTemplate:
    storageClassName: rook-cephfs
    accessModes:
      - ReadWriteMany
    resources:
      requests:
        storage: 1Gi
  dataStorageTemplate:
    storageClassName: rook-ceph-block # Must have shared storage
    accessModes:
      - ReadWriteOnce
    resources:
      requests:
        storage: 1Gi
```

## 4. Run Test Task

```
apiVersion: jmeter.zs.com/v1
kind: Jmeter
metadata:
  annotations:
    middleware.jmeter.application: mysqlpress
    middleware.jmeter.chargePerson: ""
    middleware.jmeter.configfile: ""
    middleware.jmeter.count: "5"
    middleware.jmeter.current.scriptfile: kunDB_insert.jmx   
    middleware.jmeter.measurement: mysql
    middleware.jmeter.reserveJTL: "false"
    middleware.jmeter.scriptfile: labels:
    app.kubernetes.io/created-by: jmeter
    app.kubernetes.io/instance: mysql
    app.kubernetes.io/managed-by: kustomize
    app.kubernetes.io/name: jmeter
    app.kubernetes.io/part-of: jmeter
  name: mysql
  namespace: middleware-jmeter
spec:
  command: sh start.sh -j kunDB_insert.jmx -t mysql-5.jtl -e 202308231508 -d 1 
  # need you upload jmx and jtl file to jmeter pod in shared dir  
  configStorage:
    accessModes:
    - ReadWriteMany
    resources:
      requests:
        storage: 1Gi
    storageClassName: rook-cephfs
  dataStorage:
    accessModes:
    - ReadWriteMany
    resources:
      requests:
        storage: 1Gi
    storageClassName: rook-cephfs
  instances: 1
```
## 5. view test result

### 5.1 grafana

you should be connect prometheus by jmeter jtl , and then you can view test result by grafana

### 5.2 web 

jmeter operator will create a service for web, you can access it by service ip and port 80 


