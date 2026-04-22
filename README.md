# WaterLevelProject
HK VTC FYP IoT project Water Level Project

# Water Level Monitoring System 🌊
A full-stack IoT solution to monitor and record water levels in real-time. This project uses an ESP32 and LoRa to collect data and a Docker-based backend to process, store, and manage the information.

# 🛠 Tech Stack
Hardware: ESP32 (Microcontroller), Ultrasonic Water Level Sensor.

Communication: MQTT (Eclipse Mosquitto).

Web Server: Caddy (Reverse Proxy & HTTPS).

Database: MariaDB (SQL Storage).

Database Management: phpMyAdmin.

Containerization: Docker & Docker Compose.

Backend: Node-red.

# 🏗 System Architecture
The system follows a standard IoT "Edge-to-Cloud" workflow:

ESP32 reads the water level and publishes data via MQTT.

The MQTT Broker (inside Docker) receives the messages.

A Backend Script (Python/Node/PHP - add yours here) listens to MQTT and saves data to MariaDB.

Caddy serves the web dashboard and manages traffic.

📂 Water Level Project: Folder Structure
This repository contains the full stack for an IoT-based water level monitoring system. It uses a Dockerized backend architecture for reliability and easy deployment.

Plaintext
WaterLevelProject-public/
├── 📂 Caddy/                  # Web Server & Reverse Proxy
│   ├── 📂 Setup_wizard/       # Web-based configuration tools
│   │   ├── env-generator.html      # Tool to generate .env files
│   │   ├── index.html              # Wizard landing page
│   │   ├── receiver-generator.html # Config generator for receiver units
│   │   └── sender-generator.html   # Config generator for sender units
│   ├── Caddyfile              # Web server configuration
│   └── Dockerfile             # Caddy container definition
├── 📂 ESP32/                  # Firmware for Microcontrollers
│   ├── 📂 sample/             # Example code for hardware
│   │   ├── receiver_final     # Code for the data receiving unit
│   │   └── sender_final_id3   # Code for the sensor-equipped sender
│   └── libraries.7z           # Compressed dependency libraries
├── 📂 mariadb/                # Database Service
│   ├── WaterLevelMonitoringSystem.sql # Initial database schema
│   ├── ssl.cnf                # Secure connection configuration
│   └── Dockerfile             # Database container definition
├── 📂 mqtt/                   # Communication Broker (Mosquitto)
│   ├── mosquitto.conf         # MQTT broker settings
│   ├── docker-entrypoint.sh   # Initialization script
│   └── Dockerfile             # MQTT container definition
├── 📂 nodered/                # Logic & Dashboard Engine
│   ├── flows.json             # Visual programming logic and UI
│   ├── test.js                # Testing scripts
│   └── Dockerfile             # Node-RED container definition
├── 📂 phpmyadmin/             # Database Management GUI
│   └── Dockerfile             # Web interface for MariaDB
├── docker-compose.yaml        # Orchestration (Starts all services)
├── sample_for_env             # Template for environment variables
└── README.md                  # Main documentation

# 🚀 Getting Started
Prerequisites


Docker and Docker Compose installed.

Arduino IDE (or PlatformIO) for flashing the ESP32.

1. Setup the Backend (Docker)
Clone this repository and navigate to the root directory:

2. Setup the .env file
Setup your .env file which include the user name and password for this project.
Put it in the project root directory.
Here is a sample

```
# Project related
PROJECT_NAME=WaterLevelProject

# Global Setting
TZ=Asia/Hong_Kong

# MariaDB Credentials
DB_ROOT_PASSWORD=Fill_in_password
DB_USER=USER
DB_PASSWORD=Fill_in_password


# Phpmyadmin
PHPMYADMIN_EXTERNAL_PORT=Port_number

#Nodered
NODERED_EXTERNAL_PORT='Port_number'
NODE_RED_CREDENTIAL_SECRET=Fill_in_password
NODERED_USER=USER
NODERED_PASSWORD='Fill_in_hashed_password

#MQTT
MQTT_USER=USER
MQTT_PASSWORD=Fill_in_password

#Email
EMAIL_USER='EMAIL'
EMAIL_APP_PASSWORD='GMAIL_APP_PASSWORD'
```

3. Create your ./CA_CERT file with CA Certificates
Here are the code to create Self Signed CA CERTs using Openssl inside a docker container

```
sudo docker 
```

4. Configure the ESP32

Open the firmware folder ./ESP32
Open the .ino file in your IDE.
Update the variables in the code.
Flash the code to your ESP32.

## 

##