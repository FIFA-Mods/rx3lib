#include "Rx3Morph.h"
#include "Rx3Model.h"
#include "Rx3Names.h"

using namespace rx3utils;

Model ModelFromMorphTargetsContainer(Rx3Container &rx3, Rx3Options const &options) {
    using namespace helper::rx3model;
    Model model = options.baseModel;
    if (model.objects.empty())
        return Model();
    auto morphChunks = rx3.FindAllChunks(RX3_CHUNK_MORPH_INDEXED);
    if (morphChunks.empty())
        return Model();
    for (auto &o : model.objects) {
        SetNumColors(o.vertexFormat, 1);
        for (size_t v = 0; v < o.vertices.size(); v++)
            SetAt(&o.vertices[v].colors[0], 0, v);
    }
    for (size_t i = 0; i < min(morphChunks.size(), model.objects.size()); i++) {
        Rx3Reader morphReader(morphChunks[i]);
        morphReader.Skip(8);
        uint32_t numDescriptors = morphReader.Read<uint32_t>();
        morphReader.Skip(8);
        if (numDescriptors > 0) {
            auto &shapeKeys = model.objects[i].shapeKeys;
            shapeKeys.resize(numDescriptors);
            for (size_t d = 0; d < numDescriptors; d++) {
                shapeKeys[d].name = "ShapeKey_" + to_string(d + 1);
                uint32_t numDeltaIndices = morphReader.Read<uint32_t>();
                if (numDeltaIndices > 0) {
                    shapeKeys[d].vertices.resize(numDeltaIndices);
                    for (size_t v = 0; v < numDeltaIndices; v++)
                        shapeKeys[d].vertices[v].vertexIndex = morphReader.Read<uint16_t>();
                    for (size_t v = 0; v < numDeltaIndices; v++)
                        shapeKeys[d].vertices[v].deltaPos = ReadVector3(morphReader) / 100.0f;
                }
            }
        }
    }
    return model;
}

void ModelToMorphTargetsContainer(Model const &model, path const &sourcePath, path const &rx3path, Rx3Options const &options) {
    using namespace helper::rx3model;
    if (options.baseModel.objects.empty())
        return;
    vector<pair<string, Object const *>> objects;
    for (auto const &baseModelObj : options.baseModel.objects) {
        if (!baseModelObj.vertices.empty())
            objects.emplace_back(baseModelObj.name, model.GetObjectByName(baseModelObj.name, true));
    }
    Rx3Container rx3(options.gameConfig.BigEndian);
    // nametable, morphindexed's
    vector<string> shapeKeyNames;
    for (auto const &[name, obj] : objects)
        shapeKeyNames.push_back("geo_" + name + "Shape.IndexedMorphDescriptor.");
    AddNamesChunkToRx3(rx3, shapeKeyNames, 0);
    for (auto const &[name, obj] : objects) {
        Rx3Writer morphWriter(rx3.AddChunk(RX3_CHUNK_MORPH_INDEXED));
        morphWriter.Put<uint32_t>(0);
        morphWriter.Put<uint8_t>(1);
        morphWriter.Align(4);
        morphWriter.Put<uint32_t>(obj ? obj->shapeKeys.size() : 0);
        morphWriter.Put<uint32_t>(0);
        morphWriter.Put<uint32_t>(0);
        if (obj) {
            for (size_t i = 0; i < obj->shapeKeys.size(); i++) {
                auto const &shapeKey = obj->shapeKeys[i];
                map<uint32_t, Vector3> deltas;
                for (auto const &v : shapeKey.vertices)
                    deltas[GetAt<uint32_t>(&obj->vertices[v.vertexIndex].colors[0], 0)] = v.deltaPos;
                morphWriter.Put<uint32_t>(deltas.size());
                for (auto const &[vertexIndex, deltaPos] : deltas)
                    morphWriter.Put<uint16_t>(vertexIndex);
                for (auto const &[vertexIndex, deltaPos] : deltas)
                    WriteVector3(morphWriter, deltaPos * 100.0f);
            }
        }
        morphWriter.AlignAndUpdateTotalSize();
    }
    if (options.metadata)
        AddMetadataToRx3(rx3, sourcePath, rx3path, options);
    rx3.Save(rx3path);
}
