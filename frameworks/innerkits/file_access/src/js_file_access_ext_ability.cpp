/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "js_file_access_ext_ability.h"

#include "ability_info.h"
#include "accesstoken_kit.h"
#include "extension_context.h"
#include "file_access_ext_stub_impl.h"
#include "file_access_framework_errno.h"
#include "hilog_wrapper.h"
#include "hitrace_meter.h"
#include "ipc_skeleton.h"
#include "js_runtime.h"
#include "js_runtime_utils.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_common_fileaccess.h"
#include "napi_common_util.h"
#include "napi_common_want.h"
#include "napi_remote_object.h"

namespace OHOS {
namespace FileAccessFwk {
namespace {
    constexpr size_t ARGC_ZERO = 0;
    constexpr size_t ARGC_ONE = 1;
    constexpr size_t ARGC_TWO = 2;
    constexpr size_t MAX_ARG_COUNT = 5;
}

using namespace OHOS::AppExecFwk;
using namespace OHOS::AbilityRuntime;
using OHOS::Security::AccessToken::AccessTokenKit;

JsFileAccessExtAbility* JsFileAccessExtAbility::Create(const std::unique_ptr<Runtime> &runtime)
{
    return new JsFileAccessExtAbility(static_cast<JsRuntime&>(*runtime));
}

JsFileAccessExtAbility::JsFileAccessExtAbility(JsRuntime &jsRuntime) : jsRuntime_(jsRuntime) {}
JsFileAccessExtAbility::~JsFileAccessExtAbility() = default;

void JsFileAccessExtAbility::Init(const std::shared_ptr<AbilityLocalRecord> &record,
    const std::shared_ptr<OHOSApplication> &application, std::shared_ptr<AbilityHandler> &handler,
    const sptr<IRemoteObject> &token)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "Init");
    FileAccessExtAbility::Init(record, application, handler, token);
    std::string srcPath = "";
    GetSrcPath(srcPath);
    if (srcPath.empty()) {
        HILOG_ERROR("Failed to get srcPath");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return;
    }

    std::string moduleName(Extension::abilityInfo_->moduleName);
    moduleName.append("::").append(abilityInfo_->name);
    HandleScope handleScope(jsRuntime_);

    jsObj_ = jsRuntime_.LoadModule(moduleName, srcPath);
    if (jsObj_ == nullptr) {
        HILOG_ERROR("Failed to get jsObj_");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return;
    }

    NativeObject* obj = ConvertNativeValueTo<NativeObject>(jsObj_->Get());
    if (obj == nullptr) {
        HILOG_ERROR("Failed to get JsFileAccessExtAbility object");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
}

void JsFileAccessExtAbility::OnStart(const AAFwk::Want &want)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "OnStart");
    Extension::OnStart(want);
    HandleScope handleScope(jsRuntime_);
    napi_env env = reinterpret_cast<napi_env>(&jsRuntime_.GetNativeEngine());
    napi_value napiWant = OHOS::AppExecFwk::WrapWant(env, want);
    NativeValue* nativeWant = reinterpret_cast<NativeValue*>(napiWant);
    NativeValue* argv[] = {nativeWant};
    CallObjectMethod("onCreate", argv, ARGC_ONE);
    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
}

sptr<IRemoteObject> JsFileAccessExtAbility::OnConnect(const AAFwk::Want &want)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "OnConnect");
    Extension::OnConnect(want);
    sptr<FileAccessExtStubImpl> remoteObject = new (std::nothrow) FileAccessExtStubImpl(
        std::static_pointer_cast<JsFileAccessExtAbility>(shared_from_this()),
        reinterpret_cast<napi_env>(&jsRuntime_.GetNativeEngine()));
    if (remoteObject == nullptr) {
        HILOG_ERROR("No memory allocated for FileExtStubImpl");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return nullptr;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return remoteObject->AsObject();
}

NativeValue* JsFileAccessExtAbility::CallObjectMethod(const char* name, NativeValue* const* argv, size_t argc)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CallObjectMethod");
    if (!jsObj_) {
        HILOG_ERROR("JsFileAccessExtAbility::CallObjectMethod jsObj Not found FileAccessExtAbility.js");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return nullptr;
    }

    HandleScope handleScope(jsRuntime_);
    auto& nativeEngine = jsRuntime_.GetNativeEngine();

    NativeValue* value = jsObj_->Get();
    if (value == nullptr) {
        HILOG_ERROR("Failed to get FileAccessExtAbility value");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return nullptr;
    }

    NativeObject* obj = ConvertNativeValueTo<NativeObject>(value);
    if (obj == nullptr) {
        HILOG_ERROR("Failed to get FileAccessExtAbility object");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return nullptr;
    }

    NativeValue* method = obj->GetProperty(name);
    if (method == nullptr) {
        HILOG_ERROR("Failed to get '%{public}s' from FileAccessExtAbility object", name);
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return nullptr;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return handleScope.Escape(nativeEngine.CallFunction(value, method, argv, argc));
}

