SCD41 CO2 Sensor C Driver
This repository contains a portable C driver for the Sensirion SCD41 CO₂ sensor. The driver is designed to be hardware-agnostic through a simple Hardware Abstraction Layer (HAL),.

Features
The driver currently supports the following sensor commands:

Core Measurements:

Start/Stop Periodic Measurements

Read Measurement (CO₂, Temperature, Humidity)

Perform a Single-Shot Measurement

Check Data Ready Status

Device Management:

Get Serial Number

Wake Up / Re-initialize

Perform Self-Test

Calibration & Configuration:

Set/Get Sensor Altitude

Set/Get Ambient Pressure

Enable/Disable Automatic Self-Calibration (ASC)

Persist Settings to non-volatile memory

1. Development & Testing (Host PC)
The driver is developed and tested on a host PC (Linux) using a mocked hardware interface.

Dependencies
cmake (version 3.14+)

gcc/g++ compiler toolchain

git

Setup & Build
Clone the Repository:

git clone git@github.com:acentauri92/scd41.git
cd scd41

Initialize Submodules: The project uses CppUTest as a Git submodule for unit testing.

git submodule update --init --recursive

Build the Tests: Use CMake to configure and build the test runner.

mkdir build && cd build
cmake ..
make

Run the Tests: Use the CTest runner to execute all unit tests.

make test
# Or run CTest directly
# ctest

2. Hardware Application (Raspberry Pi)
An example application is provided to demonstrate the driver's usage on a Raspberry Pi. It reads sensor data and logs it to an InfluxDB database.

Raspberry Pi Setup
Enable I2C:

sudo raspi-config

Navigate to Interface Options -> I2C and enable it.

Install Dependencies:

sudo apt-get update
sudo apt-get install libcurl4-openssl-dev

Cross-Compiling the Application
Mount Pi Filesystem (Optional but Recommended): For a robust cross-compilation environment, mount the Pi's root filesystem on your host PC using sshfs.

# On your PC
sudo apt-get install sshfs
mkdir ~/rpi_mount
sshfs <user>@<pi_ip>:/ ~/rpi_mount

Compile: Use the aarch64-linux-gnu-gcc cross-compiler. The following command compiles all necessary source files and links against libcurl.

aarch64-linux-gnu-gcc \
  src/main.c \
  src/scd41.c \
  src/rpi_i2c_hal.c \
  src/influx_logger.c \
  -Iinclude \
  --sysroot=$HOME/rpi_mount \
  -o scd41_app -lcurl

Copy and Run: Copy the compiled scd41_app executable to your Pi and run it.

3. Data Logging & Visualization
The example application can be run as a service to create a 24/7 environmental monitor.

InfluxDB & Grafana Setup
Follow the detailed guide to install and configure InfluxDB and Grafana on your Raspberry Pi. This will create the database for storing sensor data and the dashboard for visualizing it.

Systemd Service
To run the logger automatically, a systemd timer and service can be used.

Create the Service File: sudo nano /etc/systemd/system/scd41-logger.service

[Unit]
Description=SCD41 CO2 Sensor Data Logger Service

[Service]
Type=simple
User=dheeraj
WorkingDirectory=/home/dheeraj/co2/scd41
ExecStart=/home/dheeraj/co2/scd41/scd41_app

[Install]
WantedBy=multi-user.target

Create the Timer File: sudo nano /etc/systemd/system/scd41-logger.timer

[Unit]
Description=Run SCD41 Data Logger every 10 minutes

[Timer]
OnBootSec=1min
OnUnitActiveSec=10min
Unit=scd41-logger.service

[Install]
WantedBy=timers.target

Enable and Start:

sudo systemctl daemon-reload
sudo systemctl enable --now scd41-logger.timer

Code Structure
include/: Public header files for the driver and HAL.

src/: Driver source code and the Raspberry Pi example application.

tests/: Unit tests written using CppUTest.

lib/: Git submodules (e.g., CppUTest).