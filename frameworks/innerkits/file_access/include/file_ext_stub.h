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

#ifndef FILE_EXT_STUB_H
#define FILE_EXT_STUB_H

#include <iremote_stub.h>
#include <map>

#include "file_extension_info.h"
#include "ifile_ext_base.h"

namespace OHOS {
namespace FileAccessFwk {
class FileExtStub : public IRemoteStub<IFileExtBase> {
public:
    FileExtStub();
    ~FileExtStub();
    int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;
private:
    ErrCode CmdOpenFile(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdCreateFile(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdMkdir(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdDelete(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdMove(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdRename(MessageParcel &data, MessageParcel &reply);

    ErrCode CmdListFile(MessageParcel &data, MessageParcel &reply);
    ErrCode CmdGetRoots(MessageParcel &data, MessageParcel &reply);

    bool CheckCallingPermission(const std::string &permission);

    using RequestFuncType = int (FileExtStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, RequestFuncType> stubFuncMap_;
};
} // namespace FileAccessFwk
} // namespace OHOS
#endif // FILE_EXT_STUB_H
