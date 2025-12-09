# HTTP_project
基于 C++ 实现自定义的 HTTP Server 框架，该项目包括 HTTP / HTTPS 支持、动态路由处理、会话管理等功能。同时，基于该框架开发了一个支持人机对战的五子棋游戏。

# 脚本式环境配置
- 操作系统：Ubuntu 22.04.5

新建一个文件 build-HTTP.sh，将以下内容复制进入文件：
```bash
#!/bin/bash

# 停止并禁用自动更新任务
echo "Stopping and disabling apt-daily timers..."
sudo systemctl stop apt-daily.timer
sudo systemctl stop apt-daily-upgrade.timer
sudo systemctl disable apt-daily.timer
sudo systemctl disable apt-daily-upgrade.timer

# 更新系统包索引
echo "Updating system package list..."
sudo apt update

# 安装编译工具和依赖
echo "Installing g++, cmake, and make..."
sudo apt install -y g++ cmake make

# 安装 aria2 下载工具
echo "Installing aria2..."
sudo apt install -y aria2

# 下载 MySQL 5.7.29 安装包
echo "Downloading MySQL server 5.7.29..."
aria2c https://downloads.mysql.com/archives/get/p/23/file/mysql-server_5.7.29-1ubuntu18.04_amd64.deb-bundle.tar

# 解压下载的 MySQL 安装包
echo "Extracting MySQL server package..."
sudo tar -xvf mysql-server_5.7.29-1ubuntu18.04_amd64.deb-bundle.tar

# 安装 MySQL 相关库和依赖
echo "Installing libmysql libraries..."
sudo apt install -y ./libmysql*

# 安装 libtinfo5
echo "Installing libtinfo5..."
sudo apt install -y libtinfo5

# 安装 MySQL 客户端和服务器包
echo "Installing MySQL community client..."
sudo apt-get install -y ./mysql-community-client_5.7.29-1ubuntu18.04_amd64.deb

echo "Installing MySQL client..."
sudo apt-get install -y ./mysql-client_5.7.29-1ubuntu18.04_amd64.deb

echo "Installing MySQL community server..."
sudo apt-get install -y ./mysql-community-server_5.7.29-1ubuntu18.04_amd64.deb

echo "Installing MySQL server..."
sudo apt-get install -y ./mysql-server_5.7.29-1ubuntu18.04_amd64.deb

# 连接 MySQL 并创建数据库与表
echo "Configuring MySQL user and database..."
sudo mysql -uroot -proot -e "
CREATE DATABASE Gomoku;
USE Gomoku;
CREATE TABLE users (
    id INT(11) NOT NULL AUTO_INCREMENT,
    username VARCHAR(255) NOT NULL,
    password VARCHAR(255) NOT NULL,
    PRIMARY KEY (id)
);
UPDATE mysql.user SET host = '%' WHERE user = 'root' AND host = 'localhost';
GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY 'root' WITH GRANT OPTION;
FLUSH PRIVILEGES;
"

# 修改 MySQL 配置，允许远程连接
echo "Allowing MySQL to listen on all interfaces..."
sudo sed -i "s/^bind-address\s*=.*/bind-address = 0.0.0.0/" /etc/mysql/mysql.conf.d/mysqld.cnf
sudo systemctl restart mysql

# 配置防火墙，允许 3306 端口
echo "Allowing port 3306 for MySQL..."
sudo ufw allow 3306

# 下载并安装 Boost 库
echo "Downloading and installing Boost 1.69.0..."
wget https://www.programmercarl.com/download/boost_1_69_0.tar.gz
tar -zxvf boost_1_69_0.tar.gz
cd boost_1_69_0/
./bootstrap.sh
./b2
sudo ./b2 install
cd ..

# 下载并解压 muduo 库
echo "Downloading and extracting muduo-master..."
wget https://www.programmercarl.com/download/muduo-master.zip
sudo apt install -y unzip
unzip muduo-master.zip
cd muduo-master
sudo chmod +x build.sh

# 编译并安装 muduo
echo "Building and installing muduo..."
./build.sh
./build.sh install

# 移动头文件和库文件
echo "Moving muduo headers and libraries..."
cd ..
cd build/
cd release-install-cpp11/
cd include/
sudo mv muduo/ /usr/include/
cd ..
cd lib/
sudo mv * /usr/local/lib/

# 更新系统
echo "Upgrading system and installing dependencies..."
sudo apt upgrade -y
sudo apt install -y nlohmann-json3-dev
sudo apt install -y libmysqlcppconn-dev
sudo apt install -y libssl-dev

echo "Script execution completed successfully!"
```
添加可执行权限：
```bash
chmod +x build-HTTP.sh
```
运行脚本文件，注意中途设置 mysql root 用户密码为 root：
```bash
sudo ./build-HTTP.sh
```


