#pragma once
#include "Model.h"
#include "Rx3Options.h"
#include "Rx3Container.h"
#include <filesystem>

namespace helper::rx3model {

Matrix4x4 ReadMatrix4x4(Rx3Reader &reader);
void ReadMatrix4x4(Rx3Reader &reader, Matrix4x4 &out);
void WriteMatrix4x4(Rx3Writer &writer, Matrix4x4 const &mat);
Vector3 ReadVector3(Rx3Reader &reader);
void ReadVector3(Rx3Reader &reader, Vector3 &out);
void WriteVector3(Rx3Writer &writer, Vector3 const &v);

}

Model ReadModelFromFile(path const &filePath);
Model ReadModelFromRX3(path const &rx3path, Rx3Options rx3options = Rx3Options());
void Rx3MeshToObject(Object &obj, Rx3Chunk *vfChunk, Rx3Chunk *vbChunk, Rx3Chunk *ibChunk, Rx3Chunk *qibChunk, int primType,
    unsigned int numBones, Rx3Options const &options);
void ExtractModelFromRX3(Rx3Container &container, path const &outputDir, Rx3Options const &rx3options);
Model ModelFromSimpleMeshContainer(Rx3Container &rx3, Rx3Options const &options);
void ModelToSimpleMeshContainer(Model const &model, Rx3Container &rx3, Rx3Options const &options);
void ModelToSimpleMeshContainer(Model const &model, path const &sourcePath, path const &rx3path, Rx3Options const &options);
