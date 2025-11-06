FROM ubuntu:22.04

# Install dependencies and GCC-12
RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y g++-12 gcc-12 cmake libboost-all-dev && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 60 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 60

WORKDIR /app
COPY . .

# Compile executables
RUN g++ -std=c++20 -O2 gen_data.cpp DPF.cpp -o gen_data
RUN g++ -std=c++20 -O2 -pthread pB.cpp DPF.cpp -o p0 -DROLE_p0 -lboost_system
RUN g++ -std=c++20 -O2 -pthread pB.cpp DPF.cpp -o p1 -DROLE_p1 -lboost_system
RUN g++ -std=c++20 -O2 -pthread p2.cpp -o p2 -lboost_system
RUN g++ -std=c++20 -O2 verify.cpp -o verify

# Create shared_files directory and copy executables there
RUN mkdir -p /app/shared_files
RUN cp gen_data p0 p1 p2 verify /app/shared_files/