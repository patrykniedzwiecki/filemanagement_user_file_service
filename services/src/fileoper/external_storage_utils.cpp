/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "external_storage_utils.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <securec.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include "file_manager_service_def.h"
#include "file_manager_service_errno.h"
#include "log.h"
#include "storage_manager_inf.h"
using namespace std;
using namespace OHOS::StorageManager;
namespace OHOS {
namespace FileManagerService {
static bool GetPathFromUri(const string &uri, string &path)
{
    uint len = EXTERNAL_STORAGE_URI.size();
    string head = uri.substr(0, len);
    if (head != EXTERNAL_STORAGE_URI) {
        ERR_LOG("invalid format uri %{public}s, head check fail", uri.c_str());
        return false;
    }
    if (uri.size() == len) {
        ERR_LOG("uri content is invalid %{public}s", uri.c_str());
        return false;
    }
    path = uri.substr(len);
    return true;
}

static bool GetRealPath(string &path)
{
    char filePath[PATH_MAX + 1] = { 0 };
    if (realpath(path.c_str(), filePath) == nullptr) {
        ERR_LOG("untrustPath invalid %{public}d %{public}s \n", errno, strerror(errno));
        return false;
    }
    path = string(filePath);
    return true;
}

static bool GetFileInfo(const std::string &path, const std::string &name, unique_ptr<FileInfo> &fileInfo)
{
    std::string fullPath(path);
    size_t len = fullPath.size();
    if (fullPath.at(len - 1) != '/') {
        fullPath.append("/").append(name);
    } else {
        fullPath.append(name);
    }
    struct stat st;
    if (lstat(fullPath.c_str(), &st) != 0) {
        ERR_LOG("check file info fail.");
        return false;
    }
    std::string uri(EXTERNAL_STORAGE_URI);
    std::string fName(name.c_str());

    uri.append(fullPath);
    fileInfo->SetPath(uri);
    std::string type = S_ISDIR(st.st_mode) ? "album" : "file";
    fileInfo->SetType(type);
    fileInfo->SetName(fName);
    fileInfo->SetSize(st.st_size);
    fileInfo->SetAddedTime(static_cast<long>(st.st_ctim.tv_sec));
    fileInfo->SetModifiedTime(static_cast<long>(st.st_mtim.tv_sec));
    return true;
}

static bool ConvertUriToAbsolutePath(const std::string &uri, std::string &path)
{
    if (!GetPathFromUri(uri, path)) {
        ERR_LOG("GetPathFromUri fail");
        return false;
    }
    if (!GetRealPath(path)) {
        ERR_LOG("get real path fail");
        return false;
    }
    if (!StorageManagerInf::StoragePathValidCheck(path)) {
        ERR_LOG("external uri path was ejected");
        return false;
    }
    return true;
}

int ExternalStorageUtils::DoListFile(const std::string &type, const std::string &uri, MessageParcel &reply)
{
    std::string path;
    int fileCount = 0;
    if (!ConvertUriToAbsolutePath(uri, path)) {
        ERR_LOG("invalid uri[%{public}s].", uri.c_str());
        reply.WriteInt32(fileCount);
        return E_NOEXIST;
    }

    DIR *dir = opendir(path.c_str());
    if (!dir) {
        ERR_LOG("opendir path[%{public}s] fail.", path.c_str());
        reply.WriteInt32(fileCount);
        return E_NOEXIST;
    }
    std::vector<unique_ptr<FileInfo>> fileList;
    for (struct dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        unique_ptr<FileInfo> fileInfo = make_unique<FileInfo>();
        if (!GetFileInfo(path, ent->d_name, fileInfo)) {
            continue;
        }
        fileList.push_back(move(fileInfo));
    }
    closedir(dir);
    fileCount = static_cast<int32_t>(fileList.size());
    reply.WriteInt32(fileCount);
    if (fileCount == 0) {
        return E_EMPTYFOLDER;
    }
    for (int i = 0; i < fileCount; i++) {
        reply.WriteParcelable(fileList[i].get());
    }
    return SUCCESS;
}

int ExternalStorageUtils::DoCreateFile(const std::string &uri, const std::string &name, MessageParcel &reply)
{
    std::string path;
    if (!ConvertUriToAbsolutePath(uri, path)) {
        ERR_LOG("invalid uri[%{public}s].", uri.c_str());
        return E_NOEXIST;
    }
    size_t len = path.size();
    if (path.at(len -1) != '/') {
        path.append("/").append(name);
    } else {
        path.append(name);
    }
    if (access(path.c_str(), F_OK) == 0) {
        ERR_LOG("target file[%{public}s] exist.", path.c_str());
        return E_CREATE_FAIL;
    }

    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0771);
    if (fd == -1) {
        ERR_LOG("create file[%{public}s] fail.", path.c_str());
        return E_CREATE_FAIL;
    }
    close(fd);

    std::string fullUri(EXTERNAL_STORAGE_URI);
    fullUri.append(path);
    reply.WriteString(fullUri);
    return SUCCESS;
}

int ExternalStorageUtils::DoGetRoot(const std::string &name, const std::string &path, MessageParcel &reply)
{
    vector<string> vecRootPath;
    if (!StorageManagerInf::GetMountedVolumes(vecRootPath)) {
        ERR_LOG("there is valid extorage storage");
        reply.WriteInt32(0);
        return FAIL;
    }
    reply.WriteInt32(vecRootPath.size());
    for (auto rootPath : vecRootPath) {
        reply.WriteString(rootPath);
    }
    return SUCCESS;
}

bool ExternalStorageUtils::PopFileInfo(FileInfo &fileInfo, MessageParcel &reply)
{
    std::string path;
    std::string type;
    std::string name;
    int64_t size = 0;
    int64_t at = 0;
    int64_t mt = 0;

    reply.ReadString(path);
    reply.ReadString(type);
    reply.ReadString(name);
    reply.ReadInt64(size);
    reply.ReadInt64(at);
    reply.ReadInt64(mt);
    fileInfo = FileInfo(name, path, type);
    fileInfo.SetSize(size);
    fileInfo.SetAddedTime(at);
    fileInfo.SetModifiedTime(mt);
    return true;
}
} // namespace FileManagerService
} // namespace OHOS