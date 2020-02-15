FROM virtlyst_build_env as builder

FROM alpine:3.11.3

RUN echo 'http://dl-cdn.alpinelinux.org/alpine/edge/testing' >> /etc/apk/repositories

RUN apk update && apk add \
  qt5-qtbase \
  qt5-qtbase-sqlite \
  libssh \
  grantlee \
  openssh-client \
  libvirt-libs

# Copy over cutelyst installation
COPY --from=builder /usr/local /usr/local

# Install cutelyst
#COPY cutelyst/build/cutelyst-0.1.1-Linux.deb /tmp/
#RUN apt install /tmp/cutelyst-0.1.1-Linux.deb && rm /tmp/cutelyst-0.1.1-Linux.deb

WORKDIR /usr/local/Virtlyst

#COPY Virtlyst/build/virtlsyt-0.1.1-Linux.tgz /tmp/
#RUN tar xzf /tmp/virtlsyt-0.1.1-Linux.tgz && rm /tmp/virtlsyt-0.1.1-Linux.tgz

#RUN echo "/usr/local/lib/x86_64-linux-gnu" > /etc/ld.so.conf.d/usr-local.conf \
#  && echo "/usr/local/Virtlyst" >> /etc/ld.so.conf.d/usr-local.conf \
#  && ldconfig

RUN echo "/lib" >> /etc/ld-musl-x86_64.path
RUN echo "/usr/lib" >> /etc/ld-musl-x86_64.path
RUN echo "/usr/local/lib" >> /etc/ld-musl-x86_64.path
RUN echo "/usr/local/lib64" >> /etc/ld-musl-x86_64.path

RUN mkdir /usr/local/Virtlyst/data /usr/local/Virtlyst/root
COPY Virtlyst/build/src/libVirtlyst.so /usr/local/Virtlyst/
COPY Virtlyst/root /usr/local/Virtlyst/root/
COPY Virtlyst/config.ini /usr/local/Virtlyst/

RUN ldconfig || /bin/true

EXPOSE 3000
VOLUME /usr/local/Virtlyst/data
CMD ["/usr/local/bin/cutelyst-wsgi2","--ini","config.ini"]
