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

#include "JediSkillManager.h"
#include "ObjectControllerCommandMap.h"
#include "ObjectControllerOpcodes.h"
#include "PlayerObject.h"
#include "Buff.h"
#include "WorldConfig.h"

#include "MessageLib/MessageLib.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif


bool				JediSkillManager::mInsFlag = false;
JediSkillManager*	JediSkillManager::mSingleton = NULL;

//constants
const char* const self		= "self";
const char* const action	= "action";
const char* const health	= "health";
const char* const mind		= "mind";

JediSkillManager::JediSkillManager(MessageDispatch* dispatch)
{
	Dispatch = dispatch;
}

JediSkillManager::~JediSkillManager()
{
}

//Used for Force Healing self damage pools.
bool JediSkillManager::ForceHealSelfDamage(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	//Check if you need healing
	int Health = Jedi->getHam()->mHealth.getCurrentHitPoints();
	int Action = Jedi->getHam()->mAction.getCurrentHitPoints();
	int Mind = Jedi->getHam()->mMind.getCurrentHitPoints();
	int MaxHealth = Jedi->getHam()->mHealth.getMaxHitPoints();
	int MaxAction = Jedi->getHam()->mAction.getMaxHitPoints();
	int MaxMind = Jedi->getHam()->mMind.getMaxHitPoints();

	if (Health == MaxHealth)
	{
		if (Action == MaxAction)
		{
			if (Mind == MaxMind)
			{
				gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_damage_heal_self");
				return false;
			}
		}
	}

	/*
	 * The following is a switch that, depending on the HealType set in the opcode handler, will do different heal commands.
	 * Procedure: Heal damage, calculation of how much was healed, send system messages, calculation of
	 * the force cost based upon the max amount retrieved from Pre-CU archives and points healed for.
	 */
	
	//Need to define constant ints like this so the compiler doesn't freak...
	//Might need to re-do this.

	//All type heal ints
	int H = Health - MaxHealth;
	int A = Action - MaxAction;
	int M = Mind - MaxMind;
	int cost1 = min((H + A + M / 3), 340); //int cost1 = min(((H + A + M) / 3), 340);
	int cost2 = min((H + A + M / 3), 470); //int cost2 = min(((H + A + M) / 3), 470);

	//Individual pool type heal ints
	int costH1 = min(H, 65);
	int costH2 = min(H, 100);
	int costA1 = min(A, 65);
	int costA2 = min(A, 100);
	int costM1 = min(M, 65);
	int costM2 = min(M, 100);


	switch (HealType)
	{
	case 1: //Heal All Self 1
		Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 500);
		Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 500);
		Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_damage", L"", -H);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_damage", L"", -A);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_damage", L"", -M);
		Jedi->getHam()->updateCurrentForce(-cost1);
		break;
	case 2: //Heal All Self 2
		Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 1500);
		Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 1500);
		Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 1500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_damage", L"", -H);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_damage", L"", -A);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_damage", L"", -M);
		Jedi->getHam()->updateCurrentForce(-cost2);
		break;
	case 3: //Heal Health Self 1
		Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_damage", L"", -H);
		Jedi->getHam()->updateCurrentForce(-costH1);
		break;
	case 4: //Heal Health Self 2
		Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 1500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_damage", L"", -H);
		Jedi->getHam()->updateCurrentForce(-costH2);
		break;
	case 5: //Heal Action Self 1
		Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_damage", L"", -A);
		Jedi->getHam()->updateCurrentForce(-costA1);
		break;
	case 6: //Heal Action Self 2
		Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 1500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_damage", L"", -A);
		Jedi->getHam()->updateCurrentForce(-costA2);
		break;
	case 7: //Heal Mind Self 1
		Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_damage", L"", -M);
		Jedi->getHam()->updateCurrentForce(-costM1);
		break;
	case 8: //Heal Mind Self 2
		Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 1500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_damage", L"", -M);
		Jedi->getHam()->updateCurrentForce(-costM2);
		break;
	default:
		break;
	}

	//Client Effect
	//gMessageLib->sendPlayClientEffectLocMessage("clienteffect/pl_force_heal_self.cef", Jedi->mPosition, Jedi);
	gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_healing.cef", "", Jedi);
	return true;
}

