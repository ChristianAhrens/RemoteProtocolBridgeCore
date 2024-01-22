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

#include "Mux_nA_to_mB_withValFilter.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class Mux_nA_to_mB_withValFilter_withValFilter
// **************************************************************************************
/**
 * Constructor of class Mux_nA_to_mB_withValFilter_withValFilter.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
Mux_nA_to_mB_withValFilter::Mux_nA_to_mB_withValFilter(ProcessingEngineNode* parentNode)
	: Forward_only_valueChanges(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_Mux_nA_to_mB_withValFilter);
	m_protoChCntA	= 1;
	m_protoChCntB	= 1;
}

/**
 * Destructor
 */
Mux_nA_to_mB_withValFilter::~Mux_nA_to_mB_withValFilter()
{
}

/**
 * Reimplemented to set the custom parts from configuration for the datahandling object.
 *
 * @param config	The overall configuration object that can be used to query config data from
 * @param NId		The node id of the parent node this data handling object is child of (needed to access data from config)
 */
bool Mux_nA_to_mB_withValFilter::setStateXml(XmlElement* stateXml)
{
	if (!Forward_only_valueChanges::setStateXml(stateXml))
		return false;

	auto protoChCntAElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLACHCNT));
	if (protoChCntAElement)
		m_protoChCntA = protoChCntAElement->getAllSubText().getIntValue();
	else
		return false;

	auto protoChCntBElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLBCHCNT));
	if (protoChCntBElement)
		m_protoChCntB = protoChCntBElement->getAllSubText().getIntValue();
	else
		return false;

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
bool Mux_nA_to_mB_withValFilter::OnReceivedMessageFromProtocol(const ProtocolId PId, const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta)
{
	// a valid parent node is required to be able to do anything with the received message
	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	// do some sanity checks on this instances configuration parameters and the given message data origin id
	auto muxConfigValid = (m_protoChCntA > 0 || m_protoChCntA == INVALID_ADDRESS_VALUE) && (m_protoChCntB > 0 || m_protoChCntB == INVALID_ADDRESS_VALUE);
	auto isProtocolTypeA = (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) != GetProtocolAIds().end());
	auto isProtocolTypeB = (std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId) != GetProtocolBIds().end());
	auto protocolIdValid = isProtocolTypeA || isProtocolTypeB;

	if (!muxConfigValid || !protocolIdValid)
		return false;

	UpdateOnlineState(PId);

	if (IsCachedValuesQuery(roi))
		return SendValueCacheToProtocol(PId);

	auto isGetValueQuery = IsGetValueQuery(roi, msgData);
	if (isGetValueQuery)
		SetCurrentValue(PId, roi, msgData._addrVal, RemoteObjectMessageData());

	auto modMsgData = msgData;
	// check for changed value based on mapped addressing and target protocol id before forwarding data
	auto targetProtoSrc = GetTargetProtocolsAndSource(PId, modMsgData);
	auto mappedOrigAddr = GetMappedOriginAddressing(PId, modMsgData);
	auto targetProtoValid = !targetProtoSrc.first.empty();

	if (targetProtoValid && IsChangedDataValue(PId, roi, mappedOrigAddr, modMsgData))
	{
		// finally before forwarding data, the target channel has to be adjusted according to what we determined beforehand to be the correct mapped channel for target protocol
        modMsgData._addrVal._first = targetProtoSrc.second;
		auto overallSendSuccess = true;
		for (auto const& targetPId : targetProtoSrc.first)
		{
			if (msgMeta._ExternalId != targetPId || msgMeta._Category != RemoteObjectMessageMetaInfo::MC_SetMessageAcknowledgement)
			{
				// sending is only done when the value about to be sent is differing from the last known value from the protocol in question
				if (IsChangedDataValue(targetPId, roi, modMsgData._addrVal, modMsgData, false))
				{
					auto isTargetProtocolTypeA = (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), targetPId) != GetProtocolAIds().end());
					auto isAcknowledgingProtocol = isTargetProtocolTypeA ? IsTypeAAcknowledging() : IsTypeBAcknowledging();

					auto sendSuccess = parentNode->SendMessageTo(targetPId, roi, modMsgData);
					// If the value was sent successfully, save it to cache (to make it the 'last known' from this protocol).
					// In case the protocol is expected to acknowledge the value, we make an exception, since acknowledge values are
					// used to update bridged protocols that have not yet received that latest value. E.g. DS100 ack values that are a
					// reaction on a GenericOSC SET have to be bridged back to connected DiGiCo.
					if ((sendSuccess && !isAcknowledgingProtocol) || isGetValueQuery)
						SetCurrentValue(targetPId, roi, modMsgData._addrVal, modMsgData); // set the updated value as current for the complementary cache as well
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
 * Method to derive the protocol and source id the incoming data has to be sent to, in respect to the given multiplexing configuration values.
 *
 * @param PId		The id of the protocol that received the data
 * @param msgData	The actual message value/content data
 * @return			The protocol indexes the mapped value shall be sent to and the mapped source id to use when sending the data to the protocols
 */
std::pair<std::vector<ProtocolId>, ChannelId> Mux_nA_to_mB_withValFilter::GetTargetProtocolsAndSource(ProtocolId PId, const RemoteObjectMessageData &msgData)
{
	auto PIdAIter = std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId);
	auto PIdBIter = std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId);
	if (PIdAIter != GetProtocolAIds().end())
	{
		auto protocolAIndex = PIdAIter - GetProtocolAIds().begin();
		auto absChNr = static_cast<int>(protocolAIndex * (m_protoChCntA != INVALID_ADDRESS_VALUE ? m_protoChCntA : 0)) + msgData._addrVal._first;
		auto chForB = 0;
		auto protocolBIndex = 0;
		if (m_protoChCntB > 0)
		{
			chForB = static_cast<std::int32_t>(absChNr % m_protoChCntB);
			if (chForB == 0)
				chForB = m_protoChCntB;
			auto protocolBNr = static_cast<int>(((absChNr - 1) / m_protoChCntB) + 1.0f);
			protocolBIndex = protocolBNr - 1;
		}
		if (chForB == 0)
			chForB = static_cast<std::int32_t>(absChNr);

		if (m_protoChCntB == INVALID_ADDRESS_VALUE)
			// return all typeB protocols
			return std::make_pair(GetProtocolBIds(), chForB);
		else if (GetProtocolBIds().size() >= protocolBIndex + 1)
			// return the single typeB protocol the message from typeB can be demultiplexed to, combined with the determined channel for the typeB protocol
			return std::make_pair(std::vector<ProtocolId>{ GetProtocolBIds()[protocolBIndex] }, chForB);
		else
			// default no protocol id and mapped channel
			return std::make_pair(std::vector<ProtocolId>(), chForB);
	}
	else if (PIdBIter != GetProtocolBIds().end())
	{
		auto protocolBIndex = PIdBIter - GetProtocolBIds().begin();
		auto absChNr = static_cast<int>(protocolBIndex * (m_protoChCntB != INVALID_ADDRESS_VALUE ? m_protoChCntB : 0)) + msgData._addrVal._first;
		auto chForA = 0;
		auto protocolAIndex = 0;
		if (m_protoChCntA > 0)
		{
			chForA = static_cast<std::int32_t>(absChNr % m_protoChCntA);
			if (chForA == 0)
				chForA = m_protoChCntA;
			auto protocolANr = static_cast<int>(((absChNr - 1) / m_protoChCntA) + 1.0f);
			protocolAIndex = protocolANr - 1;
		}
		if (chForA == 0)
			chForA = static_cast<std::int32_t>(absChNr);

		if (m_protoChCntA == INVALID_ADDRESS_VALUE)
			// return all typeA protocols
			return std::make_pair(GetProtocolAIds(), chForA);
		else if (GetProtocolAIds().size() >= protocolAIndex + 1)
			// return the single typeA protocol the message from typeB can be demultiplexed to, combined with the determined channel for the typeA protocol
			return std::make_pair(std::vector<ProtocolId>{ GetProtocolAIds()[protocolAIndex] }, chForA);
		else
			// default no protocol id and mapped channel
			return std::make_pair(std::vector<ProtocolId>(), chForA);
	}
	else
		jassertfalse; // unknown incoming protocol id may not happen!

	return std::make_pair(std::vector<ProtocolId>(), static_cast<ChannelId>(INVALID_ADDRESS_VALUE));
}

