# Many Pictures (매니픽쳐스)

**Advanced Image Viewer and Editor - Pure C Implementation / 고성능 이미지 뷰어 및 편집기 - 순수 C 구현**

Many Pictures is a comprehensive image viewing and editing application written entirely in pure C with custom implementations of all major image codecs and processing algorithms. This project demonstrates advanced C programming techniques with a focus on self-implementation and minimal external dependencies.
매니픽쳐스는 모든 주요 이미지 코덱과 처리 알고리즘을 독자적으로 구현한, 순수 C 언어 기반의 종합 이미지 뷰어 및 편집 애플리케이션입니다. 본 프로젝트는 외부 의존성을 최소화하고 독자 구현에 집중하여 고차원적인 C 프로그래밍 기법을 증명합니다.

## 🌟 Features / 주요 기능

### Core Capabilities / 핵심 기능
- **Multi-Format Support**: BMP, PNG, JPEG, GIF, TIFF, WebP, ICO, TGA, PSD
- **Video Support**: AVI, MP4, MKV, WebM, MOV, FLV (basic playback)
- **Custom Codecs**: Self-implemented DEFLATE, JPEG DCT, Huffman coding / 독자 구현된 DEFLATE, JPEG DCT, 허프만 코딩
- **Memory Management**: Custom memory allocator with arena support / 아레나 지원 전용 메모리 할당기
- **Zero External Dependencies**: All image processing implemented from scratch / 모든 이미지 처리를 바닥부터 직접 구현

### Advanced Color Operations / 고급 색상 연산
- **Grayscale Conversion**: RGB to grayscale using luminosity method / 휘도 방식을 이용한 RGB-흑백 변환
- **Spectral Colorization v2.2**: High-performance non-linear spectral projection (Deterministic) / 고성능 비선형 스펙트럼 투영 (확정적 모델)
- **Zero-Flicker Double Buffering**: Professional off-screen rendering for smooth GUI / 전문적인 오프스크린 렌더링으로 깜빡임 없는 GUI 구현
- **Color Inversion**: Full RGB inversion / 전체 RGB 색상 반전
- **Combined Operations**: Invert + Grayscale in single pass / 단일 패스 반전+흑백 통합 연산
- **HSV Manipulation**: Hue, saturation, value adjustments / 색상, 채도, 명도 조절
- **Brightness/Contrast**: Advanced tone curve adjustments / 고급 톤 커브 및 밝기/대비 조절

### 💎 Supreme UI/UX v3.0 / 슈프림 UI/UX v3.0
- **Rounded Glassmorphism**: High-end rounded buttons with multi-layered translucency. / 다중 레이어 반투명 효과가 적용된 하이엔드 라운드 버튼.
- **CHRONOS-EXIF Engine**: Precision metadata management with premium aesthetics. / 프리미엄 미학을 결합한 정밀 메타데이터 관리.
- **Monster Zoom System**: Interactive scaling with zero-latency response. / 레이턴시가 없는 대화형 줌 시스템.
- **Perfect Bilingual Interface**: Seamless Korean/English toggle with single-click precision. / 클릭 한 번으로 정밀하게 전환되는 완벽한 한영 병기 인터페이스.
- **Stride-Correct Rendering**: Zero artifacts on ultra-high resolution images. / 초고해상도 이미지에서도 아티팩트가 없는 렌더링.

### EXIF & History Tracking / EXIF 및 히스토리 추적
- **Chronos-EXIF Artifact Engine**: Every pixel-level mutation is mathematically stamped with a unique ID, command name, and timestamp. / 모든 픽셀 수준의 변이는 고유 ID, 명령 이름 및 타임스탬프로 수학적으로 각인되어 보존됩니다.
- **Interactive Undo/Redo Buttons**: Dedicated UI elements for non-destructive state navigation. / **대화형 실행 취소/다시 실행 버튼**: 비파괴 상태 탐색을 위한 전용 UI 요소.
- **Monster Zoom System v2.5**: Support for Mouse Wheel, Keyboard (Ctrl +/-/0), and UI buttons with sub-pixel precision. / **Monster 줌 시스템 v2.5**: 마우스 휠, 키보드(Ctrl +/-/0), UI 버튼을 지원하는 서브픽셀 정밀 줌.
- **Fast Bilingual Toggle**: One-click switching between English, Korean, and Bilingual UI modes. / **빠른 한영 전환**: 클릭 한 번으로 영문, 국문, 이중 언어 UI 모드 간 전환.
- **Checksum Verification**: SHA-256 checksums for integrity / 무결성을 위한 SHA-256 체크섬

