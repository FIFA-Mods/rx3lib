#include "Rx3VertexFormat.h"
#include "ModelTypes.h"

using namespace rx3utils;

namespace rx3::vertex_format {

char const *DataTypeNames[] = {
    "unknown", "void", "1f32", "1s32", "1s16", "1s8", "2f32", "2s32", "2s16", "2s8", "3f32", "3s32",
    "3s16", "3s8", "4f32", "4s32", "4s16", "4s8", "4u8", "4u8n", "4u8endianswapp", "4u8nendianswap",
    "2s16n", "4s16n", "3u10", "3s10n", "3s11n", "2f16", "4f16", "2s16s", "3s16s", "1u16rgb565",
    "3u8rgb8", "4u8rgbx8", "1u16rgba4", "3u8rgba6", "4u8rgba8", "2u16", "4u16", "2u16n", "4u16n", "custom"
};

uint32_t DataTypeTotalSize[] = {
    0, 0, 4, 4, 2, 1, 8, 8, 4, 2, 12, 12, 6, 3, 16, 16, 8, 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 8, 4, 6, 2, 3, 4, 2, 3, 4, 4, 8, 4, 8, 0
};

DataType DataTypeIdFromName(string const &name) {
    for (size_t i = 0; i < size(DataTypeNames); i++) {
        if (name == DataTypeNames[i])
            return (DataType)i;
    }
    return dt_unknown;
}

int32_t DecodeSigned10(uint32_t bits) {
    static const int32_t kSignExtend[2] = { 0, -512 };
    int32_t v = bits & 0x3FF;
    return v | kSignExtend[v >> 9];
}

uint32_t EncodeSigned10(float value) {
    value = max(-1.0f, min(1.0f, value));
    int32_t i = (int32_t)lround(value * 511.0f);
    return (uint32_t)i & 0x3FF;
}

float UnpackFloatFrom10Bit(int value) {
    return (float)value / 511.0f;
}

float UnpackFloatFrom11Bit(int value) {
    return (float)value / 1023.0f;
}

array<float, 4> UnpackVertexAttribute(DataType dt, const unsigned char *data) {
    if (!data) return { 0, 0, 0, 0 };

    if (dt == dt_1f32) {
        float v; memcpy(&v, data, sizeof(v));
        return { v, 0, 0, 1 };
    }
    if (dt == dt_1s32) {
        int32_t v; memcpy(&v, data, sizeof(v));
        return { (float)v, 0, 0, 1 };
    }
    if (dt == dt_1s16) {
        int16_t v; memcpy(&v, data, sizeof(v));
        return { (float)v, 0, 0, 1 };
    }
    if (dt == dt_1s8) {
        int8_t v; memcpy(&v, data, sizeof(v));
        return { (float)v, 0, 0, 1 };
    }

    if (dt == dt_2f32) {
        float v[2]; memcpy(v, data, sizeof(v));
        return { v[0], v[1], 0, 1 };
    }
    if (dt == dt_2s32) {
        int32_t v[2]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], 0, 1 };
    }
    if (dt == dt_2s16) {
        int16_t v[2]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], 0, 1 };
    }
    if (dt == dt_2s8) {
        int8_t v[2]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], 0, 1 };
    }

    if (dt == dt_3f32) {
        float v[3]; memcpy(v, data, sizeof(v));
        return { v[0], v[1], v[2], 1 };
    }
    if (dt == dt_3s32) {
        int32_t v[3]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], 1 };
    }
    if (dt == dt_3s16) {
        int16_t v[3]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], 1 };
    }
    if (dt == dt_3s8) {
        int8_t v[3]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], 1 };
    }

    if (dt == dt_4f32) {
        float v[4]; memcpy(v, data, sizeof(v));
        return { v[0], v[1], v[2], v[3] };
    }
    if (dt == dt_4s32) {
        int32_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], (float)v[3] };
    }
    if (dt == dt_4s16) {
        int16_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], (float)v[3] };
    }
    if (dt == dt_4s8) {
        int8_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], (float)v[3] };
    }

    // Unsigned 8-bit integer color
    if (dt == dt_4u8) {
        uint8_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], (float)v[3] };
    }

    // Normalized unsigned 8-bit color
    if (dt == dt_4u8n) {
        uint8_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0] / 255.0f, (float)v[1] / 255.0f, (float)v[2] / 255.0f, (float)v[3] / 255.0f };
    }

    // Unsigned 8-bit integer color endian swap
    if (dt == dt_4u8endianswapp) {
        uint8_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[3], (float)v[2], (float)v[1], (float)v[0] };
    }

    // Normalized unsigned 8-bit color endian swap
    if (dt == dt_4u8nendianswap) {
        uint8_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[3] / 255.0f, (float)v[2] / 255.0f, (float)v[1] / 255.0f, (float)v[0] / 255.0f };
    }

    // 10-bit unsigned integer vector (e.g., GL_RGB10)
    if (dt == dt_3u10) {
        uint32_t packed; memcpy(&packed, data, sizeof(packed));
        uint32_t x = (packed >> 0) & 0x3FF;
        uint32_t y = (packed >> 10) & 0x3FF;
        uint32_t z = (packed >> 20) & 0x3FF;
        return { (float)x / 1023.0f, (float)y / 1023.0f, (float)z / 1023.0f, 1 };
    }

    // 10-bit signed normalized vector
    if (dt == dt_3s10n) {
        uint32_t packed;
        memcpy(&packed, data, sizeof(packed));
        int32_t x = DecodeSigned10(packed);
        int32_t y = DecodeSigned10(packed >> 10);
        int32_t z = DecodeSigned10(packed >> 20);
        const float kScale = 1.0f / 511.0f;
        return { (float)x * kScale, (float)y * kScale, (float)z * kScale, 1 };
    }

    // Half-float (16-bit float)
    if (dt == dt_2f16) {
        uint16_t v[2]; memcpy(v, data, sizeof(v));
        return { HalfFloatToFloat(v[0]), HalfFloatToFloat(v[1]), 0, 1 };
    }
    if (dt == dt_4f16) {
        uint16_t v[4]; memcpy(v, data, sizeof(v));
        return { HalfFloatToFloat(v[0]), HalfFloatToFloat(v[1]), HalfFloatToFloat(v[2]), HalfFloatToFloat(v[3]) };
    }

    // RGB565 format
    if (dt == dt_1u16rgb565) {
        uint16_t packed; memcpy(&packed, data, sizeof(packed));
        uint8_t r = (packed >> 11) & 0x1F;
        uint8_t g = (packed >> 5) & 0x3F;
        uint8_t b = (packed >> 0) & 0x1F;
        return { (float)r / 31.0f, (float)g / 63.0f, (float)b / 31.0f, 1 };
    }

    // RGBA4 format
    if (dt == dt_1u16rgba4) {
        uint16_t packed; memcpy(&packed, data, sizeof(packed));
        uint8_t r = (packed >> 12) & 0xF;
        uint8_t g = (packed >> 8) & 0xF;
        uint8_t b = (packed >> 4) & 0xF;
        uint8_t a = (packed >> 0) & 0xF;
        return { (float)r / 15.0f, (float)g / 15.0f, (float)b / 15.0f, (float)a / 15.0f };
    }

    if (dt == dt_3s11n) {
        uint32_t packed;
        memcpy(&packed, data, sizeof(packed));
        int32_t x = ((packed >> 0) & 0x7FF) - ((packed & 0x400) ? 2048 : 0);
        int32_t y = ((packed >> 11) & 0x7FF) - ((packed & 0x200000) ? 2048 : 0);
        int32_t z = ((packed >> 22) & 0x3FF) - ((packed & 0x80000000) ? 1024 : 0);  // Last 10 bits
        return { UnpackFloatFrom11Bit(x), UnpackFloatFrom11Bit(y), UnpackFloatFrom10Bit(z), 1 };
    }

    // 16-bit signed scaled values
    if (dt == dt_2s16s) {
        int16_t v[2]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], 0, 1 };
    }
    if (dt == dt_3s16s) {
        int16_t v[3]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], 1 };
    }

    // 8-bit unsigned integer RGB
    if (dt == dt_3u8rgb8) {
        uint8_t v[3]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], 1 };
    }

    // 8-bit unsigned integer RGBX
    if (dt == dt_4u8rgbx8) {
        uint8_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], 1 }; // Ignore X channel
    }

    // 6-bit-per-channel RGBA
    if (dt == dt_3u8rgba6) {
        uint8_t v[3]; memcpy(v, data, sizeof(v));
        return { (float)v[0] / 63.0f, (float)v[1] / 63.0f, (float)v[2] / 63.0f, 1 };
    }

    // Standard 8-bit RGBA
    if (dt == dt_4u8rgba8) {
        uint8_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0] / 255.0f, (float)v[1] / 255.0f, (float)v[2] / 255.0f, (float)v[3] / 255.0f };
    }

    // 16-bit unsigned integers
    if (dt == dt_2u16) {
        uint16_t v[2]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], 0, 1 };
    }
    if (dt == dt_4u16) {
        uint16_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0], (float)v[1], (float)v[2], (float)v[3] };
    }

    // 16-bit unsigned normalized
    if (dt == dt_2u16n) {
        uint16_t v[2]; memcpy(v, data, sizeof(v));
        return { (float)v[0] / 65535.0f, (float)v[1] / 65535.0f, 0, 1 };
    }
    if (dt == dt_4u16n) {
        uint16_t v[4]; memcpy(v, data, sizeof(v));
        return { (float)v[0] / 65535.0f, (float)v[1] / 65535.0f, (float)v[2] / 65535.0f, (float)v[3] / 65535.0f };
    }

    // Default case: return zero vector
    return { 0, 0, 0, 0 };
}

