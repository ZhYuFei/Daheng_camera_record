#include <iostream>
#include <opencv2/opencv.hpp>
#include "include/DxImageProc.h"
#include "include/GxIAPI.h"

int main() {
    GX_STATUS status = GX_STATUS_SUCCESS;
    GX_DEV_HANDLE hDevice = nullptr;
    GX_OPEN_PARAM openParam;

    status = GXInitLib();
    if (status != GX_STATUS_SUCCESS) {
        std::cerr << "Failed to initialize GXAPI library." << std::endl;
        return -1;
    }

    openParam.accessMode = GX_ACCESS_EXCLUSIVE;
    openParam.openMode = GX_OPEN_INDEX;
    openParam.pszContent = "1";

    status = GXOpenDevice(&openParam, &hDevice);
    if (status != GX_STATUS_SUCCESS) {
        std::cerr << "Failed to open device." << std::endl;
        GXCloseLib();
        return -1;
    }

    // 获取传感器最大尺寸
    int64_t max_width = 0, max_height = 0;
    GXGetInt(hDevice, GX_INT_WIDTH_MAX, &max_width);
    GXGetInt(hDevice, GX_INT_HEIGHT_MAX, &max_height);
    std::cout << "Max size: " << max_width << "x" << max_height << std::endl;

    // 设置输出分辨率（不超过传感器尺寸）
    int64_t output_width = 1632, output_height = 1440;
    std::cout << GXSetInt(hDevice, GX_INT_WIDTH, output_width) << std::endl;
    std::cout << GXSetInt(hDevice, GX_INT_HEIGHT, output_height) << std::endl;

    // 设置最大分辨率
    int64_t width = 0, height = 0;
    GXGetInt(hDevice, GX_INT_WIDTH, &width);
    GXGetInt(hDevice, GX_INT_HEIGHT, &height);
    std::cout << "size: " << width << "x" << height << std::endl;

    // 动态获取相机当前帧率
    double cameraFps = 0.0;
    GXGetFloat(hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, &cameraFps);
    std::cout << "Camera FPS: " << cameraFps << std::endl;

    // 设置曝光时间
    GXSetEnum(hDevice, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);
    GXSetFloat(hDevice, GX_FLOAT_EXPOSURE_TIME, 3000.0);

    // 执行一次自动白平衡
    GXSetEnum(hDevice, GX_ENUM_BALANCE_WHITE_AUTO, GX_BALANCE_WHITE_AUTO_ONCE);

    // 采集模式设置
    GXSetEnum(hDevice, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
    GXSetEnum(hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF);

    status = GXStreamOn(hDevice);
    if (status != GX_STATUS_SUCCESS) {
        std::cerr << "Failed to start acquisition." << std::endl;
        GXCloseDevice(hDevice);
        GXCloseLib();
        return -1;
    }

    // 创建视频写入器（帧率与相机同步）
    cv::VideoWriter videoWriter;
    std::string outputFile = "camera_output.mp4";
    int codec = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    videoWriter.open(outputFile, codec, cameraFps, cv::Size(width, height));
    if (!videoWriter.isOpened()) {
        std::cerr << "Failed to create video file!" << std::endl;
        GXStreamOff(hDevice);
        GXCloseDevice(hDevice);
        GXCloseLib();
        return -1;
    }

    // 创建显示窗口
    cv::namedWindow("Daheng Camera", cv::WINDOW_AUTOSIZE);
    std::cout << "Recording video (Press ESC to stop)..." << std::endl;

    PGX_FRAME_BUFFER pFrameBuffer = nullptr;
    bool isRunning = true;
    auto lastFrameTime = std::chrono::steady_clock::now();

    while (isRunning) {
        // 获取一帧图像（超时设为1秒）
        status = GXDQBuf(hDevice, &pFrameBuffer, 1000);
        if (status != GX_STATUS_SUCCESS) {
            std::cerr << "Failed to get image buffer." << std::endl;
            break;
        }

        // 直接使用 pFrameBuffer->nWidth 作为步长（假设无填充字节）
        cv::Mat rawImage(pFrameBuffer->nHeight, pFrameBuffer->nWidth, CV_8UC1, pFrameBuffer->pImgBuf);
        cv::Mat processedImage;

        switch (pFrameBuffer->nPixelFormat) {
            case GX_PIXEL_FORMAT_MONO8:
                processedImage = rawImage;
                break;
            case GX_PIXEL_FORMAT_BAYER_GR8:
                cv::cvtColor(rawImage, processedImage, cv::COLOR_BayerGR2BGR);
                break;
            case GX_PIXEL_FORMAT_BAYER_RG8:
                cv::cvtColor(rawImage, processedImage, cv::COLOR_BayerRG2BGR);
                break;
            case GX_PIXEL_FORMAT_BAYER_GB8:
                cv::cvtColor(rawImage, processedImage, cv::COLOR_BayerGB2RGB);
                break;
            case GX_PIXEL_FORMAT_BAYER_BG8:
                cv::cvtColor(rawImage, processedImage, cv::COLOR_BayerBG2BGR);
                break;
            case GX_PIXEL_FORMAT_RGB8:
                cv::cvtColor(rawImage, processedImage, cv::COLOR_RGB2BGR);
                break;
            default:
                GXQBuf(hDevice, pFrameBuffer);
                continue;
        }

        cv::imshow("Daheng Camera", processedImage);

        // 写入视频文件（严格同步帧率）
        videoWriter << processedImage;

        // 计算实际帧间隔（用于调试）
        auto currentTime = std::chrono::steady_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        std::cout << "Frame interval: " << elapsedMs << " ms" << std::endl;

        GXQBuf(hDevice, pFrameBuffer);
        
        // ESC键退出
        if (cv::waitKey(1) == 27) {
            isRunning = false;
        }
    }

    // 释放资源
    videoWriter.release();
    GXStreamOff(hDevice);
    GXCloseDevice(hDevice);
    GXCloseLib();
    cv::destroyAllWindows();

    std::cout << "Video saved to: " << outputFile << " (FPS: " << cameraFps << ")" << std::endl;
    return 0;
}