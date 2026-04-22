import { Box } from '@mantine/core';
import React, { useState, useEffect } from 'react';

export const AnimatedCircle: React.FC = () => {
  const [isVisible, setIsVisible] = useState(true);

//   useEffect(() => {
//     const interval = setInterval(() => {
//       setIsVisible(prev => !prev);
//     }, 500);

//     return () => clearInterval(interval);
//   }, []);

  return (
    <Box
      style={{
        width: '100%',
        height: '100%',
        minHeight: '400px',
        backgroundColor: 'black',
        transition: 'background-color 0.1s',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        position: 'relative',
        overflow: 'hidden',
      }}
    >
      <Box
        style={{
          width: '90px',
          height: '90px',
          borderRadius: '50%',
          border: isVisible ? '2px solid white' : '2px solid black',
          boxSizing: 'border-box',
          backgroundColor: isVisible ? 'red' : 'black',
          position: 'absolute',
          zIndex: 1,
        }}
      >
        <Box
          style={{
            width: '10px',
            height: '10px',
            borderRadius: '50%',
            backgroundColor: isVisible ? 'white' : 'black',
            position: 'absolute',
            top: '50%',
            left: '50%',
            transform: 'translate(-50%, -50%)',
            zIndex: 2,
          }}
        />
      </Box>
    </Box>
  );
};

// {/* <div
    //     style={{
    //       width: '100px',
    //       height: '100px',
    //       borderRadius: '50%',
    //       backgroundColor: isVisible ? 'red' : 'black',
    //       transition: 'background-color 0.1s',
    //     }}
    //   /> */}
