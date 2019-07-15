// -----------------------------------------------------------------------------
// File: setup.ss
// Description: the Default Setup object spawns: Default HUD, Default Camera,
//              Switch Controller, and so on. This is spawned by default on
//              every level, but that can be changed manually (see below)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// Need to hack this?
// It's not recommended to change this object. You may want to copy this object,
// give it a different name and change the "startup" parameter on the .lev file.
//
object "Default Setup"
{
    hud = null;
    cam = spawn("Default Camera");
    switchController = spawn("SwitchController");
    pauseController = spawn("PauseAndQuitController");
    waterController = spawn("WaterController");
    clearedAnim = spawn("DefaultClearedAnimation");
    openingAnim = spawn("DefaultOpeningAnimation");

    state "main"
    {
        // wait for the completion of the opening animation
        if(timeout(3.0)) {
            hud = spawn("DefaultHUD");
            state = "done";
        }
    }

    state "done"
    {
    }
}

// this is for retro-compatibility
object ".default_startup" { startup = spawn("Default Setup"); }
object "DefaultStartup" { startup = spawn("Default Setup"); }