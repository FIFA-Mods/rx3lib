#include "Rx3Skeleton.h"
#include "Rx3Container.h"
#include "Rx3Names.h"
#include "Rx3Model.h"

Matrix4x4 ComputeBoneWorldMatrix(Model const &model, int boneIndex, vector<Matrix4x4> &worldCache, vector<char> &computed) {
    if (computed[boneIndex])
        return worldCache[boneIndex];
    auto const &bone = model.skeleton.bones[boneIndex];
    Matrix4x4 world = bone.matrix;
    if (!bone.parent.empty()) {
        int parentIndex = model.GetBoneIndex(bone.parent);
        if (parentIndex >= 0)
            world = ComputeBoneWorldMatrix(model, parentIndex, worldCache, computed) * bone.matrix;
    }
    worldCache[boneIndex] = world;
    computed[boneIndex] = 1;
    return world;
}

vector<Matrix4x4> GetSourceBoneInverseBindMatrices(Skeleton const &skeleton) {
    vector<Matrix4x4> matrices(skeleton.bones.size());
    for (size_t i = 0; i < skeleton.bones.size(); i++) {
        if (skeleton.bones[i].properties.contains("ibm"))
            matrices[i] = std::get<Matrix4x4>(skeleton.bones[i].properties.at("ibm"));
    }
    return matrices;
}

vector<Matrix4x4> ComputeBoneInverseBindMatricesForModel(Model const &model, Skeleton const &baseSkeleton, bool ignoreSourceIbm) {
    vector<Matrix4x4> matrices = GetSourceBoneInverseBindMatrices(baseSkeleton);
    vector<Matrix4x4> worldCache(model.skeleton.bones.size());
    vector<char> computed(model.skeleton.bones.size(), 0);
    for (size_t i = 0; i < baseSkeleton.bones.size(); i++) {
        int boneIndex = model.GetBoneIndex(baseSkeleton.bones[i].name);
        if (boneIndex >= 0) {
            Matrix4x4 world = ComputeBoneWorldMatrix(model, boneIndex, worldCache, computed);
            matrices[i] = world.Inversed();
            for (uint32_t j = 0; j < 3; j++)
                matrices[i].m[3][j] *= 100.0f;
            // rewrite the matrix by source matrix, if present
            if (!ignoreSourceIbm && model.skeleton.bones[boneIndex].properties.contains("ibm"))
                matrices[i] = std::get<Matrix4x4>(model.skeleton.bones[boneIndex].properties.at("ibm"));
        }
    }
    return matrices;
}

void ModelToSkeletonContainer(Model const &model, path const &sourcePath, path const &rx3path, Rx3Options const &options) {
    using namespace helper::rx3model;
    Rx3Container rx3(options.gameConfig.BigEndian);
    // aimationskin
    Rx3Writer animationSkinWriter(rx3.AddChunk(RX3_CHUNK_ANIMATION_SKIN));
    animationSkinWriter.Put<uint32_t>(0);
    animationSkinWriter.Put<uint32_t>(options.targetSkeleton.bones.size());
    animationSkinWriter.Align();
    auto matrices = ComputeBoneInverseBindMatricesForModel(model, options.targetSkeleton, model.IsSkeleton());
    for (auto const &matrix : matrices)
        WriteMatrix4x4(animationSkinWriter, matrix);
    // skeleton
    Rx3Writer skeletonWriter(rx3.AddChunk(RX3_CHUNK_SKELETON));
    skeletonWriter.Put<uint32_t>(0);
    skeletonWriter.Put<uint32_t>(options.targetSkeleton.bones.size());
    skeletonWriter.Align();
    for (auto const &bone : options.targetSkeleton.bones)
        skeletonWriter.Put<int16_t>(bone.parent.empty() ? -1 : options.targetSkeleton.GetBoneIndex(bone.parent));
    skeletonWriter.AlignAndUpdateTotalSize();
    // nametable
    vector<string> boneNamesToWrite;
    for (auto const &bone : options.targetSkeleton.bones)
        boneNamesToWrite.push_back(bone.name);
    AddNamesChunkToRx3(rx3, boneNamesToWrite, RX3_CHUNK_BONE_NAME);
    // metadata
    if (options.metadata)
        AddMetadataToRx3(rx3, sourcePath, rx3path, options);
    // done
    rx3.Save(rx3path);
}

Model ModelFromSkeletonContainer(Rx3Container &rx3, Rx3Options const &options) {
    using namespace helper::rx3model;
    Model model;
    auto animationSkinChunk = rx3.FindFirstChunk(RX3_CHUNK_ANIMATION_SKIN);
    auto skeletonChunk = rx3.FindFirstChunk(RX3_CHUNK_SKELETON);
    if (animationSkinChunk && skeletonChunk) {
        vector<string> boneNames = ExtractNamesFromRx3(rx3, RX3_CHUNK_BONE_NAME);
        Rx3Reader animationSkinReader(animationSkinChunk);
        animationSkinReader.Skip(4);
        uint32_t numBones = animationSkinReader.Read<uint32_t>();
        animationSkinReader.Skip(8);
        if (numBones > 0) {
            auto &bones = model.skeleton.bones;
            bones.resize(numBones);
            for (uint32_t b = 0; b < numBones; b++)
                bones[b].name = (b < boneNames.size()) ? boneNames[b] : "bone_" + to_string(b);
            vector<Matrix4x4> boneInversedMatrices(numBones);
            for (uint32_t b = 0; b < numBones; b++) {
                ReadMatrix4x4(animationSkinReader, boneInversedMatrices[b]);
                bones[b].properties["ibm"] = boneInversedMatrices[b];
                for (uint32_t j = 0; j < 3; j++)
                    boneInversedMatrices[b].m[3][j] /= 100.0f;
                bones[b].matrix = boneInversedMatrices[b].Inversed();
            }
            Rx3Reader skeletonReader(skeletonChunk);
            skeletonReader.Skip(16);
            for (uint32_t b = 0; b < numBones; b++) {
                int16_t parentIndex = skeletonReader.Read<int16_t>();
                if (parentIndex >= 0 && parentIndex < (int32_t)bones.size()) {
                    bones[b].parent = bones[parentIndex].name;
                    bones[b].matrix = boneInversedMatrices[parentIndex] * bones[b].matrix;
                }
            }
        }
    }
    return model;
}
