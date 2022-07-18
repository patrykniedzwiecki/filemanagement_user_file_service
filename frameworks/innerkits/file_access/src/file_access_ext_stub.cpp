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

#include "file_access_ext_stub.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "file_access_extension_info.h"
#include "file_access_framework_errno.h"
#include "hilog_wrapper.h"
#include "hitrace_meter.h"
#include "ipc_object_stub.h"
#include "ipc_skeleton.h"
#include "ipc_types.h"
#include "uri.h"

namespace OHOS {
namespace FileAccessFwk {
namespace {
    const std::string FILE_ACCESS_PERMISSION = "ohos.permission.FILE_ACCESS_MANAGER";
}
FileAccessExtStub::FileAccessExtStub()
{
    stubFuncMap_[CMD_OPEN_FILE] = &FileAccessExtStub::CmdOpenFile;
    stubFuncMap_[CMD_CREATE_FILE] = &FileAccessExtStub::CmdCreateFile;
    stubFuncMap_[CMD_MKDIR] = &FileAccessExtStub::CmdMkdir;
    stubFuncMap_[CMD_DELETE] = &FileAccessExtStub::CmdDelete;
    stubFuncMap_[CMD_MOVE] = &FileAccessExtStub::CmdMove;
    stubFuncMap_[CMD_RENAME] = &FileAccessExtStub::CmdRename;
    stubFuncMap_[CMD_LIST_FILE] = &FileAccessExtStub::CmdListFile;
    stubFuncMap_[CMD_GET_ROOTS] = &FileAccessExtStub::CmdGetRoots;
}

FileAccessExtStub::~FileAccessExtStub()
{
    stubFuncMap_.clear();
}

int FileAccessExtStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "OnRemoteRequest");
    if (!CheckCallingPermission(FILE_ACCESS_PERMISSION)) {
        HILOG_ERROR("permission error");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_PERMISSION_DENIED;
    }

