/*
**      ARTSAT Project
**
**      Original Copyright (C) 2014 - 2014 Ron Hashimoto.
**                                          http://h2so5.net/
**                                          mail@h2so5.net
**      Portions Copyright (C) 2014 - 2015 HORIGUCHI Junshi.
**                                          http://iridium.jp/
**                                          zap00365@nifty.com
**      Portions Copyright (C) <year> <author>
**                                          <website>
**                                          <e-mail>
**      Version     POSIX / C++11
**      Website     http://artsat.jp/
**      E-mail      info@artsat.jp
**
**      This source code is for Xcode.
**      Xcode 6.1 (Apple LLVM 6.0)
**
**      ASDServerRPC.cpp
**
**      ------------------------------------------------------------------------
**
**      The MIT License (MIT)
**
**      Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
**      associated documentation files (the "Software"), to deal in the Software without restriction,
**      including without limitation the rights to use, copy, modify, merge, publish, distribute,
**      sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
**      furnished to do so, subject to the following conditions:
**      The above copyright notice and this permission notice shall be included in all copies or
**      substantial portions of the Software.
**      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
**      BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
**      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
**      WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
**      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
**      以下に定める条件に従い、本ソフトウェアおよび関連文書のファイル（以下「ソフトウェア」）の複製を
**      取得するすべての人に対し、ソフトウェアを無制限に扱うことを無償で許可します。
**      これには、ソフトウェアの複製を使用、複写、変更、結合、掲載、頒布、サブライセンス、および、または販売する権利、
**      およびソフトウェアを提供する相手に同じことを許可する権利も無制限に含まれます。
**      上記の著作権表示および本許諾表示を、ソフトウェアのすべての複製または重要な部分に記載するものとします。
**      ソフトウェアは「現状のまま」で、明示であるか暗黙であるかを問わず、何らの保証もなく提供されます。
**      ここでいう保証とは、商品性、特定の目的への適合性、および権利非侵害についての保証も含みますが、それに限定されるものではありません。
**      作者または著作権者は、契約行為、不法行為、またはそれ以外であろうと、ソフトウェアに起因または関連し、
**      あるいはソフトウェアの使用またはその他の扱いによって生じる一切の請求、損害、その他の義務について何らの責任も負わないものとします。
*/

#include "ASDServerRPC.h"
#include "writer.h"
#include "stringbuffer.h"
#include "artsatd.h"

#define JSON_VERSION                            ("2.0")
#define TIME_FORMAT                             ("%YYYY/%MM/%DD %hh:%mm:%ss UTC")

struct MethodTableRec {
    char const*                 name;
    ASDServerRPC::Method        method;
};
template <class T>
class Writer : public rapidjson::Writer<T> {
    public:
        explicit                Writer          (T& stream) : rapidjson::Writer<T>(stream)
                {
                }
    
                bool            Double          (double param)
                {
                    char buffer[384];
                    int ret;
                    int i;
                    
#if _MSC_VER
                    ret = sprintf_s(buffer, sizeof(buffer), "%.9f", param);
#else
                    ret = snprintf(buffer, sizeof(buffer), "%.9f", param);
#endif
                    RAPIDJSON_ASSERT(ret >= 1);
                    if (!isnan(param) && !isinf(param)) {
                        this->Prefix(rapidjson::kNumberType);
                        for (i = 0; i < ret; ++i) {
                            this->os_->Put(buffer[i]);
                        }
                    }
                    else {
                        this->Prefix(rapidjson::kStringType);
                        this->WriteString(buffer, static_cast<rapidjson::SizeType>(strlen(buffer)));
                    }
                    return true;
                }
};