bool JediSkillManager::ForceHealSelfWound(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	int HealthWounds = Jedi->getHam()->mHealth.getWounds();
	int ActionWounds = Jedi->getHam()->mAction.getWounds();
	int MindWounds = Jedi->getHam()->mMind.getWounds();
	int BattleFatigue = Jedi->getHam()->getBattleFatigue();

	if (HealthWounds <= 0)
	{
		if (ActionWounds <= 0)
		{
			if (MindWounds <= 0)
			{
				if (BattleFatigue <= 0)
				{
					gMessageLib->sendSystemMessage(Jedi, L"You have no wounds of that type to heal.");
					return false;
				}
			}
		}
	}

	//The following all have temporary force costs (50 & 100) until the actual values are found.
	int costHW1 = min(HealthWounds, 50);
	int costHW2 = min(HealthWounds, 100);
	int costAW1 = min(ActionWounds, 50);
	int costAW2 = min(ActionWounds, 100);
	int costMW1 = min(MindWounds, 50);
	int costMW2 = min(MindWounds, 100);
	int costBF1 = min(BattleFatigue, 50);
	int costBF2 = min(BattleFatigue, 100);

	switch (HealType)
	{
	case 1: //Heal Health Wound Self 1
		Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -50); //250
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_wounds", L"", -HealthWounds);
		Jedi->getHam()->updateCurrentForce(-costHW1);
		break;
	case 2: //Heal Health Wound Self 2
		Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -100); //don't know actual value
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_wounds", L"", -HealthWounds);
		Jedi->getHam()->updateCurrentForce(-costHW2);
		break;
	case 3: //Heal Action Wound Self 1
		Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -50); //250
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_wounds", L"", -ActionWounds);
		Jedi->getHam()->updateCurrentForce(-costAW1);
		break;
	case 4: //Heal Action Wound Self 2
		Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -100); //don't know actual value
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_wounds", L"", -ActionWounds);
		Jedi->getHam()->updateCurrentForce(-costAW2);
		break;
	case 5: //Heal Mind Wound Self 1
		Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -50); //250
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_wounds", L"", -MindWounds);
		Jedi->getHam()->updateCurrentForce(-costMW1);
		break;
	case 6: //Heal Mind Wound Self 2
		Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -100); //don't know actual value
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_wounds", L"", -MindWounds);
		Jedi->getHam()->updateCurrentForce(-costMW2);
		break;
	case 7: //Heal Battle Fatigue Self 1
		Jedi->getHam()->updateBattleFatigue(-50); //-150
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "battle_fatigue", L"", -BattleFatigue);
		Jedi->getHam()->updateCurrentForce(-costBF1);
		break;
	case 8: //Heal Battle Fatigue Self 2
		Jedi->getHam()->updateBattleFatigue(-100); //don't know actual value
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "battle_fatigue", L"", -BattleFatigue);
		Jedi->getHam()->updateCurrentForce(-costBF2);
		break;
	default:
		break;
	}

	//Client Effect
	//gMessageLib->sendPlayClientEffectLocMessage("clienteffect/pl_force_healing.cef", Jedi->mPosition, Jedi);
	gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_healing.cef", "", Jedi);
	return true;
}

bool JediSkillManager::ForceHealSelfState(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties)
{
	int forceCost = 50;

	//Perform Checks
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	if (!Jedi->checkState(CreatureState_Stunned))
	{
		if (!Jedi->checkState(CreatureState_Blinded))
		{
			if (!Jedi->checkState(CreatureState_Dizzy))
			{
				if (!Jedi->checkState(CreatureState_Intimidated))
				{
					gMessageLib->sendSystemMessage(Jedi, L"You have no states of that type to heal.");
					return false;
				}
			}
		}
	}

	if (Jedi->getHam()->getCurrentForce() < forceCost)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_force_power");
		return false;
	}

	//If player has the state, then clear it.
	if (Jedi->checkState(CreatureState_Stunned))
	{
		Jedi->toggleStateOff(CreatureState_Stunned);
		gMessageLib->sendSystemMessage(Jedi, L"", "cbt_spam", "no_stunned_single");
		gMessageLib->sendFlyText(Jedi, "combat_effects", "no_stunned", 255, 0, 0);
	}
	if (Jedi->checkState(CreatureState_Blinded))
	{
		Jedi->toggleStateOff(CreatureState_Blinded);
		gMessageLib->sendSystemMessage(Jedi, L"", "cbt_spam", "no_blind_single");
		gMessageLib->sendFlyText(Jedi, "combat_effects", "no_blind", 255, 0, 0);
	}
	if (Jedi->checkState(CreatureState_Dizzy))
	{
		Jedi->toggleStateOff(CreatureState_Dizzy);
		gMessageLib->sendSystemMessage(Jedi, L"", "cbt_spam", "no_dizzy_single");
		gMessageLib->sendFlyText(Jedi, "combat_effects", "no_dizzy", 255, 0, 0);
	}
	if (Jedi->checkState(CreatureState_Intimidated))
	{
		Jedi->toggleStateOff(CreatureState_Intimidated);
		gMessageLib->sendFlyText(Jedi, "combat_effects", "no_intimidated", 255, 0, 0);
	}
	
	gMessageLib->sendStateUpdate(Jedi);

	Jedi->getHam()->updateCurrentForce(-forceCost);

	gMessageLib->sendSystemMessage(Jedi, L"You cure all negative states on yourself.");
	return true;
}

