#include "Rx3Textures.h"
#include "Rx3TextureMetadata.h"
#include "Rx3Names.h"
#include "DirectXTex.h"
#include <wincodec.h>
#include <fstream>

using namespace DirectX;
using namespace rx3utils;

DXGI_FORMAT MapRx3FormatToDXGI(unsigned char format) {
    switch (format) {
    case RX3_TEXFORMAT_DXT1:     return DXGI_FORMAT_BC1_UNORM;
    case RX3_TEXFORMAT_DXT3:     return DXGI_FORMAT_BC2_UNORM;
    case RX3_TEXFORMAT_DXT5:     return DXGI_FORMAT_BC3_UNORM;
    case RX3_TEXFORMAT_ARGB8888: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case RX3_TEXFORMAT_L8:       return DXGI_FORMAT_R8_UNORM;
    case RX3_TEXFORMAT_AL8:      return DXGI_FORMAT_R8G8_UNORM;
    case RX3_TEXFORMAT_RG8:      return DXGI_FORMAT_R8G8_UNORM;
    case RX3_TEXFORMAT_BC5:      return DXGI_FORMAT_BC5_UNORM;
    case RX3_TEXFORMAT_RGB565:   return DXGI_FORMAT_B5G6R5_UNORM;
    case RX3_TEXFORMAT_ARGB4444: return DXGI_FORMAT_B4G4R4A4_UNORM;
    case RX3_TEXFORMAT_BC6:      return DXGI_FORMAT_BC6H_UF16;
    case RX3_TEXFORMAT_BC7:      return DXGI_FORMAT_BC7_UNORM;
    case RX3_TEXFORMAT_BC4:      return DXGI_FORMAT_BC4_UNORM;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

char const *Rx3FormatName(unsigned char format) {
    switch (format) {
    case RX3_TEXFORMAT_DXT1:     return "DXT1";
    case RX3_TEXFORMAT_DXT3:     return "DXT3";
    case RX3_TEXFORMAT_DXT5:     return "DXT5";
    case RX3_TEXFORMAT_ARGB8888: return "ARGB8888";
    case RX3_TEXFORMAT_L8:       return "L8";
    case RX3_TEXFORMAT_AL8:      return "AL8";
    case RX3_TEXFORMAT_RG8:      return "RG8";
    case RX3_TEXFORMAT_BC5:      return "BC5";
    case RX3_TEXFORMAT_RGB565:   return "RGB565";
    case RX3_TEXFORMAT_ARGB4444: return "ARGB4444";
    case RX3_TEXFORMAT_BC6:      return "BC6";
    case RX3_TEXFORMAT_BC7:      return "BC7";
    case RX3_TEXFORMAT_BC4:      return "BC4";
    default: return "Unknown";
    }
}

bool RequiresDX10Header(DXGI_FORMAT fmt, unsigned char rx3BaseFormat) {
    switch (fmt) {
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;
    default:
        break;
    }
    if (fmt == DXGI_FORMAT_R8G8_UNORM && rx3BaseFormat == RX3_TEXFORMAT_RG8)
        return true;
    return false;
}

HRESULT ExpandLuminanceAlphaToRGBA(Image const &src, ScratchImage &out) {
    HRESULT hr = out.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, src.width, src.height, 1, 1);
    if (FAILED(hr))
        return hr;
    Image const *dst = out.GetImage(0, 0, 0);
    for (size_t y = 0; y < src.height; y++) {
        unsigned char const *srcRow = src.pixels + y * src.rowPitch;
        unsigned char *dstRow = dst->pixels + y * dst->rowPitch;
        for (size_t x = 0; x < src.width; x++) {
            unsigned char const l = srcRow[x * 2 + 0];
            unsigned char const a = srcRow[x * 2 + 1];
            dstRow[x * 4 + 0] = l;
            dstRow[x * 4 + 1] = l;
            dstRow[x * 4 + 2] = l;
            dstRow[x * 4 + 3] = a;
        }
    }
    return S_OK;
}

void CopyLevelIntoImage(Image const *dstImage, unsigned char const *src, unsigned int srcStride, unsigned int srcHeight) {
    if (!dstImage || !dstImage->pixels || !src)
        return;
    size_t const dstRows = dstImage->rowPitch ? (dstImage->slicePitch / dstImage->rowPitch) : 0;
    size_t const rows = min<size_t>(srcHeight, dstRows);
    size_t const rowBytes = min<size_t>(srcStride, dstImage->rowPitch);
    for (size_t row = 0; row < rows; row++) {
        memcpy(dstImage->pixels + row * dstImage->rowPitch, src + row * srcStride, rowBytes);
    }
}

bool IsSourceOpaque(ScratchImage const &fullImage, Image const &srcImage, unsigned char baseFormat) {
    if (baseFormat == RX3_TEXFORMAT_AL8) {
        for (size_t y = 0; y < srcImage.height; ++y) {
            unsigned char const *row = srcImage.pixels + y * srcImage.rowPitch;
            for (size_t x = 0; x < srcImage.width; ++x) {
                if (row[x * 2 + 1] < 255)
                    return false;
            }
        }
        return true;
    }
    if (!HasAlpha(srcImage.format))
        return true;
    if (fullImage.GetImageCount() == 1)
        return fullImage.IsAlphaAllOpaque();
    ScratchImage single;
    if (FAILED(single.InitializeFromImage(srcImage)))
        return false;
    return single.IsAlphaAllOpaque();
}

unsigned char CalcMaxMipLevel(unsigned short height, unsigned short width) {
    unsigned char result = 1;
    while (width > 1 || height > 1) {
        width = max(width >> 1, 1);
        height = max(height >> 1, 1);
        result++;
    }
    return result;
}

void ExtractTexturesFromRX3(Rx3Container &container, path const &outputDir, Rx3Options const &rx3options) {
    vector<PackedTextureInfo> vecTexInfo;
    auto texNamesSection = container.FindFirstChunk(RX3_CHUNK_NAME_TABLE);
    vector<string> texNames;
    if (texNamesSection) {
        auto names = ExtractNamesFromChunk(texNamesSection);
        for (auto const &[id, name] : names) {
            if (id == RX3_CHUNK_TEXTURE)
                texNames.push_back(name);
        }
    }
    auto textureSections = container.FindAllChunks(RX3_CHUNK_TEXTURE);
    if (textureSections.empty())
        return;
    if (!exists(outputDir))
        create_directories(outputDir);
    bool const saveToPng = rx3options.textureFormat == "png";
    bool const saveToTga = rx3options.textureFormat == "tga";
    bool const isPngOrTga = saveToPng || saveToTga;
    for (size_t i = 0; i < textureSections.size(); i++) {
        string texName;
        if (i < texNames.size() && !texNames[i].empty())
            texName = texNames[i];
        else
            texName = "unnamed_" + to_string(i);
        if (texName.ends_with(".Raster"))
            texName = texName.substr(0, texName.length() - 7);
        Rx3Reader reader(textureSections[i]);
        unsigned int totalSize = reader.Read<unsigned int>();
        unsigned char type = reader.Read<unsigned char>();
        unsigned char baseFormat = reader.Read<unsigned char>();
        unsigned char dataFormat = reader.Read<unsigned char>();
        reader.Skip(1);
        unsigned short width = reader.Read<unsigned short>();
        unsigned short height = reader.Read<unsigned short>();
        unsigned short depth = reader.Read<unsigned short>();
        unsigned short mips = reader.Read<unsigned short>();
        auto &texInfo = vecTexInfo.emplace_back();
        texInfo.name = texName;
        texInfo.width = width;
        texInfo.height = height;
        texInfo.levels = (unsigned char)mips;
        texInfo.format = baseFormat;
        DXGI_FORMAT const dxgiFormat = MapRx3FormatToDXGI(baseFormat);
        bool typeSupported = type == RX3_TEXTURE_2D || type == RX3_TEXTURE_3D || type == RX3_TEXTURE_CUBE || type == RX3_TEXTURE_ARRAY;
        if (dataFormat != RX3_TEXDATAFORMAT_LINEAR || !typeSupported || dxgiFormat == DXGI_FORMAT_UNKNOWN) {
            ::Error(Format("Unsupported texture format (texture %s in file %s)", texName.c_str(), container.mName.c_str()));
            continue;
        }
        if (width == 0 || height == 0 || depth == 0 || mips == 0)
            continue;
        bool const isCubemap = type == RX3_TEXTURE_CUBE && depth == 6;
        bool const isVolume = type == RX3_TEXTURE_3D;
        bool const isArray = type == RX3_TEXTURE_ARRAY;
        bool const forceDds = isVolume || isArray || isCubemap;
        unsigned short const levelsToKeep = (isPngOrTga && !forceDds) ? 1 : mips;
        TexMetadata metadata = {};
        metadata.width = width;
        metadata.height = height;
        metadata.mipLevels = levelsToKeep;
        metadata.format = dxgiFormat;
        if (isVolume) {
            metadata.dimension = TEX_DIMENSION_TEXTURE3D;
            metadata.depth = depth;
            metadata.arraySize = 1;
        }
        else {
            metadata.dimension = TEX_DIMENSION_TEXTURE2D;
            metadata.depth = 1;
            metadata.arraySize = depth;
            if (isCubemap)
                metadata.miscFlags |= TEX_MISC_TEXTURECUBE;
        }
        ScratchImage image;
        HRESULT hr = image.Initialize(metadata);
        if (FAILED(hr)) {
            ::Error(Format("Failed to allocate texture '%s'", texName.c_str()));
            continue;
        }
        if (isVolume) {
            for (unsigned short l = 0; l < mips; l++) {
                unsigned short const curDepth = max<unsigned short>(1, static_cast<unsigned short>(depth >> l));
                for (unsigned short s = 0; s < curDepth; s++) {
                    unsigned int dataStride = reader.Read<unsigned int>();
                    unsigned int dataHeight = reader.Read<unsigned int>();
                    unsigned int levelSize = reader.Read<unsigned int>();
                    reader.Skip(4);
                    if (l < levelsToKeep) {
                        Image const *dstImage = image.GetImage(l, 0, s);
                        auto const *src = reinterpret_cast<unsigned char const *>(reader.GetCurrentPtr());
                        CopyLevelIntoImage(dstImage, src, dataStride, dataHeight);
                    }
                    reader.Skip(levelSize);
                }
            }
        }
        else {
            for (unsigned short f = 0; f < depth; f++) {
                for (unsigned short l = 0; l < mips; l++) {
                    unsigned int dataStride = reader.Read<unsigned int>();
                    unsigned int dataHeight = reader.Read<unsigned int>();
                    unsigned int levelSize = reader.Read<unsigned int>();
                    reader.Skip(4);
                    if (l < levelsToKeep) {
                        Image const *dstImage = image.GetImage(l, f, 0);
                        auto const *src = reinterpret_cast<unsigned char const *>(reader.GetCurrentPtr());
                        CopyLevelIntoImage(dstImage, src, dataStride, dataHeight);
                    }
                    reader.Skip(levelSize);
                }
            }
        }
        if (dxgiFormat == DXGI_FORMAT_BC6H_UF16 && rx3options.writeHDR && !forceDds) {
            Image const *srcImage = image.GetImage(0, 0, 0);
            if (!srcImage) {
                ::Error(Format("Texture '%s' has no image data", texName.c_str()));
                continue;
            }
            ScratchImage converted;
            Image const *finalSrc = srcImage;
            HRESULT hr = Decompress(*srcImage, DXGI_FORMAT_R16G16B16A16_FLOAT, converted);
            if (FAILED(hr)) {
                ::Error(Format("Failed to convert texture '%s' for HDR export", texName.c_str()));
                continue;
            }
            if (converted.GetImageCount() > 0)
                finalSrc = converted.GetImage(0, 0, 0);
            path const destPath = outputDir / (texName + ".hdr");
            hr = SaveToHDRFile(*finalSrc, destPath.c_str());
            if (FAILED(hr)) {
                ::Error(Format("Failed to save texture '%s'", texName.c_str()));
                continue;
            }
            continue;
        }
        if (isPngOrTga && !forceDds) {
            Image const *srcImage = image.GetImage(0, 0, 0);
            if (!srcImage) {
                ::Error(Format("Texture '%s' has no image data", texName.c_str()));
                continue;
            }
            bool opaque = IsSourceOpaque(image, *srcImage, baseFormat);
            DXGI_FORMAT colorTarget = opaque ? DXGI_FORMAT_B8G8R8X8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
            ScratchImage converted;
            Image const *finalSrc = srcImage;
            HRESULT hr = S_OK;
            if (baseFormat == RX3_TEXFORMAT_AL8) {
                ScratchImage rgba;
                hr = ExpandLuminanceAlphaToRGBA(*srcImage, rgba);
                if (SUCCEEDED(hr))
                    hr = Convert(*rgba.GetImage(0, 0, 0), colorTarget, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
            }
            else if (IsCompressed(srcImage->format))
                hr = Decompress(*srcImage, colorTarget, converted);
            else if (srcImage->format != colorTarget)
                hr = Convert(*srcImage, colorTarget, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
            if (FAILED(hr)) {
                ::Error(Format("Failed to convert texture '%s' for PNG/TGA export", texName.c_str()));
                continue;
            }
            if (converted.GetImageCount() > 0)
                finalSrc = converted.GetImage(0, 0, 0);
            path destPath = outputDir / (texName + (saveToPng ? ".png" : ".tga"));
            hr = saveToPng
                ? SaveToWICFile(*finalSrc, WIC_FLAGS_FORCE_SRGB, GUID_ContainerFormatPng, destPath.c_str())
                : SaveToTGAFile(*finalSrc, TGA_FLAGS_NONE, destPath.c_str());
            if (FAILED(hr)) {
                ::Error(Format("Failed to save texture '%s'", texName.c_str()));
                continue;
            }
            continue;
        }
        DDS_FLAGS ddsFlags = DDS_FLAGS_NONE;
        if (RequiresDX10Header(metadata.format, baseFormat))
            ddsFlags |= DDS_FLAGS_FORCE_DX10_EXT;
        path destPath = outputDir / (texName + ".dds");
        hr = SaveToDDSFile(image.GetImages(), image.GetImageCount(), metadata, ddsFlags, destPath.c_str());
        if (FAILED(hr)) {
            ::Error(Format("Failed to save texture '%s'", texName.c_str()));
            continue;
        }
    }
    if (rx3options.writeTexMetadata && !vecTexInfo.empty()) {
        ofstream outMetadata(outputDir / (container.mName + "_metadata.csv"));
        for (auto const &texInfo : vecTexInfo) {
            outMetadata << "\"" << texInfo.name << "\"," << Rx3FormatName(texInfo.format);
            int maxLevels = CalcMaxMipLevel(texInfo.width, texInfo.height);
            if (texInfo.levels < maxLevels)
                outMetadata << "," << (texInfo.levels == 1 ? 1 : ((int)texInfo.levels - maxLevels));
            outMetadata << endl;
        }
    }
}

unsigned char MapDXGIToRx3Format(DXGI_FORMAT fmt) {
    switch (fmt) {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return RX3_TEXFORMAT_DXT1;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return RX3_TEXFORMAT_DXT3;
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return RX3_TEXFORMAT_DXT5;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return RX3_TEXFORMAT_ARGB8888;
    case DXGI_FORMAT_R8_UNORM:
        return RX3_TEXFORMAT_L8;
    case DXGI_FORMAT_R8G8_UNORM:
        return RX3_TEXFORMAT_RG8;
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
        return RX3_TEXFORMAT_BC5;
    case DXGI_FORMAT_B5G6R5_UNORM:
        return RX3_TEXFORMAT_RGB565;
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return RX3_TEXFORMAT_ARGB4444;
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
        return RX3_TEXFORMAT_BC6;
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return RX3_TEXFORMAT_BC7;
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return RX3_TEXFORMAT_BC4;
    default: return RX3_TEXFORMAT_DXT1; // Fallback
    }
}

bool IsPowerOfTwo(size_t x) {
    return x > 0 && (x & (x - 1)) == 0;
}

size_t NextPowerOfTwo(size_t x) {
    size_t p = 1;
    while (p < x) p <<= 1;
    return p;
}

size_t ComputeFullMipCount(size_t w, size_t h, size_t d = 1) {
    size_t levels = 1;
    while (w > 1 || h > 1 || d > 1) {
        if (w > 1) w >>= 1;
        if (h > 1) h >>= 1;
        if (d > 1) d >>= 1;
        levels++;
    }
    return levels;
}

int FindMipLevelForSize(size_t origW, size_t origH, size_t targetW, size_t targetH, size_t maxLevels) {
    size_t w = origW, h = origH;
    for (size_t level = 0; level < maxLevels; ++level) {
        if (w == targetW && h == targetH)
            return static_cast<int>(level);
        if (w == 1 && h == 1)
            break;
        if (w > 1) w >>= 1;
        if (h > 1) h >>= 1;
    }
    return -1;
}

static bool ExtractSubMipChain(ScratchImage const &src, size_t dropLevels, ScratchImage &dst) {
    auto const &meta = src.GetMetadata();
    if (dropLevels == 0 || dropLevels >= meta.mipLevels)
        return false;

    TexMetadata newMeta = meta;
    newMeta.mipLevels = meta.mipLevels - dropLevels;
    for (size_t i = 0; i < dropLevels; ++i) {
        if (newMeta.width > 1) newMeta.width >>= 1;
        if (newMeta.height > 1) newMeta.height >>= 1;
        if (newMeta.dimension == TEX_DIMENSION_TEXTURE3D && newMeta.depth > 1) newMeta.depth >>= 1;
    }

    HRESULT hr = dst.Initialize(newMeta, CP_FLAGS_NONE);
    if (FAILED(hr))
        return false;

    if (meta.dimension == TEX_DIMENSION_TEXTURE3D) {
        for (size_t l = 0; l < newMeta.mipLevels; ++l) {
            size_t curDepth = max<size_t>(1, newMeta.depth >> l);
            for (size_t s = 0; s < curDepth; ++s) {
                const Image *srcImg = src.GetImage(l + dropLevels, 0, s);
                const Image *dstImg = dst.GetImage(l, 0, s);
                if (!srcImg || !dstImg) return false;
                memcpy(dstImg->pixels, srcImg->pixels, dstImg->slicePitch);
            }
        }
    }
    else {
        for (size_t item = 0; item < meta.arraySize; ++item) {
            for (size_t l = 0; l < newMeta.mipLevels; ++l) {
                const Image *srcImg = src.GetImage(l + dropLevels, item, 0);
                const Image *dstImg = dst.GetImage(l, item, 0);
                if (!srcImg || !dstImg) return false;
                memcpy(dstImg->pixels, srcImg->pixels, dstImg->slicePitch);
            }
        }
    }
    return true;
}

struct RawDDSPixelFormat {
    unsigned int size;
    unsigned int flags;
    unsigned int fourCC;
    unsigned int rgbBitCount;
    unsigned int rBitMask;
    unsigned int gBitMask;
    unsigned int bBitMask;
    unsigned int aBitMask;
};

enum : unsigned int {
    DDPF_ALPHAPIXELS = 0x1,
    DDPF_FOURCC = 0x4,
    DDPF_LUMINANCE = 0x20000,
};

constexpr unsigned int FourCC(char a, char b, char c, char d) {
    return (unsigned int)(unsigned char)a | ((unsigned int)(unsigned char)b << 8) |
        ((unsigned int)(unsigned char)c << 16) | ((unsigned int)(unsigned char)d << 24);
}

bool WasSourceA8L8(path const &ddsPath) {
    ifstream f(ddsPath, ios::binary);
    if (!f)
        return false;
    char magic[4];
    f.read(magic, 4);
    if (f.gcount() != 4 || memcmp(magic, "DDS ", 4) != 0)
        return false;
    f.seekg(72, ios::cur);
    RawDDSPixelFormat pf{};
    f.read(reinterpret_cast<char *>(&pf), sizeof(pf));
    if (f.gcount() != sizeof(pf))
        return false;
    if ((pf.flags & DDPF_FOURCC) && pf.fourCC == FourCC('D', 'X', '1', '0'))
        return false;
    return (pf.flags & DDPF_LUMINANCE) && (pf.flags & DDPF_ALPHAPIXELS) && pf.rgbBitCount == 16;
}

bool ImportTexturesToRX3(Rx3Container &rx3, vector<PackedTextureInfo> const &textures, Rx3Options const &rx3options,
    bool withoutNames)
{
    for (auto const &t : textures) {
        auto ext = t.filePath.extension();
        string extStr = ext.string();
        transform(extStr.begin(), extStr.end(), extStr.begin(), ::tolower);

        bool isDDS = (extStr == ".dds");
        bool isHDR = (extStr == ".hdr");
        bool isPNG = (extStr == ".png");
        bool isTGA = (extStr == ".tga");

        ScratchImage image;
        HRESULT hr = S_OK;

        // 1. Load Image Data
        if (isDDS) hr = LoadFromDDSFile(t.filePath.c_str(), DDS_FLAGS_NONE, nullptr, image);
        else if (isPNG) hr = LoadFromWICFile(t.filePath.c_str(), WIC_FLAGS_IGNORE_SRGB, nullptr, image);
        else if (isTGA) hr = LoadFromTGAFile(t.filePath.c_str(), TGA_FLAGS_NONE, nullptr, image);
        else if (isHDR) hr = LoadFromHDRFile(t.filePath.c_str(), nullptr, image);
        else continue; // Unsupported extension

        if (FAILED(hr)) {
            ::Error(Format("Failed to load texture '%s'", t.filePath.string()));
            continue;
        }

        auto const &meta = image.GetMetadata();
        bool isVolume = meta.dimension == TEX_DIMENSION_TEXTURE3D;

        unsigned char rx3Format = 0;
        DXGI_FORMAT targetDxgi = DXGI_FORMAT_UNKNOWN;
        size_t targetW = (t.width > 0) ? t.width : meta.width;
        size_t targetH = (t.height > 0) ? t.height : meta.height;

        if (isDDS) {
            // -----------------------------------------------------------
            // 2.1 DDS source: never touch format/mips unless we have to.
            // -----------------------------------------------------------
            unsigned char sourceRx3Format = MapDXGIToRx3Format(meta.format);
            if (meta.format == DXGI_FORMAT_R8G8_UNORM && WasSourceA8L8(t.filePath))
                sourceRx3Format = RX3_TEXFORMAT_AL8;
            rx3Format = sourceRx3Format;
            if (rx3options.gameConfig.TextureFormats.count(rx3Format))
                rx3Format = rx3options.gameConfig.TextureFormats.at(rx3Format);

            targetDxgi = MapRx3FormatToDXGI(rx3Format);
            if (targetDxgi == DXGI_FORMAT_UNKNOWN) {
                ::Error(Format("Unsupported texture format ('%s')", t.filePath.string()));
                continue;
            }

            size_t originalMips = meta.mipLevels;

            // Power-of-two rule against the target format. This can force a
            // size change even if the caller never asked to resize.
            if (IsCompressed(targetDxgi)) {
                if (!IsPowerOfTwo(targetW)) targetW = NextPowerOfTwo(targetW);
                if (!IsPowerOfTwo(targetH)) targetH = NextPowerOfTwo(targetH);
            }

            bool needResize = (targetW != meta.width || targetH != meta.height);

            // If we're shrinking, see if the requested size is already one
            // of the source's own mip levels - if so, just drop the larger
            // mips instead of resampling.
            if (needResize) {
                int dropLevel = FindMipLevelForSize(meta.width, meta.height, targetW, targetH, originalMips);
                if (dropLevel > 0) {
                    ScratchImage dropped;
                    if (ExtractSubMipChain(image, static_cast<size_t>(dropLevel), dropped)) {
                        swap(image, dropped);
                        needResize = false;
                    }
                }
            }

            if (needResize) {
                // Real resize path: decompress -> resize -> regen mips (if
                // the source had any) -> convert/compress to target format.
                auto const &preMeta = image.GetMetadata();
                if (IsCompressed(preMeta.format)) {
                    ScratchImage decompressed;
                    hr = Decompress(image.GetImages(), image.GetImageCount(), preMeta, DXGI_FORMAT_UNKNOWN, decompressed);
                    if (FAILED(hr)) {
                        ::Error(Format("Failed to decompress texture ('%s')", t.filePath.string()));
                        continue;
                    }
                    swap(image, decompressed);
                }

                auto const &resizeSrcMeta = image.GetMetadata();
                ScratchImage resized;
                hr = Resize(image.GetImages(), image.GetImageCount(), resizeSrcMeta, targetW, targetH, TEX_FILTER_DEFAULT, resized);
                if (FAILED(hr)) {
                    ::Error(Format("Failed to resize texture ('%s')", t.filePath.string()));
                    continue;
                }
                swap(image, resized);

                if (originalMips > 1) {
                    auto const &postResizeMeta = image.GetMetadata();
                    size_t depthForMips = isVolume ? postResizeMeta.depth : 1;
                    size_t newMaxMips = ComputeFullMipCount(targetW, targetH, depthForMips);
                    if (postResizeMeta.mipLevels != newMaxMips) {
                        ScratchImage mipImage;
                        if (isVolume)
                            hr = GenerateMipMaps3D(image.GetImages(), image.GetImageCount(), postResizeMeta, TEX_FILTER_DEFAULT, newMaxMips, mipImage);
                        else
                            hr = GenerateMipMaps(image.GetImages(), image.GetImageCount(), postResizeMeta, TEX_FILTER_DEFAULT, newMaxMips, mipImage);
                        if (FAILED(hr)) {
                            ::Error(Format("Failed to generate texture mipmaps ('%s')", t.filePath.string()));
                            continue;
                        }
                        swap(image, mipImage);
                    }
                }

                auto const &curFormatMeta = image.GetMetadata();
                if (curFormatMeta.format != targetDxgi) {
                    ScratchImage converted;
                    if (IsCompressed(targetDxgi))
                        hr = Compress(image.GetImages(), image.GetImageCount(), curFormatMeta, targetDxgi, TEX_COMPRESS_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
                    else
                        hr = Convert(image.GetImages(), image.GetImageCount(), curFormatMeta, targetDxgi, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
                    if (FAILED(hr)) {
                        ::Error(Format("Failed to convert texture format ('%s')", t.filePath.string()));
                        continue;
                    }
                    swap(image, converted);
                }
            }
            else {
                // No resize needed (either nothing requested, or handled via
                // mip-drop above). Only touch format if it actually differs.
                auto const &curMeta = image.GetMetadata();
                if (curMeta.format != targetDxgi) {
                    if (IsCompressed(curMeta.format)) {
                        ScratchImage decompressed;
                        hr = Decompress(image.GetImages(), image.GetImageCount(), curMeta, DXGI_FORMAT_UNKNOWN, decompressed);
                        if (FAILED(hr)) {
                            ::Error(Format("Failed to decompress texture ('%s')", t.filePath.string()));
                            continue;
                        }
                        swap(image, decompressed);
                    }
                    auto const &decompMeta = image.GetMetadata();
                    if (decompMeta.format != targetDxgi) {
                        ScratchImage converted;
                        if (IsCompressed(targetDxgi))
                            hr = Compress(image.GetImages(), image.GetImageCount(), decompMeta, targetDxgi, TEX_COMPRESS_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
                        else
                            hr = Convert(image.GetImages(), image.GetImageCount(), decompMeta, targetDxgi, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
                        if (FAILED(hr)) {
                            ::Error(Format("Failed to convert texture format ('%s')", t.filePath.string()));
                            continue;
                        }
                        swap(image, converted);
                    }
                }
            }
        }
        else {
            // -----------------------------------------------------------
            // 2.2 Non-DDS source (PNG/TGA/HDR): resize/mips/format are all
            // driven by the request, since there's no source compression
            // or existing mip chain to preserve.
            // -----------------------------------------------------------
            if (t.format == -1) {
                if (isHDR) rx3Format = RX3_TEXFORMAT_BC6;
                else rx3Format = image.IsAlphaAllOpaque() ? RX3_TEXFORMAT_DXT1 : RX3_TEXFORMAT_DXT5;
            }
            else {
                rx3Format = static_cast<unsigned char>(t.format);
            }
            if (rx3options.gameConfig.TextureFormats.count(rx3Format))
                rx3Format = rx3options.gameConfig.TextureFormats.at(rx3Format);

            targetDxgi = MapRx3FormatToDXGI(rx3Format);
            if (targetDxgi == DXGI_FORMAT_UNKNOWN) {
                ::Error(Format("Unsupported texture format ('%s')", t.filePath.string()));
                continue;
            }

            // Power-of-two rule against the target format.
            if (IsCompressed(targetDxgi)) {
                if (!IsPowerOfTwo(targetW)) targetW = NextPowerOfTwo(targetW);
                if (!IsPowerOfTwo(targetH)) targetH = NextPowerOfTwo(targetH);
            }

            // Resize if needed.
            if (targetW != meta.width || targetH != meta.height) {
                ScratchImage resized;
                hr = Resize(image.GetImages(), image.GetImageCount(), meta, targetW, targetH, TEX_FILTER_DEFAULT, resized);
                if (FAILED(hr)) {
                    ::Error(Format("Failed to resize texture ('%s')", t.filePath.string()));
                    continue;
                }
                swap(image, resized);
            }

            // Generate mips if asked.
            auto const &curMeta = image.GetMetadata();
            size_t maxMips = ComputeFullMipCount(targetW, targetH, 1);
            size_t desiredMips = 1;
            if (t.levels == 0) {
                // Auto: full chain for PNG/TGA, single level for HDR.
                desiredMips = isHDR ? 1 : maxMips;
            }
            else if (t.levels < 0) { // sliced: N levels off the top of the full chain
                int sliced = static_cast<int>(maxMips) + t.levels;
                desiredMips = (sliced < 1) ? maxMips : static_cast<size_t>(sliced);
            }
            else {
                desiredMips = static_cast<size_t>(t.levels);
            }
            desiredMips = clamp<size_t>(desiredMips, 1, maxMips);

            if (desiredMips > 1 && curMeta.mipLevels != desiredMips) {
                ScratchImage mipImage;
                hr = GenerateMipMaps(image.GetImages(), image.GetImageCount(), curMeta, TEX_FILTER_DEFAULT, desiredMips, mipImage);
                if (FAILED(hr)) {
                    ::Error(Format("Failed to generate texture mipmaps ('%s')", t.filePath.string()));
                    continue;
                }
                swap(image, mipImage);
            }

            // Convert/compress to target format if needed.
            auto const &curFormatMeta = image.GetMetadata();
            if (curFormatMeta.format != targetDxgi) {
                ScratchImage converted;
                if (IsCompressed(targetDxgi))
                    hr = Compress(image.GetImages(), image.GetImageCount(), curFormatMeta, targetDxgi, TEX_COMPRESS_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
                else
                    hr = Convert(image.GetImages(), image.GetImageCount(), curFormatMeta, targetDxgi, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
                if (FAILED(hr)) {
                    ::Error(Format("Failed to convert texture format ('%s')", t.filePath.string()));
                    continue;
                }
                swap(image, converted);
            }
        }

        // -----------------------------------------------------------------
        // 3. Serialize into RX3 binary format
        // -----------------------------------------------------------------
        auto const &finMeta = image.GetMetadata();
        size_t targetMips = finMeta.mipLevels;

        unsigned char rx3Type = RX3_TEXTURE_2D;
        if (finMeta.dimension == TEX_DIMENSION_TEXTURE3D) rx3Type = RX3_TEXTURE_3D;
        else if (finMeta.miscFlags & TEX_MISC_TEXTURECUBE) rx3Type = RX3_TEXTURE_CUBE;
        else if (finMeta.arraySize > 1) rx3Type = RX3_TEXTURE_ARRAY;

        vector<unsigned char> texData;
        Rx3Writer texDataWriter(texData);

        // Write 16-byte Header
        texDataWriter.Put<unsigned int>(0); // Size placeholder
        texDataWriter.Put<unsigned char>(rx3Type);
        texDataWriter.Put<unsigned char>(rx3Format);
        texDataWriter.Put<unsigned char>(RX3_TEXDATAFORMAT_LINEAR);
        texDataWriter.Put<unsigned char>(0); // Padding to align header
        texDataWriter.Put<unsigned short>(static_cast<unsigned short>(finMeta.width));
        texDataWriter.Put<unsigned short>(static_cast<unsigned short>(finMeta.height));
        texDataWriter.Put<unsigned short>(static_cast<unsigned short>(finMeta.arraySize > 1 && rx3Type != RX3_TEXTURE_3D ? finMeta.arraySize : finMeta.depth));
        texDataWriter.Put<unsigned short>(static_cast<unsigned short>(targetMips));

        // Write Texture Data Mip-by-Mip
        if (rx3Type == RX3_TEXTURE_3D) {
            for (size_t l = 0; l < targetMips; ++l) {
                size_t curDepth = max<size_t>(1, finMeta.depth >> l);
                for (size_t s = 0; s < curDepth; ++s) {
                    const Image *img = image.GetImage(l, 0, s);
                    size_t rowPitch, slicePitch;
                    ComputePitch(finMeta.format, img->width, img->height, rowPitch, slicePitch, CP_FLAGS_NONE);
                    size_t dataHeight = slicePitch / rowPitch;
                    texDataWriter.Put<unsigned int>(static_cast<unsigned int>(rowPitch));
                    texDataWriter.Put<unsigned int>(static_cast<unsigned int>(dataHeight));
                    texDataWriter.Put<unsigned int>(static_cast<unsigned int>(slicePitch));
                    texDataWriter.Put<unsigned int>(0); // padding
                    texDataWriter.Put(img->pixels, slicePitch);
                }
            }
        }
        else {
            // For Arrays, Cubes, and 2D textures (Loop slices, then mips)
            for (size_t f = 0; f < finMeta.arraySize; ++f) {
                for (size_t l = 0; l < targetMips; ++l) {
                    const Image *img = image.GetImage(l, f, 0);
                    size_t rowPitch, slicePitch;
                    ComputePitch(finMeta.format, img->width, img->height, rowPitch, slicePitch, CP_FLAGS_NONE);
                    // Fallback to manual height calc if ComputePitch acts strange for block compression on tiny mips
                    size_t dataHeight = IsCompressed(finMeta.format) ? max<size_t>(1, (img->height + 3) / 4) : img->height;
                    texDataWriter.Put<unsigned int>(static_cast<unsigned int>(rowPitch));
                    texDataWriter.Put<unsigned int>(static_cast<unsigned int>(dataHeight));
                    texDataWriter.Put<unsigned int>(static_cast<unsigned int>(slicePitch));
                    texDataWriter.Put<unsigned int>(0); // padding
                    texDataWriter.Put(img->pixels, slicePitch);
                }
            }
        }
        texDataWriter.AlignAndUpdateTotalSize();
        Rx3Writer texWriter(rx3.AddChunk(RX3_CHUNK_TEXTURE));
        texWriter.Put(texData.data(), texData.size());
    }
    // 7. Write Headers Section (Extract 16-byte headers from all populated textures)
    vector<Rx3Chunk *> textureChunks = rx3.FindAllChunks(RX3_CHUNK_TEXTURE);
    auto textureBatch = rx3.FindFirstChunk(RX3_CHUNK_TEXTURE_BATCH);
    if (!textureBatch)
        textureBatch = &rx3.AddChunk(RX3_CHUNK_TEXTURE_BATCH);
    Rx3Writer texHeadersWriter(textureBatch);
    texHeadersWriter.Put<unsigned int>(static_cast<unsigned int>(textureChunks.size()));
    texHeadersWriter.Align();
    for (auto const &chunk : textureChunks)
        texHeadersWriter.Put(chunk->mData.data(), 16);
    // 8. Write Global Names Table
    if (!withoutNames) {
        vector<string> texNames;
        for (auto const &t : textures) {
            auto name = t.name;
            // TODO: do not add the suffix for some specific containers (wipe3d, stadium)
            if (rx3options.gameConfig.TextureRasterSuffix && !name.ends_with(".Raster"))
                name += ".Raster";
            texNames.push_back(name);
        }
        AddNamesChunkToRx3(rx3, texNames, RX3_CHUNK_TEXTURE);
    }
    return true;
}

bool ImportTexturesToRX3(Rx3Container &rx3, vector<path> const &inTextures, path const &localMetadataFile,
    Rx3Options const &rx3options, bool withoutNames) {
    map<string, TexFormatTarget> metadata = rx3options.texTargetFormats;
    vector<string> order;
    if (!localMetadataFile.empty())
        ReadTexFormatFile(localMetadataFile, metadata, order);

    // Pre-parse all generalized ("X"-containing) entries once.
    vector<TexNamePattern> patterns;
    for (auto const &[key, value] : metadata) {
        if (key.find('X') == string::npos)
            continue; // plain entry, handled by exact match below
        TexNamePattern pattern;
        if (TryParseTexNamePattern(key, pattern))
            patterns.push_back(std::move(pattern));
        // else: ambiguous entry (e.g. "fileXX") - silently ignored
    }

    vector<PackedTextureInfo> textures;
    unordered_map<string, string> matchedKeyByName; // texture name -> metadata key actually used
    for (auto const &p : inTextures) {
        PackedTextureInfo t;
        t.filePath = p;
        t.name = p.stem().string();
        auto nameLowered = ToLower(t.name);

        string const *matchedKey = nullptr;
        if (auto exactIt = metadata.find(nameLowered); exactIt != metadata.end())
            matchedKey = &exactIt->first;
        else
            matchedKey = FindBestPatternMatch(patterns, nameLowered);

        if (matchedKey) {
            auto const &m = metadata.at(*matchedKey);
            t.format = m.format;
            t.width = m.width;
            t.height = m.height;
            t.levels = m.levels;
            matchedKeyByName[nameLowered] = *matchedKey;
        }

        textures.push_back(t);
    }

    unordered_map<string, size_t> orderLookup;
    for (size_t i = 0; i < order.size(); ++i)
        orderLookup[order[i]] = i;

    stable_sort(textures.begin(), textures.end(), [&](const PackedTextureInfo &a, const PackedTextureInfo &b) {
        auto nameA = ToLower(a.name);
        auto nameB = ToLower(b.name);
        auto keyItA = matchedKeyByName.find(nameA);
        auto keyItB = matchedKeyByName.find(nameB);
        string const &lookupKeyA = (keyItA != matchedKeyByName.end()) ? keyItA->second : nameA;
        string const &lookupKeyB = (keyItB != matchedKeyByName.end()) ? keyItB->second : nameB;
        auto itA = orderLookup.find(lookupKeyA);
        auto itB = orderLookup.find(lookupKeyB);
        bool hasA = (itA != orderLookup.end());
        bool hasB = (itB != orderLookup.end());
        if (hasA && hasB)
            return itA->second < itB->second;
        if (hasA)
            return true;
        if (hasB)
            return false;
        return false;
    });

    return ImportTexturesToRX3(rx3, textures, rx3options, withoutNames);
}

PackedTextureInfo::PackedTextureInfo() {}

PackedTextureInfo::PackedTextureInfo(path const &_filePath) {
    name = _filePath.stem().string();
    filePath = _filePath;
}

PackedTextureInfo::PackedTextureInfo(string const &_name, path const &_filePath) {
    name = _name;
    filePath = _filePath;
}

PackedTextureInfo::PackedTextureInfo(string const &_name, path const &_filePath, int _format) {
    name = _name;
    filePath = _filePath;
    format = _format;
}

PackedTextureInfo::PackedTextureInfo(string const &_name, path const &_filePath, int _format, char _levels) {
    name = _name;
    filePath = _filePath;
    format = _format;
    levels = _levels;
}

PackedTextureInfo::PackedTextureInfo(string const &_name, path const &_filePath, int _format, char _levels, unsigned short _width, unsigned short _height) {
    name = _name;
    filePath = _filePath;
    format = _format;
    levels = _levels;
    width = _width;
    height = _height;
}