static  MethodTableRec const                    g_method[] = {
    {"system.rpcEcho",                      &ASDServerRPC::rpcEcho},
    {"observer.getVersion",                 &ASDServerRPC::getVersion},
    {"observer.getSession",                 &ASDServerRPC::getSession},
    {"observer.setManualRotator",           &ASDServerRPC::setManualRotator},
    {"observer.getManualRotator",           &ASDServerRPC::getManualRotator},
    {"observer.setManualTransceiver",       &ASDServerRPC::setManualTransceiver},
    {"observer.getManualTransceiver",       &ASDServerRPC::getManualTransceiver},
    {"observer.setManualTNC",               &ASDServerRPC::setManualTNC},
    {"observer.getManualTNC",               &ASDServerRPC::getManualTNC},
    {"observer.setEXNORAD",                 &ASDServerRPC::setEXNORAD},
    {"observer.getEXNORAD",                 &ASDServerRPC::getEXNORAD},
    {"observer.setMode",                    &ASDServerRPC::setMode},
    {"observer.getMode",                    &ASDServerRPC::getMode},
    {"observer.getTime",                    &ASDServerRPC::getTime},
    {"observer.getObserverCallsign",        &ASDServerRPC::getObserverCallsign},
    {"observer.getObserverPosition",        &ASDServerRPC::getObserverPosition},
    {"observer.getObserverDirection",       &ASDServerRPC::getObserverDirection},
    {"observer.getObserverFrequency",       &ASDServerRPC::getObserverFrequency},
    {"observer.getSpacecraftPosition",      &ASDServerRPC::getSpacecraftPosition},
    {"observer.getSpacecraftDirection",     &ASDServerRPC::getSpacecraftDirection},
    {"observer.getSpacecraftDistance",      &ASDServerRPC::getSpacecraftDistance},
    {"observer.getSpacecraftSpeed",         &ASDServerRPC::getSpacecraftSpeed},
    {"observer.getSpacecraftFrequency",     &ASDServerRPC::getSpacecraftFrequency},
    {"observer.getSpacecraftDopplerShift",  &ASDServerRPC::getSpacecraftDopplerShift},
    {"observer.getSpacecraftAOSLOS",        &ASDServerRPC::getSpacecraftAOSLOS},
    {"observer.getSpacecraftMEL",           &ASDServerRPC::getSpacecraftMEL},
    {"observer.getRotatorStart",            &ASDServerRPC::getRotatorStart},
    {"observer.getError",                   &ASDServerRPC::getError},
    {"observer.isValidRotator",             &ASDServerRPC::isValidRotator},
    {"observer.isValidTransceiver",         &ASDServerRPC::isValidTransceiver},
    {"observer.isValidTNC",                 &ASDServerRPC::isValidTNC},
    {"observer.controlSession",             &ASDServerRPC::controlSession},
    {"observer.excludeSession",             &ASDServerRPC::excludeSession},
    {"observer.setRotatorAzimuth",          &ASDServerRPC::setRotatorAzimuth},
    {"observer.setRotatorElevation",        &ASDServerRPC::setRotatorElevation},
    {"observer.setTransceiverMode",         &ASDServerRPC::setTransceiverMode},
    {"observer.setTransceiverSender",       &ASDServerRPC::setTransceiverSender},
    {"observer.setTransceiverReceiver",     &ASDServerRPC::setTransceiverReceiver},
    {"observer.setTNCMode",                 &ASDServerRPC::setTNCMode},
    {"observer.sendTNCPacket",              &ASDServerRPC::sendTNCPacket},
    {"observer.requestCommand",             &ASDServerRPC::requestCommand},
    {"database.setName",                    &ASDServerRPC::setName},
    {"database.getName",                    &ASDServerRPC::getName},
    {"database.setCallsign",                &ASDServerRPC::setCallsign},
    {"database.getCallsign",                &ASDServerRPC::getCallsign},
    {"database.setRadioBeacon",             &ASDServerRPC::setRadioBeacon},
    {"database.getRadioBeacon",             &ASDServerRPC::getRadioBeacon},
    {"database.setRadioSender",             &ASDServerRPC::setRadioSender},
    {"database.getRadioSender",             &ASDServerRPC::getRadioSender},
    {"database.setRadioReceiver",           &ASDServerRPC::setRadioReceiver},
    {"database.getRadioReceiver",           &ASDServerRPC::getRadioReceiver},
    {"database.setOrbitData",               &ASDServerRPC::setOrbitData},
    {"database.getOrbitData",               &ASDServerRPC::getOrbitData},
    {"database.getCount",                   &ASDServerRPC::getCount},
    {"database.getField",                   &ASDServerRPC::getField},
    {"database.getFieldByName",             &ASDServerRPC::getFieldByName},
    {"database.getFieldByCallsign",         &ASDServerRPC::getFieldByCallsign},
    {"database.getEXNORADByName",           &ASDServerRPC::getEXNORADByName},
    {"database.getEXNORADByCallsign",       &ASDServerRPC::getEXNORADByCallsign}
};

/*public */ASDServerRPC::ASDServerRPC(void)
{
}

/*public virtual */ASDServerRPC::~ASDServerRPC(void)
{
    close();
}

/*public */tgs::TGSError ASDServerRPC::open(std::string const& database)
{
    int i;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    close();
    for (i = 0; i < lengthof(g_method); ++i) {
        _method[g_method[i].name] = g_method[i].method;
    }
    _database = database;
    if (error != tgs::TGSERROR_OK) {
        close();
    }
    return error;
}

/*public */void ASDServerRPC::close(void)
{
    _method.clear();
    return;
}

