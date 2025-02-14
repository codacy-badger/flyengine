import pl = require("TypeScript/pl")
import _ge = require("Scripting/GameEnums")
import gun = require("Prefabs/Guns/Gun")

export class Shotgun extends gun.Gun {
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()

        this.singleShotPerTrigger = true;
    }

    static RegisterMessageHandlers() {

        gun.Gun.RegisterMessageHandlers();

        //pl.TypescriptComponent.RegisterMessageHandler(pl.MsgSetColor, "OnMsgSetColor");
    }

    Tick(): void { }

    GetAmmoType(): _ge.Consumable {
        return _ge.Consumable.Ammo_Shotgun;
    }
    
    GetAmmoClipSize(): number {
        return 8;
    }

    Fire(): void {

        let spawn = this.GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(pl.SpawnComponent);
        if (spawn.CanTriggerManualSpawn() == false)
            return;

        this.ammoInClip -= 1;

        for (let i = 0; i < 16; ++i) {
            spawn.TriggerManualSpawn(true, new pl.Vec3(pl.Random.DoubleMinMax(-0.05, 0.05), 0, 0));
        }

        this.PlayShootSound();
    }

}

