# Servidor do jogo DARKNET (Cyber Station) — Node.js
FROM node:20-alpine

WORKDIR /app

# Dependências primeiro (cache de build)
COPY package.json ./
RUN npm install --omit=dev

# Código
COPY src ./src

EXPOSE 9000
CMD ["node", "src/index.js"]
