#pragma once
#include "rx3utils.h"

template<typename T> void Rx3SwapEndian(T &value) {}
void Rx3SwapEndian(uint32_t &value);
void Rx3SwapEndian(uint16_t &value);
void Rx3SwapEndian(int32_t &value);
void Rx3SwapEndian(int16_t &value);
void Rx3SwapEndian(float &value);

constexpr uint32_t Rx3Hash(const char *str) {
    uint32_t result = 5321;
    while (*str)
        result = static_cast<unsigned char>(*str++) + result * 33;
    return result;
}

enum Rx3ChunkId : uint32_t {
    RX3_CHUNK_TEXTURE_BATCH = Rx3Hash("texturebatch"),
    RX3_CHUNK_TEXTURE = Rx3Hash("texture"),
    RX3_CHUNK_VERTEX_BUFFER = Rx3Hash("vb"),
    RX3_CHUNK_INDEX_BUFFER_BATCH = Rx3Hash("ibbatch"),
    RX3_CHUNK_INDEX_BUFFER = Rx3Hash("ib"),
    RX3_CHUNK_BONE_REMAP = Rx3Hash("boneremap"),
    RX3_CHUNK_ANIMATION_SKIN = Rx3Hash("animationskin"),
    RX3_CHUNK_EDGE_MESH = Rx3Hash("edgemesh"),
    RX3_CHUNK_SIMPLE_MESH = Rx3Hash("simplemesh"),
    RX3_CHUNK_VERTEX_FORMAT = Rx3Hash("vertexformat"),
    RX3_CHUNK_NAME_TABLE = Rx3Hash("nametable"),
    RX3_CHUNK_LOCATION = Rx3Hash("location"),
    RX3_CHUNK_HOTSPOT = Rx3Hash("hotspot"),
    RX3_CHUNK_MATERIAL = Rx3Hash("material"),
    RX3_CHUNK_COLLISION = Rx3Hash("collision"),
    RX3_CHUNK_SCENE_INSTANCE = Rx3Hash("sceneinstance"),
    RX3_CHUNK_SCENE_LAYER = Rx3Hash("scenelayer"),
    RX3_CHUNK_SCENE_ANIMATION = Rx3Hash("sceneanimation"),
    RX3_CHUNK_MORPH_INDEXED = Rx3Hash("morphindexed"),
    // since FIFA 14
    RX3_CHUNK_COLLISION_TRI_MESH = Rx3Hash("collisiontrimesh"),
    // since FIFA 15
    RX3_CHUNK_SKELETON = Rx3Hash("skeleton"),
    RX3_CHUNK_CLOTH_DEF = Rx3Hash("clothdef"),
    // since FIFA 16
    RX3_CHUNK_QUAD_INDEX_BUFFER_BATCH = Rx3Hash("quadibbatch"),
    RX3_CHUNK_QUAD_INDEX_BUFFER = Rx3Hash("qib"),
    RX3_CHUNK_BONE_NAME = Rx3Hash("bonename"),
    RX3_CHUNK_ADJACENCY = Rx3Hash("adjacency"),
    // custom
    RX3_CHUNK_METADATA = Rx3Hash("metadata"),
};

enum Rx3TextureBaseFormat : uint8_t {
    RX3_TEXFORMAT_DXT1 = 0,
    RX3_TEXFORMAT_DXT3 = 1,
    RX3_TEXFORMAT_DXT5 = 2,
    RX3_TEXFORMAT_ARGB8888 = 3,
    RX3_TEXFORMAT_L8 = 4,
    RX3_TEXFORMAT_AL8 = 5,
    RX3_TEXFORMAT_RG8 = 6,
    RX3_TEXFORMAT_BC5 = 7,
    RX3_TEXFORMAT_RGB565 = 8,
    RX3_TEXFORMAT_ARGB4444 = 9,
    RX3_TEXFORMAT_BC6 = 10, // BC6uf
    RX3_TEXFORMAT_BC7 = 11,
    RX3_TEXFORMAT_BC4 = 12,
    RX3_TEXFORMAT_NUM_FORMATS
};

