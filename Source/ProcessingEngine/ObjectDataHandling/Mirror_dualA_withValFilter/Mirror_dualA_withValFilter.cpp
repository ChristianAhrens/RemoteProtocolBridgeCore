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

#include "Mirror_dualA_withValFilter.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class Mirror_dualA_withValFilter_withValFilter
// **************************************************************************************
/**
 * Constructor of class Mirror_dualA_withValFilter_withValFilter.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
Mirror_dualA_withValFilter::Mirror_dualA_withValFilter(ProcessingEngineNode* parentNode)
	: Forward_only_valueChanges(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_Mirror_dualA_withValFilter);

	m_currentMaster = static_cast<ProtocolId>(INVALID_ADDRESS_VALUE);
	m_currentSlave = static_cast<ProtocolId>(INVALID_ADDRESS_VALUE);
	
	SetProtoFailoverTime(1000.0f);
}

/**
 * Destructor
 */
Mirror_dualA_withValFilter::~Mirror_dualA_withValFilter()
{
}

/**
 * Overridden from ObjectDataHandling_Abstract to use incoming ids
 * to initialize internal master and slave protocol ids.
 *
 * @param PAId	Protocol Id of typeA protocol to add.
 */
void Mirror_dualA_withValFilter::AddProtocolAId(ProtocolId PAId)
{
	ObjectDataHandling_Abstract::AddProtocolAId(PAId);

	if (GetProtocolAIds().size() == 1)
	{
		m_currentMaster = PAId;
		SetChangedProtocolState(m_currentMaster, OHS_Protocol_Master);
	}
	else if (GetProtocolAIds().size() == 2)
	{
		m_currentSlave = PAId;
		SetChangedProtocolState(m_currentSlave, OHS_Protocol_Slave);
	}
	else
		jassertfalse; // only two typeA protocols are supported by this OHM!
}

/**
 * Reimplemented to set the custom parts from configuration for the datahandling object.
 *
 * @param config	The overall configuration object that can be used to query config data from
 * @param NId		The node id of the parent node this data handling object is child of (needed to access data from config)
 */
bool Mirror_dualA_withValFilter::setStateXml(XmlElement* stateXml)
{
	if (!Forward_only_valueChanges::setStateXml(stateXml))
		return false;

	auto protoFailoverTimeElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::FAILOVERTIME));
	if (protoFailoverTimeElement)
		SetProtoFailoverTime(protoFailoverTimeElement->getAllSubText().getDoubleValue());
	else
		return false;

	return true;
}

/**
 * Method to be called by parent node on receiving data from node protocol with given id
 *
 * @param receiverProtocolId	The id of the protocol that received the data
 * @param roi					The object id to send a message for
 * @param msgData				The actual message value/content data
 * @param msgMeta				The meta information on the message data that was received
 * @return	True if successful sent/forwarded, false if not
 */
bool Mirror_dualA_withValFilter::OnReceivedMessageFromProtocol(const ProtocolId receiverProtocolId, const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta)
{
	// a valid parent node is required to be able to do anything with the received message
	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	// do some sanity checks on this instances configuration parameters and the given message data origin id
	auto mirrorConfigValid = (GetProtocolAIds().size() == 2);
	auto isProtocolTypeA = (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), receiverProtocolId) != GetProtocolAIds().end());
	auto isProtocolTypeB = (std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), receiverProtocolId) != GetProtocolBIds().end());
	auto protocolIdValid = isProtocolTypeA || isProtocolTypeB;

	if (!mirrorConfigValid || !protocolIdValid)
		return false;

	UpdateOnlineState(receiverProtocolId);

	if (IsCachedValuesQuery(roi))
		return SendValueCacheToProtocol(receiverProtocolId);

	// check the incoming data regarding value change to then forward and if required mirror it to other protocols
	if (IsChangedDataValue(receiverProtocolId, roi, msgData._addrVal, msgData))
	{
		// mirror and forward to all B if data comes from A
		if (isProtocolTypeA)
		{
			// data mirroring is only done inbetween typeA protocols
			MirrorDataIfRequired(receiverProtocolId, roi, msgData);

			// now do the basic A to B forwarding - only from master A
			if (receiverProtocolId == m_currentMaster)
			{
				auto sendSuccess = true;
				for (auto const& protocolBId : GetProtocolBIds())
				{
					if (msgMeta._ExternalId != roi || msgMeta._Category != RemoteObjectMessageMetaInfo::MC_SetMessageAcknowledgement)
					{
						// sending is only done when the value about to be sent is differing from the last known value from the protocol in question
						if (IsChangedDataValue(protocolBId, roi, msgData._addrVal, msgData))
						{
							sendSuccess = parentNode->SendMessageTo(protocolBId, roi, msgData) && sendSuccess;
							// If the value was sent successfully, save it to cache (to make it the 'last known' from this protocol).
							// In case the protocol is expected to acknowledge the value, we make an exception, since acknowledge values are
							// used to update bridged protocols that have not yet received that latest value. E.g. DS100 ack values that are a
							// reaction on a GenericOSC SET have to be bridged back to connected DiGiCo.
							if (sendSuccess && !IsTypeBAcknowledging())
								SetCurrentValue(protocolBId, roi, msgData._addrVal, msgData); // set the updated value as current for the complementary cache as well
						}
					}
				}
				return sendSuccess;
			}
			else
				return true;
		}
		// forward to A current master if data comes from B
		else if (isProtocolTypeB)
		{
			// sending is only done when the value about to be sent is differing from the last known value from the protocol in question
			if (IsChangedDataValue(m_currentMaster, roi, msgData._addrVal, msgData))
			{
				auto sendSuccess = parentNode->SendMessageTo(m_currentMaster, roi, msgData);
				// If the value was sent successfully, save it to cache (to make it the 'last known' from this protocol).
				// In case the protocol is expected to acknowledge the value, we make an exception, since acknowledge values are
				// used to update bridged protocols that have not yet received that latest value. E.g. DS100 ack values that are a
				// reaction on a GenericOSC SET have to be bridged back to connected DiGiCo.
				if (sendSuccess && !IsTypeAAcknowledging())
					SetCurrentValue(m_currentMaster, roi, msgData._addrVal, msgData); // set the updated value as current for the complementary cache as well

				return sendSuccess;
			}
			else
				return true;
		}
		else
			return false;
	}
	else
		return false;
}


