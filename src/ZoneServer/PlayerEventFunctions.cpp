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
#include "Datapad.h"
#include "Inventory.h"
#include "MissionManager.h"
#include "ObjectFactory.h"
#include "ResourceManager.h"
#include "ResourceContainer.h"
#include "ResourceType.h"
#include "Buff.h"
#include "UIEnums.h"
#include "UIManager.h"
#include "Heightmap.h"
#include "WaypointObject.h"
#include "WorldManager.h"
#include "DatabaseManager/Database.h"
#include "Utils/clock.h"
#include "MessageLib/MessageLib.h"
#include "Common/LogManager.h"
#include "Common/OutOfBand.h"
#include "NetworkManager/Message.h"
#include "NetworkManager/MessageFactory.h"

#include "utils/rand.h"

#include <algorithm>

using ::common::OutOfBand;

//=============================================================================
// this event manages the logout through the /logout command
//

void PlayerObject::onLogout(const LogOutEvent* event)
{

    if(!this->checkPlayerCustomFlag(PlayerCustomFlag_LogOut))
    {
        return;
    }
    //is it time for logout yet ?
    if(Anh_Utils::Clock::getSingleton()->getLocalTime() <  event->getLogOutTime())
    {
        //tell the time and dust off
        mObjectController.addEvent(new LogOutEvent(event->getLogOutTime(),event->getLogOutSpacer()),event->getLogOutSpacer());
        uint32 timeLeft = (uint32)(event->getLogOutTime()- Anh_Utils::Clock::getSingleton()->getLocalTime())/1000;
        gMessageLib->SendSystemMessage(OutOfBand("logout", "time_left", 0, 0, 0, timeLeft), this);
        return;
    }

    gMessageLib->SendSystemMessage(OutOfBand("logout", "safe_to_log_out"), this);

    gMessageLib->sendLogout(this);
    this->togglePlayerCustomFlagOff(PlayerCustomFlag_LogOut);
    gWorldManager->addDisconnectedPlayer(this);
    //Initiate Logout

}


//=============================================================================
// this event manages the removeal of consumeables - so an object doesnt have to delete itself
// CAVE we only remove it out of the inventory / objectmap
void PlayerObject::onItemDeleteEvent(const ItemDeleteEvent* event)
{

    uint64 now = Anh_Utils::Clock::getSingleton()->getLocalTime();

    //do we have to remove the cooldown?

    Item* item = dynamic_cast<Item*>(gWorldManager->getObjectById(event->getItem()));
    if(!item)
    {
        gLogger->log(LogManager::DEBUG,"PlayerObject::onItemDeleteEvent: Item %I64u not found",event->getItem());
        return;
    }

    TangibleObject* tO = dynamic_cast<TangibleObject*>(gWorldManager->getObjectById(item->getParentId()));
    tO->deleteObject(item);

}

//=============================================================================
// this event manages injury treatment cooldowns.
//
void PlayerObject::onInjuryTreatment(const InjuryTreatmentEvent* event)
{
    uint64 now = gWorldManager->GetCurrentGlobalTick();
    uint64 t = event->getInjuryTreatmentTime();

    if(now > t)
    {
        this->togglePlayerCustomFlagOff(PlayerCustomFlag_InjuryTreatment);
        gMessageLib->SendSystemMessage(::common::OutOfBand("healing_response", "healing_response_58"), this);
    }

    //have to call once more so we can get back here...
    else
    {
        mObjectController.addEvent(new InjuryTreatmentEvent(t), t-now);
    }
}
//=============================================================================
// this event manages quickheal injury treatment cooldowns.
//
void PlayerObject::onQuickHealInjuryTreatment(const QuickHealInjuryTreatmentEvent* event)
{
    uint64 now = gWorldManager->GetCurrentGlobalTick();
    uint64 t = event->getQuickHealInjuryTreatmentTime();

    if(now > t)
    {
        this->togglePlayerCustomFlagOff(PlayerCustomFlag_QuickHealInjuryTreatment);
        gMessageLib->SendSystemMessage(::common::OutOfBand("healing_response", "healing_response_58"), this);
    }

    //have to call once more so we can get back here...
    else
    {
        mObjectController.addEvent(new QuickHealInjuryTreatmentEvent(t), t-now);
    }
}

