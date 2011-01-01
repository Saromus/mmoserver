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

#define NOMINMAX
#include <algorithm>

#include "JediSkillManager.h"
#include "WorldManager.h"
#include "ObjectControllerCommandMap.h"
#include "ObjectControllerOpcodes.h"
#include "PlayerObject.h"
#include "StateManager.h"
#include "Buff.h"
#include "WorldConfig.h"
#include "Common/OutOfBand.h"

#include "MessageLib/MessageLib.h"

using ::common::OutOfBand;

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
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
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
                gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_damage_heal_self"), Jedi);
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
    int cost1 = std::min((H + A + M / 3), 340); //int cost1 = min(((H + A + M) / 3), 340);
    int cost2 = std::min((H + A + M / 3), 470); //int cost2 = min(((H + A + M) / 3), 470);

    //Individual pool type heal ints
    int costH1 = std::min(H, 65);
    int costH2 = std::min(H, 100);
    int costA1 = std::min(A, 65);
    int costA2 = std::min(A, 100);
    int costM1 = std::min(M, 65);
    int costM2 = std::min(M, 100);

    switch (HealType)
    {
    case 1: //Heal All Self 1
        Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 500);
        Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 500);
        Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "health_damage", -H), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "action_damage", -A), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "mind_damage", -M), Jedi);
        Jedi->getHam()->updateCurrentForce(-cost1);
        break;
    case 2: //Heal All Self 2
        Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 1500);
        Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 1500);
        Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 1500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "health_damage", -H), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "action_damage", -A), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "mind_damage", -M), Jedi);
        Jedi->getHam()->updateCurrentForce(-cost2);
        break;
    case 3: //Heal Health Self 1
        Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "health_damage", -H), Jedi);
        Jedi->getHam()->updateCurrentForce(-costH1);
        break;
    case 4: //Heal Health Self 2
        Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 1500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "health_damage", -H), Jedi);
        Jedi->getHam()->updateCurrentForce(-costH2);
        break;
    case 5: //Heal Action Self 1
        Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "action_damage", -A), Jedi);
        Jedi->getHam()->updateCurrentForce(-costA1);
        break;
    case 6: //Heal Action Self 2
        Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 1500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "action_damage", -A), Jedi);
        Jedi->getHam()->updateCurrentForce(-costA2);
        break;
    case 7: //Heal Mind Self 1
        Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "mind_damage", -M), Jedi);
        Jedi->getHam()->updateCurrentForce(-costM1);
        break;
    case 8: //Heal Mind Self 2
        Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 1500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "mind_damage", -M), Jedi);
        Jedi->getHam()->updateCurrentForce(-costM2);
        break;
    default:
        break;
    }

    //Client Effect
    //gMessageLib->sendPlayClientEffectLocMessage("clienteffect/pl_force_heal_self.cef", Jedi->mPosition, Jedi);
    gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_heal_self.cef", "", Jedi);
    return true;
}

