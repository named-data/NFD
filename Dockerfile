# syntax=docker/dockerfile:1

ARG NDN_CXX_VERSION=latest
FROM ghcr.io/named-data/ndn-cxx-build:${NDN_CXX_VERSION} AS build
ARG SOURCE_DATE_EPOCH

RUN apt-get install -Uy --no-install-recommends \
        libpcap-dev \
    # use 'apt-get distclean' when we upgrade to ubuntu:24.04
    && rm -rf /var/lib/apt/lists/*

ARG JOBS
RUN --mount=type=bind,rw,target=/src <<EOF
set -eux
cd /src
./waf configure \
    --prefix=/usr \
    --libdir=/usr/lib \
    --sysconfdir=/etc \
    --localstatedir=/var \
    --sharedstatedir=/var \
    --without-systemd
./waf build
./waf install

mkdir -p /deps/debian
touch /deps/debian/control
cd /deps
for binary in nfd nfdc nfd-autoreg; do
    dpkg-shlibdeps --ignore-missing-info "/usr/bin/${binary}" -O \
        | sed -n 's|^shlibs:Depends=||p' | sed 's| ([^)]*),\?||g' > "${binary}"
done
EOF


FROM ghcr.io/named-data/ndn-cxx-runtime:${NDN_CXX_VERSION} AS nfd-autoreg

COPY --link --from=build /usr/bin/nfd-autoreg /usr/bin/

RUN --mount=type=bind,from=build,source=/deps,target=/deps \
    apt-get install -Uy --no-install-recommends $(cat /deps/nfd-autoreg) \
    && rm -rf /var/lib/apt/lists/*

VOLUME /run/nfd

ENTRYPOINT ["/usr/bin/nfd-autoreg"]


FROM ghcr.io/named-data/ndn-cxx-runtime:${NDN_CXX_VERSION} AS nfd-status-http-server

COPY --link --from=build /usr/bin/nfdc /usr/bin/
COPY --link --from=build /usr/bin/nfd-status-http-server /usr/bin/
COPY --link --from=build /usr/share/ndn/ /usr/share/ndn/

RUN --mount=type=bind,from=build,source=/deps,target=/deps \
    apt-get install -Uy --no-install-recommends $(cat /deps/nfdc) \
        python3 \
    && rm -rf /var/lib/apt/lists/*

VOLUME /run/nfd

EXPOSE 8080/tcp

ENTRYPOINT ["/usr/bin/nfd-status-http-server", "--address", "0.0.0.0"]


FROM ghcr.io/named-data/ndn-cxx-runtime:${NDN_CXX_VERSION} AS nfd

COPY --link --from=build /usr/bin/nfdc /usr/bin/
COPY --link --from=build /usr/bin/nfd /usr/bin/
COPY --link --from=build /etc/ndn/nfd.conf.sample /config/nfd.conf

RUN --mount=type=bind,from=build,source=/deps,target=/deps \
    apt-get install -Uy --no-install-recommends $(cat /deps/nfd /deps/nfdc) \
    && rm -rf /var/lib/apt/lists/*

ENV HOME=/config
VOLUME /config
VOLUME /run/nfd

EXPOSE 6363/tcp 6363/udp 9696/tcp

ENTRYPOINT ["/usr/bin/nfd"]
CMD ["--config", "/config/nfd.conf"]
