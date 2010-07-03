 /*
---------------------------------------------------------------------------------------
This source file is part of SWG:ANH (Star Wars Galaxies - A New Hope - Server Emulator)

For more information, visit http://www.swganh.com

Copyright (c) 2006 - 2010 The SWG:ANH Team
---------------------------------------------------------------------------------------
Use of this source code is governed by the GPL v3 license that can be found
in the COPYING file or at http://www.gnu.org/licenses/gpl-3.0.html

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
---------------------------------------------------------------------------------------
*/

#include "GroupManagerHandler.h"
#include "UIManager.h"
#include "WorldManager.h"
#include "PlayerObject.h"
#include "MessageLib/MessageLib.h"
#include "Common/MessageDispatch.h"
#include "Common/MessageFactory.h"
#include "Common/MessageOpcodes.h"
#include "Common/Message.h"
#include "Common/DispatchClient.h"
#include "DatabaseManager/Database.h"
#include "DatabaseManager/DataBinding.h"
#include "DatabaseManager/DatabaseResult.h"
#include "LogManager/LogManager.h"
#include "Utils/utils.h"

bool						GroupManagerHandler::mInsFlag    = false;
GroupManagerHandler*		GroupManagerHandler::mSingleton  = NULL;


//======================================================================================================================

GroupManagerHandler::GroupManagerHandler(Database* database, MessageDispatch* dispatch)
{
	mDatabase = database;
	mMessageDispatch = dispatch;

	mMessageDispatch->RegisterMessageCallback(opIsmGroupInviteRequest,std::bind(&GroupManagerHandler::_processIsmInviteRequest, this, std::placeholders::_1, std::placeholders::_2));
	mMessageDispatch->RegisterMessageCallback(opIsmGroupCREO6deltaGroupId,std::bind(&GroupManagerHandler::_processIsmGroupCREO6deltaGroupId, this, std::placeholders::_1, std::placeholders::_2));
	mMessageDispatch->RegisterMessageCallback(opIsmGroupLootModeResponse,std::bind(&GroupManagerHandler::_processIsmGroupLootModeResponse, this, std::placeholders::_1, std::placeholders::_2));
	mMessageDispatch->RegisterMessageCallback(opIsmGroupLootMasterResponse,std::bind(&GroupManagerHandler::_processIsmGroupLootMasterResponse, this, std::placeholders::_1, std::placeholders::_2));
}


//======================================================================================================================

GroupManagerHandler::~GroupManagerHandler()
{
	mInsFlag = false;
	delete(mSingleton);
}

//======================================================================================================================

GroupManagerHandler*	GroupManagerHandler::Init(Database* database, MessageDispatch* dispatch)
{
	if(!mInsFlag)
	{
		mSingleton = new GroupManagerHandler(database,dispatch);
		mInsFlag = true;
		return mSingleton;
	}
	else
		return mSingleton;

}

//======================================================================================================================

void GroupManagerHandler::Shutdown()
{
	mMessageDispatch->UnregisterMessageCallback(opIsmGroupInviteRequest);
	mMessageDispatch->UnregisterMessageCallback(opIsmGroupCREO6deltaGroupId);
	mMessageDispatch->UnregisterMessageCallback(opIsmGroupLootModeResponse);
	mMessageDispatch->UnregisterMessageCallback(opIsmGroupLootMasterResponse);
}

//=======================================================================================================================


void GroupManagerHandler::_processIsmInviteRequest(Message* message, DispatchClient* client)
{

	PlayerObject* sender = gWorldManager->getPlayerByAccId(message->getUint32()); // the player who sent the invite
	PlayerObject* target = gWorldManager->getPlayerByAccId(message->getUint32());  // the player who will recieve it

	if(sender == NULL || target == NULL)
	{
		gLogger->log(LogManager::DEBUG,"GroupManagerHandler::_processIsmInviteRequest PlayerAccId not found");
		return;
	}

	//target->setGroupId(message->getUint64()); // the group id provided by the chatserver

	gMessageLib->sendInviteSenderUpdateDeltasCreo6(sender->getId(),target);

}

//=======================================================================================================================

void GroupManagerHandler::_processIsmGroupCREO6deltaGroupId(Message* message, DispatchClient* client)
{
	// this packet is sent by the chatserver to update the group_id in the CREO6
	// it concerns the player itself and all the inrange players

	PlayerObject* const player = gWorldManager->getPlayerByAccId(message->getUint32());  // the player whos group_id has changed
	if(player == NULL)
	{
		gLogger->log(LogManager::DEBUG,"GroupManagerHandler::_processIsmGroupCREO6deltaGroupId PlayerAccId not found");
		return;
	}

	player->setGroupId(message->getUint64());


	// to in-range folks
	const PlayerObjectSet*	const inRangePlayers	= player->getKnownPlayers();
	PlayerObjectSet::const_iterator	it				= inRangePlayers->begin();

	while(it != inRangePlayers->end())
	{
		const PlayerObject* const targetObject = (*it);

		if(targetObject->isConnected())
		{
			gMessageLib->sendGroupIdUpdateDeltasCreo6(player->getGroupId(),player,targetObject);
		}

		++it;
	}

	// to self
	gMessageLib->sendGroupIdUpdateDeltasCreo6(player->getGroupId(), player, player);

}

//=======================================================================================================================

void GroupManagerHandler::_processIsmGroupLootModeResponse(Message* message, DispatchClient* client)
{
	gLogger->log(LogManager::DEBUG,"_processIsmGroupLootModeResponse");
	PlayerObject* playerObject = gWorldManager->getPlayerByAccId(message->getUint32());  // the player whos group_id has changed
	if(playerObject == NULL)
	{
		gLogger->log(LogManager::DEBUG,"GroupManagerHandler::processIsmGroupLootModeResponse PlayerAccId not found");
		return;
	}

	//send the SUI
	gLogger->log(LogManager::DEBUG,"ok");

	BStringVector availableLootModes;
	availableLootModes.push_back("Free for all");
	availableLootModes.push_back("Master Looter");
	availableLootModes.push_back("Lottery");
	availableLootModes.push_back("Random");

	gUIManager->createNewListBox(playerObject,"handleSetLootMode","@group:set_loot_type_title","@group:set_loot_type_text",availableLootModes,playerObject,SUI_Window_SelectGroupLootMode_Listbox);
}

//=======================================================================================================================

void GroupManagerHandler::_processIsmGroupLootMasterResponse(Message* message, DispatchClient* client)
{
	gLogger->log(LogManager::DEBUG,"_processIsmGroupLootMasterResponse");
	PlayerObject* playerObject = gWorldManager->getPlayerByAccId(message->getUint32());  // the player whos group_id has changed
	if(playerObject == NULL)
	{
		gLogger->log(LogManager::DEBUG,"GroupManagerHandler::_processIsmGroupLootMasterResponse PlayerAccId not found");
		return;
	}

	//send the SUI
	gLogger->log(LogManager::DEBUG,"ok");

	PlayerList inRangeMembers	= playerObject->getInRangeGroupMembers(true);
	PlayerList::iterator it		= inRangeMembers.begin();

	BStringVector namesArray;

	while(it != inRangeMembers.end())
	{
		namesArray.push_back((*it)->getFirstName().getAnsi());
		++it;
	}

	gUIManager->createNewPlayerSelectListBox(playerObject,"handleSetLootMaster","@group:master_looter_sui_title","@group:set_loot_type_text",namesArray,inRangeMembers,playerObject);
}

//=======================================================================================================================
