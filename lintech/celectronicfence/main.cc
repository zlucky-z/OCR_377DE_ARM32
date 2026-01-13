/*
 * 嵌入式Web服务器 + OCR识别结果展示
 * 
 * 算法程序持续运行，前端点击"识别"按钮时从算法获取最新结果
 * 
 * 算法程序输出识别结果的方式：
 * 
 * 方式1: 写入文件（推荐，简单可靠）
 *   echo "cdEFG123" > /tmp/ocr_result.txt
 *   前端点击识别按钮时，会自动读取这个文件
 * 
 * 方式2: POST到Web服务器（实时推送）
 *   wget --post-data='{"text":"cdEFG123"}' \
 *        --header="Content-Type: application/json" \
 *        http://localhost:8088/api/ocr/add
 * 
 * 方式3: 在同一个程序中调用
 *   sendOcrResultDirectly("cdEFG123");
 */

#include "httplib.hpp"
#include "json.hpp"
#include <fstream>
#include <mutex>
#include <deque>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>

using namespace httplib;
using json = nlohmann::json;

// 全局变量存储OCR识别结果
std::deque<json> ocrResults;
std::mutex ocrMutex;
const size_t MAX_RESULTS = 100; // 最多保存100条记录

// 前向声明
void saveOcrResults();
void addOcrResult(const std::string& text);

// 获取当前时间字符串
std::string getCurrentTime() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

// 添加OCR识别结果（内部函数）
void addOcrResult(const std::string& text) {
    std::lock_guard<std::mutex> lock(ocrMutex);
    
    json item;
    item["text"] = text;
    item["time"] = getCurrentTime();
    item["timestamp"] = std::time(nullptr);
    
    ocrResults.push_back(item);
    
    // 保持队列大小不超过MAX_RESULTS
    if (ocrResults.size() > MAX_RESULTS) {
        ocrResults.pop_front();
    }
}

// ============================================================
// 在OCR识别代码中调用此函数即可发送结果到Web界面
// ============================================================
void sendOcrResultDirectly(const std::string& text) {
    if (text.empty()) return;
    
    std::cout << "OCR识别: " << text << std::endl;
    addOcrResult(text);
    saveOcrResults();
}

// C语言接口
extern "C" {
    void send_ocr_result(const char* text) {
        if (text != NULL) {
            sendOcrResultDirectly(std::string(text));
        }
    }
}

// 从文件加载OCR结果（启动时恢复数据）
void loadOcrResults() {
    std::ifstream file("ocr_results.json");
    if (file.is_open()) {
        try {
            json data = json::parse(file);
            if (data["status"] == "success" && data.contains("texts")) {
                std::lock_guard<std::mutex> lock(ocrMutex);
                for (const auto& item : data["texts"]) {
                    ocrResults.push_back(item);
                }
                std::cout << "Loaded " << ocrResults.size() << " OCR results from file" << std::endl;
            }
        } catch (...) {
            std::cout << "Failed to load OCR results from file" << std::endl;
        }
        file.close();
    }
}

// 保存OCR结果到文件
void saveOcrResults() {
    json output;
    output["status"] = "success";
    output["texts"] = json::array();
    
    for (const auto& item : ocrResults) {
        output["texts"].push_back(item);
    }
    
    std::ofstream file("ocr_results.json");
    if (file.is_open()) {
        file << output.dump(2);
        file.close();
    }
}