bool JediSkillManager::ForceHealSelfWound(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
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
                    gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_damage_heal_self"), Jedi);
                    return false;
                }
            }
        }
    }

    //TODO: The force cost of these abilities increases based on the number of wounds healed.
    int costHW1 = std::min(HealthWounds, 15);
    int costHW2 = std::min(HealthWounds, 25);
    int costAW1 = std::min(ActionWounds, 15);
    int costAW2 = std::min(ActionWounds, 25);
    int costMW1 = std::min(MindWounds, 15);
    int costMW2 = std::min(MindWounds, 25);
    int costBF1 = std::min(BattleFatigue, 75);
    int costBF2 = std::min(BattleFatigue, 150);

    switch (HealType)
    {
    case 1: //Heal Health Wound Self 1
        Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -250); 
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "health_wounds", HealthWounds), Jedi);
        Jedi->getHam()->updateCurrentForce(-costHW1);
        break;
    case 2: //Heal Health Wound Self 2
        Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "health_wounds", HealthWounds), Jedi);
        Jedi->getHam()->updateCurrentForce(-costHW2);
        break;
    case 3: //Heal Action Wound Self 1
        Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -250);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "action_wounds", ActionWounds), Jedi);
        Jedi->getHam()->updateCurrentForce(-costAW1);
        break;
    case 4: //Heal Action Wound Self 2
        Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "action_wounds", ActionWounds), Jedi);
        Jedi->getHam()->updateCurrentForce(-costAW2);
        break;
    case 5: //Heal Mind Wound Self 1
        Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -250);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "mind_wounds", MindWounds), Jedi);
        Jedi->getHam()->updateCurrentForce(-costMW1);
        break;
    case 6: //Heal Mind Wound Self 2
        Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -500);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "mind_wounds", MindWounds), Jedi);
        Jedi->getHam()->updateCurrentForce(-costMW2);
        break;
    case 7: //Heal Battle Fatigue Self 1
        Jedi->getHam()->updateBattleFatigue(-150);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "battle_fatigue", BattleFatigue), Jedi);
        Jedi->getHam()->updateCurrentForce(-costBF1);
        break;
    case 8: //Heal Battle Fatigue Self 2
        Jedi->getHam()->updateBattleFatigue(-300);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "battle_fatigue", BattleFatigue), Jedi);
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
    int numberOfStatesCured = 0;

    //Perform Checks
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    if (!Jedi->states.checkState(CreatureState_Stunned))
    {
        if (!Jedi->states.checkState(CreatureState_Blinded))
        {
            if (!Jedi->states.checkState(CreatureState_Dizzy))
            {
                if (!Jedi->states.checkState(CreatureState_Intimidated))
                {
                    gMessageLib->SendSystemMessage(L"You have no states of that type to heal.", Jedi);
                    return false;
                }
            }
        }
    }

    if (Jedi->getHam()->getCurrentForce() < forceCost)
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
        return false;
    }

    //If player has the state, then clear it.
    if (Jedi->states.checkState(CreatureState_Stunned))
    {
        gStateManager.removeActionState(Jedi, CreatureState_Stunned);
        //gMessageLib->SendSystemMessage(OutOfBand("cbt_spam", "no_stunned_single"), Jedi);
        //gMessageLib->sendFlyText(Jedi, "combat_effects", "no_stunned", 255, 0, 0);
        numberOfStatesCured++;
    }
    if (Jedi->states.checkState(CreatureState_Blinded))
    {
        gStateManager.removeActionState(Jedi, CreatureState_Blinded);
        //gMessageLib->SendSystemMessage(OutOfBand("cbt_spam", "no_blind_single"), Jedi);
        //gMessageLib->sendFlyText(Jedi, "combat_effects", "no_blind", 255, 0, 0);
        numberOfStatesCured++;
    }
    if (Jedi->states.checkState(CreatureState_Dizzy))
    {
        gStateManager.removeActionState(Jedi, CreatureState_Dizzy);
        gMessageLib->SendSystemMessage(OutOfBand("cbt_spam", "no_dizzy_single"), Jedi);
        //gMessageLib->sendFlyText(Jedi, "combat_effects", "no_dizzy", 255, 0, 0);
        numberOfStatesCured++;
    }
    if (Jedi->states.checkState(CreatureState_Intimidated))
    {
        gStateManager.removeActionState(Jedi, CreatureState_Intimidated);
        //gMessageLib->sendFlyText(Jedi, "combat_effects", "no_intimidated", 255, 0, 0);
        numberOfStatesCured++;
    }

    //Send Update
    //gMessageLib->sendStateUpdate(Jedi);

    //The base force cost (50) of this ability is multiplied by the number of states that are healed.
    //In this case, it is 50 base force times the number of states on the player.
    //[Equation (when force cost = 50)]: ModifiedForceCost = ForceCost x NumberOfStatesCured
    int modifiedForceCost = forceCost * numberOfStatesCured;
    Jedi->getHam()->updateCurrentForce(-modifiedForceCost);

    gMessageLib->SendSystemMessage(L"You cure all negative states on yourself.", Jedi);
    return true;
}

