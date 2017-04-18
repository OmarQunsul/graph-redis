FROM ubuntu:17.04

RUN groupadd -r redis && useradd -r -g redis redis

RUN apt-get update && apt-get install -y --no-install-recommends \
		ca-certificates \
		wget \
    build-essential \
	&& rm -rf /var/lib/apt/lists/*

ADD . /usr/src/redis
WORKDIR /usr/src/redis
RUN make distclean
RUN make
RUN make install

RUN mkdir /data && chown redis:redis /data
VOLUME /data
WORKDIR /data

RUN mkdir -p /etc/redis
ADD *.conf /etc/redis/

EXPOSE 6379
CMD [ "redis-server", "/etc/redis/redis.conf" ]
