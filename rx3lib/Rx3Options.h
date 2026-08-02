#pragma once
#include "rx3utils.h"
#include "Model.h"
#include "Rx3Container.h"

enum eFolderOption {
    FOLDER_OPTION_AUTO,
    FOLDER_OPTION_ALWAYS_CREATE,
    FOLDER_OPTION_NEVER_CREATE
};

enum eBoneMatricesOption {
    BONE_MATRICES_FROM_FILE,
    BONE_MATRICES_FROM_SKELETON,
    BONE_MATRICES_FROM_BASE_MODEL
};

struct TexFormatTarget {
    int format = -1;
    char levels = 0;
    unsigned short width = 0;
    unsigned short height = 0;
};

enum eSkinPaletteOpcodesPolicty {
    SKIN_PALETTE_OPCODES_ALWAYS,
    SKIN_PALETTE_OPCODES_16BIT_BONE_IDS
};

struct GameConfig {
    bool BigEndian = false;
    unsigned char MaxBonesPerVertex = 0;
    unsigned int MaxBonesPerMesh = 0;
    bool TextureRasterSuffix = false;
    bool QuadMeshes = false;
    bool StadiumTexturesAndModelInOneContainer = false;
    bool PadAllVertexBufferBoneIndices = false;
    map<unsigned char, unsigned char> TextureFormats;

    GameConfig();
    GameConfig(bool _BigEndian, unsigned char _MaxBonesPerVertex, unsigned int _MaxBonesPerMesh, bool _TextureRasterSuffix,
        bool _QuadMeshes, bool _StadiumTexturesAndModelInOneContainer, bool _PadAllVertexBufferBoneIndices,
        map<unsigned char, unsigned char> const &_TextureFormats);
};

map<string, GameConfig> &GameConfigs();

struct Rx3Options {
    string tools;
    string toolsVersion;
    string cmdLine;
    string game;
    string textureFormat;
    string modelFormat;
    bool exportQuads;
    bool writeHDR;
    bool writeTexMetadata;
    bool tristrip;
    bool precisePositions;
    bool binormals;
    bool metadata;
    eFolderOption folderOption;
    eBoneMatricesOption boneMatricesOption;
    GameConfig gameConfig;
    map<string, TexFormatTarget> texTargetFormats;
    Model baseModel;
    Skeleton targetSkeleton;
    map<string, string> boneRemap;
    float scale;
    Vector3 movement;
    unordered_map<string, Matrix4x4> poseChangeMatrices;

    Rx3Options();
    Rx3Options(string const &gameName);
    Vector3 AdjustPosition(Vector3 const &pos) const;
};

void AddMetadataToRx3(Rx3Container &rx3, path const &in, path const &out, Rx3Options const &options);
