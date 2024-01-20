/* Copyright (c) 2020-2023, Christian Ahrens
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

	auto precisionXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::DATAPRECISION));
	if (precisionXmlElement)
		m_precision = precisionXmlElement->getAllSubText().getFloatValue();
	else
		return false;

	auto protocolsAcknowledgeXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::VALUEACK));
	if (protocolsAcknowledgeXmlElement)
	{
		auto typeABAckMask = protocolsAcknowledgeXmlElement->getAllSubText().getIntValue();
		m_typeAIsAcknowledging = (0 != (typeABAckMask & 0x01));
		m_typeBIsAcknowledging = (0 != (typeABAckMask & 0x10));
	}

	return true;
}

/**
 * Method to be called by parent node on receiving data from node protocol with given id
 *
 * @param PId		The id of the protocol that received the data
 * @param roi		The object id to send a message for
 * @param msgData	The actual message value/content data
 * @param msgMeta	The meta information on the message data that was received
 * @return	True if successful sent/forwarded, false if not
 */
bool Forward_only_valueChanges::OnReceivedMessageFromProtocol(const ProtocolId PId, const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta)
{
	const ProcessingEngineNode* parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;
	
	UpdateOnlineState(PId);

	if (IsCachedValuesQuery(roi))
		return SendValueCacheToProtocol(PId);

	// Check the incoming value against the currently cached value for the incoming protocol.
	// The currently cached value is taken from one of the two caches, A or B, and if a change 
	// is detected, the value in that cache is updated accordingly. 
	// BEWARE it is not updated in the complementary other cache - that is/has to be done after successful sending
	if (!IsChangedDataValue(PId, roi, msgData._addrVal, msgData))
		return false;

	if (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) != GetProtocolAIds().end())
	{
		// Send to all typeB protocols
		auto overallSendSuccess = true;
		for (auto const& protocolB : GetProtocolBIds())
		{
			if (msgMeta._ExternalId != protocolB || msgMeta._Category != RemoteObjectMessageMetaInfo::MC_SetMessageAcknowledgement)
			{
				// sending is only done when the value about to be sent is differing from the last known value from the protocol in question
				if (IsChangedDataValue(protocolB, roi, msgData._addrVal, msgData, false))
				{
					auto sendSuccess = parentNode->SendMessageTo(protocolB, roi, msgData);
					// If the value was sent successfully, save it to cache (to make it the 'last known' from this protocol).
					// In case the protocol is expected to acknowledge the value, we make an exception, since acknowledge values are
					// used to update bridged protocols that have not yet received that latest value. E.g. DS100 ack values that are a
					// reaction on a GenericOSC SET have to be bridged back to connected DiGiCo.
					if (sendSuccess && !IsTypeBAcknowledging())
						SetCurrentValue(protocolB, roi, msgData._addrVal, msgData); // set the updated value as current for the complementary cache as well
					overallSendSuccess = sendSuccess && overallSendSuccess;
				}
			}
		}

		return overallSendSuccess;
	}
	else if (std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId) != GetProtocolBIds().end())
	{
		// Send to all typeA protocols
		auto overallSendSuccess = true;
		for (auto const& protocolA : GetProtocolAIds())
		{
			if (msgMeta._ExternalId != protocolA || msgMeta._Category != RemoteObjectMessageMetaInfo::MC_SetMessageAcknowledgement)
			{
				// sending is only done when the value about to be sent is differing from the last known value from the protocol in question
				if (IsChangedDataValue(protocolA, roi, msgData._addrVal, msgData, false))
				{
					auto sendSuccess = parentNode->SendMessageTo(protocolA, roi, msgData);
					// If the value was sent successfully, save it to cache (to make it the 'last known' from this protocol).
					// In case the protocol is expected to acknowledge the value, we make an exception, since acknowledge values are
					// used to update bridged protocols that have not yet received that latest value. E.g. DS100 ack values that are a
					// reaction on a GenericOSC SET have to be bridged back to connected DiGiCo.
					if (sendSuccess && !IsTypeAAcknowledging())
						SetCurrentValue(protocolA, roi, msgData._addrVal, msgData); // set the updated value as current for the complementary cache as well
					overallSendSuccess = sendSuccess && overallSendSuccess;
				}
			}
		}

		return overallSendSuccess;
	}
	else
		return false;
}