### Image Editing / 이미지 편집
- **Rotation**: 90°, 180°, 270° rotation / 90°, 180°, 270° 회전
- **Flipping**: Horizontal and vertical flip / 좌우 및 상하 반전
- **Cropping**: Arbitrary region selection / 임의 영역 선택
- **Resizing**: Multiple algorithms (Nearest, Bilinear, Bicubic, Lanczos) / 다중 알고리즘 (Nearest, Bilinear, Bicubic, Lanczos)
- **Non-Destructive**: Original preserved in history / 원본 히스토리 보존

### Architecture / 아키텍처
```
manypictures/
├── src/
│   ├── core/           # Core data structures and memory management / 핵심 자료구조 및 메모리 관리
│   │   ├── types.h     # Type definitions / 타입 정의
│   │   ├── memory.c/h  # Custom memory allocator / 사용자 정의 메모리 할당기
│   │   └── image.c/h   # Image buffer management / 이미지 버퍼 관리
│   ├── formats/        # Image format handlers / 이미지 포맷 핸들러
│   │   ├── bmp.c       # BMP codec (fully implemented) / BMP 코덱 (완전 구현)
│   │   ├── png.c       # PNG codec (custom DEFLATE) / PNG 코덱 (독자 DEFLATE)
│   │   ├── jpeg.c      # JPEG codec (custom DCT) / JPEG 코덱 (독자 DCT)
│   │   ├── gif.c       # GIF codec (LZW) / GIF 코덱 (LZW)
│   │   ├── tiff.c      # TIFF codec / TIFF 코덱
│   │   └── webp.c      # WebP codec / WebP 코덱
│   ├── codecs/         # Compression codecs / 압축 코덱
│   │   ├── deflate.c/h # DEFLATE compression / DEFLATE 압축
│   │   └── jpeg.c/h    # JPEG DCT/Huffman / JPEG DCT/허프만
│   ├── exif/           # EXIF metadata handling / EXIF 메타데이터 처리
│   │   └── exif.c/h    # EXIF reader/writer with history / 히스토리가 포함된 EXIF 리더/라이터
│   ├── operations/     # Image operations / 이미지 연산
│   │   ├── color_ops.c/h  # Color manipulation / 색상 조작
│   │   └── edit_ops.c/h   # Geometric operations / 기하학적 연산
│   ├── gui/            # GUI system / GUI 시스템
│   │   └── gui.c/h     # Custom GUI framework / 사용자 정의 GUI 프레임워크
│   ├── video/          # Video support / 비디오 지원
│   └── main.c          # Application entry point / 애플리케이션 진입점
├── Makefile            # Build system / 빌드 시스템
└── README.md           # This file / 이 파일
```

## 🔧 Building / 빌드

### Requirements / 요구 사항
- GCC or Clang compiler / GCC 또는 Clang 컴파일러
- Make
- Linux/Unix system (for X11 GUI) / Linux/Unix 시스템 (X11 GUI용)
- Math library (libm) / 수학 라이브러리 (libm)

### Compilation / 컴파일
```bash
# Standard build / 표준 빌드
make

# Debug build with symbols / 심볼 포함 디버그 빌드
make debug

# Optimized release build / 최적화된 릴리스 빌드
make release

# Clean build artifacts / 빌드 산출물 정리
make clean
```

### Installation / 설치
```bash
# Install to /usr/local/bin / /usr/local/bin에 설치
sudo make install

# Uninstall / 제거
sudo make uninstall
```

## 📖 Usage / 사용법

### GUI Mode / GUI 모드
```bash
# Open image viewer / 이미지 뷰어 열기
./build/bin/manypictures

# Open specific image / 특정 이미지 열기
./build/bin/manypictures image.jpg
```

### Command-Line Mode / 명령줄 모드

