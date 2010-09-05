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

#include "BuildingObject.h"
#include "PlayerObject.h"
#include "CellObject.h"
#include "MessageLib/MessageLib.h"
#include "SpawnPoint.h"

#include "Utils/rand.h"

//=============================================================================

BuildingObject::BuildingObject() : PlayerStructure()
{
    mType = ObjType_Building;
    mMaxStorage = 0;

}

//=============================================================================

BuildingObject::~BuildingObject()
{
    SpawnPoints::iterator it = mSpawnPoints.begin();

    while(it != mSpawnPoints.end())
    {
        delete(*it);

        it = mSpawnPoints.erase(it);
    }
    //destroy the cells
    CellObjectList::iterator cellIt = mCells.begin();

    while(cellIt != mCells.end())
    {
        gWorldManager->destroyObject((*cellIt));
        //cellIt++;
        cellIt = mCells.erase(cellIt);
    }

}

//=============================================================================



//=============================================================================

SpawnPoint* BuildingObject::getRandomSpawnPoint()
{
    if(mSpawnPoints.size())
    {
        return(mSpawnPoints[gRandom->getRand()%mSpawnPoints.size()]);
    }

    return(NULL);
}

//=============================================================================

bool BuildingObject::removeCell(CellObject* cellObject)
{
    CellObjectList::iterator it = mCells.begin();

    while(it != mCells.end())
    {
        if((*it) == cellObject)
        {
            mCells.erase(it);
            return(true);
        }
        ++it;
    }
    return(false);
}

//=============================================================================

bool BuildingObject::checkForCell(CellObject* cellObject)
{
    CellObjectList::iterator it = mCells.begin();

    while(it != mCells.end())
    {
        if((*it) == cellObject)
            return(true);
        ++it;
    }
    return(false);
}

//=============================================================================
//gets the count of all tangibles in a cell
//how to not count the structure terminal ????
// - dont count tangibles marked as ststic :)
uint16 BuildingObject::getCellContentCount()
{
    uint16 count = 0;
    CellObjectList::iterator it = mCells.begin();

    while(it != mCells.end())
    {
        count += (*it)->getHeadCount();

        ++it;
    }
    return(count);
}


ObjectList BuildingObject::getAllCellChilds()
{
    ObjectIDList*	tmpList;
    ObjectList	resultList;
    ObjectIDList::iterator childIt;

    CellObjectList::iterator cellIt = mCells.begin();

    while(cellIt != mCells.end())
    {
        tmpList = (*cellIt)->getObjects();
        childIt = tmpList->begin();

        while(childIt != tmpList->end())
        {
            Object* childObject = gWorldManager->getObjectById((*childIt));
            resultList.push_back(childObject);
            ++childIt;
        }
        ++cellIt;
    }
    return(resultList);
}

//================================================================================
//
//the cells send an updated permission  to the specified player
//
void BuildingObject::updateCellPermissions(PlayerObject* player, bool access)
{
    //iterate through all the cells - do they need to be deleted ?
    //place players inside a cell in the world
    CellObjectList*				cellList	= getCellList();
    CellObjectList::iterator	cellIt		= cellList->begin();

    while(cellIt != cellList->end())
    {
        CellObject* cell = (*cellIt);

        gMessageLib->sendUpdateCellPermissionMessage(cell,access,player);

        //are we inside the cell ?
        if((player->getParentId() == cell->getId()) && (!access))
        {
            //were no longer allowed to be inside ....
            //so get going

            //TODO find outside position (sign position for example??) to place the player


            glm::vec3 playerPosition = player->mPosition;
            glm::vec3 playerWorldPosition = player->getWorldPosition();
            glm::vec3 position;

            glm::vec3 playerNewPosition;
            position.x = (0 - playerPosition.x) - 5;
            position.z = (0 - playerPosition.z) - 2;

            position.y = mPosition.y + 50;

            position.x += playerWorldPosition.x;
            //position.y += player->getWorldPosition.x;
            position.z += playerWorldPosition.z;

            player->updatePosition(0,position);


        }

        ++cellIt;
    }

}
