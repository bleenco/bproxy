import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';
import { bproxy, runNode, killAll } from '../utils/process';
import { writeConfig, tempDir, sendRequest, getFileHash, compareFiles } from '../utils/helpers';
import * as path from 'path';
import * as request from 'request';
import { exec } from 'child_process';
import * as fs from 'fs';
import { resolve } from 'url';
import { platform } from 'os';

chai.use(chaiAsPromised);
chai.use(require('chai-http'));

const expect = chai.expect;
const cwd = process.cwd();
let configPath = null;
let config = {
  "port": 8080,
  "secure_port": 8081,
  "gzip_mime_types": ["text/css", "application/javascript", "application/x-javascript"],
  "proxies": [
    {
      "hosts": ["localhost"],
      "ip": "127.0.0.1",
      "certificate_path": "test/certs/localhost.crt",
      "key_path": "test/certs/localhost.key",
      "port": 4900
    }
  ]
};

describe('POST data', () => {
  before(() => {
    return Promise.resolve()
      .then(() => {
        return new Promise((resolve, reject) => {
          const command = `npm install --prefix=${path.resolve(__dirname, '../files/bproxy-testing-website/')}`;
          exec(command, (error, stdout, stderr) => {
            if (error) {
              reject(error);
            } else {
              resolve();
            }
          });
        });
      })
      .then(() => {
        return new Promise((resolve, reject) => {
          const command = `npm run --prefix=${path.resolve(__dirname, '../files/bproxy-testing-website/')} build:prod`;
          exec(command, (error, stdout, stderr) => {
            if (error) {
              reject(error);
            } else {
              resolve();
            }
          });
        });
      })
      .then(() => {
        return new Promise((resolve, reject) => {
          const dirPath = path.resolve(__dirname, '../files/randfiles/');
          if(!fs.existsSync(dirPath)){
            fs.mkdirSync(dirPath);
          }
          resolve();
        });
      })
      .then(() => {
        return new Promise((resolve, reject) => {
          const mb = platform() === 'darwin' ? 'm' : 'M';
          const command = `dd if=/dev/urandom bs=1${mb} count=1 of=${path.resolve(__dirname, '../files/randfiles/1M.bin')}`;
          exec(command, (error, stdout, stderr) => {
            if (error) {
              reject(error);
            } else {
              resolve();
            }
          });
        });
      })
      .then(() => {
        return new Promise((resolve, reject) => {
          const mb = platform() === 'darwin' ? 'm' : 'M';
          const command = `dd if=/dev/urandom bs=100${mb} count=1 of=${path.resolve(__dirname, '../files/randfiles/100M.bin')}`;
          exec(command, (error, stdout, stderr) => {
            if (error) {
              reject(error);
            } else {
              resolve();
            }
          });
        });
      });
  });

  beforeEach(() => {
    return Promise.resolve()
      .then(() => process.chdir(path.resolve(cwd, 'test/files')))
      .then(() => runNode(false, ['./bproxy-testing-website/dist-app/api/index.js']))
      .then(() => process.chdir(cwd));
  });

  afterEach(() => killAll());

  it(`should return POST data in JSON form (https://localhost:8081/simple-form)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;
          const formData = {
            hello: 'world',
            how: 'are you',
            great: 'is this via bproxy?',
            yes: 'it is'
          };
          request.post('https://localhost:8081/simple-form', { gzip: true, strictSSL: false, form: formData }, function (error, response, body) {
            if (error) {
              reject(error);
            }
            expect(response.body).equal(JSON.stringify(formData));
            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      });
  });

  it(`should upload 1MB file (https://localhost:8081/upload)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          const filepath = path.resolve(__dirname, '../files/randfiles/1M.bin');
          var req = request.post('https://localhost:8081/upload', { strictSSL: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }
            const savedFile = JSON.parse(response.body);
            expect(savedFile[0].size).equal(1048576)
            const savedPath = savedFile[0].path;

            resolve({filepath, savedPath});
          });
          var form = req.form();
          form.append('file', fs.createReadStream(filepath));
        });
      })
      .then(({filepath, savedPath}) => {
        return compareFiles(filepath, savedPath);
      });
  });


  it(`should upload 100MB file (https://localhost:8081/upload)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          const filepath = path.resolve(__dirname, '../files/randfiles/100M.bin');
          var req = request.post('https://localhost:8081/upload', { strictSSL: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }
            const savedFile = JSON.parse(response.body);
            expect(savedFile[0].size).equal(33554431)
            const savedPath = savedFile[0].path;

            resolve({filepath, savedPath});
          });
          var form = req.form();
          form.append('file', fs.createReadStream(filepath));
        });
      })
      .then(({filepath, savedPath}) => {
        return compareFiles(filepath, savedPath);
      });
  });
});