static bool DoCallJsMethod(CallJsParam *param)
{
    JsRuntime &jsRuntime = param->jsRuntime;
    HandleScope handleScope(jsRuntime);
    napi_env env = reinterpret_cast<napi_env>(&jsRuntime.GetNativeEngine());
    size_t argc = 0;
    NativeValue* argv[MAX_ARG_COUNT] = { nullptr };
    if (param->argParser != nullptr) {
        if (!param->argParser(env, argv, argc)) {
            param->errorCode = ERR_INVALID_PARAM;
            param->errorMessage = "failed to get params.";
            return false;
        }
    }
    NativeValue* value = param->jsObj->Get();
    if (value == nullptr) {
        param->errorCode = ERR_ERROR;
        param->errorMessage = "failed to get native value object.";
        return false;
    }
    NativeObject* obj = ConvertNativeValueTo<NativeObject>(value);
    if (obj == nullptr) {
        param->errorCode = ERR_ERROR;
        param->errorMessage = "failed to get FileExtAbility object.";
        return false;
    }
    NativeValue* method = obj->GetProperty(param->funcName.c_str());
    if (method == nullptr) {
        param->errorCode = ERR_ERROR;
        param->errorMessage = string("failed to get ").append(param->funcName).append(" from FileExtAbility object.");
        return false;
    }
    if (param->retParser == nullptr) {
        param->errorCode = ERR_INVALID_PARAM;
        param->errorMessage = "ResultValueParser must not null.";
        return false;
    }
    auto& nativeEngine = jsRuntime.GetNativeEngine();
    auto ret = param->retParser(env, handleScope.Escape(nativeEngine.CallFunction(value, method, argv, argc)));
    return ret;
}

std::tuple<int, std::string> JsFileAccessExtAbility::CallJsMethod(const std::string &funcName, JsRuntime& jsRuntime,
    NativeReference *jsObj, InputArgsParser argParser, ResultValueParser retParser)
{
    uv_loop_s *loop = nullptr;
    napi_status status = napi_get_uv_event_loop(reinterpret_cast<napi_env>(&jsRuntime.GetNativeEngine()), &loop);
    if (status != napi_ok) {
        return { ERR_ERROR, "failed to get uv event loop." };
    }
    auto param = new (std::nothrow) CallJsParam(funcName, jsRuntime, jsObj, argParser, retParser);
    if (param == nullptr) {
        return { ERR_ERROR, "failed to new param." };
    }
    auto work = new (std::nothrow) uv_work_t();
    if (work == nullptr) {
        delete param;
        return { ERR_ERROR, "failed to new uv_work_t." };
    }
    work->data = reinterpret_cast<void *>(param);
    int ret = uv_queue_work(loop, work, [](uv_work_t *work) {}, [](uv_work_t *work, int status) {
        CallJsParam *param = reinterpret_cast<CallJsParam *>(work->data);
        do {
            if (param == nullptr) {
                HILOG_ERROR("failed to get CallJsParam.");
                break;
            }
            if (!DoCallJsMethod(param)) {
                HILOG_ERROR("failed to call DoAsnycWork.");
            }
        } while (false);
        std::unique_lock<std::mutex> lock(param->fileOperateMutex);
        param->isReady = true;
        param->fileOperateCondition.notify_one();
    });
    std::tuple<int, std::string> retMsg = { ERR_OK, "no error." };
    do {
        if (ret != 0) {
            retMsg = { ERR_ERROR, "failed to exec uv_queue_work." };
            break;
        }
        std::unique_lock<std::mutex> lock(param->fileOperateMutex);
        param->fileOperateCondition.wait(lock, [param]() { return param->isReady; });
        retMsg = { param->errorCode, param->errorMessage };
    } while (false);
    if (work->data != nullptr) {
        delete reinterpret_cast<CallJsParam *>(work->data);
    }
    if (work != nullptr) {
        delete work;
    }
    return retMsg;
}