# 手动环境搭建

- 操作系统：Ubuntu 22.04.5
- 数据库：mysql 5.7.29
- boost_1_69_0
- muduo 网络库

首先禁用自动更新服务，防止其与手动操作冲突：
```bash
sudo systemctl stop apt-daily-timer
sudo systemctl stop apt-daily-upgrade.timer
sudo systemctl disable apt-daily.timer
sudo systemctl disable apt-daily-upgrade.timer
```

## 编译器相关
```bash
sudo apt update
sudo apt install g++ came make
```

## Mysql 相关
mysql 5.7 压缩包下载：
使用 `aria2` 加速下载压缩包：`aria2` 是一个支持多协议、多源和多线程的下载工具。
```bash
sudo apt install aria2
aria2c https://downloads.mysql.com/archives/get/p/23/file/mysql-server_5.7.29-1ubuntu18.04_amd64.deb-bundle.tar
```

下载好之后解压缩
```bash
sudo tar -xvf mysql-server_5.7.29-1ubuntu18.04_amd64.deb-bundle.tar
```

## 安装 mysql
安装依赖 lib 包
```bash
sudo apt install ./libmysql*
sudo apt install libtinfo5
```
安装客户端和服务端，按提示可能要先安装 community 版本
```bash
sudo apt-get install ./mysql-community-client_5.7.29-1ubuntu18.04_amd64.deb
sudo apt-get install ./mysql-client_5.7.29-1ubuntu18.04_amd64.deb
sudo apt-get install ./mysql-community-server_5.7.29-1ubuntu18.04_amd64.deb
sudo apt-get install ./mysql-server_5.7.29-1ubuntu18.04_amd64.deb
```

第三行命令执行时会提示设置 MySql 的密码，用户名默认 root。这里我们手动输入`密码`为 `root`

## 启动 mysql
一般安装成功就自动启动，输入命令检查启动状态，绿色的 active 表示运行中
```bash
systemctl status mysql.service
```

## 登录 MySQL
```bash
sudo mysql -u root -p root
```

安装好 mysql 后，进入 mysql 后创建数据库数据表命令，以及设置 root 用户访问权限（比如放开远程连接）。
```sql
create database Gomoku;
use Gomoku;
CREATE TABLE users (
    id INT(11) NOT NULL AUTO_INCREMENT, 
    username VARCHAR(255) NOT NULL,
    password VARCHAR(255) NOT NULL,
    PRIMARY KEY (id)
);
UPDATE mysql.user SET host = '%' WHERE user = 'root' AND host = 'localhost';
GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY 'root' WITH GRANT OPTION;
FLUSH PRIVILEGES;
```

在系统文件中也要开启远程访问权限，设置完毕后需要重启 mysql 服务。
```bash
sudo sed -i "s/^bind-address\s*=.*/bind-address = 0.0.0.0/" /etc/mysql/mysql.conf.d/mysqld.cnf
sudo systemctl restart mysql
sudo ufw allow 3306
```

## 第三方库：boost、muduo、nlohmann/json 等相关库
首先是 boost：
```bash
wget https://www.programmercarl.com/download/boost_1_69_0.tar.gz
tar -zxvf boost_1_69_0.tar.gz
cd boost_1_69_0
./bootstrap.sh
./b2
sudo ./b2 install
cd ..
```

接着是 Muduo：
```bash
wget https://www.programmercarl.com/download/muduo-master.zip
sudo apt install unzip
unzip muduo-master.zip
cd muduo-master
sudo chmod +x build.sh
./build.sh
./build.sh install
cd ..
cd build
cd release-install-cpp11
cd include
sudo mv muduo/ /usr/include/
cd ..
cd lib/
sudo mv * /usr/local/lib
```

然后是 nlohmann/json，一直回车就行：
```bash
sudo apt upgrade
sudo apt install nlohmann-json3-dev
```

## c++ mysql 库
```bash
sudo apt install libmysqlcppconn-dev
```
## 安装 OpenSSL 开发库
```bash
sudo apt install libssl-dev
```

# 安装框架：
通过 git 安装：

```bash
git init
git clone git@github.com:IncredibleJ1021/HTTP_project.git
```

已有 build 文件夹，如果想重新部署的话可以删除掉后重新部署：
```bash
mkdir build
cd build
cmake ..
make
```

运行：
```bash
sudo ./simple_server
```

如果您是云服务器上部署的，然后访问云服务器公网 IP 地址即可；否则也可以访问 localhost。