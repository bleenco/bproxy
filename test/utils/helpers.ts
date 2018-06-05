import { writeFile, createReadStream} from 'fs';
import { mkdir } from 'temp';
import * as request from 'request';
import { createHash } from 'crypto';


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

export function getFileHash(filePath: string): Promise<string> {
  return new Promise((resolve, reject) => {
    //var fs = require('fs');
    //var crypto = require('crypto');

    // the file you want to get the hash    
    var fd = createReadStream(filePath);
    var hash = createHash('sha1');
    hash.setEncoding('hex');

    fd.on('end', function() {
        hash.end();
        resolve(hash.read().toString());
    });

    // read all file and pipe it (write it) to the hash object
    fd.pipe(hash);
  });
}