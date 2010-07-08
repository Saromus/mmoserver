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

#include "Holocron.h"
#include "Inventory.h"
#include "ObjectFactory.h"
#include "PlayerObject.h"
#include "WorldManager.h"
#include "MessageLib/MessageLib.h"

//=============================================================================

Holocron::Holocron() : Item()
{
}

//=============================================================================

Holocron::~Holocron()
{
}

//=============================================================================
//handles the radial selection

void Holocron::handleObjectMenuSelect(uint8 messageType, Object* srcObject)
{
	if (PlayerObject* player = dynamic_cast<PlayerObject*>(srcObject))
	{
		switch(messageType)
		{
		case radId_itemUse:
			{
				//Check if player already has a full force bar.
				if (player->getHam()->getCurrentForce() == player->getHam()->getMaxForce())
				{
					gMessageLib->sendSystemMessage(player, L"", "jedi_spam", "holocron_force_max");
					return;
				}
				else
				{
					//Replenish Force Power
					//Subtract Max Force from Current Force in order to get the amount of force that needs to be replenished in order to achieve max force again.
					int fp = player->getHam()->getMaxForce() - player->getHam()->getCurrentForce();
					player->getHam()->updateCurrentForce(fp);

					//Send Client Message
					gMessageLib->sendSystemMessage(player, L"", "jedi_spam", "holocron_force_replenish");

					//Now that we have used the holocron, we need to delete it from the inventory since you can't use the same holocron twice.
					Inventory* inventory = dynamic_cast<Inventory*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Inventory));
					inventory->removeObject(this);
					gMessageLib->sendDestroyObject(this->getId(), player);
					gObjectFactory->deleteObjectFromDB(this);
					gWorldManager->destroyObject(this);
				}
			}
		}
	}
}

//=============================================================================

void Holocron::prepareCustomRadialMenu(CreatureObject* creatureObject, uint8 itemCount)
{
	RadialMenu* radial = new RadialMenu();
	radial->addItem(1, 0, radId_itemUse, radAction_ObjCallback, "");
	radial->addItem(2, 0, radId_examine, radAction_ObjCallback, "");
	radial->addItem(3, 0, radId_itemDestroy, radAction_ObjCallback, "");
	RadialMenuPtr radialPtr(radial);
	mRadialMenu = radialPtr;
}