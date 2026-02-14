# Use the same base you were using
FROM ubuntu:24.04

# Avoid being asked for timezones/geographic data
ENV DEBIAN_FRONTEND=noninteractive

# Install everything you were trying to get via "Features"
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gdb \
    git \
    libicu-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Add a user so you aren't running everything as root
RUN useradd -m vscode && echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER vscode