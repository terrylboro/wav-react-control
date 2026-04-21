import { useCallback, useEffect, useRef, useState } from 'react';

export type ReceivedMessage = {
  timestamp: number;
  data: DataView;
};

const DEFAULT_SERVICE_UUID = '12345678-1234-5678-1234-56789abcdef0';
const DEFAULT_CONTROL_CHAR_UUID = '12345678-1234-5678-1234-56789abcdef1';

type UseBleDeviceOptions = {
  initialServiceUUID?: string;
  initialCharUUID?: string;
};

export function useBleDeviceInternal(options?: UseBleDeviceOptions) {
  const [deviceName, setDeviceName] = useState<string | null>(null);
  const [connected, setConnected] = useState(false);

  const [serviceUUID, setServiceUUID] = useState(
    options?.initialServiceUUID ?? DEFAULT_SERVICE_UUID
  );
  const [charUUID, setCharUUID] = useState(
    options?.initialCharUUID ?? DEFAULT_CONTROL_CHAR_UUID
  );

  const [messages, setMessages] = useState<ReceivedMessage[]>([]);
  const [latestMessage, setLatestMessage] = useState<ReceivedMessage | null>(null);
  const [error, setError] = useState<string | null>(null);

  const deviceRef = useRef<BluetoothDevice | null>(null);
  const characteristicRef = useRef<BluetoothRemoteGATTCharacteristic | null>(null);
  const disconnectHandlerRef = useRef<((event: Event) => void) | null>(null);

  const appendMessage = useCallback((value: DataView) => {
    const msg = {
      timestamp: Date.now(),
      data: value,
    };

    setLatestMessage(msg);

    setMessages((prev) => {
      const next = [...prev, msg];
      return next.length > 200 ? next.slice(next.length - 200) : next;
    });
  }, []);

  const onCharacteristicValueChanged = useCallback(
    (event: Event) => {
      const target = event.target as BluetoothRemoteGATTCharacteristic;
      if (target?.value) {
        appendMessage(target.value);
      }
    },
    [appendMessage]
  );

  const encodeCommand = useCallback((command: string) => {
    return new TextEncoder().encode(command.trim());
  }, []);

  const writeCommand = useCallback(
    async (command: string) => {
      const characteristic = characteristicRef.current;

      if (!characteristic) {
        setError('No BLE control characteristic is connected.');
        return false;
      }

      const payload = encodeCommand(command);

      if (payload.length === 0) {
        setError('BLE command cannot be empty.');
        return false;
      }

      try {
        setError(null);

        if (characteristic.properties.writeWithoutResponse) {
          await characteristic.writeValueWithoutResponse(payload);
        } else if (characteristic.properties.write) {
          await characteristic.writeValueWithResponse(payload);
        } else {
          setError('Connected characteristic is not writable.');
          return false;
        }

        return true;
      } catch (e: any) {
        setError(e?.message || String(e));
        return false;
      }
    },
    [encodeCommand]
  );

  const connect = useCallback(async () => {
    setError(null);

    if (!navigator.bluetooth) {
      setError('Web Bluetooth API not available in this browser. Use Chrome or Edge.');
      return false;
    }

    try {
      const trimmedServiceUUID = serviceUUID.trim();
      const trimmedCharUUID = charUUID.trim();

      let options: RequestDeviceOptions;

      if (trimmedServiceUUID) {
        options = {
          filters: [{ services: [trimmedServiceUUID] }],
          optionalServices: [trimmedServiceUUID],
        };
      } else {
        options = {
          acceptAllDevices: true,
          optionalServices: trimmedCharUUID ? [trimmedCharUUID] : undefined,
        };
      }

      const device = await navigator.bluetooth.requestDevice(options);
      deviceRef.current = device;
      setDeviceName(device.name || device.id || 'Unknown');

      const handleDisconnected = () => {
        setConnected(false);
      };

      disconnectHandlerRef.current = handleDisconnected;
      device.addEventListener('gattserverdisconnected', handleDisconnected);

      const server = await device.gatt!.connect();
      if (!trimmedServiceUUID || !trimmedCharUUID) {
        setError('Please provide a service UUID or characteristic UUID.');
        return false;
      }

      const service = await server.getPrimaryService(trimmedServiceUUID);
      const chosenCharacteristic = await service.getCharacteristic(trimmedCharUUID);

      characteristicRef.current = chosenCharacteristic;

      if (!(chosenCharacteristic.properties.write || chosenCharacteristic.properties.writeWithoutResponse)) {
        setError('Selected BLE characteristic does not support writes.');
        return false;
      }

      if (chosenCharacteristic.properties.notify || chosenCharacteristic.properties.indicate) {
        chosenCharacteristic.addEventListener(
          'characteristicvaluechanged',
          onCharacteristicValueChanged as EventListener
        );
        await chosenCharacteristic.startNotifications();
      } else if (chosenCharacteristic.properties.read) {
        const value = await chosenCharacteristic.readValue();
        appendMessage(value);
      }

      setConnected(true);
      return true;
    } catch (e: any) {
      setError(e?.message || String(e));
      setConnected(false);
      return false;
    }
  }, [serviceUUID, charUUID, onCharacteristicValueChanged, appendMessage]);

  const disconnect = useCallback(async () => {
    try {
      if (characteristicRef.current) {
        try {
          await characteristicRef.current.stopNotifications();
        } catch {
          // ignore
        }

        characteristicRef.current.removeEventListener(
          'characteristicvaluechanged',
          onCharacteristicValueChanged as EventListener
        );
        characteristicRef.current = null;
      }

      if (deviceRef.current && disconnectHandlerRef.current) {
        deviceRef.current.removeEventListener(
          'gattserverdisconnected',
          disconnectHandlerRef.current
        );
      }

      if (deviceRef.current?.gatt?.connected) {
        deviceRef.current.gatt.disconnect();
      }

      deviceRef.current = null;
      setConnected(false);
      setDeviceName(null);
    } catch {
      // ignore
    }
  }, [onCharacteristicValueChanged]);

  const clearMessages = useCallback(() => {
    setMessages([]);
  }, []);

  const sendStop = useCallback(() => writeCommand('STOP'), [writeCommand]);
  const sendPause = useCallback(() => writeCommand('PAUSE'), [writeCommand]);
  const sendResume = useCallback(() => writeCommand('RESUME'), [writeCommand]);
  const setVolume = useCallback(
    (volume: number) => writeCommand(`VOL:${Math.max(0, Math.min(255, Math.round(volume)))}`),
    [writeCommand]
  );
  const playFile = useCallback(
    (filename: string) => writeCommand(`PLAY:${filename}`),
    [writeCommand]
  );

  useEffect(() => {
    return () => {
      void disconnect();
    };
  }, [disconnect]);

  return {
    deviceName,
    connected,
    serviceUUID,
    setServiceUUID,
    charUUID,
    setCharUUID,
    messages,
    latestMessage,
    error,
    connect,
    disconnect,
    writeCommand,
    sendStop,
    sendPause,
    sendResume,
    setVolume,
    playFile,
    clearMessages,
  };
}
