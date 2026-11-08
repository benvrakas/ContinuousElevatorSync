## Continuous Elevator Sync

A small add-on for the **Continuous Elevator** mod (Keep Feeding To Maintain Phase). That mod removes the parts for the
current phase from the elevator once a minute, and sets the game phase back if they aren't there.

It counts that minute with a timer on world time, while your factory runs on its own factory time. On a server that
is running slowly, the factory makes fewer parts per real second, but the elevator still eats them once a real minute,
so it goes hungry and the phase drops.

This mod makes the elevator count factory time instead. When the server runs slow, the elevator slows down with the
factory, so a lagging server no longer starves it. On a healthy server nothing changes: a minute is still a minute.

### Who needs it

**Only the server**: your dedicated server, or the player hosting the game. Players joining don't need it.
It needs the Continuous Elevator mod (KeepFeedingToMaintainPhase) and does nothing without it.

### Notes

- It doesn't change what is consumed, only when. The Continuous Elevator mod still does the consuming.
- The server log (`LogContinuousElevatorSync`) shows once a minute how far factory time and world time advanced,
  so you can see how slow the server is running.
- If a future version of the Continuous Elevator mod changes its internals, this mod logs an error and does nothing,
  and the elevator goes back to counting world time.
