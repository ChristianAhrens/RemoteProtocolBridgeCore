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

#include <JuceHeader.h>

#include <SQLiteCpp/SQLiteCpp.h>

#ifdef USE_DBPR_PROJECT_UTILS

struct CoordinateMappingData
{
    juce::String    _name;
    bool            _flip;
    double          _p1x;
    double          _p1y;
    double          _p1z;
    double          _p3x;
    double          _p3y;
    double          _p3z;

    juce::String ToString() const {
        auto s = juce::String();
        s << _name << "," << int(_flip) << "," << _p1x << "," << _p1y << "," << _p1z << "," << _p3x << "," << _p3y << "," << _p3z;
        return s;
    };
    static CoordinateMappingData FromString(const juce::String& coordinateMappingDataString) {
        juce::StringArray sa;
        sa.addTokens(coordinateMappingDataString, ",", {});
        if (sa.size() != 8)
        {
            jassertfalse;
            return {};
        }
        else
        {
            CoordinateMappingData retv = {};
            retv._name = sa[0];
            retv._flip = sa[1].getIntValue() == 1;
            retv._p1x = sa[2].getDoubleValue();
            retv._p1y = sa[3].getDoubleValue();
            retv._p1z = sa[4].getDoubleValue();
            retv._p3x = sa[5].getDoubleValue();
            retv._p3y = sa[6].getDoubleValue();
            retv._p3z = sa[7].getDoubleValue();
            return retv;
        }
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
        s << _coordinateMappingData.size() << " CMP, " << _speakerPositionData.size() << " SPK";
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
        try
        {
            ProjectData projectData;

            // Open a database file in create/write mode
            auto db = SQLite::Database(projectFilePath.toStdString(), SQLite::OPEN_READONLY);

            // read data for coordinate mapping settings
            auto queryCM = SQLite::Statement(db, "SELECT * FROM MatrixCoordinateMappings");
            while (queryCM.executeStep())
            {
                auto mappingAreaId = queryCM.getColumn(1).getInt();
                auto flip = bool(queryCM.getColumn(3).getUInt());
                auto mappingAreaName = queryCM.getColumn(4).getString();

                projectData._coordinateMappingData[mappingAreaId]._flip = flip;
                projectData._coordinateMappingData[mappingAreaId]._name = mappingAreaName;
            }

            auto queryCMP = SQLite::Statement(db, "SELECT * FROM MatrixCoordinateMappingPoints");
            while (queryCMP.executeStep())
            {
                auto mappingAreaId = queryCMP.getColumn(1).getInt();
                auto pIdx = queryCMP.getColumn(2).getInt();
                auto x = queryCMP.getColumn(3).getDouble();
                auto y = queryCMP.getColumn(4).getDouble();
                auto z = queryCMP.getColumn(5).getDouble();

                if (pIdx == 0)
                {
                    projectData._coordinateMappingData[mappingAreaId]._p1x = x;
                    projectData._coordinateMappingData[mappingAreaId]._p1y = y;
                    projectData._coordinateMappingData[mappingAreaId]._p1z = z;
                }
                else
                {
                    projectData._coordinateMappingData[mappingAreaId]._p3x = x;
                    projectData._coordinateMappingData[mappingAreaId]._p3y = y;
                    projectData._coordinateMappingData[mappingAreaId]._p3z = z;
                }
            }

            // read data for speaker positions
            auto queryMO = SQLite::Statement(db, "SELECT * FROM MatrixOutputs");
            while (queryMO.executeStep())
            {
                auto outputNumber = queryMO.getColumn(1).getInt();

                projectData._speakerPositionData[outputNumber]._x = queryMO.getColumn(4).getDouble();
                projectData._speakerPositionData[outputNumber]._y = queryMO.getColumn(5).getDouble();
                projectData._speakerPositionData[outputNumber]._z = queryMO.getColumn(6).getDouble();
                projectData._speakerPositionData[outputNumber]._hor = queryMO.getColumn(7).getDouble();
                projectData._speakerPositionData[outputNumber]._vrt = queryMO.getColumn(8).getDouble();
                projectData._speakerPositionData[outputNumber]._rot = queryMO.getColumn(9).getDouble();
            }

            // read data for input names
            auto queryMI = SQLite::Statement(db, "SELECT * FROM MatrixInputs");
            while (queryMI.executeStep())
            {
                auto inputNumber = queryMI.getColumn(1).getInt();

                projectData._inputNameData[inputNumber] = queryMI.getColumn(2).getString();
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