enum Rx3TextureType : uint8_t {
    RX3_TEXTURE_1D = 0,
    RX3_TEXTURE_2D = 1,
    RX3_TEXTURE_3D = 2,
    RX3_TEXTURE_CUBE = 3, // texture consists of 6 faces
    RX3_TEXTURE_ARRAY = 4, // texture consists of multiple layers
    RX3_TEXTURE_RAW = 5
};

enum Rx3TextureDataFormat : uint8_t {
    RX3_TEXDATAFORMAT_LINEAR = 0x01,
    RX3_TEXDATAFORMAT_BIGENDIAN = 0x02,
    RX3_TEXDATAFORMAT_TILEDXENON = 0x04,
    RX3_TEXDATAFORMAT_SWIZZLEDPS3 = 0x08,
    RX3_TEXDATAFORMAT_REFPACKED = 0x80
};

enum Rx3PrimitiveType : uint32_t {
    RX3_PRIM_TRIANGLELIST = 4,
    RX3_PRIM_TRIANGLESTRIP = 6
};

class Rx3Chunk {
public:
    friend class Rx3Container;
    uint32_t mId;
    bool mBigEndian;
    vector<uint8_t> mData;
};

class Rx3Reader {
    uint8_t const *mBegin;
    uint8_t const *mCurrent;
    bool mBigEndian;
public:
    Rx3Reader(void const *data, bool bigEndian = false);
    Rx3Reader(Rx3Chunk *rx3chunk);
    Rx3Reader(Rx3Chunk const *rx3chunk);
    Rx3Reader(Rx3Chunk &rx3chunk);
    Rx3Reader(Rx3Chunk const &rx3chunk);
    size_t Position() const;
    void MoveTo(size_t position);
    void Skip(size_t bytes);
    char const *GetString();
    char const *ReadString();
    void const *GetCurrentPtr();
    void SetBigEndian(bool set);

    template<typename T>
    T const &Get() {
        static T result;
        result = *(T const *)mCurrent;
        if (mBigEndian)
            Rx3SwapEndian(result);
        return result;
    }

    template<typename T>
    T const &Read() {
        static T result;
        result = *(T const *)mCurrent;
        if (mBigEndian)
            Rx3SwapEndian(result);
        mCurrent += sizeof(T);
        return result;
    }
};

class Rx3Writer {
    vector<uint8_t> &mData;
    bool mBigEndian;

    void PutData(void const *data, size_t size);
public:
    Rx3Writer(vector<uint8_t> &data, bool bigEndian = false);
    Rx3Writer(Rx3Chunk *rx3chunk);
    Rx3Writer(Rx3Chunk const *rx3chunk);
    Rx3Writer(Rx3Chunk &rx3chunk);
    Rx3Writer(Rx3Chunk const &rx3chunk);
    void Put(string const &str);
    void Put(wstring const &str);
    void Put(const char *str);
    void Put(const wchar_t *str);
    void Put(void const *data, size_t size);
    void Align(size_t alignment = 16);
    void SetBigEndian(bool set);
    void Reserve(size_t size);
    // Writes current size at offset 0
    void UpdateTotalSize();
    // Aligns and writes current size at offset 0
    void AlignAndUpdateTotalSize();

    template<typename T>
    void Put(T const &value) {
        static T result;
        result = value;
        if (mBigEndian)
            Rx3SwapEndian(result);
        PutData((void *)&result, sizeof(T));
    }
};

class Rx3Container {
public:
    vector<Rx3Chunk> mChunks;
    string mName;
    path mPath;
    bool mBigEndian;

    Rx3Container(bool bigEndian = false);
    Rx3Container(path const &rx3path);
    bool Load(path const &rx3path);
    bool Save(path const &rx3path);
    Rx3Chunk *FindFirstChunk(uint32_t chunkId);
    vector<Rx3Chunk *> FindAllChunks(uint32_t chunkId);
    void RemoveAllChunks(uint32_t chunkId);
    Rx3Chunk &AddChunk(uint32_t chunkId);
    bool IsEmpty();
};
