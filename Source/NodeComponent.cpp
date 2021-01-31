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

#include "NodeComponent.h"

#include "MainComponent.h"
#include "ProtocolComponent.h"
#include "ConfigComponents/ObjectHandlingConfigComponents.h"

#include "ProcessingEngine.h"
#include "ProcessingEngineConfig.h"


// **************************************************************************************
//    class NodeComponent
// **************************************************************************************
/**
 * Constructor
 */
NodeComponent::NodeComponent(NodeId NId)
	: GroupComponent()
{
	GroupComponent::setColour(outlineColourId, Colours::white);

    m_NodeId = NId;

	m_protocolsAComponent = std::make_unique<ProtocolGroupComponent>();
	m_protocolsAComponent->AddListener(this);
	addAndMakeVisible(m_protocolsAComponent.get());
	m_protocolsAComponent->setText("Role A");

	m_protocolsBComponent = std::make_unique<ProtocolGroupComponent>();
	m_protocolsBComponent->AddListener(this);
	addAndMakeVisible(m_protocolsBComponent.get());
	m_protocolsBComponent->setText("Role B");

	m_NodeModeDrop = std::make_unique<ComboBox>();
	m_NodeModeDrop->addListener(this);
	addAndMakeVisible(m_NodeModeDrop.get());
	m_NodeModeDrop->addItem(ProcessingEngineConfig::ObjectHandlingModeToString(OHM_Bypass), OHM_Bypass);
	m_NodeModeDrop->addItem(ProcessingEngineConfig::ObjectHandlingModeToString(OHM_Remap_A_X_Y_to_B_XY), OHM_Remap_A_X_Y_to_B_XY);
	m_NodeModeDrop->addItem(ProcessingEngineConfig::ObjectHandlingModeToString(OHM_Mux_nA_to_mB), OHM_Mux_nA_to_mB);
	m_NodeModeDrop->addItem(ProcessingEngineConfig::ObjectHandlingModeToString(OHM_Forward_only_valueChanges), OHM_Forward_only_valueChanges);
	m_NodeModeDrop->setColour(Label::textColourId, Colours::white);
	m_NodeModeDrop->setJustificationType(Justification::right);

	m_NodeModeLabel = std::make_unique<Label>();
	addAndMakeVisible(m_NodeModeLabel.get());
	m_NodeModeLabel->setText("Data handling", dontSendNotification);
	m_NodeModeLabel->setColour(Label::textColourId, Colours::white);
	m_NodeModeLabel->setJustificationType(Justification::right);
	m_NodeModeLabel->attachToComponent(m_NodeModeDrop.get(), true);
	
	m_OHMConfigEditButton = std::make_unique<TextButton>();
	m_OHMConfigEditButton->addListener(this);
	addAndMakeVisible(m_OHMConfigEditButton.get());
	m_OHMConfigEditButton->setButtonText("Configuration");
	m_OHMConfigEditButton->setColour(TextButton::buttonColourId, Colours::dimgrey);
	m_OHMConfigEditButton->setColour(Label::textColourId, Colours::white);

	m_OHMConfigDialog = 0;
}

/**
 * Destructor
 */
NodeComponent::~NodeComponent()
{
}

/**
 * Overloaded method to resize contents
 */
void NodeComponent::resized()
{
	GroupComponent::resized();

	int windowWidth = getWidth();
	int windowHeight = getHeight();

	/*Config, TrafficLogging, Start/Stop Buttons*/
	int yPositionModeDrop = windowHeight - UIS_ElmSize - UIS_Margin_m;
	if (m_NodeModeDrop)
		m_NodeModeDrop->setBounds(UIS_AttachedLabelWidth, yPositionModeDrop, windowWidth - UIS_NodeModeDropWidthOffset - UIS_ConfigButtonWidth - UIS_Margin_m, UIS_ElmSize);
	m_OHMConfigEditButton->setBounds(windowWidth - UIS_ConfigButtonWidth - UIS_Margin_m - UIS_Margin_s, yPositionModeDrop, UIS_ConfigButtonWidth, UIS_ElmSize);

	/*gather data for dynamic sizing of protocols*/
	Array<ProtocolId> PAIds = m_protocolsAComponent->GetProtocolIds();
	int protocolsACount = PAIds.size()+1;
	Array<ProtocolId> PBIds = m_protocolsBComponent->GetProtocolIds();
	int protocolsBCount = PBIds.size()+1;
	int absProtocolCount = protocolsACount + protocolsBCount;
	int protocolsAreaHeight = yPositionModeDrop - 2*UIS_Margin_m;

	/*Dynamically sized protocol components*/
	int protocolsAHeight = absProtocolCount > 0 ? (protocolsAreaHeight / absProtocolCount)*protocolsACount : 0;
	if (m_protocolsAComponent)
		m_protocolsAComponent->setBounds(UIS_Margin_m, UIS_Margin_m + UIS_Margin_s, windowWidth - 2 * UIS_Margin_m, protocolsAHeight);

	int protocolsBHeight = absProtocolCount > 0 ? (protocolsAreaHeight / absProtocolCount)*protocolsBCount : 0;
	if (m_protocolsBComponent)
		m_protocolsBComponent->setBounds(UIS_Margin_m, protocolsAHeight + UIS_Margin_m + UIS_Margin_s, windowWidth - 2 * UIS_Margin_m, protocolsBHeight);
}

