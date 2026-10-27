#pragma once

/*The visible desert is made from three overlapping ground bands:
  layer 3 (far) moves at 0.6, layer 4 (middle) at 0.8, and layer 5
  (nearest) at 1.0.

  Use the factor of the band that actually forms the ground at the asset's
  base Y. Do not interpolate between bands: an interpolated factor makes
  middle-depth props slide across the middle ground.*/
inline constexpr float g_fMiddleGroundStartY = 733.0f;
inline constexpr float g_fNearGroundStartY = 1033.0f;

inline float GetGroundParallaxFactor(float fGroundY)
{
    if (fGroundY >= g_fNearGroundStartY)
    {
        return 1.0f;
    }

    if (fGroundY >= g_fMiddleGroundStartY)
    {
        return 0.8f;
    }

    return 0.6f;
}
