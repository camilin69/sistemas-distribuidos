FROM node:22-alpine
WORKDIR /app

# Install Angular CLI first
RUN npm install -g @angular/cli@20.3.6

# Copy package files
COPY package*.json ./

# Clean npm cache and install with force if needed
RUN npm cache clean --force
RUN npm install --legacy-peer-deps --force

# Copy source code
COPY . .

# Build the application
RUN ng build

EXPOSE 4200

# Serve the application
CMD ["ng", "serve", "--host", "0.0.0.0", "--port", "4200"]