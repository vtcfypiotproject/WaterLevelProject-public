FROM nodered/node-red:4.1.1-22

# Switch to root to handle file permissions
USER root

RUN npm install node-red-dashboard \
                node-red-node-mysql \
                node-red-node-email               
# Copy the flow file from your host to the container
# Since the build context is the root (.), the path is ./nodered/flow.json


COPY ./nodered/settings.js /data/settings.js
COPY ./nodered/flows_cred.json /data/flows_cred.json
COPY ./nodered/flows.json /data/flows.json
# Copy the CA and Client certificate files from your host to the container
# We only need these 3 for the Node-RED (client) side
COPY ./CA_CERT/ca-cert.pem /data/certs/ca-cert.pem
COPY ./CA_CERT/client-cert.pem /data/certs/client-cert.pem
COPY ./CA_CERT/client-key.pem /data/certs/client-key.pem

# Set the correct ownership so the Node-RED process can read them
# Node-RED typically runs as UID 1000 (node-red user)
RUN chown -R 1000:1000 /data/certs && \
    chmod 600 /data/certs/client-key.pem && \
    chmod 644 /data/certs/ca-cert.pem /data/certs/client-cert.pem

# Ensure the 'node-red' user owns the file so it can be edited in the UI
RUN chown node-red:node-red /data/flows.json

# Switch back to the default node-red user
USER node-red