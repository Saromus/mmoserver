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
#include "BankTerminal.h"
#include "CraftingTool.h"
#include "CurrentResource.h"
#include "Item.h"
#include "NPCObject.h"
#include "PlayerObject.h"
#include "ObjectController.h"
#include "ObjectControllerOpcodes.h"
#include "ObjectControllerCommandMap.h"
#include "Wearable.h"
#include "WorldConfig.h"
#include "WorldManager.h"
#include "TradeManager.h"
#include "TravelMapHandler.h"
#include "TreasuryManager.h"
#include "SurveyTool.h"
#include "UIManager.h"

#include "DatabaseManager/Database.h"
#include "DatabaseManager/DataBinding.h"
#include "DatabaseManager/DatabaseResult.h"
#include "Common/MessageFactory.h"
#include "Common/Message.h"
#include "MessageLib/MessageLib.h"
#include "LogManager/LogManager.h"

//======================================================================================================================
//
// trade invitation
//

void ObjectController::handleSecureTradeInvitation(uint64 targetId,Message* message)
{
	//targetId is the Id of the one who IS INVITING
	//receiverId is the Id of the one who GETS INVITED

	uint32 unknown,error;
	uint64 receiverId,unknown64;
	const uint32 error0 = 0;
	const uint32 error2 = 2;
	message->getUint32(unknown);
	message->getUint32(error);
	message->getUint64(unknown64);//sender (target) id
	message->getUint64(receiverId);

	PlayerObject*	invitingPlayer = dynamic_cast<PlayerObject*>(mObject);
	//PlayerObject*	invitingPlayer =
	PlayerObject*	invitedPlayer = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById(receiverId));

	switch(error)
	{
		case error2:
		{
			// snd invitation returns error as 2 and the id of the invited as 0 :(
			invitedPlayer = dynamic_cast<PlayerObject*>(invitingPlayer->getTarget());
			error = 0;
		}
		break;

		case error0:
		{
			//first invitation set the setInvite to the Inviter
			//invitedPlayer = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById(receiverId));
		}
		break;

		default:
		{
			// Always use a default if message damaged....
			gLogger->log(LogManager::DEBUG,"ObjController:: Error in trade invitation");
			// Since receiver is default NULL, we can use the error message below
			// return;
		}
	}

	if(!invitedPlayer)
	{
		gMessageLib->sendSystemMessage(invitingPlayer,L"","ui_trade","start_fail_target_not_player");
		return;
	}

	if(invitedPlayer->checkStatesEither(CreatureState_Combat | CreatureState_Tumbling | CreatureState_Swimming))
	{
		gMessageLib->sendSystemMessage(invitingPlayer,L"","error_message","wrong_state");
		return;
	}

	if(invitingPlayer->checkStatesEither(CreatureState_Combat | CreatureState_Tumbling | CreatureState_Swimming))
	{
		gMessageLib->sendSystemMessage(invitingPlayer,L"","error_message","wrong_state");
		return;
	}

	// Can NOT use bitwise operation on non bitwise constants. CreaturePostures are used exclusive.
	// if(invitedPlayer->checkPosturesEither(CreaturePosture_Dead | CreaturePosture_Incapacitated))
	if (invitedPlayer->checkPosture(CreaturePosture_Dead) || invitedPlayer->checkPosture(CreaturePosture_Incapacitated))
	{
		gMessageLib->sendSystemMessage(invitingPlayer,L"","error_message","wrong_state");
		return;
	}

	// Can NOT use bitwise operation on non bitwise constants.
	// if(invitingPlayer->checkPosturesEither(CreaturePosture_Dead | CreaturePosture_Incapacitated))
	if (invitingPlayer->checkPosture(CreaturePosture_Dead) || invitingPlayer->checkPosture(CreaturePosture_Incapacitated))
	{
		gMessageLib->sendSystemMessage(invitingPlayer,L"","error_message","wrong_state");
		return;
	}

	if (error == 0)
	{
		gLogger->log(LogManager::DEBUG,"ObjController:: %s invited %s to trade",invitingPlayer->getFirstName().getAnsi(),invitedPlayer->getFirstName().getAnsi());

		if (invitedPlayer->getTradeStatus() == false )
		{
			// If sender is invited by receiver already, then accept even if receiver have sender in the Ignore-list.
			if (!invitingPlayer->getTrade()->verifyInvitation(invitedPlayer))
			{
				// We are not invited, check Ignore.
				// If receiver have sender ignored, auto decline trade request.
				BString ignoreName = invitingPlayer->getFirstName();
				ignoreName.toLower();

				// check receivers ignorelist
				if (invitedPlayer->checkIgnoreList(ignoreName.getCrc()))
				{
					gMessageLib->sendSystemMessage(invitingPlayer,L"","ui_trade","request_player_denied_prose","","",L"",0,"","",L"",0,0,invitedPlayer->getId());
					return;
				}
			}
			gTradeManager->addTradeInvitation(invitedPlayer,invitingPlayer);
		}
		else
		{
			gMessageLib->sendSystemMessage(invitingPlayer,L"","ui_trade","request_player_busy_prose","","",L"",0,"","",L"",0,0,invitedPlayer->getId());
		}

	}
	else
	{
		gLogger->log(LogManager::DEBUG,"ObjController:: Error in trade invitation");
	}
}

