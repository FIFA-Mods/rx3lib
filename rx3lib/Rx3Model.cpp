#include "Rx3Model.h"
#include "Rx3Container.h"
#include "Rx3Names.h"
#include "Rx3Scene.h"
#include "Rx3Morph.h"
#include "Rx3Skeleton.h"
#include "MeshOperations/MeshTristrip.h"
#include "MeshOperations/MeshSkinning.h"

using namespace rx3utils;

namespace helper::rx3model {

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

struct PackedBoneInfo {
    uint16_t bone = 0;
    uint8_t weightPacked = 0;

    PackedBoneInfo() {}
    PackedBoneInfo(uint16_t _bone, uint8_t _weightPacked = 0) {
        bone = _bone; weightPacked = _weightPacked;
    }
};

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
    uint8_t numBonesToPad)
{
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

Matrix4x4 ReadMatrix4x4(Rx3Reader &reader) {
    Matrix4x4 mat;
    for (uint32_t r = 0; r < 4; r++) {
        for (uint32_t c = 0; c < 4; c++)
            mat.m[r][c] = reader.Read<float>();
    }
    return mat;
}

void ReadMatrix4x4(Rx3Reader &reader, Matrix4x4 &out) {
    out = ReadMatrix4x4(reader);
}

void WriteMatrix4x4(Rx3Writer &writer, Matrix4x4 const &mat) {
    for (uint32_t r = 0; r < 4; r++) {
        for (uint32_t c = 0; c < 4; c++)
            writer.Put<float>(mat.m[r][c]);
    }
}

Vector3 ReadVector3(Rx3Reader &reader) {
    Vector3 v;
    for (uint32_t i = 0; i < 3; i++)
        v[i] = reader.Read<float>();
    return v;
}

void ReadVector3(Rx3Reader &reader, Vector3 &out) {
    out = ReadVector3(reader);
}

void WriteVector3(Rx3Writer &writer, Vector3 const &v) {
    for (uint32_t i = 0; i < 3; i++)
        writer.Put<float>(v[i]);
}

}

Model ReadModelFromFile(path const &filePath) {
    Model model;
    ModelReadOptions options;
    options.AlwaysTriangulate = false;
    options.MergeMeshes = true;
    options.FbxTangents = ModelReadOptions::FBX_TANGENTS_GENERATE;
    model.Read(filePath, options);
    return model;
}