void JsFileAccessExtAbility::GetSrcPath(std::string &srcPath)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "GetSrcPath");
    if (!Extension::abilityInfo_->isStageBasedModel) {
        /* temporary compatibility api8 + config.json */
        srcPath.append(Extension::abilityInfo_->package);
        srcPath.append("/assets/js/");
        if (!Extension::abilityInfo_->srcPath.empty()) {
            srcPath.append(Extension::abilityInfo_->srcPath);
        }
        srcPath.append("/").append(Extension::abilityInfo_->name).append(".abc");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return;
    }

    if (!Extension::abilityInfo_->srcEntrance.empty()) {
        srcPath.append(Extension::abilityInfo_->moduleName + "/");
        srcPath.append(Extension::abilityInfo_->srcEntrance);
        srcPath.erase(srcPath.rfind('.'));
        srcPath.append(".abc");
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
}

int JsFileAccessExtAbility::OpenFile(const Uri &uri, int flags)
{
    auto fd = std::make_shared<int>();
    auto argParser = [uri, flags](napi_env &env, NativeValue* argv[], size_t &argc) -> bool {
        napi_value napiUri = nullptr;
        napi_status status = napi_create_string_utf8(env, uri.ToString().c_str(), NAPI_AUTO_LENGTH, &napiUri);
        if (status != napi_ok) {
            HILOG_ERROR("create uri fail.");
            return false;
        }
        napi_value napiFlags = nullptr;
        status = napi_create_int32(env, flags, &napiFlags);
        if (status != napi_ok) {
            HILOG_ERROR("create flags fail.");
            return false;
        }
        NativeValue* nativeUri = reinterpret_cast<NativeValue*>(napiUri);
        NativeValue* nativeFlags = reinterpret_cast<NativeValue*>(napiFlags);
        argv[ARGC_ZERO] = nativeUri;
        argv[ARGC_ONE] = nativeFlags;
        argc = ARGC_TWO;
        return true;
    };
    auto retParser = [fd](napi_env &env, NativeValue *result) -> bool {
        *fd = UnwrapInt32FromJS(env, reinterpret_cast<napi_value>(result));
        return true;
    };

    int errCode = ERR_OK;
    std::string errMsg = "";
    std::tie(errCode, errMsg) = CallJsMethod("openFile", jsRuntime_, jsObj_.get(), argParser, retParser);
    if (errCode != ERR_OK) {
        HILOG_ERROR("error code:%{public}d, error message:%{public}s", errCode, errMsg.c_str());
        return errCode;
    }
    return *fd;
}

int JsFileAccessExtAbility::CreateFile(const Uri &parent, const std::string &displayName, Uri &newFile)
{
    auto uri = std::make_shared<std::string>();
    auto argParser = [parent, displayName](napi_env &env, NativeValue* argv[], size_t &argc) -> bool {
        napi_value napiParent = nullptr;
        napi_status status = napi_create_string_utf8(env, parent.ToString().c_str(), NAPI_AUTO_LENGTH, &napiParent);
        if (status != napi_ok) {
            HILOG_ERROR("create parent uri fail.");
            return false;
        }
        napi_value napiDisplayName = nullptr;
        status = napi_create_string_utf8(env, displayName.c_str(), NAPI_AUTO_LENGTH, &napiDisplayName);
        if (status != napi_ok) {
            HILOG_ERROR("create displayName fail.");
            return false;
        }
        NativeValue* nativeParent = reinterpret_cast<NativeValue*>(napiParent);
        NativeValue* nativeDisplayName = reinterpret_cast<NativeValue*>(napiDisplayName);
        argv[ARGC_ZERO] = nativeParent;
        argv[ARGC_ONE] = nativeDisplayName;
        argc = ARGC_TWO;
        return true;
    };
    auto retParser = [uri](napi_env &env, NativeValue *result) -> bool {
        *uri = UnwrapStringFromJS(env, reinterpret_cast<napi_value>(result));
        return true;
    };

    int errCode = ERR_OK;
    std::string errMsg = "";
    std::tie(errCode, errMsg) = CallJsMethod("createFile", jsRuntime_, jsObj_.get(), argParser, retParser);
    if (errCode != ERR_OK) {
        HILOG_ERROR("error code:%{public}d, error message:%{public}s", errCode, errMsg.c_str());
    }
    newFile = Uri(*uri);
    return errCode;
}

