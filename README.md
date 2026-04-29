# WaterLevelProject
HK VTC FYP IoT project Water Level Project

# Development-branch 

Please use the Development-branch to setup your own Certificate and '.env'.  

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

Node-red backend listens to MQTT and saves data to MariaDB.

Caddy serves the web dashboard and manages traffic.



--

### 📂 Water Level Project: Folder Structure

```text
WaterLevelProject-public/
├── Caddy/                  # Web Server & Reverse Proxy
│   ├── Setup_wizard/       # Web-based configuration tools
│   │   ├── env-generator.html
│   │   ├── index.html
│   │   ├── receiver-generator.html
│   │   └── sender-generator.html
│   ├── Caddyfile           # Web server configuration
│   └── Dockerfile          # Caddy container definition
├── ESP32/                  # Firmware for Microcontrollers
│   ├── sample/             # Example hardware code
│   │   ├── receiver_final  # Data receiving unit
│   │   └── sender_final_id3# Sensor-equipped sender
│   └── libraries.7z        # Dependency libraries
├── mariadb/                # Database Service
│   ├── WaterLevelMonitoringSystem.sql 
│   ├── ssl.cnf             # Secure connection config
│   └── Dockerfile
├── mqtt/                   # Communication Broker (Mosquitto)
│   ├── mosquitto.conf
│   ├── docker-entrypoint.sh
│   └── Dockerfile
├── nodered/                # Logic & Dashboard Engine
│   ├── flows.json          # Visual logic and UI
│   ├── test.js
│   └── Dockerfile
├── phpmyadmin/             # Database Management GUI
│   └── Dockerfile
├── CA_CERT/
│   ├── *.pem, *.key           # Private keys & SSL certificates for MQTT/Web
│   └── server.conf            # SSL configuration settings
├── docker-compose.yaml      # Orchestration (Starts all services)
├── sample_for_env          # Template for environment variables
├── README.md               # Main documentation
└── .env              # Credentials (gitignored)
```

# 🚀 Getting Started
Prerequisites


Docker and Docker Compose installed.

Arduino IDE (or PlatformIO) for flashing the ESP32.

## 1. Setup the Backend (Docker)
Clone this repository and navigate to the root directory:

## 2. Setup the .env file
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

## 3. Create your ./CA_CERT file with CA Certificates
Here are the code to create Self Signed CA CERTs using Openssl inside a docker container.

Setup the CA Cert file.

Make sure your docker is up and running.

Make sure you are in the root directory of the project.

We will use Openssl docker image to generate our Certificates.



### 1. Make a new directory ./CA_CERT
```
mkdir -p ./CA_CERT
```
### 2. Create the server.conf file

Linux commands
```
cat > ./CA_CERT/server.conf <<EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = vtc-fyp-iot-project.uk
IP.1 = 192.168.1.101
EOF
```

Window commands
```
# Window
$conf = @'
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = vtc-fyp-iot-project.uk
IP.1 = 192.168.1.101
'@

$conf | Out-File -FilePath ./CA_CERT/server.conf -Encoding utf8
```
 
### 3. Generate Root CA
```
# 3.1 Generate the Private Key (The file OpenSSL says is missing)
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl genrsa -out ca-key.pem 4096

# 3.2 Generate the Root Certificate (Now that the key exists)
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl req -x509 -new -nodes -key ca-key.pem -sha256 -days 3650 -subj "/CN=WaterLevel-Root-CA/O=VTC-WaterLevelProject-FYP/OU=VTC-WaterLevelProject-FYP" -out ca-cert.pem
```

### 4. Generate MariaDB Server Certificate
```
# Generate Key
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl genrsa -out server-key.pem 2048

# Generate CSR
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl req -new -key server-key.pem -subj "/CN=mariadb_final/O=VTC-WaterLevelProject-FYP/OU=VTC-WaterLevelProject-FYP" -out server-csr.pem

# Sign Certificate
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl x509 -req -in server-csr.pem -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out server-cert.pem -days 365 -sha256
5. Generate MQTT Server Certificate
# Generate Key
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl genrsa -out mqtt-server-key.pem 2048

# Generate CSR
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl req -new -key mqtt-server-key.pem -subj "/CN=mqtt-server-cert/O=VTC-WaterLevelProject-FYP/OU=VTC-WaterLevelProject-FYP" -out mqtt-server-csr.pem

# Sign Certificate (Using server.conf)
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl x509 -req -in mqtt-server-csr.pem -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out mqtt-server-cert.pem -days 365 -sha256 -extfile server.conf
```
 
### 6. Generate Client Certificate
```
# Generate Key
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl genrsa -out client-key.pem 2048

# Generate CSR
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl req -new -key client-key.pem -subj "/CN=client-cert/O=VTC-WaterLevelProject-FYP/OU=VTC-WaterLevelProject-FYP" -out client-csr.pem

# Sign Certificate
docker run --rm -v "${PWD}/CA_CERT:/export" -w /export alpine/openssl x509 -req -in client-csr.pem -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out client-cert.pem -days 365 -sha256
```
### 7. Verification
```
ls -l ./CA_CERT
```
### 8. Fix the Permissions if you are using Linux
```
sudo chown -R $USER:$USER ./CA_CERT
```



## 4. Configure the ESP32

Use the Setup_Wizard.html in the root directory.

Generate the file your need for receivers and senders.

Flash your ESP32 board using the Arduino IDE.

Add the data of your ESP32 Node into the Database.

(Please check Database handbook) 