//This heals states (only DOTs tho), wounds, and damage on the player.
bool JediSkillManager::ForceHealSelfTotal(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties)
{
    int forceCost = 400;

    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    if (Jedi->getHam()->getCurrentForce() < forceCost)
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
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
    if (!Jedi->states.checkState(CreatureState_Poisoned))
    {
        if (!Jedi->states.checkState(CreatureState_Diseased))
        {
            if (!Jedi->states.checkState(CreatureState_OnFire))
            {
                if (!Jedi->states.checkState(CreatureState_Bleeding))
                {
                    //gMessageLib->SendSystemMessage(L"You have no states of that type to heal.", Jedi);
                    if (HealthWounds <= 0)
                    {
                        if (ActionWounds <= 0)
                        {
                            if (MindWounds <= 0)
                            {
                                if (BattleFatigue <= 0)
                                {
                                    //gMessageLib->SendSystemMessage(L"You have no wounds of that type to heal.", Jedi);
                                    if (Health == MaxHealth)
                                    {
                                        if (Action == MaxAction)
                                        {
                                            if (Mind == MaxMind)
                                            {
                                                //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_damage_heal_self"), Jedi);
                                                gMessageLib->SendSystemMessage(L"You have nothing that needs to be healed.", Jedi);
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

    //(Saromus) - Just to make it clear why I'm healing states first, then Wounds, then HAM...
    //Wounds must be healed before HAM is healed because, lets say for example, I have health wounds,
    //Action has no wounds, and mind has no wounds. I'm low on all 3 bars. So I use Total Heal Self. Because
    //of the Health Wounds, when it goes to heal health, it will heal your health, for the amount of wounds there
    //are, and obviously healing your health with wounds does nothing...So therefore, your wounds need to be healed/cleared
    //before your HAM is healed, incase you have any wounds. When I was talking to Shotter about this, he said it would be
    //a good idea to heal states first, then heal wounds, then HAM. When the affects of States are implemented, then it will
    //be more important to heal states first.

    //Heal negative states
    if (Jedi->states.checkState(CreatureState_Poisoned))
    {
        gStateManager.removeActionState(Jedi, CreatureState_Poisoned);
        //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_poisoned"), Jedi);
        //gMessageLib->sendStateUpdate(Jedi);
    }
    if (Jedi->states.checkState(CreatureState_Diseased))
    {
        gStateManager.removeActionState(Jedi, CreatureState_Diseased);
        //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_diseased"), Jedi);
        //gMessageLib->sendStateUpdate(Jedi);
    }
    if (Jedi->states.checkState(CreatureState_OnFire))
    {
        gStateManager.removeActionState(Jedi, CreatureState_OnFire);
        //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_fire"), Jedi);
        //gMessageLib->sendStateUpdate(Jedi);
    }
    if (Jedi->states.checkState(CreatureState_Bleeding))
    {
        gStateManager.removeActionState(Jedi, CreatureState_Bleeding);
        //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_bleeding"), Jedi);
        //gMessageLib->sendStateUpdate(Jedi);
    }

    //Heals all wounds completely.
    if (HealthWounds > 0)
    {
        Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -HealthWounds);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "health_wounds", HealthWounds), Jedi);
    }
    if (ActionWounds > 0)
    {
        Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -ActionWounds);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "action_wounds", ActionWounds), Jedi);
    }
    if (MindWounds > 0)
    {
        Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -MindWounds);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "mind_wounds", MindWounds), Jedi);
    }
    if (BattleFatigue > 0)
    {
        Jedi->getHam()->updateBattleFatigue(-BattleFatigue);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "battle_fatigue", BattleFatigue), Jedi);
    }

    int H = MaxHealth - Health;
    int A = MaxAction - Action;
    int M = MaxMind - Mind;

    //Heals all HAM completely.
    if (Health != MaxHealth)
    {
        Jedi->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, H);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "health_damage", -H), Jedi);
    }
    if (Action != MaxAction)
    {
        Jedi->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, A);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "action_damage", -A), Jedi);
    }
    if (Mind != MaxMind)
    {
        Jedi->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, M);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_self", "", "", "", "", "jedi_spam", "mind_damage", -M), Jedi);;
    }

    Jedi->getHam()->updateCurrentForce(-forceCost);

    //Client Effect
    //gMessageLib->sendPlayClientEffectLocMessage("clienteffect/pl_force_healing.cef", Jedi->mPosition, Jedi);
    gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_healing.cef", "", Jedi);

    return true;
}

bool JediSkillManager::ForceHealTargetDamage(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
    //Perform Checks
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    if (Jedi == Target)
    {
        gMessageLib->SendSystemMessage(L"You cannot heal yourself through those means.", Jedi);
        return false;
    }

    //Range = 32
    float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

    //Check if target is in range.
    if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
    {
        gMessageLib->SendSystemMessage(OutOfBand("healing", "no_line_of_sight"), Jedi);
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
                gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_damage_heal_other"), Jedi);
                return false;
            }
        }
    }

    //All type heal ints
    int H = TargetHealth - TargetMaxHealth;
    int A = TargetAction - TargetMaxAction;
    int M = TargetMind - TargetMaxMind;

    int cost1 = std::min((H + A + M / 3), 680);
    int cost2 = std::min((H + A + M / 3), 940);

    switch (HealType)
    {
    case 1:	//Heal All Other 1
        if (Jedi->getHam()->getCurrentForce() < cost1)
        {
            gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
            return false;
        }
        Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 250);
        Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 250);
        Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 250);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "health_damage", L"", -H), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "action_damage", L"", -A), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "mind_damage", L"", -M), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "health_damage", L"", -H), Target);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "action_damage", L"", -A), Target);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "mind_damage", L"", -M), Target);

        Jedi->getHam()->updateCurrentForce(-cost1);
        break;
    case 2: //Heal All Other 2
        if (Jedi->getHam()->getCurrentForce() < cost2)
        {
            gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
            return false;
        }
        Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, 750);
        Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, 750);
        Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, 750);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "health_damage", L"", -H), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "action_damage", L"", -A), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "mind_damage", L"", -M), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "health_damage", L"", -H), Target);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "action_damage", L"", -A), Target);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "mind_damage", L"", -M), Target);
        Jedi->getHam()->updateCurrentForce(-cost2);
        break;
    default:
        break;
    }

    //Animation
    gMessageLib->sendCreatureAnimation(Jedi, BString("force_healing_1"));

    return true;
}

