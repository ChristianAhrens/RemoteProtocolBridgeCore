/* Copyright(c) 2023, Christian Ahrens
 *
 * This file is part of RemoteProtocolBridgeCore <https://github.com/ChristianAhrens/RemoteProtocolBridgeCore>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#ifdef USE_DBPR_PROJECT_UTILS

#include <JuceHeader.h>

#include <SQLiteCpp/SQLiteCpp.h>

struct CoordinateMappingData
{
    juce::String    _name;
    int             _venueObjectId;
    bool            _flip;
    double          _vp1x;
    double          _vp1y;
    double          _vp1z;
    double          _vp3x;
    double          _vp3y;
    double          _vp3z;
    double          _rp1x;
    double          _rp1y;
    double          _rp1z;
    double          _rp2x;
    double          _rp2y;
    double          _rp2z;
    double          _rp3x;
    double          _rp3y;
    double          _rp3z;
    double          _rp4x;
    double          _rp4y;
    double          _rp4z;

    juce::String ToString() const {
        auto s = juce::String();
        s   << _name << "," 
            << int(_flip) << "," 
            << _vp1x << "," << _vp1y << "," << _vp1z << "," 
            << _vp3x << "," << _vp3y << "," << _vp3z << ","
            << _rp1x << "," << _rp1y << "," << _rp1z << ","
            << _rp2x << "," << _rp2y << "," << _rp2z << ","
            << _rp3x << "," << _rp3y << "," << _rp3z << ","
            << _rp4x << "," << _rp4y << "," << _rp4z;
        return s;
    };
    static CoordinateMappingData FromString(const juce::String& coordinateMappingDataString) {
        juce::StringArray sa;
        sa.addTokens(coordinateMappingDataString, ",", {});
        if (sa.size() != 20)
        {
            jassertfalse;
            return {};
        }
        else
        {
            CoordinateMappingData retv = {};
            retv._name = sa[0];
            retv._flip = sa[1].getIntValue() == 1;
            retv._vp1x = sa[2].getDoubleValue();
            retv._vp1y = sa[3].getDoubleValue();
            retv._vp1z = sa[4].getDoubleValue();
            retv._vp3x = sa[5].getDoubleValue();
            retv._vp3y = sa[6].getDoubleValue();
            retv._vp3z = sa[7].getDoubleValue();
            retv._rp1x = sa[8].getDoubleValue();
            retv._rp1y = sa[9].getDoubleValue();
            retv._rp1z = sa[10].getDoubleValue();
            retv._rp2x = sa[11].getDoubleValue();
            retv._rp2y = sa[12].getDoubleValue();
            retv._rp2z = sa[13].getDoubleValue();
            retv._rp3x = sa[14].getDoubleValue();
            retv._rp3y = sa[15].getDoubleValue();
            retv._rp3z = sa[16].getDoubleValue();
            retv._rp4x = sa[17].getDoubleValue();
            retv._rp4y = sa[18].getDoubleValue();
            retv._rp4z = sa[19].getDoubleValue();
            return retv;
        }
    };
    bool IsNull() const {
        return (_vp1x == 0.0f &&
            _vp1y == 0.0f &&
            _vp1z == 0.0f &&
            _vp3x == 0.0f &&
            _vp3y == 0.0f &&
            _vp3z == 0.0f &&
            _rp1x == 0.0f &&
            _rp1y == 0.0f &&
            _rp1z == 0.0f &&
            _rp2x == 0.0f &&
            _rp2y == 0.0f &&
            _rp2z == 0.0f &&
            _rp3x == 0.0f &&
            _rp3y == 0.0f &&
            _rp3z == 0.0f &&
            _rp4x == 0.0f &&
            _rp4y == 0.0f &&
            _rp4z == 0.0f);
    };
};
typedef std::map<int, CoordinateMappingData> CoordinateMappingDataMap;
struct SpeakerPositionData
{
    double  _x;
    double  _y;
    double  _z;
    double  _hor;
    double  _vrt;
    double  _rot;

    juce::String ToString() const {
        auto s = juce::String();
        s << _x << "," << _y << "," << _z << "," << _hor << "," << _vrt << "," << _rot;
        return s;
    };
    static SpeakerPositionData FromString(const juce::String& speakerPositionDataString) {
        juce::StringArray sa;
        sa.addTokens(speakerPositionDataString, ",", {});
        if (sa.size() != 6)
        {
            jassertfalse;
            return {};
        }
        else
        {
            SpeakerPositionData retv = {};
            retv._x = sa[0].getDoubleValue();
            retv._y = sa[1].getDoubleValue();
            retv._z = sa[2].getDoubleValue();
            retv._hor = sa[3].getDoubleValue();
            retv._vrt = sa[4].getDoubleValue();
            retv._rot = sa[5].getDoubleValue();
            return retv;
        }
    };
    bool IsNull() const {
        return (    _x == 0.0f &&
                    _y == 0.0f &&
                    _z == 0.0f &&
                    _hor == 0.0f &&
                    _vrt == 0.0f &&
                    _rot == 0.0f);
    };
};
typedef std::map<int, SpeakerPositionData> SpeakerPositionDataMap;
typedef std::map<int, juce::String> InputNameDataMap;
struct ProjectData
{
    CoordinateMappingDataMap    _coordinateMappingData;
    SpeakerPositionDataMap      _speakerPositionData;
    InputNameDataMap            _inputNameData;

    bool IsEmpty() const {
        return _coordinateMappingData.empty() && _speakerPositionData.empty();
    };
    juce::String GetInfoString() const {
        auto s = juce::String();
        auto cmdCount = 0;
        auto spdCount = 0;
        for (auto const& cmdKV : _coordinateMappingData)
            if (!cmdKV.second.IsNull())
                cmdCount++;
        for (auto const& spdKV : _speakerPositionData)
            if (!spdKV.second.IsNull())
                spdCount++;
        s << cmdCount << " CMP, " << spdCount << " SPK";
        return s;
    };
    void Clear() {
        _coordinateMappingData.clear();
        _speakerPositionData.clear();
    };
    juce::String ToString() const {
        auto s = juce::String();
        s << "|COORDMAPDATA|";
        for (auto const& cmKV : _coordinateMappingData)
        {
            s << cmKV.first << ":" << cmKV.second.ToString() << ";";
        }
        s << "|SPKPOSDATA|";
        for (auto const& spKV : _speakerPositionData)
        {
            s << spKV.first << ":" << spKV.second.ToString() << ";";
        }
        s << "|INPUTNAMEDATA|";
        for (auto const& inKV : _inputNameData)
        {
            s << inKV.first << ":" << inKV.second << ";";
        }
        return s;
    };
    static ProjectData FromString(const juce::String& projectDataString) {
        juce::StringArray sa;
        sa.addTokens(projectDataString, "|", {});
        if (sa.size() != 7)
        {
            jassertfalse;
            return {};
        }
        else
        {
            ProjectData retv = {};

            auto coordMapsDataString = sa[2];
            juce::StringArray sacm;
            sacm.addTokens(coordMapsDataString, ";", {});
            for (auto const& sacms : sacm)
            {
                juce::StringArray sacmse;
                sacmse.addTokens(sacms, ":", {});
                if (sacmse.size() == 2)
                    retv._coordinateMappingData[sacmse[0].getIntValue()] = CoordinateMappingData::FromString(sacmse[1]);
            }

            auto spkPosDataString = sa[4];
            juce::StringArray sasp;
            sasp.addTokens(spkPosDataString, ";", {});
            for (auto const& sasps : sasp)
            {
                juce::StringArray saspse;
                saspse.addTokens(sasps, ":", {});
                if (saspse.size() == 2)
                    retv._speakerPositionData[saspse[0].getIntValue()] = SpeakerPositionData::FromString(saspse[1]);
            }

            auto inputNamesDataString = sa[6];
            juce::StringArray sain;
            sasp.addTokens(inputNamesDataString, ";", {});
            for (auto const& sains : sain)
            {
                juce::StringArray sainse;
                sainse.addTokens(sains, ":", {});
                if (sainse.size() == 2)
                    retv._inputNameData[sainse[0].getIntValue()] = sainse[1];
            }

            return retv;
        }
    };
    static ProjectData OpenAndReadProject(const juce::String& projectFilePath)
    {
        // SQLiteCpp requires exception handling
        try
        {
            ProjectData projectData;

            // Open a database file in create/write mode
            auto db = SQLite::Database(projectFilePath.toStdString(), SQLite::OPEN_READONLY);

            // read data for coordinate mapping settings
            auto queryCM = SQLite::Statement(db, "SELECT * FROM MatrixCoordinateMappings");
            while (queryCM.executeStep())
            {
                auto mappingAreaId = queryCM.getColumn(1).getInt(); // RecordNumber col 1
                auto venueObjectId = queryCM.getColumn(2).getInt(); // VenueObjectId col 2
                auto flip = bool(queryCM.getColumn(3).getUInt()); // Flip col 3
                auto mappingAreaName = queryCM.getColumn(4).getString(); // Name col 4

                projectData._coordinateMappingData[mappingAreaId]._venueObjectId = venueObjectId;
                projectData._coordinateMappingData[mappingAreaId]._flip = flip;
                projectData._coordinateMappingData[mappingAreaId]._name = mappingAreaName;
            }

            for (auto& cmDataKV : projectData._coordinateMappingData)
            {
                auto queryVO = SQLite::Statement(db, "SELECT * FROM VenueObjects WHERE VenueObjectID==" + std::to_string(cmDataKV.second._venueObjectId));
                while (queryVO.executeStep())
                {
                    auto originX = float(queryVO.getColumn("OriginX").getDouble());
                    auto originY = float(queryVO.getColumn("OriginY").getDouble());
                    auto originZ = float(queryVO.getColumn("OriginZ").getDouble());
                    auto rotationX = float(queryVO.getColumn("RotationX").getDouble());
                    auto rotationY = float(queryVO.getColumn("RotationY").getDouble());
                    auto rotationZ = float(queryVO.getColumn("RotationZ").getDouble());
                    auto scaleX = float(queryVO.getColumn("ScaleX").getDouble());
                    auto scaleY = float(queryVO.getColumn("ScaleY").getDouble());
                    auto scaleZ = float(queryVO.getColumn("ScaleZ").getDouble());
                    auto parentVenueObject = queryVO.getColumn("ParentVenueObjectId").getInt();
                    jassert(parentVenueObject == 0);
                    ignoreUnused(rotationX);
                    ignoreUnused(rotationY);
                    ignoreUnused(scaleZ);
                    ignoreUnused(originZ);

                    auto translationMatrix = juce::AffineTransform::translation(originX, originY);
                    auto scalingMatrix = juce::AffineTransform::scale(scaleX, scaleY, originX, originY);
                    auto rotationMatrix = juce::AffineTransform::rotation(juce::degreesToRadians(rotationZ), originX, originY);

                    auto queryVOP = SQLite::Statement(db, "SELECT * FROM VenueObjectPoints WHERE VenueObjectID==" + std::to_string(cmDataKV.second._venueObjectId));
                    while (queryVOP.executeStep())
                    {
                        auto pointIndex = queryVOP.getColumn("PointIndex").getInt(); // col 1
                        auto x = queryVOP.getColumn("X").getDouble(); // col 2
                        auto y = queryVOP.getColumn("Y").getDouble(); // col 3
                        auto z = queryVOP.getColumn("Z").getDouble(); // col 4

                        auto transformedVenuePoint = juce::Point<double>(x, y)
                            .transformedBy(translationMatrix)
                            .transformedBy(scalingMatrix)
                            .transformedBy(rotationMatrix);

                        switch (pointIndex)
                        {
                        case 0:
                            cmDataKV.second._rp1x = transformedVenuePoint.getX();
                            cmDataKV.second._rp1y = transformedVenuePoint.getY();
                            cmDataKV.second._rp1z = z;
                            break;
                        case 1:
                            cmDataKV.second._rp2x = transformedVenuePoint.getX();
                            cmDataKV.second._rp2y = transformedVenuePoint.getY();
                            cmDataKV.second._rp2z = z;
                            break;
                        case 2:
                            cmDataKV.second._rp3x = transformedVenuePoint.getX();
                            cmDataKV.second._rp3y = transformedVenuePoint.getY();
                            cmDataKV.second._rp3z = z;
                            break;
                        case 3:
                            cmDataKV.second._rp4x = transformedVenuePoint.getX();
                            cmDataKV.second._rp4y = transformedVenuePoint.getY();
                            cmDataKV.second._rp4z = z;
                            break;
                        default:
                            break;
                        }
                    }
                }
            }

            auto queryCMP = SQLite::Statement(db, "SELECT * FROM MatrixCoordinateMappingPoints");
            while (queryCMP.executeStep())
            {
                auto mappingAreaId = queryCMP.getColumn("RecordNumber").getInt(); // col 1
                auto pIdx = queryCMP.getColumn("PointIndex").getInt(); // col 2
                auto x = queryCMP.getColumn("X").getDouble(); // col 3
                auto y = queryCMP.getColumn("Y").getDouble(); // col 4
                auto z = queryCMP.getColumn("Z").getDouble(); // col 5

                if (pIdx == 0)
                {
                    projectData._coordinateMappingData[mappingAreaId]._vp1x = x;
                    projectData._coordinateMappingData[mappingAreaId]._vp1y = y;
                    projectData._coordinateMappingData[mappingAreaId]._vp1z = z;
                }
                else
                {
                    projectData._coordinateMappingData[mappingAreaId]._vp3x = x;
                    projectData._coordinateMappingData[mappingAreaId]._vp3y = y;
                    projectData._coordinateMappingData[mappingAreaId]._vp3z = z;
                }
            }

            // read data for speaker positions
            auto queryMO = SQLite::Statement(db, "SELECT * FROM MatrixOutputs");
            while (queryMO.executeStep())
            {
                auto outputNumber = queryMO.getColumn("MatrixOutput").getInt(); // col 1

                projectData._speakerPositionData[outputNumber]._x = queryMO.getColumn("CenterOfAudioX").getDouble(); // col 4
                projectData._speakerPositionData[outputNumber]._y = queryMO.getColumn("CenterOfAudioY").getDouble(); // col 5
                projectData._speakerPositionData[outputNumber]._z = queryMO.getColumn("CenterOfAudioZ").getDouble(); // col 6
                projectData._speakerPositionData[outputNumber]._hor = queryMO.getColumn("AimingAngleHorizontal").getDouble(); // col 7
                projectData._speakerPositionData[outputNumber]._vrt = queryMO.getColumn("AimingAngleVertical").getDouble(); // col 8
                projectData._speakerPositionData[outputNumber]._rot = queryMO.getColumn("AimingAngleRotation").getDouble(); // col 9
            }

            // read data for input names
            auto queryMI = SQLite::Statement(db, "SELECT * FROM MatrixInputs");
            while (queryMI.executeStep())
            {
                auto inputNumber = queryMI.getColumn("MatrixInput").getInt(); // col 1

                projectData._inputNameData[inputNumber] = queryMI.getColumn("Name").getString(); // col 2
            }

            return projectData;
        }
        catch (std::exception& e)
        {
            DBG(juce::String(__FUNCTION__) << " SQLite exception: " << e.what());
        }

        return {};
    }
};

#endif