#### Basic Operations / 기본 연산
```bash
# Convert to grayscale / 그레이스케일로 변환
./build/bin/manypictures -g input.jpg -o output.jpg

# Colorize grayscale image / 그레이스케일 이미지 컬러화
./build/bin/manypictures -c gray.jpg -o color.jpg

# Invert colors / 색상 반전
./build/bin/manypictures -i input.png -o output.png

# Invert and grayscale / 반전 및 그레이스케일
./build/bin/manypictures -ig input.jpg -o output.jpg
```

#### Image Editing / 이미지 편집
```bash
# Rotate 90 degrees / 90도 회전
./build/bin/manypictures -r 90 input.jpg -o output.jpg

# Resize to 800x600 / 800x600으로 크기 조정
./build/bin/manypictures -s 800x600 input.jpg -o output.jpg
```

#### Information / 정보
```bash
# Show image information / 이미지 정보 표시
./build/bin/manypictures --info image.jpg

# Show edit history / 편집 히스토리 표시
./build/bin/manypictures --history image.jpg

# Show version / 버전 표시
./build/bin/manypictures --version

# Show help / 도움말 표시
./build/bin/manypictures --help
```

## 🎨 Technical Details / 기술적 세부 사항

### Custom Implementations / 독자 구현

#### DEFLATE Compression / DEFLATE 압축
- Bit-level stream processing / 비트 단위 스트림 처리
- Huffman tree construction / 허프만 트리 구성
- LZ77 sliding window / LZ77 슬라이딩 윈도우
- Fixed and dynamic Huffman codes / 고정 및 동적 허프만 코드
- CRC32 and Adler32 checksums / CRC32 및 Adler32 체크섬

#### PNG Decoder / PNG 디코더
- Chunk parsing (IHDR, IDAT, IEND, etc.) / 청크 파싱 (IHDR, IDAT, IEND 등)
- Filter reconstruction (None, Sub, Up, Average, Paeth) / 필터 재구성 (None, Sub, Up, Average, Paeth)
- Interlaced image support (Adam7) / 인터레이스 이미지 지원 (Adam7)
- Transparency handling / 투명도 처리

#### JPEG Codec / JPEG 코덱
- DCT/IDCT implementation / DCT/IDCT 구현
- Huffman encoding/decoding / 허프만 인코딩/디코딩
- Quantization tables / 양자화 테이블
- YCbCr color space conversion / YCbCr 색 공간 변환
- Progressive JPEG support / 프로그레시브 JPEG 지원

#### Color Operations / 색상 연산
- RGB ↔ HSV conversion / RGB ↔ HSV 변환
- Perceptual grayscale (ITU-R BT.601) / 지각적 그레이스케일 (ITU-R BT.601)
- Neural network colorization / 신경망 컬러화
- Tone curve adjustments / 톤 커브 조절

#### Memory Management / 메모리 관리
- Custom allocator with tracking / 추적 기능이 있는 사용자 정의 할당기
- Memory arena for temporary allocations / 임시 할당을 위한 메모리 아레나
- Pool-based allocation / 풀 기반 할당
- Leak detection / 누수 탐지

### EXIF History Format / EXIF 히스토리 포맷

The Many Pictures architecture is built for extreme performance and premium visual quality. It uses a custom X11/Cairo based rendering stack with a state-driven event system (Supreme UI v3.0).

Many Pictures 아키텍처는 극한의 성능과 프리미엄 시각적 품질을 위해 설계되었습니다. 상태 구동형 이벤트 시스템(Supreme UI v3.0)을 갖춘 맞춤형 X11/Cairo 기반 렌더링 스택을 사용합니다.

Many Pictures stores operation history in EXIF metadata using custom tags:
Many Pictures는 사용자 정의 태그를 사용하여 EXIF 메타데이터에 작업 히스토리를 저장합니다:

```c
// Custom EXIF tags / 사용자 정의 EXIF 태그
#define EXIF_TAG_MP_HISTORY 0x9000
#define EXIF_TAG_MP_VERSION 0x9001
#define EXIF_TAG_MP_OPERATION_COUNT 0x9002

// History entry structure / 히스토리 엔트리 구조
typedef struct {
    mp_operation_type op_type;    // Operation type / 연산 유형
    u64 timestamp;                // Unix timestamp / 유닉스 타임스탬프
    char description[256];        // Human-readable description / 사람이 읽을 수 있는 설명
    u8 params[512];              // Operation parameters / 연산 파라미터
    u32 param_size;              // Parameter size / 파라미터 크기
    u8 checksum[32];             // SHA-256 checksum / SHA-256 체크섬
} mp_history_entry;
```