/**
 * Reimplemented from ObjectDataHandling.
 * This implementation first forwards the call to base implementation and
 * afterwards takes care of master/slave switching if required.
 * 
 * @param	id	The protocol to update the online state for
 */
void Mirror_dualA_withValFilter::UpdateOnlineState(ProtocolId id)
{
	ObjectDataHandling_Abstract::UpdateOnlineState(id);

	// swap master and slave if the master has failed to react in the configured failover time
	if (id == m_currentSlave && GetLastProtocolReactionTSMap().count(m_currentMaster) > 0)
	{
		auto masterStaleTime = Time::getMillisecondCounterHiRes() - GetLastProtocolReactionTSMap().at(m_currentMaster);
		if (masterStaleTime > GetProtoFailoverTime())
		{
			SetChangedProtocolState(m_currentMaster, OHS_Protocol_Slave);
			SetChangedProtocolState(m_currentSlave, OHS_Protocol_Master);
			std::swap(m_currentMaster, m_currentSlave);
		}
	}
}

/**
 * Method to check if the incoming protocol typeA data shall be mirrored to the other typeA protocol
 * and if the check is positive, exec the mirroring by sending the data.
 *
 * @param PId		The id of the protocol that received the data
 * @param roi		The object id that was received
 * @param msgData	The actual message value/content data
 * @return			True if the mirroring was executed or not required, otherwise false
 */
bool Mirror_dualA_withValFilter::MirrorDataIfRequired(ProtocolId PId, const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData)
{
	// a valid parent node is required to be able to do anything with the received message
	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	// only typeA protocols are relevant and expected as input for mirroring in this OHM
	if (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) == GetProtocolAIds().end() || !parentNode)
	{
		jassertfalse;
		return false;
	}
	
	// send values received from master to slave
	if (PId == m_currentMaster && m_currentSlave != INVALID_ADDRESS_VALUE)
	{
		// sending is only done when the value about to be sent is differing from the last known value from the protocol in question
		if (IsChangedDataValue(m_currentSlave, roi, msgData._addrVal, msgData))
		{
			auto sendSuccess = parentNode->SendMessageTo(m_currentSlave, roi, msgData);
			// If the value was sent successfully, save it to cache (to make it the 'last known' from this protocol).
			// In case the protocol is expected to acknowledge the value, we make an exception, since acknowledge values are
			// used to update bridged protocols that have not yet received that latest value. E.g. DS100 ack values that are a
			// reaction on a GenericOSC SET have to be bridged back to connected DiGiCo.
			if (sendSuccess && !IsTypeAAcknowledging())
				SetCurrentValue(m_currentSlave, roi, msgData._addrVal, msgData); // set the updated value as current for the complementary cache as well

			return sendSuccess;
		}
		else
			return true;
	}
	else
		return false;
}

/**
 * Setter for the protocol failover time value member.
 * @param	timeout		The new value to set as failover time (ms)
 */
void Mirror_dualA_withValFilter::SetProtoFailoverTime(double timeout)
{
	m_protoFailoverTime = timeout;
}

/**
 * Getter for the protocol failover time value member.
 * @return	The current value for failover time (ms)
 */
double Mirror_dualA_withValFilter::GetProtoFailoverTime()
{
	return m_protoFailoverTime;
}
