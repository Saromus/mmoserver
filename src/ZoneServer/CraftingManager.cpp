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
#include "CraftingManager.h"
#include "BankTerminal.h"
#include "CraftingSessionFactory.h"
#include "CraftingTool.h"
#include "CurrentResource.h"
#include "Item.h"
#include "ManufacturingSchematic.h"
#include "CraftingSession.h"
#include "NPCObject.h"
#include "ObjectController.h"
#include "ObjectControllerCommandMap.h"
#include "ObjectControllerOpcodes.h"
#include "PlayerObject.h"
#include "SchematicManager.h"
#include "SurveyTool.h"
#include "TravelMapHandler.h"
#include "UIManager.h"
#include "Wearable.h"
#include "WorldConfig.h"
#include "WorldManager.h"
#include "ZoneTree.h"
#include "MessageLib/MessageLib.h"
#include "LogManager/LogManager.h"
#include "DatabaseManager/Database.h"
#include "DatabaseManager/DatabaseResult.h"
#include "DatabaseManager/DataBinding.h"
#include "Common/Message.h"
#include "Common/MessageFactory.h"
#include "Utils/clock.h"

#include <boost/lexical_cast.hpp>

bool						CraftingManager::mInsFlag    = false;
CraftingManager*			CraftingManager::mSingleton  = NULL;

CraftingManager::CraftingManager(Database* database) : mDatabase(database)
{
	mSI	= gWorldManager->getSI();
}


CraftingManager::~CraftingManager(void)
{
}

//=============================================================================
//
// request draftslots batch
//

bool CraftingManager::HandleRequestDraftslotsBatch(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*	playerObject	= dynamic_cast<PlayerObject*>(object);
	BString			requestStr;
	BStringVector	dataElements;
	uint16			elementCount;

	message->getStringUnicode16(requestStr);
	requestStr.convert(BSTRType_ANSI);

	elementCount = requestStr.split(dataElements,' ');

	if(!elementCount)
	{
		gLogger->log(LogManager::DEBUG,"ObjectController::_handleRequestDraftslotsBatch: Error in requestStr");
		return false;
	}

	for(uint16 i = 1;i < elementCount;i += 2)
	{
		// since we currently store everything in 1 schematic object, just look up by the crc
		// lookup of weights is done in requestresourceweightsbatch
		uint64 itemId = boost::lexical_cast<uint64>(dataElements[i].getAnsi());
		DraftSchematic* schematic = gSchematicManager->getSchematicBySlotId(static_cast<uint32>(itemId));

		if(schematic)
		{
			gMessageLib->sendDraftslotsResponse(schematic,playerObject);
		}
	}
	return true;
}

//======================================================================================================================
//
// request resource weights batch
//

bool CraftingManager::HandleRequestResourceWeightsBatch(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*	playerObject	= dynamic_cast<PlayerObject*>(object);
	BString			requestStr;
	BStringVector	dataElements;
	uint16			elementCount;

	message->getStringUnicode16(requestStr);
	requestStr.convert(BSTRType_ANSI);

	elementCount = requestStr.split(dataElements,' ');

	if(!elementCount)
	{
		gLogger->log(LogManager::DEBUG,"ObjectController::_handleRequestResourceWeightsBatch: Error in requestStr");
		return false;
	}

	for(uint16 i = 0;i < elementCount;i++)
	{
		uint64 itemId = boost::lexical_cast<uint64>(dataElements[i].getAnsi());
		DraftSchematic* schematic = gSchematicManager->getSchematicByWeightId(static_cast<uint32>(itemId));

		if(schematic)
		{
			gMessageLib->sendDraftWeightsResponse(schematic,playerObject);
		}
	}
	return true;
}

//======================================================================================================================
//
// synchronized ui listen
//

bool CraftingManager::HandleSynchronizedUIListen(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	return true;
}

//======================================================================================================================
//
// request crafting session
//