void SetupObjectMesh(Object &obj, Rx3Chunk *vfChunk, Rx3Chunk *vbChunk, Rx3Chunk *ibChunk, Rx3Chunk *qibChunk, int primType,
    unsigned int numBones, Rx3Options const &options)
{
    using namespace helper::rx3model;
    obj.vertexFormat = 0;
    Rx3Reader vertexDeclReader(vfChunk);
    Rx3Reader vertexBufferReader(vbChunk);
    vertexDeclReader.Skip(4);
    uint32_t declStrLen = vertexDeclReader.Read<uint32_t>();
    if (declStrLen > 0) {
        vertexDeclReader.Skip(8);
        string decl = vertexDeclReader.GetString();
        auto declElements = Split(decl, ' ');
        if (!declElements.empty()) {
            vertexBufferReader.Skip(4);
            uint32_t numVertices = vertexBufferReader.Read<uint32_t>();
            uint32_t vs = vertexBufferReader.Read<uint32_t>();
            vertexBufferReader.Skip(4);
            auto vb = vertexBufferReader.GetCurrentPtr();
            obj.vertices.resize(numVertices);
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
                DataType t = DataTypeIdFromName(strDataType);
                if (usage == 'p') {
                    if (usageIndex == 0) {
                        for (uint32_t v = 0; v < numVertices; v++) {
                            const unsigned char *vd = (const unsigned char *)vb + v * vs + offset;
                            obj.vertices[v].pos = UnpackVector3(t, vd) / 100.0f;
                        }
                    }
                }
                else if (usage == 'n') {
                    if (usageIndex == 0) {
                        obj.vertexFormat |= V_Normal;
                        for (uint32_t v = 0; v < numVertices; v++) {
                            const unsigned char *vd = (const unsigned char *)vb + v * vs + offset;
                            obj.vertices[v].normal = UnpackVector3(t, vd);
                        }
                    }
                }
                else if (usage == 'g') {
                    if (usageIndex == 0) {
                        obj.vertexFormat |= V_Tangent;
                        for (uint32_t v = 0; v < numVertices; v++) {
                            const unsigned char *vd = (const unsigned char *)vb + v * vs + offset;
                            obj.vertices[v].tangent = UnpackVector3(t, vd);
                        }
                    }
                }
                else if (usage == 'b') {
                    if (usageIndex == 0) {
                        obj.vertexFormat |= V_Binormal;
                        for (uint32_t v = 0; v < numVertices; v++) {
                            const unsigned char *vd = (const unsigned char *)vb + v * vs + offset;
                            obj.vertices[v].binormal = UnpackVector3(t, vd);
                        }
                    }
                }
                else if (usage == 't') {
                    if (usageIndex <= 7) {
                        SetNumTexCoords(obj.vertexFormat, usageIndex + 1);
                        for (uint32_t v = 0; v < numVertices; v++) {
                            const unsigned char *vd = (const unsigned char *)vb + v * vs + offset;
                            obj.vertices[v].uv[usageIndex] = UnpackVector2(t, vd);
                            obj.vertices[v].uv[usageIndex].y = 1.0f - obj.vertices[v].uv[usageIndex].y;
                        }
                    }
                }
                else if (usage == 'c') {
                    if (usageIndex <= 7) {
                        SetNumColors(obj.vertexFormat, usageIndex + 1);
                        for (uint32_t v = 0; v < numVertices; v++) {
                            const unsigned char *vd = (const unsigned char *)vb + v * vs + offset;
                            obj.vertices[v].colors[usageIndex] = UnpackColor(t, vd);
                        }
                    }
                }
                else if (usage == 'i') {
                    if (usageIndex <= 1) {
                        SetNumBones(obj.vertexFormat, (usageIndex + 1) * 4);
                        if (t == dt_4u8 && numBones > 255)
                            t = dt_4u16;
                        for (uint32_t v = 0; v < numVertices; v++) {
                            const unsigned char *vd = (const unsigned char *)vb + v * vs + offset;
                            array<float, 4> joints = UnpackVertexAttribute(t, vd);
                            for (uint32_t bi = 0; bi < 4; bi++) {
                                obj.vertices[v].boneIndices[usageIndex * 4 + bi] = (uint16_t)joints[bi];
                            }
                        }
                    }
                }
                else if (usage == 'w') {
                    if (usageIndex <= 1) {
                        for (uint32_t v = 0; v < numVertices; v++) {
                            const unsigned char *vd = (const unsigned char *)vb + v * vs + offset;
                            array<float, 4> weights = UnpackVertexAttribute(t, vd);
                            for (uint32_t bi = 0; bi < 4; bi++) {
                                obj.vertices[v].boneWeights[usageIndex * 4 + bi] = weights[bi];
                            }
                        }
                    }
                }
            }
            auto ReadIndex = [](Rx3Reader &reader, uint8_t stride) -> uint32_t {
                if (stride == 1)
                    return reader.Read<uint8_t>();
                else if (stride == 2)
                    return reader.Read<uint16_t>();
                else if (stride == 4)
                    return reader.Read<uint32_t>();
                return 0;
            };
            if (options.exportQuads && qibChunk) {
                Rx3Reader ibReader(qibChunk);
                ibReader.Skip(4);
                uint32_t numIndices = ibReader.Read<uint32_t>();
                uint8_t is = ibReader.Read<uint8_t>();
                ibReader.Skip(7);
                if (is == 1 || is == 2 || is == 4) {
                    auto &polys = obj.meshes.emplace_back().polygons;
                    polys.resize(numIndices / 4);
                    for (size_t t = 0; t < polys.size(); ++t) {
                        polys[t] = { ReadIndex(ibReader, is), ReadIndex(ibReader, is), ReadIndex(ibReader, is), ReadIndex(ibReader, is) };
                        if (polys[t][2] == polys[t][3])
                            polys[t].pop_back();
                    }
                }
            }
            else {
                Rx3Reader ibReader(ibChunk);
                ibReader.Skip(4);
                uint32_t numIndices = ibReader.Read<uint32_t>();
                uint8_t is = ibReader.Read<uint8_t>();
                ibReader.Skip(7);
                if (is == 1 || is == 2 || is == 4) {
                    auto &triangles = obj.meshes.emplace_back().polygons;
                    if (primType == RX3_PRIM_TRIANGLELIST) {
                        triangles.resize(numIndices / 3);
                        for (size_t t = 0; t < triangles.size(); ++t)
                            triangles[t] = { ReadIndex(ibReader, is), ReadIndex(ibReader, is), ReadIndex(ibReader, is) };
                    }
                    else if (primType == RX3_PRIM_TRIANGLESTRIP) {
                        if (numIndices >= 3) {
                            std::vector<uint32_t> raw(numIndices);
                            for (size_t i = 0; i < numIndices; ++i)
                                raw[i] = ReadIndex(ibReader, is);
                            triangles.reserve(numIndices - 2);
                            for (size_t k = 0; k + 2 < numIndices; ++k) {
                                uint32_t i0 = raw[k];
                                uint32_t i1 = raw[k + 1];
                                uint32_t i2 = raw[k + 2];
                                vector<uint32_t> tri = ((k & 1) == 0) ?
                                    vector<uint32_t>{ i0, i1, i2 } :
                                    vector<uint32_t>{ i1, i0, i2 };
                                if (tri[0] == tri[1] || tri[1] == tri[2] || tri[0] == tri[2])
                                    continue;
                                triangles.push_back(std::move(tri));
                            }
                        }
                    }
                }
            }
        }
    }
}