//This heals damage and wounds on the player
bool JediSkillManager::ForceHealSelfTotal(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties)
{
	int forceCost = 400;

	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	if (Jedi->getHam()->getCurrentForce() < forceCost)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_force_power");
		return false;
	}

	//Check if you need healing
	int Health = Jedi->getHam()->mHealth.getCurrentHitPoints();
	int Action = Jedi->getHam()->mAction.getCurrentHitPoints();
	int Mind = Jedi->getHam()->mMind.getCurrentHitPoints();
	int MaxHealth = Jedi->getHam()->mHealth.getMaxHitPoints();
	int MaxAction = Jedi->getHam()->mAction.getMaxHitPoints();
	int MaxMind = Jedi->getHam()->mMind.getMaxHitPoints();
	//Check if you need wound healing
	int HealthWounds = Jedi->getHam()->mHealth.getWounds();
	int ActionWounds = Jedi->getHam()->mAction.getWounds();
	int MindWounds = Jedi->getHam()->mMind.getWounds();
	int BattleFatigue = Jedi->getHam()->getBattleFatigue();

	//Perform Checks - (this is a very long check...but for now this is the only thing that works -_-)
	//TODO: find a cleaner way to do this.
	if (!Jedi->checkState(CreatureState_Poisoned))
	{
		if (!Jedi->checkState(CreatureState_Diseased))
		{
			if (!Jedi->checkState(CreatureState_OnFire))
			{
				if (!Jedi->checkState(CreatureState_Bleeding))
				{
					//gMessageLib->sendSystemMessage(Jedi, L"You have no states of that type to heal.");
					if (HealthWounds <= 0)
					{
						if (ActionWounds <= 0)
						{
							if (MindWounds <= 0)
							{
								if (BattleFatigue <= 0)
								{
									//gMessageLib->sendSystemMessage(Jedi, L"You have no wounds of that type to heal.");
									if (Health == MaxHealth)
									{
										if (Action == MaxAction)
										{
											if (Mind == MaxMind)
											{
												//gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_damage_heal_self");
												gMessageLib->sendSystemMessage(Jedi, L"You have nothing that needs to be healed.");
												return false;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//Saromus - Just to make it clear why I'm healing states first, then Wounds, then HAM...
	//Wounds must be healed before HAM is healed because, lets say for example, I have health wounds,
	//Action has no wounds, and mind has no wounds. I'm low on all 3 bars. So I use Total Heal Self. Because
	//of the Health Wounds, when it goes to heal health, it will heal your health, for the amount of wounds there
	//are, and obviously healing your health with wounds does nothing...So therefore, your wounds need to be healed/cleared
	//before your HAM is healed, incase you have any wounds. When I was talking to Shotter about this, he said it would also be,
	//a great idea to heal states first, then heal wounds and HAM. When the affects of States are implemented, then it will be
	//more important to heal states first.

	//Heal negative states
	if (Jedi->checkState(CreatureState_Poisoned))
	{
		Jedi->toggleStateOff(CreatureState_Poisoned);
		gMessageLib->sendSystemMessage(Jedi, L"", "dot_message", "stop_poisoned");
		gMessageLib->sendStateUpdate(Jedi);
	}
	if (Jedi->checkState(CreatureState_Diseased))
	{
		Jedi->toggleStateOff(CreatureState_Diseased);
		gMessageLib->sendSystemMessage(Jedi, L"", "dot_message", "stop_diseased");
		gMessageLib->sendStateUpdate(Jedi);
	}
	if (Jedi->checkState(CreatureState_OnFire))
	{
		Jedi->toggleStateOff(CreatureState_OnFire);
		gMessageLib->sendSystemMessage(Jedi, L"", "dot_message", "stop_fire");
		gMessageLib->sendStateUpdate(Jedi);
	}
	if (Jedi->checkState(CreatureState_Bleeding))
	{
		Jedi->toggleStateOff(CreatureState_Bleeding);
		gMessageLib->sendSystemMessage(Jedi, L"", "dot_message", "stop_bleeding");
		gMessageLib->sendStateUpdate(Jedi);
	}

	//Heals all wounds completely.
	if (HealthWounds > 0)
	{
		Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -HealthWounds);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_wounds", L"", -HealthWounds);
	}
	if (ActionWounds > 0)
	{
		Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -ActionWounds);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "Action_wounds", L"", -ActionWounds);
	}
	if (MindWounds > 0)
	{
		Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -MindWounds);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_wounds", L"", -MindWounds);
	}
	if (BattleFatigue > 0)
	{
		Jedi->getHam()->updateBattleFatigue(-BattleFatigue);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "battle_fatigue", L"", -BattleFatigue);
	}

	int H = MaxHealth - Health;
	int A = MaxAction - Action;
	int M = MaxMind - Mind;

	//Heals all HAM completely.
	if (Health != MaxHealth)
	{
		Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, H);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_damage", L"", -H);
	}
	if (Action != MaxAction)
	{
		Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, A);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_damage", L"", -A);
	}
	if (Mind != MaxMind)
	{
		Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, M);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_damage", L"", -M);
	}

	Jedi->getHam()->updateCurrentForce(-forceCost);

	//Client Effect
	//gMessageLib->sendPlayClientEffectLocMessage("clienteffect/pl_force_healing.cef", Jedi->mPosition, Jedi);
	gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_healing.cef", "", Jedi);

	return true;

}

//TODO: Find the actual values for the forcecost and heal amount for each ForceHealTargetDamage skill.
bool JediSkillManager::ForceHealTargetDamage(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
	//Perform Checks
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	if (Jedi == Target)
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot heal yourself through those means.");
		return false;
	}

	//Range = 32
	float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

	//Check if target is in range.
	if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "healing", "no_line_of_sight");
		return false;
	}

	//Check if target needs healing
	int TargetHealth = Target->getHam()->mHealth.getCurrentHitPoints();
	int TargetAction = Target->getHam()->mAction.getCurrentHitPoints();
	int TargetMind = Target->getHam()->mMind.getCurrentHitPoints();
	int TargetMaxHealth = Target->getHam()->mHealth.getMaxHitPoints();
	int TargetMaxAction = Target->getHam()->mAction.getMaxHitPoints();
	int TargetMaxMind = Target->getHam()->mMind.getMaxHitPoints();

	if (TargetHealth == TargetMaxHealth)
	{
		if (TargetAction == TargetMaxAction)
		{
			if (TargetMind == TargetMaxMind)
			{
				gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_damage_heal_other");
				return false;
			}
		}
	}

	//All type heal ints
	int H = TargetHealth - TargetMaxHealth;
	int A = TargetAction - TargetMaxAction;
	int M = TargetMind - TargetMaxMind;
	//healAllOther 1 & 2 - don't know actual value for ForceCost; Need to find this out.
	//just going to use a temporary forcecost for now...
	//the temp cost can be increased if anyone feels its too low :)
	int cost1 = min((H + A + M / 3), 200);
	int cost2 = min((H + A + M / 3), 400);

	//Don't know the actual value for how much is healed for any of the HealTypes.
	//just going to use a temporary heal amount. (which are the same values from the ForceHealSelfDamage)
	//the temp amount can be increased if anyone feels its too low :)
	switch (HealType)
	{
	case 1:	//Heal All Other 1
		if (Jedi->getHam()->getCurrentForce() < cost1)
		{
			gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_force_power");
			return false;
		}
		Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 500);
		Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 500);
		Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_damage", L"", -H, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_damage", L"", -A, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_damage", L"", -M, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "health_damage", L"", -H, "", "", L"", 0, Jedi->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "action_damage", L"", -A, "", "", L"", 0, Jedi->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "mind_damage", L"", -M, "", "", L"", 0, Jedi->getId());
		Jedi->getHam()->updateCurrentForce(-cost1);
		break;
	case 2: //Heal All Other 2
		if (Jedi->getHam()->getCurrentForce() < cost2)
		{
			gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_force_power");
			return false;
		}
		Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 1500);
		Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 1500);
		Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 1500);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_damage", L"", -H, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_damage", L"", -A, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_damage", L"", -M, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "health_damage", L"", -H, "", "", L"", 0, Jedi->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "action_damage", L"", -A, "", "", L"", 0, Jedi->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "mind_damage", L"", -M, "", "", L"", 0, Jedi->getId());
		Jedi->getHam()->updateCurrentForce(-cost2);
		break;
	default:
		break;
	}

	//Animation
	gMessageLib->sendCreatureAnimation(Jedi, BString("force_healing_1"));

	return true;
}

