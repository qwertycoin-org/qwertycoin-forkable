// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014 - 2017, XDN - project developers
// Copyright (c) 2018, The Karbo developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// This file is part of Qwertycoin.
//
// Qwertycoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Qwertycoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Qwertycoin.  If not, see <http://www.gnu.org/licenses/>.

#include <fstream>
#include <future>
#include <memory>
#include <sstream>
#include <system_error>
#include <Common/JsonValue.h>
#include <Http/HttpParser.h>
#include <Http/HttpParserErrorCodes.h>
#include <Http/HttpResponse.h>
#include <JsonRpcServer/JsonRpcServer.h>
#include <Rpc/JsonRpc.h>
#include <Serialization/JsonInputValueSerializer.h>
#include <Serialization/JsonOutputStreamSerializer.h>
#include <System/TcpConnection.h>
#include <System/TcpListener.h>
#include <System/TcpStream.h>
#include <System/Ipv4Address.h>

namespace QwertyNote {

JsonRpcServer::JsonRpcServer(System::Dispatcher &sys,
                             System::Event &stopEvent,
                             Logging::ILogger &loggerGroup)
    : HttpServer(sys, loggerGroup),
      system(sys),
      stopEvent(stopEvent),
      logger(loggerGroup, "JsonRpcServer")
{
}

void JsonRpcServer::start(const std::string &bindAddress,
                          uint16_t bindPort,
                          const std::string &m_rpcUser,
                          const std::string &m_rpcPassword)
{
    HttpServer::start(bindAddress, bindPort, m_rpcUser, m_rpcPassword);
    stopEvent.wait();
    HttpServer::stop();
}

void JsonRpcServer::processRequest(const QwertyNote::HttpRequest &req,
                                   QwertyNote::HttpResponse &resp)
{
    try {
        logger(Logging::TRACE) << "HTTP request came: \n" << req;

        if (req.getUrl() == "/json_rpc") {
            std::istringstream jsonInputStream(req.getBody());
            Common::QJsonValue jsonRpcRequest;
            Common::QJsonValue jsonRpcResponse(Common::QJsonValue::OBJECT);

            try {
                jsonInputStream >> jsonRpcRequest;
            } catch (std::runtime_error &) {
                logger(Logging::DEBUGGING) << "Couldn't parse request: \"" << req.getBody() << "\"";
                makeJsonParsingErrorResponse(jsonRpcResponse);
                resp.setStatus(QwertyNote::HttpResponse::STATUS_200);
                resp.setBody(jsonRpcResponse.toString());
                return;
            }

            processJsonRpcRequest(jsonRpcRequest, jsonRpcResponse);

            std::ostringstream jsonOutputStream;
            jsonOutputStream << jsonRpcResponse;

            resp.setStatus(QwertyNote::HttpResponse::STATUS_200);
            resp.setBody(jsonOutputStream.str());
        } else {
            logger(Logging::WARNING) << "Requested url \"" << req.getUrl() << "\" is not found";
            resp.setStatus(QwertyNote::HttpResponse::STATUS_404);
            return;
        }
    } catch (std::exception &e) {
        logger(Logging::WARNING) << "Error while processing http request: " << e.what();
        resp.setStatus(QwertyNote::HttpResponse::STATUS_500);
    }
}

void JsonRpcServer::prepareJsonResponse(const Common::QJsonValue &req, Common::QJsonValue &resp)
{
    using Common::QJsonValue;

    if (req.contains("id")) {
        resp.insert("id", req("id"));
    }

    resp.insert("jsonrpc", "2.0");
}

void JsonRpcServer::makeErrorResponse(const std::error_code &ec, Common::QJsonValue &resp)
{
    using Common::QJsonValue;

    QJsonValue error(QJsonValue::OBJECT);

    QJsonValue code;
    // application specific error code
    code = static_cast<int64_t>(QwertyNote::JsonRpc::errParseError);

    QJsonValue message;
    message = ec.message();

    QJsonValue data(QJsonValue::OBJECT);
    QJsonValue appCode;
    appCode = static_cast<int64_t>(ec.value());
    data.insert("application_code", appCode);

    error.insert("code", code);
    error.insert("message", message);
    error.insert("uData", data);

    resp.insert("error", error);
}

void JsonRpcServer::makeGenericErrorReponse(Common::QJsonValue &resp, const char *what, int errorCode)
{
    using Common::QJsonValue;

    QJsonValue error(QJsonValue::OBJECT);

    QJsonValue code;
    code = static_cast<int64_t>(errorCode);

    std::string msg;
    if (what) {
        msg = what;
    } else {
        msg = "Unknown application error";
    }

    QJsonValue message;
    message = msg;

    error.insert("code", code);
    error.insert("message", message);

    resp.insert("error", error);

}

void JsonRpcServer::makeMethodNotFoundResponse(Common::QJsonValue &resp)
{
    using Common::QJsonValue;

    QJsonValue error(QJsonValue::OBJECT);

    QJsonValue code;
    // ambiguous declaration of QJsonValue::operator= (between int and QJsonValue)
    code = static_cast<int64_t>(QwertyNote::JsonRpc::errMethodNotFound);

    QJsonValue message;
    message = "Method not found";

    error.insert("code", code);
    error.insert("message", message);

    resp.insert("error", error);
}

void JsonRpcServer::fillJsonResponse(const Common::QJsonValue &v, Common::QJsonValue &resp)
{
    resp.insert("result", v);
}

void JsonRpcServer::makeJsonParsingErrorResponse(Common::QJsonValue &resp)
{
    using Common::QJsonValue;

    resp = QJsonValue(QJsonValue::OBJECT);
    resp.insert("jsonrpc", "2.0");
    resp.insert("id", nullptr);

    QJsonValue error(QJsonValue::OBJECT);
    QJsonValue code;
    // ambiguous declaration of QJsonValue::operator= (between int and QJsonValue)
    code = static_cast<int64_t>(QwertyNote::JsonRpc::errParseError);

    QJsonValue message = "Parse error";

    error.insert("code", code);
    error.insert("message", message);

    resp.insert("error", error);
}

} // namespace QwertyNote