bool JediSkillManager::ForceHealTargetWound(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
    //Perform Checks
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    if (Jedi == Target)
    {
        gMessageLib->SendSystemMessage(L"You cannot heal yourself through those means.", Jedi);
        return false;
    }

    //Range = 32
    float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

    //Check if target is in range.
    if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
    {
        gMessageLib->SendSystemMessage(OutOfBand("healing", "no_line_of_sight"), Jedi);
        return false;
    }

    int TargetHealthWounds = Target->getHam()->mHealth.getWounds();
    int TargetActionWounds = Target->getHam()->mAction.getWounds();
    int TargetMindWounds = Target->getHam()->mMind.getWounds();
    int TargetBattleFatigue = Target->getHam()->getBattleFatigue();

    if (TargetHealthWounds <= 0)
    {
        if (TargetActionWounds <= 0)
        {
            if (TargetMindWounds <= 0)
            {
                if (TargetBattleFatigue <= 0)
                {
                    gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_damage_heal_other"), Jedi);
                    return false;
                }
            }
        }
    }

    //TODO: The force cost of these abilities increases based on the number of wounds healed.
    int costHW1 = std::min(TargetHealthWounds, 25);
    int costHW2 = std::min(TargetHealthWounds, 75);
    int costAW1 = std::min(TargetActionWounds, 25);
    int costAW2 = std::min(TargetActionWounds, 75);
    int costMW1 = std::min(TargetMindWounds, 25);
    int costMW2 = std::min(TargetMindWounds, 75);
    int costBF1 = std::min(TargetBattleFatigue, 150);
    int costBF2 = std::min(TargetBattleFatigue, 300);

    switch (HealType)
    {
    case 1: //Heal Health Wound Other 1
        Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -100);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "health_wounds", L"", TargetHealthWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "health_wounds", L"", TargetHealthWounds), Target);
        Jedi->getHam()->updateCurrentForce(-costHW1);
        break;
    case 2: //Heal Health Wound Other 2
        Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -200);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "health_wounds", L"", TargetHealthWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "health_wounds", L"", TargetHealthWounds), Target);
        Jedi->getHam()->updateCurrentForce(-costHW2);
        break;
    case 3: //Heal Action Wound Other 1
        Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -100);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "action_wounds", L"", TargetActionWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "action_wounds", L"", TargetActionWounds), Target);
        Jedi->getHam()->updateCurrentForce(-costAW1);
        break;
    case 4: //Heal Action Wound Other 2
        Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -200);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "action_wounds", L"", TargetActionWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "action_wounds", L"", TargetActionWounds), Target);
        Jedi->getHam()->updateCurrentForce(-costAW2);
        break;
    case 5: //Heal Mind Wound Other 1
        Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -100);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "mind_wounds", L"", TargetMindWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "mind_wounds", L"", TargetMindWounds), Target);
        Jedi->getHam()->updateCurrentForce(-costMW1);
        break;
    case 6: //Heal Mind Wound Other 2
        Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -200);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "mind_wounds", L"", TargetMindWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "mind_wounds", L"", TargetMindWounds), Target);
        Jedi->getHam()->updateCurrentForce(-costMW2);
        break;
    case 7: //Heal Battle Fatigue Other 1
        Target->getHam()->updateBattleFatigue(-60);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "battle_fatigue", L"", TargetBattleFatigue), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "battle_fatigue", L"", TargetBattleFatigue), Target);
        Jedi->getHam()->updateCurrentForce(-costBF1);
        break;
    case 8: //Heal Battle Fatigue Other 2
        Target->getHam()->updateBattleFatigue(-120);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "battle_fatigue", L"", TargetBattleFatigue), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "battle_fatigue", L"", TargetBattleFatigue), Target);
        Jedi->getHam()->updateCurrentForce(-costBF2);
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
    int numberOfStatesCured = 0;

    //Perform Checks
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    if (Jedi == Target)
    {
        gMessageLib->SendSystemMessage(L"You cannot heal yourself through those means.", Jedi);
        return false;
    }

    if (Jedi->getHam()->getCurrentForce() < forceCost)
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
        return false;
    }

    //Range = 32
    float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

    //Check if target is in range.
    if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
    {
        gMessageLib->SendSystemMessage(OutOfBand("healing", "no_line_of_sight"), Jedi);
        return false;
    }

    if (!Target->states.checkState(CreatureState_Stunned))
    {
        if (!Target->states.checkState(CreatureState_Blinded))
        {
            if (!Target->states.checkState(CreatureState_Dizzy))
            {
                if (!Target->states.checkState(CreatureState_Intimidated))
                {
                    gMessageLib->SendSystemMessage(L"Your target has no states of that type to heal.", Jedi);
                    return false;
                }
            }
        }
    }

    //If player has the state, then clear it.
    if (Target->states.checkState(CreatureState_Stunned))
    {
		gStateManager.removeActionState(Target, CreatureState_Stunned);
		//gMessageLib->SendSystemMessage(OutOfBand("cbt_spam", "no_stunned_single"), Target);
		//gMessageLib->sendFlyText(Target, "combat_effects", "no_stunned", 255, 0, 0);
		numberOfStatesCured++;
    }
    if (Target->states.checkState(CreatureState_Blinded))
    {
		gStateManager.removeActionState(Target, CreatureState_Blinded);
		//gMessageLib->SendSystemMessage(OutOfBand("cbt_spam", "no_blind_single"), Target);
		//gMessageLib->sendFlyText(Target, "combat_effects", "no_blind", 255, 0, 0);
		numberOfStatesCured++;
    }
    if (Target->states.checkState(CreatureState_Dizzy))
    {
		gStateManager.removeActionState(Target, CreatureState_Dizzy);
		//gMessageLib->SendSystemMessage(OutOfBand("cbt_spam", "no_dizzy_single"), Target);
		//gMessageLib->sendFlyText(Target, "combat_effects", "no_dizzy", 255, 0, 0);
		numberOfStatesCured++;
    }
    if (Target->states.checkState(CreatureState_Intimidated))
    {
		gStateManager.removeActionState(Target, CreatureState_Intimidated);
		//gMessageLib->sendFlyText(Target, "combat_effects", "no_intimidated", 255, 0, 0);
		numberOfStatesCured++;
    }

    //Send Update
    //gMessageLib->sendStateUpdate(Target);

    //The base force cost (50) of this ability is multiplied by the number of states that are healed.
    //In this case, it is 50 base force times the number of states on the player's target.
    //[Equation (when force cost = 50)]: ModifiedForceCost = ForceCost x NumberOfStatesCured
    int modifiedForceCost = forceCost * numberOfStatesCured;
    Jedi->getHam()->updateCurrentForce(-modifiedForceCost);

    //Animation
    gMessageLib->sendCreatureAnimation(Jedi, BString("force_healing_1"));

    gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "stop_states_other", 0, Target->getId(), 0), Jedi);

    return true;
}

