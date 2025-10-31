FROM node:22-alpine
WORKDIR /app

# Copy package files
COPY package*.json ./

# Install dependencies including devDependencies for build
RUN npm install

# Install Angular CLI globally
RUN npm install -g @angular/cli

# Copy source code
COPY . .

# Build the application
RUN npm run build

EXPOSE 4200

# Use npx to run ng serve
CMD ["npx", "ng", "serve", "--host", "0.0.0.0", "--port", "4200"]