const path = require('path');
const { detectQRCode: nativeDetectQRCode, detectMultipleQRCodes: nativeDetectMultipleQRCodes, hasQRCode: nativeHasQRCode } = require('./build/Release/qr_code_detector');

const MODEL_DIR = process.env.WECHAT_MODEL_DIR || path.join(__dirname, 'models');

/**
 * Detects and decodes a single QR code in an image.
 * Note: This is a synchronous operation that wraps the native function in a promise.
 * @param {string|Buffer} input - Image file path or buffer containing image data
 * @returns {Promise<Object>} Promise resolving to an object containing:
 *   - detected {boolean} - Whether a QR code was detected
 *   - data {string|null} - Decoded QR code data (null if not detected)
 *   - corners {Array<{x: number, y: number}>} - Corner points of the QR code (if detected)
 */
async function detectQRCode(input) {
  return Promise.resolve(nativeDetectQRCode(input, MODEL_DIR));
}

/**
 * Detects and decodes multiple QR codes in an image.
 * Note: This is a synchronous operation that wraps the native function in a promise.
 * @param {string|Buffer} input - Image file path or buffer containing image data
 * @returns {Promise<Object>} Promise resolving to an object containing:
 *   - detected {boolean} - Whether any QR codes were detected
 *   - count {number} - Number of QR codes detected
 *   - qrCodes {Array<Object>} - Array of detected QR codes, each containing:
 *     - data {string} - Decoded QR code data
 *     - corners {Array<{x: number, y: number}>} - Corner points of the QR code
 */
async function detectMultipleQRCodes(input) {
  return Promise.resolve(nativeDetectMultipleQRCodes(input, MODEL_DIR));
}

/**
 * Checks if an image contains a QR code without decoding it.
 * This is faster than detectQRCode() when you only need to know if a QR code is present.
 * Note: This is a synchronous operation that wraps the native function in a promise.
 * @param {string|Buffer} input - Image file path or buffer containing image data
 * @returns {Promise<Object>} Promise resolving to an object containing:
 *   - hasQRCode {boolean} - Whether a QR code was detected
 *   - corners {Array<{x: number, y: number}>} - Corner points of the QR code (if detected)
 */
async function hasQRCode(input) {
  return Promise.resolve(nativeHasQRCode(input));
}

// Also export synchronous versions
const detectQRCodeSync = (input) => nativeDetectQRCode(input, MODEL_DIR);
const detectMultipleQRCodesSync = (input) => nativeDetectMultipleQRCodes(input, MODEL_DIR);
const hasQRCodeSync = nativeHasQRCode;

module.exports = {
  detectQRCode,
  detectMultipleQRCodes,
  hasQRCode,
  detectQRCodeSync,
  detectMultipleQRCodesSync,
  hasQRCodeSync,
};