bool JediSkillManager::ForceCureTarget(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties, int HealType)
{
    int forceCost = 75;

    //Perform Checks
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    if (Jedi->getHam()->getCurrentForce() < forceCost)
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
        return false;
    }

    if (Jedi == Target)
    {
        gMessageLib->SendSystemMessage(L"You cannot heal yourself through those means.", Jedi);
        return false;
    }

    //Range = 32
    float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

    //Check if target is in range.
    if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
    {
        gMessageLib->SendSystemMessage(OutOfBand("healing", "no_line_of_sight"), Jedi);
        return false;
    }

    switch (HealType)
    {
    case 1: //Force Cure Disease
        if (!Target->states.checkState(CreatureState_Diseased))
        {
            gMessageLib->SendSystemMessage(L"Your target is not diseased.", Jedi);
            return false;
        }
        else
        {
            gStateManager.removeActionState(Target, CreatureState_Diseased);
            //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_diseased"), Target);
            //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "stop_disease_other", 0, Target->getId(), 0), Jedi);
            //gMessageLib->sendStateUpdate(Target);
            Jedi->getHam()->updateCurrentForce(-forceCost);
        }
        break;
    case 2: //Force Cure Poison
        if (!Target->states.checkState(CreatureState_Poisoned))
        {
            gMessageLib->SendSystemMessage(L"Your target is not poisoned.", Jedi);
            return false;
        }
        else
		{
            gStateManager.removeActionState(Target, CreatureState_Poisoned);
            //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_poisoned"), Target);
            //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "stop_poison_other", 0, Target->getId(), 0), Jedi);
            //gMessageLib->sendStateUpdate(Target);
            Jedi->getHam()->updateCurrentForce(-forceCost);
        }
        break;
    case 3: //Stop Bleeding
        if (!Target->states.checkState(CreatureState_Bleeding))
        {
            gMessageLib->SendSystemMessage(L"Your target is not bleeding.", Jedi);
            return false;
        }
        else
        {
            gStateManager.removeActionState(Target, CreatureState_Bleeding);
            //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_bleeding"), Target);
            //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "stop_bleeding_other", 0, Target->getId(), 0), Jedi);
            //gMessageLib->sendStateUpdate(Target);
            Jedi->getHam()->updateCurrentForce(-forceCost);
        }
        break;
    default:
        break;
    }

    //gMessageLib->sendPlayClientEffectLocMessage("clienteffect/pl_force_healing.cef", Jedi->mPosition, Jedi);

    //Client Effect
    gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_healing.cef", "", Jedi);

    return true;
}