/**
 * Method to be called by child windows when closed, to enshure
 * button states and the internal
 * window object is invalidated to avoid accessviolation
 *
 * @param childWindow	The DialogWindow object that has been triggered to close
 */
void NodeComponent::childWindowCloseTriggered(DialogWindow* childWindow)
{
	ProcessingEngineConfig* config = GetConfig();
	ProcessingEngine* engine = GetEngine();

	if (childWindow == m_OHMConfigDialog.get())
	{
		if (config && engine)
		{
			bool EngineIsRunning = engine->IsRunning();
			if (EngineIsRunning)
				engine->Stop();

			m_OHMConfigDialog->DumpConfig(*config);
			config->WriteConfiguration();
			engine->SetConfig(*config);

			if (EngineIsRunning)
				engine->Start();
		}

		if (m_OHMConfigDialog != 0 && m_OHMConfigEditButton)
		{
			m_OHMConfigEditButton->setColour(TextButton::buttonColourId, Colours::dimgrey);
			m_OHMConfigEditButton->setColour(Label::textColourId, Colours::white);
		}

		m_OHMConfigDialog.reset();
	}
}

/**
 * Method to gather data from ui input elements and dump them to given configuration object.
 *
 * @param config	The configuration object to be filled with contents from ui elements of this node
 */
void NodeComponent::DumpUItoConfig(ProcessingEngineConfig& config)
{
	m_protocolsAComponent->DumpUItoConfig(config);
	m_protocolsBComponent->DumpUItoConfig(config);
    
    // get the mode the node should run in from ui
    ObjectHandlingMode ohMode = OHM_Invalid;
    if (m_NodeModeDrop)
        ohMode = (ObjectHandlingMode)m_NodeModeDrop->getSelectedId();

    ProcessingEngineConfig::NodeData node = config.GetNodeData(m_NodeId);
    node.Id = m_NodeId;
    node.ObjectHandling.Mode = ohMode;
    config.SetNode(node.Id, node);
}

/**
 * Method to update ui input elements with data to show from config
 *
 * @param config	The configuration object to extract the data to show on ui from
 * @return	The calculated theoretically required size for this component
 */
int NodeComponent::RefreshUIfromConfig(const ProcessingEngineConfig& config)
{
    ProcessingEngineConfig::NodeData node = config.GetNodeData(m_NodeId);

	int requiredHeight = 0;
    
	requiredHeight += m_protocolsAComponent->RefreshUIfromConfig(m_NodeId, node.RoleAProtocols, config);
	requiredHeight += m_protocolsBComponent->RefreshUIfromConfig(m_NodeId, node.RoleBProtocols, config);
	requiredHeight += UIS_Margin_s;

    m_NodeModeDrop->setSelectedId(node.ObjectHandling.Mode, dontSendNotification);
	requiredHeight += UIS_ElmSize + UIS_Margin_m;

	return requiredHeight;
}

/**
 * Method to allow access to internal config.
 *
 * @return	Reference to the internal config object
 */
ProcessingEngineConfig* NodeComponent::GetConfig()
{
	if (m_parentComponent)
		return m_parentComponent->GetConfig();
	else
	{
		jassert(false);
		return 0;
	}
}

/**
 * Method to allow access to internal engine.
 *
 * @return	Reference to the internal engine object
 */
