#!/bin/bash

# ==========================================
# Water Level Monitoring System Installer
# ==========================================

# Stop script on error
set -e

echo ">>> Starting installation for Water Level Monitoring System..."

# Check if script is run as root
if [ "$EUID" -ne 0 ]; then
  echo "ERROR: Please run as root (use sudo ./install.sh)"
  exit 1
fi

# 1. System Update
echo ">>> Updating system packages..."
apt-get update && apt-get upgrade -y

# ==========================================
# 1.5 Setup UFW Firewall (IPv4 & IPv6)
# ==========================================
echo ">>> Checking UFW (Uncomplicated Firewall) status..."

# Function to install UFW across different Linux distributions
install_ufw() {
    if command -v apt-get &> /dev/null; then
        echo ">>> Detected apt-get. Installing UFW..."
        apt-get install -y ufw
    elif command -v dnf &> /dev/null; then
        echo ">>> Detected dnf. Installing UFW..."
        dnf install -y ufw
    elif command -v yum &> /dev/null; then
        echo ">>> Detected yum. Installing EPEL and UFW..."
        yum install -y epel-release
        yum install -y ufw
    elif command -v pacman &> /dev/null; then
        echo ">>> Detected pacman. Installing UFW..."
        pacman -Sy --noconfirm ufw
    elif command -v zypper &> /dev/null; then
        echo ">>> Detected zypper. Installing UFW..."
        zypper install -y ufw
    else
        echo "ERROR: Unsupported package manager. Please install UFW manually."
        exit 1
    fi
}

# Check if UFW is installed, install if not
if ! command -v ufw &> /dev/null; then
    echo ">>> UFW not found. Initiating installation..."
    install_ufw
else
    echo ">>> UFW is already installed."
fi

# Ensure IPv6 is enabled (modifies the UFW config file)
echo ">>> Configuring UFW for IPv4 and IPv6 support..."
if [ -f /etc/default/ufw ]; then
    sed -i 's/^IPV6=.*/IPV6=yes/' /etc/default/ufw
fi

# Set default policies
ufw default deny incoming
ufw default allow outgoing

# Allow Essential Ports (CRITICAL: Do not remove SSH!)
echo ">>> Setting up firewall rules for the Water Level Monitoring System..."
ufw allow ssh         # SSH access
ufw allow 14104/tcp   # MariaDB
ufw allow 14105/tcp   # phpMyAdmin
ufw allow 14106/tcp   # Node-RED
ufw allow 14107/tcp   # MQTT
ufw allow 14108/tcp   # MQTT WebSocket

# Enable UFW non-interactively
echo ">>> Enabling UFW..."
ufw --force enable
ufw status verbose
echo ">>> UFW configuration complete."

# 2. Install Docker & Docker Compose (if not installed)
if ! command -v docker &> /dev/null; then
    echo ">>> Docker not found. Installing Docker..."
    
    # Install prerequisites
    apt-get install -y ca-certificates curl gnupg lsb-release

    # Add Docker's official GPG key
    mkdir -p /etc/apt/keyrings
    if [ ! -f /etc/apt/keyrings/docker.gpg ]; then
        curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg
    fi

    # Set up the repository
    echo \
      "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
      $(lsb_release -cs) stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null
    
    # Install Docker Engine
    apt-get update
    apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
    
    echo ">>> Docker installed successfully."
else
    echo ">>> Docker is already installed. Skipping installation."
fi

# 3. Prepare Project Directory
PROJECT_DIR="water_level_project"
echo ">>> Setting up project directory: /opt/$PROJECT_DIR"
mkdir -p /opt/$PROJECT_DIR
cd /opt/$PROJECT_DIR

# 4. Install git and git pull the project from github
echo ">>> Installing git..."
apt-get update && apt-get install -y git

# Since Step 3 already moved us into /opt/$PROJECT_DIR, 
# we check if a .git folder exists to see if we should 'clone' or 'pull'.
if [ ! -d ".git" ]; then
    echo ">>> Initializing repository and cloning..."
    # We use '.' to clone the contents directly into the current directory (/opt/water_level_project)
    git clone https://github.com/vtcfypiotproject/WaterLevelProject-public .
else
    echo ">>> Project already exists. Pulling latest updates..."
    # Resetting local changes ensures the pull doesn't fail due to file conflicts
    git fetch --all
    git reset --hard origin/main
fi


# 5. Start Services
echo ">>> Pulling images and starting containers..."
docker compose pull
docker compose up -d

# 6. Final Status
HOST_IP=$(hostname -I | cut -d' ' -f1)
echo "=================================================="
echo "   INSTALLATION COMPLETE"
echo "=================================================="
echo "Services are running."
echo "You can access them via the following URLs (assuming IP: $HOST_IP):"
echo ""
echo " - phpMyAdmin:  https://$HOST_IP:14105"
echo " - Node-RED:    https://$HOST_IP:14106"
echo " - Caddy:       https://$HOST_IP:443"
echo ""
echo "Project Directory: /opt/$PROJECT_DIR"
echo "Check logs with:   cd /opt/$PROJECT_DIR && docker compose logs -f"
echo "=================================================="

exit 0
# Start the docker compose
# You may need to use sudo before docker compose 