//======================================================================================================================


//======================================================================================================================
//
// tip
//

void ObjectController::_handleTip(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{

	// Message can appear as follows:
	//
	// (uint32)playerID bank	<- banktip in case the client knows the target
	// (uint32)playerID			<- inventory tip in case the client knows the target
	// (string)playerName bank	<- banktip in case the client doesn't know the target
	//
	// can't inventory-tip someone out of range so we can assume
	// the client will send the ID rather than the name in case
	// of an inventory tip.
	BString attribute, str;
	message->getStringUnicode16(str);
	attribute = str;
	attribute.convert(BSTRType_ANSI);

	PlayerObject*	player		 = dynamic_cast<PlayerObject*>(mObject);
	PlayerObject*	target		 = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById(targetId));

	BString			targetName;
	BString			dataStr;
	//int32			amount		 = 0;
	//uint64			transferType = 0;
	BStringVector	dataElements;
	BString			bank;

	attribute.getRawData()[attribute.getLength()] = 0;

	uint32 elementCount = attribute.split(dataElements,' ');

	//do we have the right number of attributes?
	if((elementCount <1) || (elementCount >3))
	{
		gMessageLib->sendSystemMessage(player, L"","base_player","prose_tip_invalid_param","","",str.getUnicode16());
		return;
	}

	BString lower = dataElements[elementCount-1];

	// Have to convert BEFORE using toLower, since the conversion done there is removed It will assert().
	// Either do the conversion HERE, or better fix the toLower so it handles unicode also.
	dataStr.convert(BSTRType_ANSI);
	lower.toLower();

	//check for banktip
	if((lower.getCrc() == BString("bank").getCrc())&&(elementCount > 1))
	{
		uint32 amount	= atoi(dataElements[elementCount-2].getAnsi());
		bool havetarget = false;

		BString name;
		if(target && (target != player))
		{
			havetarget = true;
			name = target->getFirstName();
		}

		if(elementCount == 3)
		{
			havetarget = true;
			name = dataElements[0];
		}

		if((amount >0)&& (amount < 999999999)&&(havetarget))
		{
			//now call the treasury, find that offline bloke and get going
			gTreasuryManager->bankTipOffline(amount,player,name);
			return;
		}

		if(targetId && (!havetarget))
		{
			//please note that this is rather complex as we have a targetid even if we explicitly names a target
			gMessageLib->sendSystemMessage(player, L"You may only /tip or /tip bank to other players.");
			return;
		}
	}

	if(target == player)
	{
		gMessageLib->sendSystemMessage(player, L"You may only /tip or /tip bank to other players.");
		return;
	}

	if((elementCount == 1)&&(target))
	{
		uint32 amount	= atoi(dataElements[0].getAnsi());
		if(amount>0 && (amount < 1000001))
		{
			gTreasuryManager->inventoryTipOnline(amount,player,target);
			return;
		}
	}

	if(targetId && (!target))
	{
		//please note that this is rather complex as we have a targetid even if we explicitely name a target
		gMessageLib->sendSystemMessage(player, L"You may only /tip or /tip bank to other players.");
		return;
	}

	gMessageLib->sendSystemMessage(player, L"","base_player","prose_tip_invalid_param","","",str.getUnicode16());
	return;


}

