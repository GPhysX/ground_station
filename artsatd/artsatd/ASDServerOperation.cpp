/*
**      ARTSAT Project
**
**      Original Copyright (C) 2013 - 2014 HORIGUCHI Junshi.
**                                          http://iridium.jp/
**                                          zap00365@nifty.com
**      Portions Copyright (C) <year> <author>
**                                          <website>
**                                          <e-mail>
**      Version     POSIX / C++03
**      Website     http://artsat.jp/
**      E-mail      info@artsat.jp
**
**      This source code is for Xcode.
**      Xcode 5.1.1 (Apple LLVM 5.1)
**
**      ASDServerOperation.cpp
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

#include "ASDServerOperation.h"
#include "writer.h"
#include "stringbuffer.h"
#include "artsatd.h"
#include "ASDServerRPCMethods.h"

#define COOKIE_SESSION_ID                       ("session")
#define COOKIE_ERROR_CATEGORY                   ("category")
#define COOKIE_ERROR_MESSAGE                    ("error")
#define COOKIE_SHRINK_HARDWARE                  ("hardware")
#define COOKIE_SHRINK_DATABASE                  ("database")
#define COOKIE_SHRINK_STATUS                    ("status")
#define COOKIE_SHRINK_CONTROL                   ("control")
#define COOKIE_MAXAGE                           (365 * 24 * 60 * 60)
#define CATEGORY_SESSION                        ("session")
#define CATEGORY_HARDWARE                       ("hardware")
#define CATEGORY_DATABASE                       ("database")
#define CATEGORY_CONTROL                        ("control")
#define SHRINK_SHOW                             ("show")
#define SHRINK_HIDE                             ("hide")
#define DEVICE_OK                               ("ok")
#define DEVICE_NG                               ("ng")
#define DEFAULT_TIME                            ("----/--/-- --:--:-- JST")
#define DEFAULT_TIMEDIFF                        ("&Delta; --:--:--")
#define FORM_DISABLED                           ("disabled")
#define FORM_READONLY                           ("readonly")

struct CacheTableRec {
    char const*                 path;
    char const*                 mime;
    char const*                 cache;
    char const*                 content;
    void  (ASDServerOperation::*function)       (ASDServerOperation::RequestRec const& request, ASDServerOperation::ResponseRec* response);
};
struct MethodTableRec {
    char const*                 name;
    ASDServerRPC::Method        method;
};

static  CacheTableRec const                     g_cache[] = {
    {"/",               "text/html",        "no-cache",              "/root.html",          &ASDServerOperation::replyRoot},
    {"/hardware.html",  "text/html",        "private, max-age=3600", "/hardware.html",      &ASDServerOperation::replyHardware},
    {"/orbital.html",   "text/html",        "private, max-age=3600", "/orbital.html",       &ASDServerOperation::replyOrbital},
    {"/streaming.html", "text/html",        "private, max-age=3600", "/streaming.html",     NULL},
    {"/webcam.html",    "text/html",        "private, max-age=3600", "/webcam.html",        NULL},
    {"/rpc.json",       "application/json", "no-cache",              "",                    &ASDServerOperation::replyJSONRPC},
    {"/default.css",    "text/css",         "private, max-age=3600", "/css/default.css",    NULL}
};
static  MethodTableRec const                    g_method[] = {
    {"sys.rpcEcho",                &ASDServerRPC::sys::rpcEcho},
    {"trans.setManualRotator",     &ASDServerRPC::trans::setManualRotator},
    {"trans.getManualRotator",     &ASDServerRPC::trans::getManualRotator},
    {"trans.setManualTransceiver", &ASDServerRPC::trans::setManualTransceiver},
    {"trans.getManualTransceiver", &ASDServerRPC::trans::getManualTransceiver},
    {"trans.setManualTNC",         &ASDServerRPC::trans::setManualTNC},
    {"trans.getManualTNC",         &ASDServerRPC::trans::getManualTNC},
    {"trans.getStateRotator",      &ASDServerRPC::trans::getStateRotator},
    {"trans.getStateTransceiver",  &ASDServerRPC::trans::getStateTransceiver},
    {"trans.getStateTNC",          &ASDServerRPC::trans::getStateTNC},
    {"trans.setMode",              &ASDServerRPC::trans::setMode},
    {"trans.getMode",              &ASDServerRPC::trans::getMode},
    {"trans.setNorad",             &ASDServerRPC::trans::setNorad},
    {"trans.getNorad",             &ASDServerRPC::trans::getNorad},
    {"trans.getAngleAzimuth",      &ASDServerRPC::trans::getAngleAzimuth},
    {"trans.getAngleElevation",    &ASDServerRPC::trans::getAngleElevation},
    {"trans.getFrequencyBeacon",   &ASDServerRPC::trans::getFrequencyBeacon},
    {"trans.getFrequencySender",   &ASDServerRPC::trans::getFrequencySender},
    {"trans.getFrequencyReceiver", &ASDServerRPC::trans::getFrequencyReceiver},
    {"trans.sendSafeCommand",      &ASDServerRPC::trans::sendSafeCommand},
    {"pass.getStateNearest",       &ASDServerRPC::pass::getStateNearest},
    {"db.setName",                 &ASDServerRPC::db::setName},
    {"db.getName",                 &ASDServerRPC::db::getName},
    {"db.setCallsign",             &ASDServerRPC::db::setCallsign},
    {"db.getCallsign",             &ASDServerRPC::db::getCallsign},
    {"db.setRadioBeacon",          &ASDServerRPC::db::setRadioBeacon},
    {"db.getRadioBeacon",          &ASDServerRPC::db::getRadioBeacon},
    {"db.setRadioSender",          &ASDServerRPC::db::setRadioSender},
    {"db.getRadioSender",          &ASDServerRPC::db::getRadioSender},
    {"db.setRadioReceiver",        &ASDServerRPC::db::setRadioReceiver},
    {"db.getRadioReceiver",        &ASDServerRPC::db::getRadioReceiver},
    {"db.setOrbitData",            &ASDServerRPC::db::setOrbitData},
    {"db.getOrbitData",            &ASDServerRPC::db::getOrbitData},
    {"db.getCount",                &ASDServerRPC::db::getCount},
    {"db.getField",                &ASDServerRPC::db::getField},
    {"db.getFieldByName",          &ASDServerRPC::db::getFieldByName},
    {"db.getFieldByCallsign",      &ASDServerRPC::db::getFieldByCallsign},
    {"db.getNoradByName",          &ASDServerRPC::db::getNoradByName},
    {"db.getNoradByCallsign",      &ASDServerRPC::db::getNoradByCallsign},
    {"db.hasUpdate",               &ASDServerRPC::db::hasUpdate}
};

/*public */ASDServerOperation::ASDServerOperation(void)
{
}

