# DARKNET game server (Cyber Station) — Node.js
FROM node:20-alpine

WORKDIR /app

# Dependencies first (build cache)
COPY package.json ./
RUN npm install --omit=dev

# Source code
COPY src ./src

EXPOSE 9000
CMD ["node", "src/index.js"]
