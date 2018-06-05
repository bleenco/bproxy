import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';
import { bproxy, runNode, killAll } from '../utils/process';
import { writeConfig, tempDir, sendRequest, getFileHash } from '../utils/helpers';
import * as path from 'path';
import * as request from 'request';
import { exec } from 'child_process';
import * as fs from 'fs';

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
        const command = `/bin/bash ${path.resolve(__dirname, '../certs/make_certs.sh')}`;
        exec(command, (error, stdout, stderr) => {
          if (error !== null) {
            return Promise.reject(error);
          }
        });
      });
  })
  beforeEach(() => {
    return Promise.resolve()
      .then(() => process.chdir(path.resolve(cwd, 'test/files')))
      .then(() => runNode(false, ['./bproxy-testing-website/dist-app/api/index.js']))
      .then(() => process.chdir(cwd));
  })
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
          request.post('https://localhost:8081/simple-form', { gzip: true, strictSSL: false, form: formData}, function (error, response, body) {
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

  it(`should return hash from uploaded file (https://localhost:8081/upload)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          var req = request.post('https://localhost:8081/upload', {strictSSL:false}, function (error, response, body) {
            if (error) {
              reject(error);
            }
            const savedFile = JSON.parse(response.body);
            expect(savedFile[0].size).equal(1048576)
            const savedPath = savedFile[0].path;

            return getFileHash(savedPath).then(res => {
              expect(res).equals('ab66679562f0b2326e21409870fb4e3c51d8c9b8');
              fs.unlinkSync(savedPath);
              resolve();
            });
          });
          var form = req.form();

          const filepath = path.resolve(__dirname, '../files/randfiles/1M.bin');
          form.append('file', fs.createReadStream(filepath));
        });
      });
  });

});
