# Many Pictures - Architecture Document

## Overview

Many Pictures (매니픽쳐스) is a complex, pure C implementation of an advanced image viewer and editor. This document describes the architectural decisions, implementation details, and the intricate web of dependencies that make this project a demonstration of advanced C programming.

## Design Philosophy

### Pure C Implementation
- **Zero External Dependencies**: All image codecs implemented from scratch
- **Self-Contained**: No libpng, libjpeg, or other image libraries
- **Educational Purpose**: Every algorithm visible and understandable
- **Maximum Complexity**: Intentionally intricate to demonstrate C capabilities

### Architectural Principles
1. **Modular Design**: Clear separation of concerns
2. **Layered Architecture**: Core → Codecs → Formats → Operations → GUI
3. **Custom Everything**: Memory management, data structures, algorithms
4. **Performance-Oriented**: Cache-friendly layouts, minimal allocations

## Module Hierarchy

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│                      (main.c, gui/)                      │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────┴────────────────────────────────────┐
│                   Operations Layer                       │
│              (color_ops, edit_ops, exif)                │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────┴────────────────────────────────────┐
│                    Format Layer                          │
│          (bmp, png, jpeg, gif, tiff, webp)              │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────┴────────────────────────────────────┐
│                    Codec Layer                           │
│              (deflate, jpeg, huffman)                    │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────┴────────────────────────────────────┐
│                     Core Layer                           │
│              (types, memory, image)                      │
└─────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Memory Management (`core/memory.c`)

**Purpose**: Custom memory allocator with tracking and arena support

**Key Features**:
- Global allocation tracking
- Peak memory usage monitoring
- Memory arenas for temporary allocations
- Pool-based allocation for frequent objects
- Leak detection on shutdown

**Implementation Details**:
```c
// Memory pool structure
typedef struct mp_memory_pool {
    u8* base;           // Base address
    size_t size;        // Total size
    size_t used;        // Currently used
    size_t peak;        // Peak usage
    struct mp_memory_pool* next;
} mp_memory_pool;

// Arena for temporary allocations
typedef struct {
    mp_memory_pool* pools;
    size_t pool_size;
    u32 pool_count;
} mp_memory_arena;
```

**Complexity**: ~200 lines of intricate pointer manipulation

### 2. Image Buffer (`core/image.c`)

**Purpose**: Core image representation and manipulation

**Key Structures**:
```c
typedef struct {
    u32 width, height;
    u32 stride;
    mp_color_format format;
    u8* data;
    size_t data_size;
    mp_bool owns_data;
} mp_image_buffer;

typedef struct {
    mp_image_buffer* buffer;
    mp_image_metadata* metadata;
    char* filepath;
    mp_bool modified;
    mp_history_chain* history;
} mp_image;
```

**Operations**:
- Buffer creation/destruction
- Pixel get/set with format conversion
- Color format conversion
- Buffer cloning

**Complexity**: ~400 lines with format-specific handling

### 3. DEFLATE Codec (`codecs/deflate.c`)

**Purpose**: Custom DEFLATE compression/decompression for PNG

**Algorithm Components**:
1. **Bit Stream Processing**
   - Bit-level reading/writing
   - Buffer management
   - Byte alignment

2. **Huffman Coding**
   - Tree construction
   - Symbol encoding/decoding
   - Fixed and dynamic tables

3. **LZ77 Compression**
   - Sliding window
   - Match finding
   - Length/distance encoding

4. **Block Processing**
   - Uncompressed blocks
   - Fixed Huffman blocks
   - Dynamic Huffman blocks

**Key Functions**:
```c
mp_result mp_deflate_decompress(mp_deflate_stream* stream);
mp_result mp_deflate_compress(const u8* input, size_t input_size, 
                              u8** output, size_t* output_size);
u32 mp_crc32(const u8* data, size_t size);
u32 mp_adler32(const u8* data, size_t size);
```

**Complexity**: ~500 lines of bit manipulation and state machines

### 4. PNG Format (`formats/png.c`)

**Purpose**: Complete PNG encoder/decoder

