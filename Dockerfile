# Many Pictures - Monster Dockerfile / Many Pictures - 몬스터 Dockerfile
# Rheehose (Rhee Creative) 2008-2026

# Build stage / 빌드 단계
FROM gcc:latest AS builder

# Install dependencies / 종속성 설치
RUN apt-get update && apt-get install -y \
    libx11-dev \
    libcairo2-dev \
    pkg-config \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory / 작업 디렉토리 설정
WORKDIR /app

# Copy source code / 소스 코드 복사
COPY . .

# Build project / 프로젝트 빌드
RUN make clean && make release

# Final stage / 최종 단계
FROM debian:bookworm-slim

# Install runtime dependencies / 런타임 종속성 설치
RUN apt-get update && apt-get install -y \
    libx11-6 \
    libcairo2 \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user / 비루트 사용자 생성
RUN useradd -m rheeworker
USER rheeworker
WORKDIR /home/rheeworker

# Copy binary from builder / 빌더에서 바이너리 복사
COPY --from=builder /app/build/bin/manypictures .

# Environment setup / 환경 설정
ENV DISPLAY=:0

# Entry point / 엔트리 포인트
ENTRYPOINT ["./manypictures"]

# Rheehose (Rhee Creative) 2008-2026
