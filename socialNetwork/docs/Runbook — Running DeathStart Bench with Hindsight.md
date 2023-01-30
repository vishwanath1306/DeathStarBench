# Runbook — Running DeathStart Bench with Hindsight

Repeat the partition for machine, docker install, and GitHub keys for all the machines. 

- - - -

## Creating a new partition for machine
All of these commands were tested and run on `c220gx` machines from Cloudlab. Easier to get the machines and also has 2 disks

```
$ sudo parted

The following commands need to be run inside the parted windo

$ select /dev/sdb
$ mklabel gpt
$ mkpart primary ext4 1MB 900GB
$ quit

This exits the parted window, and goes back to normal shell

$ sudo mkfs -t ext4 /dev/sdb
$ mkdir sdb
$ sudo mount /dev/sdb ~/sdb
$ sudo chown -R $USER ~/sdb

```

- - - -

## Installing Docker and Modifying Image location
When installing docker one of the problems is that the space for storing images is not enough, thus would require change in the default location where images are stored. 

```
$ sudo apt-get update

Apt Packages
$ sudo apt install -y apt-transport-https ca-certificates curl software-properties-common
$ sudo apt-get install -y \
    ca-certificates \
    curl \
    gnupg \
    lsb-release
```


### Docker official PGP Key

```
$ sudo mkdir -p /etc/apt/keyrings
$ curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
$ echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
```

### Installing Docker Engine

```
$ sudo apt-get update
$ sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin

```

### Post-Install Steps
```
$ ssudo groupadd docker
$ sudo usermod -aG docker $USER
$ newgrp docker
$ docker run hello-world
$ sudo chmod 666 /var/run/docker.sock
```

### Changing the location of the docker images

Since the space in the root storage isn’t enough for storing all the images, you need to allocate extra space for the docker image. 

```
$ sudo systemctl stop docker.service
$ sudo systemctl stop docker.socket
$ sudo vim /lib/systemd/system/docker.service
```

Change the `ExecStart `  command to have the `-g` flag which enables Docker to store the images in a different location. 

```

ExecStart=/usr/bin/dockerd -g /users/vishwa/sdb/docker -H fd://
$ sudo mkdir /new/path/to/docker
$ sudo rsync -aqxP /var/lib/docker/ /new/path/docker
$ 
$ sudo systemctl start docker
```

To verify that the change is reflected and is working
```
$ ps aux | grep -i docker | grep -v grep

Look for "-g /new/path/docker" in the output
```


- - - -

## Downloading the GitHub Repo
Currently, Github you can download it using HTTPS or using SSH. If you’re only running the experiments, and don’t want to push to main repo, use HTTPS otherwise use SSH. 

### Adding SSH keys

Run `ssh-keygen` and get a public/private key pair. 

Copy and paste the public key `.pub` file to [Adding New GitHub SSH Key](https://github.com/settings/ssh/new)

### Downloading the GitHub Repo

For this, we’re going to be using Vish’s clone of the DSB repository because it has the hindsight integration already present. 

```
git clone git@github.com:vishwanath1306/DeathStarBench.git
```

- - - -
## Running with Docker Compose
To run this, we’re assuming you’re using Vish’s clone of DSB. It has all the necessary scripts written under the `scripts` folder. This makes sure you don’t need to scrape the internet for specific install commands. 

All the scripts need to be run in `socialNetwork` folder of the repository. 

### Pre-Run Steps

**Installing Docker Compose**

```
$ ./scripts/install_compose.sh
```

To check if it is installed use `$ docker compose version`

**Installing Other Prereq**

This installs Lua, Luasocket, Pip, and aiohttp for the social graph install. 

```
$ ./scripts/install_prereq.sh
```


### Creating Docker Containers

Once everything is installed, we need to build the docker containers for hindsight and socialNetwork + Hindsight. 

**Creating the Hindsight Container**
```
$ docker build -f Dockerfile-hindsight -t hindsight_tracing .
```

**Creating Hindsight + SocialNetwork Combo**
```
$ docker build -t dsbhsinteg .
```

### Deploying DSB + HS

Once the containers are built, deploy the system using the following command. Use the `-d` flag to run the cluster in background. 

```
$ docker compose up [-d]
```

**Optional: Deploying the graphs**

DSB SN comes with a bunch of datasets which we can load into the system. This makes it easier to load test. Run this script after deploying the cluster. 

```
$ python3 scripts/init_social_graph.py --graph=<socfb-Reed98, ego-twitter, or soc-twitter-follows-mun>
$ python3 scripts/init_social_graph.py --graph=socfb-Reed98
```

I mostly use the `socfb-Reed98`, but use whatever. 

## Stopping the DSB + HS cluster
Run the following command in `socialNetwork` folder. 
 
```
$ docker compose down
```

- - - -
## Workload Generator
There is an inbuilt workload generator for DSB developed by them under the `wrk2` folder in the `socialNetwork` repo. You can use it for generating load for the system. 

Make sure you’re running the load generator in a different node as opposed to what you have the original code. Essentially do not run it on the same place where you’re running the DSB application. 

If you’re using Cloudlab, make sure to get 2+ machines, with 1 dedicated for load generator. This makes sure that all the machines are in the same underlying network. 

### Compiling WRK

```
$ cd wrk2
$ make
``` 

### Load Generation Scripts

Run these inside the `wrk2` folder.  

Get the IP by typing `ifconfig` and getting the interface of `ens1f0`. 

**Compose Post API**
```
./wrk -D exp -t 1 -c 2 -d 30 -R 2 -L -s ./scripts/social-network/compose-post.lua http://10.10.1.1:8080/wrk2-api/post/compose
```

**Read Home Timeline API**
```
./wrk -D exp -t 1 -c 2 -d 30 -R 2 -L -s ./scripts/social-network/read-home-timeline.lua http://10.10.1.1:8080/wrk2-api/home-timeline/read
```

**Read User Timeline API**
```
./wrk -D exp -t 1 -c 2 -d 30 -R 2 -L -s ./scripts/social-network/read-user-timeline.lua http://10.10.1.1:8080/wrk2-api/user-timeline/read

```

- - - -
## Deploying DSB with Docker Swarm

Basic concept is that there’s `master` and `worker` nodes. In DSB, we’ll have one master node and others will be worker nodes. 

### Setting up the master node

```
$ docker swarm init --advertise-addr <MANAGER-IP>
```

When using Cloudlab, make sure that you run it on `node0`, with other nodes being the worker processes. More complex assignments with deploying services using node constraints will be done later. 

### Setting up the worker nodes
After setting up the master node, you will get a link for setting up the worker nodes. Those will be as follows

```
$ docker swarm join --token SWMTKN-1-3nlcxya9eya42798bt44cmb7f2zav8xrme5ddy61xzve0w4hmc-cinv0n915xa2649u0234ougxb 10.10.1.1:2377
```

Here `10.10.1.1` is the IP of `node0` in my cluster. Be careful with the IP of your system. Check it before using this command. 

### Deploying the cluster

```
$ docker stack deploy --compose-file=docker-compose-swarm.yml <cluster-name>
```

### Bringing Cluster down
```
$ docker stack rm <cluster-name>
```

#projects/heimdall
