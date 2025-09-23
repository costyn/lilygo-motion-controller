# Discovery Answers

**Timestamp:** 2025-09-23 16:30

## Q1: Will the webapp need to work on mobile/tablet devices for field control?
**Answer:** Yes, it should be mobile first. Tablet not likely to be a required form factor, but responsive design is definitely needed.

## Q2: Should the webapp maintain connection state across network interruptions?
**Answer:** The reconnect does not have to be aggressive. Try a couple times and then stop. A reconnect button, like in the LumiApp can be used by the user at a later moment.

## Q3: Will users need to control the motor remotely from different locations/networks?
**Answer:** No, remote access is not needed.

## Q4: Should the webapp store user preferences and settings locally in the browser?
**Answer:** In phase 1, that's fine. Later on we might want to store them on the controller.

## Q5: Will multiple users potentially control the same motor controller simultaneously?
**Answer:** Good point, better not to have multiple controllers be connected at once. Or perhaps if multiple are connected, the client sending commands will have a exclusive lock on the controls for a period of time, (say 10 seconds) after which another client can send commands. But this is a nice to have we can keep till phase 2.