import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';
import { bproxy, runNode, killAll } from '../utils/process';
import { writeConfig, tempDir, sendRequest } from '../utils/helpers';
import * as path from 'path';
import * as request from 'request';
import { exec } from 'child_process';

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
      "port": 4000
    }
  ]
};

describe('HTTPS Website', () => {
  before(() => {
    return Promise.resolve()
      .then(() => {
        return new Promise((resolve, reject) => {
          const command = `/bin/bash ${path.resolve(__dirname, '../certs/make_certs.sh')}`;
          exec(command, (error, stdout, stderr) => {
            if (error) {
              reject(error);
            } else {
              resolve();
            }
          });
        });
      });
  })
  beforeEach(() => {
    return Promise.resolve()
      .then(() => process.chdir(path.resolve(cwd, 'test/files')))
      .then(() => runNode(false, ['./dist/server.js']))
      .then(() => process.chdir(cwd));
  })
  afterEach(() => killAll());

  it(`should return 200 response based on https://localhost:8081 request`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => sendRequest('https://localhost:8081/', { strictSSL: false }))
      .then(resp => {
        expect(resp).to.have.status(200);
        expect(resp).to.be.html;
        expect(resp).to.not.redirect;
        expect(resp.body).to.contains('App Works!');
      });
  });

  it(`should listen on port specified in configuration (https://localhost:11220)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.secure_port = 11220)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => sendRequest('https://localhost:11220/', { strictSSL: false }))
      .then(resp => {
        expect(resp).to.have.status(200);
        expect(resp).to.be.html;
        expect(resp).to.not.redirect;
        expect(resp.body).to.contains('App Works!');
      });
  });

  it(`should return non-gzipped response (https://localhost:8081/js/app.bundle.js)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.secure_port = 8081)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => sendRequest('https://localhost:8081/js/app.bundle.js', { strictSSL: false }))
      .then(res => {
        const resp = res.toJSON();
        expect(resp.statusCode).to.equal(200);
        expect(resp.headers['content-type']).to.includes('application/javascript');
        expect(resp.body.length).to.be.greaterThan(100000);
      });
  });

  it(`should return gzipped response (https://localhost:8081/js/app.bundle.js)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.secure_port = 8081)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;

          request.get('https://localhost:8081/js/app.bundle.js', { gzip: true, strictSSL: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }

            expect(response.headers['content-encoding']).to.equal('gzip');
            expect(len).to.be.greaterThan(50000);
            expect(len).to.be.lessThan(100000);
            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      });
  });

  it(`should not return gzipped response for when mime type is not listed in config file (https://localhost:8081/js/app.bundle.js)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;

          request.get('https://localhost:8081/js/app.bundle.js', { gzip: true, strictSSL: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }

            expect(response.headers['content-encoding']).to.not.equal('gzip');
            expect(len).to.be.greaterThan(100000);
            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      });
  });

  it(`should return gzipped response for mime type ["text/css"] setted in config file (https://localhost:8081/css/app.css)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = ["text/css"])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;

          request.get('https://localhost:8081/css/app.css', { gzip: true, strictSSL: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }

            expect(response.headers['content-encoding']).to.equal('gzip');
            expect(len).to.be.equal(58);
            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      });
  });

  it(`should not return gzipped response for css when mime type is not setted in config file (https://localhost:8081/css/app.css)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;

          request.get('https://localhost:8081/css/app.css', { gzip: true, strictSSL: false }, function (error, response, body) {
            if (error) {
              reject(error);
            }

            expect(response.headers['content-encoding']).to.not.equal('gzip');
            expect(len).to.not.equal(58);
            resolve();
          })
            .on('response', response => response.on('data', data => len += data.length));
        });
      });
  });

  it(`should hang up connection when there is no "proxies" entry in config file (https://localhost:8081)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => delete config.proxies)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => new Promise((resolve, reject) => {
        const url = 'https://localhost:8081/css/app.css';
        const options = Object.assign({}, { url }, { strictSSL: false });
        request.get(options, function (err, res, body) {
          if (err) {
            if (err.code == 'ECONNRESET') {
              resolve();
            } else {
              reject();
            }
          } else {
            reject();
          }
        });
      }));
  });

});
