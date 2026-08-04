#include "Rx3Model.h"
#include "Rx3Container.h"
#include "Rx3Names.h"
#include "Rx3Scene.h"
#include "Rx3Morph.h"
#include "Rx3Skeleton.h"
#include "Rx3VertexFormat.h"
#include "ModelOperations/ModelTristrip.h"
#include "ModelOperations/ModelSkinning.h"

using namespace rx3utils;
using namespace rx3::vertex_format;

namespace helper::rx3model {

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
    options.ApplyTransforms = false;
    options.MergeMeshes = true;
    options.FbxTangents = ModelReadOptions::FBX_TANGENTS_GENERATE;
    model.Read(filePath, options);
    return model;
}

void Rx3MeshToObject(Object &obj, Rx3Chunk *vfChunk, Rx3Chunk *vbChunk, Rx3Chunk *ibChunk, Rx3Chunk *qibChunk, int primType,
    unsigned int numBones, Rx3Options const &options)
{
    using namespace helper::rx3model;
    obj.vertexFormat = 0;
    Rx3Reader vertexDeclReader(vfChunk);
    Rx3Reader vertexBufferReader(vbChunk);
    vertexDeclReader.Skip(4);
    uint32_t declStrLen = vertexDeclReader.Read<uint32_t>();
    if (declStrLen == 0)
        return;
    vertexDeclReader.Skip(8);
    string decl = vertexDeclReader.GetString();
    Rx3VertexFormat vf;
    vf.FromString(decl);
    if (vf.elements.empty())
        return;
    vertexBufferReader.Skip(4);
    uint32_t numVertices = vertexBufferReader.Read<uint32_t>();
    uint32_t vs = vertexBufferReader.Read<uint32_t>();
    uint8_t vbType = vertexBufferReader.Read<uint8_t>();
    vertexBufferReader.Skip(3);
    uint8_t *vb = (uint8_t *)vertexBufferReader.GetCurrentPtr();
    if (vbType == 0)
        VBEndianSwap(vf, numVertices, vb);
    obj.vertices.resize(numVertices);
    for (auto const &element : vf.elements) {
        switch (element.usage) {
        case 'p':
            if (element.usageIndex == 0) {
                for (uint32_t v = 0; v < numVertices; v++)
                    obj.vertices[v].pos = UnpackVector3(element.dataType, vb + v * vs + element.offset) / 100.0f;
            }
            break;
        case 'n':
            if (element.usageIndex == 0) {
                obj.vertexFormat |= V_Normal;
                for (uint32_t v = 0; v < numVertices; v++)
                    obj.vertices[v].normal = UnpackVector3(element.dataType, vb + v * vs + element.offset);
            }
            break;
        case 'g':
            if (element.usageIndex == 0) {
                obj.vertexFormat |= V_Tangent;
                for (uint32_t v = 0; v < numVertices; v++)
                    obj.vertices[v].tangent = UnpackVector3(element.dataType, vb + v * vs + element.offset);
            }
            break;
        case 'b':
            if (element.usageIndex == 0) {
                obj.vertexFormat |= V_Binormal;
                for (uint32_t v = 0; v < numVertices; v++)
                    obj.vertices[v].binormal = UnpackVector3(element.dataType, vb + v * vs + element.offset);
            }
            break;
        case 't':
            if (element.usageIndex <= 7) {
                SetNumTexCoords(obj.vertexFormat, element.usageIndex + 1);
                for (uint32_t v = 0; v < numVertices; v++) {
                    obj.vertices[v].uv[element.usageIndex] = UnpackVector2(element.dataType, vb + v * vs + element.offset);
                    obj.vertices[v].uv[element.usageIndex].y = 1.0f - obj.vertices[v].uv[element.usageIndex].y;
                }
            }
            break;
        case 'c':
            if (element.usageIndex <= 7) {
                SetNumColors(obj.vertexFormat, element.usageIndex + 1);
                for (uint32_t v = 0; v < numVertices; v++)
                    obj.vertices[v].colors[element.usageIndex] = UnpackColor(element.dataType, vb + v * vs + element.offset);
            }
        case 'i':
            if (element.usageIndex <= 1) {
                SetNumBones(obj.vertexFormat, (element.usageIndex + 1) * 4);
                auto dataType = element.dataType;
                if (dataType == dt_4u8 && numBones > 255)
                    dataType = dt_4u16;
                for (uint32_t v = 0; v < numVertices; v++) {
                    array<float, 4> joints = UnpackVertexAttribute(dataType, vb + v * vs + element.offset);
                    for (uint32_t bi = 0; bi < 4; bi++)
                        obj.vertices[v].boneIndices[element.usageIndex * 4 + bi] = (uint16_t)joints[bi];
                }
            }
            break;
        case 'w':
            if (element.usageIndex <= 1) {
                for (uint32_t v = 0; v < numVertices; v++) {
                    array<float, 4> weights = UnpackVertexAttribute(element.dataType, vb + v * vs + element.offset);
                    for (uint32_t bi = 0; bi < 4; bi++)
                        obj.vertices[v].boneWeights[element.usageIndex * 4 + bi] = weights[bi];
                }
            }
            break;
        }
    }
    if (NumBones(obj.vertexFormat) > 0) {
        size_t maxBonesPerVertex = 0;
        for (auto &v : obj.vertices) {
            auto bones = ModelSkinning::GetVertexBones(v, NumBones(obj.vertexFormat));
            ModelSkinning::SetVertexBones(v, bones, false);
            maxBonesPerVertex = max(bones.size(), maxBonesPerVertex);
        }
        SetNumBones(obj.vertexFormat, (uint8_t)maxBonesPerVertex);
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
        Rx3MeshToObject(obj, vertexFormats[i], vbs[i], ibs[i], qb, primType, model.skeleton.bones.size(), options);
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
            auto bonesSrc = ModelSkinning::GetVertexBones(o.vertices[v], NumBones(o.vertexFormat));
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
            ModelSkinning::SetVertexBones(o.vertices[v], bonesDst, true);
            newBonesPerVertex = max(bonesDst.size(), newBonesPerVertex);
        }
        SetNumBones(o.vertexFormat, (uint8_t)newBonesPerVertex);
    }
    model.skeleton = targetSkeleton;
    return true;
}

