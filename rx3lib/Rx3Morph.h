#pragma once
#include "Rx3Container.h"
#include "Rx3Options.h"
#include "Model.h"

Model ModelFromMorphTargetsContainer(Rx3Container &rx3, Rx3Options const &options);
void ModelToMorphTargetsContainer(Model const &model, path const &sourcePath, path const &rx3path, Rx3Options const &options);
