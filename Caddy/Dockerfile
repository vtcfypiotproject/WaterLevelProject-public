FROM caddy:2.11.2

# 1. Create the directory for SSL (Optional, COPY creates it if missing)
RUN mkdir -p /etc/caddy/ssl

# 2. Copy the CA certificates from your local folder
# This replaces the need for the volume: ./CA_CERT:/etc/caddy/ssl:ro
COPY ./CA_CERT /etc/caddy/ssl

# 3. Copy the existing Caddyfile and web files
COPY ./Caddy/Caddyfile /etc/caddy/Caddyfile
COPY ./Caddy/Setup_wizard /usr/share/caddy/Setup_wizard
COPY ./Caddy/index.html /usr/share/caddy/index.html

EXPOSE 80
EXPOSE 443