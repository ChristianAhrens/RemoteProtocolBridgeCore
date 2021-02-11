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
	if (!ObjectDataHandling_Abstract::setStateXml(stateXml))
		return false;

	if (stateXml->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MODE)) != ProcessingEngineConfig::ObjectHandlingModeToString(OHM_Mux_nA_to_mB_withValFilter))
		return false;

	auto precisionXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::DATAPRECISION));
	if (precisionXmlElement)
		SetPrecision(precisionXmlElement->getAllSubText().getFloatValue());
	else
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
 * @param Id		The object id to send a message for
 * @param msgData	The actual message value/content data
 * @return	True if successful sent/forwarded, false if not
 */
bool Mux_nA_to_mB_withValFilter::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	// a valid parent node is required to be able to do anything with the received message
	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	// do some sanity checks on this instances configuration parameters and the given message data origin id
	auto muxConfigValid = (m_protoChCntA > 0) && (m_protoChCntB > 0);
	auto protocolIdValid = (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) != GetProtocolAIds().end()) || (std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId) != GetProtocolBIds().end());

	if (!muxConfigValid || !protocolIdValid)
		return false;

	// check for changed value based on mapped addressing and target protocol id before forwarding data
	auto targetProtoSrc = GetTargetProtocolAndSource(PId, msgData);
	auto mappedOrigAddr = GetMappedOriginAddressing(PId, msgData);
	auto targetProtoValid = targetProtoSrc.first != INVALID_ADDRESS_VALUE;

	if (targetProtoValid && IsChangedDataValue(Id, mappedOrigAddr, msgData))
	{
		// finally before forwarding data, the target channel has to be adjusted according to what we determined beforehand to be the correct mapped channel for target protocol
		msgData._addrVal._first = targetProtoSrc.second;
		return parentNode->SendMessageTo(targetProtoSrc.first, Id, msgData);
	}
	else
		return false;
}

/**
 * Method to derive the protocol and source id the incoming data has to be sent to, in respect to the given multiplexing configuration values.
 *
 * @param PId		The id of the protocol that received the data
 * @param msgData	The actual message value/content data
 * @return	The protocol index the mapped value shall be sent to
 */
std::pair<ProtocolId, SourceId> Mux_nA_to_mB_withValFilter::GetTargetProtocolAndSource(ProtocolId PId, const RemoteObjectMessageData &msgData)
{
	auto PIdAIter = std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId);
	auto PIdBIter = std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId);
	if (PIdAIter != GetProtocolAIds().end())
	{
		jassert(msgData._addrVal._first <= m_protoChCntA);
		auto protocolAIndex = PIdAIter - GetProtocolAIds().begin();
		auto absChNr		= static_cast<int>(protocolAIndex * m_protoChCntA) + msgData._addrVal._first;
		auto protocolBIndex = absChNr / (m_protoChCntB + 1);
		auto chForB	   = static_cast<std::int32_t>(absChNr % m_protoChCntB);
		if(chForB == 0)
			chForB = static_cast<std::int32_t>(m_protoChCntB);

		if(GetProtocolBIds().size() >= protocolBIndex + 1)
			return std::make_pair(GetProtocolBIds()[protocolBIndex], chForB);
		else
			return std::make_pair(static_cast<ProtocolId>(INVALID_ADDRESS_VALUE), chForB);
	}
	else if (PIdBIter != GetProtocolBIds().end())
	{
		jassert(msgData._addrVal._first <= m_protoChCntB);
		auto protocolBIndex = PIdBIter - GetProtocolBIds().begin();
		auto absChNr		= static_cast<int>(protocolBIndex * m_protoChCntB) + msgData._addrVal._first;
		auto protocolAIndex = absChNr / (m_protoChCntA + 1);
		auto chForA	   = static_cast<std::int32_t>(absChNr % m_protoChCntA);
		if(chForA == 0)
			chForA = static_cast<std::int32_t>(m_protoChCntA);

		if(GetProtocolAIds().size() >= protocolAIndex + 1)
			return std::make_pair(GetProtocolAIds()[protocolAIndex], chForA);
		else
			return std::make_pair(static_cast<ProtocolId>(INVALID_ADDRESS_VALUE), chForA);
	}

	return std::make_pair(static_cast<ProtocolId>(INVALID_ADDRESS_VALUE), static_cast<SourceId>(INVALID_ADDRESS_VALUE));
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
		jassert(msgData._addrVal._first <= m_protoChCntA);
		auto protocolAIndex = PIdAIter - GetProtocolAIds().begin();
		auto absChNr = static_cast<int>(protocolAIndex * m_protoChCntA) + msgData._addrVal._first;

		return RemoteObjectAddressing(absChNr, msgData._addrVal._second);
	}
	// otherwise if the protocol id is found to belong to a protocol of type B, handle it accordingly as well
	else if (PIdBIter != GetProtocolBIds().end())
	{
		jassert(msgData._addrVal._first <= m_protoChCntB);
		auto protocolBIndex = PIdBIter - GetProtocolBIds().begin();
		auto absChNr = static_cast<int>(protocolBIndex * m_protoChCntB) + msgData._addrVal._first;

		return RemoteObjectAddressing(absChNr, msgData._addrVal._second);
	}
	// invalid return if the given protocol id is neither known as type a nor b
	else
	{
		return RemoteObjectAddressing();
	}
}