Vector2 UnpackVector2(DataType dt, const unsigned char *data) {
    auto unpacked = UnpackVertexAttribute(dt, data);
    return Vector2(unpacked[0], unpacked[1]);
}

Vector3 UnpackVector3(DataType dt, const unsigned char *data) {
    auto unpacked = UnpackVertexAttribute(dt, data);
    return Vector3(unpacked[0], unpacked[1], unpacked[2]);
}

RGBA UnpackColor(DataType dt, const unsigned char *data) {
    auto unpacked = UnpackVertexAttribute(dt, data);
    return RGBA(
        (unsigned char)clamp(unpacked[0] * 255.0f, 0.0f, 255.0f),
        (unsigned char)clamp(unpacked[1] * 255.0f, 0.0f, 255.0f),
        (unsigned char)clamp(unpacked[2] * 255.0f, 0.0f, 255.0f),
        (unsigned char)clamp(unpacked[2] * 255.0f, 0.0f, 255.0f));
}

uint32_t PackVector3(DataType dt, unsigned char *data, Vector3 const &vec) {
    if (dt == dt_3f32)
        memcpy(data, &vec, sizeof(Vector3));
    else if (dt == dt_4f16) {
        uint16_t buf[4] = {
            FloatToHalfFloat(vec.x),
            FloatToHalfFloat(vec.y),
            FloatToHalfFloat(vec.z),
            FloatToHalfFloat(1.0f)
        };
        memcpy(data, buf, 8);
    }
    else if (dt == dt_3s10n) {
        uint32_t x = EncodeSigned10(vec.x);
        uint32_t y = EncodeSigned10(vec.y);
        uint32_t z = EncodeSigned10(vec.z);
        uint32_t packed = x | (y << 10) | (z << 20);
        memcpy(data, &packed, 4);
    }
    return DataTypeTotalSize[dt];
}

