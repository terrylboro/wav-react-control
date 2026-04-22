import {
  Button,
  Card,
  Stack,
  Text,
  Title,
  Grid,
  Box,
  useMantineTheme,
} from '@mantine/core';

import { useBleDevice } from '../context/BleProvider';
import LiveChartCard from './LiveChartCard';
import { AnimatedCircle } from './AnimatedCircle';

type SetupScreenProps = {
  bleStatus: 'disconnected' | 'connecting' | 'connected' | 'error';
  deviceName: string | null;
  bleError: string | null;
  onConnect: () => void;
  onDisconnect: () => void;
};

export default function SetupScreen({
  bleStatus,
  deviceName,
  bleError,
  onConnect,
  onDisconnect,
}: SetupScreenProps) {
  const theme = useMantineTheme();
  const ble = useBleDevice();

  return (
    <Box
      w="100%"
      px="xs"
      py="xs"
      style={{
        display: 'flex',
        flexDirection: 'column',
        alignSelf: 'stretch',
      }}
    >
      <Stack w="100%" gap="xs">
        <Grid gutter="xs" align="stretch">
          {/* <Grid.Col span={12}>
            <LiveChartCard />
          </Grid.Col> */}
          <Grid.Col span={{ base: 12, sm: 4 }}>
            <Card
              withBorder
              shadow="sm"
              radius="md"
              p="xl"
              w="100%"
              style={{ minHeight: 240 }}
            >
              <Stack h="100%" justify="space-between" align="stretch">
                <Box>
                  <Title order={1} mb="sm">
                    Exercise 1
                  </Title>
                  <Text c="dimmed" size="sm">
                    Make sure you connect to your Bluetooth device before starting treatment!
                  </Text>
                </Box>

                <Stack gap="md">
                <Button fullWidth size="xl" onClick={onConnect} loading={bleStatus === 'connecting'} color={bleStatus === 'connected' ? theme.colors.green[6] : theme.colors.blue[6]}>
                  {bleStatus === 'connected' ? 'Connected' : 'Connect'}
                </Button>
                {deviceName && (
                  <Text size="sm" c="dimmed">
                    {deviceName}
                  </Text>
                )}
                {bleError && (
                  <Text size="sm" c="red">
                    {bleError}
                  </Text>
                )}

                <Button fullWidth size="xl" onClick={() => ble.playFile("INST.WAV")} disabled={bleStatus !== 'connected'}>
                  Instructions
                </Button>
                <Button fullWidth size="xl" onClick={() => ble.playFile("1HZ.WAV")} disabled={bleStatus !== 'connected'}>
                  Start Exercise
                </Button>
                <Button fullWidth size="xl" onClick={() => ble.playFile("SAMPL~16.WAV")} disabled={bleStatus !== 'connected'}>
                  Play Sound
                </Button>
                </Stack>
              </Stack>
            </Card>
          </Grid.Col>

          <Grid.Col span={{ base: 12, sm: 5 }}>
            <Card
              withBorder
              shadow="sm"
              radius="md"
              p="xl"
              style={{ minHeight: 465, maxHeight: 400, minWidth: 800, maxWidth: 1000 }}
            >
            <AnimatedCircle />
            </Card>
          </Grid.Col>

          
        </Grid>
      </Stack>
    </Box>
  );
}
