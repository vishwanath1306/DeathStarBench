# Useful commands to run the repo

## [OPTIONAL] Cloudlab Steps

Cloudlab doesn't have much root space, thus the need to create a new filesystem on the disk available. Use the `c220g1` instance if possible, or geting another instance with more than 1 disk. 

TODO: Need to finish the section to create partition

```
sudo parted 
```



## Compilation and Deployment

Building the Hindisight coordinator

```
docker build -f Dockerfile-hindsight -t hindsight_tracing .
```

Building the social network services
```
docker build -t dsbhsinteg .
```


## Running Workloads

Example workloads which can be triggered for testing, not for result collection.

*Compose Post Workload*
```
./wrk -D exp -t 1 -c 2 -d 30 -L -s ./scripts/social-network/compose-post.lua http://10.10.1.1:8080/wrk2-api/post/compose -R 2
```