//TODO: Find the actual values for the forcecost and heal amount for each ForceHealTargetWound skill.
bool JediSkillManager::ForceHealTargetWound(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
	//Perform Checks
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	if (Jedi == Target)
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot heal yourself through those means.");
		return false;
	}

	//Range = 32
	float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

	//Check if target is in range.
	if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "healing", "no_line_of_sight");
		return false;
	}

	int TargetHealthWounds = Target->getHam()->mHealth.getWounds();
	int TargetActionWounds = Target->getHam()->mAction.getWounds();
	int TargetMindWounds = Target->getHam()->mMind.getWounds();

	if (TargetHealthWounds <= 0)
	{
		if (TargetActionWounds <= 0)
		{
			if (TargetMindWounds <= 0)
			{
				gMessageLib->sendSystemMessage(Jedi, L"Your target has no wounds of that type to heal.");
				return false;
			}
		}
	}

	//The following all have temporary force costs (45 & 96) until the actual values are found.
	int costHW1 = min(TargetHealthWounds, 45);
	int costHW2 = min(TargetHealthWounds, 96);
	int costAW1 = min(TargetActionWounds, 45);
	int costAW2 = min(TargetActionWounds, 96);
	int costMW1 = min(TargetMindWounds, 45);
	int costMW2 = min(TargetMindWounds, 96);

	//The healWound amount is temporary until the actual values are found. For now I'm just using the same values as ForceHealSelfWound
	switch (HealType)
	{
	case 1: //Heal Health Wound Other 1
		Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -50);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_wounds", L"", -TargetHealthWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "health_wounds", L"", -TargetHealthWounds, "", "", L"", 0, Jedi->getId());
		Jedi->getHam()->updateCurrentForce(-costHW1);
		break;
	case 2: //Heal Health Wound Other 2
		Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -100);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_wounds", L"", -TargetHealthWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "health_wounds", L"", -TargetHealthWounds, "", "", L"", 0, Jedi->getId());
		Jedi->getHam()->updateCurrentForce(-costHW2);
		break;
	case 3: //Heal Action Wound Other 1
		Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -50);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_wounds", L"", -TargetActionWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "action_wounds", L"", -TargetActionWounds, "", "", L"", 0, Jedi->getId());
		Jedi->getHam()->updateCurrentForce(-costAW1);
		break;
	case 4: //Heal Action Wound Other 2
		Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -100);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_wounds", L"", -TargetActionWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "action_wounds", L"", -TargetActionWounds, "", "", L"", 0, Jedi->getId());
		Jedi->getHam()->updateCurrentForce(-costAW2);
		break;
	case 5: //Heal Mind Wound Other 1
		Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -50);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_wounds", L"", -TargetMindWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "mind_wounds", L"", -TargetMindWounds, "", "", L"", 0, Jedi->getId());
		Jedi->getHam()->updateCurrentForce(-costMW1);
		break;
	case 6: //Heal Mind Wound Other 2
		Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -100);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_wounds", L"", -TargetMindWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "mind_wounds", L"", -TargetMindWounds, "", "", L"", 0, Jedi->getId());
		Jedi->getHam()->updateCurrentForce(-costMW2);
		break;
	default:
		break;
	}

	//Animation
	gMessageLib->sendCreatureAnimation(Jedi, BString("force_healing_1"));

	return true;
}