Model ModelFromSimpleMeshContainer(Rx3Container &rx3, Rx3Options const &options) {
    using namespace helper::rx3model;
    auto ibs = rx3.FindAllChunks(RX3_CHUNK_INDEX_BUFFER);
    auto qbs = rx3.FindAllChunks(RX3_CHUNK_QUAD_INDEX_BUFFER);
    auto vbs = rx3.FindAllChunks(RX3_CHUNK_VERTEX_BUFFER);
    auto vertexFormats = rx3.FindAllChunks(RX3_CHUNK_VERTEX_FORMAT);
    auto meshes = rx3.FindAllChunks(RX3_CHUNK_SIMPLE_MESH);
    auto animationSkin = rx3.FindFirstChunk(RX3_CHUNK_ANIMATION_SKIN);
    if (ibs.empty() || ibs.size() != vbs.size() || ibs.size() != vertexFormats.size() || ibs.size() != meshes.size())
        return Model();
    Model model;
    if (animationSkin && !options.targetSkeleton.bones.empty()) {
        Rx3Reader animationSkinReader(animationSkin);
        animationSkinReader.Skip(4);
        uint32_t numBones = animationSkinReader.Read<uint32_t>();
        animationSkinReader.Skip(8);
        if (numBones == options.targetSkeleton.bones.size()) {
            model.skeleton = options.targetSkeleton;
            auto &bones = model.skeleton.bones;
            vector<Matrix4x4> boneInversedMatrices(numBones);
            for (uint32_t b = 0; b < numBones; b++) {
                ReadMatrix4x4(animationSkinReader, boneInversedMatrices[b]);
                bones[b].properties["ibm"] = boneInversedMatrices[b];
                for (uint32_t j = 0; j < 3; j++)
                    boneInversedMatrices[b].m[3][j] /= 100.0f;
                bones[b].matrix = boneInversedMatrices[b].Inversed();
            }
            for (uint32_t b = 0; b < numBones; b++) {
                if (!bones[b].parent.empty()) {
                    int16_t parentIndex = model.GetBoneIndex(bones[b].parent);
                    if (parentIndex >= 0 && parentIndex < (int32_t)bones.size())
                        bones[b].matrix = boneInversedMatrices[parentIndex] * bones[b].matrix;
                }
            }
        }
    }
    vector<string> meshNames = ExtractNamesFromRx3(rx3, RX3_CHUNK_SIMPLE_MESH);
    vector<string> objectNames(ibs.size());
    for (size_t i = 0; i < ibs.size(); i++) {
        if (i < meshNames.size()) {
            objectNames[i] = meshNames[i];
            if (objectNames[i].ends_with("_.FxRenderableSimple"))
                objectNames[i] = objectNames[i].substr(0, objectNames[i].length() - strlen("_.FxRenderableSimple"));
            else if (objectNames[i].ends_with(".FxRenderableSimple"))
                objectNames[i] = objectNames[i].substr(0, objectNames[i].length() - strlen(".FxRenderableSimple"));
        }
       else
            objectNames[i] = "object_" + to_string(i);
    }
    string nodeName = rx3.mName;
    if (std::find(objectNames.begin(), objectNames.end(), nodeName) != objectNames.end())
        nodeName += "_root";
    model.objects.resize(ibs.size() + 1);
    auto &node = model.objects[0];
    node.name = nodeName;
    for (size_t i = 0; i < ibs.size(); i++) {
        auto &obj = model.objects[i + 1];
        obj.name = objectNames[i];
        obj.parent = nodeName;
        Rx3Reader meshChunkReader(meshes[i]);
        uint16_t primType = meshChunkReader.Read<uint16_t>();
        auto qb = qbs.size() == ibs.size() ? qbs[i] : nullptr;
        SetupObjectMesh(obj, vertexFormats[i], vbs[i], ibs[i], qb, primType, model.skeleton.bones.size(), options);
    }
    return model;
}

