#include "Rx3Scene.h"
#include "Rx3Model.h"
#include "Rx3Names.h"

#pragma pack (push, 1)
struct CrowdHeader {
    uint32_t magic;
    uint16_t version;
    uint32_t numseats;
};

struct SeatBlob_0x0103 {
    Vector3 pos;
    float rotation;
    uint8_t seatcolor[3];
    uint8_t section;
    uint8_t tier;
    uint8_t attendance;
    uint8_t influencearea;
    uint8_t unused3;
    float shade[4];
    uint8_t unused_animgroups;
    uint8_t unused_numaccs;
};

struct SeatBlob_0x0104 {
    Vector3 pos;
    float rotation;
    uint8_t seatcolor[3];
    uint8_t section0;
    uint8_t section1;
    uint8_t unknown1;
    uint8_t unknown2;
    uint8_t tier;
    uint8_t attendance;
    float shade[4];
};

struct SeatBlob_0x0105 {
    Vector3 pos;
    float rotation;
    uint8_t seatcolor[3];
    uint8_t section0;
    uint8_t section1;
    uint8_t tier;
    uint8_t attendance;
    uint8_t nochair;
    uint8_t cardcolors[3];
    uint8_t crowdpattern;
    uint8_t pad[4];
};
#pragma pack (pop)

bool GetStadiumIdFromFilename(const string &s, int &outStadID, int &outLightingID) {
    auto IsNumeric = [](const string &str) {
        return !str.empty() && all_of(str.begin(), str.end(), ::isdigit);
    };
    const string prefix = "stadium_";
    if (s.size() <= prefix.size() || s.compare(0, prefix.size(), prefix) != 0)
        return false;
    size_t firstUnderscore = s.find('_', prefix.size());
    if (firstUnderscore == string::npos)
        return false;
    string stadPart = s.substr(prefix.size(), firstUnderscore - prefix.size());
    if (!IsNumeric(stadPart))
        return false;
    outStadID = stoi(stadPart);
    size_t startLight = firstUnderscore + 1;
    size_t secondUnderscore = s.find('_', startLight);
    string lightPart;
    if (secondUnderscore == string::npos)
        lightPart = s.substr(startLight);
    else
        lightPart = s.substr(startLight, secondUnderscore - startLight);
    if (!IsNumeric(lightPart))
        return false;
    outLightingID = stoi(lightPart);
    return true;
}

