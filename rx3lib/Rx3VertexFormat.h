#pragma once
#include "Rx3Utils.h"
#include "Rx3Container.h"
#include "ModelTypes.h"

namespace rx3::vertex_format {

enum DataType {
    dt_unknown, dt_void, dt_1f32, dt_1s32, dt_1s16, dt_1s8, dt_2f32, dt_2s32, dt_2s16, dt_2s8, dt_3f32, dt_3s32,
    dt_3s16, dt_3s8, dt_4f32, dt_4s32, dt_4s16, dt_4s8, dt_4u8, dt_4u8n, dt_4u8endianswapp, dt_4u8nendianswap,
    dt_2s16n, dt_4s16n, dt_3u10, dt_3s10n, dt_3s11n, dt_2f16, dt_4f16, dt_2s16s, dt_3s16s, dt_1u16rgb565,
    dt_3u8rgb8, dt_4u8rgbx8, dt_1u16rgba4, dt_3u8rgba6, dt_4u8rgba8, dt_2u16, dt_4u16, dt_2u16n, dt_4u16n, dt_custom
};

struct PackedBoneInfo {
    uint16_t bone = 0;
    uint8_t weightPacked = 0;

    PackedBoneInfo();
    PackedBoneInfo(uint16_t _bone, uint8_t _weightPacked = 0);
};

DataType DataTypeIdFromName(string const &name);
int32_t DecodeSigned10(uint32_t bits);
uint32_t EncodeSigned10(float value);
float UnpackFloatFrom10Bit(int value);
float UnpackFloatFrom11Bit(int value);
array<float, 4> UnpackVertexAttribute(DataType dt, const unsigned char *data);
Vector2 UnpackVector2(DataType dt, const unsigned char *data);
Vector3 UnpackVector3(DataType dt, const unsigned char *data);
RGBA UnpackColor(DataType dt, const unsigned char *data);
uint32_t PackVector3(DataType dt, unsigned char *data, Vector3 const &vec);
uint32_t PackVector2(DataType dt, unsigned char *data, Vector2 const &vec);
vector<PackedBoneInfo> GetPackedBones(vector<pair<uint16_t, float>> const &bones);
uint32_t WriteBoneIndices(DataType dt, uint8_t *data, vector<PackedBoneInfo> const &bones, uint8_t numBoneSets, uint8_t numBonesToPad);
uint32_t WriteBoneWeights(DataType dt, uint8_t *data, vector<PackedBoneInfo> const &bones, uint8_t numBoneSets);

}

struct Rx3VertexDeclElement {
    rx3::vertex_format::DataType dataType = rx3::vertex_format::DataType::dt_unknown;
    uint32_t offset = 0;
    char usage = 0;
    uint8_t usageIndex = 0;

    Rx3VertexDeclElement(char _usage, uint8_t _usageIndex, uint32_t _offset, rx3::vertex_format::DataType _dataType);
};

struct Rx3VertexFormat {
    vector<Rx3VertexDeclElement> elements;
    uint32_t stride = 0;
public:
    void AddElement(Rx3VertexDeclElement const &element);
    void AddElement(char usage, uint8_t usageIndex, uint32_t offset, rx3::vertex_format::DataType dataType);
    string ToString() const;
    uint32_t Stride() const;
    void FromString(string const &str);
};

void VBEndianSwap(Rx3VertexFormat const &vertexFormat, size_t count, uint8_t *data);