Model ModelFromRX3(Rx3Container &rx3, Rx3Options const &options) {
    if (rx3.FindFirstChunk(RX3_CHUNK_SCENE_INSTANCE))
        return ModelFromSceneContainer(rx3, options);
    else if (rx3.FindFirstChunk(RX3_CHUNK_SIMPLE_MESH))
        return ModelFromSimpleMeshContainer(rx3, options);
    else if (rx3.FindFirstChunk(RX3_CHUNK_MORPH_INDEXED) && !options.baseModel.objects.empty())
        return ModelFromMorphTargetsContainer(rx3, options);
    else if (rx3.FindFirstChunk(RX3_CHUNK_SKELETON))
        return ModelFromSkeletonContainer(rx3, options);
    return Model();
}

bool RemapBones(Model &model, map<string, string> boneRemap, Skeleton const &targetSkeleton) {
    if (model.skeleton.bones.empty() || targetSkeleton.bones.empty())
        return false;
    map<string, uint16_t> dstBoneIndex;
    for (size_t i = 0; i < targetSkeleton.bones.size(); i++)
        dstBoneIndex[targetSkeleton.bones[i].name] = (uint16_t)i;
    for (auto &o : model.objects) {
        if (o.vertices.empty())
            continue;
        size_t newBonesPerVertex = 0;
        for (size_t v = 0; v < o.vertices.size(); v++) {
            auto bonesSrc = MeshSkinning::GetVertexBones(o.vertices[v], NumBones(o.vertexFormat));
            map<uint16_t, float> bonesDstMap;
            for (auto const &[boneSrc, weight] : bonesSrc) {
                if (boneSrc < model.skeleton.bones.size()) {
                    auto boneSrcName = model.skeleton.bones[boneSrc].name;
                    if (boneRemap.contains(boneSrcName)) {
                        auto dstBoneName = boneRemap[boneSrcName];
                        if (dstBoneIndex.contains(dstBoneName))
                            bonesDstMap[dstBoneIndex[dstBoneName]] += weight;
                    }
                }
            }
            vector<pair<uint16_t, float>> bonesDst;
            for (auto const &[bone, weight] : bonesDstMap)
                bonesDst.emplace_back(bone, weight);
            MeshSkinning::SetVertexBones(o.vertices[v], bonesDst, true);
            newBonesPerVertex = max(bonesDst.size(), newBonesPerVertex);
        }
        SetNumBones(o.vertexFormat, (uint8_t)newBonesPerVertex);
    }
    model.skeleton = targetSkeleton;
    return true;
}

