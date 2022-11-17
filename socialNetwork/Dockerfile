FROM yg397/thrift-microservice-deps:xenial AS builder

ARG LIB_REDIS_PLUS_PLUS_VERSION=1.2.3

ARG SSH_PRIVATE_KEY

RUN apt-get update \
    && apt-get install -y automake bison flex g++ git libboost-all-dev \
    libevent-dev libssl-dev libtool make pkg-config supervisor ssh

# Apply patch and re-install Redis plus plus
RUN cd /tmp/redis-plus-plus\
&& sed -i '/Transaction transaction/i\\    ShardsPool* get_shards_pool(){\n        return &_pool;\n    }\n' \
   src/sw/redis++/redis_cluster.h \
&& cmake -DREDIS_PLUS_PLUS_USE_TLS=ON . \
&& make -j$(nproc) \
&& make install

# Installing Golang
COPY --from=golang:latest /usr/local/go /usr/local/go
ENV PATH="/usr/local/go/bin:${PATH}"
# Actual hindsight install
RUN mkdir /hindsight
WORKDIR /hindsight
RUN git clone https://gitlab.mpi-sws.org/cld/tracing/hindsight.git ; \
    cd hindsight/client ; \
    make && make install; ldconfig ;\
    cd ../agent && go build -o agent cmd/agent2/main.go; \
    mv agent /usr/local/bin/ ;

COPY ./ /social-network-microservices
RUN cd /social-network-microservices \
    && mkdir -p build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Debug .. \
    && make -j$(nproc) \
    && make install \
    && cp -r /social-network-microservices/hindsight_conf/* /etc/hindsight_conf \
    && cp -r /social-network-microservices/supervisor_conf /etc

WORKDIR /social-network-microservices