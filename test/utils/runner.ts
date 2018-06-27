import * as Mocha from 'mocha';
import * as glob from 'glob';
import * as path from 'path';
import { execSilent, killAll } from '../utils/process';

const specFiles = glob.sync(path.resolve(__dirname, '../specs/*.spec.*'));
const mo = new Mocha({ timeout: 100000, reporter: 'spec' });

Promise.resolve()
  .then(() => execSilent('npm',  ['run', 'build']))
  .then(() => {
    specFiles.forEach(file => mo.addFile(file));
    mo.run(failures => {
      killAll().then(() => process.exit(failures));
    });
  });