/**
 * Helper method to detect if incoming value has changed in any way compared with the previously received one
 * (RemoteObjectIdentifier is taken in account as well as the channel/record addressing)
 *
 * @param PId                                          The id of the protocol that received the value to check for if it differs from currently cached one.
 * @param roi					The ROI that was received and has to be checked
 * @param roAddr				The remote object addressing the data shall be verified against regarding changes
 * @param msgData				The received message data that has to be checked
 * @param setAsNewCurrentData	Bool indication if in addition to change check, the value shall be set as new current data if the check was positive
 * @return						True if a change has been detected, false if not
 */
bool Forward_only_valueChanges::IsChangedDataValue(const ProtocolId PId, const RemoteObjectIdentifier roi, const RemoteObjectAddressing& roAddr, const RemoteObjectMessageData& msgData, bool setAsNewCurrentData)
{
	if (IsGetValueQuery(roi, msgData))
		return true;
	if (IsKeepaliveObject(roi))
		return true;
    
	if (m_precision == 0)
		return true;

	auto isChangedDataValue = false;
    
    // verify the protocol in question has a set of cached values
	if (1 != m_currentValues.count(PId))
		isChangedDataValue = true;
    
    // Depending on what protocol received the value, use the corresponding value cache for the protocol type
	auto currentValues = &m_currentValues[PId];

	// if our hash does not yet contain our ROI, initialize it
	if ((currentValues->count(roi) == 0) || (currentValues->at(roi).count(roAddr) == 0))
	{
		isChangedDataValue = true;
	}
	else
	{
		const RemoteObjectMessageData& currentVal = currentValues->at(roi).at(roAddr);
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
			
			for (int i = 0; i < valCount; ++i)
			{
				switch (valType)
				{
				case ROVT_INT:
					{
					// convert payload to correct pointer type
					auto refValPtr = static_cast<int*>(refData);
					auto newValPtr = static_cast<int*>(newData);
					// grab actual value
					auto referenceValue = *refValPtr;
					auto newValue = *newValPtr;
					// increase pointer to next value (to access it in next valCount loop iteration)
					refData = refValPtr +1;
					newData = newValPtr +1;
					// if integer values differ, precision is irrelevant
					isChangedDataValue = isChangedDataValue || (referenceValue != newValue);
					}
					break;
				case ROVT_FLOAT:
					{
					// convert payload to correct pointer type
					auto refValPtr = static_cast<float*>(refData);
					auto newValPtr = static_cast<float*>(newData);
					// grab actual value and apply precision to get a comparable value
					auto referenceValue = *refValPtr;
					auto newValue		= *newValPtr;
					auto valueDifference = referenceValue - newValue;
					// increase pointer to next value (to access it in next valCount loop iteration)
					refData = refValPtr +1;
					newData = newValPtr +1;
					// if the float difference is up to equal of the configured precision, no change is signaled
					isChangedDataValue = isChangedDataValue || (std::fabs(valueDifference) > m_precision);
					}
					break;
				case ROVT_STRING:
					{
					auto refVal = String(static_cast<char*>(currentVal._payload), currentVal._payloadSize);
					auto newVal = String(static_cast<char*>(msgData._payload), msgData._payloadSize);
					isChangedDataValue = isChangedDataValue || (refVal != newVal);
					}
					break;
				case ROVT_NONE:
				default:
					isChangedDataValue = true;
					break;
				}
			}
		}
	}

	if (isChangedDataValue && setAsNewCurrentData)
		SetCurrentValue(PId, roi, roAddr, msgData);

	return isChangedDataValue;
}

