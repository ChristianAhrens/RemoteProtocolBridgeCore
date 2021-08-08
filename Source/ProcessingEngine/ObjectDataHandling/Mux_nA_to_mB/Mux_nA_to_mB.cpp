/* Copyright (c) 2020-2021, Christian Ahrens
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

#include "Mux_nA_to_mB.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class Mux_nA_to_mB
// **************************************************************************************
/**
 * Constructor of class Mux_nA_to_mB.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
Mux_nA_to_mB::Mux_nA_to_mB(ProcessingEngineNode* parentNode)
	: ObjectDataHandling_Abstract(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_Mux_nA_to_mB);
	m_protoChCntA	= 1;
	m_protoChCntB	= 1;
}

/**
 * Destructor
 */
Mux_nA_to_mB::~Mux_nA_to_mB()
{
}

/**
 * Reimplemented to set the custom parts from configuration for the datahandling object.
 *
 * @param config	The overall configuration object that can be used to query config data from
 * @param NId		The node id of the parent node this data handling object is child of (needed to access data from config)
 */
bool Mux_nA_to_mB::setStateXml(XmlElement* stateXml)
{
	if (!ObjectDataHandling_Abstract::setStateXml(stateXml))
		return false;

	if (stateXml->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MODE)) != ProcessingEngineConfig::ObjectHandlingModeToString(OHM_Mux_nA_to_mB))
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
bool Mux_nA_to_mB::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	UpdateOnlineState(PId);

	if (m_protoChCntA>0 && m_protoChCntB>0)
	{
		auto PIdAIter = std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId);
		auto PIdBIter = std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId);
		if (PIdAIter != GetProtocolAIds().end())
		{
			jassert(msgData._addrVal._first <= m_protoChCntA);
			auto protocolAIndex = PIdAIter - GetProtocolAIds().begin();
			auto absChNr = static_cast<int>(protocolAIndex * m_protoChCntA) + msgData._addrVal._first;
			auto protocolBIndex = absChNr / (m_protoChCntB + 1);
			auto chForB = static_cast<std::int32_t>(absChNr % m_protoChCntB);
			if (chForB == 0)
				chForB = static_cast<std::int32_t>(m_protoChCntB);

			msgData._addrVal._first = chForB;
			if (GetProtocolBIds().size() >= protocolBIndex + 1)
				return parentNode->SendMessageTo(GetProtocolBIds()[protocolBIndex], Id, msgData);
		}
		else if (PIdBIter != GetProtocolBIds().end())
		{
			jassert(msgData._addrVal._first <= m_protoChCntB);
			auto protocolBIndex = PIdBIter - GetProtocolBIds().begin();
			auto absChNr = static_cast<int>(protocolBIndex * m_protoChCntB) + msgData._addrVal._first;
			auto protocolAIndex = absChNr / (m_protoChCntA + 1);
			auto chForA = static_cast<std::int32_t>(absChNr % m_protoChCntA);
			if (chForA == 0)
				chForA = static_cast<std::int32_t>(m_protoChCntA);

			msgData._addrVal._first = chForA;
			if (GetProtocolAIds().size() >= protocolAIndex + 1)
				return parentNode->SendMessageTo(GetProtocolAIds()[protocolAIndex], Id, msgData);
		}
	}

	return false;
}