ProcessingEngine* NodeComponent::GetEngine()
{
	if (m_parentComponent)
		return m_parentComponent->GetEngine();
	else
	{
		jassert(false);
		return 0;
	}
}

/**
 * Getter for this nodes' NodeId.
 *
 * @return	This nodes' NodeID
 */
NodeId NodeComponent::GetNodeId()
{
	return m_NodeId;
}

/**
 * Method to add the parent listener to this instance of NodeComponent.
 * This can afterwards be used for e.g. callbacks, etc.
 *
 * @param listener	The parent listener object to be used to invoke public methods from ('callback')
 */
void NodeComponent::AddListener(MainComponent* listener)
{
    m_parentComponent = listener;
}

/**
 * Overloaded method called by button objects on click events.
 * All internal button objects are registered to trigger this by calling
 * their ::addListener method with this object as argument
 *
 * @param button	The button object that has been clicked
 */
void NodeComponent::buttonClicked(Button* button)
{
	if (button == m_OHMConfigEditButton.get())
	{
		ToggleOpenCloseObjectHandlingConfig(m_OHMConfigEditButton.get());
	}
	
	TriggerParentConfigDump();
}

/**
 * Overloaded method called by ComboBox objects on change events.
 * This is similar to ::buttonClicked but originates from inherited ComboBox::Listener
 *
 * @param comboBox	The comboBox object that has been changed
 */
void NodeComponent::comboBoxChanged(ComboBox* comboBox)
{
	ignoreUnused(comboBox);

	TriggerParentConfigDump();
}

/**
 * Overloaded method called by TextEditor objects on textchange events.
 * This is similar to ::buttonClicked but originates from inherited TextEditor::Listener
 *
 * @param textEdit	The textEdit object that has been changed
 */
void NodeComponent::textEditorTextChanged(TextEditor& textEdit)
{
	ignoreUnused(textEdit);

	TriggerParentConfigDump();
}

/**
 * Overloaded method called by TextEditor objects on keypress events.
 * This is similar to ::buttonClicked but originates from inherited TextEditor::Listener
 *
 * @param textEdit	The textEdit object that has received keypress
 */
void NodeComponent::textEditorReturnKeyPressed(TextEditor& textEdit)
{
	ignoreUnused(textEdit);

	TriggerParentConfigDump();
}

/**
 * Overloaded method called by TextEditor objects on keypress events.
 * This is similar to ::buttonClicked but originates from inherited TextEditor::Listener
 *
 * @param textEdit	The textEdit object that has received keypress
 */
void NodeComponent::textEditorEscapeKeyPressed(TextEditor& textEdit)
{
	ignoreUnused(textEdit);
}

/**
 * Overloaded method called by TextEditor objects on focuslost events.
 * This is similar to ::buttonClicked but originates from inherited TextEditor::Listener
 *
 * @param textEdit	The textEdit object that has lost focus
 */
void NodeComponent::textEditorFocusLost(TextEditor& textEdit)
{
	ignoreUnused(textEdit);
}

/**
 * Proxy method to forward triggering parent to start dumping to config
 */
void NodeComponent::TriggerParentConfigDump()
{
	if (m_parentComponent)
		m_parentComponent->DumpUItoConfig();
}

/**
 * Proxy method to forward triggering parent to start refreshing from config
 */
void NodeComponent::TriggerParentConfigRefresh()
{
	if (m_parentComponent)
		m_parentComponent->RefreshUIfromConfig();
}

/**
 * Helper method to be called by child protocolgroupcomponent for adding a new default protocol.
 * Detecting if new protocol shall be added as A or B type is done through comparing the sender
 * object pointer to internally kept member objects.
 *
 * @param targetPGC	The sending protocol group component object
 * @return	True on success, false on failure
 */
bool NodeComponent::AddDefaultProtocol(const ProtocolGroupComponent* targetPGC)
{
	if (targetPGC == m_protocolsAComponent.get())
		return AddDefaultProtocolA();
	else if (targetPGC == m_protocolsBComponent.get())
		return AddDefaultProtocolB();
	else
		return false;
}

/**
 *
 */
