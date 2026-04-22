# Use the official MariaDB image as the base
FROM mariadb:11.4

# Create the directory for certificates
RUN mkdir -p /etc/mysql/ssl

# Copy certificates from your host to the image
# Note: Use the relative path from where your Dockerfile is
COPY ./CA_CERT/ca-cert.pem /etc/mysql/ssl/
COPY ./CA_CERT/server-cert.pem /etc/mysql/ssl/
COPY ./CA_CERT/server-key.pem /etc/mysql/ssl/

# CRITICAL: Set the owner to 'mysql' and restrict the key
RUN chown -R mysql:mysql /etc/mysql/ssl && \
    chmod 600 /etc/mysql/ssl/server-key.pem

# Copy your SQL initialization if you have one
COPY ./mariadb/WaterLevelMonitoringSystem.sql /docker-entrypoint-initdb.d/

# Optional: Set the working directory (helpful for manual terminal access)
WORKDIR /var/lib/mysql