    std::u16string descriptor = FileAccessExtStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_STATE;
    }

    const auto &itFunc = stubFuncMap_.find(code);
    if (itFunc != stubFuncMap_.end()) {
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return (this->*(itFunc->second))(data, reply);
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode FileAccessExtStub::CmdOpenFile(MessageParcel &data, MessageParcel &reply)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdOpenFile");
    std::shared_ptr<Uri> uri(data.ReadParcelable<Uri>());
    if (uri == nullptr) {
        HILOG_ERROR("OpenFile uri is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    int flags = data.ReadInt32();
    if (flags < 0) {
        HILOG_ERROR("OpenFile flags is invalid");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_ERROR;
    }

    int fd = OpenFile(*uri, flags);
    if (fd < 0) {
        HILOG_ERROR("OpenFile fail, fd is %{pubilc}d", fd);
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_FD;
    }

    if (!reply.WriteFileDescriptor(fd)) {
        HILOG_ERROR("OpenFile fail to WriteFileDescriptor fd");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return ERR_OK;
}

ErrCode FileAccessExtStub::CmdCreateFile(MessageParcel &data, MessageParcel &reply)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdCreateFile");
    std::shared_ptr<Uri> parent(data.ReadParcelable<Uri>());
    if (parent == nullptr) {
        HILOG_ERROR("CreateFile parent is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    std::string displayName = data.ReadString();
    if (displayName.empty()) {
        HILOG_ERROR("CreateFile displayName is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_PARAM;
    }

    std::shared_ptr<Uri> fileNew(data.ReadParcelable<Uri>());
    if (fileNew == nullptr) {
        HILOG_ERROR("CreateFile fileNew is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    int ret = CreateFile(*parent, displayName, *fileNew);
    if (ret < 0) {
        HILOG_ERROR("CreateFile fail, ret is %{pubilc}d", ret);
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ret;
    }

    if (!reply.WriteInt32(ret)) {
        HILOG_ERROR("CreateFile fail to WriteInt32 ret");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    if (!reply.WriteParcelable(&(*fileNew))) {
        HILOG_ERROR("CreateFile fail to WriteParcelable type");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return ERR_OK;
}

ErrCode FileAccessExtStub::CmdMkdir(MessageParcel &data, MessageParcel &reply)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdMkdir");
    std::shared_ptr<Uri> parent(data.ReadParcelable<Uri>());
    if (parent == nullptr) {
        HILOG_ERROR("Mkdir parent is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    std::string displayName = data.ReadString();
    if (displayName.empty()) {
        HILOG_ERROR("Mkdir mode is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_PARAM;
    }

    std::shared_ptr<Uri> fileNew(data.ReadParcelable<Uri>());
    if (fileNew == nullptr) {
        HILOG_ERROR("Mkdir fileNew is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    int ret = Mkdir(*parent, displayName, *fileNew);
    if (ret < 0) {
        HILOG_ERROR("Mkdir fail, ret is %{pubilc}d", ret);
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ret;
    }

    if (!reply.WriteInt32(ret)) {
        HILOG_ERROR("Mkdir fail to WriteInt32 ret");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    if (!reply.WriteParcelable(&(*fileNew))) {
        HILOG_ERROR("Mkdir fail to WriteParcelable type");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return ERR_OK;
}

ErrCode FileAccessExtStub::CmdDelete(MessageParcel &data, MessageParcel &reply)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdDelete");
    std::shared_ptr<Uri> uri(data.ReadParcelable<Uri>());
    if (uri == nullptr) {
        HILOG_ERROR("Delete uri is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    int ret = Delete(*uri);
    if (ret < 0) {
        HILOG_ERROR("Delete fail, ret is %{pubilc}d", ret);
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ret;
    }

    if (!reply.WriteInt32(ret)) {
        HILOG_ERROR("Delete fail to WriteFileDescriptor ret");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return ERR_OK;
}

ErrCode FileAccessExtStub::CmdMove(MessageParcel &data, MessageParcel &reply)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdMove");
    std::shared_ptr<Uri> sourceFile(data.ReadParcelable<Uri>());
    if (sourceFile == nullptr) {
        HILOG_ERROR("Move sourceFile is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    std::shared_ptr<Uri> targetParent(data.ReadParcelable<Uri>());
    if (targetParent == nullptr) {
        HILOG_ERROR("Move targetParent is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    std::shared_ptr<Uri> fileNew(data.ReadParcelable<Uri>());
    if (fileNew == nullptr) {
        HILOG_ERROR("Move fileNew is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    int ret = Move(*sourceFile, *targetParent, *fileNew);
    if (ret < 0) {
        HILOG_ERROR("Move fail, ret is %{pubilc}d", ret);
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ret;
    }

    if (!reply.WriteInt32(ret)) {
        HILOG_ERROR("Move fail to WriteInt32 ret");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    if (!reply.WriteParcelable(&(*fileNew))) {
        HILOG_ERROR("Move fail to WriteParcelable type");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return ERR_OK;
}

ErrCode FileAccessExtStub::CmdRename(MessageParcel &data, MessageParcel &reply)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdRename");
    std::shared_ptr<Uri> sourceFile(data.ReadParcelable<Uri>());
    if (sourceFile == nullptr) {
        HILOG_ERROR("Rename sourceFileUri is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    std::string displayName = data.ReadString();
    if (displayName.empty()) {
        HILOG_ERROR("Rename mode is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_PARAM;
    }

    std::shared_ptr<Uri> fileNew(data.ReadParcelable<Uri>());
    if (fileNew == nullptr) {
        HILOG_ERROR("Rename fileUriNew is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    int ret = Rename(*sourceFile, displayName, *fileNew);
    if (ret < 0) {
        HILOG_ERROR("Rename fail, ret is %{pubilc}d", ret);
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ret;
    }

    if (!reply.WriteInt32(ret)) {
        HILOG_ERROR("Rename fail to WriteInt32 ret");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    if (!reply.WriteParcelable(&(*fileNew))) {
        HILOG_ERROR("Rename fail to WriteParcelable type");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return ERR_OK;
}

ErrCode FileAccessExtStub::CmdListFile(MessageParcel &data, MessageParcel &reply)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdListFile");
    std::shared_ptr<Uri> uri(data.ReadParcelable<Uri>());
    if (uri == nullptr) {
        HILOG_ERROR("ListFile uri is nullptr");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_INVALID_URI;
    }

    std::vector<FileInfo> vec = ListFile(*uri);
    uint64_t count {vec.size()};
    if (!reply.WriteUint64(count)) {
        HILOG_ERROR("ListFile fail to WriteInt32 count");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    for (uint64_t i = 0; i < count; i++) {
        if (!reply.WriteParcelable(&vec[i])) {
            HILOG_ERROR("ListFile fail to WriteParcelable vec");
            FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
            return ERR_IPC_ERROR;
        }
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return ERR_OK;
}

ErrCode FileAccessExtStub::CmdGetRoots(MessageParcel &data, MessageParcel &reply)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdGetRoots");
    std::vector<DeviceInfo> vec = GetRoots();
    uint64_t count {vec.size()};
    if (!reply.WriteUint64(count)) {
        HILOG_ERROR("GetRoots fail to WriteInt32 count");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return ERR_IPC_ERROR;
    }

    for (uint64_t i = 0; i < count; i++) {
        if (!reply.WriteParcelable(&vec[i])) {
            HILOG_ERROR("GetRoots fail to WriteParcelable ret");
            FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
            return ERR_IPC_ERROR;
        }
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return ERR_OK;
}

bool FileAccessExtStub::CheckCallingPermission(const std::string &permission)
{
    StartTrace(HITRACE_TAG_FILEMANAGEMENT, "CmdGetRoots");
    Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    int res = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller, permission);
    if (res != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        HILOG_ERROR("FileAccessExtStub::CheckCallingPermission have no fileAccess permission");
        FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
        return false;
    }

    FinishTrace(HITRACE_TAG_FILEMANAGEMENT);
    return true;
}
} // namespace FileAccessFwk
} // namespace OHOS