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
#include "CurrentResource.h"
#include "Item.h"
#include "ObjectController.h"
#include "ObjectControllerOpcodes.h"
#include "ObjectControllerCommandMap.h"
#include "PlayerObject.h"
#include "ResourceManager.h"
#include "ResourceType.h"
#include "SurveyTool.h"
#include "UIManager.h"
#include "Heightmap.h"
#include "WorldConfig.h"
#include "WorldManager.h"
#include "MessageLib/MessageLib.h"
#include "LogManager/LogManager.h"
#include "DatabaseManager/Database.h"
#include "DatabaseManager/DataBinding.h"
#include "DatabaseManager/DatabaseResult.h"
#include "Common/atMacroString.h"
#include "Common/Message.h"
#include "Common/MessageFactory.h"
#include "Utils/clock.h"

#include "ArtisanHeightmapAsyncContainer.h"

//======================================================================================================================
//
// request survey
//

void ObjectController::_handleRequestSurvey(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*		playerObject = dynamic_cast<PlayerObject*>(mObject);

	// don't allow survey in buildings
	if(playerObject->getParentId())
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("error_message", "survey_in_structure"), playerObject);
		return;
	}

	if(playerObject->isDead() || !playerObject->getHam()->checkMainPools(1,1,1))
		return;
		

	if(playerObject->getPerformingState() != PlayerPerformance_None)
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("error_message", "wrong_state"), playerObject);
		return;
	}

	if(playerObject->getSurveyState())
	{
		playerObject->getSampleData()->mPendingSample = false;
        gMessageLib->SendSystemMessage(::common::OutOfBand("survey", "survey_sample"), playerObject);
	}

	SurveyTool*			tool			= dynamic_cast<SurveyTool*>(gWorldManager->getObjectById(targetId));
	CurrentResource*	resource		= NULL;
	BString				resourceName;

	message->getStringUnicode16(resourceName);
	resourceName.convert(BSTRType_ANSI);

	resource = reinterpret_cast<CurrentResource*>(gResourceManager->getResourceByNameCRC(resourceName.getCrc()));

	if(tool && resource)
	{
		playerObject->setSurveyState(true);

		// play effect
		BString effect = gWorldManager->getClientEffect(tool->getInternalAttribute<uint32>("survey_effect"));

		gMessageLib->sendPlayClientEffectLocMessage(effect,playerObject->mPosition,playerObject);

		PlayerObjectSet*			playerList	= playerObject->getKnownPlayers();
		PlayerObjectSet::iterator	it			= playerList->begin();

		while(it != playerList->end())
		{
			gMessageLib->sendPlayClientEffectLocMessage(effect,playerObject->mPosition,(*it));

			++it;
		}

		// send system message
		resourceName.convert(BSTRType_Unicode16);
        gMessageLib->SendSystemMessage(::common::OutOfBand("survey", "start_survey", L"", L"", resourceName.getUnicode16()), playerObject);

		// schedule execution
		addEvent(new SurveyEvent(tool,resource),5000);
	}
}

//======================================================================================================================
//
// request sample
//

void ObjectController::_handleRequestCoreSample(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*		playerObject = dynamic_cast<PlayerObject*>(mObject);


	if(playerObject->checkIfMounted())
	{
		gMessageLib->SendSystemMessage(L"You cannot take resource samples while mounted.", playerObject);
		return;
	}

	if(playerObject->getPerformingState() != PlayerPerformance_None)
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("error_message", "wrong_state"), playerObject);
		return;
	}

	// don't allow sampling in buildings
	if(playerObject->getParentId())
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("error_message", "survey_in_structure"), playerObject);
		return;
	}

	if(playerObject->getPerformingState() != PlayerPerformance_None)
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("error_message", "sample_cancel"), playerObject);
		return;
	}

	uint64 localTime = Anh_Utils::Clock::getSingleton()->getLocalTime();
	if(!playerObject->getNextSampleTime() || (int32)(playerObject->getNextSampleTime() - localTime) <= 0)
	{
		playerObject->setNextSampleTime(localTime + 30000);
	}
	else
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("survey", "tool_recharge_time", 0, 0, 0, static_cast<int32>(playerObject->getNextSampleTime() - localTime) / 1000), playerObject);
		return;
	}

	SurveyTool*			tool		= dynamic_cast<SurveyTool*>(gWorldManager->getObjectById(targetId));
	CurrentResource*	resource	= NULL;

	BString resourceName;

	message->getStringUnicode16(resourceName);
	resourceName.convert(BSTRType_ANSI);

	resource = reinterpret_cast<CurrentResource*>(gResourceManager->getResourceByNameCRC(resourceName.getCrc()));

	if(resource == NULL || tool == NULL)
		return;

	if((resource->getType()->getCategoryId() == 903)||(resource->getType()->getCategoryId() == 904))
	{
        gMessageLib->SendSystemMessage(::common::OutOfBand("survey", "must_have_harvester"), playerObject);
		return;
	}

	ArtisanHeightmapAsyncContainer* container = new ArtisanHeightmapAsyncContainer(this, HeightmapCallback_ArtisanSurvey);
	container->addToBatch(playerObject->mPosition.x,playerObject->mPosition.z);

	container->playerObject = playerObject;
	container->resource = resource;
	container->resourceName = resourceName;
	container->tool = tool;

	gHeightmap->addNewHeightMapJob(container);

}

void ObjectController::HeightmapArtisanHandler(HeightmapAsyncContainer* ref)
{
		ArtisanHeightmapAsyncContainer* container = static_cast<ArtisanHeightmapAsyncContainer*>(ref);

		HeightResultMap* mapping = container->getResults();
		HeightResultMap::iterator it = mapping->begin();
		if(it != mapping->end() && it->second != NULL)
		{
			if(it->second->hasWater)
			{
                gMessageLib->SendSystemMessage(::common::OutOfBand("error_message", "survey_swimming"), container->playerObject);
				return;
			}

			// put us into sampling mode
			container->playerObject->setSamplingState(true);

			container->resourceName.convert(BSTRType_Unicode16);
            gMessageLib->SendSystemMessage(::common::OutOfBand("survey", "start_sampling", L"", L"", container->resourceName.getUnicode16()), container->playerObject);

			// change posture
			_handleKneel(0,NULL,NULL);

			// schedule execution
			addEvent(new SampleEvent(container->tool,container->resource),8000);
		}
}

//======================================================================================================================
//
// sent along with requestsurvey, when doing /survey command, contains nothing, not sure what its used for
//
void ObjectController::_handleSurvey(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	//PlayerObject*		playerObject = (PlayerObject*)mObject;

	//gLogger->hexDump(message->getData(),message->getSize());
}

//======================================================================================================================
//
// sent along with requestcoresample, when doing /sample command, contains nothing, not sure what its used for
//
void ObjectController::_handleSample(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	//PlayerObject*		playerObject = (PlayerObject*)mObject;

	//gLogger->hexDump(message->getData(),message->getSize());
}

//======================================================================================================================



