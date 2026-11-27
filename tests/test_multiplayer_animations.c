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
    if (!failures) puts("multiplayer animation selector: ok");
    return failures != 0;
}