/*private virtual */tgs::TGSError ASDServerRPC::onRequest(RequestRec const& request, ResponseRec* response)
{
    if (boost::iequals(request.path, "/rpc.json")) {
        replyJSON(request, response);
    }
    else {
        replyStatus(Server::response::not_found, response);
    }
    return tgs::TGSERROR_OK;
}

/*private */void ASDServerRPC::replyJSON(RequestRec const& request, ResponseRec* response) const
{
    rapidjson::Document idoc;
    rapidjson::Document odoc;
    rapidjson::Value::ValueIterator it;
    rapidjson::Value result;
    rapidjson::StringBuffer buffer;
    Writer<rapidjson::StringBuffer> writer(buffer);
    
    if (idoc.Parse<0>(request.content.c_str()).HasParseError()) {
        returnJSON(JSONCODE_PARSEERROR, result, result, &odoc, odoc.GetAllocator());
    }
    else if (!idoc.IsArray()) {
        processJSON(request.host, idoc, &odoc, odoc.GetAllocator());
    }
    else if (idoc.Empty()) {
        returnJSON(JSONCODE_INVALIDREQUEST, result, result, &odoc, odoc.GetAllocator());
    }
    else {
        odoc.SetArray();
        for (it = idoc.Begin(); it != idoc.End(); ++it) {
            processJSON(request.host, *it, &result, odoc.GetAllocator());
            if (!result.IsNull()) {
                odoc.PushBack(result, odoc.GetAllocator());
            }
        }
        if (odoc.Empty()) {
            odoc.SetNull();
        }
    }
    if (!odoc.IsNull()) {
        odoc.Accept(writer);
        response->content = buffer.GetString();
    }
    else {
        response->status = Server::response::no_content;
        response->content.clear();
    }
    response->header["Content-Type"] = "application/json";
    response->header["Cache-Control"] = "no-cache";
    response->header["Access-Control-Allow-Origin"] = "*";
    return;
}

/*private */void ASDServerRPC::processJSON(std::string const& host, rapidjson::Value& request, rapidjson::Value* response, rapidjson::Document::AllocatorType& allocator) const
{
    std::map<std::string, Method>::const_iterator it;
    rapidjson::Value jsonrpc;
    rapidjson::Value method;
    rapidjson::Value params;
    rapidjson::Value iid;
    rapidjson::Value result;
    rapidjson::Value oid;
    Variant variant;
    Param mparam;
    Param mresult;
    bool notification;
    JSONCodeEnum code;
    tgs::TGSError error;
    
    notification = false;
    code = JSONCODE_OK;
    if (request.IsObject()) {
        if (request.HasMember("jsonrpc")) {
            jsonrpc = request["jsonrpc"];
            if (jsonrpc.IsString()) {
                if (boost::iequals(jsonrpc.GetString(), JSON_VERSION)) {
                    if (request.HasMember("id")) {
                        iid = request["id"];
                        if (iid.IsInt() || iid.IsString()) {
                            oid = iid;
                        }
                        else if (!iid.IsNull()) {
                            code = JSONCODE_INVALIDREQUEST;
                        }
                    }
                    else {
                        notification = true;
                    }
                    if (code == JSONCODE_OK) {
                        if (request.HasMember("method")) {
                            method = request["method"];
                            if (method.IsString()) {
                                if (request.HasMember("params")) {
                                    params = request["params"];
                                    if (params.IsObject()) {
                                        toVariant(params, &variant);
                                        mparam = boost::get<Param>(variant);
                                    }
                                    else if (params.IsArray()) {
                                        code = JSONCODE_INVALIDPARAMS;
                                    }
                                    else if (!params.IsNull()) {
                                        code = JSONCODE_INVALIDREQUEST;
                                    }
                                }
                                if (code == JSONCODE_OK) {
                                    if ((it = _method.find(method.GetString())) != _method.end()) {
                                        error = it->second(this, host, mparam, &mresult);
                                        toJSON(mresult, &result, allocator);
                                        switch (error) {
                                            case tgs::TGSERROR_OK:
                                                // nop
                                                break;
                                            case tgs::TGSERROR_INVALID_PARAM:
                                            case tgs::TGSERROR_INVALID_FORMAT:
                                            case tgs::TGSERROR_NO_RESULT:
                                                code = JSONCODE_INVALIDPARAMS;
                                                break;
                                            default:
                                                code = JSONCODE_INTERNALERROR;
                                                break;
                                        }
                                    }
                                    else {
                                        code = JSONCODE_METHODNOTFOUND;
                                    }
                                }
                            }
                            else {
                                code = JSONCODE_INVALIDREQUEST;
                            }
                        }
                        else {
                            code = JSONCODE_INVALIDREQUEST;
                        }
                    }
                }
                else {
                    code = JSONCODE_INVALIDREQUEST;
                }
            }
            else {
                code = JSONCODE_INVALIDREQUEST;
            }
        }
        else {
            code = JSONCODE_INVALIDREQUEST;
        }
    }
    else {
        code = JSONCODE_INVALIDREQUEST;
    }
    if (notification && code == JSONCODE_OK) {
        response->SetNull();
    }
    else {
        returnJSON(code, result, oid, response, allocator);
    }
    return;
}

