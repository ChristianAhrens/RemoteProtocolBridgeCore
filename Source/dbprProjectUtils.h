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