//This heals states (only DOTs tho), wounds, and damage on the player's target.
bool JediSkillManager::ForceHealTargetTotal(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties)
{
    int forceCost = 1000;

    //Perform Checks
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    if (Jedi == Target)
    {
        gMessageLib->SendSystemMessage(L"You cannot heal yourself through those means.", Jedi);
        return false;
    }

    if (Jedi->getHam()->getCurrentForce() < forceCost)
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
        return false;
    }

    //Range = 32
    float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

    //Check if target is in range.
    if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
    {
        gMessageLib->SendSystemMessage(OutOfBand("healing", "no_line_of_sight"), Jedi);
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
    if (!Target->states.checkState(CreatureState_Poisoned))
    {
        if (!Target->states.checkState(CreatureState_Diseased))
        {
            if (!Target->states.checkState(CreatureState_OnFire))
            {
                if (!Target->states.checkState(CreatureState_Bleeding))
                {
                    //gMessageLib->SendSystemMessage(L"Your target has no states of that type to heal.", Jedi);
                    if (TargetHealthWounds <= 0)
                    {
                        if (TargetActionWounds <= 0)
                        {
                            if (TargetMindWounds <= 0)
                            {
                                if (TargetBattleFatigue <= 0)
                                {
                                    //gMessageLib->SendSystemMessage(L"Your target has no wounds of that type to heal.", Jedi);
                                    if (TargetHealth == TargetMaxHealth)
                                    {
                                        if (TargetAction == TargetMaxAction)
                                        {
                                            if (TargetMind == TargetMaxMind)
                                            {
                                                //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_damage_heal_other"), Jedi);
                                                gMessageLib->SendSystemMessage(L"Your target nothing that needs to be healed.", Jedi);
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

    if (Target->states.checkState(CreatureState_Poisoned))
    {
        gStateManager.removeActionState(Target, CreatureState_Poisoned);
        //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_poisoned"), Target);
        //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "stop_poison_other", 0, Target->getId(), 0), Jedi);
        //gMessageLib->sendStateUpdate(Target);
    }
    if (Target->states.checkState(CreatureState_Diseased))
    {
        gStateManager.removeActionState(Target, CreatureState_Diseased);
        //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_diseased"), Target);
        //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "stop_disease_other", 0, Target->getId(), 0), Jedi);
        //gMessageLib->sendStateUpdate(Target);
    }
    if (Target->states.checkState(CreatureState_OnFire))
    {
        gStateManager.removeActionState(Target, CreatureState_OnFire);
        //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_fire"), Target);
        //gMessageLib->sendStateUpdate(Target);
    }
    if (Target->states.checkState(CreatureState_Bleeding))
    {
        gStateManager.removeActionState(Target, CreatureState_Bleeding);
        //gMessageLib->SendSystemMessage(OutOfBand("dot_message", "stop_bleeding"), Target);
        //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "stop_bleeding_other", 0, Target->getId(), 0), Jedi);
        //gMessageLib->sendStateUpdate(Target);
    }

    //Heals all wounds completely.
    if (TargetHealthWounds > 0)
    {
        Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -TargetHealthWounds);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "health_wounds", L"", TargetHealthWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "health_wounds", L"", TargetHealthWounds), Target);
    }
    if (TargetActionWounds > 0)
    {
        Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -TargetActionWounds);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "action_wounds", L"", TargetActionWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "action_wounds", L"", TargetActionWounds), Target);
    }
    if (TargetMindWounds > 0)
    {
        Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -TargetMindWounds);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "mind_wounds", L"", TargetMindWounds), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "mind_wounds", L"", TargetMindWounds), Target);
    }
    if (TargetBattleFatigue > 0)
    {
        Target->getHam()->updateBattleFatigue(-TargetBattleFatigue);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "battle_fatigue", L"", TargetBattleFatigue), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "battle_fatigue", L"", TargetBattleFatigue), Target);
    }

    int H = TargetMaxHealth - TargetHealth;
    int A = TargetMaxAction - TargetAction;
    int M = TargetMaxMind - TargetMind;

    //Heals all HAM completely.
    if (TargetHealth != TargetMaxHealth)
    {
        Target->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, H);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "health_damage", L"", -H), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "health_damage", L"", -H), Target);
    }
    if (TargetAction != TargetMaxAction)
    {
        Target->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, A);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "action_damage", L"", -A), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "action_damage", L"", -A), Target);
    }
    if (TargetMind != TargetMaxMind)
    {
        Target->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, M);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_self", 0, "", "", L"", Target->getId(), "", "", L"", 0, "jedi_spam", "mind_damage", L"", -M), Jedi);
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "heal_other_other", 0, "", "", L"", Jedi->getId(), "", "", L"", 0, "jedi_spam", "mind_damage", L"", -M), Target);
    }

    Jedi->getHam()->updateCurrentForce(-forceCost);

    //Animation
    gMessageLib->sendCreatureAnimation(Jedi, BString("force_healing_1"));

    return true;
}

