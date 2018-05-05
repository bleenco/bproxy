import { writeFile } from 'fs';
import { mkdir } from 'temp';
import * as request from 'request';

export function writeConfig(path: string, config: any): Promise<void> {
  return new Promise((resolve, reject) => {
    writeFile(path, JSON.stringify(config, null, 2), { encoding: 'utf8' }, (err: NodeJS.ErrnoException) => {
      if (err) {
        reject(err);
      } else {
        resolve();
      }
    });
  });
}

export function tempDir(): Promise<string> {
  return new Promise((resolve, reject) => {
    const rand = Math.random().toString(36).substr(2, 9);
    mkdir(rand, (err: any, dirPath: string) => {
      if (err) {
        reject(err);
      } else {
        resolve(dirPath);
      }
    });
  });
}

export function sendRequest(url: string, opts: any = {}): Promise<request.Response> {
  return new Promise((resolve, reject) => {
    const options = Object.assign({}, { url }, opts);
    request.get(options, function(err, res, body) {
      if (err) {
        reject(err);
      } else {
        resolve(res);
      }
    });
  });
}
