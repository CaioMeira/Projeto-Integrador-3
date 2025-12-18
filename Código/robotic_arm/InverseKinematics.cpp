#include "InverseKinematics.h"
#include "Config.h"

#include <math.h>

namespace
{
    inline float clampf(float value, float minValue, float maxValue)
    {
        if (value < minValue)
            return minValue;
        if (value > maxValue)
            return maxValue;
        return value;
    }

    inline int clampServoAngle(float value, int servoIdx)
    {
        const int rounded = static_cast<int>(roundf(value));
        return constrain(rounded, minAngles[servoIdx], maxAngles[servoIdx]);
    }

    inline float radToDeg(float rad)
    {
        return rad * (180.0f / PI);
    }

    inline float degToRad(float deg)
    {
        return deg * (PI / 180.0f);
    }

    inline bool isFinite(float value)
    {
        return !isnan(value) && !isinf(value);
    }

    constexpr float EPSILON = 1e-3f;
}

namespace InverseKinematics
{
    bool solveXYZ(float x, float y, float z, int targetAngles[NUM_SERVOS])
    {
        const float planar = sqrtf((x * x) + (y * y));
        const float baseAngleRad = atan2f(y, x);
        const float baseAngleDeg = radToDeg(baseAngleRad) + IK_NEUTRAL.base;
        targetAngles[0] = clampServoAngle(baseAngleDeg, 0);

        const float elevation = z - ARM_KINEMATICS.baseHeightMm;
        const float rawDistance = sqrtf((planar * planar) + (elevation * elevation));
        if (rawDistance < EPSILON)
        {
            return false;
        }

        const float wristExtension = ARM_KINEMATICS.wristOffsetMm + ARM_KINEMATICS.gripperLenMm;

        float clippedDistance = clampf(rawDistance, ARM_KINEMATICS.minReachMm, ARM_KINEMATICS.maxReachMm);
        if (clippedDistance <= wristExtension + 5.0f)
        {
            clippedDistance = wristExtension + 5.0f;
        }

        float wristDistance = clippedDistance - wristExtension;
        if (wristDistance < 5.0f)
        {
            wristDistance = 5.0f;
        }

        const float scale = wristDistance / rawDistance;
        const float effPlanar = planar * scale;
        const float effElevation = elevation * scale;

        const float L1 = ARM_KINEMATICS.upperLenMm;
        const float L2 = ARM_KINEMATICS.forearmLenMm;

        float cosElbow = ((effPlanar * effPlanar) + (effElevation * effElevation) - (L1 * L1) - (L2 * L2)) / (2.0f * L1 * L2);
        cosElbow = clampf(cosElbow, -1.0f, 1.0f);
        const float elbowRad = acosf(cosElbow);

        const float shoulderRad = atan2f(effElevation, effPlanar) - atan2f(L2 * sinf(elbowRad), L1 + (L2 * cosf(elbowRad)));
        if (!isFinite(shoulderRad) || !isFinite(elbowRad))
        {
            return false;
        }

        float targetPitchRad = atan2f(elevation, planar);
        if (!isFinite(targetPitchRad))
        {
            targetPitchRad = 0.0f;
        }
        const float wristPitchRad = targetPitchRad - (shoulderRad + elbowRad);

        targetAngles[1] = clampServoAngle(radToDeg(shoulderRad) + IK_NEUTRAL.shoulder, 1);
        targetAngles[2] = targetAngles[1];
        targetAngles[3] = clampServoAngle(radToDeg(elbowRad) + IK_NEUTRAL.elbow, 3);
        targetAngles[4] = clampServoAngle(radToDeg(wristPitchRad) + IK_NEUTRAL.hand, 4);

        const float wristRotateDeg = IK_NEUTRAL.wristRotate + (baseAngleDeg - IK_NEUTRAL.base);
        targetAngles[5] = clampServoAngle(wristRotateDeg, 5);
        targetAngles[6] = clampServoAngle(IK_NEUTRAL.gripper, 6);

        return true;
    }

    bool estimateXYZ(const int angles[NUM_SERVOS], float &x, float &y, float &z)
    {
        const float baseRad = degToRad(static_cast<float>(angles[0]) - IK_NEUTRAL.base);
        const float shoulderRad = degToRad(static_cast<float>(angles[1]) - IK_NEUTRAL.shoulder);
        const float elbowRad = degToRad(static_cast<float>(angles[3]) - IK_NEUTRAL.elbow);
        const float wristRad = degToRad(static_cast<float>(angles[4]) - IK_NEUTRAL.hand);

        const float L1 = ARM_KINEMATICS.upperLenMm;
        const float L2 = ARM_KINEMATICS.forearmLenMm;
        const float wristExtension = ARM_KINEMATICS.wristOffsetMm + ARM_KINEMATICS.gripperLenMm;

        const float joint2 = shoulderRad;
        const float joint3 = shoulderRad + elbowRad;
        const float joint4 = joint3 + wristRad;

        const float planarReach = (cosf(joint2) * L1) + (cosf(joint3) * L2) + (cosf(joint4) * wristExtension);
        const float verticalOffset = (sinf(joint2) * L1) + (sinf(joint3) * L2) + (sinf(joint4) * wristExtension);

        x = planarReach * cosf(baseRad);
        y = planarReach * sinf(baseRad);
        z = ARM_KINEMATICS.baseHeightMm + verticalOffset;

        return isFinite(x) && isFinite(y) && isFinite(z);
    }
}
