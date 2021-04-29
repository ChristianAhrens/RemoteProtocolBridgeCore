/*
===============================================================================

Copyright (C) 2019 d&b audiotechnik GmbH & Co. KG. All Rights Reserved.

This file is part of RemoteProtocolBridge.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY d&b audiotechnik GmbH & Co. KG "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===============================================================================
*/

#include "Forward_only_valueChanges.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class Forward_only_valueChanges
// **************************************************************************************
/**
 * Constructor of class Forward_only_valueChanges.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
Forward_only_valueChanges::Forward_only_valueChanges(ProcessingEngineNode* parentNode)
	: ObjectDataHandling_Abstract(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_Forward_only_valueChanges);
	m_precision = 0.001f;
}

/**
 * Destructor
 */
Forward_only_valueChanges::~Forward_only_valueChanges()
{
}

/**
 * Reimplemented to set the custom parts from configuration for the datahandling object.
 *
 * @param config	The overall configuration object that can be used to query config data from
 * @param NId		The node id of the parent node this data handling object is child of (needed to access data from config)
 */
bool Forward_only_valueChanges::setStateXml(XmlElement* stateXml)
{
	if (!ObjectDataHandling_Abstract::setStateXml(stateXml))
		return false;

	if (stateXml->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MODE)) != ProcessingEngineConfig::ObjectHandlingModeToString(OHM_Forward_only_valueChanges))
		return false;

	auto precisionXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::DATAPRECISION));
	if (precisionXmlElement)
		m_precision = precisionXmlElement->getAllSubText().getFloatValue();
	else
		return false;

	return true;
}

/**
 * Method to be called by parent node on receiving data from node protocol with given id
 *
 * @param PId		The id of the protocol that received the data
 * @param Id		The object id to send a message for
 * @param msgData	The actual message value/content data
 * @return	True if successful sent/forwarded, false if not
 */
bool Forward_only_valueChanges::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	const ProcessingEngineNode* parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;
	
	UpdateOnlineState(PId);

	if (!IsChangedDataValue(Id, msgData._addrVal, msgData))
		return false;

	if (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) != GetProtocolAIds().end())
	{
		// Send to all typeB protocols
		auto sendSuccess = true;
		for (auto const& protocolB : GetProtocolBIds())
			sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolB, Id, msgData);

		return sendSuccess;
	}
	else if (std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId) != GetProtocolBIds().end())
	{
		// Send to all typeA protocols
		auto sendSuccess = true;
		for (auto const& protocolA : GetProtocolAIds())
			sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolA, Id, msgData);

		return sendSuccess;
	}
	else
		return false;
}

/**
 * Helper method to detect if incoming value has changed in any way compared with the previously received one
 * (RemoteObjectIdentifier is taken in account as well as the channel/record addressing)
 *
 * @param Id					The ROI that was received and has to be checked
 * @param roAddr				The remote object addressing the data shall be verified against regarding changes
 * @param msgData				The received message data that has to be checked
 * @param setAsNewCurrentData	Bool indication if in addition to change check, the value shall be set as new current data if the check was positive
 * @return						True if a change has been detected, false if not
 */
