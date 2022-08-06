# Build Stage
FROM --platform=linux/amd64 ubuntu:20.04 as builder

## Install build dependencies.
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y cmake clang build-essential

## Add source code to the build stage.
ADD . /str
WORKDIR /str

## Build
RUN clang -fsanitize=fuzzer fuzz_library_api.c str.c

## Package Stage
FROM --platform=linux/amd64 ubuntu:20.04
COPY --from=builder /str/a.out /fuzz-str

## Make debug testsuite
RUN mkdir /testsuite && echo "seed" > /testsuite/seed

## Set up fuzzing!
ENTRYPOINT []
CMD /fuzz-str /testsuite
