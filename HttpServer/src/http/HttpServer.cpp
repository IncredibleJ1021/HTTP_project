#include "../../include/http/HttpServer.h"

#include <any>
#include <functional>
#include <memory>

namespace http {
// 默认 http 回应函数
void defaultHttpCallback(const HttpRequest &, HttpResponse *resp) {
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(int port, const std::string &name, bool useSSL,
                       muduo::net::TcpServer::Option option)
    : listenAddr_(port), server_(&mainLoop_, listenAddr_, name, option),
      useSSL_(useSSL),
      httpCallback_(std::bind(&HttpServer::handleRequest, this,
                              std::placeholders::_1, std::placeholders::_2)) {
    initialize();
}

// 服务器运行函数
void HttpServer::start() {
    LOG_WARN << "HttpServer[" << server_.name() << "] starts listening on"
             << server_.ipPort();
    server_.start();
    mainLoop_.loop();
}

void HttpServer::initialize() {
    // 设置回调函数
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));
}

void HttpServer::setSslConfig(const ssl::SslConfig &config) {
    if (useSSL_) {
        sslCtx_ = std::make_unique<ssl::SslContext>(config);
        if (!sslCtx_->initialize()) {
            LOG_ERROR << "Failed to initialize SSL context";
            abort();
        }
    }
}

void HttpServer::onConnection(const muduo::net::TcpConnectionPtr &conn) {
    if (conn->connected()) {
        if (useSSL_) {
            auto SslConn =
                std::make_unique<ssl::SslConnection>(conn, sslCtx_.get());
            sslConn->setMessageCallback(
                std::bind(&HttpServer::onMessage, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3));
            sslConns_[conn] = std::move(sslConn);
            sslConns_[conn]->startHandshake();
        }
        conn->setContext(HttpContext());
    } else {
        if (useSSL_) {
            sslConns_.erase(conn);
        }
    }
}

void HttpServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,
                           muduo::Timestamp receiveTime) {
    try {
        // 这层判断只是代表是否支持 SSL
        if (useSSL_) {
        }
        // HttpContext对象用于解析出 buf 中的请求报文，并把报文的关键信息封装到
        // HttpRequest 对象中
        HttpContext *context =
            boost::any_cast<HttpContext>(conn->getMutableContext());
        if (!context->parseRequest(buf, receiveTime)) {
            // 如果解析 http 报文中出错
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }
        // 如果 buf 缓冲区中解析出一个完整的数据包才封装响应报文
        if (context->gotAll()) {
            onRequest(conn, context->request());
            context->reset();
        }
    } catch (const std::exception &e) {
        // 捕获异常，返回错误信息
        LOG_ERROR << "Exception in onMessage: " << e.what();
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
}

void HttpServer::onRequest(const muduo::net::TcpConnectionPtr &conn,
                           const HttpRequest &req) {
    const std::string &connection = req.getHeader("Connection");
    bool close = ((connection == "close") || (req.getVersion() == "HTTP/1.0" &&
                                              connection != "Keep-Alive"));
    HttpResponse response(close);

    // 根据报文信息来封装响应报文对象
    httpCallback_(req, &response); // 执行 onHttpCallback 函数

    // 可以给 response 设置一个成员，判断是否请求的是文件，如果是文件设置为
    // true，并且存在文件位置在这里 send 出去
    muduo::net::Buffer buf;
    response.appendToBuffer(&buf);
    // 打印完整的响应内容用于调试
    LOG_INFO << "Sending response:\n" << buf.toStringPiece().as_string();
    conn->send(&buf);
    // 如果是短连接的话，返回响应报文后就断开连接
    if (response.closeConnection()) {
        conn->shutdown();
    }
}

} // namespace http