/*public virtual */ASDServerOperation::~ASDServerOperation(void)
{
    close();
}

/*public */tgs::TGSError ASDServerOperation::open(std::string const& skeleton, std::string const& database)
{
    CacheRec cache;
    int i;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    close();
    for (i = 0; i < lengthof(g_cache); ++i) {
        cache.content.clear();
        if (strlen(g_cache[i].content) > 0) {
            error = serializeCache(skeleton + g_cache[i].content, &cache.content);
        }
        if (error == tgs::TGSERROR_OK) {
            cache.mime = g_cache[i].mime;
            cache.cache = g_cache[i].cache;
            cache.function = g_cache[i].function;
            _cache[g_cache[i].path] = cache;
        }
        else {
            break;
        }
    }
    if (error == tgs::TGSERROR_OK) {
        for (i = 0; i < lengthof(g_method); ++i) {
            _method[g_method[i].name] = g_method[i].method;
        }
        _database = database;
    }
    if (error != tgs::TGSERROR_OK) {
        close();
    }
    return error;
}

/*public */void ASDServerOperation::close(void)
{
    _method.clear();
    _cache.clear();
    return;
}

/*public */void ASDServerOperation::replyRoot(RequestRec const& request, ResponseRec* response)
{
    static char const* const s_shrink[] = {
        COOKIE_SHRINK_HARDWARE,
        COOKIE_SHRINK_DATABASE,
        COOKIE_SHRINK_STATUS,
        COOKIE_SHRINK_CONTROL
    };
    static boost::xpressive::sregex const s_ESS(boost::xpressive::sregex::compile("<!\\[ESS />.*?<!\\]ESS />"));
    static boost::xpressive::sregex const s_HWT(boost::xpressive::sregex::compile("<!\\[HWF />.*?<!\\]HWF />"));
    static boost::xpressive::sregex const s_HWF(boost::xpressive::sregex::compile("<!\\[HWT />.*?<!\\]HWT />"));
    static boost::xpressive::sregex const s_EHW(boost::xpressive::sregex::compile("<!\\[EHW />.*?<!\\]EHW />"));
    static boost::xpressive::sregex const s_DBT(boost::xpressive::sregex::compile("<!\\[DBF />.*?<!\\]DBF />"));
    static boost::xpressive::sregex const s_DBF(boost::xpressive::sregex::compile("<!\\[DBT />.*?<!\\]DBT />"));
    static boost::xpressive::sregex const s_EDB(boost::xpressive::sregex::compile("<!\\[EDB />.*?<!\\]EDB />"));
    static boost::xpressive::sregex const s_STT(boost::xpressive::sregex::compile("<!\\[STF />.*?<!\\]STF />"));
    static boost::xpressive::sregex const s_STF(boost::xpressive::sregex::compile("<!\\[STT />.*?<!\\]STT />"));
    static boost::xpressive::sregex const s_CNT(boost::xpressive::sregex::compile("<!\\[CNF />.*?<!\\]CNF />"));
    static boost::xpressive::sregex const s_CNF(boost::xpressive::sregex::compile("<!\\[CNT />.*?<!\\]CNT />"));
    static boost::xpressive::sregex const s_ECN(boost::xpressive::sregex::compile("<!\\[ECN />.*?<!\\]ECN />"));
    static boost::xpressive::sregex const s_tag(boost::xpressive::sregex::compile("<![^<>]+? />"));
    artsatd& daemon(artsatd::getInstance());
    insensitive::map<std::string, std::string>::const_iterator it;
    std::string session;
    std::string category;
    std::string message;
    bool shrink[lengthof(s_shrink)];
    int owner;
    int norad;
    boost::shared_ptr<ASDPluginInterface> plugin;
    tgs::TGSPhysicsDatabase database;
    tgs::TGSPhysicsDatabase::FieldRec field;
    std::string string;
    bool flag;
    int i;
    tgs::TGSError error;
    
    if ((it = request.cookie.find(COOKIE_SESSION_ID)) != request.cookie.end()) {
        session = it->second;
    }
    if (daemon.requestSession(&session, &flag) == tgs::TGSERROR_OK) {
        if (!flag) {
            if ((it = request.cookie.find(COOKIE_ERROR_CATEGORY)) != request.cookie.end()) {
                category = it->second;
            }
            if ((it = request.cookie.find(COOKIE_ERROR_MESSAGE)) != request.cookie.end()) {
                message = it->second;
            }
        }
        for (i = 0; i < lengthof(s_shrink); ++i) {
            shrink[i] = false;
            if ((it = request.cookie.find(s_shrink[i])) != request.cookie.end()) {
                if (it->second == SHRINK_HIDE) {
                    shrink[i] = true;
                }
            }
        }
        daemon.getSession(session, &owner, &flag, &string, NULL);
        norad = daemon.getNORAD();
        if (!request.query.empty()) {
            if ((it = request.query.find("session")) != request.query.end()) {
                if (it->second == "reload") {
                    bindError(CATEGORY_SESSION, tgs::TGSERROR_OK, &category, &message);
                }
                else if (it->second == "session") {
                    bindError(CATEGORY_SESSION, daemon.controlSession(session, owner <= 0, request.host), &category, &message);
                }
                else if (it->second == "exclusive") {
                    bindError(CATEGORY_SESSION, daemon.excludeSession(session, !flag), &category, &message);
                }
            }
            if ((it = request.query.find("shrink")) != request.query.end()) {
                for (i = 0; i < lengthof(s_shrink); ++i) {
                    if (it->second == s_shrink[i]) {
                        shrink[i] = !shrink[i];
                        break;
                    }
                }
            }
            if ((it = request.query.find("manual")) != request.query.end()) {
                if (it->second == "rotator") {
                    bindError(CATEGORY_HARDWARE, daemon.setManualRotator(session, !daemon.getManualRotator()), &category, &message);
                }
                else if (it->second == "transceiver") {
                    bindError(CATEGORY_HARDWARE, daemon.setManualTransceiver(session, !daemon.getManualTransceiver()), &category, &message);
                }
                else if (it->second == "tnc") {
                    bindError(CATEGORY_HARDWARE, daemon.setManualTNC(session, !daemon.getManualTNC()), &category, &message);
                }
            }
            if ((it = request.query.find("norad")) != request.query.end()) {
                bindError(CATEGORY_DATABASE, daemon.setNORAD(session, it->second), &category, &message);
            }
            if ((it = request.query.find("mode")) != request.query.end()) {
                bindError(CATEGORY_CONTROL, daemon.setMode(session, it->second), &category, &message);
            }
            if ((plugin = daemon.getPlugin(norad)) != NULL) {
                plugin->execute(session, request.query, &category, &message);
            }
            replyStatus(Server::response::found, response);
            response->header["Location"] = "/";
        }
        else {
            if (owner > 0) {
                boost::replace_first(response->content, "<!PM />", "green");
                if (flag) {
                    boost::replace_first(response->content, "<!EX />", "green");
                    boost::replace_first(response->content, "<!SI />", "You are controlling the ground station exclusively.");
                }
                else {
                    boost::replace_first(response->content, "<!SI />", "You are controlling the ground station.");
                }
            }
            else if (owner < 0) {
                boost::replace_first(response->content, "<!PM />", "red");
                if (flag) {
                    boost::replace_first(response->content, "<!_DP />", FORM_DISABLED);
                    boost::replace_first(response->content, "<!EX />", "red");
                    boost::replace_first(response->content, "<!SI />", string + " is controlling the ground station exclusively.");
                }
                else {
                    boost::replace_first(response->content, "<!SI />", string + " is controlling the ground station.");
                }
            }
            else {
                boost::replace_first(response->content, "<!SI />", "The ground station is on standby.");
            }
            if (category == CATEGORY_SESSION && !message.empty()) {
                boost::replace_first(response->content, "<!ES />", message);
            }
            else {
                response->content = boost::xpressive::regex_replace(response->content, s_ESS, "");
            }
            if (!shrink[0]) {
                boost::replace_first(response->content, "<!HW />", SHRINK_HIDE);
                response->content = boost::xpressive::regex_replace(response->content, s_HWT, "");
                if (daemon.getManualRotator()) {
                    boost::replace_first(response->content, "<!MR />", "red");
                }
                if (daemon.getManualTransceiver()) {
                    boost::replace_first(response->content, "<!MT />", "red");
                }
                if (daemon.getManualTNC()) {
                    boost::replace_first(response->content, "<!MM />", "red");
                }
                if (category == CATEGORY_HARDWARE && !message.empty()) {
                    boost::replace_first(response->content, "<!EH />", message);
                }
                else {
                    response->content = boost::xpressive::regex_replace(response->content, s_EHW, "");
                }
            }
            else {
                boost::replace_first(response->content, "<!HW />", SHRINK_SHOW);
                response->content = boost::xpressive::regex_replace(response->content, s_HWF, "");
            }
            boost::replace_first(response->content, "<!HR />", (artsatd::getRotator()->isValid()) ? (DEVICE_OK) : (DEVICE_NG));
            boost::replace_first(response->content, "<!HT />", (artsatd::getTransceiver()->isValid()) ? (DEVICE_OK) : (DEVICE_NG));
            boost::replace_first(response->content, "<!HM />", (artsatd::getTNC()->isValid()) ? (DEVICE_OK) : (DEVICE_NG));
            boost::replace_first(response->content, "<!ND />", stringizeNORAD(norad));
            if (category == CATEGORY_DATABASE && !message.empty()) {
                boost::replace_first(response->content, "<!ED />", message);
            }
            else {
                response->content = boost::xpressive::regex_replace(response->content, s_EDB, "");
            }
            string = daemon.getMode();
            if (!shrink[1]) {
                boost::replace_first(response->content, "<!DB />", SHRINK_HIDE);
                response->content = boost::xpressive::regex_replace(response->content, s_DBT, "");
                field.norad = -1;
                field.beacon.frequency = -1;
                field.beacon.drift = INT_MIN;
                field.sender.frequency = -1;
                field.sender.drift = INT_MIN;
                field.receiver.frequency = -1;
                field.receiver.drift = INT_MIN;
                memset(&field.tle, 0, sizeof(field.tle));
                if ((error = database.open(_database)) == tgs::TGSERROR_OK) {
                    if (database.getField(norad, &field) == tgs::TGSERROR_OK) {
                        boost::replace_first(response->content, "<!NM />", colorizeSpan("green", field.name));
                    }
                    else {
                        boost::replace_first(response->content, "<!NM />", "Unknown NORAD");
                    }
                    database.close();
                }
                else {
                    boost::replace_first(response->content, "<!NM />", colorizeSpan("red", "Database Error " + error.print()));
                }
                boost::replace_first(response->content, "<!CS />", stringizeCallsign(field.callsign));
                boost::replace_first(response->content, "<!BM />", stringizeMode(field.beacon.mode));
                boost::replace_first(response->content, "<!BF />", colorizeSpan((string == "CW_TEST") ? ("green") : (""), stringizeFrequency(field.beacon.frequency)));
                boost::replace_first(response->content, "<!BD />", stringizeDrift(field.beacon.drift));
                boost::replace_first(response->content, "<!SM />", stringizeMode(field.sender.mode));
                boost::replace_first(response->content, "<!SF />", colorizeSpan((string == "FM_TEST") ? ("green") : (""), stringizeFrequency(field.sender.frequency)));
                boost::replace_first(response->content, "<!SD />", stringizeDrift(field.sender.drift));
                boost::replace_first(response->content, "<!RM />", stringizeMode(field.receiver.mode));
                boost::replace_first(response->content, "<!RF />", colorizeSpan((string == "FM_TEST") ? ("green") : (""), stringizeFrequency(field.receiver.frequency)));
                boost::replace_first(response->content, "<!RD />", stringizeDrift(field.receiver.drift));
                boost::replace_first(response->content, "<!TU />", (field.norad >= 0) ? (stringizeTime(field.time + ir::IRXTimeDiff::localTimeOffset())) : (DEFAULT_TIME));
                boost::replace_first(response->content, "<!TL />", stringizeTLE(field.tle));
            }
            else {
                boost::replace_first(response->content, "<!DB />", SHRINK_SHOW);
                response->content = boost::xpressive::regex_replace(response->content, s_DBF, "");
            }
            if (!shrink[2]) {
                boost::replace_first(response->content, "<!ST />", SHRINK_HIDE);
                response->content = boost::xpressive::regex_replace(response->content, s_STT, "");
            }
            else {
                boost::replace_first(response->content, "<!ST />", SHRINK_SHOW);
                response->content = boost::xpressive::regex_replace(response->content, s_STF, "");
            }
            boost::replace_first(response->content, "<!" + ((!string.empty()) ? (string) : ("NO")) + " />", "green");
            if (category == CATEGORY_CONTROL && !message.empty()) {
                boost::replace_first(response->content, "<!EC />", message);
            }
            else {
                response->content = boost::xpressive::regex_replace(response->content, s_ECN, "");
            }
            if (!shrink[3]) {
                boost::replace_first(response->content, "<!CN />", SHRINK_HIDE);
                response->content = boost::xpressive::regex_replace(response->content, s_CNT, "");
                string.clear();
                if ((plugin = daemon.getPlugin(norad)) != NULL) {
                    plugin->process(session, category, message, &string);
                }
                boost::replace_first(response->content, "<!CD />", string);
            }
            else {
                boost::replace_first(response->content, "<!CN />", SHRINK_SHOW);
                response->content = boost::xpressive::regex_replace(response->content, s_CNF, "");
            }
            if (owner <= 0) {
                boost::replace_all(response->content, "<!_DF />", FORM_DISABLED);
                boost::replace_all(response->content, "<!_RO />", FORM_READONLY);
            }
            response->content = boost::xpressive::regex_replace(response->content, s_tag, "");
        }
        setCookie(COOKIE_SESSION_ID, session, COOKIE_MAXAGE, response);
        setCookie(COOKIE_ERROR_CATEGORY, category, COOKIE_MAXAGE, response);
        setCookie(COOKIE_ERROR_MESSAGE, message, COOKIE_MAXAGE, response);
        for (i = 0; i < lengthof(s_shrink); ++i) {
            setCookie(s_shrink[i], (!shrink[i]) ? (SHRINK_SHOW) : (SHRINK_HIDE), COOKIE_MAXAGE, response);
        }
    }
    else {
        replyStatus(Server::response::service_unavailable, response);
    }
    return;
}

