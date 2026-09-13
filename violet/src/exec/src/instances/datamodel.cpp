#include "executor/instances/datamodel.h"
#include "game/offsets.hpp"

DataModel::DataModel(const Instance& instance) : Instance(instance) {}

bool DataModel::GameLoaded() const {
    if (!IsValid()) return false;

    return (ReadFrom<uint64_t>(offsets::DataModel::GameLoaded) & 0xFF) == 2;
}
