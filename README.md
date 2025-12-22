# QR Code Detector

Node.js native addon for detecting and decoding QR codes using OpenCV. This module runs QR code detection in separate worker threads to ensure non-blocking operation.

## Features

- **Single QR Code Detection**: Detect and decode a single QR code in an image
- **Multiple QR Code Detection**: Detect and decode multiple QR codes in a single image
- **Quick Detection**: Check if an image contains a QR code without decoding
- **Non-blocking**: Uses worker threads for asynchronous processing
- **Multiple Input Formats**: Supports both file paths and image buffers
- **Corner Detection**: Returns corner coordinates of detected QR codes
- **Base64 Image Export**: Extracted QR code region as base64-encoded PNG

## Installation

```bash
npm install
```

### Prerequisites

- Node.js >= 14.0.0
- OpenCV 4.x installed on your system

#### macOS

```bash
brew install opencv
```

#### Linux

```bash
sudo apt-get install libopencv-dev
```

## Usage

### Detect Single QR Code

```javascript
const { detectQRCode } = require('./qr-detector');

// Using file path
const result = await detectQRCode('/path/to/image.jpg');
console.log(result);
// {
//   detected: true,
//   data: 'https://example.com',
//   corners: [
//     { x: 100, y: 100 },
//     { x: 200, y: 100 },
//     { x: 200, y: 200 },
//     { x: 100, y: 200 }
//   ],
//   qrCodeImage: 'data:image/png;base64,iVBORw0KGgo...'
// }

// Using buffer
const fs = require('fs');
const imageBuffer = fs.readFileSync('/path/to/image.jpg');
const result = await detectQRCode(imageBuffer);
```

### Detect Multiple QR Codes

```javascript
const { detectMultipleQRCodes } = require('./qr-detector');

const result = await detectMultipleQRCodes('/path/to/image.jpg');
console.log(result);
// {
//   detected: true,
//   count: 2,
//   qrCodes: [
//     {
//       data: 'https://example.com',
//       corners: [...]
//     },
//     {
//       data: 'Another QR code data',
//       corners: [...]
//     }
//   ]
// }
```

### Quick Detection (Without Decoding)

```javascript
const { hasQRCode } = require('./qr-detector');

const result = await hasQRCode('/path/to/image.jpg');
console.log(result);
// {
//   hasQRCode: true,
//   corners: [...]
// }
```

## API

### `detectQRCode(input)`

Detects and decodes a single QR code in an image.

**Parameters:**

- `input` (string|Buffer): Image file path or buffer

**Returns:** Promise<Object>

- `detected` (boolean): Whether a QR code was detected
- `data` (string|null): Decoded QR code data
- `corners` (Array): Corner points of the QR code
- `qrCodeImage` (string): Base64-encoded PNG of extracted QR code (format: `data:image/png;base64,...`)

### `detectMultipleQRCodes(input)`

Detects and decodes multiple QR codes in an image.

**Parameters:**

- `input` (string|Buffer): Image file path or buffer

**Returns:** Promise<Object>

- `detected` (boolean): Whether any QR codes were detected
- `count` (number): Number of QR codes detected
- `qrCodes` (Array): Array of detected QR codes with data and corners

### `hasQRCode(input)`

Checks if an image contains a QR code without decoding it (faster).

**Parameters:**

- `input` (string|Buffer): Image file path or buffer

**Returns:** Promise<Object>

- `hasQRCode` (boolean): Whether a QR code was detected
- `corners` (Array): Corner points of the QR code

## Architecture

This module follows the same architecture as the camera-sabotage-detector:

1. **C++ Native Addon**: Uses OpenCV's QRCodeDetector for high-performance detection
2. **N-API**: Ensures compatibility across Node.js versions
3. **Worker Threads**: Runs detection in separate threads for non-blocking operation
4. **Multiple Input Formats**: Supports both file paths and image buffers

## License

MIT
