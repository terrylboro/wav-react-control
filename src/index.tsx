import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import App from './App';
import reportWebVitals from './reportWebVitals';

import { BleProvider } from './context/BleProvider';

// Mantine theming
import { MantineProvider } from '@mantine/core';
import '@mantine/core/styles.css';
import '@mantine/charts/styles.css';

const root = ReactDOM.createRoot(document.getElementById('root') as HTMLElement);
root.render(
  <React.StrictMode>
    <MantineProvider defaultColorScheme="light">
      <BleProvider>
          <App />
      </BleProvider>
    </MantineProvider> 
  </React.StrictMode>
);

reportWebVitals();