//=============================================================================
// this event manages wound treatment cooldowns.
//
void PlayerObject::onWoundTreatment(const WoundTreatmentEvent* event)
{
    uint64 now = gWorldManager->GetCurrentGlobalTick();
    uint64 t = event->getWoundTreatmentTime();

    if(now >  t)
    {
        this->togglePlayerCustomFlagOff(PlayerCustomFlag_WoundTreatment);

        gMessageLib->SendSystemMessage(::common::OutOfBand("healing_response", "healing_response_59"), this);
    }
    //have to call once more so we can get back here...
    else
    {
        mObjectController.addEvent(new WoundTreatmentEvent(t), t-now);
    }
}

//=============================================================================
// this event manages forcerun.
//
void PlayerObject::onForceRun(const ForceRunEvent* event)
{
    uint64 now = gWorldManager->GetCurrentGlobalTick();
    uint64 t = event->getEndTime();

    //do we have to remove the force run??
    if (now > t)
    {
        if (this->checkPlayerCustomFlag(PlayerCustomFlag_ForceRun))
        {
            gMessageLib->SendSystemMessage(OutOfBand("cbt_spam", "forcerun_stop_single"), this);
            //I don't really remember if there was even any combat spam that was seen by other players for this.
            //Combat Spam
            int8 s[256];
            sprintf(s, "%s %s slows down.", this->getFirstName().getAnsi(), this->getLastName().getAnsi());
            BString bs(s);
            bs.convert(BSTRType_Unicode16);
            gMessageLib->sendCombatSpam(this, this, 0, "", "", 0, 0, bs.getUnicode16());

            this->togglePlayerCustomFlagOff(PlayerCustomFlag_ForceRun);

            this->modifySkillModValue(SMod_slope_move, -50);
            this->setCurrentRunSpeedLimit(this->getBaseRunSpeedLimit());
            this->setCurrentAcceleration(this->getBaseAcceleration());
            gMessageLib->sendUpdateMovementProperties(this);

            this->setLocomotion(kLocomotionStanding);

            //this->setUpright();
        }
    }
    //have to call once more so we can get back here...
    else
    {
        mObjectController.addEvent(new ForceRunEvent(t), t - now);
    }
}

//=============================================================================
// this event manages force meditate.
// it makes sure the client effect is played in a loop.
//
void PlayerObject::onForceMeditate(const ForceMeditateEvent* event)
{
    uint64 now = gWorldManager->GetCurrentGlobalTick();
    uint64 t = event->getEffectTime();

    //do we need to play the effect again??
    if (now > t)
    {
        if (this->isMeditating())
        {
            gMessageLib->sendPlayClientEffectObjectMessage("clienteffect/pl_force_meditate_self.cef", "", this);

            //this makes it so the event loops, thus causing the client effect to loop again and again...until we stop force meditating.
            mObjectController.addEvent(new ForceMeditateEvent(6000), 6000);
        }
    }
    //have to call once more so we can get back here...
    else
    {
        mObjectController.addEvent(new ForceMeditateEvent(t), t - now);
    }
}

