# irParseTool-server

红外热成像图片分析工具服务端基于websocket协议，使用json传输数据。

## 1 代码说明

### 1.1 编译

`./bld.sh`

### 1.2 参数说明

ir_process.h 中的两个变量定义了服务端支持的红外图片的高和宽。
```
#define SUPPORT_MAX_IMAGE_WIDTH      1024
#define SUPPORT_MAX_IMAGE_HEIGHT     768
```
threadpool.h 中的MAX_THREADS定义了服务端支持的最大并发连接数，IDLE_NUM定义了空闲线程数量。
```
#define MAX_THREADS  10
#define IDLE_NUM     1
```

## 2 获取红外图片命令

### 2.1 前端发送

`{"code":1,"type":1, "path":"/filepath/filename.temp","width":640,"height":480,"maxScale":100,"minScale":0}`

| 变量 | 描述 |
| :--: | :--: |
| code | 功能码，1 表示获取红外图片 |
| type | 图片类型， 1 表示铁锈红， 2 表示灰度 |
| path | 图片温度数据路径 |
| width | 图片宽度, 整型 |
| height | 图片高度, 整型 |
| maxScale | 温度最大值对应调色板的位置占调色板高度的百分比×100，整型，默认100，详情见图1 |
| minScale | 温度最小值对应调色板的位置占调色板高度的百分比×100，整型，默认0，详情见图1 |

![图1](https://github.com/yaoyanlianglove/irParseTool-server/blob/master/readme/tu1.png)

#### 图1 红外图片调色板

### 2.2 服务端回复

`{"code":1,"maxTempOfAll":34.9,"minTempOfAll":26.1,"image":"RGBA数据"}`

| 变量 | 描述 |
| :--: | :--: |
| code | 功能码，1 表示获取红外图片 |
| maxTempOfAll | 整个图片区域中的温度最大值，浮点，1位小数 |
| minTempOfAll | 整个图片区域中的温度最小值，浮点，1位小数 |
| image | 温度转换为图片的RGBA数据，BASE64编码 |

## 3 获取指定区域温度命令

### 3.1 前端发送

`{"code":1,"x1":1,"y1":2,"x2":100,"y2":235}`

| 变量 | 描述 |
| :--: | :--: |
| code | 功能码，2 表示获取指定区域温度 |
| x1 | 区域左上角x坐标，整型 |
| y1 | 区域左上角y坐标，整型 |
| x2 | 区域右下角x坐标，整型 |
| y2 | 区域右下角y坐标，整型 |

### 3.2 服务端回复

`{"code":1,"maxTempOfArea":10.3,"minTempOfArea":24.1,"avgTempOfArea":100.0}`

| 变量 | 描述 |
| :--: | :--: |
| code | 功能码，2 表示获取指定区域温度 |
| maxTempOfArea | 区域内温度最大值，浮点，1位小数 |
| minTempOfArea | 区域内温度最小值，浮点，1位小数 |
| avgTempOfArea | 区域内温度平均值，浮点，1位小数 |

## 4 错误回复

`{"error":4xx}`

| 错误代码 | 描述 |
| :--: | :--: |
| 401 | json变量个数错误 |
| 402 | 不能识别的功能码 |
| 403 | 不能识别的变量 |
| 404 | 红外温度数据文件不存在 |
| 405 | 参数越限 |
