import { writeFile, createReadStream } from 'fs';
import { mkdir } from 'temp';
import * as request from 'request';
import { createHash } from 'crypto';
import { resolve } from 'url';


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
    request.get(options, function (err, res, body) {
      if (err) {
        reject(err);
      } else {
        resolve(res);
      }
    });
  });
}

export function getFileHash(filePath: string): Promise<string> {
  return new Promise((resolve, reject) => {
    // the file you want to get the hash    
    var fd = createReadStream(filePath);
    var hash = createHash('sha1');
    hash.setEncoding('hex');

    fd.on('end', function () {
      hash.end();
      resolve(hash.read().toString());
    });

    // read all file and pipe it (write it) to the hash object
    fd.pipe(hash);
  });
}

export function compareFiles(filePath1: string, filePath2: string): Promise<{same : boolean} > {
  let hash1 : string;
  let hash2 : string;

  return new Promise((resolve, reject) => {
    getFileHash(filePath1)
      .then((h) => {
        return new Promise((resolve, reject) => {
          hash1 = h;
          resolve();
        });
      })
      .then(() => getFileHash(filePath2))
      .then((h) => {
        return new Promise((resolve, reject) => {
          hash2 = h;
          resolve();
        });
      })
      .then(() => {
        return new Promise((resolve, reject) => {
          if (hash1 === hash2) {
            resolve();
          } else {
            reject(new Error("Uploaded file is not same!"));
          }
        });
      }).then(() => {
        resolve();
      }).catch((err) => {
        reject(err);
      });
  });
}