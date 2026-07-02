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
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    && cmake --build /build --target SolarScape SolarScapeNSGAII SolarScapeMOEAD -j "$(nproc)"


FROM ubuntu:24.04 AS experiments

ENV DEBIAN_FRONTEND=noninteractive \
    PYTHONUNBUFFERED=1

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libboost-json-dev \
    libboost-program-options-dev \
    libtbb12 \
    libyaml-cpp-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/solarscape

RUN mkdir -p /opt/solarscape/bin /opt/solarscape/scenarios /opt/solarscape/tools

COPY --from=builder /build/SolarScape /opt/solarscape/bin/SolarScape
COPY --from=builder /build/SolarScapeNSGAII /opt/solarscape/bin/SolarScapeNSGAII
COPY --from=builder /build/SolarScapeMOEAD /opt/solarscape/bin/SolarScapeMOEAD

COPY scenario1.yml scenario2.yml scenario3.yml /opt/solarscape/scenarios/
COPY tools/run_experiments.py /opt/solarscape/tools/run_experiments.py
COPY tools/solarscape_tools /opt/solarscape/tools/solarscape_tools

VOLUME ["/data"]

ENTRYPOINT ["python3", "/opt/solarscape/tools/run_experiments.py", "--executables-dir", "/opt/solarscape/bin", "--scenarios-dir", "/opt/solarscape/scenarios", "--output-dir", "/data/experiments"]
CMD ["--runs", "5", "--jobs", "1"]