int JsFileAccessExtAbility::Mkdir(const Uri &parent, const std::string &displayName, Uri &newFile)
{
    auto uri = std::make_shared<std::string>();
    auto argParser = [parent, displayName](napi_env &env, NativeValue* argv[], size_t &argc) -> bool {
        napi_value napiParent = nullptr;
        napi_status status = napi_create_string_utf8(env, parent.ToString().c_str(), NAPI_AUTO_LENGTH, &napiParent);
        if (status != napi_ok) {
            HILOG_ERROR("create parent uri fail.");
            return false;
        }
        napi_value napiDisplayName = nullptr;
        status = napi_create_string_utf8(env, displayName.c_str(), NAPI_AUTO_LENGTH, &napiDisplayName);
        if (status != napi_ok) {
            HILOG_ERROR("create displayName fail.");
            return false;
        }
        NativeValue* nativeParent = reinterpret_cast<NativeValue*>(napiParent);
        NativeValue* nativeDisplayName = reinterpret_cast<NativeValue*>(napiDisplayName);
        argv[ARGC_ZERO] = nativeParent;
        argv[ARGC_ONE] = nativeDisplayName;
        argc = ARGC_TWO;
        return true;
    };
    auto retParser = [uri](napi_env &env, NativeValue *result) -> bool {
        *uri = UnwrapStringFromJS(env, reinterpret_cast<napi_value>(result));
        return true;
    };

    int errCode = ERR_OK;
    std::string errMsg = "";
    std::tie(errCode, errMsg) = CallJsMethod("mkdir", jsRuntime_, jsObj_.get(), argParser, retParser);
    if (errCode != ERR_OK) {
        HILOG_ERROR("error code:%{public}d, error message:%{public}s", errCode, errMsg.c_str());
    }
    if ((*uri).empty()) {
        HILOG_ERROR("call Mkdir with return empty.");
        return ERR_ERROR;
    }
    newFile = Uri(*uri);

    return errCode;
}

int JsFileAccessExtAbility::Delete(const Uri &sourceFile)
{
    auto ret = std::make_shared<int>();
    auto argParser = [sourceFile](napi_env &env, NativeValue* argv[], size_t &argc) -> bool {
        napi_value napiUri = nullptr;
        napi_status status = napi_create_string_utf8(env, sourceFile.ToString().c_str(), NAPI_AUTO_LENGTH, &napiUri);
        if (status != napi_ok) {
            HILOG_ERROR("create parent uri fail.");
            return false;
        }
        NativeValue* nativeUri = reinterpret_cast<NativeValue*>(napiUri);
        argv[ARGC_ZERO] = nativeUri;
        argc = ARGC_ONE;
        return true;
    };
    auto retParser = [ret](napi_env &env, NativeValue *result) -> bool {
        *ret = UnwrapInt32FromJS(env, reinterpret_cast<napi_value>(result));
        HILOG_INFO("delete ret code:%{public}d", *ret);
        return true;
    };

    int errCode = ERR_OK;
    std::string errMsg = "";
    std::tie(errCode, errMsg) = CallJsMethod("delete", jsRuntime_, jsObj_.get(), argParser, retParser);
    if (errCode != ERR_OK) {
        HILOG_ERROR("error code:%{public}d, error message:%{public}s", errCode, errMsg.c_str());
        return errCode;
    }

    return *ret;
}

int JsFileAccessExtAbility::Move(const Uri &sourceFile, const Uri &targetParent, Uri &newFile)
{
    auto uri = std::make_shared<std::string>();
    auto argParser = [sourceFile, targetParent](napi_env &env, NativeValue* argv[], size_t &argc) -> bool {
        napi_value napiSourceFile = nullptr;
        napi_status status = napi_create_string_utf8(env, sourceFile.ToString().c_str(),
            NAPI_AUTO_LENGTH, &napiSourceFile);
        if (status != napi_ok) {
            HILOG_ERROR("create sourceFile uri fail.");
            return false;
        }
        napi_value napiTargetParent = nullptr;
        status = napi_create_string_utf8(env, targetParent.ToString().c_str(), NAPI_AUTO_LENGTH, &napiTargetParent);
        if (status != napi_ok) {
            HILOG_ERROR("create targetParent uri fail.");
            return false;
        }
        NativeValue* nativeSourceFile = reinterpret_cast<NativeValue*>(napiSourceFile);
        NativeValue* nativeTargetParent = reinterpret_cast<NativeValue*>(napiTargetParent);
        argv[ARGC_ZERO] = nativeSourceFile;
        argv[ARGC_ONE] = nativeTargetParent;
        argc = ARGC_TWO;
        return true;
    };
    auto retParser = [uri](napi_env &env, NativeValue *result) -> bool {
        *uri = UnwrapStringFromJS(env, reinterpret_cast<napi_value>(result));
        return true;
    };

    int errCode = ERR_OK;
    std::string errMsg = "";
    std::tie(errCode, errMsg) = CallJsMethod("move", jsRuntime_, jsObj_.get(), argParser, retParser);
    if (errCode != ERR_OK) {
        HILOG_ERROR("error code:%{public}d, error message:%{public}s", errCode, errMsg.c_str());
    }
    newFile = Uri(*uri);
    return errCode;
}

