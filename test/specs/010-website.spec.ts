import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';
import { bproxy, runNode, killAll } from '../utils/process';
import { writeConfig, tempDir, sendRequest } from '../utils/helpers';
import * as path from 'path';
import * as request from 'request';

chai.use(chaiAsPromised);
chai.use(require('chai-http'));

const expect = chai.expect;
const cwd = process.cwd();
let configPath = null;
let config = {
  "port": 8080,
  "gzip_mime_types": ["text/css", "application/javascript", "application/x-javascript"],
  "proxies": [
    {
      "hosts": ["localhost"],
      "ip": "127.0.0.1",
      "port": 4000
    }
  ]
};

describe('Website', () => {

  beforeEach(() => {
    return Promise.resolve()
      .then(() => process.chdir(path.resolve(cwd, 'test/files')))
      .then(() => runNode(false, ['./dist/server.js']))
      .then(() => process.chdir(cwd));
  })
  afterEach(() => killAll());

  it(`should return 200 response based on http://localhost:8080 request`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => chai.request('http://localhost:8080').get('/'))
      .then(resp => {
        expect(resp).to.have.status(200);
        expect(resp).to.be.html;
        expect(resp).to.not.redirect;
        expect(resp.text).to.contains('App Works!');
      });
  });

  it(`should listen on port specified in configuration (http://localhost:11220)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.port = 11220)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => chai.request('http://localhost:11220').get('/'))
      .then(resp => {
        expect(resp).to.have.status(200);
        expect(resp).to.be.html;
        expect(resp).to.not.redirect;
        expect(resp.text).to.contains('App Works!');
      });
  });

  it(`should return non-gzipped response (http://localhost:8080/js/app.bundle.js)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.port = 8080)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => sendRequest('http://localhost:8080/js/app.bundle.js'))
      .then(res => {
        const resp = res.toJSON();
        expect(resp.statusCode).to.equal(200);
        expect(resp.headers['content-type']).to.includes('application/javascript');
        expect(resp.body.length).to.be.greaterThan(100000);
      });
  });

  it(`should return gzipped response (http://localhost:8080/js/app.bundle.js)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.port = 8080)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;

          request.get('http://localhost:8080/js/app.bundle.js', { gzip: true }, function(error, response, body) {
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

  it(`should not return gzipped response for when mime type is not listed in config file (http://localhost:8080/js/app.bundle.js)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;

          request.get('http://localhost:8080/js/app.bundle.js', { gzip: true }, function(error, response, body) {
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

  it(`should return gzipped response for mime type ["text/css"] setted in config file (http://localhost:8080/css/app.css)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = ["text/css"])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;

          request.get('http://localhost:8080/css/app.css', { gzip: true }, function(error, response, body) {
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

  it(`should not return gzipped response for css when mime type is not setted in config file (http://localhost:8080/css/app.css)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.gzip_mime_types = [])
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(res => {
        return new Promise((resolve, reject) => {
          let len = 0;

          request.get('http://localhost:8080/css/app.css', { gzip: true }, function(error, response, body) {
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

  it(`should return 502 response when there is no server listening on port 65535 (http://localhost:8080)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => config.proxies[0].port = 65535)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => sendRequest('http://localhost:8080'))
      .then(res => {
        const resp = res.toJSON();
        expect(resp.statusCode).to.equal(502);
        expect(resp.body).to.contains('502 Bad Gateway');
      });
  });

  it(`should return 404 response when there is no "proxies" entry in config file (http://localhost:8080)`, () => {
    return tempDir()
      .then(dir => configPath = path.join(dir, 'bproxy.json'))
      .then(() => delete config.proxies)
      .then(() => writeConfig(configPath, config))
      .then(() => bproxy(false, ['-c', configPath]))
      .then(() => sendRequest('http://localhost:8080'))
      .then(res => {
        const resp = res.toJSON();
        expect(resp.statusCode).to.equal(404);
        expect(resp.body).to.contains('404 Not Found');
      });
  });

});
