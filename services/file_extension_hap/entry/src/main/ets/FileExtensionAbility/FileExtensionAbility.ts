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
// @ts-nocheck
import Extension from '@ohos.application.FileAccessExtensionAbility';
import fs from '@ohos.file.fs';
import { init, delVolumeInfo, getVolumeInfoList, path2uri } from './VolumeManager';
import { onReceiveEvent } from './Subcriber';
import fileExtensionInfo from '@ohos.file.fileExtensionInfo';
import hilog from '@ohos.hilog';
import process from '@ohos.process';
import baseUri from '@ohos.uri';

const deviceFlag = fileExtensionInfo.DeviceFlag;
const documentFlag = fileExtensionInfo.DocumentFlag;
const deviceType = fileExtensionInfo.DeviceType;
const BUNDLE_NAME = 'com.ohos.UserFile.ExternalFileManager';
const DEFAULT_MODE = 0;
const URI_SCHEME = 'datashare://';
const FILE_SCHEME_NAME = 'file';
const FILE_PREFIX_NAME = 'file://';
const DOMAIN_CODE = 0x0001;
const TAG = 'ExternalFileManager';
const ERR_OK = 0;
const ERR_ERROR = -1;
const E_EXIST = 13900015;
const E_NOEXIST = 13900002;
const E_URIS = 14300002;
const E_GETRESULT = 14300004;

export default class FileExtAbility extends Extension {
  onCreate(want): void {
    init();
    onReceiveEvent(function (data) {
      if (data.event === 'usual.event.data.VOLUME_MOUNTED') {
        process.exit(0);
      } else {
        delVolumeInfo(data.parameters.id);
      }
    });
  }

  checkUri(uri): boolean {
    try {
      let uriTmp = new baseUri.URI(uri);
      if (uriTmp.scheme === FILE_SCHEME_NAME) {
        uri = uri.replace(FILE_PREFIX_NAME, '/');
        return true;
      } else if (uri.indexOf(URI_SCHEME) === 0) {
        uri = uri.replace(URI_SCHEME, '');
        return /^\/([^\/]+\/?)+$/.test(uri);
      } else {
        hilog.error(DOMAIN_CODE, TAG, 'checkUri error, uri is ' + uri);
        return false;
      }
    } catch (error) {
      hilog.error(DOMAIN_CODE, TAG, 'checkUri error, uri is ' + uri);
      return false;
    }
  }

  getPath(uri): string {
    let sep = '://';
    let arr = uri.split(sep);
    let minLength = 2;
    if (arr.length < minLength) {
      return uri;
    }
    let path = uri.replace(arr[0] + sep, '');
    if (arr[1].indexOf('/') > 0) {
      path = path.replace(arr[1].split('/')[0], '');
    }
    path = path.replace('/' + BUNDLE_NAME, '');
    if (path.charAt(path.length - 1) === '/') {
      path = path.substr(0, path.length - 1);
    }
    return path;
  }

  genNewFileUri(uri, displayName) {
    let newFileUri = uri;
    if (uri.charAt(uri.length - 1) === '/') {
      newFileUri += displayName;
    } else {
      newFileUri += '/' + displayName;
    }
    return newFileUri;
  }

  getFileName(uri): string {
    let arr = uri.split('/');
    let name = arr.pop();
    if (name === '') {
      name = arr.pop();
    }
    return name;
  }

  renameUri(uri, displayName): string {
    let arr = uri.split('/');
    let newFileUri = '';
    if (arr.pop() === '') {
      arr.pop();
      arr.push(displayName);
      arr.push('');
    } else {
      arr.push(displayName);
    }
    for (let index = 0; index < arr.length; index++) {
      if (arr[index] === '') {
        newFileUri += '/';
      } else if (index === arr.length - 1) {
        newFileUri += arr[index];
      } else {
        newFileUri += arr[index] + '/';
      }
    }
    return newFileUri;
  }

