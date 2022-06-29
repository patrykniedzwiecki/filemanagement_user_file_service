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
import volumeManager from '@ohos.volumeManager'
import fileExtensionInfo from "@ohos.fileExtensionInfo"
if (!globalThis.volumeInfoList) {
    globalThis.volumeInfoList = [];
}

const FLAG = fileExtensionInfo.FLAG;

function init() {
    volumeManager.getAllVolumes().then((volumes) => {
        let flags = FLAG.SUPPORTS_WRITE | FLAG.SUPPORTS_DELETE | FLAG.SUPPORTS_RENAME | FLAG.SUPPORTS_COPY |
            FLAG.SUPPORTS_MOVE | FLAG.SUPPORTS_REMOVE | FLAG.DIR_SUPPORTS_CREATE | FLAG.DIR_PREFERS_LAST_MODIFIED;
        for (let i = 0; i < volumes.length; i++) {
            let volume = volumes[i];
            let volumeInfo = {
                'volumeId': volume.id,
                'fsUuid': volume.uuid,
                'path': volume.path,
                'uri': path2uri('', volume.path),
                'displayName': volume.id,
                'deviceId': '',
                'flags': flags,
                'type': 'SD'
            }
            globalThis.volumeInfoList.push(volumeInfo);
        }
    });
}

function addVolumeInfo(volumeInfo) {
    globalThis.volumeInfoList.push(volumeInfo);
}

function path2uri(id, path) {
    return `fileAccess://${id}${path}`;
}

function delVolumeInfo(volumeId) {
    globalThis.volumeInfoList = globalThis.volumeInfoList.filter((volume) => volume.volumeId !== volumeId);
}

function getVolumeInfoList() {
    return globalThis.volumeInfoList;
}

export { init, addVolumeInfo, delVolumeInfo, getVolumeInfoList, path2uri }