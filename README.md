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
- **AI-Based Colorization**: Grayscale to color using deep neural network (5-layer MLP) / 심층 신경망(5층 MLP)을 이용한 지능형 컬러화
- **Color Inversion**: Full RGB inversion / 전체 RGB 색상 반전
- **Combined Operations**: Invert + Grayscale in single pass / 단일 패스 반전+흑백 통합 연산
- **HSV Manipulation**: Hue, saturation, value adjustments / 색상, 채도, 명도 조절
- **Brightness/Contrast**: Advanced tone curve adjustments / 고급 톤 커브 및 밝기/대비 조절

### EXIF & History Tracking / EXIF 및 히스토리 추적
- **EXIF Reader/Writer**: Full EXIF metadata support / 전체 EXIF 메타데이터 지원
- **Git-Like History**: Custom history tracking embedded in EXIF / EXIF 내장형 독자 히스토리 추적
- **Operation Recording**: Every edit recorded with parameters / 모든 편집 사항을 파라미터와 함께 기록
- **History Restoration**: Restore to any previous state (Git-like) / 이전 상태로의 복구 (Git 스타일)
- **Checksum Verification**: SHA-256 checksums for integrity / 무결성을 위한 SHA-256 체크섬

### Image Editing
- **Rotation**: 90°, 180°, 270° rotation
- **Flipping**: Horizontal and vertical flip
- **Cropping**: Arbitrary region selection
- **Resizing**: Multiple algorithms (Nearest, Bilinear, Bicubic, Lanczos)
- **Non-Destructive**: Original preserved in history

### Architecture
```
manypictures/
├── src/
│   ├── core/           # Core data structures and memory management
│   │   ├── types.h     # Type definitions
│   │   ├── memory.c/h  # Custom memory allocator
│   │   └── image.c/h   # Image buffer management
│   ├── formats/        # Image format handlers
│   │   ├── bmp.c       # BMP codec (fully implemented)
│   │   ├── png.c       # PNG codec (custom DEFLATE)
│   │   ├── jpeg.c      # JPEG codec (custom DCT)
│   │   ├── gif.c       # GIF codec (LZW)
│   │   ├── tiff.c      # TIFF codec
│   │   └── webp.c      # WebP codec
│   ├── codecs/         # Compression codecs
│   │   ├── deflate.c/h # DEFLATE compression
│   │   └── jpeg.c/h    # JPEG DCT/Huffman
│   ├── exif/           # EXIF metadata handling
│   │   └── exif.c/h    # EXIF reader/writer with history
│   ├── operations/     # Image operations
│   │   ├── color_ops.c/h  # Color manipulation
│   │   └── edit_ops.c/h   # Geometric operations
│   ├── gui/            # GUI system
│   │   └── gui.c/h     # Custom GUI framework
│   ├── video/          # Video support
│   └── main.c          # Application entry point
├── Makefile            # Build system
└── README.md           # This file
```

## 🔧 Building

### Requirements
- GCC or Clang compiler
- Make
- Linux/Unix system (for X11 GUI)
- Math library (libm)

### Compilation
```bash
# Standard build
make

# Debug build with symbols
make debug

# Optimized release build
make release

# Clean build artifacts
make clean
```

### Installation
```bash
# Install to /usr/local/bin
sudo make install

# Uninstall
sudo make uninstall
```

## 📖 Usage

### GUI Mode
```bash
# Open image viewer
./build/bin/manypictures

# Open specific image
./build/bin/manypictures image.jpg
```

### Command-Line Mode

#### Basic Operations
```bash
# Convert to grayscale
./build/bin/manypictures -g input.jpg -o output.jpg

# Colorize grayscale image
./build/bin/manypictures -c gray.jpg -o color.jpg

# Invert colors
./build/bin/manypictures -i input.png -o output.png

# Invert and grayscale
./build/bin/manypictures -ig input.jpg -o output.jpg
```

#### Image Editing
```bash
# Rotate 90 degrees
./build/bin/manypictures -r 90 input.jpg -o output.jpg

# Resize to 800x600
./build/bin/manypictures -s 800x600 input.jpg -o output.jpg
```

#### Information
```bash
# Show image information
./build/bin/manypictures --info image.jpg

# Show edit history
./build/bin/manypictures --history image.jpg

# Show version
./build/bin/manypictures --version

# Show help
./build/bin/manypictures --help
```

## 🎨 Technical Details

### Custom Implementations

#### DEFLATE Compression
- Bit-level stream processing
- Huffman tree construction
- LZ77 sliding window
- Fixed and dynamic Huffman codes
- CRC32 and Adler32 checksums

#### PNG Decoder
- Chunk parsing (IHDR, IDAT, IEND, etc.)
- Filter reconstruction (None, Sub, Up, Average, Paeth)
- Interlaced image support (Adam7)
- Transparency handling

#### JPEG Codec
- DCT/IDCT implementation
- Huffman encoding/decoding
- Quantization tables
- YCbCr color space conversion
- Progressive JPEG support

