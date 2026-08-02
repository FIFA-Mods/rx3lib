#include "Rx3Options.h"
#include "Rx3Container.h"

using namespace rx3utils;

map<string, GameConfig> &GameConfigs() {
    static map<string, GameConfig> configs = {
        { "fifa12pc", GameConfig(
            true,  // BigEndian
            4,     // MaxBonesPerVertex
            60,    // MaxBonesPerMesh
            true,  // TextureRasterSuffix
            false, // QuadMeshes
            true,  // StadiumTexturesAndModelInOneContainer
            false, // PadAllVertexBufferBoneIndices
            {
                { RX3_TEXFORMAT_DXT1, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_DXT3, RX3_TEXFORMAT_DXT3 },
                { RX3_TEXFORMAT_DXT5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_ARGB8888, RX3_TEXFORMAT_ARGB8888 },
                { RX3_TEXFORMAT_L8, RX3_TEXFORMAT_L8 },
                { RX3_TEXFORMAT_AL8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_RG8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_BC5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_RGB565, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_ARGB4444, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC6, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC7, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_BC4, RX3_TEXFORMAT_L8 }
            }
        )},
        { "fifa13pc", GameConfig(
            true,  // BigEndian
            4,     // MaxBonesPerVertex
            64,    // MaxBonesPerMesh
            true,  // TextureRasterSuffix
            false, // QuadMeshes
            true,  // StadiumTexturesAndModelInOneContainer
            false, // PadAllVertexBufferBoneIndices
            {
                { RX3_TEXFORMAT_DXT1, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_DXT3, RX3_TEXFORMAT_DXT3 },
                { RX3_TEXFORMAT_DXT5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_ARGB8888, RX3_TEXFORMAT_ARGB8888 },
                { RX3_TEXFORMAT_L8, RX3_TEXFORMAT_L8 },
                { RX3_TEXFORMAT_AL8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_RG8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_BC5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_RGB565, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_ARGB4444, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC6, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC7, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_BC4, RX3_TEXFORMAT_L8 }
            }
        )},
        { "fifa14pc", GameConfig(
            false, // BigEndian
            4,     // MaxBonesPerVertex
            64,    // MaxBonesPerMesh
            true,  // TextureRasterSuffix
            false, // QuadMeshes
            false, // StadiumTexturesAndModelInOneContainer
            false, // PadAllVertexBufferBoneIndices
            {
                { RX3_TEXFORMAT_DXT1, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_DXT3, RX3_TEXFORMAT_DXT3 },
                { RX3_TEXFORMAT_DXT5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_ARGB8888, RX3_TEXFORMAT_ARGB8888 },
                { RX3_TEXFORMAT_L8, RX3_TEXFORMAT_L8 },
                { RX3_TEXFORMAT_AL8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_RG8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_BC5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_RGB565, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_ARGB4444, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC6, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC7, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_BC4, RX3_TEXFORMAT_L8 }
            }
        )},
        { "fifa15pc", GameConfig(
            false, // BigEndian
            8,     // MaxBonesPerVertex
            UINT32_MAX, // MaxBonesPerMesh
            false, // TextureRasterSuffix
            false, // QuadMeshes
            false, // StadiumTexturesAndModelInOneContainer
            true,  // PadAllVertexBufferBoneIndices
            {
                { RX3_TEXFORMAT_DXT1, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_DXT3, RX3_TEXFORMAT_DXT3 },
                { RX3_TEXFORMAT_DXT5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_ARGB8888, RX3_TEXFORMAT_ARGB8888 },
                { RX3_TEXFORMAT_L8, RX3_TEXFORMAT_L8 },
                { RX3_TEXFORMAT_AL8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_RG8, RX3_TEXFORMAT_RG8 },
                { RX3_TEXFORMAT_BC5, RX3_TEXFORMAT_BC5 },
                { RX3_TEXFORMAT_RGB565, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_ARGB4444, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC6, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC7, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_BC4, RX3_TEXFORMAT_L8 }
            }
        )},
        { "fifa16pc", GameConfig(
            false, // BigEndian
            8,     // MaxBonesPerVertex
            UINT32_MAX, // MaxBonesPerMesh
            false, // TextureRasterSuffix
            true,  // QuadMeshes
            false, // StadiumTexturesAndModelInOneContainer
            true,  // PadAllVertexBufferBoneIndices
            {
                { RX3_TEXFORMAT_DXT1, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_DXT3, RX3_TEXFORMAT_DXT3 },
                { RX3_TEXFORMAT_DXT5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_ARGB8888, RX3_TEXFORMAT_ARGB8888 },
                { RX3_TEXFORMAT_L8, RX3_TEXFORMAT_L8 },
                { RX3_TEXFORMAT_AL8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_RG8, RX3_TEXFORMAT_RG8 },
                { RX3_TEXFORMAT_BC5, RX3_TEXFORMAT_BC5 },
                { RX3_TEXFORMAT_RGB565, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_ARGB4444, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC6, RX3_TEXFORMAT_BC6 },
                { RX3_TEXFORMAT_BC7, RX3_TEXFORMAT_BC7 },
                { RX3_TEXFORMAT_BC4, RX3_TEXFORMAT_L8 }
            }
        )},
        { "fifa23switch", GameConfig(
            false, // BigEndian
            4,     // MaxBonesPerVertex
            64,    // MaxBonesPerMesh
            false, // TextureRasterSuffix
            false, // QuadMeshes
            false, // StadiumTexturesAndModelInOneContainer
            true,  // PadAllVertexBufferBoneIndices
            {
                { RX3_TEXFORMAT_DXT1, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_DXT3, RX3_TEXFORMAT_DXT3 },
                { RX3_TEXFORMAT_DXT5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_ARGB8888, RX3_TEXFORMAT_ARGB8888 },
                { RX3_TEXFORMAT_L8, RX3_TEXFORMAT_L8 },
                { RX3_TEXFORMAT_AL8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_RG8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_BC5, RX3_TEXFORMAT_BC5 },
                { RX3_TEXFORMAT_RGB565, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_ARGB4444, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC6, RX3_TEXFORMAT_BC6 },
                { RX3_TEXFORMAT_BC7, RX3_TEXFORMAT_BC7 },
                { RX3_TEXFORMAT_BC4, RX3_TEXFORMAT_BC4 }
            }
        )},
        { "fo4", GameConfig(
            false, // BigEndian
            8,     // MaxBonesPerVertex
            UINT32_MAX, // MaxBonesPerMesh
            false, // TextureRasterSuffix
            true,  // QuadMeshes
            false, // StadiumTexturesAndModelInOneContainer
            true,  // PadAllVertexBufferBoneIndices
            {
                { RX3_TEXFORMAT_DXT1, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_DXT3, RX3_TEXFORMAT_DXT3 },
                { RX3_TEXFORMAT_DXT5, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_ARGB8888, RX3_TEXFORMAT_ARGB8888 },
                { RX3_TEXFORMAT_L8, RX3_TEXFORMAT_L8 },
                { RX3_TEXFORMAT_AL8, RX3_TEXFORMAT_AL8 },
                { RX3_TEXFORMAT_RG8, RX3_TEXFORMAT_RG8 },
                { RX3_TEXFORMAT_BC5, RX3_TEXFORMAT_BC5 },
                { RX3_TEXFORMAT_RGB565, RX3_TEXFORMAT_DXT1 },
                { RX3_TEXFORMAT_ARGB4444, RX3_TEXFORMAT_DXT5 },
                { RX3_TEXFORMAT_BC6, RX3_TEXFORMAT_BC6 },
                { RX3_TEXFORMAT_BC7, RX3_TEXFORMAT_BC7 },
                { RX3_TEXFORMAT_BC4, RX3_TEXFORMAT_L8 }
            }
        )}
    };
    return configs;
}