/**
 * Helper method to set a new RemoteObjectMessageData obj. to internal map of current values.
 * Takes care of cleaning up previously stored data if required.
 *
 * @param PId                The id of the protocol that received the value to check for if it differs from currently cached one.
 * @param roi		The ROI that shall be stored
 * @param roAddr	The remote object addressing the data shall be updated for
 * @param msgData	The message data that shall be stored
 */
void Forward_only_valueChanges::SetCurrentValue(const ProtocolId PId, const RemoteObjectIdentifier roi, const RemoteObjectAddressing& roAddr, const RemoteObjectMessageData& msgData)
{
	if (IsKeepaliveObject(roi))
		return;
	// Depending on what protocol received the value, use the corresponding value cache for the protocol type
	auto currentValues = &m_currentValues[PId];
    
	// Check if the new data value addressing is currently not present in internal hash
	// or if it differs in its value size and needs to be reinitialized
	if ((currentValues->count(roi) == 0) || (currentValues->at(roi).count(roAddr) == 0) ||
		(currentValues->at(roi).at(roAddr)._payloadSize != msgData._payloadSize))
	{
		// If the data value exists, but has wrong size, reinitialize it
		if((currentValues->count(roi) != 0) && (currentValues->at(roi).count(roAddr) != 0) &&
			(currentValues->at(roi).at(roAddr)._payloadSize != msgData._payloadSize))
		{
            currentValues->at(roi).at(roAddr)._payload = nullptr;
            currentValues->at(roi).at(roAddr)._payloadSize = 0;
            currentValues->at(roi).at(roAddr)._valCount = 0;
		}
	
        (*currentValues)[roi][roAddr].payloadCopy(msgData);
	}
	else
	{
		// do not copy entire data struct, since we need to keep our payload ptr
        currentValues->at(roi).at(roAddr)._addrVal = roAddr;
        currentValues->at(roi).at(roAddr)._valCount = msgData._valCount;
        currentValues->at(roi).at(roAddr)._valType = msgData._valType;
		memcpy(currentValues->at(roi).at(roAddr)._payload, msgData._payload, msgData._payloadSize);
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

/**
 * Checks if a given remote object id is a query for value cache contents.
 * @param	roi	The remote object identifier that might be a query for value cache contents
 * @return	True if the roi is such a query
 */
bool Forward_only_valueChanges::IsCachedValuesQuery(const RemoteObjectIdentifier roi)
{
	return (ROI_RemoteProtocolBridge_GetAllKnownValues == roi);
}

/**
 * Sends the contents of internal value cache to the specified protocol via parent listener.
 * @param	PId	The id of the protocol to send the value cache contents to.
 * @param	Bool on success, false if some error occured (params invalid, sending failed...)
 */
bool Forward_only_valueChanges::SendValueCacheToProtocol(const ProtocolId PId)
{
	const ProcessingEngineNode* parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	// verify the protocol in question has a set of cached values
	if (1 != m_currentValues.count(PId))
		return false;

    auto sendSuccess = true;
    for (auto const& cachedValue : m_currentValues.at(PId))
        for (auto const& cachedValueObject : cachedValue.second)
            sendSuccess = sendSuccess && parentNode->SendMessageTo(PId, cachedValue.first, cachedValueObject.second);
    return sendSuccess;
}

/**
 * Protected getter for the type A is acknowledging member field.
 * @return	The bool member value as currently set.
 */
bool Forward_only_valueChanges::IsTypeAAcknowledging()
{
	return m_typeAIsAcknowledging;
}

/**
 * Protected getter for the type B is acknowledging member field.
 * @return	The bool member value as currently set.
 */
bool Forward_only_valueChanges::IsTypeBAcknowledging()
{
	return m_typeBIsAcknowledging;
}
