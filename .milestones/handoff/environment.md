# Time, weather, and lobby notices

## M2.5 first slice

- The default-on **Sync host time and weather** preference is shared by F1 and System > Options.
  The host sends time and effective weather with position batches. Clients apply fresh samples within
  the same area when gameplay is ready. The passive Link actor remains independent of this path.
- Join and disconnect notices render above gameplay for five seconds. The host and existing clients see
  a named notice when a peer joins or leaves. An admitted player also gets a local notice when their
  session ends. Late joiners receive the current roster without a burst of notices.
- The 1.9.0 `mp_weather_current` site resolves the world manager's effective-weather getter. The prior
  cheat override field returns 255 during ordinary forecasts and cannot serve as the host's sole source.
  The environment and disconnect checks used game library `13365c313591b27e`.
  See the [M2.5 test record](../testing/environment.md#m25-first-slice) for the tested builds,
  Windows validation, and remaining cases.
- For overhead name feasibility, `control_camera_view()` in `runtime/nvn/vulkan/device.c` currently follows
  the motion-vector path. A default-on label needs a current world view and projection in other AA modes
  too, and must use the world viewport and the active proxy's applied pose.