//TODO: Implement an efficient way to update the Force Regen rate.
//The Regen Rate/modifier for forcemeditate is *3
bool JediSkillManager::ForceMeditateSelfSkill(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int ForceRegen)
{
    if (Jedi->isMeditating() || Jedi->isForceMeditating())
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "already_in_meditative_state"), Jedi);
        return false;
    }

    // Updates
    Jedi->modifySkillModValue(SMod_jedi_force_power_regen, +ForceRegen);
    Jedi->togglePlayerCustomFlagOn(PlayerCustomFlag_ForceMeditate);
    Jedi->setMeditateState();

    // Schedule Execution
    Jedi->getController()->addEvent(new ForceMeditateEvent(6000), 6000);

    return true;
}

//TODO: Add player damage reductions for Force Run 2 and Force Run 3.
bool JediSkillManager::ForceRunSelfSkill(PlayerObject* Jedi, ObjectControllerCmdProperties* cmdProperties, int SkillLevel)
// Force Run Skill Level
// 1 - Force Run 1 [force run +1, terrain neg. +33]
// 2 - Force Run 2 [force run +2, terrain neg. +66], Additionally, player damage is reduced by 95%.
// 3 - Force Run 3 [force run +3, terrain neg. +99], Additionally while this ability is up, all the player's damage is reduced by 95%.
{
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    //In Pre-CU, you couldn't force run while you were dizzy. (Thanks Shotter for reminding me about this.)
    if (Jedi->states.checkState(CreatureState_Dizzy))
    {
        gMessageLib->SendSystemMessage(L"You are incapable of such speed right now.", Jedi);
        return false;
    }

    //Check if Force Run has already been activated.
    if (Jedi->checkPlayerCustomFlag(PlayerCustomFlag_ForceRun))
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "already_force_running"), Jedi);
        return false;
    }

    //Check for other forms of running.
    if (Jedi->checkPlayerCustomFlag(PlayerCustomFlag_BurstRun))
    {
        gMessageLib->SendSystemMessage(L"You cannot force run right now.", Jedi);
        return false;
    }

    uint32 buffState;
    float speed;
    float acceleration;
    int slope;
    int duration; 
    int forceCost;

    if (SkillLevel = 2)
    {
        buffState = jedi_force_run_2;
        speed = 8.25f;
        acceleration = -.55f;
        slope = 50; //33;
        duration = 120;
        forceCost = 400;
    }
    else if (SkillLevel = 3)
    {
        buffState = jedi_force_run_3;
        speed = 14.25f;
        acceleration = -.55f;
        slope = 50; //55;
        duration = 120;
        forceCost = 600;
    }
    else
    {
        buffState = jedi_force_run_1;
        speed = 2.25f;
        acceleration = -.55f;
        slope = 50; //99;
        duration = 120;
        forceCost = 200;
    }

    if (Jedi->getHam()->getCurrentForce() < forceCost)
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
        return false;
    }

    // Updates
    float new_speed = Jedi->getBaseRunSpeedLimit() + speed;
    float new_acceleration = Jedi->getBaseAcceleration() + acceleration;
    Jedi->modifySkillModValue(SMod_slope_move, +slope); //terrain negotiation mod. Won't work until terrain negotiation is implemented.

    // Implement Speed
    Jedi->setCurrentRunSpeedLimit(new_speed);
    Jedi->setCurrentAcceleration(new_acceleration);
    gMessageLib->sendUpdateMovementProperties(Jedi);

    // Buff Icon
    Buff* forceRun = Buff::SimpleBuff(Jedi, Jedi, duration*1000, buffState, gWorldManager->GetCurrentGlobalTick());
    Jedi->AddBuff(forceRun);

    // Force Cost
    Jedi->getHam()->updateCurrentForce(-forceCost);

    // Schedule Execution
    Jedi->getController()->addEvent(new ForceRunEvent(duration*1000), duration*1000);

    // Client Effect
    gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_run_self.cef", "", Jedi);

    // Update Locomotion
    Jedi->states.setLocomotion(CreatureLocomotion_Running);

    // Toggle the flag for Force Run.
    Jedi->togglePlayerCustomFlagOn(PlayerCustomFlag_ForceRun);

    // Msg to Client
    gMessageLib->SendSystemMessage(OutOfBand("cbt_spam", "forcerun_start_single"), Jedi);

    return true;
}