bool CraftingManager::HandleRequestCraftingSession(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*		playerObject	= dynamic_cast<PlayerObject*>(object);
	CraftingTool*		tool			= dynamic_cast<CraftingTool*>(target);
	CraftingStation*	station			= NULL;
	uint32				expFlag			= 2;//needs to be >1 !!!!!

	message->setIndex(24);
	/*uint32				counter			= */
	message->getUint32();

	//get nearest crafting station
	ObjectSet			inRangeObjects;
	float				range = 25.0;

	if(!tool)
	{
		gLogger->log(LogManager::DEBUG,"ObjController::handleRequestcraftingsession: could not find tool %"PRIu64"",target->getId());
		gMessageLib->sendCraftAcknowledge(opCraftCancelResponse,0,0,playerObject);
		return false;
	}


	// get the tangible objects in range
	gCraftingManager->mSI->getObjectsInRange(playerObject,&inRangeObjects,(ObjType_Tangible),range);

	//and see if a fitting crafting station is near
	station = playerObject->getCraftingStation(&inRangeObjects,(ItemType) tool->getItemType());

	if(!station)
	{
		expFlag = false;
	}

	if(playerObject->isDead() || playerObject->isIncapacitated())
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("error_message", "wrong_state"), playerObject);
		return false;
	}

	if(playerObject->getPerformingState() != PlayerPerformance_None)
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("error_message", "wrong_state"), playerObject);
		return false;
	}

	if(playerObject->checkState(CreatureState_Crafting) || playerObject->getCraftingSession())
	{
		gLogger->log(LogManager::DEBUG,"ObjController::handleRequestcraftingsession: state or session");
		gMessageLib->sendCraftAcknowledge(opCraftCancelResponse,0,0,playerObject);
		return false;
	}

	if(tool->getAttribute<std::string>("craft_tool_status") == "@crafting:tool_status_working")
	{
		if(tool->getCurrentItem())
            gMessageLib->SendSystemMessage(::common::OutOfBand("system_msg", "crafting_tool_creating_prototype"), playerObject);

		// TODO: put the right message for practice
		else
            gMessageLib->SendSystemMessage(::common::OutOfBand("system_msg", "crafting_tool_creating_prototype"), playerObject);

		gMessageLib->sendCraftAcknowledge(opCraftCancelResponse,0,0,playerObject);

		return false;
	}

	gLogger->log(LogManager::DEBUG,"ObjController::handleRequestcraftingsession: new session :)");
	playerObject->setCraftingSession(gCraftingSessionFactory->createSession(Anh_Utils::Clock::getSingleton(),playerObject,tool,station,expFlag));
	return true;
}

//======================================================================================================================
//
// select draft schematic
//

bool CraftingManager::HandleSelectDraftSchematic(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*		playerObject	= dynamic_cast<PlayerObject*>(object);
	CraftingSession*	session			= playerObject->getCraftingSession();
	//DraftSchematic*		schematic		= NULL;
	BString				dataStr;
	uint32				schematicIndex	= 0;

	message->getStringUnicode16(dataStr);

	if(session)
	{
		if(swscanf(dataStr.getUnicode16(),L"%u",&schematicIndex) != 1 || !session->selectDraftSchematic(schematicIndex))
		{
			gCraftingSessionFactory->destroySession(session);
		}
	}
	return true;
}

//======================================================================================================================
//
// cancel crafting session
//

bool CraftingManager::HandleCancelCraftingSession(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*	playerObject	= dynamic_cast<PlayerObject*>(object);

	message->setIndex(24);

	/*uint32			counter			= */message->getUint32();

	gCraftingSessionFactory->destroySession(playerObject->getCraftingSession());

	gLogger->log(LogManager::DEBUG,"session canceled");
	//client complains over crafting tool already hacing an item when we go out of the slot screen!!!!!
	return true;
}

//=============================================================================================================================
//
// craft fill slot
//

void CraftingManager::handleCraftFillSlot(Object* object,Message* message)
{
	PlayerObject*		player		= dynamic_cast<PlayerObject*>(object);
	CraftingSession*	session		= player->getCraftingSession();

	uint64				resContainerId	= message->getUint64();
	uint32				slotId			= message->getUint32();
	uint32				unknownId		= message->getUint32();
	uint8				counter			= message->getUint8();

	// make sure we have a valid session and are in the assembly stage
	if(session && player->getCraftingStage() == 2)
	{
		session->handleFillSlot(resContainerId,slotId,unknownId,counter);
	}
	// it failed
	else
	{
		gMessageLib->sendCraftAcknowledge(opCraftFillSlot,CraftError_Not_In_Assembly_Stage,counter,player);
	}
}

//=============================================================================================================================
//
// craft empty slot
//

void CraftingManager::handleCraftEmptySlot(Object* object,Message* message)
{
	PlayerObject*		player		= dynamic_cast<PlayerObject*>(object);
	CraftingSession*	session		= player->getCraftingSession();
	uint32				slotId		= message->getUint32();
	uint64				containerId	= message->getUint64();
	uint8				counter		= message->getUint8();

	// make sure we have a valid session and are in the assembly stage
	if(session && player->getCraftingStage() == 2)
	{
		session->handleEmptySlot(slotId,containerId,counter);
	}
	// it failed
	else
	{
		gMessageLib->sendCraftAcknowledge(opCraftEmptySlot,CraftError_Not_In_Assembly_Stage,counter,player);
	}
}

//=============================================================================================================================
//
// craft experiment
//

void CraftingManager::handleCraftExperiment(Object* object, Message* message)
{
	PlayerObject*		player		= dynamic_cast<PlayerObject*>(object);
	CraftingSession*	session		= player->getCraftingSession();
	uint8				counter		= message->getUint8();
	uint32				propCount	= message->getUint32();
	std::vector<std::pair<uint32,uint32> >	properties;

	if(!session || player->getCraftingStage() != 3)
		return;

	for(uint32 i = 0;i < propCount;i++)
		properties.push_back(std::make_pair(message->getUint32(),message->getUint32()));

	session->experiment(counter,properties);
}

