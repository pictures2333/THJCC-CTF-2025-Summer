FROM ubuntu:20.04@sha256:e5a6aeef391a8a9bdaee3de6b28f393837c479d8217324a2340b64e45a81e0ef

RUN apt-get update && \
    DEBAIN_FRONTEND=noninteractive apt-get install -qy gcc gcc-multilib

RUN useradd -m chal
RUN chown -R root:root /home/chal
RUN chmod -R 755 /home/chal