**PNG Structure**:
```
PNG File:
├── Signature (8 bytes)
├── IHDR Chunk (Image Header)
├── PLTE Chunk (Palette, optional)
├── IDAT Chunks (Image Data)
├── Ancillary Chunks (tEXt, tIME, etc.)
└── IEND Chunk (End marker)
```

**Filter Types**:
1. **None**: No filtering
2. **Sub**: Difference from left pixel
3. **Up**: Difference from above pixel
4. **Average**: Average of left and above
5. **Paeth**: Paeth predictor algorithm

**Implementation**:
```c
static void mp_png_unfilter_scanline(u8* scanline, const u8* prev_scanline,
                                     u32 width, u32 bytes_per_pixel, 
                                     u8 filter_type);

static u8 mp_paeth_predictor(u8 a, u8 b, u8 c) {
    i32 p = a + b - c;
    i32 pa = abs(p - a);
    i32 pb = abs(p - b);
    i32 pc = abs(p - c);
    
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}
```

**Complexity**: ~600 lines with chunk parsing and filtering

### 5. JPEG Codec (`codecs/jpeg.c`)

**Purpose**: JPEG compression with DCT and Huffman coding

**JPEG Pipeline**:
```
Encoding:
RGB → YCbCr → 8x8 Blocks → DCT → Quantization → Huffman → Bitstream

Decoding:
Bitstream → Huffman → Dequantization → IDCT → 8x8 Blocks → YCbCr → RGB
```

**DCT Implementation**:
```c
void mp_jpeg_fdct(const i16 input[64], i16 output[64]) {
    // Forward Discrete Cosine Transform
    for (int v = 0; v < 8; v++) {
        for (int u = 0; u < 8; u++) {
            float sum = 0.0f;
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    sum += input[y*8+x] * 
                           cos((2*x+1)*u*PI/16) *
                           cos((2*y+1)*v*PI/16);
                }
            }
            output[v*8+u] = sum * C(u) * C(v) / 4;
        }
    }
}
```

**Quantization Tables**:
- Luma (Y) table for brightness
- Chroma (Cb/Cr) tables for color
- Quality-based scaling

**Complexity**: ~800 lines with DCT, quantization, and Huffman

### 6. Color Operations (`operations/color_ops.c`)

**Purpose**: Advanced color manipulation

**Operations**:

1. **Grayscale Conversion**
   ```c
   // ITU-R BT.601 standard
   gray = 0.299*R + 0.587*G + 0.114*B
   ```

2. **Color Inversion**
   ```c
   R' = 255 - R
   G' = 255 - G
   B' = 255 - B
   ```

3. **HSV Conversion**
   ```c
   void mp_rgb_to_hsv(u8 r, u8 g, u8 b, f32* h, f32* s, f32* v);
   void mp_hsv_to_rgb(f32 h, f32 s, f32 v, u8* r, u8* g, u8* b);
   ```

4. **Colorization Network**
   ```c
   typedef struct {
       f32* weights;
       u32 layer_sizes[8];
       u32 num_layers;
   } mp_colorization_network;
   
   // Simplified neural network for grayscale → color
   // Input: gray value + 8 context pixels
   // Hidden: 32 → 16 neurons
   // Output: RGB values
   ```

**Complexity**: ~500 lines with neural network approximation

### 7. Edit Operations (`operations/edit_ops.c`)

**Purpose**: Geometric transformations

**Operations**:

1. **Rotation**
   - 90°, 180°, 270° rotation
   - Coordinate transformation
   - Buffer reallocation

2. **Flipping**
   - Horizontal flip
   - Vertical flip
   - In-place operation

3. **Resizing**
   - Nearest neighbor
   - Bilinear interpolation
   - Bicubic interpolation (planned)
   - Lanczos resampling (planned)