//=============================================================================================================================
//
// customization
//

void CraftingManager::handleCraftCustomization(Object* object,Message* message)
{
	PlayerObject*		player		= dynamic_cast<PlayerObject*>(object);
	CraftingSession*	session		= player->getCraftingSession();
	BString				itemName;
	uint8				hmmm1,hmmm2;
	uint32				amount,color;

	if(!session)
		return;
	player->setCraftingStage(4);

	message->getStringUnicode16(itemName);
	itemName.convert(BSTRType_ANSI);

	message->getUint8(hmmm1);

	message->getUint32(amount);
	message->getUint8(hmmm2);

	CustomizationList* cList;

	cList = session->getManufacturingSchematic()->getCustomizationList();
	CustomizationList::iterator	custIt = cList->begin();

	uint32 i = 0;
	while((custIt != cList->end())&&(i < hmmm2))
	{
		message->getUint32(color);
		gLogger->log(LogManager::DEBUG,"craft customization int1 : %u",color);
		message->getUint32(color);
		gLogger->log(LogManager::DEBUG,"craft customization int2 : %u at index : %u",color,(*custIt)->cutomizationIndex);
		session->getItem()->setCustomization(static_cast<uint8>((*custIt)->cutomizationIndex),(uint16)color,3);

		i++;
		++custIt;
	}


	int8 sql[550];
	sprintf(sql,"INSERT INTO item_customization VALUES(%"PRIu64", %u, %u)",session->getItem()->getId(),session->getItem()->getCustomization(1),session->getItem()->getCustomization(2));
	mDatabase->ExecuteSqlAsync(0,0,sql);

	session->setProductionAmount(amount);
	session->customize(itemName.getAnsi());
}

//=============================================================================================================================
//
// nextcraftingstage
//

bool CraftingManager::HandleNextCraftingStage(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*		playerObject	= dynamic_cast<PlayerObject*>(object);
	CraftingSession*	session			= playerObject->getCraftingSession();
	BString				dataStr;
	uint32				counter			= 1;

	if(!session)
		return false;

	message->getStringUnicode16(dataStr);

	if(dataStr.getLength() == 0)
	{
		//Command Line Entry
		counter = session->getCounter();
	}
	else
	{
		uint32 resultCount = swscanf(dataStr.getUnicode16(),L"%u",&counter);
		if(resultCount != 1)
		{
			gCraftingSessionFactory->destroySession(session);
			return false;
		}
		gLogger->log(LogManager::DEBUG,"Counter We Got: %u", counter);
		gLogger->log(LogManager::DEBUG,"Counter We'd Use: %u", session->getCounter());
	}

	switch(session->getStage())
	{
		case 1:
		{
			//Player's Macro is wrong! :p
		}
		break;

		case 2:
		{
			session->assemble(counter);
		}
		break;

		case 3:
		{
			session->experimentationStage(counter);

		}
		break;

		case 4:
		{
			session->customizationStage(counter);
			//session->creationStage(counter);
		}
		break;

		case 5:
		{
			session->creationStage(counter);
		}
		break;

		default:
		{
			gLogger->log(LogManager::NOTICE,"ObjController::_handlenextcraftingstage: unhandled stage %u",session->getStage());
		}
		break;
	}
	return true;
}

//=============================================================================================================================
//
// createprototype
//

bool CraftingManager::HandleCreatePrototype(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*		player	= dynamic_cast<PlayerObject*>(object);
	CraftingSession*	session	= player->getCraftingSession();
	BString				dataStr;
	uint32				mode,counter;

	if(!session)
		return false;

	message->getStringUnicode16(dataStr);

	if(swscanf(dataStr.getUnicode16(),L"%u %u",&counter,&mode) != 2)
	{
		gCraftingSessionFactory->destroySession(player->getCraftingSession());
		return false;
	}

	session->createPrototype(mode,counter);
	return true;
}

//=============================================================================================================================
//
// create manufacture schematic
//

bool CraftingManager::HandleCreateManufactureSchematic(Object* object,Object* target,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*		player	= dynamic_cast<PlayerObject*>(object);
	CraftingSession*	session	= player->getCraftingSession();
	BString				dataStr;
	uint32				counter;

	if(!session)
		return false;

	message->getStringUnicode16(dataStr);

	if(swscanf(dataStr.getUnicode16(),L"%u",&counter) != 1)
	{
		gCraftingSessionFactory->destroySession(player->getCraftingSession());
		return false;
	}

	//gLogger->hexDump(message->getData(),message->getSize());
	session->createManufactureSchematic(counter);
	return true;
}

//======================================================================================================================

