#pragma once

// Defines the operational modes for the fan.
// This is in a separate file to be easily shared across the project.
enum FanMode {
  AUTO,
  MANUAL_ON,
  MANUAL_OFF,
  MANUAL_TIMED // A new mode for when a timer is active
};

// Defines what to do after a timed run completes.
enum PostTimerAction {
  STAY_MANUAL,
  REVERT_TO_AUTO
};

// Holds the state for a manual timed run.
struct ManualTimerState {
  bool isActive = false;
  unsigned long delayEndTime = 0;
  unsigned long timerEndTime = 0;
  PostTimerAction postAction = REVERT_TO_AUTO;
};