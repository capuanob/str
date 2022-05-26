# Build Stage
FROM --platform=linux/amd64 ubuntu:20.04 as builder

## Install build dependencies.
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y cmake clang git build-essential

## Add source code to the build stage.
WORKDIR /
RUN git clone https://github.com/capuanob/str.git
WORKDIR /str
RUN git checkout mayhem

## Build
RUN clang -fsanitize=fuzzer fuzz.c str.c

## Package Stage
FROM --platform=linux/amd64 ubuntu:20.04
COPY --from=builder /gif-h/a.out /fuzz-str

## Make debug corpus
RUN mkdir /corpus && echo "seed" > /corpus/seed

## Set up fuzzing!
ENTRYPOINT []
CMD /fuzz-str /corpus -close_fd_mask=2
