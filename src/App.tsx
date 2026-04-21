import './App.css';
import { useBleDevice } from './context/BleProvider';

import MainScreen from './components/MainScreen';

// Mantine UI imports
import {
  AppShell,
  useMantineTheme,
} from '@mantine/core';

function App(): JSX.Element { 

  const ble = useBleDevice();

 
  // Mantine theming
  const theme = useMantineTheme();

  return (
      <AppShell
        header={{ height: 10 }}
        padding="md"
        style={{ height: '100vh' }}
      >
        <AppShell.Main
          style={{
            minHeight: 'calc(100vh - 60px)',
            overflowY: 'auto',
            backgroundColor: theme.colors.gray[0],
          }}
        >
        <MainScreen 
          bleStatus={ble.connected ? 'connected' : 'disconnected'}
          deviceName={ble.deviceName}
          bleError={ble.error}
          onConnect={ble.connect}
          onDisconnect={ble.disconnect}
        />
        </AppShell.Main>
    </AppShell>
  );
}

export default App;
