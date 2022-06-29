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

#ifndef FILE_EXT_STUB_IMPL_H
#define FILE_EXT_STUB_IMPL_H

#include <memory>

#include "file_access_ext_ability.h"
#include "file_access_ext_stub.h"
#include "file_access_extension_info.h"
#include "native_engine/native_value.h"

namespace OHOS {
namespace FileAccessFwk {
class FileAccessExtStubImpl : public FileAccessExtStub {
public:
    explicit FileAccessExtStubImpl(const std::shared_ptr<FileAccessExtAbility>& extension, napi_env env)
        : extension_(extension) {}

    virtual ~FileAccessExtStubImpl() {}

    int OpenFile(const Uri &uri, int flags) override;
    int CreateFile(const Uri &parent, const std::string &displayName,  Uri &newFile) override;
    int Mkdir(const Uri &parent, const std::string &displayName, Uri &newFile) override;
    int Delete(const Uri &sourceFile) override;
    int Move(const Uri &sourceFile, const Uri &targetParent, Uri &newFile) override;
    int Rename(const Uri &sourceFile, const std::string &displayName, Uri &newFile) override;
    std::vector<FileInfo> ListFile(const Uri &sourceFileUri) override;
    std::vector<DeviceInfo> GetRoots() override;

private:
    std::shared_ptr<FileAccessExtAbility> GetOwner();
    std::shared_ptr<FileAccessExtAbility> extension_;
};
} // namespace FileAccessFwk
} // namespace OHOS
#endif // FILE_EXT_STUB_IMPL_H