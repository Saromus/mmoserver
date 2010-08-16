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

#include "PlayerObject.h"
#include "WorldConfig.h"
#include "ObjectController.h"
#include "ObjectControllerOpcodes.h"
#include "ObjectControllerCommandMap.h"
#include "MessageLib/MessageLib.h"
#include "LogManager/LogManager.h"
#include "DatabaseManager/Database.h"
#include "DatabaseManager/DataBinding.h"
#include "DatabaseManager/DatabaseResult.h"
#include "Common/MessageFactory.h"
#include "Common/Message.h"

//=============================================================================================================================
//
// meditate
//

void ObjectController::_handleMeditate(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject* player = dynamic_cast<PlayerObject*>(mObject);

	if (player->isMeditating())
	{
		gMessageLib->SendSystemMessage(::common::OutOfBand("jedi_spam", "already_in_meditative_state"), player);
		return;
	}

	// Begin Meditating
	gMessageLib->SendSystemMessage(::common::OutOfBand("teraskasi", "med_begin"), player);
	player->setMeditateState();

	// Schedule Execution
	addEvent(new MeditateEvent(3500), 3500);
}

//=============================================================================================================================
//
// powerboost
//

void ObjectController::_handlePowerBoost(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================
//
// forceofwill
//

void ObjectController::_handleForceOfWill(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
}

//=============================================================================================================================


