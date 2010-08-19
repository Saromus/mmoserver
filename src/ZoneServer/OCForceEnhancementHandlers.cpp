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
#include <cstdint>
#ifdef _MSC_VER
#include <regex> // NOLINT
#else
#endif

#include <boost/regex.hpp> // NOLINT
#include "JediSkillManager.h"
#include "ObjectController.h"
#include "ObjectControllerOpcodes.h"
#include "ObjectControllerCommandMap.h"
#include "PlayerObject.h"
#include "WorldManager.h"
#include "MessageLib/MessageLib.h"
#include "Common/LogManager.h"
#include "DatabaseManager/Database.h"
#include "DatabaseManager/DataBinding.h"
#include "DatabaseManager/DatabaseResult.h"
#include "NetworkManager/Message.h"
#include "NetworkManager/MessageFactory.h"




//=============================================================================================================================
//
// forceabsorb 1
//

void ObjectController::_handleForceAbsorb1(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forceabsorb 2
//

void ObjectController::_handleForceAbsorb2(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forcespeed 1
//

void ObjectController::_handleForceSpeed1(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forcespeed 2
//

void ObjectController::_handleForceSpeed2(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forcerun 1
//

void ObjectController::_handleForceRun1(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject* Jedi = dynamic_cast<PlayerObject*>(mObject);

	int SkillLevel = 1;

	mHandlerCompleted = gJediSkillManager->ForceRunSelfSkill(Jedi, cmdProperties, SkillLevel);
}

//=============================================================================================================================
//
// forcerun 2
//

void ObjectController::_handleForceRun2(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject* Jedi = dynamic_cast<PlayerObject*>(mObject);

	int SkillLevel = 2;

	mHandlerCompleted = gJediSkillManager->ForceRunSelfSkill(Jedi, cmdProperties, SkillLevel);
}

//=============================================================================================================================
//
// forcerun 3
//

void ObjectController::_handleForceRun3(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject* Jedi = dynamic_cast<PlayerObject*>(mObject);

	int SkillLevel = 3;

	mHandlerCompleted = gJediSkillManager->ForceRunSelfSkill(Jedi, cmdProperties, SkillLevel);
}

//=============================================================================================================================
//
// forceFeedback 1
//

void ObjectController::_handleForceFeedback1(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forceFeedback 2
//

void ObjectController::_handleForceFeedback2(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forcearmor 1
//

void ObjectController::_handleForceArmor1(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forcearmor 2
//

void ObjectController::_handleForceArmor2(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forceresistbleeding
//

void ObjectController::_handleForceResistBleeding(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forceresistdisease
//

void ObjectController::_handleForceResistDisease(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forceresistpoison
//

void ObjectController::_handleForceResistPoison(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forceresiststates
//

void ObjectController::_handleForceResistStates(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// transferforce
//

void ObjectController::_handleTransferForce(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject* Jedi = dynamic_cast<PlayerObject*>(mObject);
	PlayerObject* Target = dynamic_cast<PlayerObject*>(Jedi->getHealingTarget(Jedi));

	mHandlerCompleted = gJediSkillManager->TransferForce(Jedi, Target, cmdProperties);
}

//=============================================================================================================================
//
// channelforce
//

void ObjectController::_handleChannelForce(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// drainforce
//

void ObjectController::_handleDrainForce(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject* Jedi = dynamic_cast<PlayerObject*>(mObject);
	PlayerObject* Target = dynamic_cast<PlayerObject*>(Jedi->getHealingTarget(Jedi));

	mHandlerCompleted = gJediSkillManager->DrainForce(Jedi, Target, cmdProperties);
}

//=============================================================================================================================
//
// forceshield 1
//

void ObjectController::_handleForceShield1(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forceshield 2
//

void ObjectController::_handleForceShield2(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forcemeditate
//

void ObjectController::_handleForceMeditate(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{

	PlayerObject* Jedi = dynamic_cast<PlayerObject*>(mObject);

	int ForceRegen = 300; //TODO: the HamRegens should be multiplied by 3

	mHandlerCompleted = gJediSkillManager->ForceMeditateSelfSkill(Jedi, cmdProperties, ForceRegen);
}

//=============================================================================================================================
//
// regainconsciousness
//

void ObjectController::_handleRegainConsciousness(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
