#!/bin/sh
set -e

# Print what we are doing to the logs
echo "Executing custom entrypoint script..."

# Ensure the directory exists
mkdir -p /mosquitto/config

# If the variables are set, create/update the password file
if [ -n "$MQTT_USER" ] && [ -n "$MQTT_PASSWORD" ]; then
    echo "Creating MQTT password file for user: $MQTT_USER"
    # Create an empty file first
    touch /mosquitto/config/passwd
    # Add the user/pass in batch mode
    mosquitto_passwd -b /mosquitto/config/passwd "$MQTT_USER" "$MQTT_PASSWORD"
    # Ensure correct ownership
    chown mosquitto:mosquitto /mosquitto/config/passwd
else
    echo "WARNING: MQTT_USER or MQTT_PASSWORD not found in environment variables."
fi

# Add this right after the mosquitto_passwd command
chmod 0700 /mosquitto/config/passwd


exec "$@"