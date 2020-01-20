
FROM cmc_build_env as builder

ARG CUTELYST_VERSION=v2.7.0
ARG VIRTLYST_VERSION=v1.1.0

# Start with a clean image
FROM debian:stretch

# Install dependencies
RUN apt-get update \
  && apt-get install -y \
  libqt5core5a libqt5network5 libqt5sql5 libqt5sql5-sqlite libqt5sql5-psql \
  libssh-dev libqt5xml5 libvirt0 libgrantlee-templates5 openssh-client \
  && apt-get clean

# Copy over cutelyst installation
COPY --from=builder /usr/local /usr/local

WORKDIR /usr/local/Virtlyst

COPY Virtlyst/build/virtlsy-0.1.1-Linux.deb /tmp/
RUN apt install /tmp/virtlsy-0.1.1-Linux.deb && rm /tmp/virtlsy-0.1.1-Linux.deb

RUN echo "/usr/local/lib/x86_64-linux-gnu" > /etc/ld.so.conf.d/usr-local.conf \
  && echo "/usr/local/Virtlyst" >> /etc/ld.so.conf.d/usr-local.conf \
  && ldconfig

EXPOSE 3000
VOLUME /usr/local/Virtlyst/data
CMD ["/usr/local/bin/cutelyst-wsgi2","--ini","config.ini"]