/**
 * Method to get a mapped object addressing that represents the actual absolute object addressing without (de-)multiplexing offsets.
 *
 * @param PId		The id of the protocol that received the data
 * @param msgData	The actual message value/content data
 * @return	The protocol index the mapped value shall be sent to
 */
RemoteObjectAddressing Mux_nA_to_mB_withValFilter::GetMappedOriginAddressing(ProtocolId PId, const RemoteObjectMessageData& msgData)
{
	auto PIdAIter = std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId);
	auto PIdBIter = std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId);
	// if the protocol id is found to belong to a protocol of type A, handle it accordingly
	if (PIdAIter != GetProtocolAIds().end())
	{
		auto protocolAIndex = PIdAIter - GetProtocolAIds().begin();
		auto absChNr = static_cast<int>(protocolAIndex * (m_protoChCntA != INVALID_ADDRESS_VALUE ? m_protoChCntA : 0)) + msgData._addrVal._first;

		return RemoteObjectAddressing(absChNr, msgData._addrVal._second);
	}
	// otherwise if the protocol id is found to belong to a protocol of type B, handle it accordingly as well
	else if (PIdBIter != GetProtocolBIds().end())
	{
		auto protocolBIndex = PIdBIter - GetProtocolBIds().begin();
		auto absChNr = static_cast<int>(protocolBIndex * (m_protoChCntB != INVALID_ADDRESS_VALUE ? m_protoChCntB : 0)) + msgData._addrVal._first;

		return RemoteObjectAddressing(absChNr, msgData._addrVal._second);
	}
	// invalid return if the given protocol id is neither known as type a nor b
	else
	{
		return RemoteObjectAddressing();
	}
}