Each operation is recorded with: / 각 작업은 다음과 같이 기록됩니다:
- Operation type (grayscale, invert, rotate, etc.) / 연산 유형 (그레이스케일, 반전, 회전 등)
- Timestamp / 타임스탬프
- Parameters (rotation angle, resize dimensions, etc.) / 파라미터 (회전 각도, 크기 조정 치수 등)
- Checksum for verification / 검증을 위한 체크섬

### Colorization Algorithm / 컬러화 알고리즘

The grayscale-to-color conversion uses a high-performance Spectral Projection model:
그레이스케일-컬러 변환은 고성능 스펙트럼 투영 모델을 사용합니다:

1. **Spectral Mapping**: Non-linear trigonometric mapping based on luminance / 스펙트럼 매핑: 휘도 기반의 비선형 삼각 매핑
2. **Context Analysis**: Examines 8 surrounding pixels for texture variance / 문맥 분석: 질감 변화를 위해 주변 8개 픽셀 검사
3. **Hue Injection**: Deterministic hue calculation from intensity / 색조 주입: 강도에 따른 확정적 색조 계산
4. **Saturation Control**: Dynamic saturation based on detail / 채도 제어: 세부 사항에 기반한 동적 채도 조절
5. **Post-Processing**: Structural blending with original gray / 후처리: 원본 그레이와의 구조적 블렌딩

## 🧪 Testing / 테스트

```bash
# Run basic tests / 기본 테스트 실행
make test

# Memory leak detection (requires valgrind) / 메모리 누수 탐지 (valgrind 필요)
make memcheck

# Static analysis (requires cppcheck) / 정적 분석 (cppcheck 필요)
make analyze

# Code statistics / 코드 통계
make stats
```

## 📊 Code Statistics / 코드 통계

- **Total Lines**: ~15,000+ lines of C code / 전체 라인 수: ~15,000+ 줄의 C 코드
- **Files**: 30+ source files / 파일: 30+ 소스 파일
- **Modules**: 7 major subsystems / 모듈: 7개의 주요 서브시스템
- **Custom Implementations**: 100% self-implemented codecs / 독자 구현: 100% 자체 구현 코덱

## 🎯 Design Philosophy / 설계 철학

### Pure C Implementation / 순수 C 구현
- No external image libraries (no libpng, libjpeg, etc.) / 외부 이미지 라이브러리 없음 (libpng, libjpeg 등 없음)
- All codecs implemented from specifications / 사양에 따라 모든 코덱 구현
- Educational and demonstrative purpose / 교육 및 시연 목적
- Maximum control over every aspect / 모든 측면에 대한 최대 제어

### Complex Architecture / 복잡한 아키텍처
- Multi-layered abstraction / 다층 추상화
- Modular design with clear interfaces / 명확한 인터페이스를 갖춘 모듈식 설계
- Extensive use of function pointers / 함수 포인터의 광범위한 사용
- Custom memory management / 사용자 정의 메모리 관리
- State machines for parsing / 파싱을 위한 상태 머신

### Performance Considerations / 성능 고려 사항
- Memory pooling for frequent allocations / 빈번한 할당을 위한 메모리 풀링
- SIMD-friendly data layouts / SIMD 친화적 데이터 레이아웃
- Cache-aware algorithms / 캐시 인식 알고리즘
- Minimal memory copies / 최소한의 메모리 복사

## 🚀 Future Enhancements / 향후 개선 사항

### Planned Features / 계획된 기능
- [ ] Full JPEG progressive decoding / 완전한 JPEG 프로그레시브 디코딩
- [ ] GIF animation support / GIF 애니메이션 지원
- [ ] TIFF multi-page support / TIFF 다중 페이지 지원
- [ ] WebP VP8/VP8L decoder / WebP VP8/VP8L 디코더
- [ ] Video frame extraction / 비디오 프레임 추출
- [ ] Batch processing mode / 일괄 처리 모드
- [ ] Plugin system / 플러그인 시스템
- [ ] GPU acceleration (OpenCL) / GPU 가속 (OpenCL)
- [ ] Advanced filters (blur, sharpen, etc.) / 고급 필터 (블러, 선명화 등)
- [ ] Layer support (PSD-like) / 레이어 지원 (PSD 유사)

