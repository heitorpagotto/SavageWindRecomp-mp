#include <stdio.h>

#include "multiplayer_assets.h"

static int failures;

static void expect_action(SwmpProxyAction actual, SwmpProxyAction expected) {
    if (actual == expected) return;
    fprintf(stderr, "multiplayer animation: expected %s, got %s\n",
            swmp_proxy_action_name(expected), swmp_proxy_action_name(actual));
    failures++;
}

int main(void) {
    SwmpGearSet gear = {.id = {SWMP_GEAR_ID(SPEAR, 2), SWMP_GEAR_ID(SHIELD, 14), SWMP_GEAR_ID(BOW, 72)}};
    // Before a bow reaches the hand, the stowed spear can enter the melee hand-height range.
    SwmpGearPose spear = {.offset_mm = {-623, 547, -200}};
    SwmpGearPose bow = {.offset_mm = {9, 1090, -118}};
    expect_action(swmp_proxy_sample_action("WeaponEquipOn", "", gear, &spear, &bow, false, 20),
                  SWMP_PROXY_BOW_DRAW);
    expect_action(swmp_proxy_sample_action("WeaponEquipOn", "", gear, &spear, &bow, false, 0),
                  SWMP_PROXY_IDLE);
    expect_action(swmp_proxy_sample_action("WeaponEquipOn", "", gear, &spear, &bow, false, 32),
                  SWMP_PROXY_SPEAR_DRAW);
    expect_action(swmp_proxy_sample_action("WeaponEquipOn", "", gear, &spear, &bow, false, 16),
                  SWMP_PROXY_SHIELD_DRAW);
    expect_action(swmp_proxy_sample_action("WaitAttention", "", gear, NULL, NULL, true, 0),
                  SWMP_PROXY_SHIELD_GUARD);
    expect_action(swmp_proxy_sample_action("GuardFailHit", "", gear, NULL, NULL, true, 0),
                  SWMP_PROXY_SHIELD_HIT);
    expect_action(swmp_proxy_sample_action("SurfingSlide", "", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_SURF_RIDE);
    expect_action(swmp_proxy_sample_action("CutShieldRide", "BowCharge", gear, NULL, NULL, false, 53),
                  SWMP_PROXY_SURF_ATTACK);
    expect_action(swmp_proxy_sample_action("SurfingAir", "CutShieldRide", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_SURF_ATTACK);
    expect_action(swmp_proxy_sample_action("StaminaOut", "", gear, NULL, NULL, false, 20),
                  SWMP_PROXY_STAMINA_OUT_START);
    expect_action(swmp_proxy_sample_action("StaminaOut", "", gear, NULL, NULL, false, 44),
                  SWMP_PROXY_STAMINA_OUT_WAIT);
    expect_action(swmp_proxy_sample_action("StaminaOutEd", "BowCharge", gear, NULL, NULL, false, 70),
                  SWMP_PROXY_STAMINA_OUT_END);
    expect_action(swmp_proxy_sample_action("Wait", "HorseCall", (SwmpGearSet){0}, NULL, NULL, false, 0),
                  SWMP_PROXY_WHISTLE);
    expect_action(swmp_proxy_sample_action("Move", "HorseCall", (SwmpGearSet){0}, NULL, NULL, false, 0),
                  SWMP_PROXY_WHISTLE_MOVE);
    expect_action(swmp_proxy_sample_action("SquatWait", "HorseCall", (SwmpGearSet){0}, NULL, NULL, false, 0),
                  SWMP_PROXY_WHISTLE_CROUCH);
    expect_action(swmp_proxy_sample_action("ClimbSlip", "", gear, NULL, NULL, false, 30),
                  SWMP_PROXY_CLIMB_SLIP);
    expect_action(swmp_proxy_sample_action("ClimbSlip", "", gear, NULL, NULL, false, 18),
                  SWMP_PROXY_CLIMB_SLIP_FALL);
    expect_action(swmp_proxy_sample_action("ClimbMove", "HorseCall", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_CLIMB_MOVE);

    SwmpGearSet sword_shield = {.id = {SWMP_GEAR_ID(SWORD, 1), SWMP_GEAR_ID(SHIELD, 14)}};
    SwmpGearPose drawn_sword = {.offset_mm = {-500, 750, 120}};
    SwmpGearPose stowed_sword = {.offset_mm = {-250, 1120, -200}};
    expect_action(swmp_proxy_sample_action("MoveAttention", "MoveAttentionUpper", sword_shield,
                                           &drawn_sword, NULL, true, 0), SWMP_PROXY_SWORD_GUARD_WALK_FORWARD);
    expect_action(swmp_proxy_sample_action("WaitAttention", "WaitAttentionUpper", sword_shield,
                                           &drawn_sword, NULL, true, 0), SWMP_PROXY_SWORD_GUARD);
    expect_action(swmp_proxy_sample_action("MoveAttention", "MoveAttentionUpper", sword_shield,
                                           &stowed_sword, NULL, true, 0), SWMP_PROXY_SHIELD_WALK_FORWARD);
    expect_action(swmp_proxy_sample_action("MoveAttention", "MoveAttentionUpper", sword_shield,
                                           &drawn_sword, NULL, false, 0), SWMP_PROXY_IDLE);
    expect_action(swmp_proxy_sample_action("LadderWait", "", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_LADDER_WAIT);
    expect_action(swmp_proxy_sample_action("LadderUp", "", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_LADDER_UP);
    expect_action(swmp_proxy_sample_action("LadderDown", "", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_LADDER_DOWN);
    expect_action(swmp_proxy_sample_action("ClimbTreeTopSharp", "", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_POLE_HOLD);
    expect_action(swmp_proxy_sample_action("Warp", "", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_WARP_DEPART);
    expect_action(swmp_proxy_sample_action("ClimbEd", "", gear, NULL, NULL, false, 24),
                  SWMP_PROXY_CLIMB_MANTLE);
    expect_action(swmp_proxy_sample_action("WaterDiveJump", "", gear, NULL, NULL, false, 30),
                  SWMP_PROXY_WATER_DIVE);
    expect_action(swmp_proxy_sample_action("SwimWait", "", gear, NULL, NULL, false, 24),
                  SWMP_PROXY_WATER_SURFACE);
    expect_action(swmp_proxy_sample_action("SwimWait", "", gear, NULL, NULL, false, 30),
                  SWMP_PROXY_SWIM_WAIT);
    expect_action(swmp_proxy_sample_action("SwimJumpSt", "", gear, NULL, NULL, false, 30),
                  SWMP_PROXY_SWIM_JUMP);
    expect_action(swmp_proxy_sample_action("SwimJump", "", gear, NULL, NULL, false, 30),
                  SWMP_PROXY_SWIM_JUMP);
    gear.id[SWMP_GEAR_SHIELD] = 0;
    expect_action(swmp_proxy_sample_action("GuardBreak", "", gear, NULL, NULL, false, 0),
                  SWMP_PROXY_SHIELD_BREAK);

    expect_action(swmp_proxy_throw_action("WeaponThrowCharge", SWMP_GEAR_FAMILY_SWORD, 20),
                  SWMP_PROXY_SWORD_THROW_WINDUP);
    expect_action(swmp_proxy_throw_action("WeaponThrowCharge", SWMP_GEAR_FAMILY_LSWORD, 24),
                  SWMP_PROXY_LSWORD_THROW_WINDUP);
    expect_action(swmp_proxy_throw_action("WeaponThrowCharge", SWMP_GEAR_FAMILY_SPEAR, 20),
                  SWMP_PROXY_SPEAR_THROW_WINDUP);
    expect_action(swmp_proxy_throw_action("WeaponThrowCharge", SWMP_GEAR_FAMILY_SWORD, 36),
                  SWMP_PROXY_SWORD_THROW_AIM);
    expect_action(swmp_proxy_throw_action("WeaponThrowCharge", SWMP_GEAR_FAMILY_LSWORD, 36),
                  SWMP_PROXY_LSWORD_THROW_AIM);
    expect_action(swmp_proxy_throw_action("WeaponThrowCharge", SWMP_GEAR_FAMILY_SPEAR, 36),
                  SWMP_PROXY_SPEAR_THROW_AIM);
    expect_action(swmp_proxy_throw_action("WeaponThrow", SWMP_GEAR_FAMILY_SWORD, 28),
                  SWMP_PROXY_SWORD_THROW_RELEASE);
    expect_action(swmp_proxy_throw_action("WeaponThrow", SWMP_GEAR_FAMILY_LSWORD, 34),
                  SWMP_PROXY_LSWORD_THROW_RELEASE);
    expect_action(swmp_proxy_throw_action("WeaponThrow", SWMP_GEAR_FAMILY_SPEAR, 28),
                  SWMP_PROXY_SPEAR_THROW_RELEASE);
    expect_action(swmp_proxy_throw_action("WeaponThrow", SWMP_GEAR_FAMILY_SWORD, 21),
                  SWMP_PROXY_SWORD_SWING);
    expect_action(swmp_proxy_throw_action("WeaponThrow", SWMP_GEAR_FAMILY_LSWORD, 60),
                  SWMP_PROXY_AXE_SWING);
    expect_action(swmp_proxy_throw_action("WeaponThrowCatch", SWMP_GEAR_FAMILY_SWORD, 27),
                  SWMP_PROXY_SWORD_THROW_CATCH);
    expect_action(swmp_proxy_throw_action("WeaponThrowCatch", SWMP_GEAR_FAMILY_LSWORD, 27),
                  SWMP_PROXY_LSWORD_THROW_CATCH);
    expect_action(swmp_proxy_throw_action("WeaponThrowCatch", SWMP_GEAR_FAMILY_SPEAR, 27),
                  SWMP_PROXY_IDLE);
    expect_action(swmp_proxy_throw_action("WeaponThrow", SWMP_GEAR_FAMILY_NONE, 28),
                  SWMP_PROXY_IDLE);
    expect_action(swmp_proxy_carry_action(SWMP_PROXY_IDLE, 2), SWMP_PROXY_HOLD_FEW);
    expect_action(swmp_proxy_carry_action(SWMP_PROXY_WALK, 5), SWMP_PROXY_WALK_HOLD_MANY);
    expect_action(swmp_proxy_carry_action(SWMP_PROXY_RUN, 2), SWMP_PROXY_RUN_HOLD_FEW);
    expect_action(swmp_proxy_carry_action(SWMP_PROXY_PUT_MATERIALS, 0), SWMP_PROXY_PUT_MATERIALS);
    expect_action(swmp_horse_rider_action(swmp_horse_gait("Move_Gear3"), "HorseRun", "", "", SWMP_GEAR_FAMILY_NONE, false),
                  SWMP_PROXY_HORSE_CANTER);
    expect_action(swmp_horse_rider_action(SWMP_HORSE_WALK, "HorseRun", "CutHorseRide", "", SWMP_GEAR_FAMILY_SWORD, false),
                  SWMP_PROXY_HORSE_SWORD_RIGHT);
    expect_action(swmp_horse_rider_action(SWMP_HORSE_WAIT, "CutHorseRide", "", "", SWMP_GEAR_FAMILY_LSWORD, true),
                  SWMP_PROXY_HORSE_LSWORD_LEFT);
    expect_action(swmp_horse_rider_action(SWMP_HORSE_TROT, "HorseRun", "", "CutChargeHorseRide", SWMP_GEAR_FAMILY_SPEAR, false),
                  SWMP_PROXY_HORSE_SPEAR_RIGHT);
    expect_action(swmp_horse_rider_action(SWMP_HORSE_WALK, "HorseRun", "AimHorseLowerF", "BowReload", SWMP_GEAR_FAMILY_NONE, false),
                  SWMP_PROXY_HORSE_BOW_LOAD);
    expect_action(swmp_horse_rider_action(SWMP_HORSE_WALK, "HorseRun", "AimHorseLowerB", "BowCharge", SWMP_GEAR_FAMILY_NONE, false),
                  SWMP_PROXY_HORSE_BOW_BACK_AIM);
    expect_action(swmp_horse_rider_action(SWMP_HORSE_TROT, "HorseRun", "AimHorseLowerF", "BowShoot", SWMP_GEAR_FAMILY_NONE, false),
                  SWMP_PROXY_HORSE_BOW_SHOOT);
    expect_action(swmp_horse_rider_action(SWMP_HORSE_TROT, "HorseRun", "", "HorseBowEndUpper", SWMP_GEAR_FAMILY_NONE, false),
                  SWMP_PROXY_HORSE_BOW_END);
    if (!failures) puts("multiplayer animation selector: ok");
    return failures != 0;
}
