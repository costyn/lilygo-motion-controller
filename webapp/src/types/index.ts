// Motor Controller Types based on WebSocket API

export interface MotorStatus {
  type: 'status';
  position: number;
  isMoving: boolean;
  emergencyStop: boolean;
  limitSwitches: {
    min: boolean;
    max: boolean;
    any: boolean;
  };
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
  useStealthChop: boolean;
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

export interface StopCommand {
  command: 'stop';
}

export interface EmergencyStopCommand {
  command: 'emergency-stop';
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
  useStealthChop?: boolean;
}

export interface JogStartCommand {
  command: 'jogStart';
  direction: 'forward' | 'backward';
}

export interface JogStopCommand {
  command: 'jogStop';
}

export type ControlCommand =
  | MoveCommand
  | StopCommand
  | EmergencyStopCommand
  | ResetCommand
  | StatusCommand
  | GetConfigCommand
  | SetConfigCommand
  | JogStartCommand
  | JogStopCommand;

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