GameConfig::GameConfig() {}

GameConfig::GameConfig(bool _BigEndian, unsigned char _MaxBonesPerVertex, unsigned int _MaxBonesPerMesh, bool _TextureRasterSuffix,
    bool _QuadMeshes, bool _StadiumTexturesAndModelInOneContainer, bool _PadAllVertexBufferBoneIndices,
    map<unsigned char, unsigned char> const &_TextureFormats)
{
    BigEndian = _BigEndian;
    MaxBonesPerVertex = _MaxBonesPerVertex;
    MaxBonesPerMesh = _MaxBonesPerMesh;
    TextureRasterSuffix = _TextureRasterSuffix;
    QuadMeshes = _QuadMeshes;
    StadiumTexturesAndModelInOneContainer = _StadiumTexturesAndModelInOneContainer;
    PadAllVertexBufferBoneIndices = _PadAllVertexBufferBoneIndices;
    TextureFormats = _TextureFormats;
}

Rx3Options::Rx3Options() : Rx3Options("fifa16pc") {}

Rx3Options::Rx3Options(string const &gameName) {
    game = gameName;
    textureFormat = "png";
    modelFormat = "fbx";
    exportQuads = false;
    writeHDR = true;
    writeTexMetadata = true;
    metadata = true;
    folderOption = FOLDER_OPTION_AUTO;
    gameConfig = GameConfigs()[gameName];
    tristrip = false;
    precisePositions = true;
    binormals = false;
    boneMatricesOption = BONE_MATRICES_FROM_FILE;
    scale = 1.0f;
    movement = { 0.0f, 0.0f, 0.0f };
}

Vector3 Rx3Options::AdjustPosition(Vector3 const &pos) const {
    Vector3 newPos = pos;
    if (scale != 1.0f)
        newPos *= scale;
    if (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f)
        newPos += movement;
    return newPos;
}

void AddMetadataToRx3(Rx3Container &rx3, path const &in, path const &out, Rx3Options const &options) {
    if (rx3.FindFirstChunk(RX3_CHUNK_METADATA))
        return;
    string metadata;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream timess;
    timess << std::put_time(&tm, "%d-%b-%Y %H:%M:%S");
    metadata += "<Metadata>";
    if (!options.tools.empty())
        metadata += "<Tools>" + options.tools + "</Tools>";
    if (!options.toolsVersion.empty())
        metadata += "<ToolsVersion>" + options.toolsVersion + "</ToolsVersion>";
    if (!options.cmdLine.empty())
        metadata += "<ToolsCommand>" + options.cmdLine + "</ToolsCommand>";
    metadata += "<TimeStamp>" + timess.str() + "</TimeStamp>";
    if (!out.empty())
        metadata += "<OriginalFileName>" + ToUTF8(out.c_str()) + "</OriginalFileName>";
    if (!in.empty())
        metadata += "<SourceFile>" + ToUTF8(in.c_str()) + "</SourceFile>";
    metadata += "</Metadata>";
    Rx3Writer metadataWriter(rx3.AddChunk(RX3_CHUNK_METADATA));
    metadataWriter.Put<uint32_t>(metadata.size() + 1);
    metadataWriter.Align();
    metadataWriter.Put(metadata);
    metadataWriter.Align();
}
