FROM ubuntu:18.04 AS build
MAINTAINER Christian Parpart <christian@parpart.family>

RUN apt-get -qqy update
RUN apt-get install -qqy cmake make g++-7

WORKDIR /app/src

COPY /3rdparty /app/src/3rdparty
COPY /cmake /app/src/cmake
COPY /src /app/src/src
COPY /CMakeLists.txt $WORKDIR
RUN ls -hlaF

ARG BUILD_CONCURRENCY="0"

RUN cmake -DCMAKE_BUILD_TYPE=Release \
          -DKLEX_EXAMPLES=OFF \
          -DKLEX_TESTS=OFF \
          -DMKLEX_LINK_STATIC=ON \
          -DCMAKE_CXX_COMPILER=g++-7 \
          $WORKDIR

RUN make \
    -j$(awk "BEGIN {                                       \
        if (${BUILD_CONCURRENCY} != 0) {                   \
            print(${BUILD_CONCURRENCY});                   \
        } else {                                           \
            x=($(grep -c ^processor /proc/cpuinfo) * 2/3); \
            if (x > 1) {                                   \
                printf(\"%d\n\", x);                       \
            } else {                                       \
                print(1);                                  \
            }                                              \
        }                                                  \
    }")

RUN strip mklex

FROM scratch
COPY --from=build /app/src/mklex /usr/bin/mklex
ENTRYPOINT ["/usr/bin/mklex"]
CMD ["--help"]