bool JediSkillManager::ForceHealTargetState(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties)
{
	int forceCost = 50;

	//Perform Checks
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	if (Jedi == Target)
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot heal yourself through those means.");
		return false;
	}

	if (Jedi->getHam()->getCurrentForce() < forceCost)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_force_power");
		return false;
	}

	//Range = 32
	float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

	//Check if target is in range.
	if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "healing", "no_line_of_sight");
		return false;
	}

	if (!Target->checkState(CreatureState_Stunned))
	{
		if (!Target->checkState(CreatureState_Blinded))
		{
			if (!Target->checkState(CreatureState_Dizzy))
			{
				if (!Target->checkState(CreatureState_Intimidated))
				{
					gMessageLib->sendSystemMessage(Jedi, L"You have no states of that type to heal.");
					return false;
				}
			}
		}
	}

	//If player has the state, then clear it.
	if (Target->checkState(CreatureState_Stunned))
	{
		Target->toggleStateOff(CreatureState_Stunned);
		gMessageLib->sendSystemMessage(Target, L"", "cbt_spam", "no_stunned_single");
		gMessageLib->sendFlyText(Target, "combat_effects", "no_stunned", 255, 0, 0);
	}
	if (Target->checkState(CreatureState_Blinded))
	{
		Target->toggleStateOff(CreatureState_Blinded);
		gMessageLib->sendSystemMessage(Target, L"", "cbt_spam", "no_blind_single");
		gMessageLib->sendFlyText(Target, "combat_effects", "no_blind", 255, 0, 0);
	}
	if (Target->checkState(CreatureState_Dizzy))
	{
		Target->toggleStateOff(CreatureState_Dizzy);
		gMessageLib->sendSystemMessage(Target, L"", "cbt_spam", "no_dizzy_single");
		gMessageLib->sendFlyText(Target, "combat_effects", "no_dizzy", 255, 0, 0);
	}
	if (Target->checkState(CreatureState_Intimidated))
	{
		Target->toggleStateOff(CreatureState_Intimidated);
		gMessageLib->sendFlyText(Target, "combat_effects", "no_intimidated", 255, 0, 0);
	}

	gMessageLib->sendStateUpdate(Target);

	Jedi->getHam()->updateCurrentForce(-forceCost);

	//Animation
	gMessageLib->sendCreatureAnimation(Jedi, BString("force_healing_1"));

	gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "stop_states_other", "", "", L"", 0, "", "", L"", Target->getId());

	return true;
}

