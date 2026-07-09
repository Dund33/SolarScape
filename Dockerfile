FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    libboost-json-dev \
    libboost-program-options-dev \
    libtbb-dev \
    libyaml-cpp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B /build \
    -DCMAKE_BUILD_TYPE=Release \
    -DSOLARSCAPE_ENABLE_NATIVE_OPTIMIZATIONS=OFF \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    && cmake --build /build --target SolarScape SolarScapeNSGAII SolarScapeNSGAIII SolarScapeMOEAD -j "$(nproc)"


FROM debian:trixie-slim AS experiments

ENV DEBIAN_FRONTEND=noninteractive \
    PYTHONUNBUFFERED=1

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libboost-json1.83.0 \
    libboost-program-options1.83.0 \
    libtbb12 \
    libyaml-cpp0.8 \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/solarscape

RUN mkdir -p /opt/solarscape/bin /opt/solarscape/scenarios /opt/solarscape/tools

COPY --from=builder /build/SolarScape /opt/solarscape/bin/SolarScape
COPY --from=builder /build/SolarScapeNSGAII /opt/solarscape/bin/SolarScapeNSGAII
COPY --from=builder /build/SolarScapeNSGAIII /opt/solarscape/bin/SolarScapeNSGAIII
COPY --from=builder /build/SolarScapeMOEAD /opt/solarscape/bin/SolarScapeMOEAD

COPY scenario1.yml scenario2.yml scenario3.yml /opt/solarscape/scenarios/
COPY tools/run_experiments.py /opt/solarscape/tools/run_experiments.py
COPY tools/run_experiments_entrypoint.sh /opt/solarscape/tools/run_experiments_entrypoint.sh
COPY tools/solarscape_tools /opt/solarscape/tools/solarscape_tools
RUN chmod +x /opt/solarscape/tools/run_experiments_entrypoint.sh

VOLUME ["/data"]

ENTRYPOINT ["/opt/solarscape/tools/run_experiments_entrypoint.sh"]
CMD ["--runs", "5"]
