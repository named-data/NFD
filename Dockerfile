FROM ghcr.io/named-data/ndn-cxx:latest as builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends libpcap-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /NFD

RUN cd /NFD \
    && ./waf configure --without-pch --prefix=/usr --sysconfdir=/etc --localstatedir=/var \
    && ./waf \
    && ./waf install \
    # get list of dependencies
    && mkdir -p /shlibdeps/debian && cd /shlibdeps && touch debian/control \
    && dpkg-shlibdeps --ignore-missing-info /usr/lib/libndn-cxx.so* /usr/bin/nfdc /usr/bin/nfd \
    && sed -n '/^shlibs:Depends=/ s|shlibs:Depends=||p' debian/substvars | sed -e 's|,||g' -e 's| ([^)]*)||g' > /deps.txt

# use same base distro version as named-data/ndn-cxx
FROM debian:bookworm

COPY --from=builder /deps.txt /
RUN apt-get update \
    && apt-get install -y --no-install-recommends $(cat /deps.txt) \
    && rm -rf /var/lib/apt/lists/* /deps.txt

COPY --from=builder /usr/lib/libndn-cxx.so* /usr/lib/
COPY --from=builder /usr/bin/nfd /usr/bin/
COPY --from=builder /usr/bin/nfdc /usr/bin/

COPY --from=builder /usr/bin/nfd-status-http-server /usr/bin/
COPY --from=builder /usr/share/ndn/ /usr/share/ndn/

COPY --from=builder /etc/ndn/nfd.conf.sample /config/nfd.conf

ENV HOME=/config

VOLUME /config
VOLUME /run/nfd

EXPOSE 6363/tcp
EXPOSE 6363/udp
EXPOSE 9696/tcp

ENTRYPOINT ["/usr/bin/nfd"]
CMD ["--config", "/config/nfd.conf"]
