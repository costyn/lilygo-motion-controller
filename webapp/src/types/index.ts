// Motor Controller Types based on WebSocket API

export interface MotorStatus {
  type: 'status';
  position: number;
  isMoving: boolean;
  emergencyStop: boolean;
  limitSwitches: {
    min: boolean;
    max: boolean;
  };
  isCalibrating?: boolean;
  isCalibrated?: boolean;
  isManualMinSet?: boolean; // Min set by hand, waiting for Max
  calibrationState?: string;
  encoderFitted?: boolean; // Off means stalls and lost steps can't be detected
  encoderPosition?: number; // Steps travelled since boot, measured by the encoder
  encoderMagnetDetected?: boolean;
  positionError?: number; // Step counter minus encoder, in steps
  stallCount?: number; // Stalls since boot
  lastStallSpeed?: number; // Peak speed of the last stall, in steps/sec
  driverFaults?: number; // TMC2209 overtemperature and short flags since boot
  speed?: number; // Speed slider, shared between browsers
  targetPosition?: number; // Last commanded position, shared between browsers
  pattern?: string; // Name of the running pattern, empty when none
  patternRound?: number; // Round in progress, from 1
  patternRepeat?: number; // Rounds asked for, 0 for until stopped
}

export interface PositionUpdate {
  type: 'position';
  position: number;
}

export interface MotorConfig {
  type: 'config';
  maxSpeed: number;
  acceleration: number;
  minLimit: number;
  maxLimit: number;
  freewheelAfterMove: boolean;
  calibrationSpeed: number;
  calibrationDirectDrive: boolean;
  motorCurrent: number; // mA RMS
  encoderFitted: boolean;
}

// One move of a pattern. Give either seconds or speed.
export interface PatternSegment {
  to: number; // % of travel, 0 is Min and 100 is Max
  seconds?: number; // Duration of the move
  speed?: number; // % of Max Speed, when seconds isn't given
  ramp?: number; // 0.05-0.5: share of the distance spent speeding up, and again slowing down
  hold?: number; // Seconds to wait at the target
}

export interface Pattern {
  name: string;
  repeat: number; // 0 for until stopped
  segments: PatternSegment[];
}

export interface Program {
  presets: number[]; // Quick positions, % of travel
  patterns: Pattern[];
}

export interface ProgramMessage {
  type: 'program';
  program: Program;
}

export interface ConfigUpdatedResponse {
  type: 'configUpdated';
  status: 'success' | 'error';
  message?: string;
}

export interface ErrorResponse {
  type: 'error';
  message: string;
}

export type WebSocketMessage =
  | MotorStatus
  | PositionUpdate
  | MotorConfig
  | ConfigUpdatedResponse
  | ErrorResponse
  | ProgramMessage;

// Command types to send to controller
export interface MoveCommand {
  command: 'move';
  position: number;
  speed: number;
}

export interface EmergencyStopCommand {
  command: 'emergencyStop';
}

export interface ResetCommand {
  command: 'reset';
}

export interface StatusCommand {
  command: 'status';
}

export interface GetConfigCommand {
  command: 'getConfig';
}

export interface SetConfigCommand {
  command: 'setConfig';
  maxSpeed?: number;
  acceleration?: number;
  freewheelAfterMove?: boolean;
  calibrationSpeed?: number;
  calibrationDirectDrive?: boolean;
  motorCurrent?: number;
  encoderFitted?: boolean;
}

export interface JogStartCommand {
  command: 'jogStart';
  direction: 'forward' | 'backward';
  speed: number;
}

export interface JogStopCommand {
  command: 'jogStop';
}

export interface CalibrateCommand {
  command: 'calibrate';
}

export interface SetLimitHereCommand {
  command: 'setMinHere' | 'setMaxHere';
}

export interface SetSpeedCommand {
  command: 'setSpeed';
  speed: number;
}

export interface CycleCommand {
  command: 'cycle';
  count: number;
  speed: number;
}

export interface RunPatternCommand {
  command: 'runPattern';
  index: number;
}

export interface GetProgramCommand {
  command: 'getProgram';
}

// A null program restores the built-in defaults
export interface SetProgramCommand {
  command: 'setProgram';
  program: Program | null;
}

export type ControlCommand =
  | MoveCommand
  | EmergencyStopCommand
  | ResetCommand
  | StatusCommand
  | GetConfigCommand
  | SetConfigCommand
  | JogStartCommand
  | JogStopCommand
  | CalibrateCommand
  | SetLimitHereCommand
  | SetSpeedCommand
  | CycleCommand
  | RunPatternCommand
  | GetProgramCommand
  | SetProgramCommand;

// Connection state
export interface ConnectionState {
  isConnected: boolean;
  isConnecting: boolean;
  reconnectAttempts: number;
  lastError?: string;
}

// Preset positions for localStorage
export interface PresetPosition {
  id: string;
  name: string;
  position: number;
  speed: number;
  createdAt: number;
}

// Debug message
export interface DebugMessage {
  timestamp: number;
  message: string;
}