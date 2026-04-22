FROM eclipse-mosquitto:2.0-openssl

# Create the directory for certificates
RUN mkdir -p /mosquitto/config/certs

# Copy the new descriptive files
COPY ./CA_CERT/ca-cert.pem /mosquitto/config/certs/ca-cert.pem
COPY ./CA_CERT/mqtt-server-cert.pem /mosquitto/config/certs/mqtt-server-cert.pem
COPY ./CA_CERT/mqtt-server-key.pem /mosquitto/config/certs/mqtt-server-key.pem

# Copy your configuration file
COPY ./mqtt/mosquitto.conf /mosquitto/config/mosquitto.conf

# Copy the entrypoint script
COPY ./mqtt/docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

# Set permissions for certificates
RUN chown -R mosquitto:mosquitto /mosquitto/config/certs && \
    chmod 600 /mosquitto/config/certs/mqtt-server-key.pem && \
    chmod 644 /mosquitto/config/certs/ca-cert.pem /mosquitto/config/certs/mqtt-server-cert.pem


ENTRYPOINT ["/docker-entrypoint.sh"]
# ---------------------

# CMD now becomes the arguments passed to the entrypoint script (the "$@")
CMD ["/usr/sbin/mosquitto", "-c", "/mosquitto/config/mosquitto.conf"]