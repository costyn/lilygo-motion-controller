import React, { useState, useEffect, useRef } from 'react';
import './DebugConsole.css';

const DebugConsole: React.FC = () => {
  const [isVisible, setIsVisible] = useState(false);
  const [messages, setMessages] = useState<string[]>([]);
  const [connectionStatus, setConnectionStatus] = useState<'disconnected' | 'connecting' | 'connected'>('disconnected');
  const consoleRef = useRef<HTMLDivElement>(null);
  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    if (isVisible && !wsRef.current) {
      // Connect to debug WebSocket
      setConnectionStatus('connecting');
      setMessages(['[Connecting to debug stream...]']);

      const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const wsUrl = `${protocol}//${window.location.host}/debug`;

      const ws = new WebSocket(wsUrl);

      ws.onopen = () => {
        setConnectionStatus('connected');
        setMessages(prev => [...prev, '[Connected to debug stream]']);
      };

      ws.onmessage = (event) => {
        setMessages(prev => [...prev, event.data]);
      };

      ws.onclose = () => {
        wsRef.current = null;
        setConnectionStatus('disconnected');
        setMessages(prev => [...prev, '[Connection closed]']);
      };

      ws.onerror = (error) => {
        console.error('Debug WebSocket error:', error);
        setConnectionStatus('disconnected');
        setMessages(prev => [...prev, '[Connection error - retrying...]']);
      };

      wsRef.current = ws;
    } else if (!isVisible && wsRef.current) {
      // Close WebSocket when hiding console
      wsRef.current.close();
      wsRef.current = null;
      setConnectionStatus('disconnected');
    }

    return () => {
      if (wsRef.current) {
        wsRef.current.close();
        wsRef.current = null;
      }
    };
  }, [isVisible]);

  // Auto-scroll to bottom when new messages arrive
  useEffect(() => {
    if (consoleRef.current) {
      consoleRef.current.scrollTop = consoleRef.current.scrollHeight;
    }
  }, [messages]);

  const handleToggle = () => {
    setIsVisible(!isVisible);
  };

  const handleClear = () => {
    setMessages([]);
  };

  return (
    <div className="debug-console-container">
      <div className="debug-console-controls">
        <button onClick={handleToggle} className="debug-toggle-btn">
          {isVisible ? 'Hide' : 'Show'} Debug Console
          {isVisible && connectionStatus === 'connecting' && ' (Connecting...)'}
        </button>
        {isVisible && (
          <button onClick={handleClear} className="debug-clear-btn">
            Clear
          </button>
        )}
      </div>

      {isVisible && (
        <div ref={consoleRef} className="debug-console">
          {messages.map((msg, index) => (
            <div key={index} className="debug-message">
              {msg}
            </div>
          ))}
        </div>
      )}
    </div>
  );
};

export default DebugConsole;