void ModelToSimpleMeshContainer(Model const &source, Rx3Container &rx3, Rx3Options const &options) {
    using namespace helper::rx3model;
    Model model = source;
    model.MergeMeshes();
    bool remappedBones = false;
    bool hasSkeleton = !model.skeleton.bones.empty() && !options.targetSkeleton.bones.empty();
    if (hasSkeleton) {
        if (!options.poseChangeMatrices.empty())
            MeshSkinning::ChangePose(model, options.poseChangeMatrices);
        remappedBones = RemapBones(model, options.boneRemap, options.targetSkeleton);
    }
    // ibbatch, quadibbatch, vertexformat's, nametable, ib's, qib's, boneremap's, vb's, animationskin's, simplemesh's, adjacency's
    vector<vector<uint8_t>> vbs, ibs, qibs, boneremaps, adjacencies;
    vector<Rx3PrimitiveType> primTypes;
    vector<string> vertexFormats;
    vector<pair<uint32_t, string>> nametable;
    vector<Matrix4x4> ibms;
    DataType posDataType = options.precisePositions ? dt_3f32 : dt_4f16;
    DataType bonesDataType = dt_4u8;
    uint8_t numBoneSets = 0;
    uint8_t numBonesPerVertex = 0;
    uint8_t numBonesToPad = 0;
    vector<vector<vector<PackedBoneInfo>>> packedBonesPerObject;
    bool hasQuads = false;
    for (auto &o : model.objects) {
        for (auto const &p : o.firstMesh().polygons) {
            if (p.size() == 4) {
                hasQuads = true;
                break;
            }
        }
    }

    auto IsObjectWriteable = [](Object const &o) {
        return !o.meshes.empty() && !o.vertices.empty() && !o.firstMesh().polygons.empty();
    };

    // calculate skeleton
    if (hasSkeleton) {
        bool adjustMatrices = false;
        if (options.boneMatricesOption == BONE_MATRICES_FROM_FILE && !remappedBones) { // use skeleton from FBX
            ibms = ComputeBoneInverseBindMatricesForModel(model, options.targetSkeleton);
            adjustMatrices = true;
        }
        if (options.boneMatricesOption == BONE_MATRICES_FROM_BASE_MODEL) {
            ibms = GetSourceBoneInverseBindMatrices(options.baseModel.skeleton);
            adjustMatrices = false; // bone matrices are taken from the reference RX3 file as is
        }
        if (ibms.empty() || ibms.size() != options.targetSkeleton.bones.size()) { // BONE_MATRICES_FROM_SKELETON and also a fallback
            ibms = GetSourceBoneInverseBindMatrices(options.targetSkeleton);
            adjustMatrices = true; // rows in matrices from skeleton file may end with 1
        }
        if (adjustMatrices) {
            for (auto &ibm : ibms) {
                for (size_t r = 0; r < 4; r++)
                    ibm.m[r][3] = 0.0f;
            }
        }
        if (!remappedBones)
            model.RetargetSkeleton(options.targetSkeleton);
        model.LimitBonesPerVertex(options.gameConfig.MaxBonesPerVertex);
        for (auto &o : model.objects) {
            auto &objectPackedBones = packedBonesPerObject.emplace_back();
            if (IsObjectWriteable(o)) {
                objectPackedBones.resize(o.vertices.size());
                for (size_t v = 0; v < o.vertices.size(); v++) {
                    auto &packedBones = objectPackedBones[v];
                    auto bones = MeshSkinning::GetVertexBones(o.vertices[v], NumBones(o.vertexFormat));
                    if (bones.empty())
                        packedBones.push_back(PackedBoneInfo(0, 255));
                    else if (bones.size() == 1)
                        packedBones.push_back(PackedBoneInfo(bones[0].first, 255));
                    else
                        packedBones = GetPackedBones(bones);
                    if (packedBones.empty())
                        packedBones.push_back(PackedBoneInfo(0, 255));
                    else if (packedBones.size() == 1)
                        packedBones[0].weightPacked = 255;
                    numBonesPerVertex = max(static_cast<uint8_t>(packedBones.size()), numBonesPerVertex);
                }
            }
        }
        if (options.gameConfig.MaxBonesPerVertex > 4)
            numBoneSets = (model.skeleton.bones.size() > 255 || numBonesPerVertex > 4) ? 2 : 1;
        else
            numBoneSets = 1;
        numBonesToPad = options.gameConfig.PadAllVertexBufferBoneIndices ? (numBoneSets * 4) : numBonesPerVertex;
        if (model.skeleton.bones.size() > 255)
            bonesDataType = dt_4u16;
    }

    // 2f16, 4f16, 3f32, 4u8n, 4u8, 4u16, 3s10n
    auto AddVertexDecl = [](string &dst, char usage, unsigned char usageIndex, unsigned int offset, DataType dataType) {
        if (!dst.empty())
            dst += " ";
        dst += Format("%c%X:%02X:00:0001:%s", usage, usageIndex, offset, DataTypeNames[dataType]);
        return DataTypeTotalSize[dataType];
    };

    for (size_t oi = 0; oi < model.objects.size(); oi++) {
        auto const &o = model.objects[oi];
        if (IsObjectWriteable(o)) {
            auto mesh = o.firstMesh();
            uint32_t indexSize = o.vertices.size() > 0xFFFF ? 4 : 2;
            nametable.emplace_back(RX3_CHUNK_SIMPLE_MESH, o.name + ".FxRenderableSimple");
            string vf;
            uint32_t vertexOffset = AddVertexDecl(vf, 'p', 0, 0, posDataType);
            if (o.vertexFormat & V_Normal)
                vertexOffset += AddVertexDecl(vf, 'n', 0, vertexOffset, dt_3s10n);
            if (o.vertexFormat & V_Tangent)
                vertexOffset += AddVertexDecl(vf, 'g', 0, vertexOffset, dt_3s10n);
            if (options.binormals && o.vertexFormat & V_Binormal)
                vertexOffset += AddVertexDecl(vf, 'b', 0, vertexOffset, dt_3s10n);
            for (uint8_t t = 0; t < NumTexCoords(o.vertexFormat); t++)
                vertexOffset += AddVertexDecl(vf, 't', t, vertexOffset, dt_2f16);
            if (numBoneSets > 0) {
                for (uint8_t set = 0; set < numBoneSets; set++)
                    vertexOffset += AddVertexDecl(vf, 'i', set, vertexOffset, bonesDataType);
                for (uint8_t set = 0; set < numBoneSets; set++)
                    vertexOffset += AddVertexDecl(vf, 'w', set, vertexOffset, dt_4u8n);
            }
            vertexFormats.push_back(vf);
            vector<uint8_t> skinPalette;
            uint32_t vertexStride = vertexOffset;
            vector<uint8_t> vertexBuffer(o.vertices.size() * vertexStride);
            uint32_t vbOffset = 0;
            for (size_t v = 0; v < o.vertices.size(); v++) {
                vbOffset += PackVector3(posDataType, &vertexBuffer[vbOffset], options.AdjustPosition(o.vertices[v].pos * 100.0f));
                if (o.vertexFormat & V_Normal)
                    vbOffset += PackVector3(dt_3s10n, &vertexBuffer[vbOffset], o.vertices[v].normal);
                if (o.vertexFormat & V_Tangent)
                    vbOffset += PackVector3(dt_3s10n, &vertexBuffer[vbOffset], o.vertices[v].tangent);
                if (options.binormals && o.vertexFormat & V_Binormal)
                    vbOffset += PackVector3(dt_3s10n, &vertexBuffer[vbOffset], o.vertices[v].binormal);
                for (size_t t = 0; t < NumTexCoords(o.vertexFormat); t++) {
                    vbOffset += PackVector2(dt_2f16, &vertexBuffer[vbOffset],
                        Vector2(o.vertices[v].uv[t].x, 1.0f - o.vertices[v].uv[t].y));
                }
                if (numBoneSets > 0) {
                    auto const &bones = packedBonesPerObject[oi][v];
                    uint8_t const *boneIndices = &vertexBuffer[vbOffset];
                    vbOffset += WriteBoneIndices(bonesDataType, &vertexBuffer[vbOffset], bones, numBoneSets, numBonesToPad);
                    vbOffset += WriteBoneWeights(dt_4u8n, &vertexBuffer[vbOffset], bones, numBoneSets);
                    for (int b = 3; b >= 0; b--) {
                        if (std::find(skinPalette.begin(), skinPalette.end(), boneIndices[b]) == skinPalette.end())
                            skinPalette.push_back(boneIndices[b]);
                    }
                }
            }
            // vb
            Rx3Writer vbWriter(vbs.emplace_back());
            vbWriter.Put<uint32_t>(0);
            vbWriter.Put<uint32_t>(o.vertices.size());
            vbWriter.Put<uint32_t>(vertexStride);
            vbWriter.Put<uint32_t>(1);
            vbWriter.Align();
            vbWriter.Put(vertexBuffer.data(), vertexBuffer.size());
            vbWriter.AlignAndUpdateTotalSize();
            if (hasQuads) {
                // qib
                mesh.LeaveTrisAndQuads(o.vertices);
                Rx3Writer qibWriter(qibs.emplace_back());
                qibWriter.Put<uint32_t>(0);
                qibWriter.Put<uint32_t>(mesh.polygons.size() * 4);
                qibWriter.Put<uint32_t>(indexSize);
                qibWriter.Align();
                for (auto const &p : mesh.polygons) {
                    array<uint32_t, 4> quad = { p[0], p[1], p[2], p.size() == 4 ? p[3] : p[2] };
                    for (auto index : quad)
                        indexSize == 4 ? qibWriter.Put<uint32_t>(index) : qibWriter.Put<uint16_t>(index);
                }
                qibWriter.AlignAndUpdateTotalSize();
                // adjacency
                struct AdjacencyRecord {
                    uint32_t count = 0;
                    array<uint32_t, 15> quadIndices{};
                };
                vector<AdjacencyRecord> records(o.vertices.size());
                uint32_t quadIndex = 0;
                for (auto const &p : mesh.polygons) {
                    std::array<uint32_t, 4> quad = { p[0], p[1], p[2], p.size() == 4 ? p[3] : p[2] };
                    for (size_t i = 0; i < 4; i++) {
                        uint32_t vi = quad[i];
                        bool seen = false;
                        for (size_t j = 0; j < i; j++)
                            if (quad[j] == vi) { seen = true; break; }
                        if (seen) continue;
                        auto &rec = records[vi];
                        if (rec.count < 15)
                            rec.quadIndices[rec.count++] = quadIndex;
                    }
                    quadIndex++;
                }
                constexpr float kWeldEpsilonSq = 0.00000011920929f;
                for (size_t a = 0; a < o.vertices.size(); a++) {
                    for (size_t b = a + 1; b < o.vertices.size(); b++) {
                        auto delta = o.vertices[a].pos - o.vertices[b].pos;
                        float distSq = Dot(delta, delta);
                        if (distSq > kWeldEpsilonSq)
                            continue;
                        auto &recA = records[a];
                        auto &recB = records[b];
                        uint32_t origCountA = recA.count;
                        uint32_t origCountB = recB.count;
                        for (uint32_t i = 0; (i < origCountB && recA.count < 15); i++)
                            recA.quadIndices[recA.count++] = recB.quadIndices[i];
                        for (uint32_t i = 0; (i < origCountA && recB.count < 15); i++)
                            recB.quadIndices[recB.count++] = recA.quadIndices[i];
                    }
                }
                Rx3Writer adjacencyWriter(adjacencies.emplace_back());
                adjacencyWriter.Put<uint32_t>(0);
                adjacencyWriter.Align();
                for (auto const &rec : records) {
                    adjacencyWriter.Put<uint32_t>(rec.count);
                    for (uint32_t i = 0; i < 15; i++)
                        adjacencyWriter.Put<uint32_t>(i < rec.count ? rec.quadIndices[i] : 0);
                }
                adjacencyWriter.AlignAndUpdateTotalSize();
            }
            // ib
            mesh.Triangulate(o.vertices);
            Rx3Writer ibWriter(ibs.emplace_back());
            ibWriter.Put<uint32_t>(0);
            vector<uint16_t> tristrips;
            if (options.tristrip && o.vertices.size() < 0xFFFF)
                tristrips = MeshTristrip::GenerateTristrips(mesh.polygons);
            if (!tristrips.empty()) {
                ibWriter.Put(tristrips.size());
                ibWriter.Put(indexSize);
                ibWriter.Align();
                for (uint16_t index : tristrips)
                    ibWriter.Put<uint16_t>(index);
                primTypes.push_back(RX3_PRIM_TRIANGLESTRIP);
            }
            else {
                ibWriter.Put(mesh.polygons.size() * 3);
                ibWriter.Put(indexSize);
                ibWriter.Align();
                for (auto const &p : mesh.polygons) {
                    array<uint32_t, 3> tri = { p[0], p[1], p[2] };
                    for (auto index : tri)
                        indexSize == 4 ? ibWriter.Put<uint32_t>(index) : ibWriter.Put<uint16_t>(index);
                }
                primTypes.push_back(RX3_PRIM_TRIANGLELIST);
            }
            ibWriter.AlignAndUpdateTotalSize();
            // boneremap
            if (hasSkeleton) {
                if (skinPalette.size() > 255) {
                    ::Error(L"Too many bones in the skinning palette (%d)\nIn model %s", skinPalette.size(), source.name.c_str());
                    skinPalette.resize(255);
                }
                else if (skinPalette.size() > options.gameConfig.MaxBonesPerMesh) {
                    ::Error(L"Too many bones in the skinning palette (%d)\nIn model %s", skinPalette.size(), source.name.c_str());
                    skinPalette.resize(options.gameConfig.MaxBonesPerMesh);
                }
                Rx3Writer boneRemapWriter(boneremaps.emplace_back());
                boneRemapWriter.Put<uint32_t>(0);
                boneRemapWriter.Put<uint8_t>(static_cast<uint8_t>(skinPalette.size()));
                boneRemapWriter.Align();
                vector<uint8_t> boneRemapTable(256, 0);
                for (uint8_t b = 0; b < skinPalette.size(); b++)
                    boneRemapTable[skinPalette[b]] = b;
                boneRemapWriter.Put(boneRemapTable.data(), boneRemapTable.size());
                boneRemapWriter.Align();
                vector<uint8_t> skinPaletteTable(256, 0);
                for (uint8_t b = 0; b < skinPalette.size(); b++)
                    skinPaletteTable[b] = skinPalette[b];
                boneRemapWriter.Put(skinPaletteTable.data(), skinPaletteTable.size());
                boneRemapWriter.AlignAndUpdateTotalSize();
            }
        }
    }

    // ibbatch
    Rx3Writer ibBatchWriter(rx3.AddChunk(RX3_CHUNK_INDEX_BUFFER_BATCH));
    ibBatchWriter.Put<uint32_t>(ibs.size());
    ibBatchWriter.Align();
    for (auto const &ib : ibs)
        ibBatchWriter.Put(ib.data(), 16);
    // quadibbatch
    if (!qibs.empty()) {
        Rx3Writer qibBatchWriter(rx3.AddChunk(RX3_CHUNK_QUAD_INDEX_BUFFER_BATCH));
        qibBatchWriter.Put<uint32_t>(qibs.size());
        qibBatchWriter.Align();
        for (auto const &qib : qibs)
            qibBatchWriter.Put(qib.data(), 16);
    }
    // vertexformat's
    for (auto const &vf : vertexFormats) {
        Rx3Writer vertexFormatWriter(rx3.AddChunk(RX3_CHUNK_VERTEX_FORMAT));
        vertexFormatWriter.Put<uint32_t>(0);
        vertexFormatWriter.Put<uint32_t>(vf.size() + 1);
        vertexFormatWriter.Align();
        vertexFormatWriter.Put(vf);
        vertexFormatWriter.AlignAndUpdateTotalSize();
    }
    // nametable
    AddNamesChunkToRx3(rx3, nametable);
    // ib's
    for (auto const &ib : ibs) {
        Rx3Writer ibWriter(rx3.AddChunk(RX3_CHUNK_INDEX_BUFFER));
        ibWriter.Put(ib.data(), ib.size());
    }
    // qib's
    for (auto const &qib : qibs) {
        Rx3Writer qibWriter(rx3.AddChunk(RX3_CHUNK_QUAD_INDEX_BUFFER));
        qibWriter.Put(qib.data(), qib.size());
    }
    // boneremap's
    for (auto const &boneremap : boneremaps) {
        Rx3Writer boneremapWriter(rx3.AddChunk(RX3_CHUNK_BONE_REMAP));
        boneremapWriter.Put(boneremap.data(), boneremap.size());
    }
    // vb's
    for (auto const &vb : vbs) {
        Rx3Writer vbWriter(rx3.AddChunk(RX3_CHUNK_VERTEX_BUFFER));
        vbWriter.Put(vb.data(), vb.size());
    }
    // animationskin's
    if (!ibms.empty()) {
        for (auto const &ib : ibs) {
            Rx3Writer animationSkinWriter(rx3.AddChunk(RX3_CHUNK_ANIMATION_SKIN));
            animationSkinWriter.Put<uint32_t>(0);
            animationSkinWriter.Put<uint32_t>(ibms.size());
            animationSkinWriter.Align();
            for (auto const &ibm : ibms)
                WriteMatrix4x4(animationSkinWriter, ibm);
            animationSkinWriter.AlignAndUpdateTotalSize();
        }
    }
    // simplemesh's
    for (auto pt : primTypes) {
        Rx3Writer meshWriter(rx3.AddChunk(RX3_CHUNK_SIMPLE_MESH));
        meshWriter.Put<uint16_t>(pt);
        meshWriter.Align();
    }
    // adjacency's
    for (auto const &adjacency : adjacencies) {
        Rx3Writer adjacencyWriter(rx3.AddChunk(RX3_CHUNK_ADJACENCY));
        adjacencyWriter.Put(adjacency.data(), adjacency.size());
    }
}

void ModelToSimpleMeshContainer(Model const &source, path const &sourcePath, path const &rx3path, Rx3Options const &options) {
    Rx3Container rx3(options.gameConfig.BigEndian);
    ModelToSimpleMeshContainer(source, rx3, options);
    if (options.metadata)
        AddMetadataToRx3(rx3, sourcePath, rx3path, options);
    rx3.Save(rx3path);
}

void ExtractModelFromRX3(Rx3Container &container, path const &outputDir, Rx3Options const &rx3options) {
    Model m = ModelFromRX3(container, rx3options);
    if (!exists(outputDir))
        create_directories(outputDir);
    bool fbx = m.IsSkeleton() || m.HasShapeKeys() || rx3options.modelFormat != "obj";
    ModelWriteOptions options;
    options.AlwaysTriangulate = false;
    options.FbxAscii = rx3options.modelFormat == "fbxascii";
    m.Write(outputDir / (container.mName + (fbx ? ".fbx" : ".obj")), options);
}

Model ReadModelFromRX3(path const &rx3path, Rx3Options rx3options) {
    Rx3Container rx3(rx3path);
    return ModelFromRX3(rx3, rx3options);
}