Model ModelFromSceneContainer(Rx3Container &rx3, Rx3Options const &options) {
    using namespace helper::rx3model;
    Model model;
    auto indexBufferChunks = rx3.FindAllChunks(RX3_CHUNK_INDEX_BUFFER);
    auto quadBufferChunks = rx3.FindAllChunks(RX3_CHUNK_QUAD_INDEX_BUFFER);
    auto vertexBufferChunks = rx3.FindAllChunks(RX3_CHUNK_VERTEX_BUFFER);
    auto vertexFormatChunks = rx3.FindAllChunks(RX3_CHUNK_VERTEX_FORMAT);
    auto meshChunks = rx3.FindAllChunks(RX3_CHUNK_SIMPLE_MESH);
    auto sceneInstanceChunks = rx3.FindAllChunks(RX3_CHUNK_SCENE_INSTANCE);
    auto collisionMeshChunks = rx3.FindAllChunks(RX3_CHUNK_COLLISION_TRI_MESH);
    auto sceneLayerChunks = rx3.FindAllChunks(RX3_CHUNK_SCENE_LAYER);
    auto materialSections = rx3.FindAllChunks(RX3_CHUNK_MATERIAL);
    auto locationChunks = rx3.FindAllChunks(RX3_CHUNK_LOCATION);
    path crowdPath;
    auto rx3path = rx3.mPath;
    if (rx3path.has_parent_path()) {
        auto parentDir = rx3path.parent_path();
        string filename = rx3path.stem().string();
        int stadiumId = -1, lightingId = -1;
        if (GetStadiumIdFromFilename(filename, stadiumId, lightingId) && stadiumId != -1 && lightingId != -1) {
            string crowdFilename = "crowd_" + to_string(stadiumId) + "_" + to_string(lightingId) + ".dat";
            if (exists(parentDir / crowdFilename))
                crowdPath = parentDir / crowdFilename;
            else if (parentDir.has_parent_path() && exists(parentDir.parent_path() / "crowdplacement" / crowdFilename))
                crowdPath = parentDir.parent_path() / "crowdplacement" / crowdFilename;
        }
    }
    if (!sceneLayerChunks.empty()) {
        vector<string> allNames, meshNames, locationNames, texNames;
        set<string> usedTextures;
        auto names = ExtractNamesFromRx3(rx3);
        for (auto const &[id, name] : names) {
            if (id == RX3_CHUNK_SIMPLE_MESH) {
                if (name.ends_with(".FxRenderableSimple"))
                    meshNames.push_back(name.substr(0, name.length() - strlen(".FxRenderableSimple")));
                else
                    meshNames.push_back(name);
            }
            else if (id == RX3_CHUNK_LOCATION)
                locationNames.push_back(name);
            else if (id == RX3_CHUNK_TEXTURE)
                texNames.push_back(name);
            allNames.push_back(name);
        }
        vector<uint32_t> primTypes;
        for (auto const &gt : meshChunks) {
            Rx3Reader gtReader(gt);
            primTypes.push_back(gtReader.Read<uint16_t>());
        }
        // materials
        if (!materialSections.empty()) {
            model.materials.resize(materialSections.size());
            for (size_t i = 0; i < materialSections.size(); i++) {
                Material &mat = model.materials[i];
                Rx3Reader materialReader(materialSections[i]);
                materialReader.Skip(4);
                auto numMatTextures = materialReader.Read<uint32_t>();
                materialReader.Skip(8);
                string shaderName = materialReader.ReadString();
                mat.name = "Material" + to_string(i + 1) + " [" + shaderName + "]";
                if (numMatTextures > 0) {
                    for (unsigned int t = 0; t < numMatTextures; t++) {
                        string texTypeName = materialReader.ReadString();
                        auto texId = materialReader.Read<int32_t>();
                        if (texId != -1) {
                            string texName;
                            if (texNames.empty())
                                texName = "texture_" + to_string(texId);
                            else
                                texName = texNames[texId];
                            if (texTypeName == "diffuseTexture")
                                mat.texture = texName;
                            else if (texTypeName == "normalMap")
                                mat.normalMap = texName;
                            else
                                mat.properties[texTypeName] = texName;
                            usedTextures.insert(texName);
                        }
                    }
                }
            }
        }
        // textures
        if (texNames.empty() && !usedTextures.empty()) {
            for (auto const &t : usedTextures)
                texNames.push_back(t);
        }
        if (!texNames.empty()) {
            model.textures.resize(texNames.size());
            for (size_t i = 0; i < texNames.size(); i++) {
                model.textures[i].name = texNames[i];
                model.textures[i].filename = texNames[i] + ".png";
            }
        }
        model.objects.resize(sceneLayerChunks.size());
        for (size_t layerIdx = 0; layerIdx < sceneLayerChunks.size(); layerIdx++) {
            Rx3Reader sceneLayerReader(sceneLayerChunks[layerIdx]);
            sceneLayerReader.Skip(4);
            auto layerType = sceneLayerReader.Read<uint32_t>();
            sceneLayerReader.Skip(8);
            model.objects[layerIdx].name = sceneLayerReader.ReadString();
            auto instanceIdx = sceneLayerReader.Read<uint32_t>();
            if (layerType == 0) { // collision
                if (instanceIdx < collisionMeshChunks.size()) {
                    auto &colObj = model.objects.emplace_back();
                    colObj.parent = model.objects[layerIdx].name;
                    Rx3Reader collisionReader(collisionMeshChunks[instanceIdx]);
                    collisionReader.Skip(16);
                    colObj.name = collisionReader.ReadString();
                    collisionReader.Skip(4);
                    auto numTriangles = collisionReader.Read<uint32_t>();
                    colObj.firstMesh().polygons.resize(numTriangles);
                    colObj.vertices.resize(numTriangles * 3);
                    for (uint32_t tri = 0; tri < numTriangles; tri++) {
                        colObj.firstMesh().polygons[tri].resize(3);
                        for (uint32_t v = 0; v < 3; v++) {
                            colObj.firstMesh().polygons[tri][v] = tri * 3 + v;
                            colObj.vertices[tri * 3 + v].pos = ReadVector3(collisionReader) / 100.0f;
                        }
                    }
                }
            }
            else { // instance
                if (instanceIdx < sceneInstanceChunks.size()) {
                    Rx3Reader scenInstanceReader(sceneInstanceChunks[instanceIdx]);
                    scenInstanceReader.Skip(4);
                    auto status = scenInstanceReader.Read<uint32_t>();
                    scenInstanceReader.Skip(8);
                    model.objects[layerIdx].transform = ReadMatrix4x4(scenInstanceReader);
                    for (uint32_t j = 0; j < 3; j++)
                        model.objects[layerIdx].transform.m[3][j] /= 100.0f;
                    scenInstanceReader.Skip(32);
                    if (status == 1) {
                        auto numMeshes = scenInstanceReader.Read<uint32_t>();
                        scenInstanceReader.Skip(4);
                        for (uint32_t m = 0; m < numMeshes; m++) {
                            scenInstanceReader.Skip(32);
                            auto meshIdx = scenInstanceReader.Read<uint32_t>();
                            auto materialIdx = scenInstanceReader.Read<uint32_t>();
                            if (meshIdx < indexBufferChunks.size() && meshIdx < vertexBufferChunks.size() && meshIdx < vertexFormatChunks.size()) {
                                auto &meshObj = model.objects.emplace_back();
                                meshObj.name = meshIdx < meshNames.size() ? meshNames[meshIdx] : ("object_" + to_string(model.objects.size()));
                                meshObj.parent = model.objects[layerIdx].name;
                                int primType = meshIdx < primTypes.size() ? primTypes[meshIdx] : RX3_PRIM_TRIANGLELIST;
                                auto quadBuffer = quadBufferChunks.size() == indexBufferChunks.size() ? quadBufferChunks[meshIdx] : nullptr;
                                SetupObjectMesh(meshObj, vertexFormatChunks[meshIdx], vertexBufferChunks[meshIdx], indexBufferChunks[meshIdx],
                                    quadBuffer, primType, 0, options);
                                if (materialIdx < model.materials.size())
                                    meshObj.firstMesh().material = model.materials[materialIdx].name;
                            }
                        }
                    }
                }
            }
        }
        if (!locationChunks.empty()) {
            auto &locationsRootObj = model.objects.emplace_back();
            locationsRootObj.name = "Locations";
            for (size_t i = 0; i < locationChunks.size(); i++) {
                Rx3Reader locationReader(locationChunks[i]);
                locationReader.Skip(4);
                auto pos = ReadVector3(locationReader) / 100.0f;
                auto rot = ReadVector3(locationReader);
                auto &locationObj = model.objects.emplace_back();
                locationObj.name = i < locationNames.size() ? locationNames[i] : "location_" + to_string(i + 1);
                locationObj.parent = "Locations";
                const float radToDeg = static_cast<float>(180.0 / M_PI);
                locationObj.transform.SetRotation({ (float)rot.x * radToDeg, (float)rot.y * radToDeg, (float)rot.z * radToDeg });
                locationObj.transform.SetTranslation(pos);
            }
        }
        if (!crowdPath.empty()) {
            FILE *crowdFile = nullptr;
            _wfopen_s(&crowdFile, crowdPath.c_str(), L"rb");
            if (crowdFile) {
                CrowdHeader header{};
                fread(&header, sizeof(CrowdHeader), 1, crowdFile);
                if (header.magic == 'DWRC') {
                    auto &crowdObj = model.objects.emplace_back();
                    crowdObj.name = "Crowd";
                    map<uint8_t, Object> tiers;
                    for (uint32_t i = 0; i < header.numseats; i++) {
                        Vector3 pos;
                        float angle = 0.0f;
                        RGBA seatcolor(0, 0, 0, 0), shade(0, 0, 0, 0), cardcolors(0, 0, 0, 0);
                        uint8_t tier = 0, section0 = 0, section1 = 0, attendance = 0, nochair = 0, crowdpattern = 0;
                        if (header.version == 0x103) {
                            SeatBlob_0x0103 seat;
                            fread(&seat, sizeof(SeatBlob_0x0103), 1, crowdFile);
                            pos = seat.pos / 100.0f;
                            angle = seat.rotation;
                            seatcolor.Set(seat.seatcolor[0], seat.seatcolor[1], seat.seatcolor[2], 255);
                            shade.Set(seat.shade[0], seat.shade[1], seat.shade[2], seat.shade[3]);
                            section0 = seat.section;
                            tier = seat.tier;
                            attendance = seat.attendance;
                        }
                        else if (header.version == 0x104) {
                            SeatBlob_0x0104 seat;
                            fread(&seat, sizeof(SeatBlob_0x0104), 1, crowdFile);
                            pos = seat.pos / 100.0f;
                            angle = seat.rotation;
                            seatcolor.Set(seat.seatcolor[0], seat.seatcolor[1], seat.seatcolor[2], 255);
                            shade.Set(seat.shade[0], seat.shade[1], seat.shade[2], seat.shade[3]);
                            section0 = seat.section0;
                            section1 = seat.section1;
                            tier = seat.tier;
                            attendance = seat.attendance;
                        }
                        else if (header.version == 0x105) {
                            SeatBlob_0x0105 seat;
                            fread(&seat, sizeof(SeatBlob_0x0105), 1, crowdFile);
                            pos = seat.pos / 100.0f;
                            angle = seat.rotation;
                            seatcolor.Set(seat.seatcolor[0], seat.seatcolor[1], seat.seatcolor[2], 255);
                            section0 = seat.section0;
                            section1 = seat.section1;
                            tier = seat.tier;
                            attendance = seat.attendance;
                            nochair = seat.nochair;
                            cardcolors = RGBA(seat.cardcolors[0], seat.cardcolors[1], seat.cardcolors[2], 255);
                            crowdpattern = seat.crowdpattern;
                        }
                        else
                            continue;
                        Object &obj = tiers[tier];
                        const float SeatScale = 0.3f;
                        Vector3 rect[4] = {
                            { -SeatScale, 0, 0 },
                            { -SeatScale, SeatScale * 2, 0 },
                            {  SeatScale, SeatScale * 2, 0 },
                            {  SeatScale, 0, 0 }
                        };
                        Matrix4x4 mat;
                        mat.SetIdentity();
                        mat.SetRotation({ 0.0f, angle + 90.0f, 0.0f });
                        mat.SetTranslation(pos);
                        size_t vertIndex = obj.vertices.size();
                        obj.firstMesh().polygons.push_back({ vertIndex + 0, vertIndex + 1, vertIndex + 2, vertIndex + 3 });
                        for (size_t v = 0; v < 4; v++) {
                            auto &vert = obj.vertices.emplace_back();
                            vert.pos = mat * rect[v];
                            vert.colors[0] = seatcolor;
                            vert.colors[1] = shade;
                            vert.colors[2] = RGBA(section0, section0, section0, 255);
                            vert.colors[3] = RGBA(section1, section1, section1, 255);
                            vert.colors[4] = RGBA(attendance, attendance, attendance, 255);
                            vert.colors[5] = RGBA(nochair, nochair, nochair, 255);
                            vert.colors[6] = cardcolors;
                            vert.colors[7] = RGBA(crowdpattern, crowdpattern, crowdpattern, 255);
                        }
                    }
                    string layerNames[] = { "SeatColor", "Shade", "NeutralHomeAway", "UltraHomeAway", "Attendance", "NoChair",
                        "CardColors", "CrowdPattern" };
                    for (auto &[tier, obj] : tiers) {
                        obj.name = "tier_" + to_string(tier);
                        obj.parent = "Crowd";
                        obj.vertexFormat = V_8Colors;
                        for (size_t l = 0; l < size(layerNames); l++)
                            obj.colorLayerNames[l] = layerNames[l];
                        model.objects.push_back(obj);
                    }
                }
                fclose(crowdFile);
            }
        }
    }
    return model;
}
