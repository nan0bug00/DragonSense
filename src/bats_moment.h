#pragma once

namespace BatsMoment
{
    // Installs the script-event hook. Call at kDataLoaded.
    void OnDataLoaded();

    // Resets the one-shot guard. Call on kNewGame / kPostLoadGame.
    void OnLoad();
}
