FROM golang:1.25-bookworm AS builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/go.mod backend/go.sum ./backend/
RUN cd backend && go mod download

COPY . .

RUN g++ -O2 -std=c++17 -o /app/main /app/main.cpp
RUN cd backend && go build -o /app/backend/app .

FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/main /app/main
COPY --from=builder /app/backend/app /app/backend/app
COPY --from=builder /app/frontend/public /app/frontend/public

EXPOSE 8080

CMD ["/app/backend/app"]
