# Architecture

## What the Continuous Elevator mod does

Found by dumping its cooked Blueprints (CUE4Parse, game usmap not needed for function bytecode):

- `KFMP_Sub_C` is a world subsystem. Its ubergraph runs `Delay(1.0)` in a loop. Each pass does `Int = Int - 1`.
- When `Int <= 0` it checks, per space elevator phase, that the elevator's inventory holds the phase's required stacks
  (`CheckStacks`), removes them (`RemoveStacks`), and sets `Int = 60`. If the stacks are missing it sets the game phase back.
- `Int` is a SaveGame property.
- The building (`Build_CE_C`) only moves items from conveyors into its inventory. It has no timing.

`Delay` is a latent action ticked with the world's delta time. Factories tick separately, on the buildable subsystem's
factory clock (`AFGBuildableSubsystem::GetFactorySimulationTime()`), so on a lagging server the two clocks can advance
at different rates.

## What this mod does

A core ticker (server and hosted games only, never clients) reads the factory clock each tick and accumulates factory
seconds. It then drives the Blueprint's `Int` through reflection:

1. While fewer than 60 factory seconds have passed, `Int` is held at 1000 so the Blueprint cannot reach 0.
2. At 60 factory seconds `Int` is set to 1. The Blueprint's next step brings it to 0 and it consumes as usual.
3. When `Int` reads 60 (the Blueprint's reset) the consume is done; 60 seconds are subtracted from the accumulator
   (the remainder carries over) and holding resumes.

On first sight of the subsystem (after a load), progress is resumed from the saved `Int` (`60 - Int` seconds).
Timing is accurate to about one second, the Blueprint's step. A factory-clock step below 0 or above 10 s is
ignored (load or hitch).

No hooks are used. If the class or the `Int` property is not found, the mod logs once and does nothing.

## Logging

`LogContinuousElevatorSync`: start, subsystem found, seeding, each consume, and once a minute the factory and world
time advanced, with their ratio.