/*public */void ASDServerOperation::replyHardware(RequestRec const& request, ResponseRec* response)
{
    artsatd& daemon(artsatd::getInstance());
    insensitive::map<std::string, std::string>::const_iterator it;
    std::string session;
    int owner;
    bool exclusive;
    int online;
    std::string string;
    ASDDeviceRotator::DataRec rotator;
    ASDDeviceTransceiver::DataRec transceiver;
    double x;
    double y;
    double z;
    
    if ((it = request.cookie.find(COOKIE_SESSION_ID)) != request.cookie.end()) {
        session = it->second;
    }
    daemon.getObserverCallsign(&string);
    boost::replace_first(response->content, "<!CS />", stringizeCallsign(string));
    daemon.getObserverPosition(&x, &y, &z);
    boost::replace_first(response->content, "<!LT />", stringizeLatitude(x));
    boost::replace_first(response->content, "<!LN />", stringizeLongitude(y));
    boost::replace_first(response->content, "<!AT />", stringizeAltitude(z));
    rotator = artsatd::getRotator().getData();
    boost::replace_first(response->content, "<!AZ />", colorizeSpan("cyan", stringizeAzimuth(rotator.azimuth)));
    boost::replace_first(response->content, "<!EL />", colorizeSpan("magenta", stringizeElevation(rotator.elevation)));
    transceiver = artsatd::getTransceiver().getData();
    boost::replace_first(response->content, "<!SF />", stringizeFrequency(transceiver.frequencySender));
    boost::replace_first(response->content, "<!RF />", stringizeFrequency(transceiver.frequencyReceiver));
    daemon.getSession(session, &owner, &exclusive, &session, &online);
    session = stringizeSession(session);
    if (owner > 0) {
        session = colorizeSpan("green", session);
    }
    else if (exclusive) {
        session = colorizeSpan("red", session);
    }
    boost::replace_first(response->content, "<!SS />", session);
    boost::replace_first(response->content, "<!OS />", stringizeOnline(online));
    return;
}