bool NodeComponent::AddDefaultProtocolA()
{
	ProcessingEngine* engine = GetEngine();
	ProcessingEngineConfig* config = GetConfig();

	if (engine && config)
	{
		bool EngineIsRunning = engine->IsRunning();
		if (EngineIsRunning)
			engine->Stop();

		config->AddDefaultProtocolA(m_NodeId);
		config->WriteConfiguration();
		engine->SetConfig(*config);

		TriggerParentConfigRefresh();

		if (EngineIsRunning)
			engine->Start();

		return true;
	}
	else
		return false;
}

/**
 *
 */
bool NodeComponent::AddDefaultProtocolB()
{
	ProcessingEngine* engine = GetEngine();
	ProcessingEngineConfig* config = GetConfig();

	if (engine && config)
	{
		bool EngineIsRunning = engine->IsRunning();
		if (EngineIsRunning)
			engine->Stop();

		config->AddDefaultProtocolB(m_NodeId);
		config->WriteConfiguration();
		engine->SetConfig(*config);

		TriggerParentConfigRefresh();

		if (EngineIsRunning)
			engine->Start();

		return true;
	}
	else
		return false;
}

/**
 * Method to remove a given nodes' protocol with specified is
 *
 * @param NId	The node the protocol is part of
 * @param PId	The protocol to remove
 * @return	True on success false on failure
 */
bool NodeComponent::RemoveProtocol(const NodeId& NId, const ProtocolId& PId)
{
	ProcessingEngine* engine = GetEngine();
	ProcessingEngineConfig* config = GetConfig();

	if (engine && config)
	{
		bool EngineIsRunning = engine->IsRunning();
		if (EngineIsRunning)
			engine->Stop();

		config->RemoveProtocol(NId, PId);
		config->WriteConfiguration();
		engine->SetConfig(*config);

		TriggerParentConfigRefresh();

		if (EngineIsRunning)
			engine->Start();

		return true;
	}
	else
		return false;
}

/**
 * Helper method to do the toggling of config dialog on respective buttonClicked
 *
 * @param button	The button object that has been clicked
 */
void NodeComponent::ToggleOpenCloseObjectHandlingConfig(Button* button)
{
	jassert(button);
	if (!m_parentComponent || !button)
		return;

	ProcessingEngineConfig* config = m_parentComponent->GetConfig();
	ProcessingEngine* engine = m_parentComponent->GetEngine();

	// if the config dialog exists, this is a uncheck (close) click,
	// which means we have to process edited data
	if (m_OHMConfigDialog)
	{
		if (config && engine)
		{
			bool EngineIsRunning = engine->IsRunning();
			if (EngineIsRunning)
				engine->Stop();

			m_OHMConfigDialog->DumpConfig(*config);
			config->WriteConfiguration();
			engine->SetConfig(*config);

			if (EngineIsRunning)
				engine->Start();
		}

		button->setColour(TextButton::buttonColourId, Colours::dimgrey);
		button->setColour(Label::textColourId, Colours::white);

		m_OHMConfigDialog.reset();
	}
	// otherwise we have to create the dialog and show it
	else
	{
		if (config && engine)
		{
			ObjectHandlingMode ohMode = config->GetObjectHandlingData(m_NodeId).Mode;

			String dialogTitle = ProcessingEngineConfig::ObjectHandlingModeToString(ohMode) + " obj. handling configuration (Node Id" + String(m_NodeId) + ")";

			m_OHMConfigDialog = std::make_unique<ObjectHandlingConfigWindow>(dialogTitle, Colours::dimgrey, false, m_NodeId, ohMode);
			m_OHMConfigDialog->AddListener(this);
			m_OHMConfigDialog->setResizable(true, true);
			m_OHMConfigDialog->setUsingNativeTitleBar(true);
			m_OHMConfigDialog->setVisible(true);
			m_OHMConfigDialog->SetConfig(*config);
#if defined JUCE_IOS ||  defined JUCE_ANDROID
            m_OHMConfigDialog->setFullScreen(true);
#else
			const std::pair<int, int> size = m_OHMConfigDialog->GetSuggestedSize();
			m_OHMConfigDialog->setResizeLimits(size.first, size.second, size.first, size.second);
			m_OHMConfigDialog->setBounds(Rectangle<int>(getScreenBounds().getX(), getScreenBounds().getY(), size.first, size.second));
#endif
			button->setColour(TextButton::buttonColourId, Colours::lightblue);
			button->setColour(Label::textColourId, Colours::dimgrey);
		}
	}
}
