import pl = require("TypeScript/pl")
import monitor = require("Features/RenderToTexture/Monitor_data/Monitor")

export class Corridor extends pl.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    monitor1State: number = 0;

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    OnSimulationStarted(): void {
        this.SetTickInterval(pl.Time.Milliseconds(1000));
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "SecretDoorButton") {

            let door = pl.World.TryGetObjectWithGlobalKey("SecretDoor");
            if (door != null) {

                let slider = door.TryGetComponentOfBaseType(pl.SliderComponent);

                if (slider != null && !slider.Running) {

                    // slider direction toggles automatically, just need to set the running state again
                    slider.Running = true;
                }
            }
        }

        if (msg.Message == "MoveA" || msg.Message == "MoveB") {
            let obj = pl.World.TryGetObjectWithGlobalKey("Obj");

            let move = obj.TryGetComponentOfBaseType(pl.MoveToComponent);

            move.Running = true;

            if (msg.Message == "MoveA") {
                move.SetTargetPosition(new pl.Vec3(10, -1, 1.5));
            }
            else {
                move.SetTargetPosition(new pl.Vec3(10, 3, 1.5));
            }
        }

        if (msg.Message == "SwitchMonitor1") {

            ++this.monitor1State;

            if (this.monitor1State > 2)
                this.monitor1State = 0;

            let msg = new monitor.MsgSwitchMonitor();

            switch (this.monitor1State) {
                case 0:
                    msg.screenMaterial = "{ 6c56721b-d71a-4795-88ac-39cae26c39f1 }";
                    msg.renderTarget = "{ 2fe9db45-6e52-4e17-8e27-5744f9e8ada6 }";
                    break;
                case 1:
                    msg.screenMaterial = "{ eb4cb027-44b2-4f69-8f88-3d5594f0fa9d }";
                    msg.renderTarget = "{ 852fa58a-7bea-4486-832b-3a2b2792fea3 }";
                    break;
                case 2:
                    msg.screenMaterial = "{ eb842e16-7314-4f8a-8479-0f92e43ca708 }";
                    msg.renderTarget = "{ 673e8ea0-b70e-4e47-a72b-037d67024a71 }";
                    break;
            }

            let mon = pl.Utils.FindPrefabRootScript<monitor.Monitor>(pl.World.TryGetObjectWithGlobalKey("Monitor1"), "Monitor");

            if (mon != null) {

                // can call the function directly
                mon.OnMsgSwitchMonitor(msg);

                //monitor.SendMessage(msg);
            }
        }
    }

    Tick(): void {
    }
}