bool JediSkillManager::ForceCureTarget(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
	int forceCost = 75;

	//Perform Checks
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	if (Jedi->getHam()->getCurrentForce() < forceCost)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_force_power");
		return false;
	}

	if (Jedi == Target)
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot heal yourself through those means.");
		return false;
	}

	//Range = 32
	float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

	//Check if target is in range.
	if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "healing", "no_line_of_sight");
		return false;
	}

	switch (HealType)
	{
	case 1: //Force Cure Disease
		if (!Target->checkState(CreatureState_Diseased))
		{
			gMessageLib->sendSystemMessage(Jedi, L"Your target is not diseased.");
			return false;
		}
		else
		{
			Target->toggleStateOff(CreatureState_Diseased);
			gMessageLib->sendSystemMessage(Target, L"", "dot_message", "stop_diseased");
			gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "stop_disease_other", "", "", L"", 0, "", "", L"", Target->getId());
			gMessageLib->sendStateUpdate(Target);
			Jedi->getHam()->updateCurrentForce(-forceCost);
		}
		break;
	case 2: //Force Cure Poison
		if (!Target->checkState(CreatureState_Poisoned))
		{
			gMessageLib->sendSystemMessage(Jedi, L"Your target is not poisoned.");
			return false;
		}
		else
		{
			Target->toggleStateOff(CreatureState_Poisoned);
			gMessageLib->sendSystemMessage(Target, L"", "dot_message", "stop_poisoned");
			gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "stop_poison_other", "", "", L"", 0, "", "", L"", Target->getId());
			gMessageLib->sendStateUpdate(Target);
			Jedi->getHam()->updateCurrentForce(-forceCost);
		}
		break;
	case 3: //Stop Bleeding
		if (!Target->checkState(CreatureState_Bleeding))
		{
			gMessageLib->sendSystemMessage(Jedi, L"Your target is not bleeding.");
			return false;
		}
		else
		{
			Target->toggleStateOff(CreatureState_Bleeding);
			gMessageLib->sendSystemMessage(Target, L"", "dot_message", "stop_bleeding");
			gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "stop_bleeding_other", "", "", L"", 0, "", "", L"", Target->getId());
			gMessageLib->sendStateUpdate(Target);
			Jedi->getHam()->updateCurrentForce(-forceCost);
		}
		break;
	default:
		break;
	}

	//Client Effect
	//gMessageLib->sendPlayClientEffectLocMessage("clienteffect/pl_force_healing.cef", Jedi->mPosition, Jedi);
	gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_healing.cef", "", Jedi);
	return true;
}