/*public */void ASDServerOperation::replyOrbital(RequestRec const& request, ResponseRec* response)
{
    artsatd& daemon(artsatd::getInstance());
    std::string string;
    ir::IRXTime time;
    ir::IRXTime aos;
    ir::IRXTime los;
    double x;
    double y;
    double z;
    
    daemon.getSatellitePosition(&x, &y, &z);
    boost::replace_first(response->content, "<!LT />", stringizeLatitude(x));
    boost::replace_first(response->content, "<!LN />", stringizeLongitude(y));
    boost::replace_first(response->content, "<!AT />", stringizeAltitude(z));
    daemon.getSatelliteDirection(&x, &y);
    boost::replace_first(response->content, "<!AZ />", colorizeSpan("cyan", stringizeAzimuth(x)));
    boost::replace_first(response->content, "<!EL />", colorizeSpan("magenta", stringizeElevation(y)));
    string = daemon.getMode();
    daemon.getSatelliteFrequency(&x, &y, &z);
    boost::replace_first(response->content, "<!BF />", colorizeSpan((string == "CW") ? ("green") : (""), stringizeFrequency(x)));
    boost::replace_first(response->content, "<!SF />", colorizeSpan((string == "FM") ? ("green") : (""), stringizeFrequency(y)));
    boost::replace_first(response->content, "<!RF />", colorizeSpan((string == "FM") ? ("green") : (""), stringizeFrequency(z)));
    daemon.getSatelliteDopplerShift(&x, &y);
    boost::replace_first(response->content, "<!SD />", stringizeDopplerShift(x));
    boost::replace_first(response->content, "<!RD />", stringizeDopplerShift(y));
    time = daemon.getTime();
    boost::replace_first(response->content, "<!TM />", stringizeTime(time));
    daemon.getSatelliteAOSLOS(&aos, &los);
    if (aos.asTime_t() > 0 && los.asTime_t() > 0) {
        if (aos <= time && time <= los) {
            boost::replace_first(response->content, "<!DA />", colorizeSpan("red", "<!DA />"));
            boost::replace_first(response->content, "<!DL />", colorizeSpan("green", "<!DL />"));
        }
    }
    if (aos.asTime_t() > 0) {
        boost::replace_first(response->content, "<!AS />", stringizeTime(aos));
        boost::replace_first(response->content, "<!DA />", stringizeTimeDiff(aos - time));
    }
    else {
        boost::replace_first(response->content, "<!AS />", DEFAULT_TIME);
        boost::replace_first(response->content, "<!DA />", DEFAULT_TIMEDIFF);
    }
    if (los.asTime_t() > 0) {
        boost::replace_first(response->content, "<!LS />", stringizeTime(los));
        boost::replace_first(response->content, "<!DL />", stringizeTimeDiff(los - time));
    }
    else {
        boost::replace_first(response->content, "<!LS />", DEFAULT_TIME);
        boost::replace_first(response->content, "<!DL />", DEFAULT_TIMEDIFF);
    }
    daemon.getSatelliteMEL(&y);
    boost::replace_first(response->content, "<!ML />", colorizeSpan("magenta", stringizeElevation(y)));
    daemon.getRotatorStart(&aos);
    if (aos.asTime_t() > 0) {
        boost::replace_first(response->content, "<!RS />", stringizeTime(aos));
        boost::replace_first(response->content, "<!DR />", stringizeTimeDiff(aos - time));
    }
    else {
        boost::replace_first(response->content, "<!RS />", DEFAULT_TIME);
        boost::replace_first(response->content, "<!DR />", DEFAULT_TIMEDIFF);
    }
    return;
}

