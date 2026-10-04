* **Mapgen ARG (`arg`)**：基于多频段 3D 分形噪声算法生成交错的蛛网状地形。
* **Mapgen Skygrid (`skygrid`)**：结构化空岛方块网格生成器，具备对 Node 注册库的自动提取与过滤机制（自动过滤冲突节点）
* **Mapgen Glitch (`glitch`)**：基于区块坐标的确定性哈希种子偏移算法，生成不连续、具错乱感的世界。
* **Mapgen Farlands (`farlands`)**：bro尝试复刻边境之地但四不像
* **Mapgen Backrooms (`backrooms`)**：后室结构生成器，基于二维墙体与三维货架噪声矩阵组合构建回廊地形（这个本来是要作为边境之地，但看了看像后室）
* **Mapgen Layered (`layered`)**：千层饼式多重世界堆叠生成器。在不同的垂直高度区间（Overworld、Band 1~4）根据 Chunk Group 随机洗牌组合上述多种奇观生成算法，依旧大杂烩

集成 Apache `ftpserver-core` 与 `mina-core`，新增 `FtpService` 后台服务与 `FtpActivity` 管理界面（这个实际上我之前在Rainbow-Porygon，Vector-Breakthrough就实现过了）
引入 `com.google.crypto.tink:tink-android` 加密库，结合 Android KeyStore (AEAD) 对凭据数据进行硬安全加密存储。
配置 `FOREGROUND_SERVICE_DATA_SYNC` 权限与常驻通知栏快捷开关，呃就是个“前台服务”防止系统给应用暂停了

* **`l_mapgen.cpp` 容错处理**：修复 `minetest.get_mapgen_object` 读取 `heightmap`、`biomemap`、`heatmap` 及 `humidmap` 时，因特定生成器未初始化相关数组导致的悬空指针崩溃问题。增加了空指针检查并提供默认回退数组。

* **子模块集成**：集成 `games/mineclonia`（基于 Git Submodule）。
* **高清材质包**：内置 `Unofficial-Faithful32x-Luanti` 32x 高清材质包（就是我之前做的那个）
* **默认配置预设**：新增 `minetest.conf` 预设，默认启用中文环境 (`language = zh_CN`)（反正又没有外国人来，我又不做i18n）、死亡不掉落 (`keepInventory = true`) 并预设材质包路径。

* **许可证更新**：项目使用 GNU AGPL v3 (GNU Affero General Public License) 开源协议（我都用了我之前AGPLv3的代码顺手升一下许可证）