bool JediSkillManager::TransferForce(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties)
{
    int transferredForce = 200;

    // Perform Checks
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
    return false;
    }

    if (Jedi->getHam()->getCurrentForce() < transferredForce)
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "no_force_power"), Jedi);
        return false;
    }

    if (Jedi == Target)
    {
        gMessageLib->SendSystemMessage(L"You cannot transfer force to yourself.", Jedi);
        return false;
    }

    // You cannot use transfer force on a non-jedi player.
    if (Target->getJediState() == 0)
    {
        //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "not_this_target"), Jedi); //This command cannot be used on this target.
        gMessageLib->SendSystemMessage(L"You can only transfer force to other Jedi.", Jedi);
        return false;
    }

    // Range = 32
    float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

    // Make sure target is in range.
    if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
    {
        gMessageLib->SendSystemMessage(OutOfBand("healing", "no_line_of_sight"), Jedi);
        return false;
    }

    //gMessageLib->sendCreatureAnimation(Jedi, BString("force_transfer_1"));

    // Deduct force power from player.
    Jedi->getHam()->updateCurrentForce(-transferredForce);

    // Play Animation
    gMessageLib->sendCreatureAnimation(Jedi, BString("force_transfer_1"));

    // Transfer force power to friendly jedi.
    Target->getHam()->updateCurrentForce(transferredForce);

    return true;
}

bool JediSkillManager::DrainForce(PlayerObject* Jedi, PlayerObject* Target, ObjectControllerCmdProperties* cmdProperties)
{
    int forceDrained = 100;

    // Perform Checks
    if (Jedi->checkIfMounted())
    {
        gMessageLib->SendSystemMessage(L"You cannot do that while mounted on a creature or vehicle.", Jedi);
        return false;
    }

    if (Jedi == Target)
    {
        gMessageLib->SendSystemMessage(L"You cannot drain force from yourself.", Jedi);
        return false;
    }

    // You cannot drain force from a non-jedi player...obviously.
    if (Target->getJediState() == 0)
    {
        //gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "not_this_target"), Jedi); //This command cannot be used on this target.
        gMessageLib->SendSystemMessage(L"You can only drain force from other Jedi.", Jedi);
        return false;
    }

    // Check if target has enough force to be drained.
    if (Target->getHam()->getCurrentForce() < forceDrained)
    {
        gMessageLib->SendSystemMessage(OutOfBand("jedi_spam", "target_no_force"), Jedi);
        return false;
    }

    // Range = 32
    float distance = gWorldConfig->getConfiguration<float>("Player_heal_distance", (float)32.0);

    // Make sure target is in range.
    if (glm::distance(Jedi->mPosition, Target->mPosition) > distance)
    {
        gMessageLib->SendSystemMessage(OutOfBand("healing", "no_line_of_sight"), Jedi);
        return false;
    }

    // Drain force from enemy jedi.
    Target->getHam()->updateCurrentForce(-forceDrained);

    // Play Animation
    gMessageLib->sendCreatureAnimation(Jedi, BString("force_drain_1"));

    // Add drained force to player.
    Jedi->getHam()->updateCurrentForce(forceDrained);
	
    return true;
}
