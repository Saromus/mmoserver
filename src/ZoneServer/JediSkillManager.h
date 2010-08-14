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

#pragma once //same thing as #include guards

#include <vector>


#define gJediSkillManager JediSkillManager::getSingletonPtr()

class CreatureObject;
class Database;
class Message;
class MessageDispatch;
class ObjectControllerCommandMap;
class ObjectControllerCmdProperties;
class PlayerObject;

class JediSkillManager
{
public:
	~JediSkillManager();

	static JediSkillManager* getSingletonPtr() { return mSingleton; }
	static JediSkillManager* Init(MessageDispatch* dispatch)
	{
		if(!mInsFlag)
		{
			mSingleton = new JediSkillManager(dispatch);
			mInsFlag = true;
			return mSingleton;
		}
		else
			return mSingleton;
	}

	//ForceHealSelfSkill
	bool ForceHealSelfDamage(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int HealType);
	bool ForceHealSelfWound(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int HealType);
	bool ForceHealSelfState(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties);
	bool ForceHealSelfTotal(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties);

	//ForceHealTargetSkill
	bool ForceHealTargetDamage(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType);
	bool ForceHealTargetWound(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType);
	bool ForceHealTargetState(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties);
	bool ForceCureTarget(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType);
	bool ForceHealTargetTotal(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties);
	
	//Force Enhancement
	bool ForceMeditateSelfSkill(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int ForceRegen);
	bool ForceRunSelfSkill(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int SkillLevel);
	
	//Target Skills
	bool TransferForce(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties);

private:
	static JediSkillManager*	mSingleton;
	static bool					mInsFlag;
	MessageDispatch*			Dispatch;

	JediSkillManager(MessageDispatch* dispatch);

};