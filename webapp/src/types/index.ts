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
  | ErrorResponse;

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
  | SetLimitHereCommand;

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