  recurseDir(path, cb): void {
    try {
      let stat = fs.statSync(path);
      if (stat.isDirectory()) {
        let fileName = fs.listFileSync(path);
        for (let fileLen = 0; fileLen < fileName.length; fileLen++) {
          stat = fs.statSync(path + '/' + fileName[fileLen]);
          if (stat.isDirectory()) {
            this.recurseDir(path + '/' + fileName[fileLen], cb);
          } else {
            cb(path + '/' + fileName[fileLen], false);
          }
        }
      } else {
        cb(path, false);
      }
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'recurseDir error ' + e.message);
      cb(path, true);
    }
  }

  isCrossDeviceLink(sourceFileUri, targetParentUri): boolean {
    let roots = this.getRoots().roots;
    for (let index = 0; index < roots.length; index++) {
      let uri = roots[index].uri;
      if (sourceFileUri.indexOf(uri) === 0 && targetParentUri.indexOf(uri) === 0) {
        return false;
      }
    }
    return true;
  }

  openFile(sourceFileUri, flags): {number, number} {
    if (!this.checkUri(sourceFileUri)) {
      return {
        fd: ERR_ERROR,
        code: E_URIS,
      };
    }
    try {
      let path = this.getPath(sourceFileUri);
      let file = fs.openSync(path, flags);
      return {
        fd: file.fd,
        code: ERR_OK,
      };
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'openFile error ' + e.message);
      return {
        fd: ERR_ERROR,
        code: e.code,
      };
    }
  }

  createFile(parentUri, displayName): {string, number} {
    if (!this.checkUri(parentUri)) {
      return {
        uri: '',
        code: E_URIS,
      };
    }
    try {
      let newFileUri = this.genNewFileUri(parentUri, displayName);
      if (this.access(newFileUri).isExist) {
        return {
          uri: '',
          code: E_EXIST,
        };
      }
      let path = this.getPath(newFileUri);
      let file = fs.openSync(path, fs.OpenMode.CREATE);
      fs.closeSync(file);
      return {
        uri: newFileUri,
        code: ERR_OK,
      };
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'createFile error ' + e.message);
      return {
        uri: '',
        code: e.code,
      };
    }
  }

  mkdir(parentUri, displayName): {string, number} {
    if (!this.checkUri(parentUri)) {
      return {
        uri: '',
        code: E_URIS,
      };
    }
    try {
      let newFileUri = this.genNewFileUri(parentUri, displayName);
      let path = this.getPath(newFileUri);
      fs.mkdirSync(path);
      return {
        uri: newFileUri,
        code: ERR_OK,
      };
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'mkdir error ' + e.message);
      return {
        uri: '',
        code: e.code,
      };
    }
  }

  delete(selectFileUri): number {
    if (!this.checkUri(selectFileUri)) {
      return E_URIS;
    }
    let path = this.getPath(selectFileUri);
    let code = ERR_OK;
    try {
      let stat = fs.statSync(path);
      if (stat.isDirectory()) {
        fs.rmdirSync(path);
      } else {
        fs.unlinkSync(path);
      }
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'deleteFile error ' + e.message);
      return e.code;
    }
    return code;
  }

  async movedir(oldPath, newPath, mode): boolean {
    try {
      // The default mode of the fs.moveDir interface is 0
      await fs.moveDir(oldPath, newPath, mode);
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'movedir error ' + e.message, 'movedir error code' + e.code);
      return false;
    }
    return true;
  }

  move(sourceFileUri, targetParentUri): {string, number} {
    if (!this.checkUri(sourceFileUri) || !this.checkUri(targetParentUri)) {
      return {
        uri: '',
        code: E_URIS,
      };
    }
    let displayName = this.getFileName(sourceFileUri);
    let newFileUri = this.genNewFileUri(targetParentUri, displayName);
    let oldPath = this.getPath(sourceFileUri);
    let newPath = this.getPath(newFileUri);
    if (oldPath === newPath) {
      // move to the same directory
      return {
        uri: newFileUri,
        code: ERR_OK,
      };
    } else if (newPath.indexOf(oldPath) === 0) {
      // move to a subdirectory of the source directory
      return {
        uri: '',
        code: E_GETRESULT,
      };
    }
    try {
      // The source file does not exist or the destination is not a directory
      let isAccess = fs.accessSync(oldPath);
      if (!isAccess) {
        return {
          uri: '',
          code: E_GETRESULT,
        };
      }
      let stat = fs.statSync(this.getPath(targetParentUri));
      if (!stat || !stat.isDirectory()) {
        return {
          uri: '',
          code: E_GETRESULT,
        };
      }
      // If not across devices, use fs.renameSync to move
      if (!this.isCrossDeviceLink(sourceFileUri, targetParentUri)) {
        fs.renameSync(oldPath, newPath);
        return {
          uri: newFileUri,
          code: ERR_OK,
        };
      }
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'move error ' + e.message);
      return {
        uri: '',
        code: e.code,
      };
    }
    let result = this.movedir(srcPath, destPath, DEFAULT_MODE);
    if (result) {
      return {
        uri: newFileUri,
        code: ERR_OK,
      };
    } else {
      return {
        uri: '',
        code: E_GETRESULT,
      };
    }
  }

  rename(sourceFileUri, displayName): {string, number} {
    if (!this.checkUri(sourceFileUri)) {
      return {
        uri: '',
        code: E_URIS,
      };
    }
    try {
      let newFileUri = this.renameUri(sourceFileUri, displayName);
      let oldPath = this.getPath(sourceFileUri);
      let newPath = this.getPath(newFileUri);
      fs.renameSync(oldPath, newPath);
      return {
        uri: newFileUri,
        code: ERR_OK,
      };
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'rename error ' + e.message);
      return {
        uri: '',
        code: e.code,
      };
    }
  }

  access(sourceFileUri): {boolean, number} {
    if (!this.checkUri(sourceFileUri)) {
      hilog.error(DOMAIN_CODE, TAG, 'access checkUri fail');
      return {
        isExist: false,
        code: E_URIS,
      };
    }
    let isAccess = false;
    try {
      let path = this.getPath(sourceFileUri);
      isAccess = fs.accessSync(path);
      if (!isAccess) {
        return {
          isExist: false,
          code: ERR_OK,
        };
      }
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'access error ' + e.message);
      return {
        isExist: false,
        code: e.code,
      };
    }
    return {
      isExist: true,
      code: ERR_OK,
    };
  }

  listFile(sourceFileUri, offset, count, filter) {
    if (!this.checkUri(sourceFileUri)) {
      return {
        infos: [],
        code: E_URIS,
      };
    }
    let infos = [];
    let path = this.getPath(sourceFileUri);
    let statPath = fs.statSync(path);
    if (!statPath.isDirectory()) {
      return {
        infos: [],
        code: E_GETRESULT,
      };
    }

    try {
      let fileName = fs.listFileSync(path);
      for (let i = 0; i < fileName.length; i++) {
        if (offset > i) {
          continue;
        }
        if (i === (offset + count)) {
          break;
        }
        let mode = documentFlag.SUPPORTS_READ | documentFlag.SUPPORTS_WRITE;
        let stat = fs.statSync(path + '/' + fileName[i]);
        if (stat.isDirectory()) {
          mode |= documentFlag.REPRESENTS_DIR;
        } else {
          mode |= documentFlag.REPRESENTS_FILE;
        }
        infos.push({
          uri: this.genNewFileUri(sourceFileUri, fileName[i]),
          fileName: fileName[i],
          mode: mode,
          size: stat.size,
          mtime: stat.mtime,
          mimeType: '',
        });
      }
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'listFile error ' + e.message);
      return {
        infos: [],
        code: E_GETRESULT,
      };
    }
    return {
      infos: infos,
      code: ERR_OK,
    };
  }

  getFileInfoFromUri(selectFileUri) {
    if (!this.checkUri(selectFileUri)) {
      return {
        fileInfo: {},
        code: E_URIS,
      };
    }
    let fileInfo = {};
    try {
      let path = this.getPath(selectFileUri);
      let fileName = this.getFileName(path);
      let stat = fs.statSync(path);
      let mode = documentFlag.SUPPORTS_READ | documentFlag.SUPPORTS_WRITE;
      if (stat.isDirectory()) {
        mode |= documentFlag.REPRESENTS_DIR;
      } else {
        mode |= documentFlag.REPRESENTS_FILE;
      }
      fileInfo = {
        uri: selectFileUri,
        fileName: fileName,
        mode: mode,
        size: stat.size,
        mtime: stat.mtime,
        mimeType: '',
      };
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'getFileInfoFromUri error ' + e.message);
      return {
        fileInfo: {},
        code: e.code,
      };
    }
    return {
      fileInfo: fileInfo,
      code: ERR_OK,
    };
  }

  getRoots() {
    let roots = [
      {
        uri: 'file://com.ohos.UserFile.ExternalFileManager/data/storage/el1/bundle/storage_daemon',
        displayName: 'shared_disk',
        deviceType: deviceType.DEVICE_SHARED_DISK,
        deviceFlags: deviceFlag.SUPPORTS_READ | deviceFlag.SUPPORTS_WRITE,
      },
    ];
    roots = roots.concat(getVolumeInfoList());
    return {
      roots: roots,
      code: ERR_OK,
    };
  }

  getNormalResult(dirPath, column, queryResults): void {
    if (column === 'display_name') {
      let index = dirPath.lastIndexOf('/');
      let target = dirPath.substring(index + 1, );
      queryResults.push(String(target));
    } else if (column === 'relative_path') {
      let index = dirPath.lastIndexOf('/');
      let target = dirPath.substring(0, index + 1);
      queryResults.push(target);
    } else {
      queryResults.push('');
    }
  }

  getResultFromStat(dirPath, column, stat, queryResults): void {
    if (column === 'size' && stat.isDirectory()) {
      let size = 0;
      this.recurseDir(dirPath, function (filePath, isDirectory) {
        if (!isDirectory) {
          let fileStat = fs.statSync(filePath);
          size += fileStat.size;
        }
      });
      queryResults.push(String(size));
    } else {
      queryResults.push(String(stat[column]));
    }
  }

  query(uri, columns): {[], number} {
    if (!this.checkUri(uri)) {
      return {
        results: [],
        code: E_URIS,
      };
    }

    if (!this.access(uri).isExist) {
      return {
        results: [],
        code: E_NOEXIST,
      };
    }

    let queryResults = [];
    try {
      let dirPath = this.getPath(uri);
      let stat = fs.statSync(dirPath);
      for (let index in columns) {
        let column = columns[index];
        if (column in stat) {
          this.getResultFromStat(dirPath, column, stat, queryResults);
        } else {
          this.getNormalResult(dirPath, column, queryResults);
        }
      }
    } catch (e) {
      hilog.error(DOMAIN_CODE, TAG, 'query error ' + e.message);
      return {
        results: [],
        code: E_GETRESULT,
      };
    }
    return {
      results: queryResults,
      code: ERR_OK,
    };
  }
};