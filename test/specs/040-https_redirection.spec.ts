import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';
import { bproxy, runNode, killAll } from '../utils/process';
import { writeConfig, tempDir, delay, compareFiles } from '../utils/helpers';
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
      "port": 4901,
      "ssl_passthrough": true,
      "force_ssl": true
    }
  ]
};

describe('Https redirection', () => {
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
      .catch(err => console.error(err));
  });

  beforeEach(() => {
    return Promise.resolve()
      .then(() => process.chdir(path.resolve(cwd, 'test/files')))
      .then(() => runNode(false, ['./bproxy-testing-website/dist-app/api/index_ssl.js']))
      .then(() => runNode(false, ['./bproxy-testing-website/dist-app/api/index.js']))
      .then(() => process.chdir(cwd))
      .catch(err => console.error(err));
  });

  afterEach(() => killAll().catch(err => console.error(err)));

  it(`should redirect to https for ssl_passthrough and force_ssl enabled (http://localhost:8080/)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => delay(500))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;
          const formData = {
            hello: 'world',
            how: 'are you',
            great: 'is this via bproxy?',
            yes: 'it is'
          };
          request.get('http://localhost:8080/', { gzip: true, strictSSL: false, followRedirect: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }
            expect(response.statusCode).to.equal(301);
            expect(response.headers['location']).to.contain('https');

            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      })
      .catch(err => console.error(err));
  });

  it(`should redirect to https for ssl_passthrough enabled (http://localhost:8080/)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => config.proxies[0].force_ssl = false)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => delay(500))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;
          request.get('http://localhost:8080/', { gzip: true, strictSSL: false, followRedirect: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }
            expect(response.statusCode).to.equal(301);
            expect(response.headers['location']).to.contain('https');

            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      })
      .catch(err => console.error(err));
  });

  it(`should redirect to https for force_ssl enabled (http://localhost:8080/)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => config.proxies[0].force_ssl = true)
      .then(() => config.proxies[0].ssl_passthrough = false)
      .then(() => config.proxies[0].port = 4900)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => delay(500))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;
          request.get('http://localhost:8080/', { gzip: true, strictSSL: false, followRedirect: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }
            expect(response.statusCode).to.equal(301);
            expect(response.headers['location']).to.contain('https');

            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      })
      .catch(err => console.error(err));
  });

  it(`should not redirect to https (http://localhost:8080/)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => config.proxies[0].force_ssl = false)
      .then(() => config.proxies[0].ssl_passthrough = false)
      .then(() => config.proxies[0].port = 4900)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => delay(500))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;
          request.get('http://localhost:8080/', { gzip: true, strictSSL: false, followRedirect: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }
            expect(response.statusCode).to.equal(200);

            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      })
      .catch(err => console.error(err));
  });
});