//This heals damage and wounds on the player
bool JediSkillManager::ForceHealTargetTotal(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties)
{
	int forceCost = 1000;

	//Perform Checks
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	if (Jedi == Target)
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot heal yourself through those means.");
		return false;
	}

	if (Jedi->getHam()->getCurrentForce() < forceCost)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_force_power");
		return false;
	}

	//Range = 32
	float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

	//Check if target is in range.
	if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "healing", "no_line_of_sight");
		return false;
	}

	//Check if target needs healing
	int TargetHealth = Target->getHam()->mHealth.getCurrentHitPoints();
	int TargetAction = Target->getHam()->mAction.getCurrentHitPoints();
	int TargetMind = Target->getHam()->mMind.getCurrentHitPoints();
	int TargetMaxHealth = Target->getHam()->mHealth.getMaxHitPoints();
	int TargetMaxAction = Target->getHam()->mAction.getMaxHitPoints();
	int TargetMaxMind = Target->getHam()->mMind.getMaxHitPoints();
	//Check if target needs wound healing
	int TargetHealthWounds = Target->getHam()->mHealth.getWounds();
	int TargetActionWounds = Target->getHam()->mAction.getWounds();
	int TargetMindWounds = Target->getHam()->mMind.getWounds();
	int TargetBattleFatigue = Target->getHam()->getBattleFatigue();

	//Perform Checks - (this is a very long check...but for now this is the only thing that works -_-)
	//TODO: find a cleaner way to do this.
	if (!Target->checkState(CreatureState_Poisoned))
	{
		if (!Target->checkState(CreatureState_Diseased))
		{
			if (!Target->checkState(CreatureState_OnFire))
			{
				if (!Target->checkState(CreatureState_Bleeding))
				{
					//gMessageLib->sendSystemMessage(Jedi, L"Your target has no states of that type to heal.");
					if (TargetHealthWounds <= 0)
					{
						if (TargetActionWounds <= 0)
						{
							if (TargetMindWounds <= 0)
							{
								if (TargetBattleFatigue <= 0)
								{
									//gMessageLib->sendSystemMessage(Jedi, L"Your target has no wounds of that type to heal.");
									if (TargetHealth == TargetMaxHealth)
									{
										if (TargetAction == TargetMaxAction)
										{
											if (TargetMind == TargetMaxMind)
											{
												//gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_damage_heal_other");
												gMessageLib->sendSystemMessage(Jedi, L"Your target nothing that needs to be healed.");
												return false;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//Saromus - Just to make it clear why I'm healing states first, then Wounds, then HAM...
	//Wounds must be healed before HAM is healed because, lets say for example, I have health wounds,
	//Action has no wounds, and mind has no wounds. I'm low on all 3 bars. So I use Total Heal Self. Because
	//of the Health Wounds, when it goes to heal health, it will heal your health, for the amount of wounds there
	//are, and obviously healing your health with wounds does nothing...So therefore, your wounds need to be healed/cleared
	//before your HAM is healed, incase you have any wounds. When I was talking to Shotter about this, he said it would also be,
	//a great idea to heal states first, then heal wounds and HAM. When the affects of States are implemented, then it will be
	//more important to heal states first.

	if (Target->checkState(CreatureState_Poisoned))
	{
		Target->toggleStateOff(CreatureState_Poisoned);
		gMessageLib->sendSystemMessage(Target, L"", "dot_message", "stop_poisoned");
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "stop_poison_other", "", "", L"", 0, "", "", L"", Target->getId());
		gMessageLib->sendStateUpdate(Target);
	}
	if (Target->checkState(CreatureState_Diseased))
	{
		Target->toggleStateOff(CreatureState_Diseased);
		gMessageLib->sendSystemMessage(Target, L"", "dot_message", "stop_diseased");
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "stop_disease_other", "", "", L"", 0, "", "", L"", Target->getId());
		gMessageLib->sendStateUpdate(Target);
	}
	if (Target->checkState(CreatureState_OnFire))
	{
		Target->toggleStateOff(CreatureState_OnFire);
		gMessageLib->sendSystemMessage(Target, L"", "dot_message", "stop_fire");
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "stop_disease_other", "", "", L"", 0, "", "", L"", Target->getId());
		gMessageLib->sendStateUpdate(Target);
	}
	if (Target->checkState(CreatureState_Bleeding))
	{
		Jedi->toggleStateOff(CreatureState_Bleeding);
		gMessageLib->sendSystemMessage(Target, L"", "dot_message", "stop_bleeding");
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "stop_bleeding_other", "", "", L"", 0, "", "", L"", Target->getId());
		gMessageLib->sendStateUpdate(Target);
	}

	//Heals all wounds completely.
	if (TargetHealthWounds > 0)
	{
		Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -TargetHealthWounds);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_wounds", L"", -TargetHealthWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "health_wounds", L"", -TargetHealthWounds, "", "", L"", 0, Jedi->getId());
	}
	if (TargetActionWounds > 0)
	{
		Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -TargetActionWounds);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_wounds", L"", -TargetActionWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "action_wounds", L"", -TargetActionWounds, "", "", L"", 0, Jedi->getId());
	}
	if (TargetMindWounds > 0)
	{
		Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -TargetMindWounds);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_wounds", L"", -TargetMindWounds, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "mind_wounds", L"", -TargetMindWounds, "", "", L"", 0, Jedi->getId());
	}
	if (TargetBattleFatigue > 0)
	{
		Target->getHam()->updateBattleFatigue(-TargetBattleFatigue);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "battle_fatigue", L"", -TargetBattleFatigue, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "battle_fatigue", L"", -TargetBattleFatigue, "", "", L"", 0, Jedi->getId());
	}

	int H = TargetMaxHealth - TargetHealth;
	int A = TargetMaxAction - TargetAction;
	int M = TargetMaxMind - TargetMind;

	//Heals all HAM completely.
	if (TargetHealth != TargetMaxHealth)
	{
		Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, H);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "health_damage", L"", -H, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "health_damage", L"", -H, "", "", L"", 0, Jedi->getId());
	}
	if (TargetAction != TargetMaxAction)
	{
		Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, A);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "action_damage", L"", -A, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "action_damage", L"", -A, "", "", L"", 0, Jedi->getId());
	}
	if (TargetMind != TargetMaxMind)
	{
		Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, M);
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "heal_self", "jedi_spam", "mind_damage", L"", -M, "", "", L"", Target->getId());
		gMessageLib->sendSystemMessage(Target, L"", "jedi_spam", "heal_other_other", "jedi_spam", "mind_damage", L"", -M, "", "", L"", 0, Jedi->getId());
	}

	Jedi->getHam()->updateCurrentForce(-forceCost);

	//Animation
	gMessageLib->sendCreatureAnimation(Jedi, BString("force_healing_1"));

	return true;
}
//TODO: Implement a efficient way to update the Force Regen rate.
//The Regen Rate/modifier from forcemeditate is *3
//BUG: When you stand up from force meditate, it doesn't set the mood to "none." 
//see - PlayerObject.cpp : setUpright(), setProne(), & setCrouched() for the mood reset for forcemeditate, that doesn't work -_-
bool JediSkillManager::ForceMeditateSelfSkill(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int ForceRegen)
{
	if (Jedi->isForceMeditating())
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "already_in_meditative_state");
		return false;
	}

	// Updates
	Jedi->modifySkillModValue(SMod_jedi_force_power_regen, +ForceRegen);
	Jedi->setMeditateState();

	// Schedule Execution
	Jedi->getController()->addEvent(new ForceMeditateEvent(6000), 6000);

	if (!Jedi->isForceMeditating())
	{
		Jedi->modifySkillModValue(SMod_jedi_force_power_regen, -ForceRegen);
		return false;
	}

	return true;
}