uint32_t PackVector2(DataType dt, unsigned char *data, Vector2 const &vec) {
    if (dt == dt_2f32)
        memcpy(data, &vec, sizeof(Vector2));
    else if (dt == dt_2f16) {
        uint16_t buf[2] = {
            FloatToHalfFloat(vec.x),
            FloatToHalfFloat(vec.y)
        };
        memcpy(data, buf, 4);
    }
    return DataTypeTotalSize[dt];
}

 PackedBoneInfo::PackedBoneInfo() {}

 PackedBoneInfo::PackedBoneInfo(uint16_t _bone, uint8_t _weightPacked) {
     bone = _bone;
     weightPacked = _weightPacked;
 }

vector<PackedBoneInfo> GetPackedBones(vector<pair<uint16_t, float>> const &bones) {
    vector<PackedBoneInfo> result;
    size_t n = bones.size();
    if (n == 0)
        return result;
    double sum = 0.0;
    for (auto const &b : bones)
        sum += b.second;
    if (sum <= 0.0)
        return result;
    result.resize(n);
    vector<double> frac(n);
    vector<int> val(n);
    int sumFloor = 0;
    for (size_t i = 0; i < n; i++) {
        double scaled = (bones[i].second / sum) * 255.0;
        val[i] = static_cast<int>(std::floor(scaled));
        frac[i] = scaled - val[i];
        sumFloor += val[i];
        result[i].bone = bones[i].first;
    }
    int remainder = 255 - sumFloor;
    vector<size_t> idx(n);
    iota(idx.begin(), idx.end(), 0);
    stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
        return frac[a] > frac[b];
    });
    for (int i = 0; i < remainder; i++)
        val[idx[i]] += 1;
    for (size_t i = 0; i < n; i++)
        result[i].weightPacked = static_cast<uint8_t>(std::clamp(val[i], 0, 255));
    return result;
}

uint32_t WriteBoneIndices(DataType dt, uint8_t *data, vector<PackedBoneInfo> const &bones, uint8_t numBoneSets,
    uint8_t numBonesToPad) {
    uint16_t lastIndex = 0;
    for (uint8_t i = 0; i < numBoneSets * 4; i++) {
        uint16_t indexToWrite = (i < bones.size()) ? bones[i].bone : lastIndex;
        if (i >= numBonesToPad)
            indexToWrite = 0;
        lastIndex = indexToWrite;
        if (dt == dt_4u8)
            data[i] = static_cast<uint8_t>(indexToWrite);
        else if (dt == dt_4u16)
            memcpy(data + i * 2, &indexToWrite, 2);
    }
    return DataTypeTotalSize[dt] * numBoneSets;
}

