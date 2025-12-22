#include <napi.h>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <vector>
#include <string>

// Helper function to read image from buffer
cv::Mat readImageFromBuffer(const Napi::Buffer<uint8_t>& buffer) {
    std::vector<uint8_t> vec(buffer.Data(), buffer.Data() + buffer.Length());
    return cv::imdecode(vec, cv::IMREAD_COLOR);
}

// Main QR code detection function
Napi::Object DetectQRCode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    cv::Mat image;

    try {
        // Validate input
        if (info.Length() < 1) {
            Napi::TypeError::New(env, "Expected an image path or buffer").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        // Read input image
        if (info[0].IsString()) {
            std::string imagePath = info[0].As<Napi::String>().Utf8Value();
            image = cv::imread(imagePath, cv::IMREAD_COLOR);
        } else if (info[0].IsBuffer()) {
            Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
            image = readImageFromBuffer(buffer);
        } else {
            Napi::TypeError::New(env, "Expected string or buffer argument").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        if (image.empty()) {
            Napi::Error::New(env, "Failed to read image").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        // Initialize QR code detector
        cv::QRCodeDetector qrDecoder;
        
        // Try to detect and decode QR code
        std::vector<cv::Point> points;
        std::string decodedData = qrDecoder.detectAndDecode(image, points);
        
        // If decoding failed but detection succeeded, try with grayscale and preprocessing
        if (decodedData.empty() && !points.empty()) {
            cv::Mat gray;
            if (image.channels() == 3) {
                cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
            } else {
                gray = image.clone();
            }
            
            // Try with adaptive thresholding
            cv::Mat binary;
            cv::adaptiveThreshold(gray, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);
            decodedData = qrDecoder.detectAndDecode(binary, points);
            
            // If still empty, try with Otsu's thresholding
            if (decodedData.empty()) {
                cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                decodedData = qrDecoder.detectAndDecode(binary, points);
            }
        }

        // Create result object
        Napi::Object result = Napi::Object::New(env);
        
        if (!decodedData.empty()) {
            // QR code detected and decoded successfully
            result.Set("detected", Napi::Boolean::New(env, true));
            result.Set("data", Napi::String::New(env, decodedData));
            
            // Add corner points if available
            if (!points.empty()) {
                Napi::Array cornersArray = Napi::Array::New(env, points.size());
                for (size_t i = 0; i < points.size(); i++) {
                    Napi::Object point = Napi::Object::New(env);
                    point.Set("x", Napi::Number::New(env, points[i].x));
                    point.Set("y", Napi::Number::New(env, points[i].y));
                    cornersArray.Set(uint32_t(i), point);
                }
                result.Set("corners", cornersArray);
                
                // Extract the QR code region and encode as base64
                if (points.size() >= 4) {
                    // Get bounding rectangle
                    cv::Rect boundingRect = cv::boundingRect(points);
                    
                    // Add padding
                    int padding = 10;
                    boundingRect.x = std::max(0, boundingRect.x - padding);
                    boundingRect.y = std::max(0, boundingRect.y - padding);
                    boundingRect.width = std::min(image.cols - boundingRect.x, boundingRect.width + 2 * padding);
                    boundingRect.height = std::min(image.rows - boundingRect.y, boundingRect.height + 2 * padding);
                    
                    // Extract QR code region
                    cv::Mat qrRegion = image(boundingRect);
                    
                    // Encode as PNG
                    std::vector<uint8_t> buffer;
                    std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 9};
                    cv::imencode(".png", qrRegion, buffer, params);
                    
                    // Convert to base64
                    std::string base64;
                    static const char* base64_chars = 
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz"
                        "0123456789+/";
                    
                    int i = 0;
                    int j = 0;
                    unsigned char char_array_3[3];
                    unsigned char char_array_4[4];
                    
                    for (size_t idx = 0; idx < buffer.size(); idx++) {
                        char_array_3[i++] = buffer[idx];
                        if (i == 3) {
                            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                            char_array_4[3] = char_array_3[2] & 0x3f;
                            
                            for(i = 0; i < 4; i++)
                                base64 += base64_chars[char_array_4[i]];
                            i = 0;
                        }
                    }
                    
                    if (i) {
                        for(j = i; j < 3; j++)
                            char_array_3[j] = '\0';
                        
                        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                        
                        for (j = 0; j < i + 1; j++)
                            base64 += base64_chars[char_array_4[j]];
                        
                        while(i++ < 3)
                            base64 += '=';
                    }
                    
                    result.Set("qrCodeImage", Napi::String::New(env, "data:image/png;base64," + base64));
                }
            }
        } else {
            // No QR code detected
            result.Set("detected", Napi::Boolean::New(env, false));
            result.Set("data", env.Null());
        }

        return result;
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }
}

// Function to detect multiple QR codes in an image
// Note: This uses detectAndDecode in a loop approach for better stability
Napi::Object DetectMultipleQRCodes(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    cv::Mat image;

    try {
        // Validate input
        if (info.Length() < 1) {
            Napi::TypeError::New(env, "Expected an image path or buffer").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        // Read input image
        if (info[0].IsString()) {
            std::string imagePath = info[0].As<Napi::String>().Utf8Value();
            image = cv::imread(imagePath, cv::IMREAD_COLOR);
        } else if (info[0].IsBuffer()) {
            Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
            image = readImageFromBuffer(buffer);
        } else {
            Napi::TypeError::New(env, "Expected string or buffer argument").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        if (image.empty()) {
            Napi::Error::New(env, "Failed to read image").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        // Initialize QR code detector
        cv::QRCodeDetector qrDecoder;
        
        // For now, detect single QR code (stable approach)
        // Multi-QR detection can cause protobuf conflicts in some OpenCV builds
        std::vector<cv::Point> points;
        std::string decodedData = qrDecoder.detectAndDecode(image, points);

        // Create result object
        Napi::Object result = Napi::Object::New(env);
        
        if (!decodedData.empty()) {
            result.Set("detected", Napi::Boolean::New(env, true));
            result.Set("count", Napi::Number::New(env, 1));
            
            // Create array with single QR code
            Napi::Array qrCodesArray = Napi::Array::New(env, 1);
            Napi::Object qrCode = Napi::Object::New(env);
            qrCode.Set("data", Napi::String::New(env, decodedData));
            
            if (!points.empty()) {
                Napi::Array cornersArray = Napi::Array::New(env, points.size());
                for (size_t i = 0; i < points.size(); i++) {
                    Napi::Object point = Napi::Object::New(env);
                    point.Set("x", Napi::Number::New(env, points[i].x));
                    point.Set("y", Napi::Number::New(env, points[i].y));
                    cornersArray.Set(uint32_t(i), point);
                }
                qrCode.Set("corners", cornersArray);
                
                // Extract the QR code region and encode as base64
                if (points.size() >= 4) {
                    cv::Rect boundingRect = cv::boundingRect(points);
                    int padding = 10;
                    boundingRect.x = std::max(0, boundingRect.x - padding);
                    boundingRect.y = std::max(0, boundingRect.y - padding);
                    boundingRect.width = std::min(image.cols - boundingRect.x, boundingRect.width + 2 * padding);
                    boundingRect.height = std::min(image.rows - boundingRect.y, boundingRect.height + 2 * padding);
                    
                    cv::Mat qrRegion = image(boundingRect);
                    std::vector<uint8_t> buffer;
                    std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 9};
                    cv::imencode(".png", qrRegion, buffer, params);
                    
                    // Convert to base64
                    std::string base64;
                    static const char* base64_chars = 
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz"
                        "0123456789+/";
                    
                    int i = 0;
                    int j = 0;
                    unsigned char char_array_3[3];
                    unsigned char char_array_4[4];
                    
                    for (size_t idx = 0; idx < buffer.size(); idx++) {
                        char_array_3[i++] = buffer[idx];
                        if (i == 3) {
                            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                            char_array_4[3] = char_array_3[2] & 0x3f;
                            
                            for(i = 0; i < 4; i++)
                                base64 += base64_chars[char_array_4[i]];
                            i = 0;
                        }
                    }
                    
                    if (i) {
                        for(j = i; j < 3; j++)
                            char_array_3[j] = '\0';
                        
                        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                        
                        for (j = 0; j < i + 1; j++)
                            base64 += base64_chars[char_array_4[j]];
                        
                        while(i++ < 3)
                            base64 += '=';
                    }
                    
                    qrCode.Set("qrCodeImage", Napi::String::New(env, "data:image/png;base64," + base64));
                }
            }
            
            qrCodesArray.Set(uint32_t(0), qrCode);
            result.Set("qrCodes", qrCodesArray);
        } else {
            result.Set("detected", Napi::Boolean::New(env, false));
            result.Set("count", Napi::Number::New(env, 0));
            result.Set("qrCodes", Napi::Array::New(env, 0));
        }

        return result;
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }
}

// Function to check if image contains a QR code (quick detection without decoding)
Napi::Object HasQRCode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    cv::Mat image;

    try {
        // Validate input
        if (info.Length() < 1) {
            Napi::TypeError::New(env, "Expected an image path or buffer").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        // Read input image
        if (info[0].IsString()) {
            std::string imagePath = info[0].As<Napi::String>().Utf8Value();
            image = cv::imread(imagePath, cv::IMREAD_COLOR);
        } else if (info[0].IsBuffer()) {
            Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
            image = readImageFromBuffer(buffer);
        } else {
            Napi::TypeError::New(env, "Expected string or buffer argument").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        if (image.empty()) {
            Napi::Error::New(env, "Failed to read image").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        // Initialize QR code detector
        cv::QRCodeDetector qrDecoder;
        
        // Only detect, don't decode
        std::vector<cv::Point> points;
        bool detected = qrDecoder.detect(image, points);

        // Create result object
        Napi::Object result = Napi::Object::New(env);
        result.Set("hasQRCode", Napi::Boolean::New(env, detected));
        
        if (detected && !points.empty()) {
            Napi::Array cornersArray = Napi::Array::New(env, points.size());
            for (size_t i = 0; i < points.size(); i++) {
                Napi::Object point = Napi::Object::New(env);
                point.Set("x", Napi::Number::New(env, points[i].x));
                point.Set("y", Napi::Number::New(env, points[i].y));
                cornersArray.Set(uint32_t(i), point);
            }
            result.Set("corners", cornersArray);
        }

        return result;
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }
}

// Initialize the addon
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(
        Napi::String::New(env, "detectQRCode"),
        Napi::Function::New(env, DetectQRCode)
    );
    exports.Set(
        Napi::String::New(env, "detectMultipleQRCodes"),
        Napi::Function::New(env, DetectMultipleQRCodes)
    );
    exports.Set(
        Napi::String::New(env, "hasQRCode"),
        Napi::Function::New(env, HasQRCode)
    );
    return exports;
}

NODE_API_MODULE(qr_code_detector, Init)

