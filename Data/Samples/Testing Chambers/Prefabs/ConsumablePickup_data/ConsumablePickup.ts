import pl = require("TypeScript/pl")
import _gm = require("Scripting/GameMessages")

export class ConsumablePickup extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    ConsumableType: number = 0;
    Amount: number = 0;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    }

    OnMsgTriggerTriggered(msg: pl.MsgTriggerTriggered): void {

        if (msg.TriggerState == pl.TriggerState.Activated && msg.Message == "Pickup") {

            // TODO: need GO handles in messages to identify who entered the trigger
            let player = pl.World.TryGetObjectWithGlobalKey("Player");
            if (player == null)
                return;

            let hm = new _gm.MsgAddConsumable();
            hm.consumableType = this.ConsumableType;
            hm.amount = this.Amount;

            player.SendMessage(hm, true);

            if (hm.return_consumed == false)
                return;

            let sound = this.GetOwner().TryGetComponentOfBaseType(pl.FmodEventComponent);
            sound.StartOneShot();

            let del = new pl.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    }
}