/*public */void ASDServerOperation::replyJSONRPC(RequestRec const& request, ResponseRec* response)
{
    rapidjson::Document req;
    rapidjson::Document res;
    rapidjson::Value result_obj;
    int req_id = -1;
    int error_code = 0;
    std::string error_msg;
    
    req.Parse<0>(request.content.c_str());
    res.SetObject();
    res.AddMember("jsonrpc", "2.0", res.GetAllocator());
    if (req.HasParseError()) {
        error_code = -32700;
        error_msg = "Parse error";
        response->status = 400;
    }
    else {
        if (req["id"].IsInt()) {
            req_id = req["id"].GetInt();
            if (req_id >= 0 && req["method"].IsString()) {
                std::string method = req["method"].GetString();
                if (_method.find(method) != _method.end()) {
                    ASDServerRPC::Params args, result;
                    if (req.HasMember("params") && req["params"].IsObject()) {
                        ASDServerRPC::Variant variant = ASDServerRPC::toVariant(req["params"]);
                        args = boost::get<ASDServerRPC::Params>(variant);
                    }
                    ASDServerRPC::Result rpc_result = _method[method](args, &result);
                    if (rpc_result == ASDServerRPC::RPC_OK) {
                        response->status = 200;
                    }
                    else if (rpc_result == ASDServerRPC::RPC_WRONG_ARGS) {
                        error_code = -32602;
                        error_msg = "Invalid params";
                        response->status = 400;
                    }
                    else {
                        error_code = -32603;
                        error_msg = "Internal error";
                        response->status = 500;
                    }
                    result_obj.SetObject();
                    ASDServerRPC::toJson(result, &result_obj, res.GetAllocator());
                }
                else {
                    error_code = -32601;
                    error_msg = "Method not found";
                    response->status = 400;
                }
            }
            else {
                error_code = -32600;
                error_msg = "Invalid Request";
                response->status = 400;
            }
        }
        else {
            error_code = -32600;
            error_msg = "Invalid Request";
            response->status = 400;
        }
    }
    if (req_id == -1) {
        rapidjson::Value null;
        null.SetNull();
        res.AddMember("id", null, res.GetAllocator());
    }
    else {
        res.AddMember("id", req_id, res.GetAllocator());
    }
    if (error_code == 0) {
        res.AddMember("result", result_obj, res.GetAllocator());
    }
    else {
        rapidjson::Value res_error;
        res_error.SetObject();
        res_error.AddMember("code", error_code, res.GetAllocator());
        res_error.AddMember("message", error_msg.c_str(), res.GetAllocator());
        if (result_obj.IsObject() && result_obj.MemberBegin() != result_obj.MemberEnd()) {
            res_error.AddMember("data", result_obj, res.GetAllocator());
        }
        res.AddMember("error", res_error, res.GetAllocator());
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    res.Accept(writer);
    response->content += buffer.GetString();
    return;
}

/*private virtual */tgs::TGSError ASDServerOperation::onRequest(RequestRec const& request, ResponseRec* response)
{
    insensitive::map<std::string, CacheRec>::const_iterator it;
    
    if ((it = _cache.find(request.path)) != _cache.end()) {
        if (!it->second.mime.empty()) {
            response->header["Content-Type"] = it->second.mime;
        }
        if (!it->second.cache.empty()) {
            response->header["Cache-Control"] = it->second.cache;
        }
        response->content = it->second.content;
        if (it->second.function != NULL) {
            (this->*it->second.function)(request, response);
        }
    }
    else {
        replyStatus(Server::response::not_found, response);
    }
    return tgs::TGSERROR_OK;
}

/*private static */tgs::TGSError ASDServerOperation::serializeCache(std::string const& file, std::string* result)
{
    std::ifstream stream;
    std::string line;
    std::string string;
    tgs::TGSError error(tgs::TGSERROR_OK);
    
    stream.open(file.c_str());
    if (stream.is_open()) {
        while (std::getline(stream, line)) {
            boost::trim(line);
            string += line;
        }
        *result = string;
        stream.close();
    }
    else {
        error = tgs::TGSERROR_FILE_NOTFOUND;
    }
    return error;
}

/*private static */void ASDServerOperation::bindError(std::string const& name, tgs::TGSError error, std::string* category, std::string* message)
{
    *category = name;
    if (error == tgs::TGSERROR_OK) {
        message->clear();
    }
    else {
        *message = error.print();
    }
    return;
}

/*private static */std::string ASDServerOperation::colorizeSpan(std::string const& color, std::string const& string)
{
    return (!color.empty()) ? ("<span class='" + color + "'>" + string + "</span>") : (string);
}

/*private static */std::string ASDServerOperation::stringizeLatitude(double param)
{
    std::string result("-       -");
    
    if (!std::isnan(param)) {
        result = (boost::format("%s %7.3lf") % ((param >= 0.0) ? ("N") : ("S")) % std::abs(param)).str();
    }
    return result + "&deg;";
}

/*private static */std::string ASDServerOperation::stringizeLongitude(double param)
{
    std::string result("-       -");
    
    if (!std::isnan(param)) {
        if (param < -180.0) {
            param += 360.0;
        }
        else if (param >= 180.0) {
            param -= 360.0;
        }
        result = (boost::format("%s %7.3lf") % ((param >= 0.0) ? ("E") : ("W")) % std::abs(param)).str();
    }
    return result + "&deg;";
}

/*private static */std::string ASDServerOperation::stringizeAltitude(double param)
{
    std::string result("      -");
    
    if (!std::isnan(param)) {
        result = (boost::format("%7.3lf") % param).str();
    }
    return result + " km";
}

/*private static */std::string ASDServerOperation::stringizeAzimuth(int param)
{
    std::string result("  -");
    
    if (param >= 0) {
        result = (boost::format("%3d") % param).str();
    }
    return result + "&deg;";
}

/*private static */std::string ASDServerOperation::stringizeAzimuth(double param)
{
    std::string result("      -");
    
    if (!std::isnan(param)) {
        result = (boost::format("%7.3lf") % param).str();
    }
    return result + "&deg;";
}

/*private static */std::string ASDServerOperation::stringizeElevation(int param)
{
    std::string result(" -");
    
    if (param >= 0) {
        result = (boost::format("%2d") % param).str();
    }
    return result + "&deg;";
}

/*private static */std::string ASDServerOperation::stringizeElevation(double param)
{
    std::string result("      -");
    
    if (!std::isnan(param)) {
        result = (boost::format("%+7.3lf") % param).str();
    }
    return result + "&deg;";
}

/*private static */std::string ASDServerOperation::stringizeNORAD(int param)
{
    std::string result;
    
    if (param >= 0) {
        result = (boost::format("%05d") % param).str();
    }
    return result;
}

/*private static */std::string ASDServerOperation::stringizeCallsign(std::string const& param)
{
    return (!param.empty()) ? (param) : ("-");
}

/*private static */std::string ASDServerOperation::stringizeMode(std::string const& param)
{
    return (!param.empty()) ? (param) : ("-");
}

/*private static */std::string ASDServerOperation::stringizeFrequency(int param)
{
    std::string result("         -");
    
    if (param >= 0) {
        result = (boost::format("%10.6lf") % (param / 1000000.0)).str();
    }
    return result + " MHz";
}

/*private static */std::string ASDServerOperation::stringizeFrequency(double param)
{
    std::string result("         -");
    
    if (!std::isnan(param)) {
        result = (boost::format("%10.6lf") % (param / 1000000.0)).str();
    }
    return result + " MHz";
}

/*private static */std::string ASDServerOperation::stringizeDrift(int param)
{
    std::string result("         -");
    
    if (param != INT_MIN) {
        result = (boost::format("%+10d") % param).str();
    }
    return result + " Hz";
}

/*private static */std::string ASDServerOperation::stringizeDopplerShift(double param)
{
    std::string result("         -");
    
    if (!std::isnan(param)) {
        result = (boost::format("%10.6lf") % (param * 100.0)).str();
    }
    return result + " %";
}

/*private static */std::string ASDServerOperation::stringizeTLE(tgs::TLERec const& param)
{
    std::string result;
    
    result  = (strlen(param.one) > 0) ? (param.one) : ("1                                                                    ");
    result += "<br />";
    result += (strlen(param.two) > 0) ? (param.two) : ("2                                                                    ");
    return result;
}

/*private static */std::string ASDServerOperation::stringizeTime(ir::IRXTime const& param)
{
    return param.format("%YYYY/%MM/%DD %hh:%mm:%ss JST");
}

/*private static */std::string ASDServerOperation::stringizeTimeDiff(ir::IRXTimeDiff const& param)
{
    long hour;
    int minute;
    int second;
    std::string result;
    
    param.get(&hour, &minute, &second);
    result  = (param >= ir::IRXTimeDiff(0)) ? (" ") : ("-");
    result += (boost::format("%02d:%02d:%02d") % std::abs(hour) % std::abs(minute) % std::abs(second)).str();
    return "&Delta;" + result;
}

/*private static */std::string ASDServerOperation::stringizeSession(std::string const& param)
{
    std::vector<std::string> fragment;
    std::string result("---.---.---.---");
    
    boost::split(fragment, param, boost::is_any_of("."));
    if (fragment.size() == 4) {
        result = (boost::format("%3s.%3s.%3s.%3s") % fragment[0] % fragment[1] % fragment[2] % fragment[3]).str();
    }
    return result;
}

/*private static */std::string ASDServerOperation::stringizeOnline(int param)
{
    return (param >= 0) ? ((boost::format("%d") % param).str()) : ("-");
}