bool Forward_only_valueChanges::IsChangedDataValue(const RemoteObjectIdentifier Id, const RemoteObjectAddressing& roAddr, const RemoteObjectMessageData& msgData, bool setAsNewCurrentData)
{
	if (m_precision == 0)
		return true;

	auto isChangedDataValue = false;

	// if our hash does not yet contain our ROI, initialize it
	if ((m_currentValues.count(Id) == 0) || (m_currentValues.at(Id).count(roAddr) == 0))
	{
		isChangedDataValue = true;
	}
	else
	{
		const RemoteObjectMessageData& currentVal = m_currentValues.at(Id).at(roAddr);
		if ((currentVal._valType != msgData._valType) || (currentVal._valCount != msgData._valCount) || (currentVal._payloadSize != msgData._payloadSize))
		{
			isChangedDataValue = true;
		}
		else
		{
			uint16 valCount = currentVal._valCount;
			RemoteObjectValueType valType = currentVal._valType;
			auto refData = currentVal._payload;
			auto newData = msgData._payload;
	
			auto referencePrecisionValue = 0;
			auto newPrecisionValue = 0;
			
			auto changeFound = false;
			for (int i = 0; i < valCount; ++i)
			{
				switch (valType)
				{
				case ROVT_INT:
					{
					// convert payload to correct pointer type
					auto refVal = static_cast<int*>(refData);
					auto newVal = static_cast<int*>(newData);
					// grab actual value
					referencePrecisionValue = *refVal;
					newPrecisionValue = *newVal;
					// increase pointer to next value (to access it in next valCount loop iteration)
					refData = refVal+1;
					newData = newVal+1;
					}
					break;
				case ROVT_FLOAT:
					{
					// convert payload to correct pointer type
					auto refVal = static_cast<float*>(refData);
					auto newVal = static_cast<float*>(newData);
					// grab actual value and apply precision to get a comparable value
					referencePrecisionValue = static_cast<int>(std::roundf(*refVal / m_precision));
					newPrecisionValue		= static_cast<int>(std::roundf(*newVal / m_precision));
					// increase pointer to next value (to access it in next valCount loop iteration)
					refData = refVal+1;
					newData = newVal+1;
					}
					break;
				case ROVT_STRING:
					{
					auto refVal = String(static_cast<char*>(currentVal._payload), currentVal._payloadSize);
					auto newVal = String(static_cast<char*>(msgData._payload), msgData._payloadSize);
					changeFound = (refVal != newVal);
					}
					break;
				case ROVT_NONE:
				default:
					changeFound = true;
					break;
				}
	
	
				if (referencePrecisionValue != newPrecisionValue)
					changeFound = true;
			}
	
			isChangedDataValue = changeFound;
		}
	}

	if (isChangedDataValue && setAsNewCurrentData)
		SetCurrentDataValue(Id, roAddr, msgData);

	return isChangedDataValue;
}

/**
 * Helper method to set a new RemoteObjectMessageData obj. to internal map of current values.
 * Takes care of cleaning up previously stored data if required.
 *
 * @param Id		The ROI that shall be stored
 * @param roAddr	The remote object addressing the data shall be updated for
 * @param msgData	The message data that shall be stored
 */
void Forward_only_valueChanges::SetCurrentDataValue(const RemoteObjectIdentifier Id, const RemoteObjectAddressing& roAddr, const RemoteObjectMessageData& msgData)
{
	// Check if the new data value addressing is currently not present in internal hash
	// or if it differs in its value size and needs to be reinitialized
	if ((m_currentValues.count(Id) == 0) || (m_currentValues.at(Id).count(roAddr) == 0) ||
		(m_currentValues.at(Id).at(roAddr)._payloadSize != msgData._payloadSize))
	{
		// If the data value exists, but has wrong size, reinitialize it
		if((m_currentValues.count(Id) != 0) && (m_currentValues.at(Id).count(roAddr) != 0) &&
			(m_currentValues.at(Id).at(roAddr)._payloadSize != msgData._payloadSize))
		{
            m_currentValues.at(Id).at(roAddr)._payload = nullptr;
            m_currentValues.at(Id).at(roAddr)._payloadSize = 0;
            m_currentValues.at(Id).at(roAddr)._valCount = 0;
		}
	
		m_currentValues[Id][roAddr].payloadCopy(msgData);
	}
	else
	{
		// do not copy entire data struct, since we need to keep our payload ptr
		m_currentValues.at(Id).at(roAddr)._addrVal = roAddr;
		m_currentValues.at(Id).at(roAddr)._valCount = msgData._valCount;
		m_currentValues.at(Id).at(roAddr)._valType = msgData._valType;
		memcpy(m_currentValues.at(Id).at(roAddr)._payload, msgData._payload, msgData._payloadSize);
	}
}

/**
 * Getter for the private precision value
 * @return	The internal precision value.
 */
float Forward_only_valueChanges::GetPrecision()
{
	return m_precision;
}

/**
 * Setter for the private precision value
 * @param precision	The precision value to set.
 */
void Forward_only_valueChanges::SetPrecision(float precision)
{
	m_precision = precision;
}