/*private static */void ASDServerRPC::returnJSON(JSONCodeEnum code, rapidjson::Value& result, rapidjson::Value& id, rapidjson::Value* response, rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Value error;
    
    response->SetObject();
    response->AddMember("jsonrpc", JSON_VERSION, allocator);
    if (code == JSONCODE_OK) {
        response->AddMember("result", result, allocator);
    }
    else {
        error.SetObject();
        error.AddMember("code", code, allocator);
        switch (code) {
            case JSONCODE_PARSEERROR:
                error.AddMember("message", "Parse error", allocator);
                break;
            case JSONCODE_INVALIDREQUEST:
                error.AddMember("message", "Invalid Request", allocator);
                break;
            case JSONCODE_METHODNOTFOUND:
                error.AddMember("message", "Method not found", allocator);
                break;
            case JSONCODE_INVALIDPARAMS:
                error.AddMember("message", "Invalid params", allocator);
                break;
            case JSONCODE_INTERNALERROR:
                error.AddMember("message", "Internal error", allocator);
                break;
            default:
                error.AddMember("message", "Server error", allocator);
                break;
        }
        if (!result.IsNull()) {
            error.AddMember("data", result, allocator);
        }
        response->AddMember("error", error, allocator);
    }
    response->AddMember("id", id, allocator);
    return;
}

/*private static */void ASDServerRPC::toVariant(rapidjson::Value const& param, Variant* result)
{
    Variant value;
    
    switch (param.GetType()) {
        case rapidjson::kFalseType:
            *result = false;
            break;
        case rapidjson::kTrueType:
            *result = true;
            break;
        case rapidjson::kNumberType:
            *result = param.GetDouble();
            break;
        case rapidjson::kStringType:
            *result = static_cast<std::string>(param.GetString());
            break;
        case rapidjson::kArrayType:
            {
                std::list<Variant> list;
                rapidjson::Value::ConstValueIterator it;
                for (it = param.Begin(); it != param.End(); ++it) {
                    toVariant(*it, &value);
                    list.push_back(value);
                }
                *result = list;
            }
            break;
        case rapidjson::kObjectType:
            {
                std::map<std::string, Variant> map;
                rapidjson::Value::ConstMemberIterator it;
                for (it = param.MemberBegin(); it != param.MemberEnd(); ++it) {
                    toVariant(it->value, &value);
                    map[it->name.GetString()] = value;
                }
                *result = map;
            }
            break;
        default:
            *result = boost::blank();
            break;
    }
    return;
}

/*private static */void ASDServerRPC::toJSON(Variant const& param, rapidjson::Value* result, rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Value value;
    
    switch (param.which()) {
        case VARIANTTYPE_BOOL:
            result->SetBool(boost::get<bool>(param));
            break;
        case VARIANTTYPE_INT:
            result->SetInt(boost::get<int>(param));
            break;
        case VARIANTTYPE_DOUBLE:
            result->SetDouble(boost::get<double>(param));
            break;
        case VARIANTTYPE_STRING:
            result->SetString(boost::get<std::string>(param).c_str(), allocator);
            break;
        case VARIANTTYPE_LIST:
            {
                result->SetArray();
                std::list<Variant> const& list(boost::get<std::list<Variant> >(param));
                std::list<Variant>::const_iterator it;
                for (it = list.begin(); it != list.end(); ++it) {
                    toJSON(*it, &value, allocator);
                    result->PushBack(value, allocator);
                }
            }
            break;
        case VARIANTTYPE_MAP:
            {
                result->SetObject();
                std::map<std::string, Variant> const& map(boost::get<std::map<std::string, Variant> >(param));
                std::map<std::string, Variant>::const_iterator it;
                for (it = map.begin(); it != map.end(); ++it) {
                    toJSON(it->second, &value, allocator);
                    result->AddMember(rapidjson::Value(it->first.c_str(), allocator), value, allocator);
                }
            }
            break;
        default:
            result->SetNull();
            break;
    }
    return;
}

/*public */tgs::TGSError ASDServerRPC::rpcEcho(std::string const& host, Param const& param, Param* result) const
{
    *result = param;
    return tgs::TGSERROR_OK;
}