uint32_t WriteBoneWeights(DataType dt, uint8_t *data, vector<PackedBoneInfo> const &bones, uint8_t numBoneSets) {
    for (uint8_t i = 0; i < numBoneSets * 4; i++) {
        if (dt == dt_4u8n)
            data[i] = (i < bones.size()) ? bones[i].weightPacked : 0;
    }
    return DataTypeTotalSize[dt] * numBoneSets;
}

}

Rx3VertexDeclElement::Rx3VertexDeclElement(char _usage, uint8_t _usageIndex, uint32_t _offset, rx3::vertex_format::DataType _dataType) {
    dataType = _dataType;
    offset = _offset;
    usage = _usage;
    usageIndex = _usageIndex;
}

void Rx3VertexFormat::AddElement(Rx3VertexDeclElement const &element) {
    elements.push_back(element);
    stride += rx3::vertex_format::DataTypeTotalSize[element.dataType];
}

void Rx3VertexFormat::AddElement(char usage, uint8_t usageIndex, uint32_t offset, rx3::vertex_format::DataType dataType) {
    AddElement(Rx3VertexDeclElement(usage, usageIndex, offset, dataType));
}

inline uint32_t Rx3VertexFormat::Stride() const {
    return stride;
}

void Rx3VertexFormat::FromString(string const &str) {
    auto declElements = Split(str, ' ');
    if (!declElements.empty()) {
        for (size_t d = 0; d < declElements.size(); d++) {
            auto elementInfo = Split(declElements[d], ':');
            string strUsage, strOffset, strDataType;
            if (elementInfo.size() == 5) {
                strUsage = elementInfo[0];
                strOffset = elementInfo[1];
                strDataType = elementInfo[4];
            }
            else if (elementInfo.size() == 4) {
                strUsage = elementInfo[0];
                strOffset = elementInfo[1];
                strDataType = elementInfo[3];
            }
            else if (elementInfo.size() == 3) {
                strUsage = elementInfo[0];
                strOffset = elementInfo[1];
                strDataType = elementInfo[2];
            }
            char usage = 0;
            unsigned char usageIndex = 0;
            if (strUsage.size() == 2) {
                usage = strUsage[0];
                usageIndex = (strUsage[1] >= '0' && strUsage[1] <= '9') ? (strUsage[1] - '0') : 0;
            }
            uint32_t offset = strOffset.empty() ? 0 : SafeConvertInt<uint32_t>(strOffset, true);
            AddElement(usage, usageIndex, offset, rx3::vertex_format::DataTypeIdFromName(strDataType));
        }
    }
}

string Rx3VertexFormat::ToString() const {
    string result;
    for (auto const &e : elements) {
        if (!result.empty())
            result += " ";
        result += Format("%c%X:%02X:00:0001:%s", e.usage, e.usageIndex, e.offset, rx3::vertex_format::DataTypeNames[e.dataType]);
    }
    return result;
}

void VBEndianSwap(Rx3VertexFormat const &vertexFormat, size_t count, uint8_t *data) {
    using namespace rx3::vertex_format;
    size_t stride = vertexFormat.Stride();
    for (auto const &e : vertexFormat.elements) {
        switch (e.dataType) {
        case dt_2f32:
            for (size_t i = 0; i < count; i++) {
                float *pVal = (float *)(data + i * stride + e.offset);
                Rx3SwapEndian(pVal[0]);
                Rx3SwapEndian(pVal[1]);
            }
            break;
        case dt_2s16:
        case dt_2s16n:
        case dt_2f16:
            for (size_t i = 0; i < count; i++) {
                uint16_t *pVal = (uint16_t *)(data + i * stride + e.offset);
                Rx3SwapEndian(pVal[0]);
                Rx3SwapEndian(pVal[1]);
            }
            break;
        case dt_3f32:
            for (size_t i = 0; i < count; i++) {
                float *pVal = (float *)(data + i * stride + e.offset);
                Rx3SwapEndian(pVal[0]);
                Rx3SwapEndian(pVal[1]);
                Rx3SwapEndian(pVal[2]);
            }
            break;
        case dt_4s8:
        case dt_4u8:
        case dt_4u8n:
        case dt_3s10n:
        case dt_3s11n:
            for (size_t i = 0; i < count; i++) {
                uint32_t *pVal = (uint32_t *)(data + i * stride + e.offset);
                Rx3SwapEndian(pVal[0]);
            }
            break;
        case dt_4f16:
            for (size_t i = 0; i < count; i++) {
                uint16_t *pVal = (uint16_t *)(data + i * stride + e.offset);
                Rx3SwapEndian(pVal[0]);
                Rx3SwapEndian(pVal[1]);
                Rx3SwapEndian(pVal[2]);
                Rx3SwapEndian(pVal[3]);
            }
            break;
        }
    }
}
