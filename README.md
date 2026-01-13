# OCR_377DE_ARM32

该项目为 Sigmastar 377DE 平台的 OCR 识别 Demo，配有前端 Web 操作页面以及后端算法响应，搭建基本 OCR 文本检测识别场景。

## 项目简介

本项目基于 Sigmastar 377DE ARM32 平台，实现了完整的 OCR（光学字符识别）解决方案，包括：
- 基于 PaddleOCR v4 的文本检测和识别算法
- 嵌入式 Web 服务器（基于 httplib）
- 响应式前端界面，支持实时查看识别结果

## 项目结构

```
.
├── lintech/
│   └── celectronicfence/          # Web 服务器后端
│       ├── main.cc                # 主程序（Web 服务器 + OCR 结果管理）
│       ├── main                   # 编译后的可执行文件
│       ├── httplib.hpp            # HTTP 服务器库
│       ├── json.hpp               # JSON 解析库
│       ├── ocr_results.json       # OCR 识别结果存储文件
│       └── static/                # 前端静态资源
│           ├── index.html         # 主页面
│           ├── stream_new.html    # 视频流页面
│           ├── alarm.html         # 报警页面
│           └── ...                # 其他前端资源
│
└── OCR_demo/
    └── OCR_demo/                  # OCR 算法程序
        ├── prog_det_ocr           # OCR 检测识别可执行文件
        ├── param_snr0.ini         # 算法参数配置文件
        └── model/                 # OCR 模型文件
            ├── ch_PP-OCRv4_det_infer_640_2.img      # 中文检测模型
            └── en_PP-OCRv4_rec_infer_48_208.img     # 英文识别模型
```

## 功能特性

- **OCR 文本识别**：支持中英文混合文本检测和识别
- **Web 界面**：提供友好的 Web 操作界面，实时展示识别结果
- **多种集成方式**：支持文件、HTTP API、函数调用等多种方式获取识别结果
- **历史记录**：自动保存最近 100 条识别记录
- **实时更新**：前端可实时获取最新的识别结果

## 快速开始

### 1. 启动 Web 服务器

```bash
cd lintech/celectronicfence
./main
```

Web 服务器默认运行在 `http://localhost:8088`

### 2. 运行 OCR 算法程序

```bash
cd OCR_demo/OCR_demo
./prog_det_ocr param_snr0.ini
```

### 3. 访问 Web 界面

在浏览器中打开：`http://<设备IP>:8088`

## OCR 结果集成方式

算法程序可以通过以下三种方式将识别结果发送到 Web 界面：

### 方式 1：写入文件（推荐）

```bash
echo "cdEFG123" > /tmp/ocr_result.txt
```

前端点击"识别"按钮时，会自动读取该文件。

### 方式 2：POST 到 Web 服务器

```bash
wget --post-data='{"text":"cdEFG123"}' \
     --header="Content-Type: application/json" \
     http://localhost:8088/api/ocr/add
```

或使用 curl：

```bash
curl -X POST http://localhost:8088/api/ocr/add \
     -H "Content-Type: application/json" \
     -d '{"text":"cdEFG123"}'
```

### 方式 3：在同一程序中调用

C++ 代码：
```cpp
sendOcrResultDirectly("cdEFG123");
```

C 代码：
```c
send_ocr_result("cdEFG123");
```

## API 接口

### 获取 OCR 结果列表

```
GET /api/ocr/list
```

返回示例：
```json
{
  "status": "success",
  "texts": [
    {
      "text": "c23",
      "time": "14:30:25",
      "timestamp": 1705132225
    }
  ]
}
```

### 添加 OCR 结果

```
POST /api/ocr/add
Content-Type: application/json

{
  "text": "识别的文本内容"
}
```

### 清空历史记录

```
POST /api/ocr/clear
```

## 配置说明

### OCR 算法参数

编辑 [param_snr0.ini](OCR_demo/OCR_demo/param_snr0.ini) 文件可调整算法参数。

### Web 服务器端口

默认端口为 8088，可在 [main.cc](lintech/celectronicfence/main.cc) 中修改。

## 技术栈

- **平台**：Sigmastar 377DE (ARM32)
- **OCR 引擎**：PaddleOCR v4
- **Web 服务器**：cpp-httplib
- **前端框架**：Bootstrap 5
- **数据格式**：JSON (nlohmann/json)

## 注意事项

1. 确保设备有足够的内存运行 OCR 模型
2. 模型文件较大（约 5.7MB），需要一定时间
3. Web 服务器最多保存 100 条历史记录
4. 建议在局域网环境下使用

## 许可证

本项目仅供学习和演示使用。