//=============================================================================
// this event manages teras kasi meditate.
// also manages healing tickets for meditation.
// see - http://wiki.swganh.org/index.php/Meditate_(Ability)
//
void PlayerObject::onMeditate(const MeditateEvent* event)
{
    uint64 now = gWorldManager->GetCurrentGlobalTick();
    uint64 t = event->getTickTime();

    //do we need to tick again??
    if (now > t)
    {
        if (this->isMeditating())
        {
            int healAmount;
            int meditateMod = this->getSkillModValue(SMod_meditate);
            Ham* playerHam = this->getHam();

            //*DOT Healing*
            // DOTs are healed in a particular order which is unknown.
            // however the purposed order (which can be found on the wikipage) is: [Heal bleeds, poisons, then diseases]
            //TODO: Meditation DOT healing.

            //*Wound Healing*
            // Player must have +75 Meditate SkillMod (Master Meditative Techniques skillbox) in order to be able to heal wounds.
            if (meditateMod >= 75)
            {
                int HealthWounds = playerHam->mHealth.getWounds();
                int StrengthWounds = playerHam->mStrength.getWounds();
                int ConstitutionWounds = playerHam->mConstitution.getWounds();
                int ActionWounds = playerHam->mAction.getWounds();
                int QuicknessWounds = playerHam->mQuickness.getWounds();
                int StaminaWounds = playerHam->mStamina.getWounds();
                int MindWounds = playerHam->mMind.getWounds();
                int FocusWounds = playerHam->mFocus.getWounds();
                int WillpowerWounds = playerHam->mWillpower.getWounds();
				
                if (meditateMod > 0 && meditateMod < 100)
                    healAmount = (gRandom->getRand() % 20) + 10; //10-20
                else if (meditateMod >= 100)
                    //some documentation says its 20-30 and a few say its 20-35. I need someone to verify which one should be used. So for now I'll put it at 20-30.
                    healAmount = (gRandom->getRand() % 30) + 20; //20-30
                    //heal = (gRandom->getRand() % 35) + 20; //20-35
                else
                    return;

                // Wound healing order - [Action, Health and Mind] (including secondary stats.)
                // Meditation only heals one wound pool per tick.
                // The (if, else if, ...) statements will make sure that only one wound pool is healed per tick. 
                // If a wound pool = 0, then it will move onto the next wound pool until it finds a wound pool with wounds.
                if (ActionWounds > 0)
                {
                    if (ActionWounds < healAmount)
                        healAmount = ActionWounds;

                    playerHam->updatePropertyValue(HamBar_Action, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "action_wounds", healAmount), this);
                }
                else if (QuicknessWounds > 0)
                {
                    if (QuicknessWounds < healAmount)
                        healAmount = QuicknessWounds;

                    playerHam->updatePropertyValue(HamBar_Quickness, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "quickness_wounds", healAmount), this);
                }
                else if (StaminaWounds > 0)
                {
                    if (StaminaWounds < healAmount)
                        healAmount = StaminaWounds;

                    playerHam->updatePropertyValue(HamBar_Stamina, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "stamina_wounds", healAmount), this);
                }
                else if (HealthWounds > 0)
                {
					if (HealthWounds < healAmount)
                        healAmount = HealthWounds;

                    playerHam->updatePropertyValue(HamBar_Health, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "health_wounds", healAmount), this);
                }
                else if (StrengthWounds > 0)
                {
                    if (StrengthWounds < healAmount)
                        healAmount = StrengthWounds;

                    playerHam->updatePropertyValue(HamBar_Strength, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "strength_wounds", healAmount), this);
                }
                else if (ConstitutionWounds > 0)
                {
                    if (ConstitutionWounds < healAmount)
                        healAmount = ConstitutionWounds;

                    playerHam->updatePropertyValue(HamBar_Constitution, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "constitution_wounds", healAmount), this);
                }
                else if (MindWounds > 0)
                {
                    if (MindWounds < healAmount)
                        healAmount = MindWounds;

                    playerHam->updatePropertyValue(HamBar_Mind, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "mind_wounds", healAmount), this);
                }
                else if (FocusWounds > 0)
                {
                    if (FocusWounds < healAmount)
                        healAmount = FocusWounds;

                    playerHam->updatePropertyValue(HamBar_Focus, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "focus_wounds", healAmount), this);
                }
                else if (WillpowerWounds > 0)
                {
                    if (WillpowerWounds < healAmount)
                        healAmount = WillpowerWounds;

                    playerHam->updatePropertyValue(HamBar_Willpower, HamProperty_Wounds, -healAmount);
                    gMessageLib->SendSystemMessage(OutOfBand("teraskasi", "prose_curewound", "", "", "", "", "jedi_spam", "willpower_wounds", healAmount), this);
                }
                else // Either there are no wounds to heal, or all the wound pools have been healed.
                {
                    return;
                }
            }

            // Loop - tick again
            mObjectController.addEvent(new MeditateEvent(5000), 5000);
        }
    }
    //have to call once more so we can get back here...
    else
    {
        mObjectController.addEvent(new MeditateEvent(t), t - now);
    }
}