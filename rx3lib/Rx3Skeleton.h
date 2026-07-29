#pragma once
#include "Model.h"
#include "Rx3Container.h"
#include "Rx3Options.h"

Model ModelFromSkeletonContainer(Rx3Container &rx3, Rx3Options const &options);
void ModelToSkeletonContainer(Model const &model, path const &sourcePath, path const &rx3path, Rx3Options const &options);
vector<Matrix4x4> GetSourceBoneInverseBindMatrices(Skeleton const &skeleton);
vector<Matrix4x4> ComputeBoneInverseBindMatricesForModel(Model const &model, Skeleton const &baseSkeleton);
