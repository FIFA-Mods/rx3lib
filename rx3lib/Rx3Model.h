#pragma once
#include "Model.h"
#include "Rx3Options.h"
#include "Rx3Container.h"
#include <filesystem>

namespace helper::rx3model {

enum DataType {
    dt_unknown, dt_void, dt_1f32, dt_1s32, dt_1s16, dt_1s8, dt_2f32, dt_2s32, dt_2s16, dt_2s8, dt_3f32, dt_3s32,
    dt_3s16, dt_3s8, dt_4f32, dt_4s32, dt_4s16, dt_4s8, dt_4u8, dt_4u8n, dt_4u8endianswapp, dt_4u8nendianswap,
    dt_2s16n, dt_4s16n, dt_3u10, dt_3s10n, dt_3s11n, dt_2f16, dt_4f16, dt_2s16s, dt_3s16s, dt_1u16rgb565,
    dt_3u8rgb8, dt_4u8rgbx8, dt_1u16rgba4, dt_3u8rgba6, dt_4u8rgba8, dt_2u16, dt_4u16, dt_2u16n, dt_4u16n, dt_custom
};

DataType DataTypeIdFromName(string const &name);

Matrix4x4 ReadMatrix4x4(Rx3Reader &reader);
void ReadMatrix4x4(Rx3Reader &reader, Matrix4x4 &out);
void WriteMatrix4x4(Rx3Writer &writer, Matrix4x4 const &mat);
Vector3 ReadVector3(Rx3Reader &reader);
void ReadVector3(Rx3Reader &reader, Vector3 &out);
void WriteVector3(Rx3Writer &writer, Vector3 const &v);

}

Model ReadModelFromFile(path const &filePath);
Model ReadModelFromRX3(path const &rx3path, Rx3Options rx3options = Rx3Options());
void SetupObjectMesh(Object &obj, Rx3Chunk *vfChunk, Rx3Chunk *vbChunk, Rx3Chunk *ibChunk, Rx3Chunk *qibChunk, int primType,
    unsigned int numBones, Rx3Options const &options);
void ExtractModelFromRX3(Rx3Container &container, path const &outputDir, Rx3Options const &rx3options);
Model ModelFromSimpleMeshContainer(Rx3Container &rx3, Rx3Options const &options);
void ModelToSimpleMeshContainer(Model const &model, path const &sourcePath, path const &rx3path, Rx3Options const &options);