**Bilinear Interpolation**:
```c
static mp_pixel mp_sample_bilinear(const mp_image_buffer* buffer, 
                                   f32 x, f32 y) {
    u32 x0 = (u32)x, y0 = (u32)y;
    u32 x1 = x0 + 1, y1 = y0 + 1;
    f32 fx = x - x0, fy = y - y0;
    
    mp_pixel p00 = get_pixel(buffer, x0, y0);
    mp_pixel p10 = get_pixel(buffer, x1, y0);
    mp_pixel p01 = get_pixel(buffer, x0, y1);
    mp_pixel p11 = get_pixel(buffer, x1, y1);
    
    // Bilinear interpolation formula
    result.r = (1-fx)*(1-fy)*p00.r + fx*(1-fy)*p10.r +
               (1-fx)*fy*p01.r + fx*fy*p11.r;
    // ... same for g, b, a
}
```

**Complexity**: ~400 lines with multiple algorithms

### 8. EXIF Handler (`exif/exif.c`)

**Purpose**: EXIF metadata with custom history tracking

**EXIF Structure**:
```
EXIF Data:
├── TIFF Header
│   ├── Byte Order (II/MM)
│   ├── Magic Number (42)
│   └── IFD Offset
├── IFD0 (Image File Directory)
│   ├── Standard Tags
│   │   ├── Make
│   │   ├── Model
│   │   ├── DateTime
│   │   └── Software
│   └── EXIF SubIFD
│       ├── ExposureTime
│       ├── FNumber
│       └── ISO
└── Custom IFD (Many Pictures)
    ├── MP_HISTORY (0x9000)
    ├── MP_VERSION (0x9001)
    └── MP_OPERATION_COUNT (0x9002)
```

**History Entry**:
```c
typedef struct mp_history_entry {
    mp_operation_type op_type;    // Operation type
    u64 timestamp;                // Unix timestamp
    char description[256];        // Description
    u8 params[512];              // Parameters
    u32 param_size;              // Parameter size
    u8 checksum[32];             // SHA-256 checksum
    struct mp_history_entry* next;
    struct mp_history_entry* prev;
} mp_history_entry;
```

**Git-Like History**:
- Each operation recorded
- Parameters preserved
- Checksums for integrity
- Linked list of operations
- Restoration to any point

**Complexity**: ~400 lines with binary format parsing

### 9. GUI System (`gui/gui.c`)

**Purpose**: Custom GUI framework (stub implementation)

**Widget Hierarchy**:
```
mp_window (main window)
├── mp_widget (toolbar)
│   ├── Button (Open)
│   ├── Button (Save)
│   ├── Button (Grayscale)
│   ├── Button (Colorize)
│   └── Button (Invert)
├── mp_widget (image_view)
│   ├── Scrollbars
│   └── Canvas
└── mp_widget (statusbar)
    ├── Label (filename)
    ├── Label (dimensions)
    └── Label (zoom)
```

**Event System**:
```c
typedef enum {
    MP_EVENT_MOUSE_DOWN,
    MP_EVENT_MOUSE_UP,
    MP_EVENT_MOUSE_MOVE,
    MP_EVENT_KEY_DOWN,
    MP_EVENT_KEY_UP,
    MP_EVENT_PAINT,
    MP_EVENT_RESIZE,
    MP_EVENT_CLOSE
} mp_event_type;

typedef void (*mp_event_callback)(mp_widget* widget, mp_event* event);
```

**Complexity**: ~400 lines (stub), would be 2000+ for full X11/GTK

## Data Flow

### Image Loading
```
File → Format Detection → Format Handler → Codec → Image Buffer
  │           │                 │              │          │
  │           ├─ Magic bytes    │              │          │
  │           └─ Extension      │              │          │
  │                             │              │          │
  │                             ├─ BMP ────────┤          │
  │                             ├─ PNG ─→ DEFLATE         │
  │                             ├─ JPEG ─→ DCT/Huffman    │
  │                             └─ GIF ─→ LZW             │
  │                                                        │
  └────────────────────────────────────────────────────→ Display
```

### Operation Application
```
Image Buffer → Operation → Modified Buffer → History Entry → EXIF
     │             │              │                │           │
     │             ├─ Grayscale   │                │           │
     │             ├─ Colorize    │                │           │
     │             ├─ Invert      │                │           │
     │             ├─ Rotate      │                │           │
     │             └─ Resize      │                │           │
     │                            │                │           │
     └────────────────────────────┴────────────────┴──────────→ Save
```