void RenameObjects(Model &model) {
    for (auto &o : model.objects) {
        if (o.name.starts_with("gkglove_mat") && o.name.find("accessory") == string::npos)
            o.name = "accessory_" + o.name;
    }
}

void ModelToSimpleMeshContainer(Model const &source, Rx3Container &rx3, Rx3Options const &options) {
    using namespace helper::rx3model;
    Model model = source;
    model.MergeMeshes();
    model.ApplyTransforms();
    RenameObjects(model);
    bool remappedBones = false;
    bool hasSkeleton = !model.skeleton.bones.empty() && !options.targetSkeleton.bones.empty();
    if (hasSkeleton) {
        if (!options.poseChangeMatrices.empty())
            ModelSkinning::ChangePose(model, options.poseChangeMatrices);
        if (!options.boneRemap.empty())
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
    uint8_t vbType = options.forceBigEndian ? 0 : 1;
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
        if (options.boneMatricesOption == BONE_MATRICES_FROM_FBX_FILE && !remappedBones) { // use skeleton from FBX
            ibms = ComputeBoneInverseBindMatricesForModel(model, options.targetSkeleton, true);
            adjustMatrices = true;
        }
        else if (options.boneMatricesOption == BONE_MATRICES_FROM_SOURCE_RX3 && !remappedBones) { // use skeleton from source RX3
            ibms = ComputeBoneInverseBindMatricesForModel(model, options.targetSkeleton, false);
            adjustMatrices = true;
        }
        else if (options.boneMatricesOption == BONE_MATRICES_FROM_BASE_MODEL) {
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
                    auto bones = ModelSkinning::GetVertexBones(o.vertices[v], NumBones(o.vertexFormat));
                    if (bones.empty())
                        packedBones.push_back(PackedBoneInfo(0, 255));
                    else if (bones.size() == 1)
                        packedBones.push_back(PackedBoneInfo(bones[0].first, 255));
                    else
                        packedBones = GetPackedBones(bones);
                    if (packedBones.empty())
                        packedBones.push_back(PackedBoneInfo(1, 255));
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

    for (size_t oi = 0; oi < model.objects.size(); oi++) {
        auto const &o = model.objects[oi];
        if (IsObjectWriteable(o)) {
            auto mesh = o.firstMesh();
            uint8_t indexSize = o.vertices.size() > 0xFFFF ? 4 : 2;
            nametable.emplace_back(RX3_CHUNK_SIMPLE_MESH, o.name + ".FxRenderableSimple");
            Rx3VertexFormat vf;
            vf.AddElement('p', 0, vf.Stride(), posDataType);
            if (o.vertexFormat & V_Normal)
                vf.AddElement('n', 0, vf.Stride(), dt_3s10n);
            if (o.vertexFormat & V_Tangent)
                vf.AddElement('g', 0, vf.Stride(), dt_3s10n);
            if (options.binormals && o.vertexFormat & V_Binormal)
                vf.AddElement('b', 0, vf.Stride(), dt_3s10n);
            for (uint8_t t = 0; t < NumTexCoords(o.vertexFormat); t++)
                vf.AddElement('t', t, vf.Stride(), dt_2f16);
            if (numBoneSets > 0) {
                for (uint8_t set = 0; set < numBoneSets; set++)
                    vf.AddElement('i', set, vf.Stride(), bonesDataType);
                for (uint8_t set = 0; set < numBoneSets; set++)
                    vf.AddElement('w', set, vf.Stride(), dt_4u8n);
            }
            vertexFormats.push_back(vf.ToString());
            vector<uint8_t> skinPalette;
            vector<uint8_t> vertexBuffer(o.vertices.size() * vf.Stride());
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
            if (vbType == 0)
                VBEndianSwap(vf, o.vertices.size(), vertexBuffer.data());
            // vb
            Rx3Writer vbWriter(vbs.emplace_back(), rx3.mBigEndian);
            vbWriter.Put<uint32_t>(0);
            vbWriter.Put<uint32_t>(o.vertices.size());
            vbWriter.Put<uint32_t>(vf.Stride());
            vbWriter.Put<uint8_t>(vbType);
            vbWriter.Put<uint8_t>(0);
            vbWriter.Put<uint8_t>(0);
            vbWriter.Put<uint8_t>(0);
            vbWriter.Align();
            vbWriter.Put(vertexBuffer.data(), vertexBuffer.size());
            vbWriter.AlignAndUpdateTotalSize();
            if (hasQuads) {
                // qib
                mesh.LeaveTrisAndQuads(o.vertices);
                Rx3Writer qibWriter(qibs.emplace_back());
                qibWriter.Put<uint32_t>(0);
                qibWriter.Put<uint32_t>(mesh.polygons.size() * 4);
                qibWriter.Put<uint8_t>(indexSize);
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
                Rx3Writer adjacencyWriter(adjacencies.emplace_back(), rx3.mBigEndian);
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
            Rx3Writer ibWriter(ibs.emplace_back(), rx3.mBigEndian);
            ibWriter.Put<uint32_t>(0);
            vector<uint16_t> tristrips;
            if (options.tristrip && o.vertices.size() < 0xFFFF)
                tristrips = ModelTristrip::GenerateTristrips(mesh.polygons);
            if (!tristrips.empty()) {
                ibWriter.Put<uint32_t>(tristrips.size());
                ibWriter.Put<uint8_t>(indexSize);
                ibWriter.Align();
                for (uint16_t index : tristrips)
                    ibWriter.Put<uint16_t>(index);
                primTypes.push_back(RX3_PRIM_TRIANGLESTRIP);
            }
            else {
                ibWriter.Put<uint32_t>(mesh.polygons.size() * 3);
                ibWriter.Put<uint8_t>(indexSize);
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
                Rx3Writer boneRemapWriter(boneremaps.emplace_back(), rx3.mBigEndian);
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
    Rx3Container rx3(options.gameConfig.BigEndian || options.forceBigEndian);
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