#### Color Operations
- RGB ↔ HSV conversion
- Perceptual grayscale (ITU-R BT.601)
- Neural network colorization
- Tone curve adjustments

#### Memory Management
- Custom allocator with tracking
- Memory arena for temporary allocations
- Pool-based allocation
- Leak detection

### EXIF History Format

Many Pictures stores operation history in EXIF metadata using custom tags:

```c
// Custom EXIF tags
#define EXIF_TAG_MP_HISTORY 0x9000
#define EXIF_TAG_MP_VERSION 0x9001
#define EXIF_TAG_MP_OPERATION_COUNT 0x9002

// History entry structure
typedef struct {
    mp_operation_type op_type;    // Operation type
    u64 timestamp;                // Unix timestamp
    char description[256];        // Human-readable description
    u8 params[512];              // Operation parameters
    u32 param_size;              // Parameter size
    u8 checksum[32];             // SHA-256 checksum
} mp_history_entry;
```

Each operation is recorded with:
- Operation type (grayscale, invert, rotate, etc.)
- Timestamp
- Parameters (rotation angle, resize dimensions, etc.)
- Checksum for verification

### Colorization Algorithm

The grayscale-to-color conversion uses a simplified neural network:

1. **Context Analysis**: Examines 8 surrounding pixels
2. **Feature Extraction**: Luminance patterns and gradients
3. **Neural Network**: 3-layer network (9→32→16→3)
4. **Color Prediction**: RGB values based on context
5. **Post-Processing**: Smoothing and refinement

## 🧪 Testing

```bash
# Run basic tests
make test

# Memory leak detection (requires valgrind)
make memcheck

# Static analysis (requires cppcheck)
make analyze

# Code statistics
make stats
```

## 📊 Code Statistics

- **Total Lines**: ~15,000+ lines of C code
- **Files**: 30+ source files
- **Modules**: 7 major subsystems
- **Custom Implementations**: 100% self-implemented codecs

## 🎯 Design Philosophy

### Pure C Implementation
- No external image libraries (no libpng, libjpeg, etc.)
- All codecs implemented from specifications
- Educational and demonstrative purpose
- Maximum control over every aspect

### Complex Architecture
- Multi-layered abstraction
- Modular design with clear interfaces
- Extensive use of function pointers
- Custom memory management
- State machines for parsing

### Performance Considerations
- Memory pooling for frequent allocations
- SIMD-friendly data layouts
- Cache-aware algorithms
- Minimal memory copies

## 🚀 Future Enhancements

### Planned Features
- [ ] Full JPEG progressive decoding
- [ ] GIF animation support
- [ ] TIFF multi-page support
- [ ] WebP VP8/VP8L decoder
- [ ] Video frame extraction
- [ ] Batch processing mode
- [ ] Plugin system
- [ ] GPU acceleration (OpenCL)
- [ ] Advanced filters (blur, sharpen, etc.)
- [ ] Layer support (PSD-like)

### Advanced Operations
- [ ] Content-aware scaling
- [ ] Perspective correction
- [ ] HDR tone mapping
- [ ] Noise reduction
- [ ] Super-resolution
- [ ] Style transfer

## 📝 License

This project is created for educational purposes demonstrating advanced C programming techniques.

## 🤝 Contributing

This is a demonstration project showcasing pure C implementation. Contributions focusing on:
- Additional codec implementations
- Performance optimizations
- Bug fixes
- Documentation improvements

are welcome.

## 📚 References

### Specifications
- PNG: RFC 2083
- JPEG: ITU-T T.81
- DEFLATE: RFC 1951
- EXIF: JEITA CP-3451
- BMP: Microsoft BMP Format Specification

### Algorithms
- DCT: Discrete Cosine Transform (JPEG)
- Huffman Coding: Data compression
- LZ77: Lempel-Ziv compression
- Paeth Filter: PNG prediction
- Bilinear Interpolation: Image scaling

## 🎓 Educational Value

This project demonstrates:
- **Low-level programming**: Bit manipulation, byte ordering
- **Algorithm implementation**: Compression, encoding, transforms
- **Data structures**: Trees, linked lists, hash tables
- **Memory management**: Custom allocators, pools, arenas
- **File formats**: Binary parsing, chunk-based formats
- **Image processing**: Color spaces, filters, transformations
- **Software architecture**: Modular design, abstraction layers

## 💡 Key Insights

### Why Pure C?
1. **Educational**: Understanding how things work at the lowest level
2. **Performance**: Direct control over memory and CPU
3. **Portability**: C runs everywhere
4. **Challenge**: Implementing complex algorithms from scratch

### Complexity Highlights
- **DEFLATE**: 1000+ lines of bit-level stream processing
- **PNG**: Complete chunk parser with filter reconstruction
- **JPEG**: DCT implementation with quantization
- **EXIF**: Binary format parser with endianness handling
- **Colorization**: Neural network in pure C

---

**Many Pictures** - Where complexity meets elegance in pure C.

*"The best way to understand something is to build it yourself."*