int JsFileAccessExtAbility::Rename(const Uri &sourceFile, const std::string &displayName, Uri &newFile)
{
    auto uri = std::make_shared<std::string>();
    auto argParser = [sourceFile, displayName](napi_env &env, NativeValue* argv[], size_t &argc) -> bool {
        napi_value napiSourceFile = nullptr;
        napi_status status = napi_create_string_utf8(env, sourceFile.ToString().c_str(),
            NAPI_AUTO_LENGTH, &napiSourceFile);
        if (status != napi_ok) {
            HILOG_ERROR("create sourceFile uri fail.");
            return false;
        }
        napi_value napiDisplayName = nullptr;
        status = napi_create_string_utf8(env, displayName.c_str(), NAPI_AUTO_LENGTH, &napiDisplayName);
        if (status != napi_ok) {
            HILOG_ERROR("create displayName fail.");
            return false;
        }
        NativeValue* nativeSourceFile = reinterpret_cast<NativeValue*>(napiSourceFile);
        NativeValue* nativeDisplayName = reinterpret_cast<NativeValue*>(napiDisplayName);
        argv[ARGC_ZERO] = nativeSourceFile;
        argv[ARGC_ONE] = nativeDisplayName;
        argc = ARGC_TWO;
        return true;
    };
    auto retParser = [uri](napi_env &env, NativeValue *result) -> bool {
        *uri = UnwrapStringFromJS(env, reinterpret_cast<napi_value>(result));
        return true;
    };

    int errCode = ERR_OK;
    std::string errMsg = "";
    std::tie(errCode, errMsg) = CallJsMethod("rename", jsRuntime_, jsObj_.get(), argParser, retParser);
    if (errCode != ERR_OK) {
        HILOG_ERROR("error code:%{public}d, error message:%{public}s", errCode, errMsg.c_str());
    }
    newFile = Uri(*uri);
    return errCode;
}

std::vector<FileInfo> JsFileAccessExtAbility::ListFile(const Uri &sourceFile)
{
    auto fileVec = std::make_shared<std::vector<FileInfo>>();
    auto argParser = [sourceFile](napi_env &env, NativeValue* argv[], size_t &argc) -> bool {
        napi_value napiUri = nullptr;
        napi_status status = napi_create_string_utf8(env, sourceFile.ToString().c_str(), NAPI_AUTO_LENGTH, &napiUri);
        if (status != napi_ok) {
            HILOG_ERROR("create sourceFile uri fail.");
            return false;
        }
        NativeValue* nativeUri = reinterpret_cast<NativeValue*>(napiUri);
        argv[ARGC_ZERO] = nativeUri;
        argc = ARGC_ONE;
        return true;
    };
    auto retParser = [fileVec](napi_env &env, NativeValue *result) -> bool {
        return UnwrapArrayFileInfoFromJS(env, reinterpret_cast<napi_value>(result), *fileVec);
    };
    int errCode = ERR_OK;
    std::string errMsg = "";
    std::tie(errCode, errMsg) = CallJsMethod("listFile", jsRuntime_, jsObj_.get(), argParser, retParser);
    if (errCode != ERR_OK) {
        HILOG_ERROR("error code:%{public}d, error message:%{public}s", errCode, errMsg.c_str());
    }
    return std::move(*fileVec);
}

std::vector<DeviceInfo> JsFileAccessExtAbility::GetRoots()
{
    auto devVec = std::make_shared<std::vector<DeviceInfo>>();
    auto argParser = [](napi_env &env, NativeValue *argv[], size_t &argc) -> bool {
        argc = ARGC_ZERO;
        return true;
    };
    auto retParser = [devVec](napi_env &env, NativeValue *result) -> bool {
        return UnwrapArrayDeviceInfoFromJS(env, reinterpret_cast<napi_value>(result), *devVec);
    };
    int errCode = ERR_OK;
    std::string errMsg = "";
    std::tie(errCode, errMsg) = CallJsMethod("getRoots", jsRuntime_, jsObj_.get(), argParser, retParser);
    if (errCode != ERR_OK) {
        HILOG_ERROR("error code:%{public}d, error message:%{public}s", errCode, errMsg.c_str());
    }
    return std::move(*devVec);
}
} // namespace FileAccessFwk
} // namespace OHOS