int main(int argc, char *argv[])
{
    Server svr;

    // 启动时加载历史OCR结果
    loadOcrResults();

    // 设置静态文件挂载点（将根路径映射到本地./static目录）
    auto ret = svr.set_mount_point("/", "./static");
    if (!ret)
    {
        printf("The ./static directory doesn't exist...\n");
        return 1;
    }

    // API: 获取OCR识别结果
    svr.Get("/api/recognized-text", [](const Request& req, Response& res) {
        std::lock_guard<std::mutex> lock(ocrMutex);
        
        json response;
        response["status"] = "success";
        response["texts"] = json::array();
        
        // 返回所有识别结果
        for (const auto& item : ocrResults) {
            response["texts"].push_back(item);
        }
        
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(response.dump(), "application/json");
    });

    // API: 获取OCR识别结果（前端点击识别按钮时调用）
    // GET /api/ocr/add - 从算法程序读取最新的识别结果
    svr.Get("/api/ocr/add", [](const Request& req, Response& res) {
        std::cout << "========================================" << std::endl;
        std::cout << "前端请求获取OCR识别结果" << std::endl;
        std::cout << "时间: " << getCurrentTime() << std::endl;
        
        // ========== 从算法输出源读取识别结果 ==========
        // 方式1: 从共享文件读取（推荐）
        std::string recognizedText = "";
        std::string resultFile = "/tmp/ocr_result.txt";  // 算法输出的结果文件路径
        
        std::ifstream file(resultFile);
        if (file.is_open()) {
            std::getline(file, recognizedText);
            file.close();
            std::cout << "从文件读取到识别结果: " << recognizedText << std::endl;
        } else {
            std::cout << "警告: 无法打开结果文件 " << resultFile << std::endl;
            // 如果读取不到文件，返回空结果或默认值
            recognizedText = "";
        }
        
        // 方式2: 如果算法程序已经调用了addOcrResult()保存了结果，可以从队列中获取最新的
        if (recognizedText.empty()) {
            std::lock_guard<std::mutex> lock(ocrMutex);
            if (!ocrResults.empty()) {
                recognizedText = ocrResults.back()["text"];
                std::cout << "从历史记录获取最新结果: " << recognizedText << std::endl;
            }
        }
        // =================================================
        
        std::cout << "返回识别结果: " << recognizedText << std::endl;
        std::cout << "========================================" << std::endl;
        
        // 构造响应（与fastapi的格式一致）
        json response;
        response["status"] = "success";
        response["message"] = "OCR数据已接收";
        response["text"] = recognizedText;
        response["number"] = "1";  // 识别结果数量
        response["timestamp"] = getCurrentTime();
        
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(response.dump(), "application/json");
    });

    // API: OCR识别结果（供OCR算法程序POST调用）
    svr.Post("/api/ocr/add", [](const Request& req, Response& res) {
        try {
            auto data = json::parse(req.body);
            std::string text = data["text"];
            
            std::cout << "OCR识别: " << text << std::endl;
            addOcrResult(text);
            saveOcrResults();
            
            json response;
            response["status"] = "success";
            response["message"] = "OCR result added";
            
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            res.status = 400;
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(response.dump(), "application/json");
        }
    });

    // API: 清空OCR识别结果
    svr.Post("/api/ocr/clear", [](const Request& req, Response& res) {
        std::lock_guard<std::mutex> lock(ocrMutex);
        ocrResults.clear();
        saveOcrResults();
        
        json response;
        response["status"] = "success";
        response["message"] = "OCR results cleared";
        
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(response.dump(), "application/json");
    });

    // API: 触发OCR识别（前端点击"识别"按钮时调用）
    // 需要在这个接口中实现识别逻辑，并在响应中返回识别结果
    svr.Post("/api/ocr/trigger", [](const Request& req, Response& res) {
        std::cout << "========================================" << std::endl;
        std::cout << "收到OCR识别触发请求" << std::endl;
        std::cout << "时间: " << getCurrentTime() << std::endl;
        std::cout << "========================================" << std::endl;
        
        // TODO: 调用OCR识别算法，获取识别结果
        // 例: std::string recognizedText = your_ocr_function();
        
        std::string recognizedText = "";  // 算法那边替换这里，填入实际识别结果
        
        // =================================================
        
        // 返回识别结果给前端
        json response;
        response["status"] = "success";
        response["message"] = "Recognition completed";
        response["text"] = recognizedText;  // 识别到的文本内容
        response["timestamp"] = std::time(nullptr);
        
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(response.dump(), "application/json");
    });

    // 启动HTTP服务器，监听所有网卡的8088端口
    std::cout << "========================================" << std::endl;
    std::cout << "Static file server running on 0.0.0.0:8088" << std::endl;
    std::cout << "OCR API endpoints:" << std::endl;
    std::cout << "  - GET  /api/ocr/add (前端获取识别结果)" << std::endl;
    std::cout << "  - POST /api/ocr/add (算法POST识别结果)" << std::endl;
    std::cout << "  - GET  /api/recognized-text (获取历史记录)" << std::endl;
    std::cout << "  - POST /api/ocr/clear (清空识别结果)" << std::endl;
    std::cout << "  - POST /api/ocr/trigger (触发识别请求)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "算法程序请将识别结果写入: /tmp/ocr_result.txt" << std::endl;
    std::cout << "========================================" << std::endl;
    svr.listen("0.0.0.0", 8088);

    return 0;
}