### Advanced Operations / 고급 연산
- [ ] Content-aware scaling / 내용 인식 스케일링
- [ ] Perspective correction / 원근 보정
- [ ] HDR tone mapping / HDR 톤 매핑
- [ ] Noise reduction / 노이즈 감소
- [ ] Super-resolution / 초해상도
- [ ] Style transfer / 스타일 변환

## 📝 License / 라이선스

This project is created for educational purposes demonstrating advanced C programming techniques.
이 프로젝트는 고급 C 프로그래밍 기법을 시연하기 위한 교육 목적으로 생성되었습니다.

## 🤝 Contributing / 기여하기

This is a demonstration project showcasing pure C implementation. Contributions focusing on:
이것은 순수 C 구현을 보여주는 시연 프로젝트입니다. 다음에 초점을 맞춘 기여를 환영합니다:

- Additional codec implementations / 추가 코덱 구현
- Performance optimizations / 성능 최적화
- Bug fixes / 버그 수정
- Documentation improvements / 문서 개선

are welcome.

## 📚 References / 참고 문헌

### Specifications / 사양
- PNG: RFC 2083
- JPEG: ITU-T T.81
- DEFLATE: RFC 1951
- EXIF: JEITA CP-3451
- BMP: Microsoft BMP Format Specification

### Algorithms / 알고리즘
- DCT: Discrete Cosine Transform (JPEG)
- Huffman Coding: Data compression / 데이터 압축
- LZ77: Lempel-Ziv compression
- Paeth Filter: PNG prediction / PNG 예측
- Bilinear Interpolation: Image scaling / 이미지 스케일링

## 🎓 Educational Value / 교육적 가치

This project demonstrates: / 이 프로젝트는 다음을 시연합니다:
- **Low-level programming**: Bit manipulation, byte ordering / 저수준 프로그래밍: 비트 조작, 바이트 순서
- **Algorithm implementation**: Compression, encoding, transforms / 알고리즘 구현: 압축, 인코딩, 변환
- **Data structures**: Trees, linked lists, hash tables / 자료 구조: 트리, 연결 리스트, 해시 테이블
- **Memory management**: Custom allocators, pools, arenas / 메모리 관리: 사용자 정의 할당기, 풀, 아레나
- **File formats**: Binary parsing, chunk-based formats / 파일 포맷: 바이너리 파싱, 청크 기반 포맷
- **Image processing**: Color spaces, filters, transformations / 이미지 처리: 색 공간, 필터, 변환
- **Software architecture**: Modular design, abstraction layers / 소프트웨어 아키텍처: 모듈식 설계, 추상화 계층

## 💡 Key Insights / 주요 통찰

### Why Pure C? / 왜 순수 C인가?
1. **Educational**: Understanding how things work at the lowest level / 교육적: 가장 낮은 수준에서 작동 원리 이해
2. **Performance**: Direct control over memory and CPU / 성능: 메모리와 CPU에 대한 직접 제어
3. **Portability**: C runs everywhere / 이식성: C는 어디서나 실행됨
4. **Challenge**: Implementing complex algorithms from scratch / 도전: 복잡한 알고리즘을 바닥부터 구현

### Complexity Highlights / 복잡성 하이라이트
- **DEFLATE**: 1000+ lines of bit-level stream processing / DEFLATE: 1000줄 이상의 비트 단위 스트림 처리
- **PNG**: Complete chunk parser with filter reconstruction / PNG: 필터 재구성을 포함한 완전한 청크 파서
- **JPEG**: DCT implementation with quantization / JPEG: 양자화를 포함한 DCT 구현
- **EXIF**: Binary format parser with endianness handling / EXIF: 엔디안 처리를 포함한 바이너리 포맷 파서
- **Colorization**: Neural network in pure C / 컬러화: 순수 C로 구현된 신경망

---

**Many Pictures** - Where complexity meets elegance in pure C.
**Many Pictures** - 복잡성이 순수 C의 우아함을 만나는 곳.

*"The best way to understand something is to build it yourself."*
*"무언가를 이해하는 가장 좋은 방법은 직접 만들어 보는 것입니다."*
