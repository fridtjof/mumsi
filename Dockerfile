FROM alpine:3.16 AS builder

RUN apk update \
    && apk --no-cache add build-base \
    boost-dev \
    cmake \
    pkgconfig \
    protobuf-dev \
    opus-dev \
    speexdsp-dev \
    pjproject-dev \
    # musl doesn't ship execinfo.h
    libexecinfo-dev \
    git \
    && apk --no-cache add log4cpp-dev --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/

COPY . mumsi/

WORKDIR mumsi

# Build mumlib & mumsi
RUN mkdir build && cd build \
    && cmake .. \
    && make -j

FROM alpine:3.16 AS runner

RUN apk update \
    && apk --no-cache add \
    boost \
    protobuf \
    opus \
    speexdsp \
    pjproject \
    libexecinfo \
    && apk --no-cache add log4cpp --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/

RUN mkdir app
VOLUME /config
EXPOSE 5060

WORKDIR /config

COPY --from=builder /mumsi/build/mumsi /app
COPY media/*.wav /app/media/

CMD ["/app/mumsi", "config.ini"]
