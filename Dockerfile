FROM alpine:3.24 AS builder

RUN apk update \
    && apk --no-cache add build-base \
    boost-dev \
    cmake \
    pkgconfig \
    protobuf-dev \
    opus-dev \
    speexdsp-dev \
    pjproject-dev \
    git \
    && apk --no-cache add log4cpp-dev --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/

COPY . /mumsi/

WORKDIR /mumsi

# Build mumlib & mumsi
RUN mkdir build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Debug .. \
    && make -j

FROM alpine:3.24 AS runner

RUN apk update \
    && apk --no-cache add \
    build-base gdb \
    boost-dev \
    cmake \
    pkgconfig \
    protobuf-dev \
    opus-dev \
    speexdsp-dev \
    pjproject-dev \
    boost \
    protobuf \
    opus \
    speexdsp \
    pjproject \
    && apk --no-cache add log4cpp log4cpp-dev --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/

RUN mkdir app
VOLUME /config
EXPOSE 5060

WORKDIR /config

COPY --from=builder /mumsi/build/mumsi /app
COPY media/*.wav /app/media/

CMD ["/app/mumsi", "config.ini"]