### History Restoration
```
EXIF → History Chain → Operation Replay → Restored Image
  │          │                │                  │
  │          ├─ Entry 1: Load                    │
  │          ├─ Entry 2: Grayscale               │
  │          ├─ Entry 3: Rotate 90°              │
  │          └─ Entry 4: Resize 800x600          │
  │                                               │
  └───────────────────────────────────────────────┘
```

## Complexity Metrics

### Lines of Code
- **Core**: ~800 lines
- **Codecs**: ~1500 lines
- **Formats**: ~2000 lines
- **Operations**: ~900 lines
- **EXIF**: ~400 lines
- **GUI**: ~400 lines (stub)
- **Main**: ~300 lines
- **Total**: ~6300 lines (without full implementations)

### Full Implementation Estimate
- **Complete JPEG**: +2000 lines
- **Complete GIF**: +800 lines
- **Complete TIFF**: +1000 lines
- **Complete WebP**: +1500 lines
- **Full GUI (X11)**: +3000 lines
- **Video Support**: +2000 lines
- **Total Estimated**: ~16,600 lines

### Algorithmic Complexity
- **DEFLATE**: O(n²) for compression, O(n) for decompression
- **PNG Filtering**: O(n) per scanline
- **JPEG DCT**: O(n²) per 8x8 block
- **Bilinear Resize**: O(w×h) where w,h are output dimensions
- **Colorization**: O(n) with neural network forward pass

## Memory Management

### Allocation Strategy
1. **Small Objects** (<256 bytes): Pool allocation
2. **Medium Objects** (256B-64KB): Direct malloc
3. **Large Objects** (>64KB): Direct malloc with tracking
4. **Temporary Data**: Arena allocation

### Memory Pools
```c
// Example: Image buffer pool
mp_memory_pool* image_pool = mp_pool_create(1024 * 1024); // 1MB pool
mp_image_buffer* buffer = mp_pool_alloc(image_pool, sizeof(mp_image_buffer));
// ... use buffer ...
mp_pool_reset(image_pool); // Reset entire pool at once
```

### Leak Detection
```c
mp_memory_init();
// ... application code ...
mp_memory_shutdown(); // Reports any leaks
```

## Performance Considerations

### Cache Optimization
- **Scanline Processing**: Sequential memory access
- **Block Processing**: 8x8 blocks fit in L1 cache
- **Stride Alignment**: 16-byte aligned buffers

### SIMD Opportunities
- Color conversion (RGB ↔ YCbCr)
- Filter application
- Resize operations
- DCT/IDCT

### Parallelization
- Multi-threaded image loading
- Parallel block processing (JPEG)
- Concurrent filter application

## Testing Strategy

### Unit Tests
- Memory allocator correctness
- Codec round-trip (compress → decompress)
- Color conversion accuracy
- Geometric transformation correctness

### Integration Tests
- Load → Modify → Save → Load
- EXIF preservation
- History restoration
- Format conversion

### Performance Tests
- Large image handling (>100MP)
- Memory usage profiling
- Operation timing
- Codec benchmarks

## Future Enhancements

### Planned Features
1. **Complete Codecs**
   - Full JPEG progressive
   - GIF animation
   - TIFF multi-page
   - WebP VP8/VP8L

2. **Advanced Operations**
   - Content-aware scaling
   - Perspective correction
   - HDR tone mapping
   - Noise reduction

3. **Video Support**
   - Frame extraction
   - Basic playback
   - Thumbnail generation

4. **GPU Acceleration**
   - OpenCL kernels
   - CUDA support
   - Vulkan compute

## Conclusion

Many Pictures demonstrates that complex image processing can be implemented entirely in pure C without external libraries. The project showcases:

- **Low-level programming**: Bit manipulation, binary formats
- **Algorithm implementation**: Compression, transforms, filters
- **Data structures**: Trees, linked lists, pools
- **Memory management**: Custom allocators, tracking
- **Software architecture**: Modular, layered design

The intentional complexity serves as an educational tool and a testament to what can be achieved with careful C programming.

---

**Total Complexity Score**: 🌟🌟🌟🌟🌟 (Maximum)

*"Complexity is not the enemy; it's the canvas on which we paint our understanding."*