//BUG: While force running, if you /kneel or /sit, it will kill the force run. I'll fix this another day...
bool JediSkillManager::ForceRunSelfSkill(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int SkillLevel)
// Force Run Skill Level
// 1 - Force Run 1 [force run +1, terrain neg. +33]
// 2 - Force Run 2 [force run +2, terrain neg. +66]
// 3 - Force Run 3 [force run +3, terrain neg. +99]
{
	if (Jedi->checkIfMounted())
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot do that while mounted on a creature or vehicle.");
		return false;
	}

	//In Pre-CU, you couldn't force run while you were dizzy. (Thanks Shotter for reminding about about this.)
	if (Jedi->checkState(CreatureState_Dizzy))
	{
		gMessageLib->sendSystemMessage(Jedi, L"You cannot Force Run while you are dizzy.");
		return false;
	}

	//Check if Force Run has already been activated.
	if (Jedi->checkPlayerCustomFlag(PlayerCustomFlag_ForceRun))
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "already_force_running");
		return false;
	}

	uint32 buffState;
	float speed;
	float acceleration;
	//float terrainNegotiation;
	int slope;
	int duration;
	int forceCost;

	if (SkillLevel = 2)
	{
		buffState = jedi_force_run_2;
		speed = 8.624f;
		acceleration = -.55f;
		//terrainNegotiation = 66;
		slope = 50;
		duration = 120;
		forceCost = 400;
	}
	else if (SkillLevel = 3)
	{
		buffState = jedi_force_run_3;
		speed = 14.624f;
		acceleration = -.55f;
		//terrainNegotiation = 99;
		slope = 50;
		duration = 120;
		forceCost = 600;
	}
	else
	{
		buffState = jedi_force_run_1;
		speed = 2.624f;
		acceleration = -.55f;
		//terrainNegotiation = 33;
		slope = 50;
		duration = 120;
		forceCost = 200;
	}

	if (Jedi->getHam()->getCurrentForce() < forceCost)
	{
		gMessageLib->sendSystemMessage(Jedi, L"", "jedi_spam", "no_force_power");
		return false;
	}

	// Updates
	float new_speed = Jedi->getBaseRunSpeedLimit() + speed;
	float new_acceleration = Jedi->getBaseAcceleration() + acceleration;
	//float new_terrainNegotiation = Jedi->getBaseTerrainNegotiation() + terrainNegotiation;
	Jedi->modifySkillModValue(SMod_slope_move, +slope);

	// Implement Speed
	Jedi->setCurrentRunSpeedLimit(new_speed);
	Jedi->setCurrentAcceleration(new_acceleration);
	//Jedi->setCurrentTerrainNegotiation(new_terrainNegotiation);
	gMessageLib->sendUpdateMovementProperties(Jedi);
	//gMessageLib->sendTerrainNegotiation(Jedi);

	// Buff Icon
	Buff* forceRun = Buff::SimpleBuff(Jedi, Jedi, 120, buffState, gWorldManager->GetCurrentGlobalTick());
	Jedi->AddBuff(forceRun);

	Jedi->togglePlayerCustomFlagOn(buffState);
	Jedi->togglePlayerCustomFlagOn(PlayerCustomFlag_ForceRun);

	// Force Cost
	Jedi->getHam()->updateCurrentForce(-forceCost);

	// Schedule Execution
	Jedi->getController()->addEvent(new ForceRunEvent(120*1000), 120*1000);

	// Client Effect
	gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_run_self.cef", "", Jedi);

	// Msg to Client
	gMessageLib->sendSystemMessage(Jedi, L"", "cbt_spam", "forcerun_start_single");

	return true;

}