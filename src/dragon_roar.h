#pragma once

namespace DragonRoar
{
    // Builds the roar record table and installs the sound-record hook.
    // Must run at kDataLoaded: the form arrays do not exist before it.
    void OnDataLoaded();
}