/*public */tgs::TGSError ASDServerRPC::getVersion(std::string const& host, Param const& param, Param* result) const
{
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, NULL, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getInstance().getVersion(), "version", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSession(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int owner;
    bool exclusive;
    std::string current;
    int online;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSession(session, &owner, &exclusive, &current, &online);
        setResult(owner, "owner", result);
        setResult(exclusive, "exclusive", result);
        setResult(current, "host", result);
        setResult(online, "online", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setManualRotator(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    bool manual;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "manual", &manual)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().setManualRotator(session, manual)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getManualRotator(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getInstance().getManualRotator(), "manual", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setManualTransceiver(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    bool manual;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "manual", &manual)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().setManualTransceiver(session, manual)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getManualTransceiver(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getInstance().getManualTransceiver(), "manual", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setManualTNC(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    bool manual;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "manual", &manual)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().setManualTNC(session, manual)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getManualTNC(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getInstance().getManualTNC(), "manual", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setEXNORAD(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    std::string query;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().setEXNORAD(session, exnorad)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
        else if (error == tgs::TGSERROR_NO_RESULT) {
            if ((error = getParam(param, "query", &query)) == tgs::TGSERROR_OK) {
                if ((error = artsatd::getInstance().setEXNORAD(session, query)) != tgs::TGSERROR_OK) {
                    error = setError(error, result);
                }
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getEXNORAD(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getInstance().getEXNORAD(), "exnorad", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setMode(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    std::string mode;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "mode", &mode)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().setMode(session, mode)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getMode(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getInstance().getMode(), "mode", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getTime(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult((artsatd::getInstance().getTime() - ir::IRXTimeDiff::localTimeOffset()).format(TIME_FORMAT), "time", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getObserverCallsign(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    std::string callsign;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getObserverCallsign(&callsign);
        setResult(callsign, "callsign", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getObserverPosition(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    double latitude;
    double longitude;
    double altitude;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getObserverPosition(&latitude, &longitude, &altitude);
        setResult(latitude, "latitude", result);
        setResult(longitude, "longitude", result);
        setResult(altitude, "altitude", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getObserverDirection(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    ASDDeviceRotator::DataRec rotator;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getRotator().getData(&rotator);
        setResult(rotator.azimuth, "azimuth", result);
        setResult(rotator.elevation, "elevation", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getObserverFrequency(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    ASDDeviceTransceiver::DataRec transceiver;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getTransceiver().getData(&transceiver);
        setResult(transceiver.frequencySender, "sender", result);
        setResult(transceiver.frequencyReceiver, "receiver", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSpacecraftPosition(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    double latitude;
    double longitude;
    double altitude;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSpacecraftPosition(&latitude, &longitude, &altitude);
        setResult(latitude, "latitude", result);
        setResult(longitude, "longitude", result);
        setResult(altitude, "altitude", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSpacecraftDirection(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    double azimuth;
    double elevation;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSpacecraftDirection(&azimuth, &elevation);
        setResult(azimuth, "azimuth", result);
        setResult(elevation, "elevation", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSpacecraftDistance(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    double distance;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    // TODO: need to modify
    //if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSpacecraftDistance(&distance);
        setResult(distance, "distance", result);
    //}
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSpacecraftSpeed(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    double speed;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSpacecraftSpeed(&speed);
        setResult(speed, "speed", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSpacecraftFrequency(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    double beacon;
    double sender;
    double receiver;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSpacecraftFrequency(&beacon, &sender, &receiver);
        setResult(beacon, "beacon", result);
        setResult(sender, "sender", result);
        setResult(receiver, "receiver", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSpacecraftDopplerShift(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    double sender;
    double receiver;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSpacecraftDopplerShift(&sender, &receiver);
        setResult(sender, "sender", result);
        setResult(receiver, "receiver", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSpacecraftAOSLOS(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    ir::IRXTime aos;
    ir::IRXTime los;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSpacecraftAOSLOS(&aos, &los);
        setResult((aos - ir::IRXTimeDiff::localTimeOffset()).format(TIME_FORMAT), "aos", result);
        setResult((los - ir::IRXTimeDiff::localTimeOffset()).format(TIME_FORMAT), "los", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getSpacecraftMEL(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    double mel;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getSpacecraftMEL(&mel);
        setResult(mel, "mel", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getRotatorStart(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    ir::IRXTime start;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        artsatd::getInstance().getRotatorStart(&start);
        setResult((start - ir::IRXTimeDiff::localTimeOffset()).format(TIME_FORMAT), "start", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getError(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError state;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        state = artsatd::getInstance().getError();
        setResult(state.get(), "code", result);
        setResult(state.print(), "message", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::isValidRotator(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getRotator().isValid(), "valid", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::isValidTransceiver(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getTransceiver().isValid(), "valid", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::isValidTNC(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        setResult(artsatd::getTNC().isValid(), "valid", result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::controlSession(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    bool owner;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "owner", &owner)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().controlSession(session, owner, host)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::excludeSession(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    bool exclusive;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exclusive", &exclusive)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().excludeSession(session, exclusive)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setRotatorAzimuth(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int azimuth;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "azimuth", &azimuth)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().controlManualRotator(session, &artsatd::controlRotatorAzimuth, &azimuth)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setRotatorElevation(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int elevation;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "elevation", &elevation)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().controlManualRotator(session, &artsatd::controlRotatorElevation, &elevation)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setTransceiverMode(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    std::string mode;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "mode", &mode)) == tgs::TGSERROR_OK) {
            if (mode == "CW") {
                error = artsatd::getInstance().controlManualTransceiver(session, &artsatd::controlTransceiverModeCW, NULL);
            }
            else if (mode == "FM") {
                error = artsatd::getInstance().controlManualTransceiver(session, &artsatd::controlTransceiverModeFM, NULL);
            }
            else {
                error = tgs::TGSERROR_INVALID_PARAM;
            }
            if (error != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setTransceiverSender(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int sender;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "sender", &sender)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().controlManualTransceiver(session, &artsatd::controlTransceiverSender, &sender)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setTransceiverReceiver(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int receiver;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "receiver", &receiver)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().controlManualTransceiver(session, &artsatd::controlTransceiverReceiver, &receiver)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setTNCMode(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    std::string mode;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "mode", &mode)) == tgs::TGSERROR_OK) {
            if (mode == "Command") {
                error = artsatd::getInstance().controlManualTNC(session, &artsatd::controlTNCModeCommand, NULL);
            }
            else if (mode == "Converse") {
                error = artsatd::getInstance().controlManualTNC(session, &artsatd::controlTNCModeConverse, NULL);
            }
            else {
                error = tgs::TGSERROR_INVALID_PARAM;
            }
            if (error != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::sendTNCPacket(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    std::string packet;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "packet", &packet)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().controlManualTNC(session, &artsatd::controlTNCPacket, &packet)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::requestCommand(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    std::string command;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "command", &command)) == tgs::TGSERROR_OK) {
            if ((error = artsatd::getInstance().requestCommand(session, command)) != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setName(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    std::string name;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = getParam(param, "name", &name)) == tgs::TGSERROR_OK) {
                // TODO: need to lock the session
                if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                    error = database.setName(exnorad, name);
                    database.close();
                }
                if (error != tgs::TGSERROR_OK) {
                    error = setError(error, result);
                }
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getName(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase database;
    std::string name;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                if ((error = database.getName(exnorad, &name)) == tgs::TGSERROR_OK) {
                    setResult(name, "name", result);
                }
                database.close();
            }
            if (error != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setCallsign(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    std::string callsign;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = getParam(param, "callsign", &callsign)) == tgs::TGSERROR_OK) {
                // TODO: need to lock the session
                if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                    error = database.setCallsign(exnorad, callsign);
                    database.close();
                }
                if (error != tgs::TGSERROR_OK) {
                    error = setError(error, result);
                }
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getCallsign(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase database;
    std::string callsign;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                if ((error = database.getCallsign(exnorad, &callsign)) == tgs::TGSERROR_OK) {
                    setResult(callsign, "callsign", result);
                }
                database.close();
            }
            if (error != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setRadioBeacon(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase::RadioRec radio;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = getParam(param, &radio)) == tgs::TGSERROR_OK) {
                // TODO: need to lock the session
                if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                    if ((error = database.getRadioBeacon(exnorad, &radio)) == tgs::TGSERROR_OK) {
                        if ((error = getParam(param, &radio)) == tgs::TGSERROR_OK) {
                            error = database.setRadioBeacon(exnorad, radio);
                        }
                    }
                    database.close();
                }
                if (error != tgs::TGSERROR_OK) {
                    error = setError(error, result);
                }
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getRadioBeacon(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSPhysicsDatabase::RadioRec radio;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                if ((error = database.getRadioBeacon(exnorad, &radio)) == tgs::TGSERROR_OK) {
                    setResult(radio, result);
                }
                database.close();
            }
            if (error != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setRadioSender(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase::RadioRec radio;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = getParam(param, &radio)) == tgs::TGSERROR_OK) {
                // TODO: need to lock the session
                if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                    if ((error = database.getRadioSender(exnorad, &radio)) == tgs::TGSERROR_OK) {
                        if ((error = getParam(param, &radio)) == tgs::TGSERROR_OK) {
                            error = database.setRadioSender(exnorad, radio);
                        }
                    }
                    database.close();
                }
                if (error != tgs::TGSERROR_OK) {
                    error = setError(error, result);
                }
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getRadioSender(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSPhysicsDatabase::RadioRec radio;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                if ((error = database.getRadioSender(exnorad, &radio)) == tgs::TGSERROR_OK) {
                    setResult(radio, result);
                }
                database.close();
            }
            if (error != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setRadioReceiver(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase::RadioRec radio;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = getParam(param, &radio)) == tgs::TGSERROR_OK) {
                // TODO: need to lock the session
                if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                    if ((error = database.getRadioReceiver(exnorad, &radio)) == tgs::TGSERROR_OK) {
                        if ((error = getParam(param, &radio)) == tgs::TGSERROR_OK) {
                            error = database.setRadioReceiver(exnorad, radio);
                        }
                    }
                    database.close();
                }
                if (error != tgs::TGSERROR_OK) {
                    error = setError(error, result);
                }
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getRadioReceiver(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSPhysicsDatabase::RadioRec radio;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                if ((error = database.getRadioReceiver(exnorad, &radio)) == tgs::TGSERROR_OK) {
                    setResult(radio, result);
                }
                database.close();
            }
            if (error != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::setOrbitData(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::OrbitData orbit;
    std::string string;
    ir::IRXTime time;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, &orbit)) == tgs::TGSERROR_OK) {
            if ((error = getParam(param, "time", &string)) == tgs::TGSERROR_OK) {
                error = time.parse(TIME_FORMAT, string);
            }
            else if (error == tgs::TGSERROR_NO_RESULT) {
                error = tgs::TGSERROR_OK;
                time = ir::IRXTime::currentUTCTime();
            }
            if (error == tgs::TGSERROR_OK) {
                // TODO: need to lock the session
                if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                    error = database.setOrbitData(orbit, time);
                    database.close();
                }
                if (error != tgs::TGSERROR_OK) {
                    error = setError(error, result);
                }
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getOrbitData(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    int exnorad;
    tgs::TGSPhysicsDatabase database;
    tgs::OrbitData orbit;
    ir::IRXTime time;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = getParam(param, "exnorad", &exnorad)) == tgs::TGSERROR_OK) {
            if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                if ((error = database.getOrbitData(exnorad, &orbit, &time)) == tgs::TGSERROR_OK) {
                    setResult(orbit, result);
                    setResult(time.format(TIME_FORMAT), "time", result);
                }
                database.close();
            }
            if (error != tgs::TGSERROR_OK) {
                error = setError(error, result);
            }
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getCount(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSPhysicsDatabase database;
    int count;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
            if ((error = database.getCount(&count)) == tgs::TGSERROR_OK) {
                setResult(count, "count", result);
            }
            database.close();
        }
        if (error != tgs::TGSERROR_OK) {
            error = setError(error, result);
        }
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getField(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        error = setError(tgs::TGSERROR_NO_SUPPORT, result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getFieldByName(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        error = setError(tgs::TGSERROR_NO_SUPPORT, result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getFieldByCallsign(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        error = setError(tgs::TGSERROR_NO_SUPPORT, result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getEXNORADByName(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        error = setError(tgs::TGSERROR_NO_SUPPORT, result);
    }
    return error;
}

/*public */tgs::TGSError ASDServerRPC::getEXNORADByCallsign(std::string const& host, Param const& param, Param* result) const
{
    std::string session;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = updateSession(host, param, &session, result)) == tgs::TGSERROR_OK) {
        error = setError(tgs::TGSERROR_NO_SUPPORT, result);
    }
    return error;
}

/*private static */tgs::TGSError ASDServerRPC::updateSession(std::string const& host, Param const& param, std::string* session, Param* result)
{
    std::string string;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    error = getParam(param, "session", &string);
    switch (error) {
        case tgs::TGSERROR_OK:
        case tgs::TGSERROR_NO_RESULT:
            if ((error = artsatd::getInstance().requestSession(host, &string, NULL)) == tgs::TGSERROR_OK) {
                setResult(string, "session", result);
                if (session != NULL) {
                    *session = string;
                }
            }
            else {
                error = setError(error, result);
            }
            break;
        default:
            // nop
            break;
    }
    return error;
}

/*private static */tgs::TGSError ASDServerRPC::getParam(Param const& param, tgs::TGSPhysicsDatabase::RadioRec* result)
{
    tgs::TGSError trial;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    error = tgs::TGSERROR_NO_RESULT;
    if ((trial = getParam(param, "mode", &result->mode)) != tgs::TGSERROR_NO_RESULT) {
        error = trial;
    }
    if (error == tgs::TGSERROR_OK || error == tgs::TGSERROR_NO_RESULT) {
        if ((trial = getParam(param, "frequency", &result->frequency)) != tgs::TGSERROR_NO_RESULT) {
            error = trial;
        }
        if (error == tgs::TGSERROR_OK || error == tgs::TGSERROR_NO_RESULT) {
            if ((trial = getParam(param, "drift", &result->drift)) != tgs::TGSERROR_NO_RESULT) {
                error = trial;
            }
        }
    }
    return error;
}

/*private static */tgs::TGSError ASDServerRPC::getParam(Param const& param, tgs::OrbitData* result)
{
    Param object;
    std::string name;
    std::string data1;
    std::string data2;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((error = getParam(param, "tle", &object)) == tgs::TGSERROR_OK) {
        if ((error = getParam(object, "name", &name)) == tgs::TGSERROR_OK) {
            if ((error = getParam(object, "one", &data1)) == tgs::TGSERROR_OK) {
                if ((error = getParam(object, "two", &data2)) == tgs::TGSERROR_OK) {
                    error = convertTLE(name, data1, data2, result);
                }
            }
        }
    }
    else if (error == tgs::TGSERROR_NO_RESULT) {
        if ((error = getParam(param, "scd", &object)) == tgs::TGSERROR_OK) {
            if ((error = getParam(object, "name", &name)) == tgs::TGSERROR_OK) {
                if ((error = getParam(object, "info", &data1)) == tgs::TGSERROR_OK) {
                    if ((error = getParam(object, "param", &data2)) == tgs::TGSERROR_OK) {
                        error = convertSCD(name, data1, data2, result);
                    }
                }
            }
        }
        else if (error == tgs::TGSERROR_NO_RESULT) {
            if ((error = getParam(param, "none", &object)) == tgs::TGSERROR_OK) {
                *result = tgs::OrbitData();
            }
        }
    }
    return error;
}

template <typename T>
/*private static */tgs::TGSError ASDServerRPC::getParam(Param const& param, std::string const& key, T* result)
{
    Param::const_iterator it;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((it = param.find(key)) != param.end()) {
        if (it->second.type() == typeid(T)) {
            *result = boost::get<T>(it->second);
        }
        else {
            error = tgs::TGSERROR_INVALID_FORMAT;
        }
    }
    else {
        error = tgs::TGSERROR_NO_RESULT;
    }
    return error;
}

/*private static */tgs::TGSError ASDServerRPC::getParam(Param const& param, std::string const& key, int* result)
{
    Param::const_iterator it;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    if ((it = param.find(key)) != param.end()) {
        if (it->second.type() == typeid(double)) {
            *result = boost::get<double>(it->second);
        }
        else {
            error = tgs::TGSERROR_INVALID_FORMAT;
        }
    }
    else {
        error = tgs::TGSERROR_NO_RESULT;
    }
    return error;
}

/*private static */void ASDServerRPC::setResult(tgs::TGSPhysicsDatabase::RadioRec const& param, Param* result)
{
    setResult(param.mode, "mode", result);
    setResult(param.frequency, "frequency", result);
    setResult(param.drift, "drift", result);
    return;
}

/*private static */void ASDServerRPC::setResult(tgs::OrbitData const& param, Param* result)
{
    Param object;
    
    switch (param.getType()) {
        case tgs::OrbitData::TYPE_TLE:
            setResult(static_cast<tgs::TLERec const&>(param).name, "name", &object);
            setResult(static_cast<tgs::TLERec const&>(param).one, "one", &object);
            setResult(static_cast<tgs::TLERec const&>(param).two, "two", &object);
            setResult(object, "tle", result);
            break;
        case tgs::OrbitData::TYPE_SCD:
            setResult(static_cast<tgs::SCDRec const&>(param).name, "name", &object);
            setResult(static_cast<tgs::SCDRec const&>(param).info, "info", &object);
            setResult(static_cast<tgs::SCDRec const&>(param).param, "param", &object);
            setResult(object, "scd", result);
            break;
        default:
            setResult(object, "none", result);
            break;
    }
    return;
}

template <typename T>
/*private static */void ASDServerRPC::setResult(T const& param, std::string const& key, Param* result)
{
    (*result)[key] = param;
    return;
}

/*private static */tgs::TGSError ASDServerRPC::setError(tgs::TGSError error, Param* result)
{
    Param object;
    
    object["code"] = error;
    object["message"] = error.print();
    (*result)["error"] = object;
    return tgs::TGSERROR_FAILED;
}
